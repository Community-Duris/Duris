#include "item/held_retirement_recovery.h"
#include "item/item_movement_transaction.h"
#include "item/held_retirement_transport.h"
#include "item/native_quest_transport.h"
#include "persistence/death_recovery_visibility.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/persistence_diagnostics.h"
#include "player/player_save_pipeline.h"
#include "persistence/persistence_mode.h"
#include "economy/shop_trade_accounting.h"
#include "economy/native_mobile_birth_command.h"
#include "economy/native_mobile_birth_result.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#include "economy/native_mobile_birth_cash_role_result.h"
#include "economy/native_mobile_birth_recovery.h"
#include "flatfile/flatfile_accounting_native_mobile_birth_shared_shop_transaction.h"
#include "flatfile/flatfile_accounting_native_mobile_birth_ordinary_transaction.h"
#include "world/quest_mobile_native_birth.h"
#include "flatfile/flatfile_accounting_zone_reset_item_transaction.h"
#include "economy/zone_reset_item_command.h"
#include "item/item_transfer_command.h"
#include "world/native_quest_recovery_context.h"
#include "item/quest_reward_continuation.h"
#include "economy/auction_native_command_context.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <cerrno>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// This global class is the header's exact private friend. The original worker
// owns its methods; no public caller can construct an execution handle.
class critical_shared_native_execution_dispatch final
{
    public:
	static void worker_main();
	static void cancel_ordinary_waits() noexcept;
};

// Exact global private friend of the actual shared budget scope. The lender
// never infers ownership from thread identity or a nonzero cached byte value.
class critical_room_shared_budget_lender final
{
    public:
	static bool reserve(const std::unique_lock<std::mutex> &actual_lock,
			    bool (*actual_reserve)(size_t, void *) noexcept, void *actual_guard,
			    size_t exclusive_live, size_t *current_output = nullptr) noexcept;
	static bool reset_before_replay(const std::unique_lock<std::mutex> &actual_lock) noexcept;

	using operation_callback = bool (*)(bool (*)(size_t, void *) noexcept, void *, size_t,
					    void *) noexcept;
	static bool invoke(const std::unique_lock<std::mutex> &, bool (*)(size_t, void *) noexcept,
			   void *, size_t, operation_callback, void *) noexcept;
	static size_t operation_frame_bytes() noexcept;
	static bool callback_matches(const std::unique_lock<std::mutex> &,
				     bool (*)(size_t, void *) noexcept, void *) noexcept;
	static constexpr size_t retained_storage_bytes() noexcept
	{
		return sizeof(active_operation_);
	}

    private:
	struct operation_borrow_frame;
	struct callback_borrow_frame;
	static operation_borrow_frame *active_operation_;
};

struct critical_room_shared_budget_lender::callback_borrow_frame
{
	const std::unique_lock<std::mutex> &lock;
	bool (*reserve)(size_t, void *) noexcept;
	void *guard;
	size_t *output;
	size_t current = 0, full = 0;
	bool borrowed = false, accepted = false, ended = false;
};

// A genuine stack object remains registered across the complete native operation.
// Access to active_operation_ is serialized by the original coordinator mutex.
struct critical_room_shared_budget_lender::operation_borrow_frame
{
	const std::unique_lock<std::mutex> &lock;
	bool (*reserve)(size_t, void *) noexcept;
	void *guard;
	operation_callback operation;
	void *operation_context;
	size_t outer, current = 0;
	bool accepted = false, ended = false;
	static bool relay(size_t bytes, void *context) noexcept
	{
		auto &scope = *static_cast<operation_borrow_frame *>(context);
		return critical_room_shared_budget_lender::reserve(scope.lock, scope.reserve,
								   scope.guard, bytes);
	}
};
critical_room_shared_budget_lender::operation_borrow_frame
	*critical_room_shared_budget_lender::active_operation_ = nullptr;

namespace
{
// Actual scoped bridge, not a lock flag or public permit. Every relay proves
// ownership through its original unique_lock before lending current storage.
struct room_locked_budget_callback
{
	const std::unique_lock<std::mutex> &lock;
	bool (*actual_reserve)(size_t, void *) noexcept;
	void *actual_guard;
	static bool relay(size_t exclusive_live, void *opaque) noexcept
	{
		auto &callback = *static_cast<room_locked_budget_callback *>(opaque);
		return critical_room_shared_budget_lender::reserve(callback.lock,
								   callback.actual_reserve,
								   callback.actual_guard,
								   exclusive_live);
	}
};

// Pure immutable classification only. This grants neither live publication
// nor reservation authority; only the private pipeline owner can consume it.
bool accounted_shop_publication(const critical_command &command) noexcept
{
	try
	{
		economic_frozen_intent intent;
		shop_trade_payload payload = {};
		economic_account_key wallet, bank, counterparty;
		return command.publication_required &&
		       shop_trade_payload_version_is_accounted(command.payload_version) &&
		       shop_trade_accounting_decode(command, &intent, &payload, &wallet, &bank,
						    &counterparty) == economic_accounting_error::ok;
	}
	catch (...)
	{
		return false;
	}
}

bool guarded_refusal_owner(const critical_command &command) noexcept
{
	// These retained commands already passed immutable admission validation.
	// Retention/cache decisions must not erase an owner when decode allocation
	// fails. Conservative shape retains the original; private cancellation still
	// verifies the full immutable intent and exact canonical refusal receipt.
	return held_retirement_transport_command(command) ||
	       (command.publication_required &&
		command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		command.type == critical_command_type::auction &&
		command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION) ||
	       command.type == critical_command_type::collector ||
	       (command.publication_required &&
		command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		command.type == critical_command_type::item_transfer &&
		native_quest_transport_command(command)) ||
	       (command.publication_required &&
		command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		command.type == critical_command_type::shop_trade &&
		shop_trade_payload_version_is_accounted(command.payload_version));
}

// A birth has its own retained constructor owner, never a player-save token.
// Admission has already validated the full command; conservative shape avoids
// allocating while deciding whether a genuine refusal owner must be retained.
bool retained_admission_refusal_owner(const critical_command &command) noexcept
{
	const bool birth =
		command.publication_required &&
		command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		command.type == critical_command_type::native_mobile_birth &&
		(native_mobile_birth_payload_version_supported(command.payload_version) ||
		 command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION);
	const bool reset = command.publication_required &&
			   command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			   command.type == critical_command_type::zone_reset_item_birth &&
			   (command.payload_version == ZONE_RESET_ITEM_PAYLOAD_VERSION ||
			    command.payload_version == ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION);
	return birth || reset ||
	       (command.publication_required &&
		command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		command.type == critical_command_type::auction &&
		command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION) ||
	       (guarded_refusal_owner(command) &&
		player_save_execution_guard::publication_operation_held(command.operation_id));
}

enum class critical_operation_phase : uint8_t
{
	awaiting_durability,
	queued,
	executing,
	uncertain_admission,
	blocked,
	admission_failed,
	publication_pending,
	native_continuation_pending,
};

struct native_operation_context
{
	uint64_t revision = 0;
	critical_native_recovery_phase phase = critical_native_recovery_phase::execution_pending;
	std::vector<uint8_t> attachment;
};

struct operation_state
{
	critical_command command;
	std::unique_ptr<native_operation_context> native;
	bool native_context_uncertain = false;
	bool native_ack_uncertain = false;
	bool native_physical_released = false;
	const critical_ordinary_native_execution_owner *ordinary_execution = nullptr;
	std::unique_ptr<flatfile_accounting_native_mobile_birth_ordinary_transaction>
		ordinary_flat_transaction;
	size_t ordinary_flat_transaction_bytes = 0;
	const critical_shared_native_execution_owner *shared_execution = nullptr;
	std::unique_ptr<flatfile_accounting_native_mobile_birth_shared_shop_transaction>
		flat_transaction;
	size_t flat_transaction_bytes = 0;
	const critical_zone_reset_item_execution_owner *room_execution = nullptr;
	std::unique_ptr<flatfile_accounting_zone_reset_item_transaction> room_flat_transaction;
	size_t room_flat_transaction_bytes = 0;
	size_t retained_bytes;
	uint64_t queued_at_usec;
	unsigned int attempt;
	uint64_t attachments;
	critical_operation_phase phase;
	bool retain_until_publication;
	bool publication_checkpointing = false;
	bool admission_failure_queued;
	bool owned_refusal_delivered = false;
	critical_completion admission_failure_completion;
	critical_completion publication_completion;
};

struct completed_state
{
	critical_command command;
	critical_completion completion;
	size_t encoded_size;
};

const critical_mixed_startup_replay_owner *active_mixed_startup_owner = nullptr;

struct replay_observer_context
{
	critical_replay_observer_fn observer;
	void *context;
	critical_native_recovery_observer_fn native_observer;
	critical_native_recovery_observer_bounded_fn native_bounded_observer = nullptr;
};

std::mutex coordinator_mutex;
std::condition_variable work_available;
std::condition_variable result_available;
std::condition_variable admission_available;
std::condition_variable publication_checkpoint_finished;
size_t publication_checkpoints_inflight = 0;
size_t guarded_publications_inflight = 0;
// Actual queue objects retain the original std::deque algorithms and allocator.
// The data-free owner can inspect the protected grandparent on the pinned ABI;
// no existing deque is cast to a different object or interpreted by guessed layout.
class native_identity_queue final : public std::deque<std::string>
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using storage_base = std::_Deque_base<std::string, std::allocator<std::string>>;
#endif
    public:
	using std::deque<std::string>::deque;
	using std::deque<std::string>::operator=;
	native_identity_queue() = default;
	native_identity_queue(const native_identity_queue &) = default;
	native_identity_queue(native_identity_queue &&) = default;
	native_identity_queue &operator=(const native_identity_queue &) = default;
	native_identity_queue &operator=(native_identity_queue &&) = default;
	bool current_heap_bytes(size_t *output) const noexcept
	{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		if (!output)
			return false;
		const auto &actual =
			this->std::_Deque_base<std::string, std::allocator<std::string>>::_M_impl;
		if (!actual._M_map || !actual._M_map_size ||
		    actual._M_map_size > SIZE_MAX / sizeof(std::string *))
			return false;
		size_t bytes = actual._M_map_size * sizeof(std::string *);
		const size_t blocks =
			static_cast<size_t>(actual._M_finish._M_node - actual._M_start._M_node) + 1;
		const size_t block_bytes =
			std::__deque_buf_size(sizeof(std::string)) * sizeof(std::string);
		if (blocks > (SIZE_MAX - bytes) / block_bytes)
			return false;
		bytes += blocks * block_bytes;
		for (const auto &identity : *this)
			if (identity.capacity() > 15)
			{
				if (identity.capacity() == SIZE_MAX ||
				    identity.capacity() + 1 > SIZE_MAX - bytes)
					return false;
				bytes += identity.capacity() + 1;
			}
		*output = bytes;
		return true;
#else
		(void)output;
		return false;
#endif
	}
	static bool initial_heap_bytes(size_t *output) noexcept
	{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		if (!output)
			return false;
		// The actual original default constructor initializes zero elements.
		*output = storage_base::_S_initial_map_size * sizeof(std::string *) +
			  std::__deque_buf_size(sizeof(std::string)) * sizeof(std::string);
		return true;
#else
		(void)output;
		return false;
#endif
	}
	bool push_back_extra_peak(const std::string &identity, size_t *output) const noexcept
	{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		if (!output || size() == max_size() || identity.size() == SIZE_MAX)
			return false;
		const auto &actual =
			this->std::_Deque_base<std::string, std::allocator<std::string>>::_M_impl;
		if (!actual._M_map || !actual._M_map_size)
			return false;
		const size_t text = identity.size() > 15 ? identity.size() + 1 : 0;
		if (actual._M_finish._M_cur != actual._M_finish._M_last - 1)
		{
			*output = text;
			return true;
		}
		const size_t block =
			std::__deque_buf_size(sizeof(std::string)) * sizeof(std::string);
		if (text > SIZE_MAX - block)
			return false;
		size_t peak = block + text;
		const size_t finish_index =
			static_cast<size_t>(actual._M_finish._M_node - actual._M_map);
		if (finish_index >= actual._M_map_size)
			return false;
		if (actual._M_map_size - finish_index < 2)
		{
			const size_t nodes = static_cast<size_t>(actual._M_finish._M_node -
								 actual._M_start._M_node) +
					     1;
			if (nodes == SIZE_MAX || nodes + 1 > SIZE_MAX / 2)
				return false;
			// Original _M_reallocate_map first repositions when the actual map suffices.
			if (actual._M_map_size <= 2 * (nodes + 1))
			{
				if (actual._M_map_size > (SIZE_MAX - 2) / 2)
					return false;
				const size_t new_map = 2 * actual._M_map_size + 2;
				if (new_map > SIZE_MAX / sizeof(std::string *))
					return false;
				const size_t request = new_map * sizeof(std::string *);
				const size_t old_map = actual._M_map_size * sizeof(std::string *);
				if (request - old_map > SIZE_MAX - peak)
					return false;
				// New map allocation coexists with the old one. Old map dies before the
				// new block and copied string are allocated; retain the larger real peak.
				peak = std::max(request, request - old_map + peak);
			}
		}
		*output = peak;
		return true;
#else
		(void)identity;
		(void)output;
		return false;
#endif
	}
};
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && !defined(_GLIBCXX_DEBUG)
static_assert(sizeof(native_identity_queue) == sizeof(std::deque<std::string>));
#endif

// The supported original unordered_map owns this exact underlying table.
// A genuinely new data-free owner exposes its real public policy accessor;
// no existing unordered_map is cast, mirrored or interpreted by layout.
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
template <typename Value, typename Hash = std::hash<std::string>,
	  typename Equal = std::equal_to<std::string>>
class native_admission_string_table final
	: public std::__umap_hashtable<std::string, Value, Hash, Equal>
{
	using table_type = std::__umap_hashtable<std::string, Value, Hash, Equal>;
	using original_type = std::unordered_map<std::string, Value, Hash, Equal>;
	using actual_node =
		std::__detail::_Hash_node<typename table_type::value_type,
					  std::__cache_default<std::string, Hash>::value>;

    public:
	using table_type::table_type;
	using table_type::operator=;
	using table_type::find;
	native_admission_string_table() = default;
	native_admission_string_table(const native_admission_string_table &) = default;
	native_admission_string_table(native_admission_string_table &&) = default;
	native_admission_string_table &operator=(const native_admission_string_table &) = default;
	native_admission_string_table &operator=(native_admission_string_table &&) = default;
	// Exact original unordered_map transparent lookup forwarding. All ordinary
	// key lookup/mutation algorithms are inherited from the actual original table.
	template <typename Lookup>
	auto find(const Lookup &key) -> decltype(std::declval<table_type &>()._M_find_tr(key))
	{
		return this->_M_find_tr(key);
	}
	template <typename Lookup> auto find(const Lookup &key) const
		-> decltype(std::declval<const table_type &>()._M_find_tr(key))
	{
		return this->_M_find_tr(key);
	}
	bool current_table_heap_bytes(size_t *output) const noexcept
	{
		if (!output)
			return false;
		size_t bytes = 0;
		// The actual table's bucket_count==1 uses its inline single bucket.
		const size_t buckets = this->bucket_count();
		if (!buckets ||
		    (buckets > 1 && buckets > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *)))
			return false;
		if (buckets > 1)
			bytes = buckets * sizeof(std::__detail::_Hash_node_base *);
		if (this->size() > (SIZE_MAX - bytes) / sizeof(actual_node))
			return false;
		bytes += this->size() * sizeof(actual_node);
		for (const auto &entry : *this)
			if (entry.first.capacity() > 15)
			{
				if (entry.first.capacity() == SIZE_MAX ||
				    entry.first.capacity() + 1 > SIZE_MAX - bytes)
					return false;
				bytes += entry.first.capacity() + 1;
			}
		// Nested mapped-value ownership is separately inspected by its real owner.
		*output = bytes;
		return true;
	}
	bool next_unique_insert_extra_peak(const std::string &key, size_t fresh_mapped_heap,
					   size_t *output) const noexcept
	{
		if (!output || this->size() == this->max_size() || key.size() == SIZE_MAX)
			return false;
		const size_t text = key.size() > 15 ? key.size() + 1 : 0;
		if (text > SIZE_MAX - sizeof(actual_node) ||
		    fresh_mapped_heap > SIZE_MAX - sizeof(actual_node) - text)
			return false;
		size_t extra = sizeof(actual_node) + text + fresh_mapped_heap;
		try
		{
			// Copy the genuine CURRENT policy. Its original _M_need_rehash owns
			// exact resize thresholds/primes; the real table state remains unchanged.
			auto policy = this->__rehash_policy();
			const auto next =
				policy._M_need_rehash(this->bucket_count(), this->size(), 1);
			if (next.first && next.second > 1)
			{
				if (next.second >
				    (SIZE_MAX - extra) / sizeof(std::__detail::_Hash_node_base *))
					return false;
				// Original node/key/default mapped construction precedes rehash.
				// Entire new bucket array coexists with old table ownership until
				// _M_rehash_aux finishes; do not subtract old buckets at this peak.
				extra += next.second * sizeof(std::__detail::_Hash_node_base *);
			}
			*output = extra;
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
};
#else
template <typename Value, typename Hash = std::hash<std::string>,
	  typename Equal = std::equal_to<std::string>>
using native_admission_string_table = std::unordered_map<std::string, Value, Hash, Equal>;
#endif

native_admission_string_table<std::unique_ptr<operation_state>> operations;
native_identity_queue pending;
native_identity_queue pending_admission;
critical_completion_delivery completion_delivery;
struct entity_key_hash
{
	using is_transparent = void;
	size_t operator()(std::string_view key) const noexcept
	{
		return std::hash<std::string_view>{}(key);
	}
};
std::unordered_map<std::string, std::string, entity_key_hash, std::equal_to<>> active_keys;
native_admission_string_table<native_identity_queue, entity_key_hash, std::equal_to<>> fences;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
static_assert(sizeof(decltype(operations)) ==
	      sizeof(std::unordered_map<std::string, std::unique_ptr<operation_state>>));
static_assert(sizeof(decltype(fences)) ==
	      sizeof(std::unordered_map<std::string, native_identity_queue, entity_key_hash,
					std::equal_to<>>));
#endif
std::unordered_map<std::string, completed_state> completed_cache;
native_identity_queue completed_order;
size_t completed_cache_bytes = 0;
size_t pending_admission_bytes = 0;
size_t admission_inflight_bytes = 0;
std::vector<std::thread> workers;
std::thread admission_worker;
critical_apply_fn apply_callback = nullptr;
critical_shared_native_apply_fn shared_native_apply_callback = nullptr;
critical_ordinary_native_apply_fn ordinary_native_apply_callback = nullptr;
critical_zone_reset_item_apply_fn zone_reset_apply_callback = nullptr;
critical_extension_validator_fn extension_validator_callback = nullptr;
critical_extension_validator_bounded_fn extension_validator_bounded_callback = nullptr;
critical_native_recovery_observer_fn native_replay_observer_callback = nullptr;
critical_native_recovery_observer_bounded_fn native_replay_observer_bounded_callback = nullptr;
critical_native_recovery_publication_validator_fn native_publication_validator_callback = nullptr;
critical_native_birth_recovery_validators native_birth_validators;
critical_native_recovery_pair_validator_fn native_quest_pair_validator = nullptr;
critical_native_auction_recovery_validators native_auction_validators = {};
critical_zone_reset_recovery_validators zone_reset_validators = {};
void *apply_context = nullptr;
critical_drain_observer_fn drain_observer = nullptr;
critical_coordinator_health health = {};
bool stop_requested = false;
bool lifecycle_guard_active = false;
bool lifecycle_guard_was_accepting = false;
bool lifecycle_guard_initialized_runtime = false;
std::thread::id lifecycle_guard_thread;
bool recovery_requested = false;
uint64_t uncertain_recovery_not_before_usec = 0;
uint64_t uncertain_recovery_delay_usec = 1000000;
uint64_t coordinator_generation = 0;
bool coordinator_generation_exhausted = false;
uint64_t next_cutover_lease_id = 1;
bool cutover_lease_ids_exhausted = false;
uint64_t active_cutover_generation = 0;
uint64_t active_cutover_lease_id = 0;
enum class cutover_owner_phase : uint8_t
{
	none,
	issuing,
	lease_idle,
	transaction_active,
};
cutover_owner_phase active_cutover_phase = cutover_owner_phase::none;
std::thread::id active_cutover_thread;
const void *active_cutover_connection = nullptr;
unsigned long active_cutover_session = 0;
bool cutover_reopen_allowed = false;
bool cutover_was_accepting = false;
bool cutover_outcome_uncertain = false;
// Minted only by the real runtime reservation transfer; never caller values.
bool cutover_runtime_origin = false;
bool cutover_runtime_was_accepting = false;

void update_depth();

uint64_t now_usec()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
					     std::chrono::steady_clock::now().time_since_epoch())
					     .count());
}

uint64_t wall_now_usec()
{
	return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
					     std::chrono::system_clock::now().time_since_epoch())
					     .count());
}

void advance_coordinator_generation()
{
	if (coordinator_generation_exhausted)
		return;
	if (coordinator_generation == UINT64_MAX)
	{
		coordinator_generation = 0;
		coordinator_generation_exhausted = true;
		return;
	}
	++coordinator_generation;
}

void invalidate_active_cutover_lease()
{
	active_cutover_generation = 0;
	active_cutover_lease_id = 0;
	active_cutover_phase = cutover_owner_phase::none;
	active_cutover_thread = {};
	active_cutover_connection = nullptr;
	active_cutover_session = 0;
	cutover_reopen_allowed = false;
	cutover_was_accepting = false;
	cutover_outcome_uncertain = false;
	cutover_runtime_origin = false;
	cutover_runtime_was_accepting = false;
}

void abandon_issuing_cutover_locked()
{
	if (active_cutover_phase != cutover_owner_phase::issuing)
		return;
	invalidate_active_cutover_lease();
	// An unsuccessful owned acquisition keeps the coordinator quiesced, as
	// before; only explicit resume can reopen it after no owner remains.
	health.accepting = false;
	update_depth();
}

void defer_uncertain_recovery()
{
	recovery_requested = true;
	const uint64_t now = now_usec();
	uncertain_recovery_not_before_usec = now + uncertain_recovery_delay_usec;
	uncertain_recovery_delay_usec =
		std::min<uint64_t>(uncertain_recovery_delay_usec * 2, 30000000);
}

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

std::array<char, 9> entity_key_bytes(const critical_entity_key &key) noexcept
{
	std::array<char, 9> encoded = {};
	encoded[0] = static_cast<char>(key.type);
	for (unsigned int index = 0; index < 8; ++index)
		encoded[index + 1] = static_cast<char>(key.id >> (index * 8));
	return encoded;
}

std::string entity_key(const critical_entity_key &key)
{
	const auto encoded = entity_key_bytes(key);
	return std::string(encoded.data(), encoded.size());
}

constexpr size_t NATIVE_COORDINATOR_ENVELOPE_OVERHEAD = 60; // CCJ header40 + recovery prefix20.

bool native_v12_command(const critical_command &command) noexcept
{
	return command.type == critical_command_type::item_transfer &&
	       command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       command.payload_version == 12;
}

bool native_fee_context_command(const critical_command &command) noexcept
{
	return command.type == critical_command_type::item_transfer &&
	       command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       command.payload_version == ITEM_TRANSFER_NATIVE_MOBILE_COST_RECOVERY_PAYLOAD_VERSION;
}

bool native_quest_context_command(const critical_command &command) noexcept
{
	return native_quest_transport_command(command);
}

bool zone_reset_typed_command(const critical_command &command) noexcept
{
	return command.publication_required &&
	       command.type == critical_command_type::zone_reset_item_birth &&
	       command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       (command.payload_version == ZONE_RESET_ITEM_PAYLOAD_VERSION ||
		command.payload_version == ZONE_RESET_ITEM_PLACEMENT_PAYLOAD_VERSION);
}
bool zone_reset_validators_ready() noexcept
{
	return zone_reset_validators.valid && zone_reset_validators.initial &&
	       zone_reset_validators.successor && zone_reset_validators.publication &&
	       zone_reset_validators.terminal;
}

bool native_birth_typed_command(const critical_command &command) noexcept
{
	return command.type == critical_command_type::native_mobile_birth &&
	       command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       (command.payload_version == NATIVE_MOBILE_BIRTH_RECIPE_PAYLOAD_VERSION ||
		command.payload_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION ||
		command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION);
}

// Both constructor families retain their original advancement fences until
// authentic terminal origin transfer and journal retirement have succeeded.
bool native_birth_origin_command(const critical_command &command) noexcept
{
	return command.type == critical_command_type::native_mobile_birth &&
	       (command.payload_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_PAYLOAD_VERSION ||
		command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION);
}

// Conservative family selection only: malformed NMB4+SHOP remains on the
// shared path and must pass its complete original carrier codec. This grants
// no execution or source authority and never falls back to ordinary decoding.
bool native_birth_shared_shop_command(const critical_command &command) noexcept
{
	return native_birth_typed_command(command) &&
	       command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
	       std::any_of(command.keys.begin(), command.keys.end(), [](const auto &key)
			   { return key.type == critical_entity_type::shopkeeper; });
}

// Historical registered callbacks and their readiness prerequisite remain
// unchanged. Every selected role requires its full immutable recovery codec.
bool native_birth_recovery_valid(const critical_native_recovery_envelope &e) noexcept
{
	if (native_birth_shared_shop_command(e.command))
		return native_mobile_birth_shared_shop_recovery_valid(e);
	return e.command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ?
		       native_mobile_birth_cash_role_recovery_valid(e) :
		       native_birth_validators.valid(e);
}
bool native_birth_recovery_initial(const critical_native_recovery_envelope &e) noexcept
{
	if (native_birth_shared_shop_command(e.command))
		return native_mobile_birth_shared_shop_recovery_initial(e);
	return e.command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ?
		       native_mobile_birth_cash_role_recovery_initial(e) :
		       native_birth_validators.initial(e);
}
bool native_birth_recovery_successor(const critical_native_recovery_envelope &e,
				     const critical_native_recovery_envelope &next) noexcept
{
	if (native_birth_shared_shop_command(e.command))
		return native_mobile_birth_shared_shop_recovery_successor(e, next);
	return e.command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ?
		       native_mobile_birth_cash_role_recovery_successor(e, next) :
		       native_birth_validators.successor(e, next);
}
bool native_birth_recovery_publication(const critical_native_recovery_envelope &e,
				       const critical_completion &receipt) noexcept
{
	if (native_birth_shared_shop_command(e.command))
		return native_mobile_birth_shared_shop_recovery_publication(e, receipt);
	return e.command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ?
		       native_mobile_birth_cash_role_recovery_publication(e, receipt) :
		       native_birth_validators.publication(e, receipt);
}
bool native_birth_recovery_terminal(const critical_native_recovery_envelope &e) noexcept
{
	if (native_birth_shared_shop_command(e.command))
		return native_mobile_birth_shared_shop_recovery_terminal(e);
	return e.command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ?
		       native_mobile_birth_cash_role_recovery_terminal(e) :
		       native_birth_validators.terminal(e);
}

bool native_auction_typed_command(const critical_command &command) noexcept
{
	return command.type == critical_command_type::auction &&
	       command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION;
}

bool native_auction_background_command(const critical_command &command) noexcept
{
	auction_native_command_context image;
	return native_auction_typed_command(command) &&
	       auction_native_command_decode(command, &image) == economic_accounting_error::ok &&
	       !image.payload.actor_pid &&
	       (image.payload.action == auction_action::finalize ||
		image.payload.action == auction_action::remove);
}

bool native_auction_validators_ready() noexcept
{
	return native_auction_validators.valid && native_auction_validators.initial &&
	       native_auction_validators.successor && native_auction_validators.publication &&
	       native_auction_validators.terminal;
}

bool native_transport_command(const critical_command &command) noexcept
{
	return held_retirement_transport_command(command) ||
	       native_quest_transport_command(command) || native_birth_typed_command(command) ||
	       native_auction_typed_command(command) || zone_reset_typed_command(command);
}

bool native_birth_validators_ready() noexcept
{
	return native_birth_validators.valid && native_birth_validators.initial &&
	       native_birth_validators.successor && native_birth_validators.publication &&
	       native_birth_validators.terminal;
}

bool native_receipt_equal(const critical_completion &a, const critical_completion &b) noexcept
{
	return critical_completion_disposition_valid(a) &&
	       critical_completion_disposition_valid(b) &&
	       a.operation_id.bytes == b.operation_id.bytes && a.outcome == b.outcome &&
	       a.disposition == b.disposition && a.durable_revision == b.durable_revision &&
	       a.error_code == b.error_code && a.failure_stage == b.failure_stage &&
	       a.result_size == b.result_size && a.result_payload == b.result_payload &&
	       a.attempt == b.attempt && a.queued_at_usec == b.queued_at_usec &&
	       a.started_at_usec == b.started_at_usec &&
	       a.completed_at_usec == b.completed_at_usec &&
	       a.recovery_correlation == b.recovery_correlation;
}

bool native_envelope_size(const critical_native_recovery_envelope &envelope, size_t *retained)
{
	if (!retained || !native_transport_command(envelope.command) ||
	    (held_retirement_transport_command(envelope.command) &&
	     envelope.phase != critical_native_recovery_phase::execution_pending) ||
	    !envelope.command.publication_required || !envelope.revision ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     envelope.phase != critical_native_recovery_phase::continuation_pending) ||
	    envelope.attachment.empty() ||
	    envelope.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES)
		return false;
	std::vector<uint8_t> encoded;
	if (critical_command_encode(envelope.command, &encoded) !=
		    critical_command_codec_result::ok ||
	    encoded.size() >
		    CRITICAL_COORDINATOR_MAX_BYTES - NATIVE_COORDINATOR_ENVELOPE_OVERHEAD ||
	    envelope.attachment.size() > CRITICAL_COORDINATOR_MAX_BYTES -
						 NATIVE_COORDINATOR_ENVELOPE_OVERHEAD -
						 encoded.size())
		return false;
	*retained =
		encoded.size() + envelope.attachment.size() + NATIVE_COORDINATOR_ENVELOPE_OVERHEAD;
	return true;
}

critical_native_recovery_envelope native_envelope(const operation_state &state)
{
	critical_native_recovery_envelope envelope;
	envelope.command = state.command;
	if (state.native)
	{
		envelope.revision = state.native->revision;
		envelope.phase = state.native->phase;
		envelope.attachment = state.native->attachment;
	}
	return envelope;
}

bool native_matches(const operation_state &state, const critical_native_recovery_envelope &envelope)
{
	return state.native && state.native->revision == envelope.revision &&
	       state.native->phase == envelope.phase &&
	       state.native->attachment == envelope.attachment &&
	       critical_command_equal(state.command, envelope.command);
}

bool native_envelopes_equal(const critical_native_recovery_envelope &left,
			    const critical_native_recovery_envelope &right)
{
	return left.revision == right.revision && left.phase == right.phase &&
	       left.attachment == right.attachment &&
	       critical_command_equal(left.command, right.command);
}

bool operation_is_queued(const operation_state &state)
{
	return state.phase == critical_operation_phase::queued;
}

bool operation_is_executing(const operation_state &state)
{
	return state.phase == critical_operation_phase::executing;
}

bool operation_is_publication_pending(const operation_state &state)
{
	return state.phase == critical_operation_phase::publication_pending;
}

bool operation_is_uncertain(const operation_state &state)
{
	return state.phase == critical_operation_phase::uncertain_admission;
}

bool operation_is_awaiting_durability(const operation_state &state)
{
	return state.phase == critical_operation_phase::awaiting_durability;
}

bool operation_is_blocked(const operation_state &state)
{
	return state.phase == critical_operation_phase::uncertain_admission ||
	       state.phase == critical_operation_phase::blocked ||
	       state.phase == critical_operation_phase::admission_failed;
}

bool operation_is_admission_failed(const operation_state &state)
{
	return state.phase == critical_operation_phase::admission_failed;
}

bool keys_available(const std::string &identity, const critical_command &command)
{
	for (const critical_entity_key &key : command.keys)
	{
		const std::string encoded = entity_key(key);
		auto fence = fences.find(encoded);
		if (active_keys.find(encoded) != active_keys.end() || fence == fences.end() ||
		    fence->second.empty() || fence->second.front() != identity)
			return false;
	}
	return true;
}

void acquire_keys(const std::string &identity, const critical_command &command)
{
	for (const critical_entity_key &key : command.keys)
		active_keys.emplace(entity_key(key), identity);
}

void release_keys(const std::string &identity, const critical_command &command)
{
	for (const critical_entity_key &key : command.keys)
	{
		const auto encoded = entity_key_bytes(key);
		auto found = active_keys.find(std::string_view(encoded.data(), encoded.size()));
		if (found != active_keys.end() && found->second == identity)
			active_keys.erase(found);
	}
}

void add_fences(const std::string &identity, const critical_command &command)
{
	for (const critical_entity_key &key : command.keys)
		fences[entity_key(key)].push_back(identity);
	health.fenced_keys = fences.size();
}

void remove_fences(const std::string &identity, const critical_command &command)
{
	for (const critical_entity_key &key : command.keys)
	{
		// On the publication ACK path the durable frame is already retired.
		// Lookup must not depend on a temporary string's allocation or SSO.
		const auto encoded = entity_key_bytes(key);
		auto found = fences.find(std::string_view(encoded.data(), encoded.size()));
		if (found == fences.end())
			continue;
		auto &identities = found->second;
		identities.erase(std::remove(identities.begin(), identities.end(), identity),
				 identities.end());
		if (identities.empty())
			fences.erase(found);
	}
	health.fenced_keys = fences.size();
}

void update_depth()
{
	health.queued = 0;
	health.inflight = 0;
	health.blocked = 0;
	health.publication_pending = 0;
	health.native_continuation_pending = 0;
	health.retained_bytes = 0;
	health.awaiting_durability = 0;
	uint64_t oldest = 0;
	const uint64_t now = now_usec();
	for (const auto &[identity, state] : operations)
	{
		(void)identity;
		if (state->phase == critical_operation_phase::native_continuation_pending)
			++health.native_continuation_pending;
		else if (operation_is_publication_pending(*state))
			++health.publication_pending;
		else if (operation_is_blocked(*state))
			++health.blocked;
		else if (operation_is_executing(*state))
			++health.inflight;
		else
			++health.queued;
		if (operation_is_awaiting_durability(*state))
			++health.awaiting_durability;
		health.retained_bytes += state->retained_bytes;
		if (!oldest || state->queued_at_usec < oldest)
			oldest = state->queued_at_usec;
	}
	health.oldest_age_msec = oldest && now > oldest ? (now - oldest) / 1000 : 0;
	health.high_water_operations =
		std::max(health.high_water_operations,
			 health.queued + health.inflight + health.blocked +
				 health.publication_pending + health.native_continuation_pending);
	health.high_water_bytes = std::max(health.high_water_bytes, health.retained_bytes);
	health.completed_cache = completed_cache.size();
	health.admission_queue_bytes = pending_admission_bytes + admission_inflight_bytes;
	health.admission_worker_running = admission_worker.joinable() && !stop_requested;
	health.append_inflight = admission_inflight_bytes != 0;
	health.cutover_issuing = active_cutover_phase == cutover_owner_phase::issuing;
	health.cutover_transaction_active = active_cutover_phase ==
					    cutover_owner_phase::transaction_active;
	health.cutover_outcome_uncertain = cutover_outcome_uncertain;
}

void remember_completed(const std::string &identity, const critical_command &command,
			const critical_completion &completion) noexcept
{
	// This bounded cache is an optimization, never durable authority. Every
	// fallible cache preparation belongs inside the exception boundary; ACK
	// cleanup must finish even if the original frame is already checkpointed.
	try
	{
		std::vector<uint8_t> encoded;
		if (critical_command_encode(command, &encoded) != critical_command_codec_result::ok)
			return;
		const size_t retained_size = encoded.size() + sizeof(critical_completion);
		if (encoded.size() > CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES ||
		    retained_size < encoded.size() ||
		    retained_size > CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES)
			return;
		while (!completed_order.empty() &&
		       (completed_cache.size() >= CRITICAL_COORDINATOR_COMPLETED_CACHE_MAX ||
			completed_cache_bytes >
				CRITICAL_COORDINATOR_COMPLETED_CACHE_BYTES - retained_size))
		{
			auto found = completed_cache.find(completed_order.front());
			if (found != completed_cache.end())
			{
				completed_cache_bytes -= found->second.encoded_size;
				completed_cache.erase(found);
			}
			completed_order.pop_front();
		}
		auto inserted = completed_cache.emplace(
			identity, completed_state{ .command = command,
						   .completion = completion,
						   .encoded_size = retained_size });
		if (!inserted.second)
			return;
		try
		{
			completed_order.push_back(identity);
		}
		catch (...)
		{
			// Insertion succeeded but FIFO admission failed. Roll back only
			// this new entry before charging its retained bytes.
			completed_cache.erase(inserted.first);
			return;
		}
		completed_cache_bytes += retained_size;
	}
	catch (...)
	{
		// No cache failure may strand a completed publication obligation.
	}
}

bool completion_is_retryable(const critical_completion &completion)
{
	return completion.outcome == critical_apply_outcome::retryable_failure ||
	       completion.outcome == critical_apply_outcome::ambiguous_commit;
}

bool schedule_retry_locked(const std::string &identity, operation_state &state,
			   const critical_completion &completion)
{
	if (!completion_is_retryable(completion) ||
	    state.attempt > CRITICAL_COORDINATOR_MAX_RETRIES)
		return false;
	release_keys(identity, state.command);
	state.phase = critical_operation_phase::queued;
	if (completion.outcome == critical_apply_outcome::ambiguous_commit)
		++health.ambiguous;
	++state.attempt;
	state.queued_at_usec = now_usec();
	pending.push_back(identity);
	++health.retries;
	work_available.notify_all();
	return true;
}

void retain_exhausted_retry_locked(const std::string &identity, operation_state &state,
				   const critical_completion &completion)
{
	release_keys(identity, state.command);
	state.phase = critical_operation_phase::blocked;
	state.publication_completion = completion;
	if (completion.outcome == critical_apply_outcome::ambiguous_commit)
		++health.ambiguous;
	++health.terminal_failures;
}

bool execution_supported(const critical_command &command)
{
	if (critical_command_valid(command))
		return true;
	return command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       critical_command_envelope_valid(command) && extension_validator_callback &&
	       extension_validator_callback(command);
}

// Only the private native-envelope submit/replay cuts can select shared
// execution. A bare command cannot confer this support, and the genuine
// borrowed worker callback must be installed before a full carrier is accepted.
bool native_envelope_execution_supported(const critical_native_recovery_envelope &envelope)
{
	if (native_birth_shared_shop_command(envelope.command))
		return shared_native_apply_callback &&
		       native_mobile_birth_shared_shop_recovery_valid(envelope);
	return execution_supported(envelope.command);
}

bool enqueue_replayed(critical_command command, void *context)
{
	std::vector<uint8_t> encoded;
	if (native_transport_command(command) || !execution_supported(command) ||
	    critical_command_encode(command, &encoded) != critical_command_codec_result::ok)
		return false;
	const std::string identity = operation_key(command.operation_id);
	if (operations.find(identity) != operations.end() ||
	    operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
	    encoded.size() > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
		return false;
	try
	{
		auto state = std::make_unique<operation_state>();
		state->command = std::move(command);
		state->retained_bytes = encoded.size();
		state->queued_at_usec = now_usec();
		state->attempt = 1;
		state->attachments = 0;
		state->phase = critical_operation_phase::queued;
		state->retain_until_publication = state->command.publication_required;
		state->admission_failure_queued = false;
		operations.emplace(identity, std::move(state));
		pending.push_back(identity);
		add_fences(identity, operations.at(identity)->command);
	}
	catch (const std::bad_alloc &)
	{
		auto inserted = operations.find(identity);
		if (inserted != operations.end())
		{
			remove_fences(identity, inserted->second->command);
			operations.erase(inserted);
		}
		pending.erase(std::remove(pending.begin(), pending.end(), identity), pending.end());
		return false;
	}

	const replay_observer_context *replay =
		static_cast<const replay_observer_context *>(context);
	if (replay && replay->observer)
	{
		bool observed = false;
		try
		{
			observed =
				replay->observer(operations.at(identity)->command, replay->context);
		}
		catch (...)
		{
			observed = false;
		}
		if (!observed)
		{
			auto inserted = operations.find(identity);
			if (inserted != operations.end())
			{
				remove_fences(identity, inserted->second->command);
				operations.erase(inserted);
			}
			pending.erase(std::remove(pending.begin(), pending.end(), identity),
				      pending.end());
			update_depth();
			return false;
		}
	}
	update_depth();
	return true;
}

bool enqueue_native_replayed(critical_native_recovery_envelope envelope, void *context)
{
	const auto *replay = static_cast<const replay_observer_context *>(context);
	size_t retained = 0;
	if (!replay || !replay->native_observer || !native_envelope_execution_supported(envelope) ||
	    !native_envelope_size(envelope, &retained) ||
	    (native_birth_typed_command(envelope.command) &&
	     (!native_birth_validators_ready() || !native_birth_recovery_valid(envelope))) ||
	    (zone_reset_typed_command(envelope.command) &&
	     (!zone_reset_validators_ready() || !zone_reset_validators.valid(envelope))) ||
	    (native_auction_typed_command(envelope.command) &&
	     (!native_auction_validators_ready() || !native_auction_validators.valid(envelope))))
		return false;
	const std::string identity = operation_key(envelope.command.operation_id);
	if (operations.find(identity) != operations.end() ||
	    operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
	    retained > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
		return false;
	const bool continuation = envelope.phase ==
				  critical_native_recovery_phase::continuation_pending;
	// Constructor births retain advancement fences until their real terminal
	// carrier has transferred durably to the native lifetime owner. Replay never
	// re-executes a phase2 birth, including when that transfer was interrupted.
	const bool holds_fences = !continuation || native_birth_origin_command(envelope.command) ||
				  zone_reset_typed_command(envelope.command);
	try
	{
		auto state = std::make_unique<operation_state>();
		state->command = envelope.command;
		state->native = std::make_unique<native_operation_context>();
		state->native->revision = envelope.revision;
		state->native->phase = envelope.phase;
		state->native->attachment = envelope.attachment;
		state->retained_bytes = retained;
		state->queued_at_usec = now_usec();
		state->attempt = 1;
		state->attachments = 0;
		state->phase = continuation ?
				       critical_operation_phase::native_continuation_pending :
				       critical_operation_phase::queued;
		state->retain_until_publication = true;
		state->admission_failure_queued = false;
		state->native_physical_released = continuation;
		operations.emplace(identity, std::move(state));
		if (!continuation)
			pending.push_back(identity);
		if (holds_fences)
			add_fences(identity, operations.at(identity)->command);
		// Native phase2 is passive continuation registration, never execution.
		// The original body codec and domain owner must authenticate its contents.
		if (!replay->native_observer(envelope, replay->context))
		{
			if (holds_fences)
				remove_fences(identity, operations.at(identity)->command);
			operations.erase(identity);
			pending.erase(std::remove(pending.begin(), pending.end(), identity),
				      pending.end());
			update_depth();
			return false;
		}
	}
	catch (...)
	{
		auto found = operations.find(identity);
		if (found != operations.end())
		{
			if (holds_fences)
				remove_fences(identity, found->second->command);
			operations.erase(found);
		}
		pending.erase(std::remove(pending.begin(), pending.end(), identity), pending.end());
		update_depth();
		return false;
	}
	update_depth();
	return true;
}

bool collect_replayed_identity(critical_command command, void *context)
{
	if (!context)
		return false;
	try
	{
		static_cast<std::unordered_set<std::string> *>(context)->insert(
			operation_key(command.operation_id));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return true;
}

unsigned int journal_failure_error(critical_command_journal_result result)
{
	switch (result)
	{
	case critical_command_journal_result::quota_exceeded:
		return ENOSPC;
	case critical_command_journal_result::unsafe_permissions:
		return EACCES;
	case critical_command_journal_result::corrupt_data:
		return EBADMSG;
	case critical_command_journal_result::not_initialized:
		return EPIPE;
	case critical_command_journal_result::invalid:
		return EINVAL;
	case critical_command_journal_result::io_failure:
	case critical_command_journal_result::replay_blocked:
	case critical_command_journal_result::append_uncertain:
	case critical_command_journal_result::ok:
		return EIO;
	}
	return EIO;
}

void retain_admission_failure_locked(operation_state &state, unsigned int error_code)
{
	state.phase = critical_operation_phase::admission_failed;
	state.admission_failure_completion = {
		.operation_id = state.command.operation_id,
		.outcome = critical_apply_outcome::terminal_failure,
		.durable_revision = 0,
		.error_code = error_code,
		.attempt = state.attempt,
		.queued_at_usec = state.queued_at_usec,
		.started_at_usec = 0,
		.completed_at_usec = now_usec(),
		.result_size = 0,
		.result_payload = {},
		.disposition = critical_completion_disposition::never_admitted
	};
	if (!state.admission_failure_queued)
	{
		state.admission_failure_queued = completion_delivery.try_enqueue(
			critical_completion_channel::admission_failure,
			state.admission_failure_completion);
		// The completion remains in the operation state when the delivery buffer
		// is full or allocation is temporarily unavailable. pulse() retries it
		// before considering the operation for retirement.
	}
	++health.admission_failures;
}

bool recovery_due_locked()
{
	return recovery_requested && (!uncertain_recovery_not_before_usec ||
				      now_usec() >= uncertain_recovery_not_before_usec);
}

struct mixed_recovery_record
{
	critical_command command;
	std::unique_ptr<critical_native_recovery_envelope> native;
};
using mixed_recovery_records = std::unordered_map<std::string, mixed_recovery_record>;

bool collect_mixed_legacy(critical_command command, void *context)
{
	if (!context || native_transport_command(command))
		return false;
	try
	{
		const auto identity = operation_key(command.operation_id);
		mixed_recovery_record record;
		record.command = std::move(command);
		return static_cast<mixed_recovery_records *>(context)
			->emplace(identity, std::move(record))
			.second;
	}
	catch (...)
	{
		return false;
	}
}

bool collect_mixed_native(critical_native_recovery_envelope envelope, void *context)
{
	if (!context)
		return false;
	try
	{
		const auto identity = operation_key(envelope.command.operation_id);
		mixed_recovery_record record;
		record.command = envelope.command;
		record.native =
			std::make_unique<critical_native_recovery_envelope>(std::move(envelope));
		return static_cast<mixed_recovery_records *>(context)
			->emplace(identity, std::move(record))
			.second;
	}
	catch (...)
	{
		return false;
	}
}

bool recover_mixed_native_on_worker()
{
	std::vector<std::pair<std::string, mixed_recovery_record>> candidates;
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!health.initialized || stop_requested)
			return false;
		for (const auto &[identity, state] : operations)
			if (operation_is_uncertain(*state))
			{
				mixed_recovery_record candidate;
				candidate.command = state->command;
				if (state->native)
					candidate.native =
						std::make_unique<critical_native_recovery_envelope>(
							native_envelope(*state));
				candidates.emplace_back(identity, std::move(candidate));
			}
	}
	catch (...)
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		defer_uncertain_recovery();
		return false;
	}
	// Corrected mixed sync proves file + namespace before replay can expose an
	// uncertain native admission. Replay then collects complete identities.
	mixed_recovery_records journal;
	if (critical_command_journal_sync() != critical_command_journal_result::ok ||
	    critical_command_journal_replay_with_native(collect_mixed_legacy, collect_mixed_native,
							&journal) !=
		    critical_command_journal_result::ok)
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		defer_uncertain_recovery();
		return false;
	}
	bool recovered = true, wake = false;
	for (const auto &[identity, candidate] : candidates)
	{
		const auto original = journal.find(identity);
		critical_command_journal_result result = critical_command_journal_result::invalid;
		if (candidate.native)
		{
			// Missing or changed native context remains uncertain; never reappend
			// it or manufacture never-admitted/absence evidence from an ID scan.
			if (original != journal.end() && original->second.native &&
			    native_envelopes_equal(*candidate.native, *original->second.native))
				result = critical_command_journal_sync_native_recovery(
					*candidate.native);
		}
		else if (original == journal.end())
			result = critical_command_journal_append(
				candidate.command); // Original legacy policy.
		else if (!original->second.native &&
			 critical_command_equal(candidate.command, original->second.command))
			result = critical_command_journal_result::ok;
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || !operation_is_uncertain(*found->second) ||
		    !critical_command_equal(found->second->command, candidate.command) ||
		    static_cast<bool>(found->second->native) !=
			    static_cast<bool>(candidate.native) ||
		    (candidate.native && !native_matches(*found->second, *candidate.native)))
		{
			recovered = false;
			continue;
		}
		if (result == critical_command_journal_result::ok)
		{
			try
			{
				if (std::find(pending.begin(), pending.end(), identity) ==
				    pending.end())
					pending.push_back(identity);
				found->second->phase = critical_operation_phase::queued;
				wake = true;
			}
			catch (...)
			{
				recovered = false;
			}
		}
		else if (candidate.native ||
			 result == critical_command_journal_result::append_uncertain)
			recovered = false;
		else
		{
			retain_admission_failure_locked(*found->second,
							journal_failure_error(result));
			recovered = false;
		}
	}
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		bool uncertain = false;
		for (const auto &[identity, state] : operations)
			uncertain = uncertain || operation_is_uncertain(*state);
		if (uncertain)
			defer_uncertain_recovery();
		else
		{
			recovery_requested = false;
			uncertain_recovery_not_before_usec = 0;
			uncertain_recovery_delay_usec = 1000000;
		}
		update_depth();
	}
	if (wake)
		work_available.notify_all();
	return recovered;
}

bool recover_uncertain_on_worker()
{
	bool mixed_native = false;
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		for (const auto &[identity, state] : operations)
			mixed_native = mixed_native || static_cast<bool>(state->native);
	}
	if (mixed_native)
		return recover_mixed_native_on_worker();
	std::vector<std::pair<std::string, critical_command>> candidates;
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!health.initialized || stop_requested)
			return false;
		for (const auto &[identity, state] : operations)
			if (operation_is_uncertain(*state))
				candidates.emplace_back(identity, state->command);
	}
	catch (const std::bad_alloc &)
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		defer_uncertain_recovery();
		return false;
	}

	if (candidates.empty())
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		recovery_requested = false;
		uncertain_recovery_not_before_usec = 0;
		uncertain_recovery_delay_usec = 1000000;
		update_depth();
		return true;
	}

	std::unordered_set<std::string> journal_identities;
	if (critical_command_journal_replay(collect_replayed_identity, &journal_identities) !=
	    critical_command_journal_result::ok)
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		defer_uncertain_recovery();
		return false;
	}
	if (critical_command_journal_sync() != critical_command_journal_result::ok)
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		defer_uncertain_recovery();
		return false;
	}

	bool recovered = true;
	bool woke_worker = false;
	for (const auto &[identity, command] : candidates)
	{
		const bool journaled = journal_identities.find(identity) !=
				       journal_identities.end();
		const critical_command_journal_result result =
			journaled ? critical_command_journal_result::ok :
				    critical_command_journal_append(command);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || !operation_is_uncertain(*found->second))
			continue;
		if (result == critical_command_journal_result::ok)
		{
			try
			{
				if (std::find(pending.begin(), pending.end(), identity) ==
				    pending.end())
					pending.push_back(identity);
			}
			catch (const std::bad_alloc &)
			{
				recovered = false;
				continue;
			}
			found->second->phase = critical_operation_phase::queued;
			woke_worker = true;
		}
		else if (result == critical_command_journal_result::append_uncertain)
		{
			recovered = false;
		}
		else
		{
			retain_admission_failure_locked(*found->second,
							journal_failure_error(result));
			recovered = false;
		}
	}

	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		bool uncertain = false;
		for (const auto &[identity, state] : operations)
		{
			(void)identity;
			uncertain = uncertain || operation_is_uncertain(*state);
		}
		if (!uncertain)
		{
			recovery_requested = false;
			uncertain_recovery_not_before_usec = 0;
			uncertain_recovery_delay_usec = 1000000;
		}
		else
			defer_uncertain_recovery();
		update_depth();
	}
	if (woke_worker)
		work_available.notify_all();
	return recovered;
}

void finish_admission(const std::string &identity, critical_command_journal_result result)
{
	bool wake_worker = false;
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end())
			return;
		operation_state &state = *found->second;
		auto trace = persistence_command_trace(state.command,
						       persistence_trace_stage::command_journal);
		trace.outcome = static_cast<uint32_t>(result);
		persistence_trace_record(trace);
		admission_inflight_bytes = 0;
		if (state.native && result == critical_command_journal_result::replay_blocked)
		{
			// The locked journal result proves no append was attempted. Keep the
			// original queue node and body; only the earlier uncertainty may settle.
			pending_admission_bytes += state.retained_bytes;
			update_depth();
			return;
		}
		if (state.native)
			pending_admission.erase(std::remove(pending_admission.begin(),
							    pending_admission.end(), identity),
						pending_admission.end());
		if (result == critical_command_journal_result::ok)
		{
			try
			{
				pending.push_back(identity);
				state.phase = critical_operation_phase::queued;
				++health.durable_admissions;
				wake_worker = true;
			}
			catch (const std::bad_alloc &)
			{
				// The journal is durable. Keep the identity fenced and ask the
				// recovery worker to retry the in-memory ready handoff.
				state.phase = critical_operation_phase::uncertain_admission;
				++health.admission_uncertain;
				defer_uncertain_recovery();
			}
		}
		else if (result == critical_command_journal_result::append_uncertain)
		{
			state.phase = critical_operation_phase::uncertain_admission;
			++health.ambiguous;
			++health.admission_uncertain;
			defer_uncertain_recovery();
		}
		else
		{
			retain_admission_failure_locked(state, journal_failure_error(result));
			++health.terminal_failures;
		}
		update_depth();
	}
	if (wake_worker)
		work_available.notify_all();
}

void admission_worker_main()
{
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		health.admission_worker_running = true;
		update_depth();
	}
	for (;;)
	{
		std::string identity;
		critical_command command;
		std::unique_ptr<critical_native_recovery_envelope> native;
		bool recover = false;
		{
			std::unique_lock<std::mutex> lock(coordinator_mutex);
			for (;;)
			{
				if (stop_requested || !pending_admission.empty())
					break;
				if (recovery_due_locked())
				{
					recover = true;
					break;
				}
				if (!recovery_requested)
				{
					admission_available.wait(
						lock,
						[] {
							return stop_requested ||
							       !pending_admission.empty() ||
							       recovery_requested;
						});
					continue;
				}
				const uint64_t now = now_usec();
				const uint64_t wait_usec =
					uncertain_recovery_not_before_usec > now ?
						uncertain_recovery_not_before_usec - now :
						0;
				admission_available.wait_for(lock,
							     std::chrono::microseconds(wait_usec));
			}
			if (stop_requested && pending_admission.empty())
				break;
			if (!pending_admission.empty())
			{
				identity = pending_admission.front();
				auto found = operations.find(identity);
				if (found == operations.end() ||
				    !operation_is_awaiting_durability(*found->second))
				{
					pending_admission.pop_front();
					update_depth();
					continue;
				}
				// Native no-attempt deferral retains this allocated queue node.
				// Legacy admissions preserve their original dequeue ordering.
				if (!found->second->native)
					pending_admission.pop_front();
				pending_admission_bytes -= found->second->retained_bytes;
				admission_inflight_bytes = found->second->retained_bytes;
				try
				{
					command = found->second->command;
					if (found->second->native)
						native = std::make_unique<
							critical_native_recovery_envelope>(
							native_envelope(*found->second));
				}
				catch (const std::bad_alloc &)
				{
					if (found->second->native)
						pending_admission.pop_front();
					admission_inflight_bytes = 0;
					retain_admission_failure_locked(*found->second, ENOMEM);
					++health.terminal_failures;
					update_depth();
					continue;
				}
				update_depth();
			}
			else if (!recover)
			{
				continue;
			}
		}
		if (recover)
		{
			(void)recover_uncertain_on_worker();
			continue;
		}
		const auto result =
			native ? critical_command_journal_append_native_recovery(*native) :
				 critical_command_journal_append(command);
		finish_admission(identity, result);
		if (native && result == critical_command_journal_result::replay_blocked)
		{
			bool settle_admission = false;
			{
				std::unique_lock<std::mutex> lock(coordinator_mutex);
				settle_admission = recovery_due_locked();
				if (!settle_admission)
					admission_available.wait_for(lock,
								     std::chrono::milliseconds(10));
			}
			if (settle_admission)
				(void)recover_uncertain_on_worker();
		}
	}
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		health.admission_worker_running = false;
		admission_inflight_bytes = 0;
		update_depth();
	}
}

// A conservative worker-only fence: the genuine NMB4 builder adds the selected
// SHOP key only for shared cash. This predicate never grants global support.
bool shared_native_worker_fenced(const critical_command &command) noexcept
{
	return command.type == critical_command_type::native_mobile_birth &&
	       command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
	       std::any_of(command.keys.begin(), command.keys.end(), [](const auto &key)
			   { return key.type == critical_entity_type::shopkeeper; });
}
// Only an installed ordinary callback selects its distinct worker family.
// Original SQL ordinary commands retain their historical generic callback.
bool ordinary_native_worker_fenced(const critical_command &command) noexcept
{
	return ordinary_native_apply_callback &&
	       command.type == critical_command_type::native_mobile_birth &&
	       command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
	       !native_birth_shared_shop_command(command);
}
constexpr size_t ORDINARY_NATIVE_EXECUTION_RETAINED_BYTES =
	sizeof(critical_ordinary_native_execution_owner) +
	sizeof(const critical_ordinary_native_execution_owner *) +
	sizeof(quest_mobile_native_birth_ordinary_execution_lease);

constexpr size_t SHARED_NATIVE_EXECUTION_RETAINED_BYTES =
	sizeof(critical_shared_native_execution_owner) +
	sizeof(const critical_shared_native_execution_owner *);

// Authenticate the real original worker registry, not merely a caller thread.
// Every call observes this existing vector under coordinator_mutex.
bool shared_native_worker_registered(std::thread::id actual) noexcept
{
	return std::any_of(workers.begin(), workers.end(), [&](const std::thread &worker)
			   { return worker.joinable() && worker.get_id() == actual; });
}

bool ordinary_native_worker_ready(const operation_state &state) noexcept
{
	if (!ordinary_native_worker_fenced(state.command))
		return true;
	if (!shared_native_worker_registered(std::this_thread::get_id()) || !state.native ||
	    state.ordinary_execution || state.shared_execution || state.room_execution ||
	    !state.retain_until_publication || state.publication_checkpointing ||
	    state.native_context_uncertain || state.native_ack_uncertain ||
	    state.native_physical_released || !coordinator_generation ||
	    coordinator_generation_exhausted ||
	    state.native->phase != critical_native_recovery_phase::execution_pending ||
	    health.retained_bytes > CRITICAL_COORDINATOR_MAX_BYTES ||
	    ORDINARY_NATIVE_EXECUTION_RETAINED_BYTES >
		    CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
		return false;
	try
	{
		// Full original owning decoder. Allocating scratch is transient, not a
		// bounded/native qualification or a substitute for the producer owner.
		critical_native_recovery_envelope original;
		original.command = state.command;
		original.revision = state.native->revision;
		original.phase = state.native->phase;
		original.attachment = state.native->attachment;
		return native_mobile_birth_cash_role_recovery_valid(original);
	}
	catch (...)
	{
		return false;
	}
}

bool shared_native_worker_ready(const operation_state &state) noexcept
{
	if (!shared_native_worker_fenced(state.command))
		return true;
	// Missing callback, uncertain original state or pure decode/budget refusal
	// keeps this SAME queued operation and attempt. Never call the legacy worker.
	return shared_native_apply_callback &&
	       shared_native_worker_registered(std::this_thread::get_id()) && state.native &&
	       !state.shared_execution && state.retain_until_publication &&
	       !state.publication_checkpointing && !state.native_context_uncertain &&
	       !state.native_ack_uncertain && !state.native_physical_released &&
	       coordinator_generation && !coordinator_generation_exhausted &&
	       state.native->phase == critical_native_recovery_phase::execution_pending &&
	       health.retained_bytes <= CRITICAL_COORDINATOR_MAX_BYTES &&
	       SHARED_NATIVE_EXECUTION_RETAINED_BYTES <=
		       CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes &&
	       native_mobile_birth_shared_shop_recovery_execution_valid(
		       state.command, state.native->attachment, state.native->revision);
}

// Conservative client-free flat ROOM anti-fallthrough only. Typed canonical
// admission, genuine source and complete INITIAL validation are separate.
bool zone_reset_worker_fenced(const critical_command &command) noexcept
{
	const char *root = persistence_mode_flatfile_root();
	return command.type == critical_command_type::zone_reset_item_birth &&
	       persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY &&
	       !persistence_mode_requires_mysql() && root && *root;
}
constexpr size_t ZONE_RESET_EXECUTION_RETAINED_BYTES =
	sizeof(critical_zone_reset_item_execution_owner) +
	sizeof(const critical_zone_reset_item_execution_owner *);
bool zone_reset_worker_ready(const operation_state &state) noexcept
{
	if (!zone_reset_worker_fenced(state.command))
		return true;
	// No full codec, envelope copy, storage callback or allocation under mutex.
	// Missing callback or uncertain/invalid pin keeps this SAME queued attempt.
	return zone_reset_apply_callback && zone_reset_typed_command(state.command) &&
	       shared_native_worker_registered(std::this_thread::get_id()) && state.native &&
	       !state.room_execution && !state.shared_execution && state.retain_until_publication &&
	       !state.publication_checkpointing && !state.native_context_uncertain &&
	       !state.native_ack_uncertain && !state.native_physical_released &&
	       coordinator_generation && !coordinator_generation_exhausted &&
	       state.native->phase == critical_native_recovery_phase::execution_pending &&
	       state.native->revision && !state.native->attachment.empty() &&
	       health.retained_bytes <= CRITICAL_COORDINATOR_MAX_BYTES &&
	       ZONE_RESET_EXECUTION_RETAINED_BYTES <=
		       CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes;
}
} // namespace

bool critical_ordinary_native_execution_owner::current_locked() const noexcept
{
	return worker_ == std::this_thread::get_id() && borrowed_current_locked();
}

bool critical_ordinary_native_execution_owner::borrowed_current_locked() const noexcept
{
	if (!operation_ || !native_ || !command_ || !generation_ || !attempt_ ||
	    !health.initialized || !health.running || stop_requested ||
	    coordinator_generation_exhausted || coordinator_generation != generation_ ||
	    phase_ != critical_native_recovery_phase::execution_pending ||
	    !shared_native_worker_registered(worker_))
		return false;
	// Membership is the genuine pinned object, never a replacement found by ID.
	const auto found = std::find_if(operations.begin(), operations.end(), [&](const auto &entry)
					{ return entry.second.get() == operation_; });
	if (found == operations.end())
		return false;
	const auto &state = *found->second;
	// Existing replacement/retirement APIs reject executing owners. Thus these
	// exact borrowed command/attachment bodies remain immutable for this pin.
	return operation_is_executing(state) && state.ordinary_execution == this &&
	       state.native.get() == native_ && &state.command == command_ &&
	       state.attempt == attempt_ && state.retain_until_publication &&
	       !state.publication_checkpointing && !state.native_context_uncertain &&
	       !state.native_ack_uncertain && !state.native_physical_released &&
	       state.native->revision == revision_ && state.native->phase == phase_ &&
	       state.native->attachment.data() == attachment_.data() &&
	       state.native->attachment.size() == attachment_.size() &&
	       state.retained_bytes >= ORDINARY_NATIVE_EXECUTION_RETAINED_BYTES &&
	       ordinary_native_worker_fenced(state.command);
}

bool critical_ordinary_native_execution_owner::current() const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		return current_locked();
	}
	catch (...)
	{
		return false;
	}
}

bool critical_ordinary_native_execution_owner::borrowed_current() const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		return borrowed_current_locked();
	}
	catch (...)
	{
		return false;
	}
}

void critical_shared_native_execution_dispatch::cancel_ordinary_waits() noexcept
{
	quest_mobile_native_birth_ordinary_execution_lease::cancel_pending();
}

flatfile_accounting_native_mobile_birth_ordinary_transaction *
critical_ordinary_native_execution_owner::flat_transaction() const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!current_locked())
			return nullptr;
		return static_cast<const operation_state *>(operation_)
			->ordinary_flat_transaction.get();
	}
	catch (...)
	{
		return nullptr;
	}
}

bool critical_ordinary_native_execution_owner::retain_flat_transaction(
	std::unique_ptr<flatfile_accounting_native_mobile_birth_ordinary_transaction> &proposal,
	size_t participant_bytes) const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!current_locked() || !proposal || participant_bytes < sizeof(*proposal))
			return false;
		auto &state = *const_cast<operation_state *>(
			static_cast<const operation_state *>(operation_));
		if (state.ordinary_flat_transaction || state.ordinary_flat_transaction_bytes)
			return false;
		constexpr size_t slot_bytes = sizeof(state.ordinary_flat_transaction) +
					      sizeof(state.ordinary_flat_transaction_bytes);
		if (participant_bytes > CRITICAL_COORDINATOR_MAX_BYTES - slot_bytes)
			return false;
		const size_t charged = participant_bytes + slot_bytes;
		update_depth();
		if (health.retained_bytes > CRITICAL_COORDINATOR_MAX_BYTES ||
		    state.retained_bytes > CRITICAL_COORDINATOR_MAX_BYTES ||
		    charged > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes ||
		    charged > CRITICAL_COORDINATOR_MAX_BYTES - state.retained_bytes)
			return false;
		state.ordinary_flat_transaction = std::move(proposal);
		state.ordinary_flat_transaction_bytes = charged;
		state.retained_bytes += charged;
		update_depth();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_ordinary_native_execution_owner::release_unpublished_flat_transaction(
	flatfile_accounting_native_mobile_birth_ordinary_transaction *proposal) const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		// Shutdown changes running/stop and generation BEFORE joining this
		// worker. Pure heap cleanup needs the original still-executing pin,
		// never renewed execution authority from those active policy flags.
		if (!proposal || !operation_ || !native_ || !command_ || !generation_ ||
		    !attempt_ || worker_ != std::this_thread::get_id())
			return false;
		const auto found = std::find_if(operations.begin(), operations.end(),
						[&](const auto &entry)
						{ return entry.second.get() == operation_; });
		if (found == operations.end())
			return false;
		auto &state = *found->second;
		if (!operation_is_executing(state) || state.ordinary_execution != this ||
		    state.native.get() != native_ || &state.command != command_ ||
		    state.attempt != attempt_ || !state.native ||
		    state.native->revision != revision_ || state.native->phase != phase_ ||
		    phase_ != critical_native_recovery_phase::execution_pending ||
		    state.native->attachment.data() != attachment_.data() ||
		    state.native->attachment.size() != attachment_.size())
			return false;
		if (state.ordinary_flat_transaction.get() != proposal ||
		    !state.ordinary_flat_transaction_bytes ||
		    state.retained_bytes < state.ordinary_flat_transaction_bytes)
			return false;
		// The sole private flat friend proves genuine not_published before
		// invoking this pure-proposal release. Possible publication is retained.
		state.ordinary_flat_transaction.reset();
		state.retained_bytes -= state.ordinary_flat_transaction_bytes;
		state.ordinary_flat_transaction_bytes = 0;
		update_depth();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_shared_native_execution_owner::current_locked() const noexcept
{
	if (!operation_ || !native_ || !command_ || !generation_ || !attempt_ ||
	    worker_ != std::this_thread::get_id() || !health.initialized || !health.running ||
	    stop_requested || coordinator_generation_exhausted ||
	    coordinator_generation != generation_ ||
	    phase_ != critical_native_recovery_phase::execution_pending ||
	    !shared_native_worker_registered(worker_))
		return false;
	// Membership is the genuine pinned object, never a replacement found by ID.
	const auto found = std::find_if(operations.begin(), operations.end(), [&](const auto &entry)
					{ return entry.second.get() == operation_; });
	if (found == operations.end())
		return false;
	const auto &state = *found->second;
	// Existing replacement/retirement APIs reject executing owners. Thus these
	// exact borrowed command/attachment bodies remain immutable for this pin.
	return operation_is_executing(state) && state.shared_execution == this &&
	       state.native.get() == native_ && &state.command == command_ &&
	       state.attempt == attempt_ && state.retain_until_publication &&
	       !state.publication_checkpointing && !state.native_context_uncertain &&
	       !state.native_ack_uncertain && !state.native_physical_released &&
	       state.native->revision == revision_ && state.native->phase == phase_ &&
	       state.native->attachment.data() == attachment_.data() &&
	       state.native->attachment.size() == attachment_.size() &&
	       state.retained_bytes >= SHARED_NATIVE_EXECUTION_RETAINED_BYTES &&
	       shared_native_worker_fenced(state.command);
}

bool critical_shared_native_execution_owner::current() const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		return current_locked();
	}
	catch (...)
	{
		return false;
	}
}

flatfile_accounting_native_mobile_birth_shared_shop_transaction *
critical_shared_native_execution_owner::flat_transaction() const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!current_locked())
			return nullptr;
		return static_cast<const operation_state *>(operation_)->flat_transaction.get();
	}
	catch (...)
	{
		return nullptr;
	}
}

bool critical_shared_native_execution_owner::retain_flat_transaction(
	std::unique_ptr<flatfile_accounting_native_mobile_birth_shared_shop_transaction> &proposal,
	size_t participant_bytes) const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!current_locked() || !proposal || participant_bytes < sizeof(*proposal))
			return false;
		auto &state = *const_cast<operation_state *>(
			static_cast<const operation_state *>(operation_));
		if (state.flat_transaction || state.flat_transaction_bytes)
			return false;
		constexpr size_t slot_bytes =
			sizeof(state.flat_transaction) + sizeof(state.flat_transaction_bytes);
		if (participant_bytes > CRITICAL_COORDINATOR_MAX_BYTES - slot_bytes)
			return false;
		const size_t charged = participant_bytes + slot_bytes;
		update_depth();
		if (health.retained_bytes > CRITICAL_COORDINATOR_MAX_BYTES ||
		    state.retained_bytes > CRITICAL_COORDINATOR_MAX_BYTES ||
		    charged > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes ||
		    charged > CRITICAL_COORDINATOR_MAX_BYTES - state.retained_bytes)
			return false;
		state.flat_transaction = std::move(proposal);
		state.flat_transaction_bytes = charged;
		state.retained_bytes += charged;
		update_depth();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_shared_native_execution_owner::release_unpublished_flat_transaction(
	flatfile_accounting_native_mobile_birth_shared_shop_transaction *proposal) const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		// Shutdown changes running/stop and generation BEFORE joining this
		// worker. Pure heap cleanup needs the original still-executing pin,
		// never renewed execution authority from those active policy flags.
		if (!proposal || !operation_ || !native_ || !command_ || !generation_ ||
		    !attempt_ || worker_ != std::this_thread::get_id())
			return false;
		const auto found = std::find_if(operations.begin(), operations.end(),
						[&](const auto &entry)
						{ return entry.second.get() == operation_; });
		if (found == operations.end())
			return false;
		auto &state = *found->second;
		if (!operation_is_executing(state) || state.shared_execution != this ||
		    state.native.get() != native_ || &state.command != command_ ||
		    state.attempt != attempt_ || !state.native ||
		    state.native->revision != revision_ || state.native->phase != phase_ ||
		    phase_ != critical_native_recovery_phase::execution_pending ||
		    state.native->attachment.data() != attachment_.data() ||
		    state.native->attachment.size() != attachment_.size())
			return false;
		if (state.flat_transaction.get() != proposal || !state.flat_transaction_bytes ||
		    state.retained_bytes < state.flat_transaction_bytes)
			return false;
		// The sole private flat friend proves genuine not_published before
		// invoking this pure-proposal release. Possible publication is retained.
		state.flat_transaction.reset();
		state.retained_bytes -= state.flat_transaction_bytes;
		state.flat_transaction_bytes = 0;
		update_depth();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_zone_reset_item_execution_owner::current_locked() const noexcept
{
	if (!operation_ || !native_ || !command_ || !generation_ || !attempt_ ||
	    worker_ != std::this_thread::get_id() || !health.initialized || !health.running ||
	    stop_requested || coordinator_generation_exhausted ||
	    coordinator_generation != generation_ ||
	    phase_ != critical_native_recovery_phase::execution_pending ||
	    !shared_native_worker_registered(worker_))
		return false;
	// Membership is the genuine pinned object, never a replacement found by ID.
	const auto found = std::find_if(operations.begin(), operations.end(), [&](const auto &entry)
					{ return entry.second.get() == operation_; });
	if (found == operations.end())
		return false;
	const auto &state = *found->second;
	// Existing replacement/retirement APIs reject executing owners. Thus these
	// exact borrowed command/attachment bodies remain immutable for this pin.
	return operation_is_executing(state) && state.room_execution == this &&
	       state.native.get() == native_ && &state.command == command_ &&
	       state.attempt == attempt_ && state.retain_until_publication &&
	       !state.publication_checkpointing && !state.native_context_uncertain &&
	       !state.native_ack_uncertain && !state.native_physical_released &&
	       state.native->revision == revision_ && state.native->phase == phase_ &&
	       state.native->attachment.data() == attachment_.data() &&
	       state.native->attachment.size() == attachment_.size() &&
	       state.retained_bytes >= ZONE_RESET_EXECUTION_RETAINED_BYTES &&
	       zone_reset_worker_fenced(state.command);
}

bool critical_zone_reset_item_execution_owner::current() const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		return current_locked();
	}
	catch (...)
	{
		return false;
	}
}

flatfile_accounting_zone_reset_item_transaction *
critical_zone_reset_item_execution_owner::flat_transaction() const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!current_locked())
			return nullptr;
		return static_cast<const operation_state *>(operation_)->room_flat_transaction.get();
	}
	catch (...)
	{
		return nullptr;
	}
}

bool critical_zone_reset_item_execution_owner::retain_flat_transaction(
	std::unique_ptr<flatfile_accounting_zone_reset_item_transaction> &proposal,
	size_t participant_bytes) const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!current_locked() || !proposal || participant_bytes < sizeof(*proposal))
			return false;
		auto &state = *const_cast<operation_state *>(
			static_cast<const operation_state *>(operation_));
		if (state.room_flat_transaction || state.room_flat_transaction_bytes)
			return false;
		constexpr size_t slot_bytes = sizeof(state.room_flat_transaction) +
					      sizeof(state.room_flat_transaction_bytes);
		if (participant_bytes > CRITICAL_COORDINATOR_MAX_BYTES - slot_bytes)
			return false;
		const size_t charged = participant_bytes + slot_bytes;
		update_depth();
		if (health.retained_bytes > CRITICAL_COORDINATOR_MAX_BYTES ||
		    state.retained_bytes > CRITICAL_COORDINATOR_MAX_BYTES ||
		    charged > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes ||
		    charged > CRITICAL_COORDINATOR_MAX_BYTES - state.retained_bytes)
			return false;
		state.room_flat_transaction = std::move(proposal);
		state.room_flat_transaction_bytes = charged;
		state.retained_bytes += charged;
		update_depth();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_zone_reset_item_execution_owner::release_unpublished_flat_transaction(
	flatfile_accounting_zone_reset_item_transaction *proposal) const noexcept
{
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		// Shutdown changes running/stop and generation BEFORE joining this
		// worker. Pure heap cleanup needs the original still-executing pin,
		// never renewed execution authority from those active policy flags.
		if (!proposal || !operation_ || !native_ || !command_ || !generation_ ||
		    !attempt_ || worker_ != std::this_thread::get_id())
			return false;
		const auto found = std::find_if(operations.begin(), operations.end(),
						[&](const auto &entry)
						{ return entry.second.get() == operation_; });
		if (found == operations.end())
			return false;
		auto &state = *found->second;
		if (!operation_is_executing(state) || state.room_execution != this ||
		    state.native.get() != native_ || &state.command != command_ ||
		    state.attempt != attempt_ || !state.native ||
		    state.native->revision != revision_ || state.native->phase != phase_ ||
		    phase_ != critical_native_recovery_phase::execution_pending ||
		    state.native->attachment.data() != attachment_.data() ||
		    state.native->attachment.size() != attachment_.size())
			return false;
		if (state.room_flat_transaction.get() != proposal ||
		    !state.room_flat_transaction_bytes ||
		    state.retained_bytes < state.room_flat_transaction_bytes)
			return false;
		// The sole private flat friend proves genuine not_published before
		// invoking this pure-proposal release. Possible publication is retained.
		state.room_flat_transaction.reset();
		state.retained_bytes -= state.room_flat_transaction_bytes;
		state.room_flat_transaction_bytes = 0;
		update_depth();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

void critical_shared_native_execution_dispatch::worker_main()
{
	for (;;)
	{
		std::string identity;
		critical_command copied_command;
		critical_ordinary_native_execution_owner ordinary_owner;
		critical_ordinary_native_apply_fn ordinary_apply = nullptr;
		operation_state *ordinary_state = nullptr;
		critical_shared_native_execution_owner shared_owner;
		critical_zone_reset_item_execution_owner room_owner;
		critical_zone_reset_item_apply_fn room_apply = nullptr;
		operation_state *room_state = nullptr;
		const critical_command *command_view = &copied_command;
		critical_shared_native_apply_fn shared_apply = nullptr;
		operation_state *shared_state = nullptr;
		unsigned int attempt = 0;
		uint64_t queued_at = 0;
		bool retain_publication = false;
		{
			std::unique_lock<std::mutex> lock(coordinator_mutex);
			work_available.wait(
				lock,
				[]
				{
					if (stop_requested)
						return true;
					for (const std::string &candidate : pending)
					{
						auto found = operations.find(candidate);
						if (found != operations.end() &&
						    operation_is_queued(*found->second) &&
						    keys_available(candidate,
								   found->second->command) &&
						    ordinary_native_worker_ready(*found->second) &&
						    shared_native_worker_ready(*found->second) &&
						    zone_reset_worker_ready(*found->second))
							return true;
					}
					return false;
				});
			if (stop_requested)
				return;
			auto ready = pending.end();
			for (auto iterator = pending.begin(); iterator != pending.end(); ++iterator)
			{
				auto found = operations.find(*iterator);
				if (found != operations.end() &&
				    operation_is_queued(*found->second) &&
				    keys_available(*iterator, found->second->command) &&
				    ordinary_native_worker_ready(*found->second) &&
				    shared_native_worker_ready(*found->second) &&
				    zone_reset_worker_ready(*found->second))
				{
					ready = iterator;
					break;
				}
			}
			if (ready == pending.end())
				continue;
			identity = *ready;
			pending.erase(ready);
			operation_state &state = *operations.at(identity);
			state.phase = critical_operation_phase::executing;
			acquire_keys(identity, state.command);
			if (ordinary_native_worker_fenced(state.command))
			{
				// The complete source was decoded before dequeue; no new body copy.
				ordinary_owner.operation_ = &state;
				ordinary_owner.native_ = state.native.get();
				ordinary_owner.command_ = &state.command;
				ordinary_owner.attachment_ = state.native->attachment;
				ordinary_owner.generation_ = coordinator_generation;
				ordinary_owner.revision_ = state.native->revision;
				ordinary_owner.phase_ = state.native->phase;
				ordinary_owner.attempt_ = state.attempt;
				ordinary_owner.worker_ = std::this_thread::get_id();
				state.ordinary_execution = &ordinary_owner;
				state.retained_bytes += ORDINARY_NATIVE_EXECUTION_RETAINED_BYTES;
				command_view = &state.command;
				ordinary_apply = ordinary_native_apply_callback;
				ordinary_state = &state;
			}
			else if (shared_native_worker_fenced(state.command))
			{
				// The complete source was decoded before dequeue; no new body copy.
				shared_owner.operation_ = &state;
				shared_owner.native_ = state.native.get();
				shared_owner.command_ = &state.command;
				shared_owner.attachment_ = state.native->attachment;
				shared_owner.generation_ = coordinator_generation;
				shared_owner.revision_ = state.native->revision;
				shared_owner.phase_ = state.native->phase;
				shared_owner.attempt_ = state.attempt;
				shared_owner.worker_ = std::this_thread::get_id();
				state.shared_execution = &shared_owner;
				state.retained_bytes += SHARED_NATIVE_EXECUTION_RETAINED_BYTES;
				command_view = &state.command;
				shared_apply = shared_native_apply_callback;
				shared_state = &state;
			}
			else if (zone_reset_worker_fenced(state.command))
			{
				// Borrow the immutable original pin only. Genuine ROOM callback
				// authenticates full INITIAL carrier/season outside mutex.
				room_owner.operation_ = &state;
				room_owner.native_ = state.native.get();
				room_owner.command_ = &state.command;
				room_owner.attachment_ = state.native->attachment;
				room_owner.generation_ = coordinator_generation;
				room_owner.revision_ = state.native->revision;
				room_owner.phase_ = state.native->phase;
				room_owner.attempt_ = state.attempt;
				room_owner.worker_ = std::this_thread::get_id();
				state.room_execution = &room_owner;
				state.retained_bytes += ZONE_RESET_EXECUTION_RETAINED_BYTES;
				command_view = &state.command;
				room_apply = zone_reset_apply_callback;
				room_state = &state;
			}
			else
				try
				{
					copied_command = state.command;
				}
				catch (const std::bad_alloc &)
				{
					release_keys(identity, state.command);
					state.phase = critical_operation_phase::blocked;
					++health.terminal_failures;
					update_depth();
					continue;
				}
			attempt = state.attempt;
			queued_at = state.queued_at_usec;
			retain_publication = state.retain_until_publication;
			update_depth();
		}
		const auto release_ordinary = [&]() noexcept
		{
			if (!ordinary_state)
				return;
			std::lock_guard<std::mutex> lock(coordinator_mutex);
			// Shutdown joins this original worker before erasing operation storage.
			if (ordinary_state->ordinary_execution == &ordinary_owner)
			{
				ordinary_state->ordinary_execution = nullptr;
				ordinary_state->retained_bytes -=
					ORDINARY_NATIVE_EXECUTION_RETAINED_BYTES;
				update_depth();
			}
		};
		const auto release_shared = [&]() noexcept
		{
			if (!shared_state)
				return;
			std::lock_guard<std::mutex> lock(coordinator_mutex);
			// Shutdown joins this original worker before erasing operation storage.
			if (shared_state->shared_execution == &shared_owner)
			{
				shared_state->shared_execution = nullptr;
				shared_state->retained_bytes -=
					SHARED_NATIVE_EXECUTION_RETAINED_BYTES;
				update_depth();
			}
		};
		const auto release_room = [&]() noexcept
		{
			if (!room_state)
				return;
			std::lock_guard<std::mutex> lock(coordinator_mutex);
			// Shutdown joins this original worker before erasing operation storage.
			if (room_state->room_execution == &room_owner)
			{
				room_state->room_execution = nullptr;
				room_state->retained_bytes -= ZONE_RESET_EXECUTION_RETAINED_BYTES;
				update_depth();
			}
		};
		if ((ordinary_apply && !ordinary_owner.current()) ||
		    (shared_apply && !shared_owner.current()) ||
		    (room_apply && !room_owner.current()))
		{
			// Lost coordinator lifetime before invocation: no fabricated completion
			// or never-admitted receipt. Original durable state stays owned.
			release_ordinary();
			release_shared();
			release_room();
			return;
		}
		const critical_command &command = *command_view;
		auto trace =
			persistence_command_trace(command, persistence_trace_stage::command_apply);
		trace.attempt = attempt;
		persistence_trace_record(trace);
		const uint64_t started = now_usec();
		critical_apply_result applied = {};
		try
		{
			if (ordinary_apply)
				applied = ordinary_apply(ordinary_owner, apply_context);
			else if (shared_apply)
				applied = shared_apply(shared_owner, apply_context);
			else if (room_apply)
				applied = room_apply(room_owner, apply_context);
			else
				applied = apply_callback(command, apply_context);
		}
		catch (...)
		{
			// The shared callback may have issued SQL before throwing. Preserve
			// uncertainty; the ordinary callback's historical policy is unchanged.
			applied = { (ordinary_apply || shared_apply || room_apply) ?
					    critical_apply_outcome::ambiguous_commit :
					    critical_apply_outcome::retryable_failure,
				    0, 0 };
		}
		release_ordinary();
		release_shared();
		release_room();
		if (!retain_publication &&
		    (applied.outcome == critical_apply_outcome::applied ||
		     applied.outcome == critical_apply_outcome::already_applied ||
		     applied.outcome == critical_apply_outcome::terminal_failure))
		{
			const auto checkpoint =
				critical_command_journal_checkpoint(command.operation_id);
			auto checkpoint_trace = trace;
			checkpoint_trace.stage = persistence_trace_stage::command_checkpoint;
			checkpoint_trace.outcome = static_cast<uint32_t>(checkpoint);
			checkpoint_trace.error = applied.error_code;
			checkpoint_trace.diagnosis = static_cast<uint32_t>(applied.failure_stage);
			checkpoint_trace.durable_revision = applied.durable_revision;
			checkpoint_trace.incident = checkpoint !=
							    critical_command_journal_result::ok &&
						    attempt > CRITICAL_COORDINATOR_MAX_RETRIES;
			persistence_trace_record(checkpoint_trace);
			if (checkpoint != critical_command_journal_result::ok)
				applied = { critical_apply_outcome::retryable_failure,
					    applied.durable_revision, applied.error_code };
		}
		// Report the effective completion, including a failed journal checkpoint.
		trace.stage = persistence_trace_stage::command_result;
		trace.outcome = static_cast<uint32_t>(applied.outcome);
		trace.error = applied.error_code;
		trace.diagnosis = static_cast<uint32_t>(applied.failure_stage);
		trace.durable_revision = applied.durable_revision;
		trace.incident = applied.outcome == critical_apply_outcome::terminal_failure ||
				 ((applied.outcome == critical_apply_outcome::retryable_failure ||
				   applied.outcome == critical_apply_outcome::ambiguous_commit) &&
				  attempt > CRITICAL_COORDINATOR_MAX_RETRIES);
		persistence_trace_record(trace);
		critical_completion completion = { .operation_id = command.operation_id,
						   .outcome = applied.outcome,
						   .durable_revision = applied.durable_revision,
						   .error_code = applied.error_code,
						   .attempt = attempt,
						   .queued_at_usec = queued_at,
						   .started_at_usec = started,
						   .completed_at_usec = now_usec(),
						   .failure_stage = applied.failure_stage,
						   .result_size = applied.result_size,
						   .result_payload = applied.result_payload };
		std::unique_lock<std::mutex> lock(coordinator_mutex);
		result_available.wait(
			lock, [] { return stop_requested || completion_delivery.has_capacity(); });
		if (stop_requested)
			return;
		completion_delivery.enqueue(critical_completion_channel::execution, completion);
	}
}

namespace
{

bool cutover_ready_locked(bool lifecycle_owner = false)
{
	update_depth();
	if ((lifecycle_guard_active && !lifecycle_owner) || !health.initialized ||
	    !health.running || health.accepting || stop_requested ||
	    guarded_publications_inflight || health.queued || health.inflight || health.blocked ||
	    health.publication_pending || health.native_continuation_pending ||
	    health.awaiting_durability || health.admission_queue_bytes || health.append_inflight ||
	    health.fenced_keys || !operations.empty() || !pending.empty() ||
	    !pending_admission.empty() || pending_admission_bytes || admission_inflight_bytes ||
	    !active_keys.empty() || !fences.empty() || completion_delivery.size())
		return false;

	// Keep journal readiness inside this coordinator-locked decision so lease
	// issuance cannot race a new admission between preflight and ownership.
	const critical_command_journal_health journal = critical_command_journal_health_copy();
	return journal.initialized && !journal.append_uncertain && !journal.records;
}
} // namespace

bool critical_command_coordinator_init(
	const char *journal_directory_path, critical_apply_fn apply, void *context,
	unsigned int worker_count, critical_replay_observer_fn replay_observer,
	void *replay_context, critical_extension_validator_fn extension_validator,
	critical_native_recovery_observer_fn native_replay_observer,
	critical_native_recovery_publication_validator_fn native_publication_validator,
	critical_native_birth_recovery_validators birth_validators,
	critical_native_recovery_pair_validator_fn quest_pair_validator,
	critical_native_auction_recovery_validators auction_validators,
	critical_zone_reset_recovery_validators reset_validators,
	critical_shared_native_apply_fn shared_native_apply,
	critical_zone_reset_item_apply_fn zone_reset_apply,
	critical_extension_validator_bounded_fn extension_validator_bounded,
	critical_native_recovery_observer_bounded_fn native_replay_observer_bounded,
	critical_ordinary_native_apply_fn ordinary_native_apply)
{
	if (!apply || !worker_count || worker_count > CRITICAL_COORDINATOR_DEFAULT_WORKERS * 4)
		return false;
	// Real lifecycle exclusion precedes this original uninterrupted init lock.
	auto lifecycle = critical_mixed_startup_replay_owner::acquire_lifecycle();
	std::unique_lock<std::mutex> lock(coordinator_mutex);
	if (health.initialized || lifecycle_guard_active ||
	    active_cutover_phase != cutover_owner_phase::none)
		return false;
	critical_mixed_startup_replay_owner startup(lock, lifecycle);
	// Scalar-only reset after original init guard, before journal replay.
	// Unregistered defaults remain unchanged; no stale observer survives boot.
	if (!critical_room_shared_budget_lender::reset_before_replay(lock))
		return false;
	advance_coordinator_generation();
	invalidate_active_cutover_lease();
	if (coordinator_generation_exhausted ||
	    !critical_command_journal_init(journal_directory_path))
		return false;
	operations.clear();
	publication_checkpoints_inflight = 0;
	pending.clear();
	pending_admission.clear();
	completion_delivery.clear();
	active_keys.clear();
	fences.clear();
	completed_cache.clear();
	completed_order.clear();
	completed_cache_bytes = 0;
	pending_admission_bytes = 0;
	admission_inflight_bytes = 0;
	health = {};
	health.initialized = true;
	health.accepting = true;
	health.running = true;
	apply_callback = apply;
	shared_native_apply_callback = shared_native_apply;
	ordinary_native_apply_callback = ordinary_native_apply;
	zone_reset_apply_callback = zone_reset_apply;
	extension_validator_callback = extension_validator;
	extension_validator_bounded_callback = extension_validator_bounded;
	native_replay_observer_callback = native_replay_observer;
	native_replay_observer_bounded_callback = native_replay_observer_bounded;
	native_publication_validator_callback = native_publication_validator;
	native_birth_validators = birth_validators;
	native_quest_pair_validator = quest_pair_validator;
	native_auction_validators = auction_validators;
	zone_reset_validators = reset_validators;
	apply_context = context;
	stop_requested = false;
	recovery_requested = false;
	uncertain_recovery_not_before_usec = 0;
	uncertain_recovery_delay_usec = 1000000;
	replay_observer_context replay = { replay_observer, replay_context, native_replay_observer,
					   native_replay_observer_bounded };
	// Missing native observer uses legacy replay's complete preflight refusal,
	// before any passive legacy registration callback can see a native journal.
	const auto replayed =
		native_replay_observer ?
			critical_command_journal_replay_with_native(
				enqueue_replayed, enqueue_native_replayed, &replay) :
			critical_command_journal_replay(enqueue_replayed,
							replay_observer ? &replay : nullptr);
	if (replayed != critical_command_journal_result::ok)
	{
		health = {};
		shared_native_apply_callback = nullptr;
		ordinary_native_apply_callback = nullptr;
		zone_reset_apply_callback = nullptr;
		extension_validator_callback = nullptr;
		extension_validator_bounded_callback = nullptr;
		native_replay_observer_callback = nullptr;
		native_replay_observer_bounded_callback = nullptr;
		native_publication_validator_callback = nullptr;
		native_birth_validators = {};
		native_quest_pair_validator = nullptr;
		native_auction_validators = {};
		zone_reset_validators = {};
		critical_command_journal_shutdown();
		return false;
	}
	try
	{
		admission_worker = std::thread(admission_worker_main);
		for (unsigned int index = 0; index < worker_count; ++index)
			workers.emplace_back(
				critical_shared_native_execution_dispatch::worker_main);
	}
	catch (const std::system_error &)
	{
		stop_requested = true;
		work_available.notify_all();
		admission_available.notify_all();
		lock.unlock();
		critical_shared_native_execution_dispatch::cancel_ordinary_waits();
		if (admission_worker.joinable())
			admission_worker.join();
		for (std::thread &worker : workers)
			if (worker.joinable())
				worker.join();
		lock.lock();
		workers.clear();
		admission_worker = {};
		health = {};
		shared_native_apply_callback = nullptr;
		ordinary_native_apply_callback = nullptr;
		zone_reset_apply_callback = nullptr;
		extension_validator_callback = nullptr;
		extension_validator_bounded_callback = nullptr;
		critical_command_journal_shutdown();
		return false;
	}
	work_available.notify_all();
	return true;
}

bool critical_command_coordinator_try_acquire_lifecycle_guard(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (lifecycle_guard_active || active_cutover_phase != cutover_owner_phase::none ||
	    guarded_publications_inflight)
		return false;
	lifecycle_guard_active = true;
	lifecycle_guard_was_accepting = health.accepting;
	lifecycle_guard_initialized_runtime = false;
	lifecycle_guard_thread = std::this_thread::get_id();
	health.accepting = false;
	return true;
}

bool critical_command_coordinator_lifecycle_guard_held_by_current_thread(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	return lifecycle_guard_active && lifecycle_guard_thread == std::this_thread::get_id();
}

void critical_command_coordinator_release_lifecycle_guard(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!lifecycle_guard_active || lifecycle_guard_thread != std::this_thread::get_id())
		return;
	lifecycle_guard_active = false;
	lifecycle_guard_thread = {};
	if (health.initialized && !stop_requested &&
	    active_cutover_phase == cutover_owner_phase::none)
		health.accepting = lifecycle_guard_was_accepting;
	lifecycle_guard_was_accepting = false;
	lifecycle_guard_initialized_runtime = false;
}

bool critical_command_coordinator_shutdown(void)
{
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		// Shutdown may hand off durable native phases through the journal, but
		// must not discard a RAM-only no-attempt body or an unsettled append.
		const bool native_admission_unsettled = std::any_of(
			operations.begin(), operations.end(),
			[](const auto &entry)
			{
				const auto &state = *entry.second;
				return state.native &&
				       (operation_is_awaiting_durability(state) ||
					state.phase ==
						critical_operation_phase::uncertain_admission);
			});
		if (active_cutover_phase != cutover_owner_phase::none ||
		    guarded_publications_inflight || native_admission_unsettled)
		{
			health.shutdown_refused = true;
			health.accepting = false;
			return false;
		}
		stop_requested = true;
		health.accepting = false;
		advance_coordinator_generation();
		invalidate_active_cutover_lease();
		work_available.notify_all();
		result_available.notify_all();
		admission_available.notify_all();
	}
	critical_shared_native_execution_dispatch::cancel_ordinary_waits();
	if (admission_worker.joinable())
		admission_worker.join();
	for (std::thread &worker : workers)
		if (worker.joinable())
			worker.join();
	std::unique_lock<std::mutex> lock(coordinator_mutex);
	publication_checkpoint_finished.wait(lock,
					     [] { return publication_checkpoints_inflight == 0; });
	workers.clear();
	operations.clear();
	pending.clear();
	pending_admission.clear();
	completion_delivery.clear();
	active_keys.clear();
	fences.clear();
	completed_cache.clear();
	completed_order.clear();
	completed_cache_bytes = 0;
	pending_admission_bytes = 0;
	admission_inflight_bytes = 0;
	health = {};
	apply_callback = nullptr;
	shared_native_apply_callback = nullptr;
	ordinary_native_apply_callback = nullptr;
	zone_reset_apply_callback = nullptr;
	extension_validator_callback = nullptr;
	extension_validator_bounded_callback = nullptr;
	native_replay_observer_callback = nullptr;
	native_replay_observer_bounded_callback = nullptr;
	native_publication_validator_callback = nullptr;
	native_birth_validators = {};
	native_quest_pair_validator = nullptr;
	native_auction_validators = {};
	zone_reset_validators = {};
	apply_context = nullptr;
	recovery_requested = false;
	uncertain_recovery_not_before_usec = 0;
	uncertain_recovery_delay_usec = 1000000;
	critical_command_journal_shutdown();
	return true;
}

critical_submit_result critical_command_coordinator_submit_internal(critical_command command,
								    bool retain_until_publication)
{
	if (native_transport_command(command))
		return critical_submit_result::
			invalid; // Typed native routes require their original carrier.
	// The policy must be in the immutable journal bytes before admission/fsync.
	// A direct submission cannot downgrade a caller-supplied publication hold.
	if (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
	{
		if (command.publication_required && !retain_until_publication)
			return critical_submit_result::invalid;
		command.publication_required = retain_until_publication;
	}
	else if (command.publication_required)
		return critical_submit_result::invalid;
	const bool supplied_acceptance_time = command.accepted_at_usec != 0;
	if (!supplied_acceptance_time)
		command.accepted_at_usec = wall_now_usec();
	// Frozen accounting commands are already canonical. Sorting after binding
	// would silently change the immutable admission decision.
	if (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ?
		    !critical_command_envelope_valid(command) :
		    !critical_command_normalize(&command))
		return critical_submit_result::invalid;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!execution_supported(command))
		return critical_submit_result::invalid;
	if (!health.initialized || !health.accepting || stop_requested)
		return critical_submit_result::unavailable;
	const std::string identity = operation_key(command.operation_id);
	auto completed = completed_cache.find(identity);
	if (completed != completed_cache.end())
	{
		if (!supplied_acceptance_time)
			command.accepted_at_usec = completed->second.command.accepted_at_usec;
		if (!critical_command_equal(completed->second.command, command))
			return critical_submit_result::identity_conflict;
		// An attached completed cache entry has no retained operation to publish.
		// A newly installed typed publication hold must receive definite refusal.
		if (retain_until_publication && guarded_refusal_owner(command) &&
		    player_save_execution_guard::publication_operation_held(command.operation_id))
			return critical_submit_result::invalid;
		++health.attached;
		return critical_submit_result::attached;
	}
	auto found = operations.find(identity);
	if (found != operations.end())
	{
		if (!supplied_acceptance_time)
			command.accepted_at_usec = found->second->command.accepted_at_usec;
		if (!critical_command_equal(found->second->command, command))
			return critical_submit_result::identity_conflict;
		if (retain_until_publication != found->second->retain_until_publication)
			return critical_submit_result::identity_conflict;
		++found->second->attachments;
		++health.attached;
		return critical_submit_result::attached;
	}
	std::vector<uint8_t> encoded;
	if (critical_command_encode(command, &encoded) != critical_command_codec_result::ok)
		return critical_submit_result::invalid;
	if (operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
	    encoded.size() > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
	{
		++health.overloads;
		return critical_submit_result::overloaded;
	}
	bool admission_queued = false;
	try
	{
		auto state = std::make_unique<operation_state>();
		state->command = command;
		state->retained_bytes = encoded.size();
		state->queued_at_usec = now_usec();
		state->attempt = 1;
		state->attachments = 0;
		state->phase = critical_operation_phase::awaiting_durability;
		state->retain_until_publication = retain_until_publication;
		state->admission_failure_queued = false;
		operations.emplace(identity, std::move(state));
		pending_admission.push_back(identity);
		pending_admission_bytes += encoded.size();
		admission_queued = true;
		add_fences(identity, operations.at(identity)->command);
	}
	catch (const std::bad_alloc &)
	{
		auto inserted = operations.find(identity);
		if (inserted != operations.end())
		{
			remove_fences(identity, inserted->second->command);
			operations.erase(inserted);
		}
		pending_admission.erase(std::remove(pending_admission.begin(),
						    pending_admission.end(), identity),
					pending_admission.end());
		if (admission_queued)
			pending_admission_bytes -= encoded.size();
		++health.overloads;
		return critical_submit_result::overloaded;
	}
	persistence_trace_record(
		persistence_command_trace(command, persistence_trace_stage::command_admitted));
	++health.accepted;
	update_depth();
	// The operation and its per-key fence are now retained, but the journal
	// worker must acknowledge fsync before it is moved to the execution queue.
	admission_available.notify_one();
	return critical_submit_result::awaiting_durability;
}

namespace
{
critical_submit_result native_recovery_submit(critical_native_recovery_envelope envelope)
{
	size_t retained = 0;
	if (envelope.revision != 1 ||
	    envelope.phase != critical_native_recovery_phase::execution_pending ||
	    !native_envelope_size(envelope, &retained))
		return critical_submit_result::invalid;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!native_envelope_execution_supported(envelope) ||
	    (native_birth_typed_command(envelope.command) &&
	     (!native_birth_validators_ready() || !native_birth_recovery_initial(envelope))) ||
	    (zone_reset_typed_command(envelope.command) &&
	     (!zone_reset_validators_ready() || !zone_reset_validators.initial(envelope))) ||
	    (native_auction_typed_command(envelope.command) &&
	     (!native_auction_validators_ready() || !native_auction_validators.initial(envelope))))
		return critical_submit_result::invalid;
	if (!health.initialized || !health.accepting || stop_requested ||
	    !native_replay_observer_callback)
		return critical_submit_result::unavailable;
	const auto identity = operation_key(envelope.command.operation_id);
	if (completed_cache.find(identity) != completed_cache.end())
		return critical_submit_result::
			identity_conflict; // Command-only cache cannot prove a body.
	auto found = operations.find(identity);
	if (found != operations.end())
	{
		if (!native_matches(*found->second, envelope))
			return critical_submit_result::identity_conflict;
		++found->second->attachments;
		++health.attached;
		return critical_submit_result::attached;
	}
	if (operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
	    retained > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
	{
		++health.overloads;
		return critical_submit_result::overloaded;
	}
	bool admission_queued = false;
	try
	{
		auto state = std::make_unique<operation_state>();
		state->command = std::move(envelope.command);
		state->native = std::make_unique<native_operation_context>();
		state->native->revision = envelope.revision;
		state->native->phase = envelope.phase;
		state->native->attachment = std::move(envelope.attachment);
		state->retained_bytes = retained;
		state->queued_at_usec = now_usec();
		state->attempt = 1;
		state->attachments = 0;
		state->phase = critical_operation_phase::awaiting_durability;
		state->retain_until_publication = true;
		state->admission_failure_queued = false;
		operations.emplace(identity, std::move(state));
		pending_admission.push_back(identity);
		pending_admission_bytes += retained;
		admission_queued = true;
		add_fences(identity, operations.at(identity)->command);
	}
	catch (...)
	{
		auto inserted = operations.find(identity);
		if (inserted != operations.end())
		{
			remove_fences(identity, inserted->second->command);
			operations.erase(inserted);
		}
		pending_admission.erase(std::remove(pending_admission.begin(),
						    pending_admission.end(), identity),
					pending_admission.end());
		if (admission_queued)
			pending_admission_bytes -= retained;
		++health.overloads;
		update_depth();
		return critical_submit_result::overloaded;
	}
	persistence_trace_record(persistence_command_trace(
		operations.at(identity)->command, persistence_trace_stage::command_admitted));
	++health.accepted;
	update_depth();
	admission_available.notify_one();
	return critical_submit_result::awaiting_durability;
}

bool native_context_copy(const critical_command &command, critical_native_recovery_phase phase,
			 critical_native_recovery_envelope *output) noexcept
{
	if (!output)
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(operation_key(command.operation_id));
		if (!health.initialized || stop_requested || found == operations.end() ||
		    !found->second->native || found->second->native->phase != phase ||
		    found->second->publication_checkpointing ||
		    found->second->native_context_uncertain ||
		    found->second->native_ack_uncertain ||
		    !critical_command_equal(command, found->second->command) ||
		    (phase == critical_native_recovery_phase::execution_pending ?
			     !operation_is_publication_pending(*found->second) :
			     (found->second->phase !=
				      critical_operation_phase::native_continuation_pending ||
			      !found->second->native_physical_released)))
			return false;
		auto copy = native_envelope(*found->second);
		if (zone_reset_typed_command(command) &&
		    (!zone_reset_validators_ready() || !zone_reset_validators.valid(copy)))
			return false;
		if (native_auction_typed_command(command) &&
		    (!native_auction_validators_ready() || !native_auction_validators.valid(copy)))
			return false;
		*output = std::move(copy);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool native_context_checkpoint(const critical_native_recovery_envelope &expected,
			       const critical_native_recovery_envelope *successor,
			       critical_native_recovery_phase phase,
			       uint64_t expected_generation = 0,
			       bool (*durable_transfer)(const critical_native_recovery_envelope &,
							void *) noexcept = nullptr,
			       void *transfer_context = nullptr) noexcept
{
	std::string identity;
	uint64_t generation = 0;
	operation_state *reserved_operation = nullptr;
	size_t original_retained = 0, successor_retained = 0;
	bool prior_uncertain = false;
	critical_native_recovery_envelope prepared;
	try
	{
		if (expected.phase != phase ||
		    !native_envelope_size(expected, &original_retained) ||
		    (successor && (successor->phase != phase || expected.revision == UINT64_MAX ||
				   successor->revision != expected.revision + 1 ||
				   !critical_command_equal(expected.command, successor->command) ||
				   !native_envelope_size(*successor, &successor_retained))) ||
		    (!successor && phase != critical_native_recovery_phase::continuation_pending))
			return false;
		const bool constructor_retirement =
			!successor && (native_birth_origin_command(expected.command) ||
				       zone_reset_typed_command(expected.command));
		if ((constructor_retirement && (!expected_generation || !durable_transfer)) ||
		    (durable_transfer && !constructor_retirement))
			return false;
		if (successor)
			prepared =
				*successor; // Every allocation precedes reservation and journal mutation.
		identity = operation_key(expected.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (!health.initialized || stop_requested || found == operations.end() ||
		    !native_matches(*found->second, expected) ||
		    found->second->publication_checkpointing ||
		    found->second->native_ack_uncertain || !coordinator_generation ||
		    coordinator_generation_exhausted ||
		    (expected_generation && coordinator_generation != expected_generation) ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    (phase == critical_native_recovery_phase::execution_pending ?
			     !operation_is_publication_pending(*found->second) :
			     (found->second->phase !=
				      critical_operation_phase::native_continuation_pending ||
			      !found->second->native_physical_released)))
			return false;
		if (zone_reset_typed_command(expected.command) &&
		    (!zone_reset_validators_ready() || !zone_reset_validators.valid(expected) ||
		     (successor ? !zone_reset_validators.successor(expected, prepared) :
				  !zone_reset_validators.terminal(expected))))
			return false;
		if (native_birth_typed_command(expected.command) &&
		    (!native_birth_validators_ready() || !native_birth_recovery_valid(expected) ||
		     (successor ? !native_birth_recovery_successor(expected, prepared) :
				  !native_birth_recovery_terminal(expected))))
			return false;
		if (native_auction_typed_command(expected.command) &&
		    (!native_auction_validators_ready() ||
		     !native_auction_validators.valid(expected) ||
		     (successor ? !native_auction_validators.successor(expected, prepared) :
				  !native_auction_validators.terminal(expected))))
			return false;
		if (found->second->ordinary_flat_transaction)
		{
			const size_t extra = found->second->ordinary_flat_transaction_bytes;
			if (!extra || successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
			    extra > CRITICAL_COORDINATOR_MAX_BYTES - successor_retained)
				return false;
			// Both known-pure CAS refusal and success preserve the full
			// ordinary proposal charge throughout actor publication/retire.
			original_retained = found->second->retained_bytes;
			successor_retained += extra;
		}
		if (found->second->flat_transaction)
		{
			const size_t extra = found->second->flat_transaction_bytes;
			if (!extra || successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
			    extra > CRITICAL_COORDINATOR_MAX_BYTES - successor_retained)
				return false;
			// Both known-pure CAS refusal and success preserve the full
			// original proposal charge throughout actor publication/retire.
			original_retained = found->second->retained_bytes;
			successor_retained += extra;
		}
		if (found->second->room_flat_transaction)
		{
			const size_t extra = found->second->room_flat_transaction_bytes;
			if (!extra || successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
			    extra > CRITICAL_COORDINATOR_MAX_BYTES - successor_retained)
				return false;
			// Both known-pure CAS refusal and success preserve the full
			// original ROOM proposal charge throughout actor publication/retire.
			original_retained = found->second->retained_bytes;
			successor_retained += extra;
		}
		const size_t reserved = std::max(found->second->retained_bytes, successor_retained);
		if (reserved > CRITICAL_COORDINATOR_MAX_BYTES -
				       (health.retained_bytes - found->second->retained_bytes))
			return false;
		prior_uncertain = found->second->native_context_uncertain;
		found->second->retained_bytes = reserved;
		found->second->publication_checkpointing = true;
		generation = coordinator_generation;
		reserved_operation = found->second.get();
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
		update_depth();
	}
	catch (...)
	{
		return false;
	}
	auto result = critical_command_journal_result::io_failure;
	try
	{
		// The exact terminal envelope and generation are pinned across this
		// owner callback, outside the coordinator mutex. A lost transfer reply
		// retains both journal and fences; its next exact retry must reconcile
		// the original durable record before any journal retirement is attempted.
		if (!durable_transfer || durable_transfer(expected, transfer_context))
			result = successor ?
					 critical_command_journal_replace_native_recovery(
						 expected, prepared) :
					 critical_command_journal_retire_native_recovery(expected);
	}
	catch (...)
	{
	}
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--guarded_publications_inflight;
	--publication_checkpoints_inflight;
	publication_checkpoint_finished.notify_all();
	auto found = operations.find(identity);
	// The immutable command was fully compared before reserving this exact
	// operation. No fallible encode/allocation may run after durable CAS/retire.
	if (found == operations.end() || coordinator_generation != generation ||
	    found->second.get() != reserved_operation ||
	    !found->second->publication_checkpointing || !found->second->native ||
	    found->second->native->revision != expected.revision ||
	    found->second->native->phase != expected.phase ||
	    found->second->native->attachment != expected.attachment)
		return false;
	auto &state = *found->second;
	state.publication_checkpointing = false;
	if (result != critical_command_journal_result::ok)
	{
		state.native_context_uncertain =
			prior_uncertain ||
			result == critical_command_journal_result::append_uncertain;
		if (!state.native_context_uncertain)
			state.retained_bytes = original_retained;
		update_depth();
		return false;
	}
	if (successor)
	{
		state.native->revision = prepared.revision;
		state.native->attachment = std::move(prepared.attachment);
		state.retained_bytes = successor_retained;
		state.native_context_uncertain = false;
	}
	else
	{
		// Release the birth advancement gate only after confirmed original
		// transfer AND journal retirement. Existing non-birth phase2 owners have
		// already released their fences at their own original physical ACK.
		if (native_birth_origin_command(state.command) ||
		    zone_reset_typed_command(state.command))
			remove_fences(identity, state.command);
		// No command-only completed cache can establish original body identity.
		operations.erase(found);
		++health.completed;
	}
	update_depth();
	work_available.notify_all();
	return true;
}
} // namespace

critical_submit_result
critical_native_auction_submission_owner::submit(critical_native_recovery_envelope envelope)
{
	if (!native_auction_typed_command(envelope.command))
		return critical_submit_result::invalid;
	return native_recovery_submit(std::move(envelope));
}

bool critical_native_auction_publication_owner::copy_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
	if (!output || !native_auction_typed_command(command))
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(operation_key(command.operation_id));
		if (!health.initialized || stop_requested || !coordinator_generation ||
		    coordinator_generation_exhausted || found == operations.end() ||
		    !found->second->native || found->second->publication_checkpointing ||
		    found->second->native_context_uncertain ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !critical_command_equal(command, found->second->command) ||
		    !native_auction_validators_ready())
			return false;
		const auto &state = *found->second;
		// A durable phase2 rewrite can precede exact player-hold consumption.
		// This is still held physical publication, never notice authority.
		const bool held_execution =
			state.native->phase == critical_native_recovery_phase::execution_pending &&
			operation_is_publication_pending(state) && !state.native_physical_released;
		const bool held_ack_retry =
			state.native->phase ==
				critical_native_recovery_phase::continuation_pending &&
			state.phase == critical_operation_phase::native_continuation_pending &&
			!state.native_physical_released;
		if (!held_execution && !held_ack_retry)
			return false;
		auto original = native_envelope(state);
		if (!native_auction_validators.valid(original) ||
		    ((held_ack_retry || state.native_ack_uncertain) &&
		     !native_auction_validators.publication(original,
							    state.publication_completion)))
			return false;
		*output = std::move(original);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_native_auction_publication_owner::checkpoint_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	if (!native_auction_typed_command(expected.command))
		return false;
	return native_context_checkpoint(expected, &successor,
					 critical_native_recovery_phase::execution_pending);
}

bool critical_native_auction_continuation_owner::copy_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
	if (!native_auction_typed_command(command))
		return false;
	return native_context_copy(command, critical_native_recovery_phase::continuation_pending,
				   output);
}

bool critical_native_auction_continuation_owner::checkpoint_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	if (!native_auction_typed_command(expected.command))
		return false;
	return native_context_checkpoint(expected, &successor,
					 critical_native_recovery_phase::continuation_pending);
}

bool critical_native_auction_continuation_owner::retire_continuation(
	const critical_native_recovery_envelope &expected) noexcept
{
	if (!native_auction_typed_command(expected.command))
		return false;
	return native_context_checkpoint(expected, nullptr,
					 critical_native_recovery_phase::continuation_pending);
}

critical_submit_result critical_native_auction_background_publication_owner::submit(
	critical_native_recovery_envelope envelope)
{
	if (!native_auction_background_command(envelope.command))
		return critical_submit_result::invalid;
	return native_recovery_submit(std::move(envelope));
}

bool critical_native_auction_background_publication_owner::copy_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
	if (!native_auction_background_command(command))
		return false;
	// Held phase1 includes an authenticated exact uncertain-ACK retry. Phase2
	// is available only after the real native physical boundary was released.
	return critical_native_auction_publication_owner::copy_context(command, output) ||
	       native_context_copy(command, critical_native_recovery_phase::continuation_pending,
				   output);
}

bool critical_native_auction_background_publication_owner::checkpoint_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	if (!native_auction_background_command(expected.command))
		return false;
	return native_context_checkpoint(expected, &successor,
					 critical_native_recovery_phase::execution_pending);
}

bool critical_native_auction_background_publication_owner::observe_generation(
	const critical_native_recovery_envelope &expected, uint64_t *output) noexcept
{
	if (!output || !native_auction_background_command(expected.command))
		return false;
	try
	{
		const auto identity = operation_key(expected.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (found == operations.end() || !found->second->retain_until_publication ||
		    !native_matches(*found->second, expected) || !health.initialized ||
		    !coordinator_generation || coordinator_generation_exhausted || stop_requested ||
		    found->second->publication_checkpointing ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()))
			return false;
		*output = coordinator_generation;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_native_auction_background_publication_owner::cancel_refusal(
	const critical_native_recovery_envelope &original, const critical_completion &expected,
	uint64_t generation,
	bool (*native_cleanup)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
	if (!native_cleanup || !generation ||
	    !native_auction_background_command(original.command) ||
	    expected.operation_id.bytes != original.command.operation_id.bytes ||
	    !critical_completion_disposition_valid(expected) ||
	    expected.disposition != critical_completion_disposition::never_admitted)
		return false;
	std::string identity;
	critical_native_recovery_envelope frozen;
	critical_completion receipt = expected;
	operation_state *pinned = nullptr;
	const auto matches = [&](const operation_state &state) noexcept
	{
		return operation_is_admission_failed(state) && state.owned_refusal_delivered &&
		       state.retain_until_publication && state.native &&
		       state.native->revision == frozen.revision &&
		       state.native->phase == frozen.phase &&
		       state.native->attachment == frozen.attachment &&
		       native_receipt_equal(state.admission_failure_completion, receipt);
	};
	try
	{
		// Cleanup may destroy the caller's native owner. Copies and canonical
		// command comparison must finish before pinning and invoking that callback.
		frozen = original;
		identity = operation_key(frozen.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || found->second->publication_checkpointing ||
		    !health.initialized || coordinator_generation != generation ||
		    coordinator_generation_exhausted || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !matches(*found->second) || !native_matches(*found->second, frozen) ||
		    !native_auction_validators_ready() ||
		    !native_auction_validators.initial(frozen))
			return false;
		pinned = found->second.get();
		pinned->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	const bool cleaned = native_cleanup(frozen.command, receipt, context);
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--publication_checkpoints_inflight;
	--guarded_publications_inflight;
	publication_checkpoint_finished.notify_all();
	auto found = operations.find(identity);
	if (found == operations.end() || coordinator_generation != generation ||
	    found->second.get() != pinned)
		return false;
	const bool same = found->second->publication_checkpointing && matches(*found->second);
	found->second->publication_checkpointing = false;
	if (!cleaned || !same || !health.initialized || coordinator_generation_exhausted ||
	    stop_requested ||
	    (lifecycle_guard_active && lifecycle_guard_thread != std::this_thread::get_id()))
		return false;
	// This exact delivered never-admitted owner has no durable frame to retire.
	remove_fences(identity, found->second->command);
	operations.erase(found);
	update_depth();
	work_available.notify_all();
	return true;
}

bool critical_native_auction_background_publication_owner::acknowledge(
	const critical_native_recovery_envelope &expected, const critical_completion &receipt,
	uint64_t generation) noexcept
{
	if (!generation || !native_auction_background_command(expected.command) ||
	    expected.phase != critical_native_recovery_phase::execution_pending ||
	    expected.revision == UINT64_MAX ||
	    receipt.operation_id.bytes != expected.command.operation_id.bytes ||
	    !critical_completion_disposition_valid(receipt) ||
	    (receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied &&
	     receipt.outcome != critical_apply_outcome::terminal_failure) ||
	    receipt.disposition != critical_completion_disposition::execution ||
	    receipt.result_size != AUCTION_RESULT_PAYLOAD_BYTES)
		return false;
	std::string identity;
	critical_native_recovery_envelope frozen, successor;
	operation_state *pinned = nullptr;
	bool prior_uncertain = false;
	try
	{
		// Every fallible copy/encoding precedes the pinned journal CAS.
		frozen = expected;
		successor = frozen;
		++successor.revision;
		successor.phase = critical_native_recovery_phase::continuation_pending;
		identity = operation_key(frozen.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || !health.initialized || stop_requested ||
		    coordinator_generation != generation || coordinator_generation_exhausted ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !operation_is_publication_pending(*found->second) ||
		    !found->second->retain_until_publication ||
		    found->second->publication_checkpointing ||
		    found->second->native_context_uncertain ||
		    !native_matches(*found->second, frozen) ||
		    !native_receipt_equal(found->second->publication_completion, receipt) ||
		    !native_auction_validators_ready() ||
		    !native_auction_validators.publication(frozen, receipt) ||
		    !native_auction_validators.successor(frozen, successor))
			return false;
		prior_uncertain = found->second->native_ack_uncertain;
		pinned = found->second.get();
		pinned->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	auto result = critical_command_journal_result::io_failure;
	try
	{
		result = critical_command_journal_replace_native_recovery(frozen, successor);
	}
	catch (...)
	{
	}
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--publication_checkpoints_inflight;
	--guarded_publications_inflight;
	publication_checkpoint_finished.notify_all();
	auto found = operations.find(identity);
	// No allocation, command encoding or owner callback after durable CAS.
	if (found == operations.end() || coordinator_generation != generation ||
	    found->second.get() != pinned || !found->second->publication_checkpointing ||
	    !operation_is_publication_pending(*found->second) || !found->second->native ||
	    found->second->native->revision != frozen.revision ||
	    found->second->native->phase != frozen.phase ||
	    found->second->native->attachment != frozen.attachment)
		return false;
	auto &state = *found->second;
	state.publication_checkpointing = false;
	if (result != critical_command_journal_result::ok)
	{
		state.native_ack_uncertain =
			prior_uncertain ||
			result == critical_command_journal_result::append_uncertain;
		return false;
	}
	state.native->revision = successor.revision;
	state.native->phase = successor.phase;
	state.phase = critical_operation_phase::native_continuation_pending;
	state.native_ack_uncertain = false;
	state.native_physical_released = true;
	remove_fences(identity, state.command);
	// Native background auction consumes no PC hold or command-only cache entry.
	// Preserve the original envelope until genuine after-ACK notice retirement.
	update_depth();
	work_available.notify_all();
	return true;
}

critical_submit_result
critical_held_retirement_submission_owner::submit(critical_native_recovery_envelope envelope)
{
	held_retirement_recovery original;
	if (!held_retirement_command_identity(envelope.command) ||
	    !held_retirement_recovery_decode(envelope.command, envelope.attachment, &original) ||
	    original.receipt.present || original.physical_stage)
		return critical_submit_result::invalid;
	return native_recovery_submit(std::move(envelope));
}

bool critical_held_retirement_publication_owner::copy_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
	if (!held_retirement_command_identity(command))
		return false;
	return native_context_copy(command, critical_native_recovery_phase::execution_pending,
				   output);
}

bool critical_held_retirement_publication_owner::copy_publication(
	const critical_command &command, const critical_completion &completion,
	held_retirement_publication_snapshot *output) noexcept
try
{
	if (!output || !held_retirement_command_identity(command) ||
	    !critical_completion_disposition_valid(completion))
		return false;
	const auto identity = operation_key(command.operation_id);
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	const auto found = operations.find(identity);
	if (!health.initialized || stop_requested || !coordinator_generation ||
	    coordinator_generation_exhausted || found == operations.end() ||
	    (lifecycle_guard_active && lifecycle_guard_thread != std::this_thread::get_id()))
		return false;
	const auto &state = *found->second;
	if (!state.native || state.publication_checkpointing || state.native_context_uncertain ||
	    state.native->phase != critical_native_recovery_phase::execution_pending ||
	    !critical_command_equal(state.command, command) || !state.retain_until_publication)
		return false;
	held_retirement_publication_snapshot prepared;
	if (operation_is_admission_failed(state))
	{
		if (!state.owned_refusal_delivered || state.native_ack_uncertain ||
		    !native_receipt_equal(state.admission_failure_completion, completion) ||
		    completion.disposition != critical_completion_disposition::never_admitted)
			return false;
		prepared.mode = held_retirement_publication_mode::original_refusal;
	}
	else
	{
		if (!operation_is_publication_pending(state) ||
		    !native_receipt_equal(state.publication_completion, completion) ||
		    completion.disposition != critical_completion_disposition::execution)
			return false;
		prepared.mode = state.native_ack_uncertain ?
					held_retirement_publication_mode::terminal_ack_retry :
					held_retirement_publication_mode::execution;
	}
	prepared.envelope = native_envelope(state);
	if (!held_retirement_publication_snapshot_valid(command, completion, prepared))
		return false;
	*output = std::move(prepared);
	return true;
}
catch (...)
{
	return false;
}

bool critical_held_retirement_publication_owner::checkpoint_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	if (!held_retirement_recovery_transition_valid(expected, successor))
		return false;
	return native_context_checkpoint(expected, &successor,
					 critical_native_recovery_phase::execution_pending);
}

critical_submit_result
critical_native_quest_submission_owner::submit(critical_native_recovery_envelope envelope)
{
	if (!native_quest_context_command(envelope.command))
		return critical_submit_result::invalid;
	return native_recovery_submit(std::move(envelope));
}

bool critical_native_quest_publication_owner::copy_fee_acceptance_context(
	const critical_command &actual_fee,
	critical_native_recovery_envelope *original_parent) noexcept
{
	try
	{
		item_transfer_payload fee{};
		std::vector<uint8_t> frame;
		if (!original_parent || !native_fee_context_command(actual_fee) ||
		    !item_transfer_command_decode_payload(actual_fee, &fee) ||
		    !fee.native_cost.fee_only || !fee.native_recovery.present ||
		    critical_command_encode(actual_fee, &frame) !=
			    critical_command_codec_result::ok)
			return false;
		const auto identity = operation_key(actual_fee.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (!health.initialized || stop_requested || !coordinator_generation ||
		    coordinator_generation_exhausted ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    found == operations.end())
			return false;
		const auto &state = *found->second;
		if (!state.native || !critical_command_equal(state.command, actual_fee) ||
		    state.publication_checkpointing || state.native_context_uncertain)
			return false;
		const bool executing = state.phase == critical_operation_phase::queued ||
				       state.phase == critical_operation_phase::executing;
		const bool publishing = state.phase ==
					critical_operation_phase::publication_pending;
		const bool released = state.phase ==
				      critical_operation_phase::native_continuation_pending;
		const bool ack_retry = (publishing && state.native_ack_uncertain) ||
				       (released && !state.native_physical_released);
		if ((!executing && !publishing && !released) ||
		    (state.native_ack_uncertain && !publishing) ||
		    (!released && state.native_physical_released) ||
		    state.native->phase !=
			    (released ? critical_native_recovery_phase::continuation_pending :
					critical_native_recovery_phase::execution_pending))
			return false;
		if (publishing || ack_retry)
		{
			const auto &receipt = state.publication_completion;
			item_native_mobile_fee_result result{}, expected{};
			if (!critical_completion_disposition_valid(receipt) ||
			    receipt.disposition != critical_completion_disposition::execution ||
			    receipt.operation_id.bytes != actual_fee.operation_id.bytes ||
			    receipt.failure_stage != critical_failure_stage::none ||
			    (receipt.outcome != critical_apply_outcome::applied &&
			     receipt.outcome != critical_apply_outcome::already_applied &&
			     receipt.outcome != critical_apply_outcome::terminal_failure) ||
			    ((receipt.outcome == critical_apply_outcome::terminal_failure) !=
			     (receipt.error_code != 0)) ||
			    receipt.result_size != ITEM_TRANSFER_NATIVE_MOBILE_FEE_RESULT_BYTES ||
			    !item_native_mobile_fee_result_decode({ receipt.result_payload.data(),
								    receipt.result_size },
								  &result) ||
			    !item_native_mobile_fee_result_build(fee, &expected))
				return false;
			if (receipt.outcome == critical_apply_outcome::terminal_failure)
			{
				expected.mobile_cash_revision =
					fee.native_cost.projection.before_revision;
				expected.mobile_revision =
					fee.native_mobile.reference.mobile_revision;
			}
			if (result != expected ||
			    receipt.durable_revision !=
				    std::max({ result.mobile_cash_revision, result.mobile_revision,
					       result.stock_revision,
					       result.native_custody_revision,
					       result.player_custody_revision }) ||
			    !std::all_of(receipt.result_payload.begin() + receipt.result_size,
					 receipt.result_payload.end(),
					 [](uint8_t byte) { return byte == 0; }))
				return false;
		}
		auto child = native_envelope(state);
		if (ack_retry && !(released ? native_quest_recovery_fee_ack_context_valid(
						      child, state.publication_completion) :
					      native_quest_recovery_publication_context_valid(
						      child, state.publication_completion)))
			return false;
		size_t child_size = 0;
		native_quest_recovery_context child_context;
		if (!native_envelope_size(child, &child_size) ||
		    native_quest_recovery_context_decode(actual_fee, child.attachment,
							 &child_context) !=
			    player_snapshot_codec_result::ok ||
		    (executing && (child_context.receipt.present ||
				   child_context.publication_stage !=
					   native_quest_recovery_publication_stage::captured)) ||
		    (released &&
		     child_context.publication_stage !=
			     native_quest_recovery_publication_stage::physically_proven))
			return false;
		critical_native_recovery_envelope selected;
		bool have_parent = false;
		for (const auto &[key, entry] : operations)
		{
			const auto &candidate = *entry;
			if (key == identity || !candidate.native ||
			    !native_v12_command(candidate.command) ||
			    candidate.phase !=
				    critical_operation_phase::native_continuation_pending ||
			    !candidate.native_physical_released ||
			    candidate.native->phase !=
				    critical_native_recovery_phase::continuation_pending)
				continue;
			auto parent = native_envelope(candidate);
			native_quest_recovery_context context;
			size_t parent_size = 0;
			if (!native_envelope_size(parent, &parent_size) ||
			    native_quest_recovery_context_decode(parent.command, parent.attachment,
								 &context) !=
				    player_snapshot_codec_result::ok)
				return false;
			if (context.next_child_operation.bytes != actual_fee.operation_id.bytes ||
			    context.next_child_command != frame)
				continue;
			// Canonical parent decoding proves this exact child's original native
			// lifetime and giver. Parent handoff1 precedes actual child admission.
			if (have_parent || candidate.publication_checkpointing ||
			    candidate.native_ack_uncertain || candidate.native_context_uncertain ||
			    (context.child_handoff_stage != 1 &&
			     context.child_handoff_stage != 2) ||
			    !context.branch_program_frozen ||
			    context.next_branch >= context.branches.size() ||
			    fee.native_cost.completion_slot != context.next_branch ||
			    context.publication_stage !=
				    native_quest_recovery_publication_stage::physically_proven ||
			    !context.receipt.present || context.receipt.error_code ||
			    context.receipt.failure_stage != critical_failure_stage::none ||
			    (context.receipt.outcome != critical_apply_outcome::applied &&
			     context.receipt.outcome != critical_apply_outcome::already_applied) ||
			    context.receipt.result_size != ITEM_TRANSFER_RESULT_BYTES)
				return false;
			if (fee.continuation.kind ==
			    item_transfer_continuation_kind::quest_offering)
			{
				quest_reward_continuation terms;
				if (!quest_reward_continuation_decode(fee.continuation.data.data(),
								      fee.continuation.data.size(),
								      &terms) ||
				    !quest_fee_reward_trigger_binding_valid(terms,
									    parent.command) ||
				    terms.action_operation.bytes != actual_fee.operation_id.bytes ||
				    !std::equal(terms.original_acceptance_result.begin(),
						terms.original_acceptance_result.end(),
						context.receipt.result_payload.begin()))
					return false;
			}
			else if (fee.continuation.kind != item_transfer_continuation_kind::none)
				return false;
			selected = std::move(parent);
			have_parent = true;
		}
		if (!have_parent)
			return false;
		static_assert(
			noexcept(std::declval<critical_native_recovery_envelope &>() =
					 std::declval<critical_native_recovery_envelope &&>()));
		*original_parent = std::move(selected);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_native_quest_publication_owner::copy_fee_ack_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
try
{
	if (!output || !native_fee_context_command(command))
		return false;
	const auto identity = operation_key(command.operation_id);
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	const auto found = operations.find(identity);
	if (!health.initialized || stop_requested || !coordinator_generation ||
	    coordinator_generation_exhausted || found == operations.end() ||
	    (lifecycle_guard_active && lifecycle_guard_thread != std::this_thread::get_id()))
		return false;
	const auto &state = *found->second;
	if (!state.native || !state.retain_until_publication || state.publication_checkpointing ||
	    state.native_context_uncertain || state.native_physical_released ||
	    !critical_command_equal(state.command, command))
		return false;
	const bool uncertain = state.phase == critical_operation_phase::publication_pending &&
			       state.native_ack_uncertain &&
			       state.native->phase ==
				       critical_native_recovery_phase::execution_pending;
	const bool transitioned =
		state.phase == critical_operation_phase::native_continuation_pending &&
		!state.native_ack_uncertain &&
		state.native->phase == critical_native_recovery_phase::continuation_pending;
	if (!uncertain && !transitioned)
		return false;
	auto envelope = native_envelope(state);
	size_t bytes = 0;
	if (!native_envelope_size(envelope, &bytes) ||
	    !(transitioned ? native_quest_recovery_fee_ack_context_valid(
				     envelope, state.publication_completion) :
			     native_quest_recovery_publication_context_valid(
				     envelope, state.publication_completion)))
		return false;
	*output = std::move(envelope);
	return true;
}
catch (...)
{
	return false;
}

bool critical_native_quest_publication_owner::copy_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
	if (!native_quest_context_command(command))
		return false;
	return native_context_copy(command, critical_native_recovery_phase::execution_pending,
				   output) ||
	       copy_fee_ack_context(command, output);
}

bool critical_native_quest_publication_owner::checkpoint_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	if (!native_quest_context_command(expected.command))
		return false;
	return native_context_checkpoint(expected, &successor,
					 critical_native_recovery_phase::execution_pending);
}

bool critical_native_quest_continuation_owner::copy_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
	if (!native_quest_context_command(command))
		return false;
	return native_context_copy(command, critical_native_recovery_phase::continuation_pending,
				   output);
}

bool critical_native_quest_continuation_owner::copy_parent_context(
	const critical_native_recovery_envelope &child,
	critical_native_recovery_envelope *output) noexcept
{
	try
	{
		size_t child_size = 0;
		if (!output || !native_quest_context_command(child.command) ||
		    child.phase != critical_native_recovery_phase::continuation_pending ||
		    !native_envelope_size(child, &child_size))
			return false;
		const auto child_key = operation_key(child.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto child_found = operations.find(child_key);
		if (!health.initialized || stop_requested || !coordinator_generation ||
		    coordinator_generation_exhausted || !native_quest_pair_validator ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    child_found == operations.end())
			return false;
		const auto &child_state = *child_found->second;
		if (!native_matches(child_state, child) || child_state.publication_checkpointing ||
		    child_state.native_ack_uncertain ||
		    child_state.phase != critical_operation_phase::native_continuation_pending ||
		    !child_state.native_physical_released)
			return false;
		critical_native_recovery_envelope selected;
		bool have_parent = false;
		// The original admission limits bound this map. The lock retains both
		// operation lifetimes and coordinator generation throughout the scan/copy.
		for (const auto &[identity, entry] : operations)
		{
			const auto &parent_state = *entry;
			if (identity == child_key || !parent_state.native ||
			    !native_v12_command(parent_state.command) ||
			    parent_state.command.operation_id.bytes ==
				    child.command.operation_id.bytes ||
			    parent_state.native->phase !=
				    critical_native_recovery_phase::continuation_pending ||
			    parent_state.phase !=
				    critical_operation_phase::native_continuation_pending ||
			    !parent_state.native_physical_released)
				continue;
			auto parent = native_envelope(parent_state);
			size_t parent_size = 0;
			if (!native_matches(parent_state, parent) ||
			    !native_envelope_size(parent, &parent_size))
				return false;
			if (!native_quest_pair_validator(parent, child, nullptr))
				continue;
			if (have_parent || parent_state.publication_checkpointing ||
			    parent_state.native_ack_uncertain)
				return false;
			selected = std::move(parent);
			have_parent = true;
		}
		if (!have_parent)
			return false;
		// Every fallible copy/check finishes before publishing the local value.
		// native_context_uncertain is intentionally preserved, not cleared or
		// used as absence authority: the paired journal owner must settle it.
		static_assert(
			noexcept(std::declval<critical_native_recovery_envelope &>() =
					 std::declval<critical_native_recovery_envelope &&>()));
		*output = std::move(selected);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_native_quest_continuation_owner::copy_fee_obligation_context(
	const critical_operation_id &actual_action, std::span<const uint8_t> literal_continuation,
	critical_native_recovery_envelope *actual_child,
	critical_native_recovery_envelope *original_parent) noexcept
{
	try
	{
		if (!actual_child || !original_parent || actual_child == original_parent ||
		    critical_operation_id_is_zero(actual_action) || literal_continuation.empty() ||
		    literal_continuation.size() > ITEM_TRANSFER_CONTINUATION_MAX_BYTES)
			return false;
		const auto identity = operation_key(actual_action);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (!health.initialized || stop_requested || !coordinator_generation ||
		    coordinator_generation_exhausted || !native_quest_pair_validator ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    found == operations.end())
			return false;
		const auto &state = *found->second;
		if (!state.native || !native_fee_context_command(state.command) ||
		    state.phase != critical_operation_phase::native_continuation_pending ||
		    !state.native_physical_released || state.publication_checkpointing ||
		    state.native_ack_uncertain || state.native_context_uncertain)
			return false;
		item_transfer_payload payload{};
		if (!item_transfer_command_decode_payload(state.command, &payload) ||
		    !payload.native_cost.fee_only || !payload.native_recovery.present ||
		    payload.continuation.kind != item_transfer_continuation_kind::quest_offering ||
		    payload.continuation.data.size() != literal_continuation.size() ||
		    !std::equal(literal_continuation.begin(), literal_continuation.end(),
				payload.continuation.data.begin()))
			return false;
		auto child = native_envelope(state);
		size_t child_size = 0;
		if (child.phase != critical_native_recovery_phase::continuation_pending ||
		    !native_matches(state, child) || !native_envelope_size(child, &child_size))
			return false;
		critical_native_recovery_envelope parent;
		bool selected = false;
		for (const auto &[key, entry] : operations)
		{
			const auto &candidate = *entry;
			if (key == identity || !candidate.native ||
			    !native_v12_command(candidate.command) ||
			    candidate.phase !=
				    critical_operation_phase::native_continuation_pending ||
			    !candidate.native_physical_released)
				continue;
			auto original = native_envelope(candidate);
			size_t parent_size = 0;
			if (original.phase !=
				    critical_native_recovery_phase::continuation_pending ||
			    !native_matches(candidate, original) ||
			    !native_envelope_size(original, &parent_size))
				return false;
			if (!native_quest_pair_validator(original, child, nullptr))
				continue;
			if (selected || candidate.publication_checkpointing ||
			    candidate.native_ack_uncertain || candidate.native_context_uncertain)
				return false;
			parent = std::move(original);
			selected = true;
		}
		if (!selected)
			return false;
		static_assert(
			noexcept(std::declval<critical_native_recovery_envelope &>() =
					 std::declval<critical_native_recovery_envelope &&>()));
		*actual_child = std::move(child);
		*original_parent = std::move(parent);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_native_quest_continuation_owner::checkpoint_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	if (!native_quest_context_command(expected.command))
		return false;
	return native_context_checkpoint(expected, &successor,
					 critical_native_recovery_phase::continuation_pending);
}

bool critical_native_quest_continuation_owner::retire_continuation(
	const critical_native_recovery_envelope &expected) noexcept
{
	if (!native_quest_context_command(expected.command))
		return false;
	return native_context_checkpoint(expected, nullptr,
					 critical_native_recovery_phase::continuation_pending);
}

bool critical_native_quest_continuation_owner::transition_pair(
	const critical_native_recovery_envelope &parent,
	const critical_native_recovery_envelope &child,
	const critical_native_recovery_envelope *successor) noexcept
{
	std::string parent_key, child_key;
	uint64_t generation = 0;
	operation_state *parent_pin = nullptr, *child_pin = nullptr;
	size_t parent_size = 0, child_size = 0, successor_size = 0;
	size_t prior_parent_retained = 0;
	bool prior_uncertain = false;
	critical_native_recovery_envelope prepared;
	try
	{
		if (!native_v12_command(parent.command) ||
		    !native_quest_context_command(child.command) ||
		    parent.command.operation_id.bytes == child.command.operation_id.bytes ||
		    parent.phase != critical_native_recovery_phase::continuation_pending ||
		    child.phase != critical_native_recovery_phase::continuation_pending ||
		    !native_envelope_size(parent, &parent_size) ||
		    !native_envelope_size(child, &child_size) ||
		    (successor &&
		     (parent.revision == UINT64_MAX || successor->revision != parent.revision + 1 ||
		      successor->phase != parent.phase ||
		      !critical_command_equal(parent.command, successor->command) ||
		      !native_envelope_size(*successor, &successor_size))))
			return false;
		if (successor)
			prepared = *successor;
		parent_key = operation_key(parent.command.operation_id);
		child_key = operation_key(child.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto parent_found = operations.find(parent_key);
		const auto child_found = operations.find(child_key);
		if (!health.initialized || stop_requested || !coordinator_generation ||
		    coordinator_generation_exhausted || !native_quest_pair_validator ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    parent_found == operations.end() || child_found == operations.end())
			return false;
		auto &parent_state = *parent_found->second;
		auto &child_state = *child_found->second;
		if (!native_matches(parent_state, parent) || !native_matches(child_state, child) ||
		    parent_state.publication_checkpointing ||
		    child_state.publication_checkpointing || parent_state.native_ack_uncertain ||
		    child_state.native_ack_uncertain ||
		    parent_state.phase != critical_operation_phase::native_continuation_pending ||
		    child_state.phase != critical_operation_phase::native_continuation_pending ||
		    !parent_state.native_physical_released ||
		    !child_state.native_physical_released ||
		    !native_quest_pair_validator(parent, child, successor ? &prepared : nullptr))
			return false;
		const size_t reserved = std::max(parent_state.retained_bytes, successor_size);
		if (health.retained_bytes < parent_state.retained_bytes ||
		    reserved > CRITICAL_COORDINATOR_MAX_BYTES -
				       (health.retained_bytes - parent_state.retained_bytes))
			return false;
		prior_parent_retained = parent_state.retained_bytes;
		prior_uncertain = parent_state.native_context_uncertain ||
				  child_state.native_context_uncertain;
		parent_state.retained_bytes = reserved;
		parent_state.publication_checkpointing = child_state.publication_checkpointing =
			true;
		parent_pin = &parent_state;
		child_pin = &child_state;
		generation = coordinator_generation;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
		update_depth();
	}
	catch (...)
	{
		return false;
	}
	auto result = critical_command_journal_result::io_failure;
	try
	{
		result = critical_command_journal_transition_native_pair(
			parent, child, successor ? &prepared : nullptr);
	}
	catch (...)
	{
	}
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--guarded_publications_inflight;
	--publication_checkpoints_inflight;
	publication_checkpoint_finished.notify_all();
	const auto parent_found = operations.find(parent_key);
	const auto child_found = operations.find(child_key);
	// Both immutable commands were fully compared before reservation. After I/O
	// only pointer/scalar/vector equality, noexcept moves and erases are allowed.
	if (coordinator_generation != generation || parent_found == operations.end() ||
	    child_found == operations.end() || parent_found->second.get() != parent_pin ||
	    child_found->second.get() != child_pin || !parent_pin->native || !child_pin->native ||
	    !parent_pin->publication_checkpointing || !child_pin->publication_checkpointing ||
	    parent_pin->native->revision != parent.revision ||
	    child_pin->native->revision != child.revision ||
	    parent_pin->native->phase != parent.phase || child_pin->native->phase != child.phase ||
	    parent_pin->native->attachment != parent.attachment ||
	    child_pin->native->attachment != child.attachment)
		return false;
	parent_pin->publication_checkpointing = child_pin->publication_checkpointing = false;
	if (result != critical_command_journal_result::ok)
	{
		const bool uncertain = prior_uncertain ||
				       result == critical_command_journal_result::append_uncertain;
		parent_pin->native_context_uncertain = child_pin->native_context_uncertain =
			uncertain;
		if (!uncertain)
			parent_pin->retained_bytes = prior_parent_retained;
		update_depth();
		return false;
	}
	if (successor)
	{
		parent_pin->native->revision = prepared.revision;
		parent_pin->native->attachment = std::move(prepared.attachment);
		parent_pin->retained_bytes = successor_size;
		parent_pin->native_context_uncertain = false;
	}
	else
	{
		operations.erase(parent_found);
		++health.completed;
	}
	operations.erase(child_found); // unordered_map erase does not invalidate other iterators.
	++health.completed;
	update_depth();
	work_available.notify_all();
	return true;
}

critical_submit_result critical_command_coordinator_submit(critical_command command)
{
	return critical_command_coordinator_submit_internal(std::move(command), false);
}

critical_submit_result critical_command_coordinator_submit_for_publication(critical_command command)
{
	return critical_command_coordinator_submit_internal(std::move(command), true);
}

critical_command_durability
critical_command_coordinator_durability(const critical_operation_id &operation_id)
{
	if (critical_operation_id_is_zero(operation_id))
		return critical_command_durability::unknown;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	const std::string identity = operation_key(operation_id);
	if (completed_cache.find(identity) != completed_cache.end())
		return critical_command_durability::durable;
	auto found = operations.find(identity);
	if (found == operations.end())
		return critical_command_durability::unknown;
	if (operation_is_admission_failed(*found->second))
		return critical_command_durability::failed;
	if (operation_is_uncertain(*found->second))
		return critical_command_durability::uncertain;
	if (operation_is_awaiting_durability(*found->second))
		return critical_command_durability::awaiting_durability;
	return critical_command_durability::durable;
}

bool critical_command_coordinator_recover_uncertain(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!health.initialized || stop_requested)
		return false;
	bool uncertain = false;
	for (const auto &[identity, state] : operations)
	{
		(void)identity;
		uncertain = uncertain || operation_is_uncertain(*state);
	}
	if (!uncertain)
	{
		recovery_requested = false;
		uncertain_recovery_not_before_usec = 0;
		uncertain_recovery_delay_usec = 1000000;
		return true;
	}
	recovery_requested = true;
	uncertain_recovery_not_before_usec = 0;
	admission_available.notify_one();
	return true;
}

bool recovery_due()
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!health.initialized || stop_requested)
		return false;
	bool uncertain = false;
	for (const auto &[identity, state] : operations)
	{
		(void)identity;
		uncertain = uncertain || operation_is_uncertain(*state);
	}
	if (!uncertain || !recovery_due_locked())
		return false;
	recovery_requested = true;
	return true;
}

bool critical_command_coordinator_get_completed(const critical_operation_id &operation_id,
						critical_completion *completion)
{
	if (!completion || critical_operation_id_is_zero(operation_id))
		return false;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	const std::string identity = operation_key(operation_id);
	auto operation = operations.find(identity);
	if (operation != operations.end() && operation_is_publication_pending(*operation->second))
	{
		*completion = operation->second->publication_completion;
		return true;
	}
	const auto found = completed_cache.find(identity);
	if (found == completed_cache.end())
		return false;
	*completion = found->second.completion;
	return true;
}

bool critical_command_coordinator_acknowledge_publication(const critical_operation_id &operation_id)
{
	if (critical_operation_id_is_zero(operation_id))
		return false;
	std::string identity;
	try
	{
		identity = operation_key(operation_id);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() ||
		    !operation_is_publication_pending(*found->second) || found->second->native ||
		    found->second->command.type == critical_command_type::native_mobile_birth ||
		    zone_reset_typed_command(found->second->command) ||
		    found->second->publication_checkpointing ||
		    player_save_execution_guard::publication_operation_held(operation_id))
			return false;
		found->second->publication_checkpointing = true;
		++publication_checkpoints_inflight;
	}

	critical_command_journal_result checkpoint = critical_command_journal_result::io_failure;
	try
	{
		checkpoint = critical_command_journal_checkpoint(operation_id);
	}
	catch (...)
	{
		// Leave the journal entry and publication fence available for retry.
	}

	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--publication_checkpoints_inflight;
	publication_checkpoint_finished.notify_all();
	auto found = operations.find(identity);
	if (found == operations.end() || !operation_is_publication_pending(*found->second))
		return false;
	operation_state &state = *found->second;
	state.publication_checkpointing = false;
	auto trace =
		persistence_command_trace(state.command, persistence_trace_stage::publication_ack);
	trace.outcome = static_cast<uint32_t>(checkpoint);
	persistence_trace_record(trace);
	if (checkpoint != critical_command_journal_result::ok)
		return false;
	remove_fences(identity, state.command);
	remember_completed(identity, state.command, state.publication_completion);
	operations.erase(found);
	++health.completed;
	update_depth();
	work_available.notify_all();
	return true;
}

critical_submit_result
critical_native_mobile_birth_publication_owner::submit(critical_native_recovery_envelope envelope)
{
	if (!native_birth_typed_command(envelope.command))
		return critical_submit_result::invalid;
	return native_recovery_submit(std::move(envelope));
}

bool critical_native_mobile_birth_publication_owner::copy_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
	return native_birth_typed_command(command) &&
	       (native_context_copy(command, critical_native_recovery_phase::execution_pending,
				    output) ||
		native_context_copy(command, critical_native_recovery_phase::continuation_pending,
				    output));
}

bool critical_native_mobile_birth_publication_owner::copy_context_bounded(
	const critical_native_recovery_envelope &expected,
	critical_native_recovery_envelope *output, uint64_t *generation,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)expected;
	(void)output;
	(void)generation;
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#else
	if (!output || !generation || !reserve ||
	    !native_birth_shared_shop_command(expected.command) ||
	    !critical_command_envelope_valid(expected.command) || !expected.revision ||
	    (expected.phase != critical_native_recovery_phase::execution_pending &&
	     expected.phase != critical_native_recovery_phase::continuation_pending))
		return false;
	const auto add = [](size_t &total, size_t count, size_t width) noexcept
	{
		if (width && count > (SIZE_MAX - total) / width)
			return false;
		total += count * width;
		return true;
	};
	// Same complete CCM1 length as the original canonical encoder. The scalar
	// envelope validator already checks its original format limits; no codec,
	// allocation or hashing occurs while calculating prospective storage.
	size_t wire = CRITICAL_COMMAND_HEADER_BYTES;
	if (!add(wire, expected.command.keys.size(), CRITICAL_COMMAND_ENTITY_KEY_BYTES) ||
	    !add(wire, expected.command.expected_revisions.size(),
		 CRITICAL_COMMAND_EXPECTED_REVISION_BYTES) ||
	    !add(wire, expected.command.payload.size(), 1) ||
	    !add(wire, CRITICAL_COMMAND_ACCOUNTING_PREFIX_BYTES, 1) ||
	    !add(wire, expected.command.accounting_intent.size(), 1))
		return false;
	size_t equality = 3 * sizeof(std::vector<uint8_t>), copied = 0;
	if (!add(equality, wire, 2) ||
	    !add(copied, expected.command.keys.size(), sizeof(critical_entity_key)) ||
	    !add(copied, expected.command.expected_revisions.size(),
		 sizeof(critical_expected_revision)) ||
	    !add(copied, expected.command.payload.size(), 1) ||
	    !add(copied, expected.command.accounting_intent.size(), 1) ||
	    !add(copied, expected.attachment.size(), 1))
		return false;
	size_t peak = outer_live;
	if (!add(peak, sizeof(critical_native_recovery_envelope), 1) ||
	    !add(peak, sizeof(std::string), 1) ||
	    !add(peak, sizeof(std::lock_guard<std::mutex>), 1) ||
	    !add(peak, expected.command.operation_id.bytes.size() + 1, 1) ||
	    !add(peak, std::max(equality, copied), 1) || !reserve(peak, context))
		return false;
	try
	{
		// Under the pinned fresh-string policy the 16-byte key requests 17
		// chars; fresh vector copy assignment requests exactly its size.
		const auto identity = operation_key(expected.command.operation_id);
		critical_native_recovery_envelope copy;
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (!health.initialized || stop_requested || !coordinator_generation ||
		    coordinator_generation_exhausted || found == operations.end() ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()))
			return false;
		const auto &state = *found->second;
		if (!state.native || !state.retain_until_publication ||
		    state.publication_checkpointing || state.native_context_uncertain ||
		    state.native_ack_uncertain || state.native->revision != expected.revision ||
		    state.native->phase != expected.phase ||
		    state.native->attachment != expected.attachment ||
		    state.command.keys.size() != expected.command.keys.size() ||
		    state.command.expected_revisions.size() !=
			    expected.command.expected_revisions.size() ||
		    state.command.payload.size() != expected.command.payload.size() ||
		    state.command.accounting_intent.size() !=
			    expected.command.accounting_intent.size() ||
		    (expected.phase == critical_native_recovery_phase::execution_pending ?
			     !operation_is_publication_pending(state) :
			     (state.phase != critical_operation_phase::native_continuation_pending ||
			      !state.native_physical_released)) ||
		    !critical_command_equal(expected.command, state.command))
			return false;
		// Direct fresh assignment avoids native_envelope's additional return
		// object's inline storage; original canonical equality remains mandatory.
		copy.command = state.command;
		copy.revision = state.native->revision;
		copy.phase = state.native->phase;
		copy.attachment = state.native->attachment;
		*output = std::move(copy);
		*generation = coordinator_generation;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool critical_native_mobile_birth_publication_owner::checkpoint_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	if (!native_birth_typed_command(expected.command))
		return false;
	return native_context_checkpoint(expected, &successor, expected.phase);
}

bool critical_native_mobile_birth_publication_owner::retire(
	const critical_native_recovery_envelope &expected) noexcept
{
	if (!native_birth_typed_command(expected.command))
		return false;
	return native_context_checkpoint(expected, nullptr,
					 critical_native_recovery_phase::continuation_pending);
}

bool critical_native_mobile_birth_publication_owner::retire(
	const critical_native_recovery_envelope &expected, uint64_t generation,
	bool (*durable_transfer)(const critical_native_recovery_envelope &, void *) noexcept,
	void *context) noexcept
{
	if (!native_birth_typed_command(expected.command) ||
	    !native_birth_origin_command(expected.command) || !generation || !durable_transfer)
		return false;
	return native_context_checkpoint(expected, nullptr,
					 critical_native_recovery_phase::continuation_pending,
					 generation, durable_transfer, context);
}

bool critical_native_mobile_birth_publication_owner::observe_generation(
	const critical_native_recovery_envelope &expected, uint64_t *output) noexcept
{
	if (!output || !native_birth_typed_command(expected.command))
		return false;
	try
	{
		const auto identity = operation_key(expected.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (found == operations.end() || !found->second->retain_until_publication ||
		    !native_matches(*found->second, expected) || !health.initialized ||
		    !coordinator_generation || coordinator_generation_exhausted || stop_requested ||
		    found->second->publication_checkpointing ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()))
			return false;
		*output = coordinator_generation;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_native_mobile_birth_publication_owner::cancel_refusal(
	const critical_native_recovery_envelope &original, const critical_completion &expected,
	uint64_t generation,
	bool (*native_cleanup)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
	if (!native_cleanup || !generation || !native_birth_typed_command(original.command) ||
	    expected.operation_id.bytes != original.command.operation_id.bytes ||
	    !critical_completion_disposition_valid(expected) ||
	    expected.disposition != critical_completion_disposition::never_admitted)
		return false;
	std::string identity;
	critical_native_recovery_envelope frozen;
	critical_completion receipt = expected;
	operation_state *pinned = nullptr;
	const auto matches = [&](const operation_state &state) noexcept
	{
		return operation_is_admission_failed(state) && state.owned_refusal_delivered &&
		       state.retain_until_publication && state.native &&
		       state.native->revision == frozen.revision &&
		       state.native->phase == frozen.phase &&
		       state.native->attachment == frozen.attachment &&
		       native_receipt_equal(state.admission_failure_completion, receipt);
	};
	try
	{
		// Cleanup may destroy the caller's native owner. Copies and canonical
		// command comparison must finish before pinning and invoking that callback.
		frozen = original;
		identity = operation_key(frozen.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || found->second->publication_checkpointing ||
		    !health.initialized || coordinator_generation != generation ||
		    coordinator_generation_exhausted || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !matches(*found->second) || !native_matches(*found->second, frozen))
			return false;
		pinned = found->second.get();
		pinned->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	const bool cleaned = native_cleanup(frozen.command, receipt, context);
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--publication_checkpoints_inflight;
	--guarded_publications_inflight;
	publication_checkpoint_finished.notify_all();
	auto found = operations.find(identity);
	if (found == operations.end() || coordinator_generation != generation ||
	    found->second.get() != pinned)
		return false;
	const bool same = found->second->publication_checkpointing && matches(*found->second);
	found->second->publication_checkpointing = false;
	if (!cleaned || !same || !health.initialized || coordinator_generation_exhausted ||
	    stop_requested ||
	    (lifecycle_guard_active && lifecycle_guard_thread != std::this_thread::get_id()))
		return false;
	// This exact delivered never-admitted owner has no durable frame to retire.
	remove_fences(identity, found->second->command);
	operations.erase(found);
	update_depth();
	work_available.notify_all();
	return true;
}

bool critical_native_mobile_birth_publication_owner::acknowledge(
	const critical_native_recovery_envelope &expected, const critical_completion &receipt,
	uint64_t generation) noexcept
{
	if (!generation || !native_birth_typed_command(expected.command) ||
	    expected.phase != critical_native_recovery_phase::execution_pending ||
	    expected.revision == UINT64_MAX ||
	    receipt.operation_id.bytes != expected.command.operation_id.bytes ||
	    (receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied) ||
	    receipt.disposition != critical_completion_disposition::execution ||
	    (!native_birth_shared_shop_command(expected.command) &&
	     receipt.durable_revision != 1) ||
	    receipt.error_code || receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != (expected.command.payload_version ==
						    NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ?
					    NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES :
					    NATIVE_MOBILE_BIRTH_RESULT_BYTES))
		return false;
	std::string identity;
	critical_native_recovery_envelope frozen, successor;
	operation_state *pinned = nullptr;
	bool prior_uncertain = false;
	try
	{
		// Every fallible copy/encoding precedes the pinned journal CAS.
		frozen = expected;
		successor = frozen;
		++successor.revision;
		successor.phase = critical_native_recovery_phase::continuation_pending;
		identity = operation_key(frozen.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || !health.initialized || stop_requested ||
		    coordinator_generation != generation || coordinator_generation_exhausted ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !operation_is_publication_pending(*found->second) ||
		    !found->second->retain_until_publication ||
		    found->second->publication_checkpointing ||
		    found->second->native_context_uncertain ||
		    !native_matches(*found->second, frozen) ||
		    !native_receipt_equal(found->second->publication_completion, receipt) ||
		    !native_birth_validators_ready() ||
		    !native_birth_recovery_publication(frozen, receipt) ||
		    !native_birth_recovery_successor(frozen, successor))
			return false;
		prior_uncertain = found->second->native_ack_uncertain;
		pinned = found->second.get();
		pinned->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	auto result = critical_command_journal_result::io_failure;
	try
	{
		result = critical_command_journal_replace_native_recovery(frozen, successor);
	}
	catch (...)
	{
	}
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--publication_checkpoints_inflight;
	--guarded_publications_inflight;
	publication_checkpoint_finished.notify_all();
	auto found = operations.find(identity);
	// No allocation, command encoding or owner callback after durable CAS.
	if (found == operations.end() || coordinator_generation != generation ||
	    found->second.get() != pinned || !found->second->publication_checkpointing ||
	    !operation_is_publication_pending(*found->second) || !found->second->native ||
	    found->second->native->revision != frozen.revision ||
	    found->second->native->phase != frozen.phase ||
	    found->second->native->attachment != frozen.attachment)
		return false;
	auto &state = *found->second;
	state.publication_checkpointing = false;
	if (result != critical_command_journal_result::ok)
	{
		state.native_ack_uncertain =
			prior_uncertain ||
			result == critical_command_journal_result::append_uncertain;
		return false;
	}
	state.native->revision = successor.revision;
	state.native->phase = successor.phase;
	state.phase = critical_operation_phase::native_continuation_pending;
	state.native_ack_uncertain = false;
	state.native_physical_released = true;
	// Original constructor evidence must transfer before this lifetime can
	// progress. Recipe-only historical births preserve their original ACK.
	if (!native_birth_origin_command(state.command))
		remove_fences(identity, state.command);
	// Birth consumes no player-save hold and creates no command-only cache entry.
	// The original envelope remains until the birth owner retires its terminal tail.
	update_depth();
	work_available.notify_all();
	return true;
}

critical_submit_result
critical_zone_reset_item_publication_owner::submit(critical_native_recovery_envelope envelope)
{
	if (!zone_reset_typed_command(envelope.command))
		return critical_submit_result::invalid;
	return native_recovery_submit(std::move(envelope));
}

bool critical_zone_reset_item_publication_owner::copy_context(
	const critical_command &command, critical_native_recovery_envelope *output) noexcept
{
	return zone_reset_typed_command(command) &&
	       (native_context_copy(command, critical_native_recovery_phase::execution_pending,
				    output) ||
		native_context_copy(command, critical_native_recovery_phase::continuation_pending,
				    output));
}

bool critical_zone_reset_item_publication_owner::checkpoint_context(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor) noexcept
{
	if (!zone_reset_typed_command(expected.command))
		return false;
	return native_context_checkpoint(expected, &successor, expected.phase);
}

bool critical_zone_reset_item_publication_owner::retire(
	const critical_native_recovery_envelope &expected, uint64_t generation,
	bool (*durable_transfer)(const critical_native_recovery_envelope &, void *) noexcept,
	void *context) noexcept
{
	if (!zone_reset_typed_command(expected.command) || !generation || !durable_transfer)
		return false;
	return native_context_checkpoint(expected, nullptr,
					 critical_native_recovery_phase::continuation_pending,
					 generation, durable_transfer, context);
}

bool critical_zone_reset_item_publication_owner::observe_generation(
	const critical_native_recovery_envelope &expected, uint64_t *output) noexcept
{
	if (!output || !zone_reset_typed_command(expected.command))
		return false;
	try
	{
		const auto identity = operation_key(expected.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (found == operations.end() || !found->second->retain_until_publication ||
		    !native_matches(*found->second, expected) || !health.initialized ||
		    !coordinator_generation || coordinator_generation_exhausted || stop_requested ||
		    found->second->publication_checkpointing ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()))
			return false;
		*output = coordinator_generation;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_zone_reset_item_publication_owner::cancel_refusal(
	const critical_native_recovery_envelope &original, const critical_completion &expected,
	uint64_t generation,
	bool (*native_cleanup)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
	if (!native_cleanup || !generation || !zone_reset_typed_command(original.command) ||
	    expected.operation_id.bytes != original.command.operation_id.bytes ||
	    !critical_completion_disposition_valid(expected) ||
	    expected.disposition != critical_completion_disposition::never_admitted)
		return false;
	std::string identity;
	critical_native_recovery_envelope frozen;
	critical_completion receipt = expected;
	operation_state *pinned = nullptr;
	const auto matches = [&](const operation_state &state) noexcept
	{
		return operation_is_admission_failed(state) && state.owned_refusal_delivered &&
		       state.retain_until_publication && state.native &&
		       state.native->revision == frozen.revision &&
		       state.native->phase == frozen.phase &&
		       state.native->attachment == frozen.attachment &&
		       native_receipt_equal(state.admission_failure_completion, receipt);
	};
	try
	{
		// Cleanup may destroy the caller's native owner. Copies and canonical
		// command comparison must finish before pinning and invoking that callback.
		frozen = original;
		identity = operation_key(frozen.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || found->second->publication_checkpointing ||
		    !health.initialized || coordinator_generation != generation ||
		    coordinator_generation_exhausted || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !matches(*found->second) || !native_matches(*found->second, frozen))
			return false;
		pinned = found->second.get();
		pinned->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	const bool cleaned = native_cleanup(frozen.command, receipt, context);
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--publication_checkpoints_inflight;
	--guarded_publications_inflight;
	publication_checkpoint_finished.notify_all();
	auto found = operations.find(identity);
	if (found == operations.end() || coordinator_generation != generation ||
	    found->second.get() != pinned)
		return false;
	const bool same = found->second->publication_checkpointing && matches(*found->second);
	found->second->publication_checkpointing = false;
	if (!cleaned || !same || !health.initialized || coordinator_generation_exhausted ||
	    stop_requested ||
	    (lifecycle_guard_active && lifecycle_guard_thread != std::this_thread::get_id()))
		return false;
	// This exact delivered never-admitted owner has no durable frame to retire.
	remove_fences(identity, found->second->command);
	operations.erase(found);
	update_depth();
	work_available.notify_all();
	return true;
}

bool critical_zone_reset_item_publication_owner::acknowledge(
	const critical_native_recovery_envelope &expected, const critical_completion &receipt,
	uint64_t generation) noexcept
{
	if (!generation || !zone_reset_typed_command(expected.command) ||
	    expected.phase != critical_native_recovery_phase::execution_pending ||
	    expected.revision == UINT64_MAX ||
	    receipt.operation_id.bytes != expected.command.operation_id.bytes ||
	    (receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied) ||
	    receipt.disposition != critical_completion_disposition::execution ||
	    !receipt.durable_revision || receipt.error_code ||
	    receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != ITEM_TRANSFER_RESULT_BYTES)
		return false;
	std::string identity;
	critical_native_recovery_envelope frozen, successor;
	operation_state *pinned = nullptr;
	bool prior_uncertain = false;
	try
	{
		// Every fallible copy/encoding precedes the pinned journal CAS.
		frozen = expected;
		successor = frozen;
		++successor.revision;
		successor.phase = critical_native_recovery_phase::continuation_pending;
		identity = operation_key(frozen.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || !health.initialized || stop_requested ||
		    coordinator_generation != generation || coordinator_generation_exhausted ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !operation_is_publication_pending(*found->second) ||
		    !found->second->retain_until_publication ||
		    found->second->publication_checkpointing ||
		    found->second->native_context_uncertain ||
		    !native_matches(*found->second, frozen) ||
		    !native_receipt_equal(found->second->publication_completion, receipt) ||
		    !zone_reset_validators_ready() ||
		    !zone_reset_validators.publication(frozen, receipt) ||
		    !zone_reset_validators.successor(frozen, successor))
			return false;
		prior_uncertain = found->second->native_ack_uncertain;
		pinned = found->second.get();
		pinned->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	auto result = critical_command_journal_result::io_failure;
	try
	{
		result = critical_command_journal_replace_native_recovery(frozen, successor);
	}
	catch (...)
	{
	}
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--publication_checkpoints_inflight;
	--guarded_publications_inflight;
	publication_checkpoint_finished.notify_all();
	auto found = operations.find(identity);
	// No allocation, command encoding or owner callback after durable CAS.
	if (found == operations.end() || coordinator_generation != generation ||
	    found->second.get() != pinned || !found->second->publication_checkpointing ||
	    !operation_is_publication_pending(*found->second) || !found->second->native ||
	    found->second->native->revision != frozen.revision ||
	    found->second->native->phase != frozen.phase ||
	    found->second->native->attachment != frozen.attachment)
		return false;
	auto &state = *found->second;
	state.publication_checkpointing = false;
	if (result != critical_command_journal_result::ok)
	{
		state.native_ack_uncertain =
			prior_uncertain ||
			result == critical_command_journal_result::append_uncertain;
		return false;
	}
	state.native->revision = successor.revision;
	state.native->phase = successor.phase;
	state.phase = critical_operation_phase::native_continuation_pending;
	state.native_ack_uncertain = false;
	state.native_physical_released = true;
	// Original constructor evidence must transfer before this lifetime can
	// progress. Recipe-only historical births preserve their original ACK.
	// Type22 fences remain until genuine durable terminal transfer and retirement.
	// Reset consumes no player-save hold and creates no command-only cache entry.
	// The original envelope remains until the birth owner retires its terminal tail.
	update_depth();
	work_available.notify_all();
	return true;
}

bool critical_native_mobile_birth_publication_owner::observe_generation(
	const critical_command &original, uint64_t *output) noexcept
{
	if (!output)
		return false;
	try
	{
		quest_mobile_native_image image;
		if (native_mobile_birth_command_decode(original, &image) !=
		    economic_accounting_error::ok)
			return false;
		std::vector<uint8_t> expected;
		if (critical_command_encode(original, &expected) !=
		    critical_command_codec_result::ok)
			return false;
		const auto identity = operation_key(original.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (found == operations.end() || !found->second->retain_until_publication ||
		    found->second->native || !health.initialized || !coordinator_generation ||
		    coordinator_generation_exhausted || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()))
			return false;
		std::vector<uint8_t> actual;
		if (critical_command_encode(found->second->command, &actual) !=
			    critical_command_codec_result::ok ||
		    actual != expected)
			return false;
		*output = coordinator_generation;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_native_mobile_birth_publication_owner::cancel_refusal(
	const critical_command &original, const critical_completion &expected, uint64_t generation,
	bool (*native_cleanup)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
	if (!native_cleanup || !generation ||
	    expected.operation_id.bytes != original.operation_id.bytes ||
	    !critical_completion_disposition_valid(expected) ||
	    expected.disposition != critical_completion_disposition::never_admitted)
		return false;
	std::string identity;
	std::vector<uint8_t> frozen;
	critical_command cleanup_command;
	critical_completion cleanup_completion = expected;
	operation_state *pinned = nullptr;
	const auto matches_receipt = [&](const operation_state &state) noexcept
	{
		const auto &receipt = state.admission_failure_completion;
		return operation_is_admission_failed(state) && state.owned_refusal_delivered &&
		       state.retain_until_publication && !state.native &&
		       critical_completion_disposition_valid(receipt) &&
		       receipt.operation_id.bytes == cleanup_completion.operation_id.bytes &&
		       receipt.outcome == cleanup_completion.outcome &&
		       receipt.disposition == cleanup_completion.disposition &&
		       receipt.durable_revision == cleanup_completion.durable_revision &&
		       receipt.error_code == cleanup_completion.error_code &&
		       receipt.failure_stage == cleanup_completion.failure_stage &&
		       receipt.result_size == cleanup_completion.result_size &&
		       receipt.result_payload == cleanup_completion.result_payload &&
		       receipt.attempt == cleanup_completion.attempt &&
		       receipt.queued_at_usec == cleanup_completion.queued_at_usec &&
		       receipt.started_at_usec == cleanup_completion.started_at_usec &&
		       receipt.completed_at_usec == cleanup_completion.completed_at_usec &&
		       receipt.recovery_correlation == cleanup_completion.recovery_correlation;
	};
	try
	{
		quest_mobile_native_image image;
		if (native_mobile_birth_command_decode(original, &image) !=
			    economic_accounting_error::ok ||
		    critical_command_encode(original, &frozen) != critical_command_codec_result::ok)
			return false;
		// Allocate before pinning, and never retain references to a caller body
		// which the proved cleanup may release. All proof uses these exact copies.
		cleanup_command = original;
		identity = operation_key(original.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || found->second->publication_checkpointing ||
		    !health.initialized || coordinator_generation != generation ||
		    coordinator_generation_exhausted || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !matches_receipt(*found->second))
			return false;
		// Authenticate immutable command bytes before cleanup can free its owner.
		std::vector<uint8_t> actual;
		if (critical_command_encode(found->second->command, &actual) !=
			    critical_command_codec_result::ok ||
		    actual != frozen)
			return false;
		pinned = found->second.get();
		pinned->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	// The private birth owner cleans only its exact never-admitted detached
	// constructor reservation. No coordinator mutex crosses native handlers.
	const bool cleaned = native_cleanup(cleanup_command, cleanup_completion, context);
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	const auto finish = [&]()
	{
		--publication_checkpoints_inflight;
		--guarded_publications_inflight;
		publication_checkpoint_finished.notify_all();
	};
	auto found = operations.find(identity);
	if (found == operations.end() || coordinator_generation != generation)
	{
		finish();
		return false;
	}
	// No command encoding or allocation follows successful native cleanup.
	const bool same = found->second.get() == pinned &&
			  found->second->publication_checkpointing &&
			  matches_receipt(*found->second);
	found->second->publication_checkpointing = false;
	if (!cleaned || !same || !health.initialized || coordinator_generation_exhausted ||
	    stop_requested ||
	    (lifecycle_guard_active && lifecycle_guard_thread != std::this_thread::get_id()))
	{
		finish();
		return false;
	}
	remove_fences(identity, found->second->command);
	operations.erase(found);
	update_depth();
	finish();
	work_available.notify_all();
	return true;
}

bool critical_native_mobile_birth_publication_owner::acknowledge(const critical_command &original,
								 const critical_completion &expected,
								 uint64_t generation) noexcept
{
	std::string identity;
	try
	{
		if (!generation || expected.operation_id.bytes != original.operation_id.bytes ||
		    (expected.outcome != critical_apply_outcome::applied &&
		     expected.outcome != critical_apply_outcome::already_applied) ||
		    expected.disposition != critical_completion_disposition::execution ||
		    expected.durable_revision != 1 || expected.error_code ||
		    expected.failure_stage != critical_failure_stage::none ||
		    expected.result_size != NATIVE_MOBILE_BIRTH_RESULT_BYTES)
			return false;
		quest_mobile_native_image image;
		native_mobile_birth_result result;
		if (native_mobile_birth_command_decode(original, &image) !=
			    economic_accounting_error::ok ||
		    !native_mobile_birth_result_decode(
			    std::span<const uint8_t>(expected.result_payload.data(),
						     expected.result_size),
			    &result) ||
		    result.mobile_instance_id != image.reference.mobile_instance_id)
			return false;
		std::vector<uint8_t> frozen;
		if (critical_command_encode(original, &frozen) != critical_command_codec_result::ok)
			return false;
		identity = operation_key(original.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (found == operations.end() ||
		    !operation_is_publication_pending(*found->second) ||
		    !found->second->retain_until_publication || found->second->native ||
		    found->second->publication_checkpointing || !health.initialized ||
		    coordinator_generation != generation || coordinator_generation_exhausted ||
		    stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()))
			return false;
		const auto &receipt = found->second->publication_completion;
		std::vector<uint8_t> actual;
		if (critical_command_encode(found->second->command, &actual) !=
			    critical_command_codec_result::ok ||
		    actual != frozen || receipt.operation_id.bytes != expected.operation_id.bytes ||
		    receipt.outcome != expected.outcome ||
		    receipt.disposition != expected.disposition ||
		    receipt.durable_revision != expected.durable_revision ||
		    receipt.error_code != expected.error_code ||
		    receipt.failure_stage != expected.failure_stage ||
		    receipt.result_size != expected.result_size ||
		    receipt.result_payload != expected.result_payload ||
		    receipt.attempt != expected.attempt ||
		    receipt.queued_at_usec != expected.queued_at_usec ||
		    receipt.started_at_usec != expected.started_at_usec ||
		    receipt.completed_at_usec != expected.completed_at_usec ||
		    receipt.recovery_correlation != expected.recovery_correlation)
			return false;
		found->second->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	critical_command_journal_result checkpoint = critical_command_journal_result::io_failure;
	try
	{
		checkpoint = critical_command_journal_checkpoint(original.operation_id);
	}
	catch (...)
	{
		// Retain the exact operation/fences and consumed-publication owner.
	}
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--guarded_publications_inflight;
	--publication_checkpoints_inflight;
	publication_checkpoint_finished.notify_all();
	const auto found = operations.find(identity);
	if (found == operations.end() || coordinator_generation != generation ||
	    !operation_is_publication_pending(*found->second) || found->second->native)
		return false;
	auto &state = *found->second;
	state.publication_checkpointing = false;
	auto trace =
		persistence_command_trace(state.command, persistence_trace_stage::publication_ack);
	trace.outcome = static_cast<uint32_t>(checkpoint);
	persistence_trace_record(trace);
	if (checkpoint != critical_command_journal_result::ok)
		return false;
	remove_fences(identity, state.command);
	remember_completed(identity, state.command, state.publication_completion);
	operations.erase(found);
	++health.completed;
	update_depth();
	work_available.notify_all();
	return true;
}

bool critical_command_coordinator_restored_shop_publication_current(
	const player_save_restored_publication_owner &owner) noexcept
{
#ifndef __NO_MYSQL__
	(void)owner;
	return false;
#else
	if (!owner.flat_shop_restored_ || owner.flat_shop_ || owner.acknowledged_ ||
	    owner.pid_ <= 0 || !owner.generation_ || !owner.flat_shop_restored_epoch_ ||
	    owner.flat_shop_restored_epoch_ !=
		    player_save_execution_guard::current_ownership_epoch() ||
	    !owner.reservation_.matches_pid(owner.pid_) || !owner.reservation_.valid() ||
	    persistence_mode_get() != PERSISTENCE_MODE_FLATFILE_PRIMARY ||
	    owner.command_.type != critical_command_type::shop_trade ||
	    !owner.command_.publication_required ||
	    owner.command_.payload_version != SHOP_TRADE_RECOVERY_PAYLOAD_VERSION ||
	    !critical_completion_disposition_valid(owner.completion_) ||
	    owner.completion_.disposition != critical_completion_disposition::execution ||
	    owner.command_.operation_id.bytes != owner.completion_.operation_id.bytes)
		return false;
	try
	{
		economic_frozen_intent intent;
		shop_trade_payload payload{};
		economic_account_key wallet, bank, counterparty;
		if (shop_trade_accounting_decode(owner.command_, &intent, &payload, &wallet, &bank,
						 &counterparty) != economic_accounting_error::ok ||
		    !payload.recovery_manifest_recorded ||
		    payload.player_pid != static_cast<uint32_t>(owner.pid_) ||
		    !payload.selected_item_uid ||
		    payload.selected_item_uid != owner.flat_shop_restored_root_uid_)
			return false;
		const auto identity = operation_key(owner.completion_.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (found == operations.end() || !health.initialized || stop_requested ||
		    !coordinator_generation || coordinator_generation_exhausted ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !owner.reservation_.valid())
			return false;
		const auto &state = *found->second;
		// This phase is set only by actual completion delivery after execution
		// keys are released. Completed-cache lookup cannot replace this state.
		if (!operation_is_publication_pending(state) || !state.retain_until_publication ||
		    state.native || state.native_context_uncertain || state.native_ack_uncertain ||
		    state.publication_checkpointing || !keys_available(identity, state.command) ||
		    !native_receipt_equal(state.publication_completion, owner.completion_) ||
		    !critical_command_equal(state.command, owner.command_))
			return false;
		const auto outcome = state.publication_completion.outcome;
		if (outcome != critical_apply_outcome::applied &&
		    outcome != critical_apply_outcome::already_applied &&
		    outcome != critical_apply_outcome::terminal_failure)
			return false;
		std::vector<uint8_t> frozen;
		return critical_command_encode(state.command, &frozen) ==
			       critical_command_codec_result::ok &&
		       frozen == owner.frozen_ && owner.reservation_.valid();
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool critical_command_coordinator_acknowledge_publication(
	player_save_restored_publication_owner &owner)
{
	if (!owner.publication_proven_ || owner.acknowledged_ || !owner.reservation_.valid())
		return false;
	std::string identity;
	std::unique_ptr<critical_native_recovery_envelope> native_expected, native_successor;
	bool native_already_transitioned = false;
	bool held_execution_retirement = false;
	operation_state *reserved_held_operation = nullptr;
	critical_completion held_receipt;
	try
	{
		identity = operation_key(owner.completion_.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() ||
		    (!operation_is_publication_pending(*found->second) &&
		     !(found->second->native &&
		       found->second->phase ==
			       critical_operation_phase::native_continuation_pending &&
		       !found->second->native_physical_released)) ||
		    found->second->publication_checkpointing || !coordinator_generation ||
		    coordinator_generation_exhausted || !health.initialized || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !owner.reservation_.valid())
			return false;
		const auto &state = *found->second;
		const auto &receipt = state.publication_completion;
		const auto success = [](critical_apply_outcome outcome)
		{
			return outcome == critical_apply_outcome::applied ||
			       outcome == critical_apply_outcome::already_applied;
		};
		const bool same_outcome =
			(success(receipt.outcome) && success(owner.completion_.outcome)) ||
			(receipt.outcome == critical_apply_outcome::terminal_failure &&
			 owner.completion_.outcome == critical_apply_outcome::terminal_failure &&
			 receipt.error_code && owner.completion_.error_code &&
			 receipt.disposition == critical_completion_disposition::execution &&
			 owner.completion_.disposition ==
				 critical_completion_disposition::execution &&
			 (receipt.failure_stage == critical_failure_stage::none ||
			  (owner.command_.type == critical_command_type::coin_transfer &&
			   critical_failure_stage_valid(receipt.failure_stage))) &&
			 receipt.failure_stage == owner.completion_.failure_stage);
		std::vector<uint8_t> frozen;
		if (critical_command_encode(state.command, &frozen) !=
			    critical_command_codec_result::ok ||
		    frozen != owner.frozen_ ||
		    receipt.operation_id.bytes != owner.completion_.operation_id.bytes ||
		    !same_outcome || receipt.disposition != owner.completion_.disposition ||
		    receipt.durable_revision != owner.completion_.durable_revision ||
		    receipt.error_code != owner.completion_.error_code ||
		    receipt.failure_stage != owner.completion_.failure_stage ||
		    receipt.result_size != owner.completion_.result_size ||
		    receipt.result_payload != owner.completion_.result_payload)
			return false;
		if (state.native)
		{
			held_execution_retirement = held_retirement_command_identity(state.command);
			if (held_execution_retirement &&
			    !native_receipt_equal(receipt, owner.completion_))
				return false;
			const bool auction = native_auction_typed_command(state.command);
			if (state.native_context_uncertain ||
			    (auction ? !native_auction_validators_ready() :
				       ((!native_quest_context_command(state.command) &&
					 !held_execution_retirement) ||
					!native_publication_validator_callback)))
				return false;
			native_expected = std::make_unique<critical_native_recovery_envelope>(
				native_envelope(state));
			native_already_transitioned =
				state.phase ==
				critical_operation_phase::native_continuation_pending;
			if (!(auction ? native_auction_validators.publication(*native_expected,
									      receipt) :
					(native_already_transitioned &&
							 native_fee_context_command(state.command) ?
						 native_quest_recovery_fee_ack_context_valid(
							 *native_expected, receipt) :
						 native_publication_validator_callback(
							 *native_expected, receipt))))
				return false;
			if (held_execution_retirement &&
			    (native_already_transitioned ||
			     state.native->phase !=
				     critical_native_recovery_phase::execution_pending))
				return false;
			if (!native_already_transitioned && !held_execution_retirement)
			{
				if (state.native->phase !=
					    critical_native_recovery_phase::execution_pending ||
				    state.native->revision == UINT64_MAX)
					return false;
				native_successor =
					std::make_unique<critical_native_recovery_envelope>(
						*native_expected);
				++native_successor->revision;
				native_successor->phase =
					critical_native_recovery_phase::continuation_pending;
				if (auction && !native_auction_validators.successor(
						       *native_expected, *native_successor))
					return false;
			}
		}
		if (held_execution_retirement)
		{
			reserved_held_operation = found->second.get();
			held_receipt = receipt;
		}
		owner.coordinator_generation_ = coordinator_generation;
		found->second->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	critical_command_journal_result checkpoint = critical_command_journal_result::io_failure;
	try
	{
		checkpoint =
			native_expected ?
				(held_execution_retirement ?
					 critical_held_retirement_journal_owner::retire_execution(
						 *native_expected) :
				 native_already_transitioned ?
					 critical_command_journal_result::ok :
					 critical_command_journal_replace_native_recovery(
						 *native_expected, *native_successor)) :
				critical_command_journal_checkpoint(owner.completion_.operation_id);
	}
	catch (...)
	{
	}
	std::unique_lock<std::mutex> lock(coordinator_mutex);
	const auto finish_guarded = [&]()
	{
		--guarded_publications_inflight;
		--publication_checkpoints_inflight;
		publication_checkpoint_finished.notify_all();
	};
	auto found = operations.find(identity);
	if (found == operations.end() ||
	    (!operation_is_publication_pending(*found->second) &&
	     !(native_already_transitioned && found->second->native &&
	       found->second->phase == critical_operation_phase::native_continuation_pending)) ||
	    coordinator_generation != owner.coordinator_generation_)
	{
		finish_guarded();
		return false;
	}
	auto &state = *found->second;
	// HRT has no continuation: bind terminal retirement to the exact reserved
	// operation, immutable envelope and complete receipt. Command bytes were
	// compared before the pin; no fallible encoding follows durable retirement.
	if (held_execution_retirement &&
	    (found->second.get() != reserved_held_operation || !state.publication_checkpointing ||
	     !state.native || !native_expected ||
	     state.native->revision != native_expected->revision ||
	     state.native->phase != native_expected->phase ||
	     state.native->attachment != native_expected->attachment ||
	     !native_receipt_equal(state.publication_completion, held_receipt) ||
	     !native_receipt_equal(owner.completion_, held_receipt) || !owner.reservation_.valid()))
	{
		// Release only this operation's pin. Retain its exact context, fences
		// and any uncertain retirement; another operation's pin is not ours.
		if (found->second.get() == reserved_held_operation)
		{
			state.publication_checkpointing = false;
			if (checkpoint == critical_command_journal_result::append_uncertain)
				state.native_ack_uncertain = true;
		}
		finish_guarded();
		return false;
	}
	state.publication_checkpointing = false;
	auto trace =
		persistence_command_trace(state.command, persistence_trace_stage::publication_ack);
	trace.outcome = static_cast<uint32_t>(checkpoint);
	persistence_trace_record(trace);
	if (checkpoint != critical_command_journal_result::ok)
	{
		if (native_expected &&
		    checkpoint == critical_command_journal_result::append_uncertain)
			state.native_ack_uncertain = true;
		finish_guarded();
		return false;
	}
	if (native_expected && !held_execution_retirement)
	{
		if (!native_already_transitioned)
		{
			state.native->revision = native_successor->revision;
			state.native->phase = native_successor->phase;
			state.phase = critical_operation_phase::native_continuation_pending;
			remove_fences(identity, state.command);
		}
		state.native_ack_uncertain = false;
		owner.acknowledged_ = true;
		update_depth();
		work_available.notify_all();
		lock.unlock();
		const bool consumed = owner.consume_acknowledged_hold();
		lock.lock();
		found = operations.find(identity);
		if (found != operations.end() && found->second->native &&
		    found->second->phase == critical_operation_phase::native_continuation_pending &&
		    coordinator_generation == owner.coordinator_generation_ && consumed)
			found->second->native_physical_released = true;
		finish_guarded();
		update_depth();
		return consumed;
	}
	remove_fences(identity, state.command);
	remember_completed(identity, state.command, state.publication_completion);
	operations.erase(found);
	++health.completed;
	owner.acknowledged_ = true;
	update_depth();
	work_available.notify_all();
	lock.unlock();
	// Do not acquire the pipeline owner under coordinator_mutex. Keep lifecycle
	// exclusion until exact-generation consumption and its release notice finish.
	const bool consumed = owner.consume_acknowledged_hold();
	lock.lock();
	finish_guarded();
	return consumed;
}

bool critical_command_coordinator_cancel_collector_publication(
	player_save_restored_publication_owner &owner)
{
	if (!owner.reservation_.valid() || owner.acknowledged_ ||
	    owner.command_.type != critical_command_type::collector ||
	    !critical_completion_disposition_valid(owner.completion_) ||
	    owner.completion_.disposition != critical_completion_disposition::never_admitted)
		return false;
	std::unique_lock<std::mutex> lock(coordinator_mutex);
	const auto identity = operation_key(owner.completion_.operation_id);
	auto found = operations.find(identity);
	if (found == operations.end() || !operation_is_admission_failed(*found->second) ||
	    !found->second->owned_refusal_delivered || !found->second->retain_until_publication ||
	    found->second->publication_checkpointing || !health.initialized || stop_requested ||
	    !owner.reservation_.valid())
		return false;
	const auto &receipt = found->second->admission_failure_completion;
	std::vector<uint8_t> frozen;
	if (critical_command_encode(found->second->command, &frozen) !=
		    critical_command_codec_result::ok ||
	    frozen != owner.frozen_ ||
	    receipt.operation_id.bytes != owner.completion_.operation_id.bytes ||
	    !critical_completion_disposition_valid(receipt) ||
	    receipt.disposition != owner.completion_.disposition ||
	    receipt.outcome != owner.completion_.outcome ||
	    receipt.error_code != owner.completion_.error_code ||
	    receipt.attempt != owner.completion_.attempt ||
	    receipt.queued_at_usec != owner.completion_.queued_at_usec ||
	    receipt.completed_at_usec != owner.completion_.completed_at_usec ||
	    receipt.durable_revision != owner.completion_.durable_revision ||
	    receipt.failure_stage != owner.completion_.failure_stage ||
	    receipt.result_size != owner.completion_.result_size ||
	    receipt.result_payload != owner.completion_.result_payload)
		return false;
	++guarded_publications_inflight;
	remove_fences(identity, found->second->command);
	operations.erase(found);
	owner.acknowledged_ = true;
	update_depth();
	lock.unlock();
	const bool consumed = owner.consume_acknowledged_hold();
	lock.lock();
	--guarded_publications_inflight;
	publication_checkpoint_finished.notify_all();
	work_available.notify_all();
	return consumed;
}

bool critical_command_coordinator_cancel_shop_publication(
	player_save_restored_publication_owner &owner,
	bool (*native_cleanup)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context)
{
	if (!native_cleanup || !owner.reservation_.valid() || owner.acknowledged_ ||
	    !accounted_shop_publication(owner.command_) ||
	    !critical_completion_disposition_valid(owner.completion_) ||
	    owner.completion_.disposition != critical_completion_disposition::never_admitted)
		return false;
	std::string identity;
	operation_state *pinned = nullptr;
	// Only the retained original refusal authorizes cleanup. In particular,
	// the callback must not run before its command and full receipt are proved.
	const auto matches_original = [&](const operation_state &state)
	{
		const auto &receipt = state.admission_failure_completion;
		std::vector<uint8_t> frozen;
		return operation_is_admission_failed(state) && state.owned_refusal_delivered &&
		       state.retain_until_publication &&
		       critical_command_encode(state.command, &frozen) ==
			       critical_command_codec_result::ok &&
		       frozen == owner.frozen_ &&
		       receipt.operation_id.bytes == owner.completion_.operation_id.bytes &&
		       critical_completion_disposition_valid(receipt) &&
		       receipt.disposition == owner.completion_.disposition &&
		       receipt.outcome == owner.completion_.outcome &&
		       receipt.error_code == owner.completion_.error_code &&
		       receipt.attempt == owner.completion_.attempt &&
		       receipt.queued_at_usec == owner.completion_.queued_at_usec &&
		       receipt.started_at_usec == owner.completion_.started_at_usec &&
		       receipt.completed_at_usec == owner.completion_.completed_at_usec &&
		       receipt.durable_revision == owner.completion_.durable_revision &&
		       receipt.failure_stage == owner.completion_.failure_stage &&
		       receipt.result_size == owner.completion_.result_size &&
		       receipt.result_payload == owner.completion_.result_payload;
	};
	try
	{
		identity = operation_key(owner.completion_.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || found->second->publication_checkpointing ||
		    !coordinator_generation || coordinator_generation_exhausted ||
		    !health.initialized || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !owner.reservation_.valid() || !matches_original(*found->second))
			return false;
		owner.coordinator_generation_ = coordinator_generation;
		pinned = found->second.get();
		found->second->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	// The private owner performs only original refusal cleanup under its held
	// reservation. No coordinator mutex is held over native handlers or SQL.
	const bool cleaned = native_cleanup(owner.command_, owner.completion_, context);
	std::unique_lock<std::mutex> lock(coordinator_mutex);
	const auto finish_guarded = [&]()
	{
		--guarded_publications_inflight;
		--publication_checkpoints_inflight;
		publication_checkpoint_finished.notify_all();
	};
	auto found = operations.find(identity);
	if (found == operations.end() || found->second.get() != pinned ||
	    coordinator_generation != owner.coordinator_generation_)
	{
		finish_guarded();
		return false;
	}
	bool original = false;
	try
	{
		original = found->second->publication_checkpointing &&
			   matches_original(*found->second);
	}
	catch (...)
	{
	}
	if (!cleaned || !original || !owner.reservation_.valid() || !health.initialized ||
	    stop_requested || coordinator_generation_exhausted ||
	    (lifecycle_guard_active && lifecycle_guard_thread != std::this_thread::get_id()))
	{
		// Keep the original command, refusal, native hold and all owner fences.
		found->second->publication_checkpointing = false;
		finish_guarded();
		return false;
	}
	// Keep the authentic refusal, original body and fences pinned over exact
	// player-hold consumption. A failed consumption must remain retryable.
	owner.acknowledged_ = true;
	lock.unlock();
	const bool consumed = owner.consume_acknowledged_hold();
	lock.lock();
	found = operations.find(identity);
	if (found == operations.end() || found->second.get() != pinned ||
	    coordinator_generation != owner.coordinator_generation_ ||
	    !found->second->publication_checkpointing)
	{
		finish_guarded();
		return false;
	}
	found->second->publication_checkpointing = false;
	if (!consumed)
	{
		owner.acknowledged_ = false;
		finish_guarded();
		return false;
	}
	remove_fences(identity, found->second->command);
	operations.erase(found);
	update_depth();
	// No journal checkpoint, execution receipt, completed-operation cache or
	// health.completed increment is created for a never-admitted command.
	finish_guarded();
	work_available.notify_all();
	return true;
}

bool critical_command_coordinator_cancel_held_retirement_publication(
	player_save_restored_publication_owner &owner,
	const critical_native_recovery_envelope &expected,
	bool (*native_cleanup)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context) noexcept
{
	if (!native_cleanup || !owner.reservation_.valid() || owner.acknowledged_ ||
	    !held_retirement_command_identity(owner.command_) ||
	    !critical_completion_disposition_valid(owner.completion_) ||
	    owner.completion_.disposition != critical_completion_disposition::never_admitted)
		return false;
	held_retirement_publication_snapshot original;
	try
	{
		original.envelope = expected;
		original.mode = held_retirement_publication_mode::original_refusal;
		if (!held_retirement_publication_snapshot_valid(owner.command_, owner.completion_,
								original))
			return false;
	}
	catch (...)
	{
		return false;
	}
	std::string identity;
	operation_state *reserved_operation = nullptr;
	const auto matches_original = [&](const operation_state &state)
	{
		std::vector<uint8_t> frozen;
		return operation_is_admission_failed(state) && state.owned_refusal_delivered &&
		       state.retain_until_publication && !state.native_context_uncertain &&
		       !state.native_ack_uncertain && native_matches(state, original.envelope) &&
		       critical_command_encode(state.command, &frozen) ==
			       critical_command_codec_result::ok &&
		       frozen == owner.frozen_ &&
		       native_receipt_equal(state.admission_failure_completion, owner.completion_);
	};
	try
	{
		identity = operation_key(owner.completion_.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || found->second->publication_checkpointing ||
		    !coordinator_generation || coordinator_generation_exhausted ||
		    !health.initialized || stop_requested || !owner.reservation_.valid() ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !matches_original(*found->second))
			return false;
		reserved_operation = found->second.get();
		owner.coordinator_generation_ = coordinator_generation;
		reserved_operation->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	// Callback uses the already pinned original envelope; no coordinator recopy,
	// journal checkpoint or source/native mutation is authorized by refusal.
	const bool cleaned = native_cleanup(owner.command_, owner.completion_, context);
	std::unique_lock<std::mutex> lock(coordinator_mutex);
	const auto finish_guarded = [&]()
	{
		--guarded_publications_inflight;
		--publication_checkpoints_inflight;
		publication_checkpoint_finished.notify_all();
	};
	auto found = operations.find(identity);
	if (found == operations.end() || found->second.get() != reserved_operation ||
	    coordinator_generation != owner.coordinator_generation_)
	{
		finish_guarded();
		return false;
	}
	bool original_matches = false;
	try
	{
		original_matches = found->second->publication_checkpointing &&
				   matches_original(*found->second);
	}
	catch (...)
	{
	}
	found->second->publication_checkpointing = false;
	if (!cleaned || !original_matches || !owner.reservation_.valid() || !health.initialized ||
	    stop_requested || coordinator_generation_exhausted ||
	    (lifecycle_guard_active && lifecycle_guard_thread != std::this_thread::get_id()))
	{
		finish_guarded();
		return false;
	}
	remove_fences(identity, found->second->command);
	operations.erase(found);
	owner.acknowledged_ = true;
	update_depth();
	lock.unlock();
	const bool consumed = owner.consume_acknowledged_hold();
	lock.lock();
	finish_guarded();
	work_available.notify_all();
	return consumed;
}

bool critical_command_coordinator_cancel_auction_publication(
	player_save_restored_publication_owner &owner,
	bool (*native_cleanup)(const critical_command &, const critical_completion &,
			       void *) noexcept,
	void *context)
{
	if (!native_cleanup || !owner.reservation_.valid() || owner.acknowledged_ ||
	    !native_auction_typed_command(owner.command_) || !owner.command_.publication_required ||
	    !critical_completion_disposition_valid(owner.completion_) ||
	    owner.completion_.disposition != critical_completion_disposition::never_admitted)
		return false;
	std::string identity;
	critical_native_recovery_envelope original_native;
	operation_state *pinned = nullptr;
	// Only the retained original refusal authorizes cleanup. In particular,
	// the callback must not run before its command and full receipt are proved.
	const auto matches_original = [&](const operation_state &state)
	{
		const auto &receipt = state.admission_failure_completion;
		std::vector<uint8_t> frozen;
		return native_matches(state, original_native) &&
		       native_auction_validators_ready() &&
		       native_auction_validators.initial(original_native) &&
		       operation_is_admission_failed(state) && state.owned_refusal_delivered &&
		       state.retain_until_publication &&
		       critical_command_encode(state.command, &frozen) ==
			       critical_command_codec_result::ok &&
		       frozen == owner.frozen_ &&
		       receipt.operation_id.bytes == owner.completion_.operation_id.bytes &&
		       critical_completion_disposition_valid(receipt) &&
		       receipt.disposition == owner.completion_.disposition &&
		       receipt.outcome == owner.completion_.outcome &&
		       receipt.error_code == owner.completion_.error_code &&
		       receipt.attempt == owner.completion_.attempt &&
		       receipt.queued_at_usec == owner.completion_.queued_at_usec &&
		       receipt.started_at_usec == owner.completion_.started_at_usec &&
		       receipt.completed_at_usec == owner.completion_.completed_at_usec &&
		       receipt.durable_revision == owner.completion_.durable_revision &&
		       receipt.failure_stage == owner.completion_.failure_stage &&
		       receipt.result_size == owner.completion_.result_size &&
		       receipt.result_payload == owner.completion_.result_payload;
	};
	try
	{
		identity = operation_key(owner.completion_.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || !found->second->native ||
		    !native_auction_validators_ready())
			return false;
		original_native = native_envelope(*found->second);
		if (found == operations.end() || found->second->publication_checkpointing ||
		    !coordinator_generation || coordinator_generation_exhausted ||
		    !health.initialized || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !owner.reservation_.valid() || !matches_original(*found->second))
			return false;
		owner.coordinator_generation_ = coordinator_generation;
		pinned = found->second.get();
		found->second->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	// The private owner performs only original refusal cleanup under its held
	// reservation. No coordinator mutex is held over native handlers or SQL.
	const bool cleaned = native_cleanup(owner.command_, owner.completion_, context);
	std::unique_lock<std::mutex> lock(coordinator_mutex);
	const auto finish_guarded = [&]()
	{
		--guarded_publications_inflight;
		--publication_checkpoints_inflight;
		publication_checkpoint_finished.notify_all();
	};
	auto found = operations.find(identity);
	if (found == operations.end() || found->second.get() != pinned ||
	    coordinator_generation != owner.coordinator_generation_)
	{
		finish_guarded();
		return false;
	}
	bool original = false;
	try
	{
		original = found->second->publication_checkpointing &&
			   matches_original(*found->second);
	}
	catch (...)
	{
	}
	if (!cleaned || !original || !owner.reservation_.valid() || !health.initialized ||
	    stop_requested || coordinator_generation_exhausted ||
	    (lifecycle_guard_active && lifecycle_guard_thread != std::this_thread::get_id()))
	{
		// Keep the original command, refusal, native hold and all owner fences.
		found->second->publication_checkpointing = false;
		finish_guarded();
		return false;
	}
	// Keep the authentic refusal, original body and fences pinned over exact
	// player-hold consumption. A failed consumption must remain retryable.
	owner.acknowledged_ = true;
	lock.unlock();
	const bool consumed = owner.consume_acknowledged_hold();
	lock.lock();
	found = operations.find(identity);
	if (found == operations.end() || found->second.get() != pinned ||
	    coordinator_generation != owner.coordinator_generation_ ||
	    !found->second->publication_checkpointing)
	{
		finish_guarded();
		return false;
	}
	found->second->publication_checkpointing = false;
	if (!consumed)
	{
		owner.acknowledged_ = false;
		finish_guarded();
		return false;
	}
	remove_fences(identity, found->second->command);
	operations.erase(found);
	update_depth();
	// No journal checkpoint, execution receipt, completed-operation cache or
	// health.completed increment is created for a never-admitted command.
	finish_guarded();
	work_available.notify_all();
	return true;
}

void queue_unqueued_admission_failures_locked()
{
	for (const auto &[identity, state] : operations)
	{
		(void)identity;
		if (!operation_is_admission_failed(*state) || state->admission_failure_queued ||
		    state->owned_refusal_delivered)
			continue;
		if (!completion_delivery.try_enqueue(critical_completion_channel::admission_failure,
						     state->admission_failure_completion))
			return;
		state->admission_failure_queued = true;
	}
}

size_t critical_command_coordinator_pulse(critical_completion *completions, size_t capacity)
{
	if (capacity && !completions)
		return 0;
	if (recovery_due())
		admission_available.notify_one();
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	queue_unqueued_admission_failures_locked();
	size_t published = 0;
	while (completion_delivery.size(critical_completion_channel::execution))
	{
		const critical_completion *front =
			completion_delivery.front(critical_completion_channel::execution);
		if (!front)
			break;
		critical_completion completion = *front;
		const std::string identity = operation_key(completion.operation_id);
		auto found = operations.find(identity);
		if (found == operations.end() || !operation_is_executing(*found->second) ||
		    found->second->attempt != completion.attempt)
		{
			completion_delivery.pop_front(critical_completion_channel::execution);
			result_available.notify_one();
			++health.stale_completions;
			continue;
		}
		const bool retryable = completion_is_retryable(completion);
		// Exhausted retries need a final notification just like other outcomes.
		// Retain the result until it can be published, before changing its state.
		const bool will_retry = retryable &&
					found->second->attempt <= CRITICAL_COORDINATOR_MAX_RETRIES;
		if (!will_retry && published >= capacity)
			break;
		completion_delivery.pop_front(critical_completion_channel::execution);
		result_available.notify_one();
		operation_state &state = *found->second;
		(void)death_recovery_command_correlation(state.command,
							 completion.recovery_correlation.data());
		if (state.retain_until_publication && !retryable)
		{
			release_keys(identity, state.command);
			state.phase = critical_operation_phase::publication_pending;
			state.publication_completion = completion;
			if (completion.outcome == critical_apply_outcome::terminal_failure)
				++health.terminal_failures;
			if (published < capacity)
				completions[published++] = completion;
			continue;
		}
		if (retryable)
		{
			if (will_retry && schedule_retry_locked(identity, state, completion))
				continue;
			retain_exhausted_retry_locked(identity, state, completion);
			if (published < capacity)
				completions[published++] = completion;
			continue;
		}
		release_keys(identity, state.command);
		remove_fences(identity, state.command);
		remember_completed(identity, state.command, completion);
		if (completion.outcome == critical_apply_outcome::terminal_failure)
			++health.terminal_failures;
		++health.completed;
		if (published < capacity)
			completions[published++] = completion;
		operations.erase(found);
	}
	while (completion_delivery.size(critical_completion_channel::admission_failure))
	{
		const critical_completion *front =
			completion_delivery.front(critical_completion_channel::admission_failure);
		if (!front)
			break;
		const critical_completion completion = *front;
		const std::string identity = operation_key(completion.operation_id);
		auto found = operations.find(identity);
		if (found == operations.end() || !operation_is_admission_failed(*found->second) ||
		    found->second->attempt != completion.attempt)
		{
			completion_delivery.pop_front(
				critical_completion_channel::admission_failure);
			++health.stale_completions;
			continue;
		}
		if (published >= capacity)
			break;
		completion_delivery.pop_front(critical_completion_channel::admission_failure);
		operation_state &state = *found->second;
		state.admission_failure_queued = false;
		release_keys(identity, state.command);
		remove_fences(identity, state.command);
		completions[published++] = completion;
		if (state.retain_until_publication &&
		    retained_admission_refusal_owner(state.command))
			state.owned_refusal_delivered = true;
		else
			operations.erase(found);
	}
	if (published < capacity)
	{
		for (auto found = operations.begin(); found != operations.end(); ++found)
		{
			if (!operation_is_admission_failed(*found->second) ||
			    found->second->admission_failure_queued ||
			    found->second->owned_refusal_delivered)
				continue;
			const std::string identity = found->first;
			const critical_completion completion =
				found->second->admission_failure_completion;
			release_keys(identity, found->second->command);
			remove_fences(identity, found->second->command);
			completions[published++] = completion;
			if (found->second->retain_until_publication &&
			    retained_admission_refusal_owner(found->second->command))
				found->second->owned_refusal_delivered = true;
			else
				operations.erase(found);
			break;
		}
	}
	update_depth();
	work_available.notify_all();
	return published;
}

bool critical_command_coordinator_is_fenced(const critical_entity_key &key,
					    critical_operation_id *operation_id)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	auto found = fences.find(entity_key(key));
	if (found == fences.end() || found->second.empty())
		return false;
	if (operation_id)
	{
		auto operation = operations.find(found->second.front());
		if (operation == operations.end())
			return false;
		*operation_id = operation->second->command.operation_id;
	}
	return true;
}

void critical_command_coordinator_quiesce(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	health.accepting = false;
	if (active_cutover_phase != cutover_owner_phase::none)
	{
		cutover_reopen_allowed = false;
		cutover_runtime_was_accepting = false;
	}
	if (lifecycle_guard_active && lifecycle_guard_initialized_runtime)
		lifecycle_guard_was_accepting = false;
}

void critical_command_coordinator_resume(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (health.initialized && !lifecycle_guard_active && !stop_requested &&
	    active_cutover_phase == cutover_owner_phase::none)
		health.accepting = true;
}

bool critical_command_coordinator_drain(uint64_t timeout_msec)
{
	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_msec);
	critical_completion completions[64] = {};
	for (;;)
	{
		const size_t completed = critical_command_coordinator_pulse(completions, 64);
		critical_drain_observer_fn observer = nullptr;
		{
			std::lock_guard<std::mutex> lock(coordinator_mutex);
			observer = drain_observer;
		}
		if (observer &&
		    (completed || player_save_execution_guard::current_ownership_epoch()))
			observer(completions, completed);
		const critical_coordinator_health snapshot =
			critical_command_coordinator_health_copy();
		if (!snapshot.queued && !snapshot.inflight && !snapshot.blocked &&
		    !snapshot.publication_pending && !snapshot.native_continuation_pending &&
		    !snapshot.awaiting_durability && !snapshot.admission_queue_bytes &&
		    !snapshot.append_inflight)
			return true;
		if (std::chrono::steady_clock::now() >= deadline)
			return false;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

bool critical_command_coordinator_cutover_ready(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	return cutover_ready_locked();
}

bool critical_command_coordinator_owner::boot_recovery_ready()
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	return lifecycle_guard_active && lifecycle_guard_thread == std::this_thread::get_id() &&
	       active_cutover_phase == cutover_owner_phase::none && cutover_ready_locked(true);
}

bool critical_command_coordinator_owner::transfer_lifecycle_guard_to_cutover_lease(
	uint64_t *generation, uint64_t *lease_id) noexcept
{
	if (!generation || !lease_id || generation == lease_id)
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!lifecycle_guard_active ||
		    lifecycle_guard_thread != std::this_thread::get_id() ||
		    active_cutover_phase != cutover_owner_phase::none ||
		    !cutover_ready_locked(true) || coordinator_generation_exhausted ||
		    !coordinator_generation || cutover_lease_ids_exhausted ||
		    !next_cutover_lease_id)
			return false;
		const uint64_t issued = next_cutover_lease_id;
		if (issued == UINT64_MAX)
		{
			next_cutover_lease_id = 0;
			cutover_lease_ids_exhausted = true;
		}
		else
			++next_cutover_lease_id;
		cutover_runtime_origin = true;
		cutover_runtime_was_accepting = lifecycle_guard_was_accepting;
		// Terminal cutover cleanup must not resume this runtime before its
		// actual owner authenticates the resulting projection and SQL lifetime.
		cutover_was_accepting = false;
		cutover_reopen_allowed = false;
		cutover_outcome_uncertain = false;
		active_cutover_generation = coordinator_generation;
		active_cutover_lease_id = issued;
		active_cutover_thread = std::this_thread::get_id();
		active_cutover_phase = cutover_owner_phase::lease_idle;
		lifecycle_guard_active = false;
		lifecycle_guard_thread = {};
		lifecycle_guard_was_accepting = false;
		lifecycle_guard_initialized_runtime = false;
		health.accepting = false;
		*generation = coordinator_generation;
		*lease_id = issued;
		update_depth();
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool critical_command_coordinator_owner::acquire_cutover_lease(uint64_t timeout_msec,
							       uint64_t *generation,
							       uint64_t *lease_id)
{
	if (!generation || !lease_id)
		return false;
	*generation = 0;
	*lease_id = 0;
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (!health.initialized || !health.running || stop_requested ||
		    lifecycle_guard_active || guarded_publications_inflight ||
		    active_cutover_phase != cutover_owner_phase::none)
			return false;
		cutover_runtime_origin = false;
		cutover_runtime_was_accepting = false;
		cutover_was_accepting = health.accepting;
		cutover_reopen_allowed = health.accepting;
		cutover_outcome_uncertain = false;
		active_cutover_phase = cutover_owner_phase::issuing;
		active_cutover_thread = std::this_thread::get_id();
		health.accepting = false;
		update_depth();
	}

	try
	{
		if (!critical_command_coordinator_drain(timeout_msec))
		{
			std::lock_guard<std::mutex> lock(coordinator_mutex);
			abandon_issuing_cutover_locked();
			return false;
		}
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		if (active_cutover_phase != cutover_owner_phase::issuing ||
		    active_cutover_thread != std::this_thread::get_id() ||
		    !cutover_ready_locked() || coordinator_generation_exhausted ||
		    !coordinator_generation || cutover_lease_ids_exhausted ||
		    !next_cutover_lease_id)
		{
			abandon_issuing_cutover_locked();
			return false;
		}
		const uint64_t issued_lease_id = next_cutover_lease_id;
		if (next_cutover_lease_id == UINT64_MAX)
		{
			next_cutover_lease_id = 0;
			cutover_lease_ids_exhausted = true;
		}
		else
			++next_cutover_lease_id;
		active_cutover_generation = coordinator_generation;
		active_cutover_lease_id = issued_lease_id;
		active_cutover_phase = cutover_owner_phase::lease_idle;
		*generation = coordinator_generation;
		*lease_id = issued_lease_id;
		update_depth();
		return true;
	}
	catch (...)
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		abandon_issuing_cutover_locked();
		return false;
	}
}

bool critical_command_coordinator_owner::validate_cutover_lease(uint64_t generation,
								uint64_t lease_id)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!generation || !lease_id || generation != active_cutover_generation ||
	    lease_id != active_cutover_lease_id ||
	    active_cutover_phase != cutover_owner_phase::lease_idle ||
	    active_cutover_thread != std::this_thread::get_id())
		return false;
	return cutover_ready_locked();
}

bool critical_command_coordinator_owner::release_cutover_lease(uint64_t generation,
							       uint64_t lease_id)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (cutover_runtime_origin || !generation || !lease_id ||
	    generation != active_cutover_generation || lease_id != active_cutover_lease_id ||
	    active_cutover_phase != cutover_owner_phase::lease_idle ||
	    active_cutover_thread != std::this_thread::get_id())
		return false;
	invalidate_active_cutover_lease();
	update_depth();
	return true;
}

bool critical_command_coordinator_owner::begin_cutover_transaction(uint64_t generation,
								   uint64_t lease_id,
								   const void *connection,
								   unsigned long session)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!generation || !lease_id || !connection || !session ||
	    generation != active_cutover_generation || lease_id != active_cutover_lease_id ||
	    active_cutover_phase != cutover_owner_phase::lease_idle ||
	    active_cutover_thread != std::this_thread::get_id())
		return false;
	active_cutover_phase = cutover_owner_phase::transaction_active;
	active_cutover_connection = connection;
	active_cutover_session = session;
	cutover_outcome_uncertain = true;
	update_depth();
	return true;
}

bool critical_command_coordinator_owner::validate_cutover_transaction(uint64_t generation,
								      uint64_t lease_id,
								      const void *connection,
								      unsigned long session)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	return generation && lease_id && connection && session &&
	       generation == active_cutover_generation && lease_id == active_cutover_lease_id &&
	       active_cutover_phase == cutover_owner_phase::transaction_active &&
	       active_cutover_thread == std::this_thread::get_id() &&
	       active_cutover_connection == connection && active_cutover_session == session;
}

void critical_command_coordinator_owner::set_cutover_outcome_uncertain(uint64_t generation,
								       uint64_t lease_id,
								       const void *connection,
								       unsigned long session,
								       bool uncertain)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (generation == active_cutover_generation && lease_id == active_cutover_lease_id &&
	    active_cutover_phase == cutover_owner_phase::transaction_active &&
	    active_cutover_connection == connection && active_cutover_session == session)
		cutover_outcome_uncertain = uncertain;
	update_depth();
}

bool critical_command_coordinator_owner::acquire_initialized_lifecycle_guard()
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (lifecycle_guard_active || active_cutover_phase != cutover_owner_phase::none ||
	    guarded_publications_inflight)
		return false;
	lifecycle_guard_active = true;
	lifecycle_guard_was_accepting = health.accepting;
	lifecycle_guard_initialized_runtime = true;
	lifecycle_guard_thread = std::this_thread::get_id();
	health.accepting = false;
	return true;
}

bool critical_command_coordinator_owner::return_runtime_cutover_to_lifecycle_guard(
	uint64_t generation, uint64_t lease_id, const void *connection, unsigned long session)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!cutover_runtime_origin || lifecycle_guard_active || !generation || !lease_id ||
	    !connection || !session || generation != active_cutover_generation ||
	    lease_id != active_cutover_lease_id ||
	    active_cutover_phase != cutover_owner_phase::transaction_active ||
	    active_cutover_thread != std::this_thread::get_id() ||
	    active_cutover_connection != connection || active_cutover_session != session ||
	    cutover_outcome_uncertain || !cutover_ready_locked())
		return false;
	const bool original_accepting = cutover_runtime_was_accepting;
	invalidate_active_cutover_lease();
	lifecycle_guard_active = true;
	lifecycle_guard_thread = std::this_thread::get_id();
	lifecycle_guard_was_accepting = original_accepting;
	lifecycle_guard_initialized_runtime = true;
	health.accepting = false;
	health.shutdown_refused = false;
	update_depth();
	return true;
}

bool critical_command_coordinator_owner::finish_initialized_lifecycle_reservation()
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (!lifecycle_guard_active || lifecycle_guard_thread != std::this_thread::get_id() ||
	    active_cutover_phase != cutover_owner_phase::none || !cutover_ready_locked(true))
		return false;
	lifecycle_guard_active = false;
	lifecycle_guard_thread = {};
	health.accepting = lifecycle_guard_was_accepting;
	lifecycle_guard_was_accepting = false;
	lifecycle_guard_initialized_runtime = false;
	update_depth();
	return true;
}

bool critical_command_coordinator_owner::finish_cutover_transaction(uint64_t generation,
								    uint64_t lease_id,
								    const void *connection,
								    unsigned long session)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	if (cutover_runtime_origin || !generation || !lease_id || !connection || !session ||
	    generation != active_cutover_generation || lease_id != active_cutover_lease_id ||
	    active_cutover_phase != cutover_owner_phase::transaction_active ||
	    active_cutover_thread != std::this_thread::get_id() ||
	    active_cutover_connection != connection || active_cutover_session != session)
		return false;
	const bool reopen = cutover_was_accepting && cutover_reopen_allowed && !stop_requested;
	invalidate_active_cutover_lease();
	health.accepting = reopen;
	health.shutdown_refused = false;
	update_depth();
	return true;
}

void critical_command_coordinator_set_drain_observer(critical_drain_observer_fn observer)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	drain_observer = observer;
}

critical_coordinator_health critical_command_coordinator_health_copy(void)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	update_depth();
	return health;
}

size_t critical_command_coordinator_recovery_copy(critical_recovery_case *cases, size_t capacity,
						  size_t *total, size_t offset)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	size_t copied = 0, found = 0;
	const uint64_t now = now_usec();
	for (const auto &[identity, operation] : operations)
	{
		(void)identity;
		critical_recovery_case entry;
		if (!death_recovery_command_correlation(operation->command, entry.correlation))
			continue;
		++found;
		if (!cases || copied >= capacity || found <= offset)
			continue;
		entry.operation_id = operation->command.operation_id;
		entry.attempts = operation->attempt;
		entry.error_code = operation->publication_completion.error_code;
		entry.owner = operation->phase == critical_operation_phase::publication_pending ?
				      "item_movement_publication" :
				      "critical_command";
		entry.state = operation->phase == critical_operation_phase::blocked &&
					      operation->attempt >
						      CRITICAL_COORDINATOR_MAX_RETRIES ?
				      "unresolved_retry_exhausted" :
				      "unresolved";
		entry.elapsed_msec = now >= operation->command.accepted_at_usec ?
					     (now - operation->command.accepted_at_usec) / 1000 :
					     0;
		cases[copied++] = entry;
	}
	if (total)
		*total = found;
	return copied;
}

bool critical_command_coordinator_inject_completion_for_tests(const critical_completion &completion)
{
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	return completion_delivery.try_enqueue(critical_completion_channel::execution, completion);
}

void critical_command_coordinator_reset_for_tests(void)
{
	if (!critical_command_coordinator_shutdown())
		return;
	critical_command_coordinator_release_lifecycle_guard();
	critical_command_coordinator_set_drain_observer(nullptr);
	critical_command_journal_reset_for_tests();
}

#include "economy/zone_reset_item_recovery.h"

namespace
{
bool room_retire_add(size_t &bytes, size_t amount) noexcept
{
	if (bytes > CRITICAL_COORDINATOR_MAX_BYTES ||
	    amount > CRITICAL_COORDINATOR_MAX_BYTES - bytes)
		return false;
	bytes += amount;
	return true;
}
bool room_retire_size_bounded(const critical_native_recovery_envelope &envelope, size_t *retained,
			      bool (*reserve)(size_t, void *) noexcept, void *context,
			      size_t outer) noexcept
{
	// Same original native envelope hard gates and canonical encoded size.
	if (!retained || !native_transport_command(envelope.command) ||
	    !envelope.command.publication_required || !envelope.revision ||
	    envelope.phase != critical_native_recovery_phase::continuation_pending ||
	    envelope.attachment.empty() ||
	    envelope.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES)
		return false;
	size_t live = outer;
	if (!room_retire_add(live, sizeof(std::vector<uint8_t>)) || !reserve(live, context))
		return false;
	std::vector<uint8_t> encoded;
	if (critical_command_encode_bounded(envelope.command, &encoded, reserve, context, live) !=
		    critical_command_codec_result::ok ||
	    encoded.size() >
		    CRITICAL_COORDINATOR_MAX_BYTES - NATIVE_COORDINATOR_ENVELOPE_OVERHEAD ||
	    envelope.attachment.size() > CRITICAL_COORDINATOR_MAX_BYTES -
						 NATIVE_COORDINATOR_ENVELOPE_OVERHEAD -
						 encoded.size())
		return false;
	*retained =
		encoded.size() + envelope.attachment.size() + NATIVE_COORDINATOR_ENVELOPE_OVERHEAD;
	return true;
}
bool room_retire_matches_bounded(const operation_state &state,
				 const critical_native_recovery_envelope &envelope,
				 bool (*reserve)(size_t, void *) noexcept, void *context,
				 size_t outer) noexcept
{
	if (!state.native || state.native->revision != envelope.revision ||
	    state.native->phase != envelope.phase ||
	    state.native->attachment != envelope.attachment)
		return false;
	size_t left = 0, right = 0;
	if (critical_command_encoder_working_bytes(state.command, &left) !=
		    critical_command_codec_result::ok ||
	    critical_command_encoder_working_bytes(envelope.command, &right) !=
		    critical_command_codec_result::ok ||
	    left < sizeof(std::vector<uint8_t>) || right < sizeof(std::vector<uint8_t>))
		return false;
	// critical_command_equal owns two output vectors; each nested original
	// encoder owns its result vector. Keep the first encoded heap in pass two.
	size_t first = outer, second = outer;
	if (!room_retire_add(first, 2 * sizeof(std::vector<uint8_t>)) ||
	    !room_retire_add(first, left) ||
	    !room_retire_add(second, 2 * sizeof(std::vector<uint8_t>)) ||
	    !room_retire_add(second, left - sizeof(std::vector<uint8_t>)) ||
	    !room_retire_add(second, right) || !reserve(std::max(first, second), context))
		return false;
	try
	{
		return critical_command_equal(state.command, envelope.command);
	}
	catch (...)
	{
		return false;
	}
}
} // namespace

bool critical_zone_reset_item_publication_owner::retire_bounded(
	const critical_native_recovery_envelope &expected, uint64_t expected_generation,
	bool (*durable_transfer)(const critical_native_recovery_envelope &, void *,
				 size_t) noexcept,
	void *transfer_context, bool (*reserve)(size_t, void *) noexcept, void *budget_context,
	size_t outer) noexcept
{
	if (!zone_reset_typed_command(expected.command) || !expected_generation ||
	    !durable_transfer || !reserve ||
	    expected.phase != critical_native_recovery_phase::continuation_pending)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t live = outer;
	// Actual original 16-byte operation_key requests 17 chars. The lock and
	// nonallocating post-retirement fence lookup temporaries are pre-admitted
	// before pinning; no fallible budget callback follows durable retirement.
	if (!room_retire_add(live, sizeof(std::string)) ||
	    !room_retire_add(live, expected.command.operation_id.bytes.size() + 1) ||
	    !room_retire_add(live, sizeof(std::unique_lock<std::mutex>)) ||
	    !room_retire_add(live, sizeof(room_locked_budget_callback)) ||
	    !room_retire_add(live, 2 * sizeof(std::array<char, 9>)) ||
	    !room_retire_add(live, sizeof(std::string_view)) || !reserve(live, budget_context))
		return false;
	uint64_t generation = 0;
	operation_state *reserved_operation = nullptr;
	size_t original_retained = 0, successor_retained = 0;
	bool prior_uncertain = false;
	try
	{
		const std::string identity = operation_key(expected.command.operation_id);
		if (!room_retire_size_bounded(expected, &original_retained, reserve, budget_context,
					      live))
			return false;
		{
			std::unique_lock<std::mutex> lock(coordinator_mutex);
			room_locked_budget_callback proof_budget{ lock, reserve, budget_context };
			auto found = operations.find(identity);
			if (!health.initialized || stop_requested || found == operations.end() ||
			    !room_retire_matches_bounded(*found->second, expected,
							 room_locked_budget_callback::relay,
							 &proof_budget, live) ||
			    found->second->publication_checkpointing ||
			    found->second->native_ack_uncertain || !coordinator_generation ||
			    coordinator_generation_exhausted ||
			    coordinator_generation != expected_generation ||
			    (lifecycle_guard_active &&
			     lifecycle_guard_thread != std::this_thread::get_id()) ||
			    found->second->phase !=
				    critical_operation_phase::native_continuation_pending ||
			    !found->second->native_physical_released ||
			    !zone_reset_validators_ready() ||
			    !zone_reset_validators.valid_bounded ||
			    !zone_reset_validators.terminal_bounded ||
			    !zone_reset_validators.valid_bounded(expected,
								 room_locked_budget_callback::relay,
								 &proof_budget, live) ||
			    !zone_reset_validators.terminal_bounded(
				    expected, room_locked_budget_callback::relay, &proof_budget,
				    live))
				return false;
			// Preserve every original retained proposal/ceiling rule even though
			// this capability retires only, never constructs a successor.
			if (found->second->ordinary_flat_transaction)
			{
				const size_t extra = found->second->ordinary_flat_transaction_bytes;
				if (!extra || successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
				    extra > CRITICAL_COORDINATOR_MAX_BYTES - successor_retained)
					return false;
				original_retained = found->second->retained_bytes;
				successor_retained += extra;
			}
			if (found->second->flat_transaction)
			{
				const size_t extra = found->second->flat_transaction_bytes;
				if (!extra || successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
				    extra > CRITICAL_COORDINATOR_MAX_BYTES - successor_retained)
					return false;
				original_retained = found->second->retained_bytes;
				successor_retained += extra;
			}
			if (found->second->room_flat_transaction)
			{
				const size_t extra = found->second->room_flat_transaction_bytes;
				if (!extra || successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
				    extra > CRITICAL_COORDINATOR_MAX_BYTES - successor_retained)
					return false;
				original_retained = found->second->retained_bytes;
				successor_retained += extra;
			}
			const size_t reserved =
				std::max(found->second->retained_bytes, successor_retained);
			if (reserved >
			    CRITICAL_COORDINATOR_MAX_BYTES -
				    (health.retained_bytes - found->second->retained_bytes))
				return false;
			prior_uncertain = found->second->native_context_uncertain;
			found->second->retained_bytes = reserved;
			found->second->publication_checkpointing = true;
			generation = coordinator_generation;
			reserved_operation = found->second.get();
			++publication_checkpoints_inflight;
			++guarded_publications_inflight;
			update_depth();
		}
		auto result = critical_command_journal_result::io_failure;
		// Exact original operation/generation remain pinned while the real owner
		// transfers its complete terminal BODY under its borrowed recovered root
		// lock. Relay THIS live coordinator prefix, not an uncounted two-arg shim.
		if (durable_transfer(expected, transfer_context, live))
		{
			try
			{
				// Transfer above retains its original outside-coordinator ownership.
				// Only the genuine bounded journal provider takes coor->journal.
				std::unique_lock<std::mutex> journal_owner(coordinator_mutex);
				room_locked_budget_callback journal_budget{ journal_owner, reserve,
									    budget_context };
				size_t journal_live = live;
				// Genuine provider lock_guard and named scratch coexist with the
				// unique_lock/descriptor already admitted in the original prefix.
				if (room_retire_add(journal_live,
						    sizeof(std::lock_guard<std::mutex>) +
							    sizeof(journal_live)))
					result =
						critical_command_journal_retire_native_recovery_bounded(
							expected,
							room_locked_budget_callback::relay,
							&journal_budget, journal_live);
				else
					result = critical_command_journal_result::quota_exceeded;
			}
			catch (...)
			{
				// A genuine new lock failure still reaches original unpin/recheck.
			}
		}
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		--guarded_publications_inflight;
		--publication_checkpoints_inflight;
		publication_checkpoint_finished.notify_all();
		auto found = operations.find(identity);
		if (found == operations.end() || coordinator_generation != generation ||
		    found->second.get() != reserved_operation ||
		    !found->second->publication_checkpointing || !found->second->native ||
		    found->second->native->revision != expected.revision ||
		    found->second->native->phase != expected.phase ||
		    found->second->native->attachment != expected.attachment)
			return false;
		auto &state = *found->second;
		state.publication_checkpointing = false;
		if (result != critical_command_journal_result::ok)
		{
			state.native_context_uncertain =
				prior_uncertain ||
				result == critical_command_journal_result::append_uncertain;
			if (!state.native_context_uncertain)
				state.retained_bytes = original_retained;
			update_depth();
			return false;
		}
		remove_fences(identity, state.command);
		operations.erase(found);
		++health.completed;
		update_depth();
		work_available.notify_all();
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

namespace
{
bool room_checkpoint_heap(const critical_native_recovery_envelope &envelope, bool fresh,
			  size_t &bytes) noexcept
{
	const auto &command = envelope.command;
	const size_t keys = fresh ? command.keys.size() : command.keys.capacity();
	const size_t revisions = fresh ? command.expected_revisions.size() :
					 command.expected_revisions.capacity();
	return keys <= CRITICAL_COORDINATOR_MAX_BYTES / sizeof(critical_entity_key) &&
	       room_retire_add(bytes, keys * sizeof(critical_entity_key)) &&
	       revisions <= CRITICAL_COORDINATOR_MAX_BYTES / sizeof(critical_expected_revision) &&
	       room_retire_add(bytes, revisions * sizeof(critical_expected_revision)) &&
	       room_retire_add(bytes,
			       fresh ? command.payload.size() : command.payload.capacity()) &&
	       room_retire_add(bytes, fresh ? command.accounting_intent.size() :
					      command.accounting_intent.capacity()) &&
	       room_retire_add(bytes,
			       fresh ? envelope.attachment.size() : envelope.attachment.capacity());
}
bool room_checkpoint_size_bounded(const critical_native_recovery_envelope &envelope,
				  size_t *retained, bool (*reserve)(size_t, void *) noexcept,
				  void *context, size_t outer) noexcept
{
	if (!retained || !native_transport_command(envelope.command) ||
	    (held_retirement_transport_command(envelope.command) &&
	     envelope.phase != critical_native_recovery_phase::execution_pending) ||
	    !envelope.command.publication_required || !envelope.revision ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     envelope.phase != critical_native_recovery_phase::continuation_pending) ||
	    envelope.attachment.empty() ||
	    envelope.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES)
		return false;
	size_t live = outer;
	if (!room_retire_add(live, sizeof(std::vector<uint8_t>)) || !reserve(live, context))
		return false;
	std::vector<uint8_t> encoded;
	if (critical_command_encode_bounded(envelope.command, &encoded, reserve, context, live) !=
		    critical_command_codec_result::ok ||
	    encoded.size() >
		    CRITICAL_COORDINATOR_MAX_BYTES - NATIVE_COORDINATOR_ENVELOPE_OVERHEAD ||
	    envelope.attachment.size() > CRITICAL_COORDINATOR_MAX_BYTES -
						 NATIVE_COORDINATOR_ENVELOPE_OVERHEAD -
						 encoded.size())
		return false;
	*retained =
		encoded.size() + envelope.attachment.size() + NATIVE_COORDINATOR_ENVELOPE_OVERHEAD;
	return true;
}
bool room_checkpoint_equal_bounded(const critical_command &left, const critical_command &right,
				   bool (*reserve)(size_t, void *) noexcept, void *context,
				   size_t outer) noexcept
{
	size_t a = 0, b = 0;
	if (critical_command_encoder_working_bytes(left, &a) != critical_command_codec_result::ok ||
	    critical_command_encoder_working_bytes(right, &b) !=
		    critical_command_codec_result::ok ||
	    a < sizeof(std::vector<uint8_t>) || b < sizeof(std::vector<uint8_t>))
		return false;
	size_t first = outer, second = outer;
	if (!room_retire_add(first, 2 * sizeof(std::vector<uint8_t>)) ||
	    !room_retire_add(first, a) ||
	    !room_retire_add(second, 2 * sizeof(std::vector<uint8_t>)) ||
	    !room_retire_add(second, a - sizeof(std::vector<uint8_t>)) ||
	    !room_retire_add(second, b) || !reserve(std::max(first, second), context))
		return false;
	try
	{
		return critical_command_equal(left, right);
	}
	catch (...)
	{
		return false;
	}
}
} // complete original native checkpoint size and equality

bool critical_zone_reset_item_publication_owner::checkpoint_context_bounded(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor,
	bool (*reserve)(size_t, void *) noexcept, void *budget_context, size_t outer,
	uint64_t expected_generation) noexcept
{
	if (!reserve || !zone_reset_typed_command(expected.command) ||
	    successor.phase != expected.phase || expected.revision == UINT64_MAX ||
	    successor.revision != expected.revision + 1)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t live = outer;
	if (!room_retire_add(live, sizeof(critical_native_recovery_envelope)) ||
	    !room_retire_add(live, sizeof(std::string)) ||
	    !room_retire_add(live, expected.command.operation_id.bytes.size() + 1) ||
	    !room_retire_add(live, sizeof(std::unique_lock<std::mutex>)) ||
	    !room_retire_add(live, sizeof(room_locked_budget_callback)) ||
	    !reserve(live, budget_context))
		return false;
	uint64_t generation = 0;
	operation_state *reserved_operation = nullptr;
	size_t original_retained = 0, successor_retained = 0;
	bool prior_uncertain = false;
	try
	{
		critical_native_recovery_envelope prepared;
		if (!room_checkpoint_size_bounded(expected, &original_retained, reserve,
						  budget_context, live) ||
		    !room_checkpoint_equal_bounded(expected.command, successor.command, reserve,
						   budget_context, live) ||
		    !room_checkpoint_size_bounded(successor, &successor_retained, reserve,
						  budget_context, live))
			return false;
		size_t copy_live = live;
		if (!room_checkpoint_heap(successor, true, copy_live) ||
		    !reserve(copy_live, budget_context))
			return false;
		prepared = successor; // Full command and BODY clone before reservation/effects.
		if (!room_checkpoint_heap(prepared, false, live))
			return false;
		const std::string identity = operation_key(expected.command.operation_id);
		{
			std::unique_lock<std::mutex> lock(coordinator_mutex);
			room_locked_budget_callback proof_budget{ lock, reserve, budget_context };
			auto found = operations.find(identity);
			if (!health.initialized || stop_requested || found == operations.end() ||
			    !room_retire_matches_bounded(*found->second, expected,
							 room_locked_budget_callback::relay,
							 &proof_budget, live) ||
			    found->second->publication_checkpointing ||
			    found->second->native_ack_uncertain || !coordinator_generation ||
			    coordinator_generation_exhausted ||
			    (expected_generation &&
			     coordinator_generation != expected_generation) ||
			    (lifecycle_guard_active &&
			     lifecycle_guard_thread != std::this_thread::get_id()) ||
			    (expected.phase == critical_native_recovery_phase::execution_pending ?
				     !operation_is_publication_pending(*found->second) :
				     (found->second->phase !=
					      critical_operation_phase::native_continuation_pending ||
				      !found->second->native_physical_released)) ||
			    !zone_reset_validators_ready() ||
			    !zone_reset_validators.valid_bounded ||
			    !zone_reset_validators.successor_bounded ||
			    !zone_reset_validators.valid_bounded(expected,
								 room_locked_budget_callback::relay,
								 &proof_budget, live) ||
			    !zone_reset_validators.successor_bounded(
				    expected, prepared, room_locked_budget_callback::relay,
				    &proof_budget, live))
				return false;
			if (found->second->ordinary_flat_transaction)
			{
				const size_t extra = found->second->ordinary_flat_transaction_bytes;
				if (!extra || successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
				    extra > CRITICAL_COORDINATOR_MAX_BYTES - successor_retained)
					return false;
				original_retained = found->second->retained_bytes;
				successor_retained += extra;
			}
			if (found->second->flat_transaction)
			{
				const size_t extra = found->second->flat_transaction_bytes;
				if (!extra || successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
				    extra > CRITICAL_COORDINATOR_MAX_BYTES - successor_retained)
					return false;
				original_retained = found->second->retained_bytes;
				successor_retained += extra;
			}
			if (found->second->room_flat_transaction)
			{
				const size_t extra = found->second->room_flat_transaction_bytes;
				if (!extra || successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
				    extra > CRITICAL_COORDINATOR_MAX_BYTES - successor_retained)
					return false;
				original_retained = found->second->retained_bytes;
				successor_retained += extra;
			}
			const size_t reserved =
				std::max(found->second->retained_bytes, successor_retained);
			if (reserved >
			    CRITICAL_COORDINATOR_MAX_BYTES -
				    (health.retained_bytes - found->second->retained_bytes))
				return false;
			prior_uncertain = found->second->native_context_uncertain;
			found->second->retained_bytes = reserved;
			found->second->publication_checkpointing = true;
			generation = coordinator_generation;
			reserved_operation = found->second.get();
			++publication_checkpoints_inflight;
			++guarded_publications_inflight;
			update_depth();
		}
		auto result = critical_command_journal_result::io_failure;
		try
		{
			// Actual coor->journal lock order only during the original bounded
			// provider. No outside native transfer is brought under this lock.
			std::unique_lock<std::mutex> journal_owner(coordinator_mutex);
			room_locked_budget_callback journal_budget{ journal_owner, reserve,
								    budget_context };
			size_t journal_live = live;
			// Coexisting genuine journal provider lock_guard; the original
			// unique_lock and relay descriptor are already in live.
			if (room_retire_add(journal_live, sizeof(std::lock_guard<std::mutex>) +
								  sizeof(journal_live)))
				result = critical_command_journal_replace_native_recovery_bounded(
					expected, prepared, room_locked_budget_callback::relay,
					&journal_budget, journal_live);
			else
				result = critical_command_journal_result::quota_exceeded;
		}
		catch (...)
		{
			// Genuine new lock failure still reaches the original unpin/recheck;
			// never strand the operation after its original pin was installed.
		}
		// Same original exact operation pin and nonallocating post-durable recheck.
		// No codec/allocation/budget callback follows confirmed journal replacement.
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		--guarded_publications_inflight;
		--publication_checkpoints_inflight;
		publication_checkpoint_finished.notify_all();
		auto found = operations.find(identity);
		if (found == operations.end() || coordinator_generation != generation ||
		    found->second.get() != reserved_operation ||
		    !found->second->publication_checkpointing || !found->second->native ||
		    found->second->native->revision != expected.revision ||
		    found->second->native->phase != expected.phase ||
		    found->second->native->attachment != expected.attachment)
			return false;
		auto &state = *found->second;
		state.publication_checkpointing = false;
		if (result != critical_command_journal_result::ok)
		{
			state.native_context_uncertain =
				prior_uncertain ||
				result == critical_command_journal_result::append_uncertain;
			if (!state.native_context_uncertain)
				state.retained_bytes = original_retained;
			update_depth();
			return false;
		}
		state.native->revision = prepared.revision;
		state.native->attachment = std::move(prepared.attachment);
		state.retained_bytes = successor_retained;
		state.native_context_uncertain = false;
		update_depth();
		work_available.notify_all();
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool critical_zone_reset_item_publication_owner::acknowledge_bounded(
	const critical_native_recovery_envelope &expected, const critical_completion &receipt,
	uint64_t generation, bool (*reserve)(size_t, void *) noexcept, void *budget_context,
	size_t outer) noexcept
{
	if (!reserve || !generation || !zone_reset_typed_command(expected.command) ||
	    expected.phase != critical_native_recovery_phase::execution_pending ||
	    expected.revision == UINT64_MAX ||
	    receipt.operation_id.bytes != expected.command.operation_id.bytes ||
	    (receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied) ||
	    receipt.disposition != critical_completion_disposition::execution ||
	    !receipt.durable_revision || receipt.error_code ||
	    receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != ITEM_TRANSFER_RESULT_BYTES)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t live = outer;
	if (!room_retire_add(live, 2 * sizeof(critical_native_recovery_envelope)) ||
	    !room_retire_add(live, sizeof(std::string)) ||
	    !room_retire_add(live, expected.command.operation_id.bytes.size() + 1) ||
	    !room_retire_add(live, sizeof(std::unique_lock<std::mutex>)) ||
	    !room_retire_add(live, sizeof(room_locked_budget_callback)) ||
	    !reserve(live, budget_context))
		return false;
	std::string identity;
	critical_native_recovery_envelope frozen, successor;
	operation_state *pinned = nullptr;
	bool prior_uncertain = false;
	try
	{
		// Every fallible copy/encoding precedes the pinned journal CAS.
		size_t copy_live = live;
		if (!room_checkpoint_heap(expected, true, copy_live) ||
		    !reserve(copy_live, budget_context))
			return false;
		frozen = expected;
		if (!room_checkpoint_heap(frozen, false, live))
			return false;
		copy_live = live;
		if (!room_checkpoint_heap(frozen, true, copy_live) ||
		    !reserve(copy_live, budget_context))
			return false;
		successor = frozen;
		if (!room_checkpoint_heap(successor, false, live))
			return false;
		++successor.revision;
		successor.phase = critical_native_recovery_phase::continuation_pending;
		// Move assignment materializes the operation_key return object alongside
		// the already-live identity; its allocation is already in the main prefix.
		size_t key_live = live;
		if (!room_retire_add(key_live, sizeof(std::string)) ||
		    !reserve(key_live, budget_context))
			return false;
		identity = operation_key(frozen.command.operation_id);
		std::unique_lock<std::mutex> lock(coordinator_mutex);
		room_locked_budget_callback proof_budget{ lock, reserve, budget_context };
		auto found = operations.find(identity);
		if (found == operations.end() || !health.initialized || stop_requested ||
		    coordinator_generation != generation || coordinator_generation_exhausted ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !operation_is_publication_pending(*found->second) ||
		    !found->second->retain_until_publication ||
		    found->second->publication_checkpointing ||
		    found->second->native_context_uncertain ||
		    !room_retire_matches_bounded(*found->second, frozen,
						 room_locked_budget_callback::relay, &proof_budget,
						 live) ||
		    !native_receipt_equal(found->second->publication_completion, receipt) ||
		    !zone_reset_validators_ready() || !zone_reset_validators.publication_bounded ||
		    !zone_reset_validators.successor_bounded ||
		    !zone_reset_validators.publication_bounded(frozen, receipt,
							       room_locked_budget_callback::relay,
							       &proof_budget, live) ||
		    !zone_reset_validators.successor_bounded(frozen, successor,
							     room_locked_budget_callback::relay,
							     &proof_budget, live))
			return false;
		prior_uncertain = found->second->native_ack_uncertain;
		pinned = found->second.get();
		pinned->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	auto result = critical_command_journal_result::io_failure;
	try
	{
		// Actual coor->journal order only during the genuine bounded CAS.
		std::unique_lock<std::mutex> journal_owner(coordinator_mutex);
		room_locked_budget_callback journal_budget{ journal_owner, reserve,
							    budget_context };
		size_t journal_live = live;
		if (room_retire_add(journal_live,
				    sizeof(std::lock_guard<std::mutex>) + sizeof(journal_live)))
			result = critical_command_journal_replace_native_recovery_bounded(
				frozen, successor, room_locked_budget_callback::relay,
				&journal_budget, journal_live);
		else
			result = critical_command_journal_result::quota_exceeded;
	}
	catch (...)
	{
	}
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--publication_checkpoints_inflight;
	--guarded_publications_inflight;
	publication_checkpoint_finished.notify_all();
	auto found = operations.find(identity);
	// No allocation, command encoding or owner callback after durable CAS.
	if (found == operations.end() || coordinator_generation != generation ||
	    found->second.get() != pinned || !found->second->publication_checkpointing ||
	    !operation_is_publication_pending(*found->second) || !found->second->native ||
	    found->second->native->revision != frozen.revision ||
	    found->second->native->phase != frozen.phase ||
	    found->second->native->attachment != frozen.attachment)
		return false;
	auto &state = *found->second;
	state.publication_checkpointing = false;
	if (result != critical_command_journal_result::ok)
	{
		state.native_ack_uncertain =
			prior_uncertain ||
			result == critical_command_journal_result::append_uncertain;
		return false;
	}
	state.native->revision = successor.revision;
	state.native->phase = successor.phase;
	state.phase = critical_operation_phase::native_continuation_pending;
	state.native_ack_uncertain = false;
	state.native_physical_released = true;
	// Original constructor evidence must transfer before this lifetime can
	// progress. Recipe-only historical births preserve their original ACK.
	// Type22 fences remain until genuine durable terminal transfer and retirement.
	// Reset consumes no player-save hold and creates no command-only cache entry.
	// The original envelope remains until the birth owner retires its terminal tail.
	update_depth();
	work_available.notify_all();
	return true;
#endif
}

bool critical_zone_reset_item_publication_owner::copy_context_bounded(
	const critical_command &command, critical_native_recovery_envelope *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!output || !reserve || !zone_reset_typed_command(command))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t live = outer;
	if (!room_retire_add(live, sizeof(critical_native_recovery_envelope)) ||
	    !room_retire_add(live, sizeof(std::string)) ||
	    !room_retire_add(live, command.operation_id.bytes.size() + 1) ||
	    !room_retire_add(live, sizeof(std::unique_lock<std::mutex>)) ||
	    !room_retire_add(live, sizeof(room_locked_budget_callback)) || !reserve(live, context))
		return false;
	try
	{
		// The original phase1/phase2 attempts differ only in this actual phase test.
		// One same-lock lookup preserves both complete predicates and proofs.
		const std::string identity = operation_key(command.operation_id);
		critical_native_recovery_envelope copy;
		std::unique_lock<std::mutex> lock(coordinator_mutex);
		room_locked_budget_callback locked_budget{ lock, reserve, context };
		const auto found = operations.find(identity);
		if (!health.initialized || stop_requested || found == operations.end() ||
		    !found->second->native || found->second->publication_checkpointing ||
		    found->second->native_context_uncertain ||
		    found->second->native_ack_uncertain ||
		    !room_checkpoint_equal_bounded(command, found->second->command,
						   room_locked_budget_callback::relay,
						   &locked_budget, live))
			return false;
		const auto &state = *found->second;
		const auto phase = state.native->phase;
		if (phase == critical_native_recovery_phase::execution_pending ?
			    !operation_is_publication_pending(state) :
			    (phase != critical_native_recovery_phase::continuation_pending ||
			     state.phase != critical_operation_phase::native_continuation_pending ||
			     !state.native_physical_released))
			return false;
		// Inspect actual current vectors without constructing an uncharged carrier.
		const auto &current = state.command;
		if (current.keys.size() >
			    CRITICAL_COORDINATOR_MAX_BYTES / sizeof(critical_entity_key) ||
		    current.expected_revisions.size() >
			    CRITICAL_COORDINATOR_MAX_BYTES / sizeof(critical_expected_revision) ||
		    !room_retire_add(live, current.keys.size() * sizeof(critical_entity_key)) ||
		    !room_retire_add(live, current.expected_revisions.size() *
						   sizeof(critical_expected_revision)) ||
		    !room_retire_add(live, current.payload.size()) ||
		    !room_retire_add(live, current.accounting_intent.size()) ||
		    !room_retire_add(live, state.native->attachment.size()) ||
		    !room_locked_budget_callback::relay(live, &locked_budget))
			return false;
		// Same original native_envelope field copies into one actual fresh object.
		// Direct fields avoid a separate uncharged return carrier or fabricated BODY.
		copy.command = current;
		copy.revision = state.native->revision;
		copy.phase = phase;
		copy.attachment = state.native->attachment;
		if (!zone_reset_validators_ready() || !zone_reset_validators.valid_bounded ||
		    !zone_reset_validators.valid_bounded(copy, room_locked_budget_callback::relay,
							 &locked_budget, live))
			return false;
		*output = std::move(copy);
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool critical_zone_reset_item_publication_owner::observe_generation_bounded(
	const critical_native_recovery_envelope &expected, uint64_t *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!output || !reserve || !zone_reset_typed_command(expected.command))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t live = outer;
	if (!room_retire_add(live, sizeof(std::string)) ||
	    !room_retire_add(live, expected.command.operation_id.bytes.size() + 1) ||
	    !room_retire_add(live, sizeof(std::unique_lock<std::mutex>)) ||
	    !room_retire_add(live, sizeof(room_locked_budget_callback)) || !reserve(live, context))
		return false;
	try
	{
		const std::string identity = operation_key(expected.command.operation_id);
		std::unique_lock<std::mutex> lock(coordinator_mutex);
		room_locked_budget_callback locked_budget{ lock, reserve, context };
		const auto found = operations.find(identity);
		if (found == operations.end() || !found->second->retain_until_publication ||
		    !room_retire_matches_bounded(*found->second, expected,
						 room_locked_budget_callback::relay, &locked_budget,
						 live) ||
		    !health.initialized || !coordinator_generation ||
		    coordinator_generation_exhausted || stop_requested ||
		    found->second->publication_checkpointing ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()))
			return false;
		*output = coordinator_generation;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool critical_zone_reset_item_publication_owner::cancel_refusal_bounded(
	const critical_native_recovery_envelope &original, const critical_completion &expected,
	uint64_t generation,
	bool (*native_cleanup)(const critical_command &, const critical_completion &, void *,
			       size_t) noexcept,
	void *cleanup_context, bool (*reserve)(size_t, void *) noexcept, void *budget_context,
	size_t outer, bool *cleanup_called, bool *cleanup_succeeded) noexcept
{
	if (cleanup_called)
		*cleanup_called = false;
	if (cleanup_succeeded)
		*cleanup_succeeded = false;
	if (!native_cleanup || !reserve || !cleanup_called || !cleanup_succeeded ||
	    cleanup_called == cleanup_succeeded || !generation ||
	    !zone_reset_typed_command(original.command) ||
	    expected.operation_id.bytes != original.command.operation_id.bytes ||
	    !critical_completion_disposition_valid(expected) ||
	    expected.disposition != critical_completion_disposition::never_admitted)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	std::string identity;
	critical_native_recovery_envelope frozen;
	critical_completion receipt = expected;
	operation_state *pinned = nullptr;
	const auto matches = [&](const operation_state &state) noexcept
	{
		return operation_is_admission_failed(state) && state.owned_refusal_delivered &&
		       state.retain_until_publication && state.native &&
		       state.native->revision == frozen.revision &&
		       state.native->phase == frozen.phase &&
		       state.native->attachment == frozen.attachment &&
		       native_receipt_equal(state.admission_failure_completion, receipt);
	};
	size_t live = outer;
	// Every owned fixed object and the original operation_key allocation remain
	// live through cleanup. Sequential locks/fence lookups share one reservation.
	// Receipt is a fixed inline value, including its complete result/correlation.
	if (!room_retire_add(live, sizeof(identity)) ||
	    !room_retire_add(live, original.command.operation_id.bytes.size() + 1) ||
	    !room_retire_add(live, sizeof(frozen)) || !room_retire_add(live, sizeof(receipt)) ||
	    !room_retire_add(live, sizeof(matches)) ||
	    !room_retire_add(live, sizeof(std::unique_lock<std::mutex>)) ||
	    !room_retire_add(live, sizeof(room_locked_budget_callback)) ||
	    !room_retire_add(live, 2 * sizeof(std::array<char, 9>)) ||
	    !room_retire_add(live, sizeof(std::string_view)) || !reserve(live, budget_context))
		return false;
	try
	{
		// The genuine cleanup can destroy the caller's native owner. Freeze every
		// command/BODY byte and receipt before its operation is pinned.
		size_t copy_live = live;
		if (!room_checkpoint_heap(original, true, copy_live) ||
		    !reserve(copy_live, budget_context))
			return false;
		frozen = original;
		if (!room_checkpoint_heap(frozen, false, live))
			return false;
		// Original move assignment materializes a return string alongside identity;
		// the heap request belongs to live, the return object's inline storage here.
		size_t key_live = live;
		if (!room_retire_add(key_live, sizeof(std::string)) ||
		    !reserve(key_live, budget_context))
			return false;
		identity = operation_key(frozen.command.operation_id);
		std::unique_lock<std::mutex> lock(coordinator_mutex);
		room_locked_budget_callback locked_budget{ lock, reserve, budget_context };
		auto found = operations.find(identity);
		if (found == operations.end() || found->second->publication_checkpointing ||
		    !health.initialized || coordinator_generation != generation ||
		    coordinator_generation_exhausted || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !matches(*found->second) ||
		    !room_retire_matches_bounded(*found->second, frozen,
						 room_locked_budget_callback::relay, &locked_budget,
						 live))
			return false;
		pinned = found->second.get();
		pinned->publication_checkpointing = true;
		++publication_checkpoints_inflight;
		++guarded_publications_inflight;
	}
	catch (...)
	{
		return false;
	}
	// Relay the full current coordinator prefix to the actual native owner.
	// Marker/context storage must belong to a caller frame surviving cleanup.
	*cleanup_called = true;
	const bool cleaned = native_cleanup(frozen.command, receipt, cleanup_context, live);
	*cleanup_succeeded = cleaned;
	std::lock_guard<std::mutex> lock(coordinator_mutex);
	--publication_checkpoints_inflight;
	--guarded_publications_inflight;
	publication_checkpoint_finished.notify_all();
	auto found = operations.find(identity);
	if (found == operations.end() || coordinator_generation != generation ||
	    found->second.get() != pinned)
		return false;
	const bool same = found->second->publication_checkpointing && matches(*found->second);
	found->second->publication_checkpointing = false;
	if (!cleaned || !same || !health.initialized || coordinator_generation_exhausted ||
	    stop_requested ||
	    (lifecycle_guard_active && lifecycle_guard_thread != std::this_thread::get_id()))
		return false;
	// Exact original delivered never-admitted owner: no journal or ACK authority.
	// All post-cleanup checks, latch rollback and successful removal are original
	// nonallocating work; no codec, reserve or owner callback runs in this tail.
	remove_fences(identity, found->second->command);
	operations.erase(found);
	update_depth();
	work_available.notify_all();
	return true;
#endif
}

bool critical_zone_reset_item_publication_owner::completion_bounded(
	const critical_operation_id &operation_id, critical_completion *completion,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!reserve || !completion || critical_operation_id_is_zero(operation_id))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	return false;
#else
	size_t live = outer;
	// The actual original key is directly initialized under the original mutex:
	// its 16-byte binary identity requests 17 chars on the supported string ABI.
	// Caller-owned fixed receipt output and its other retained state belong to
	// outer; this lookup constructs no completion clone or command codec.
	if (!room_retire_add(live, sizeof(std::lock_guard<std::mutex>)) ||
	    !room_retire_add(live, sizeof(std::string)) ||
	    !room_retire_add(live, operation_id.bytes.size() + 1) ||
	    !room_retire_add(live, sizeof(decltype(operations)::iterator)) ||
	    !room_retire_add(live, sizeof(decltype(completed_cache)::iterator)) ||
	    !reserve(live, context))
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const std::string identity = operation_key(operation_id);
		auto operation = operations.find(identity);
		if (operation != operations.end() &&
		    operation_is_publication_pending(*operation->second))
		{
			*completion = operation->second->publication_completion;
			return true;
		}
		const auto found = completed_cache.find(identity);
		if (found == completed_cache.end())
			return false;
		*completion = found->second.completion;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool critical_zone_reset_item_publication_owner::admission_supported_bounded(
	const critical_command &command, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer) noexcept
{
	if (!reserve || command.type != critical_command_type::zone_reset_item_birth)
		return false;
	size_t live = outer;
	if (!room_retire_add(live, sizeof(std::unique_lock<std::mutex>)) ||
	    !room_retire_add(live, sizeof(room_locked_budget_callback)) || !reserve(live, context))
		return false;
	try
	{
		std::unique_lock<std::mutex> lock(coordinator_mutex);
		room_locked_budget_callback locked_budget{ lock, reserve, context };
		// Same original execution_supported predicates, specialized only to ROOM.
		// The original callback remains mandatory. Never fall back to invoking its
		// unbounded allocator when the paired prospective provider is absent.
		if (critical_command_valid(command))
			return true;
		return command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		       critical_command_envelope_valid(command) && extension_validator_callback &&
		       extension_validator_bounded_callback &&
		       extension_validator_bounded_callback(
			       command, room_locked_budget_callback::relay, &locked_budget, live);
	}
	catch (...)
	{
		return false;
	}
}

namespace
{
bool room_storage_add(size_t &total, size_t bytes) noexcept
{
	if (bytes > SIZE_MAX - total)
		return false;
	total += bytes;
	return true;
}
bool room_storage_array(size_t &total, size_t capacity, size_t width) noexcept
{
	return (!width || capacity <= SIZE_MAX / width) &&
	       room_storage_add(total, capacity * width);
}
bool room_storage_command(size_t &total, const critical_command &command) noexcept
{
	return room_storage_array(total, command.keys.capacity(), sizeof(critical_entity_key)) &&
	       room_storage_array(total, command.expected_revisions.capacity(),
				  sizeof(critical_expected_revision)) &&
	       room_storage_add(total, command.payload.capacity()) &&
	       room_storage_add(total, command.accounting_intent.capacity());
}
bool room_storage_string(size_t &total, const std::string &value) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return value.capacity() <= 15 ||
	       (value.capacity() != SIZE_MAX && room_storage_add(total, value.capacity() + 1));
#else
	(void)total;
	(void)value;
	return false;
#endif
}
template <typename Map> bool room_storage_map(size_t &total, const Map &map) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	using node = std::__detail::_Hash_node<
		typename Map::value_type,
		std::__cache_default<typename Map::key_type, typename Map::hasher>::value>;
	if (!map.bucket_count() ||
	    (map.bucket_count() > 1 &&
	     !room_storage_array(total, map.bucket_count(),
				 sizeof(std::__detail::_Hash_node_base *))) ||
	    !room_storage_array(total, map.size(), sizeof(node)))
		return false;
	for (const auto &entry : map)
		if (!room_storage_string(total, entry.first))
			return false;
	return true;
#else
	(void)total;
	(void)map;
	return false;
#endif
}
// Pure current retained owner census. Its caller owns the actual coordinator
// mutex. Do not call through an unlocked root global observer or journal lock.
bool room_coordinator_current_storage_bytes_locked(size_t *output) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)output;
	return false;
#else
	if (!output)
		return false;
	size_t total =
		sizeof(active_mixed_startup_owner) + sizeof(coordinator_mutex) +
		critical_room_shared_budget_lender::retained_storage_bytes() +
		sizeof(work_available) + sizeof(result_available) + sizeof(admission_available) +
		sizeof(publication_checkpoint_finished) + sizeof(publication_checkpoints_inflight) +
		sizeof(guarded_publications_inflight) + sizeof(operations) + sizeof(pending) +
		sizeof(pending_admission) + sizeof(completion_delivery) + sizeof(active_keys) +
		sizeof(fences) + sizeof(completed_cache) + sizeof(completed_order) +
		sizeof(completed_cache_bytes) + sizeof(pending_admission_bytes) +
		sizeof(admission_inflight_bytes) + sizeof(workers) + sizeof(admission_worker) +
		sizeof(apply_callback) + sizeof(shared_native_apply_callback) +
		sizeof(ordinary_native_apply_callback) + sizeof(zone_reset_apply_callback) +
		sizeof(extension_validator_callback) +
		sizeof(extension_validator_bounded_callback) +
		sizeof(native_replay_observer_callback) +
		sizeof(native_replay_observer_bounded_callback) +
		sizeof(native_publication_validator_callback) + sizeof(native_birth_validators) +
		sizeof(native_quest_pair_validator) + sizeof(native_auction_validators) +
		sizeof(zone_reset_validators) + sizeof(apply_context) + sizeof(drain_observer) +
		sizeof(health) + sizeof(stop_requested) + sizeof(lifecycle_guard_active) +
		sizeof(lifecycle_guard_was_accepting) +
		sizeof(lifecycle_guard_initialized_runtime) + sizeof(lifecycle_guard_thread) +
		sizeof(recovery_requested) + sizeof(uncertain_recovery_not_before_usec) +
		sizeof(uncertain_recovery_delay_usec) + sizeof(coordinator_generation) +
		sizeof(coordinator_generation_exhausted) + sizeof(next_cutover_lease_id) +
		sizeof(cutover_lease_ids_exhausted) + sizeof(active_cutover_generation) +
		sizeof(active_cutover_lease_id) + sizeof(active_cutover_phase) +
		sizeof(active_cutover_thread) + sizeof(active_cutover_connection) +
		sizeof(active_cutover_session) + sizeof(cutover_reopen_allowed) +
		sizeof(cutover_was_accepting) + sizeof(cutover_outcome_uncertain) +
		sizeof(cutover_runtime_origin) + sizeof(cutover_runtime_was_accepting);
	if (!room_storage_map(total, operations) || !room_storage_map(total, active_keys) ||
	    !room_storage_map(total, fences) || !room_storage_map(total, completed_cache) ||
	    !room_storage_array(total, workers.capacity(), sizeof(std::thread)))
		return false;
	size_t heap = 0;
	if (!pending.current_heap_bytes(&heap) || !room_storage_add(total, heap) ||
	    !pending_admission.current_heap_bytes(&heap) || !room_storage_add(total, heap) ||
	    !completed_order.current_heap_bytes(&heap) || !room_storage_add(total, heap) ||
	    !completion_delivery.current_heap_bytes(&heap) || !room_storage_add(total, heap))
		return false;
	for (const auto &entry : active_keys)
		if (!room_storage_string(total, entry.second))
			return false;
	for (const auto &entry : fences)
		if (!entry.second.current_heap_bytes(&heap) || !room_storage_add(total, heap))
			return false;
	for (const auto &entry : completed_cache)
		if (!room_storage_command(total, entry.second.command))
			return false;
	for (const auto &entry : operations)
	{
		if (!entry.second || !room_storage_add(total, sizeof(operation_state)) ||
		    !room_storage_command(total, entry.second->command))
			return false;
		const auto &state = *entry.second;
		if (state.native && (!room_storage_add(total, sizeof(native_operation_context)) ||
				     !room_storage_add(total, state.native->attachment.capacity())))
			return false;
		// Real immutable capacity proof captured by the sole flat owner before
		// transfer. Do not inspect mutable worker-owned participant state here.
		// Pointer/charge slots are already in sizeof(operation_state), exactly once.
		if (state.ordinary_flat_transaction)
		{
			const size_t slots = sizeof(state.ordinary_flat_transaction) +
					     sizeof(state.ordinary_flat_transaction_bytes);
			if (state.ordinary_flat_transaction_bytes <
				    slots + sizeof(*state.ordinary_flat_transaction) ||
			    !room_storage_add(total, state.ordinary_flat_transaction_bytes - slots))
				return false;
		}
		else if (state.ordinary_flat_transaction_bytes)
			return false;
		if (state.flat_transaction)
		{
			const size_t slots = sizeof(state.flat_transaction) +
					     sizeof(state.flat_transaction_bytes);
			if (state.flat_transaction_bytes <
				    slots + sizeof(*state.flat_transaction) ||
			    !room_storage_add(total, state.flat_transaction_bytes - slots))
				return false;
		}
		else if (state.flat_transaction_bytes)
			return false;
		if (state.room_flat_transaction)
		{
			const size_t slots = sizeof(state.room_flat_transaction) +
					     sizeof(state.room_flat_transaction_bytes);
			if (state.room_flat_transaction_bytes <
				    slots + sizeof(*state.room_flat_transaction) ||
			    !room_storage_add(total, state.room_flat_transaction_bytes - slots))
				return false;
		}
		else if (state.room_flat_transaction_bytes)
			return false;
	}
	*output = total;
	return true;
#endif
}
} // namespace

bool critical_zone_reset_item_publication_owner::current_storage_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		return room_coordinator_current_storage_bytes_locked(output);
	}
	catch (...)
	{
		return false;
	}
}

// Complete one-callback scope. This real inline frame stays live through ROOT's
// reserve/charge; its bytes are prospectively included with the entire request.
// Only the genuine locked current census is lent to the shared aggregate.
bool critical_room_shared_budget_lender::reserve(const std::unique_lock<std::mutex> &actual_lock,
						 bool (*actual_reserve)(size_t, void *) noexcept,
						 void *actual_guard, size_t exclusive_live,
						 size_t *current_output) noexcept
{
	if (!actual_reserve || actual_lock.mutex() != &coordinator_mutex ||
	    !actual_lock.owns_lock())
		return false;
	callback_borrow_frame scope{ actual_lock, actual_reserve, actual_guard, current_output };
	if (!room_coordinator_current_storage_bytes_locked(&scope.current))
		return false;
	if (scope.output)
		*scope.output = scope.current;
	scope.full = exclusive_live;
	// Whole-operation profile already owns this actual callback carrier.
	// Original one-callback callers retain their original carrier addition.
	if ((!active_operation_ && !room_storage_add(scope.full, sizeof(callback_borrow_frame))) ||
	    !room_storage_add(scope.full, scope.current))
		return false;
	// Original unregistered callbacks receive the complete prefix, never a
	// guessed partial budget or new game-thread requirement. Once registered,
	// the real shared owner authenticates callback/context/exact actual guard.
	if (item_native_quest_coordinator_budget_scope_owner::registered())
	{
		if (active_operation_)
		{
			// The entire operation owns the loan. Refresh its exact identity with
			// CURRENT scanned under the same actual lock; never create a second loan.
			if (&active_operation_->lock != &scope.lock ||
			    active_operation_->reserve != scope.reserve ||
			    active_operation_->guard != scope.guard ||
			    !item_native_quest_coordinator_budget_scope_owner::refresh_borrow(
				    active_operation_, scope.reserve, scope.guard, scope.current))
				return false;
		}
		else
		{
			if (!item_native_quest_coordinator_budget_scope_owner::begin_borrow(
				    &scope, scope.reserve, scope.guard, scope.current))
				return false;
			scope.borrowed = true;
		}
	}
	// noexcept callback, no allocating work or early return between scalar begin
	// and scalar end. ROOT exclusive_prefix subtracts only this exact CURRENT C
	// before its old max(); shared capacity adds that same C once while borrowed.
	scope.accepted = scope.reserve(scope.full, scope.guard);
	scope.ended = !scope.borrowed ||
		      item_native_quest_coordinator_budget_scope_owner::end_borrow(&scope);
	return scope.accepted && scope.ended;
}

bool critical_room_shared_budget_lender::reset_before_replay(
	const std::unique_lock<std::mutex> &actual_lock) noexcept
{
	return actual_lock.mutex() == &coordinator_mutex && actual_lock.owns_lock() &&
	       item_native_quest_coordinator_budget_scope_owner::reset_before_replay();
}

namespace
{
bool mixed_startup_checkpoint_size_bounded(const critical_native_recovery_envelope &, size_t *,
					   bool (*)(size_t, void *) noexcept, void *,
					   size_t) noexcept;

// One genuine live owner, including all original local carriers, lock and
// lookup/capacity scratch. No probe containers or guessed resize state.
struct room_submit_workspace
{
	critical_native_recovery_envelope envelope;
	std::string identity;
	std::string fence_key;
	std::unique_ptr<operation_state> state;
	std::unique_lock<std::mutex> lock;
	decltype(operations)::iterator found;
	decltype(fences)::iterator fence_found;
	decltype(completed_cache)::iterator completed;
	native_identity_queue *fence_queue = nullptr;
	bool (*reserve)(size_t, void *) noexcept = nullptr;
	void *context = nullptr;
	size_t *current_output = nullptr;
	size_t outer = 0, partial = 0, current = 0, absolute = 0;
	size_t retained = 0, extra = 0, fresh = 0, key_index = 0;
	bool denied = false, admission_queued = false, inserted = false, prefix_valid = false;
	bool rollback_cleanup_reserved = false;
	room_submit_workspace(bool (*callback)(size_t, void *) noexcept, void *opaque,
			      size_t caller_outer, size_t *output)
		: lock(coordinator_mutex, std::defer_lock)
		, reserve(callback)
		, context(opaque)
		, current_output(output)
		, outer(caller_outer)
	{
	}
};

// Caller owns coordinator_mutex for every use, including nested codec requests.
// partial already includes actual caller/local carriers and nested codec peak;
// this relay adds ONLY current persistent coordinator ownership exactly once.
bool room_submit_reserve_locked(size_t partial, void *opaque) noexcept
{
	auto &work = *static_cast<room_submit_workspace *>(opaque);
	if (!critical_room_shared_budget_lender::reserve(work.lock, work.reserve, work.context,
							 partial, work.current_output))
	{
		work.denied = true;
		return false;
	}
	return true;
}

// Refresh real local ownership after every clone/move/allocation. A moved-from
// vector's actual capacity is measured, never inferred to be zero. A consumed
// unique_ptr is naturally excluded while its state is counted by the census.
bool room_submit_local_prefix(room_submit_workspace &work, size_t request = 0) noexcept
{
	work.partial = work.outer;
	work.prefix_valid =
		room_storage_add(work.partial, sizeof(room_submit_workspace)) &&
		(!work.rollback_cleanup_reserved ||
		 room_storage_add(work.partial,
				  2 * sizeof(std::array<char, 9>) + sizeof(std::string_view))) &&
		room_storage_command(work.partial, work.envelope.command) &&
		room_storage_add(work.partial, work.envelope.attachment.capacity()) &&
		room_storage_string(work.partial, work.identity) &&
		room_storage_string(work.partial, work.fence_key) &&
		(!work.state ||
		 (room_storage_add(work.partial, sizeof(operation_state)) &&
		  room_storage_command(work.partial, work.state->command) &&
		  (!work.state->native ||
		   (room_storage_add(work.partial, sizeof(native_operation_context)) &&
		    room_storage_add(work.partial, work.state->native->attachment.capacity()))))) &&
		room_storage_add(work.partial, request);
	if (!work.prefix_valid)
		work.denied = true;
	return work.prefix_valid;
}

bool room_submit_admit(room_submit_workspace &work, size_t request = 0) noexcept
{
	if (!room_submit_local_prefix(work, request))
	{
		work.denied = true;
		return false;
	}
	return room_submit_reserve_locked(work.partial, &work);
}

critical_submit_result room_submit_return(room_submit_workspace &work,
					  critical_submit_result result) noexcept
{
	// Pure observation only: no fallible budget callback after attachment or the
	// successful accepted/trace/notification tail. Output is never zero-as-empty.
	if (room_coordinator_current_storage_bytes_locked(&work.current))
		*work.current_output = work.current;
	return result;
}

critical_submit_result room_submit_rollback(room_submit_workspace &work) noexcept
{
	// Same original rollback/removal/health sequence. Also covers a false budget
	// result between default construction of a genuine new fence and its push.
	if (work.inserted)
	{
		work.found = operations.find(work.identity);
		if (work.found != operations.end())
		{
			remove_fences(work.identity, work.found->second->command);
			operations.erase(work.found);
		}
		pending_admission.erase(std::remove(pending_admission.begin(),
						    pending_admission.end(), work.identity),
					pending_admission.end());
		if (work.admission_queued)
			pending_admission_bytes -= work.retained;
	}
	work.state.reset();
	++health.overloads;
	update_depth();
	// Retained buckets/deque maps/blocks surviving rollback are included by the
	// authentic same-lock census. No modeled restoration to pre-admission size.
	return room_submit_return(work, critical_submit_result::overloaded);
}
} // namespace

critical_submit_result critical_zone_reset_item_publication_owner::submit_bounded(
	const critical_native_recovery_envelope &original, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer, size_t *current_coordinator_bytes) noexcept
{
	if (!reserve || !current_coordinator_bytes || !zone_reset_typed_command(original.command) ||
	    original.revision != 1 ||
	    original.phase != critical_native_recovery_phase::execution_pending)
		return critical_submit_result::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return critical_submit_result::overloaded;
#else
	room_submit_workspace work(reserve, context, outer, current_coordinator_bytes);
	try
	{
		// All complete canonical proofs and genuine clone allocations now run
		// under the original mutex so the FIRST request contains actual current
		// coordinator ownership. Original pure predicate order remains unchanged.
		work.lock.lock();
		if (!room_submit_admit(work))
			return room_submit_rollback(work);
		if (!room_submit_local_prefix(work) ||
		    !room_checkpoint_size_bounded(original, &work.retained,
						  room_submit_reserve_locked, &work, work.partial))
			return work.denied ?
				       room_submit_rollback(work) :
				       room_submit_return(work, critical_submit_result::invalid);
		// Complete original carrier copy, all vector request sizes genuine. The
		// authentic original belongs to caller outer and remains live throughout.
		work.fresh = 0;
		if (!room_checkpoint_heap(original, true, work.fresh) ||
		    !room_submit_admit(work, work.fresh))
			return room_submit_rollback(work);
		work.envelope = original;
		if (!room_submit_admit(work))
			return room_submit_rollback(work);
		// Same original execution_supported proof specialized to the exact ROOM
		// wrapper gate. Type22 is outside legacy valid; the original selected
		// extension callback stays mandatory, paired with its genuine companion.
		if (!room_submit_local_prefix(work) ||
		    !(critical_command_valid(work.envelope.command) ||
		      (work.envelope.command.schema_version ==
			       CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		       critical_command_envelope_valid(work.envelope.command) &&
		       extension_validator_callback && extension_validator_bounded_callback &&
		       extension_validator_bounded_callback(work.envelope.command,
							    room_submit_reserve_locked, &work,
							    work.partial))) ||
		    !zone_reset_validators_ready() || !zone_reset_validators.initial_bounded ||
		    !zone_reset_validators.initial_bounded(
			    work.envelope, room_submit_reserve_locked, &work, work.partial))
			return work.denied ?
				       room_submit_rollback(work) :
				       room_submit_return(work, critical_submit_result::invalid);
		if (!health.initialized || !health.accepting || stop_requested ||
		    !native_replay_observer_callback)
			return room_submit_return(work, critical_submit_result::unavailable);
		// operation_key's 16 bytes require an actual 17-byte GCC13 string request;
		// assignment also owns its returned temporary string until the full expression.
		work.extra = sizeof(std::string);
		if (!room_storage_add(work.extra, original.command.operation_id.bytes.size() + 1) ||
		    !room_submit_admit(work, work.extra))
			return room_submit_rollback(work);
		work.identity = operation_key(work.envelope.command.operation_id);
		if (!room_submit_admit(work))
			return room_submit_rollback(work);
		work.completed = completed_cache.find(work.identity);
		if (work.completed != completed_cache.end())
			return room_submit_return(work, critical_submit_result::identity_conflict);
		work.found = operations.find(work.identity);
		if (work.found != operations.end())
		{
			if (!room_submit_local_prefix(work) ||
			    !room_retire_matches_bounded(*work.found->second, work.envelope,
							 room_submit_reserve_locked, &work,
							 work.partial))
				return work.denied ?
					       room_submit_rollback(work) :
					       room_submit_return(
						       work,
						       critical_submit_result::identity_conflict);
			++work.found->second->attachments;
			++health.attached;
			return room_submit_return(work, critical_submit_result::attached);
		}
		if (operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
		    work.retained > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
		{
			++health.overloads;
			return room_submit_return(work, critical_submit_result::overloaded);
		}
		if (!room_submit_admit(work, sizeof(operation_state) +
						     sizeof(std::unique_ptr<operation_state>)))
			return room_submit_rollback(work);
		work.state = std::make_unique<operation_state>();
		if (!room_submit_admit(work))
			return room_submit_rollback(work);
		work.state->command = std::move(work.envelope.command);
		if (!room_submit_admit(work,
				       sizeof(native_operation_context) +
					       sizeof(std::unique_ptr<native_operation_context>)))
			return room_submit_rollback(work);
		work.state->native = std::make_unique<native_operation_context>();
		work.state->native->revision = work.envelope.revision;
		work.state->native->phase = work.envelope.phase;
		work.state->native->attachment = std::move(work.envelope.attachment);
		work.state->retained_bytes = work.retained;
		work.state->queued_at_usec = now_usec();
		work.state->attempt = 1;
		work.state->attachments = 0;
		work.state->phase = critical_operation_phase::awaiting_durability;
		work.state->retain_until_publication = true;
		work.state->admission_failure_queued = false;
		// Genuine remove_fences owns both optional-NRVO array objects and its
		// string_view. Admit cleanup BEFORE insertion can create rollback work;
		// retain this allowance through every later prefix. Never call reserve
		// while cleaning up after denial or a genuine allocation exception.
		work.rollback_cleanup_reserved = true;
		if (!operations.next_unique_insert_extra_peak(work.identity, 0, &work.extra) ||
		    !room_submit_admit(work, work.extra))
			return room_submit_rollback(work);
		operations.emplace(work.identity, std::move(work.state));
		work.inserted = true;
		if (!room_submit_admit(work) ||
		    !pending_admission.push_back_extra_peak(work.identity, &work.extra) ||
		    !room_submit_admit(work, work.extra))
			return room_submit_rollback(work);
		pending_admission.push_back(work.identity);
		pending_admission_bytes += work.retained;
		work.admission_queued = true;
		if (!room_submit_admit(work))
			return room_submit_rollback(work);
		// Genuine original add_fences, with admission between the real map's
		// absent-key/default queue construction and that real queue's push.
		work.found = operations.find(work.identity);
		for (work.key_index = 0; work.key_index < work.found->second->command.keys.size();
		     ++work.key_index)
		{
			if (!room_submit_admit(work, sizeof(std::string) +
							     2 * sizeof(std::array<char, 9>)))
				return room_submit_rollback(work);
			work.fence_key =
				entity_key(work.found->second->command.keys[work.key_index]);
			work.fence_found = fences.find(work.fence_key);
			if (work.fence_found == fences.end())
			{
				if (!native_identity_queue::initial_heap_bytes(&work.fresh) ||
				    !fences.next_unique_insert_extra_peak(
					    work.fence_key, work.fresh, &work.extra) ||
				    !room_submit_admit(work, work.extra))
					return room_submit_rollback(work);
				// Original entity_key result is an rvalue and invokes this exact
				// genuine table/default-mapped construction path.
				work.fence_queue = &fences[std::move(work.fence_key)];
				if (!room_submit_admit(work))
					return room_submit_rollback(work);
			}
			else
				work.fence_queue = &work.fence_found->second;
			if (!work.fence_queue->push_back_extra_peak(work.identity, &work.extra) ||
			    !room_submit_admit(work, work.extra))
				return room_submit_rollback(work);
			work.fence_queue->push_back(work.identity);
			if (!room_submit_admit(work))
				return room_submit_rollback(work);
		}
		health.fenced_keys = fences.size();
		// Actual fixed diagnostic return/argument objects (NRVO optional) and
		// recorder's unique_lock, before the original nonallocating accepted tail.
		if (!room_submit_admit(work, 2 * sizeof(persistence_trace_event) +
						     sizeof(std::unique_lock<std::mutex>)))
			return room_submit_rollback(work);
		persistence_trace_record(
			persistence_command_trace(operations.at(work.identity)->command,
						  persistence_trace_stage::command_admitted));
		++health.accepted;
		update_depth();
		admission_available.notify_one();
		return room_submit_return(work, critical_submit_result::awaiting_durability);
	}
	catch (...)
	{
		// Only genuine standard-library exceptions enter here. A failed lock owns
		// no coordinator state and must not perform an unlocked census or rollback.
		if (!work.lock.owns_lock())
			return critical_submit_result::unavailable;
		return room_submit_rollback(work);
	}
#endif
}

namespace
{
// Genuine init owns coordinator_mutex throughout this private ROOM callback.
// Input envelope, replay context and original lock belong to caller_outer.
// Private state stays here until its exact transfer into CURRENT coordinator.
struct room_replay_workspace
{
	const std::unique_lock<std::mutex> &lock;
	std::string identity;
	std::string fence_key;
	std::unique_ptr<operation_state> state;
	decltype(operations)::iterator found;
	decltype(fences)::iterator fence_found;
	native_identity_queue *fence_queue = nullptr;
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer = 0, partial = 0, retained = 0, extra = 0, fresh = 0, key_index = 0;
	bool continuation = false, holds_fences = false, inserted = false;
	bool rollback_cleanup_reserved = false;
	room_replay_workspace(const std::unique_lock<std::mutex> &actual_lock,
			      bool (*callback)(size_t, void *) noexcept, void *opaque,
			      size_t caller_outer) noexcept
		: lock(actual_lock)
		, reserve(callback)
		, context(opaque)
		, outer(caller_outer)
	{
	}
};

bool room_replay_reserve_locked(size_t exclusive_live, void *opaque) noexcept
{
	auto &work = *static_cast<room_replay_workspace *>(opaque);
	// A genuine whole-operation callback already refreshes CURRENT C under
	// this exact lock. Forward its actual relay/context, never reborrow using
	// the relay itself as if it were the registered ROOT callback/guard pair.
	if (critical_room_shared_budget_lender::callback_matches(work.lock, work.reserve,
								 work.context))
		return work.reserve(exclusive_live, work.context);
	// Preserve the original single-callback path outside a matching operation.
	return critical_room_shared_budget_lender::reserve(work.lock, work.reserve, work.context,
							   exclusive_live);
}

bool room_replay_local_prefix(room_replay_workspace &work, size_t request = 0) noexcept
{
	work.partial = work.outer;
	return room_storage_add(work.partial, sizeof(room_replay_workspace)) &&
	       (!work.rollback_cleanup_reserved ||
		room_storage_add(work.partial,
				 2 * sizeof(std::array<char, 9>) + sizeof(std::string_view))) &&
	       room_storage_string(work.partial, work.identity) &&
	       room_storage_string(work.partial, work.fence_key) &&
	       (!work.state ||
		(room_storage_add(work.partial, sizeof(operation_state)) &&
		 room_storage_command(work.partial, work.state->command) &&
		 (!work.state->native ||
		  (room_storage_add(work.partial, sizeof(native_operation_context)) &&
		   room_storage_add(work.partial, work.state->native->attachment.capacity()))))) &&
	       room_storage_add(work.partial, request);
}

bool room_replay_admit(room_replay_workspace &work, size_t request = 0) noexcept
{
	return room_replay_local_prefix(work, request) &&
	       room_replay_reserve_locked(work.partial, &work);
}

bool room_replay_rollback(room_replay_workspace &work) noexcept
{
	// Original rollback, including empty newly constructed fence queues. Its
	// genuine fixed cleanup carriers were reserved BEFORE table insertion.
	// No callback, codec, new carrier or synthetic throw follows a refusal.
	if (work.inserted)
	{
		work.found = operations.find(work.identity);
		if (work.found != operations.end())
		{
			if (work.holds_fences)
				remove_fences(work.identity, work.found->second->command);
			operations.erase(work.found);
		}
	}
	work.state.reset();
	pending.erase(std::remove(pending.begin(), pending.end(), work.identity), pending.end());
	update_depth();
	return false;
}

// Private unselected ROOM replay insertion, NOT submission or mixed-family
// fallback. The complete mixed caller owns the real init lock and authentic
// journal/input prefix. Its optional observer must be the provided genuine
// startup restore callback with original duplicate/new-registry rollback.
[[maybe_unused]] bool
enqueue_room_native_replayed_bounded(const critical_native_recovery_envelope &original,
				     const replay_observer_context &replay,
				     const std::unique_lock<std::mutex> &actual_init_lock,
				     bool (*reserve)(size_t, void *) noexcept, void *budget_context,
				     size_t exclusive_outer) noexcept
{
	if (!reserve || !replay.native_observer || !replay.native_bounded_observer ||
	    !zone_reset_typed_command(original.command) ||
	    actual_init_lock.mutex() != &coordinator_mutex || !actual_init_lock.owns_lock())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	room_replay_workspace work(actual_init_lock, reserve, budget_context, exclusive_outer);
	try
	{
		if (!room_replay_admit(work))
			return false;
		// Same selected native_envelope_execution_supported decision for ROOM:
		// original execution support remains mandatory and its genuine bounded
		// companion performs the exact allocating proof under this real lock.
		if (!room_replay_local_prefix(work) ||
		    !(critical_command_valid(original.command) ||
		      (original.command.schema_version ==
			       CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		       critical_command_envelope_valid(original.command) &&
		       extension_validator_callback && extension_validator_bounded_callback &&
		       extension_validator_bounded_callback(
			       original.command, room_replay_reserve_locked, &work, work.partial))))
			return false;
		if (!room_replay_admit(work) ||
		    !room_checkpoint_size_bounded(original, &work.retained,
						  room_replay_reserve_locked, &work, work.partial))
			return false;
		// Replay accepts authentic execution AND continuation. initial_bounded
		// would incorrectly reject phase2/revision continuations and is not used.
		if (!room_replay_admit(work) || !zone_reset_validators_ready() ||
		    !zone_reset_validators.valid_bounded ||
		    !zone_reset_validators.valid_bounded(original, room_replay_reserve_locked,
							 &work, work.partial))
			return false;
		if (!room_replay_admit(work))
			return false;
		// Original operation_key return/local coexistence. Its actual 16-byte
		// GCC13 string requests 17 bytes; assignment consumes the returned string.
		work.extra = sizeof(std::string);
		if (!room_storage_add(work.extra, original.command.operation_id.bytes.size() + 1) ||
		    !room_replay_admit(work, work.extra))
			return false;
		work.identity = operation_key(original.command.operation_id);
		if (!room_replay_admit(work))
			return false;
		if (operations.find(work.identity) != operations.end() ||
		    operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
		    work.retained > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
			return false;
		work.continuation = original.phase ==
				    critical_native_recovery_phase::continuation_pending;
		work.holds_fences = !work.continuation ||
				    native_birth_origin_command(original.command) ||
				    zone_reset_typed_command(original.command);
		if (!room_replay_admit(work, sizeof(operation_state) +
						     sizeof(std::unique_ptr<operation_state>)))
			return room_replay_rollback(work);
		work.state = std::make_unique<operation_state>();
		if (!room_replay_admit(work))
			return room_replay_rollback(work);
		// Fresh default state owns no vector capacity. This genuine source profile
		// reserves ALL command/attachment copy requests and the original native
		// allocation before that complete original copy sequence. There is no
		// callback inside the sequence, and its resulting private state is censused
		// at the next cut using actual vector capacities, not encoded byte length.
		work.fresh = 0;
		if (!room_checkpoint_heap(original, true, work.fresh) ||
		    !room_storage_add(work.fresh, sizeof(native_operation_context)) ||
		    !room_storage_add(work.fresh,
				      sizeof(std::unique_ptr<native_operation_context>)) ||
		    !room_replay_admit(work, work.fresh))
			return room_replay_rollback(work);
		work.state->command = original.command;
		work.state->native = std::make_unique<native_operation_context>();
		work.state->native->revision = original.revision;
		work.state->native->phase = original.phase;
		work.state->native->attachment = original.attachment;
		work.state->retained_bytes = work.retained;
		work.state->queued_at_usec = now_usec();
		work.state->attempt = 1;
		work.state->attachments = 0;
		work.state->phase = work.continuation ?
					    critical_operation_phase::native_continuation_pending :
					    critical_operation_phase::queued;
		work.state->retain_until_publication = true;
		work.state->admission_failure_queued = false;
		work.state->native_physical_released = work.continuation;
		work.rollback_cleanup_reserved = true;
		if (!operations.next_unique_insert_extra_peak(work.identity, 0, &work.extra) ||
		    !room_replay_admit(work, work.extra))
			return room_replay_rollback(work);
		operations.emplace(work.identity, std::move(work.state));
		work.inserted = true;
		// state is now genuinely owned by CURRENT C; the moved-from pointer is
		// excluded from private scratch before any pending/fence request.
		if (!room_replay_admit(work))
			return room_replay_rollback(work);
		if (!work.continuation)
		{
			if (!pending.push_back_extra_peak(work.identity, &work.extra) ||
			    !room_replay_admit(work, work.extra))
				return room_replay_rollback(work);
			pending.push_back(work.identity);
			if (!room_replay_admit(work))
				return room_replay_rollback(work);
		}
		if (work.holds_fences)
		{
			work.found = operations.find(work.identity);
			for (work.key_index = 0;
			     work.key_index < work.found->second->command.keys.size();
			     ++work.key_index)
			{
				if (!room_replay_admit(work,
						       sizeof(std::string) +
							       2 * sizeof(std::array<char, 9>)))
					return room_replay_rollback(work);
				work.fence_key = entity_key(
					work.found->second->command.keys[work.key_index]);
				work.fence_found = fences.find(work.fence_key);
				if (work.fence_found == fences.end())
				{
					if (!native_identity_queue::initial_heap_bytes(
						    &work.fresh) ||
					    !fences.next_unique_insert_extra_peak(
						    work.fence_key, work.fresh, &work.extra) ||
					    !room_replay_admit(work, work.extra))
						return room_replay_rollback(work);
					work.fence_queue = &fences[std::move(work.fence_key)];
					if (!room_replay_admit(work))
						return room_replay_rollback(work);
				}
				else
					work.fence_queue = &work.fence_found->second;
				if (!work.fence_queue->push_back_extra_peak(work.identity,
									    &work.extra) ||
				    !room_replay_admit(work, work.extra))
					return room_replay_rollback(work);
				work.fence_queue->push_back(work.identity);
				if (!room_replay_admit(work))
					return room_replay_rollback(work);
			}
			health.fenced_keys = fences.size();
		}
		// Actual optional ROOM startup observer only; original mandatory observer
		// remains installed, but no unbounded mixed fallback runs on this route.
		// The genuine startup wrapper itself owns post-attach current/rollback.
		if (!room_replay_local_prefix(work) ||
		    !replay.native_bounded_observer(original, replay.context,
						    room_replay_reserve_locked, &work,
						    work.partial))
			return room_replay_rollback(work);
		// No fallible callback after successful new ROOT registry attachment.
		// Exact original successful tail is nonallocating.
		update_depth();
		return true;
	}
	catch (...)
	{
		// Genuine allocation exceptions only; actual init lock remains owned.
		// Partially copied private state and actual inserted nodes are removed,
		// while real surviving bucket/deque capacity stays in CURRENT C.
		return room_replay_rollback(work);
	}
#endif
}
} // namespace: complete private bounded ROOM passive replay insertion

namespace
{
namespace mixed_replay_state_construction_source
{
constexpr size_t P = sizeof(void *);
constexpr size_t N = sizeof(size_t);
// Actual default vector: vector, _Vector_base, _Vector_impl,
// allocator, __new_allocator and _Vector_impl_data receivers.
constexpr size_t vector_default = 6 * P;
// unique_ptr -> __uniq_ptr_data -> __uniq_ptr_impl -> tuple -> both
// _Tuple_impl levels -> both _Head_base levels -> default_delete.
// The scalar pointer head has no further class constructor.
constexpr size_t unique_default = 9 * P;
// Mutable impl pointer/deleter accessor -> get -> __get_helper ->
// _Tuple_impl::_M_head -> _Head_base::_M_head, each receiver/ref + result ref.
constexpr size_t tuple_member = 5 * (2 * P);
// Raw-pointer unique constructor and inherited data constructor, impl pointer
// constructor, six tuple/default-head/deleter defaults, mutable pointer access.
constexpr size_t pointer_unique = 3 * (2 * P) + 6 * P + tuple_member;
// Returned make_unique temporary is null after transfer. Its destructor owns
// this and pointer-ref, mutable pointer access, then the implicit data/impl/
// tuple/two impl/two head/deleter destructor receivers. No deleter body runs.
constexpr size_t moved_temporary_cleanup = 2 * P + tuple_member + 8 * P;
// Destination is the genuine default-null work.state or state.native field.
// Unique/data/impl assignments each this, source and returned ref; release
// owns this/local/result and two ptr accessors; reset owns this/new/old and two
// ptr accessors. Both deleter accessors, forward(ref/result), generated empty
// default_delete assignment(this/source/returned ref), null temporary cleanup.
// Binding make_unique's prvalue to && does NOT call std::move.
constexpr size_t null_destination_transfer = 3 * (3 * P) + (3 * P + 2 * tuple_member) +
					     (3 * P + 2 * tuple_member) + 2 * tuple_member + 2 * P +
					     3 * P + moved_temporary_cleanup;
// operation_state's actual generated default receiver; command receiver;
// four command vectors; four unique fields (native + three transaction slots);
// two completion receivers. Each default-initialized operation_id invokes its
// implicit id and array default receivers (three occurrences). The explicitly
// brace-initialized fixed completion arrays are aggregates, not extra calls.
constexpr size_t state_default =
	P + P + 4 * vector_default + 4 * unique_default + 2 * P + 3 * (2 * P);
// Actual native context generated default receiver and attachment vector.
constexpr size_t context_default = P + vector_default;
// No make_unique argument formal exists for the zero-argument overload. The
// operator-new size/result and new-expression result carrier are N+2P. Object
// and returned unique object storage are the original explicit heap/INLINE,
// not SOURCE. Out-of-line allocation runtime qualification remains separate.
constexpr size_t allocation_call = N + 2 * P;
constexpr size_t state_window =
	allocation_call + state_default + pointer_unique + null_destination_transfer;
// Context's assignment destination is state->native: actual unique arrow/get
// const tuple chain is seven receiver/result pairs. It is separate from the
// null destination assignment's own receiver and from actual native heap.
constexpr size_t context_window = allocation_call + context_default + pointer_unique +
				  null_destination_transfer + 7 * (2 * P);
#if defined(__linux__) && defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) &&               \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                         \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                      \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                   \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
constexpr bool supported = sizeof(void *) == 8 && sizeof(size_t) == 8 &&
			   sizeof(std::ptrdiff_t) == 8;
#else
constexpr bool supported = false;
#endif
} // namespace mixed_replay_state_construction_source
} // namespace

namespace
{
namespace mixed_replay_command_assignment_source
{
constexpr size_t P = sizeof(void *), N = sizeof(size_t), D = sizeof(std::ptrdiff_t);
constexpr size_t B = sizeof(bool);
constexpr size_t max_source(size_t a, size_t b) noexcept
{
	return a > b ? a : b;
}
// Real selected normal_iterator generated copy/move receiver + source; dtor receiver.
constexpr size_t iterator_copy = 2 * P, iterator_move = 2 * P, iterator_cleanup = P;
constexpr size_t iterator_named_value_lifetime = iterator_copy + iterator_cleanup;
// Const begin/end: this, result iterator, iterator(this,const pointer-ref),
// actual T* -> const T* conversion temporary. Mutable endpoints use stored
// pointer lvalues and have no conversion temporary.
constexpr size_t const_endpoint = 2 * P + 2 * P + P;
constexpr size_t mutable_endpoint = 2 * P + 2 * P;
constexpr size_t vector_query = P + N + D;
constexpr size_t allocator_access = 2 * P;
// cpp_type_traits::__miter_base: by-value iterator and returned iterator,
// true named-lvalue copy + parameter destructor + non-NRVO return move.
constexpr size_t miter_normal = 2 * P + iterator_named_value_lifetime + iterator_move;
// stl_iterator::__niter_base(normal_iterator): formal/returned pointer,
// actual base(this,ref), named-argument copy and parameter cleanup.
constexpr size_t niter_normal = 2 * P + 2 * P + iterator_named_value_lifetime;
constexpr size_t niter_pointer = 2 * P;
constexpr size_t niter_wrap_pointer = 3 * P;
// Iterator destination wrap has from/result/returned iterator, named from
// copy/destruction, its actual niter_base, member +(this,difference,result),
// pointer/count iterator constructor and genuine pointer prvalue temporary.
constexpr size_t niter_wrap_iterator =
	3 * P + iterator_named_value_lifetime + niter_normal + 2 * P + D + 2 * P + P;
// Trivial memmove specialization owns three pointers + returned pointer,
// actual _Num difference local. expect has genuine long argument/return ABI;
// memmove has destination/source/size/result interface. Their out-of-line
// runtime/emitted bodies remain ROOT qualification, not a guessed SRC margin.
constexpr size_t expect_interface = 3 * sizeof(long);
constexpr size_t memmove_interface = 3 * P + N;
template <class T> constexpr size_t scalar_assignment_source() noexcept
{
	if constexpr (std::is_same_v<T, critical_expected_revision>)
		return 3 * P + 3 * P; // generated revision + key this/source/result refs
	if constexpr (std::is_same_v<T, critical_entity_key>)
		return 3 * P;
	return 0; // byte assignment is builtin, no generated DTO call
}
template <class T> constexpr size_t trivial_copy_leaf() noexcept
{
	return 4 * P + D + expect_interface +
	       max_source(memmove_interface, 2 * P + scalar_assignment_source<T>());
}
// std::copy -> two real miter normal calls -> copy_move_a -> niter wrap;
// a1/a2 each three pointers + returned pointer; a2 invokes constant-evaluation
// query (ordinary runtime false). No iterator_category tag is materialized by
// that type-only dispatch. Normal parameters are actual one-pointer values.
template <class T> constexpr size_t copy_to_pointer() noexcept
{
	constexpr size_t copy_entry = 4 * P + 2 * iterator_named_value_lifetime;
	constexpr size_t copy_a = 4 * P + 2 * iterator_cleanup;
	return copy_entry + 2 * miter_normal + copy_a + 2 * niter_normal + niter_pointer +
	       niter_wrap_pointer + 2 * (4 * P) + B + trivial_copy_leaf<T>();
}
template <class T> constexpr size_t copy_to_iterator_from_endpoints() noexcept
{
	// Empty original vector assignment passes three endpoint prvalues: no
	// generated entry copies, but each actual by-value parameter is destroyed.
	constexpr size_t copy_entry = 4 * P + 3 * iterator_cleanup;
	// copy_move_a takes two miter prvalues plus named destination iterator.
	constexpr size_t copy_a = 4 * P + 2 * iterator_cleanup + iterator_named_value_lifetime;
	return copy_entry + 2 * miter_normal + copy_a + 3 * niter_normal + niter_wrap_iterator +
	       2 * (4 * P) + B + trivial_copy_leaf<T>();
}
// Genuine allocation interfaces of vector/base, std allocator_traits,
// C++20 allocator + constant query, new_allocator + max_size/expect and new.
// Alignment of the three actual element types selects ordinary unaligned new.
constexpr size_t allocate_source = (2 * P + N) + (2 * P + N) + (2 * P + N + B) + (3 * P + N) +
				   (P + N) + expect_interface + (P + N);
constexpr size_t deallocate_source =
	(2 * P + N) + (2 * P + N) + (2 * P + N + B) + (2 * P + N) + (P + N);
// allocator-specialized _Destroy -> two-arg _Destroy -> trivial destroy_aux,
// plus actual vector allocator getter. No element destructor executes.
constexpr size_t destroy_pointer_source = 3 * P + 2 * P + B + 2 * P + allocator_access;
// First allocator _Destroy receives two prvalues; both are cleaned. Its child
// two-arg _Destroy and true aux receive named lvalues and really copy/clean two
// iterators each. Object bytes of the vector remain outer-owned, not SOURCE.
constexpr size_t destroy_iterator_source =
	3 * P + 2 * iterator_cleanup + (2 * P + 2 * iterator_named_value_lifetime + B) +
	(2 * P + 2 * iterator_named_value_lifetime) + allocator_access;
template <class T> constexpr size_t allocation_copy_branch() noexcept
{
	// _M_allocate_and_copy: this/n/first/last/result local/returned pointer;
	// the two true endpoint prvalues are parameters, with cleanup only.
	constexpr size_t allocate_copy_entry = 5 * P + N + 2 * iterator_cleanup;
	// allocator specialization -> public uninitialized_copy -> true helper;
	// each receives named first/last and therefore copies/cleans both. Public
	// uninitialized_copy also owns real __can_memmove and __assignable bools.
	constexpr size_t uninitialized = (5 * P + B + 2 * iterator_named_value_lifetime) +
					 (4 * P + 2 * B + 2 * iterator_named_value_lifetime) +
					 (4 * P + 2 * iterator_named_value_lifetime);
	return 2 * const_endpoint + allocate_copy_entry + allocate_source + allocator_access +
	       uninitialized + copy_to_pointer<T>() + destroy_pointer_source + deallocate_source;
}
template <class T> constexpr size_t empty_copy_branch() noexcept
{
	return 2 * const_endpoint + 2 * mutable_endpoint + copy_to_iterator_from_endpoints<T>() +
	       destroy_iterator_source;
}
template <class T> constexpr size_t fresh_vector_const_assignment() noexcept
{
	// operator=(const vector&): this/source/result ref, __xlen and __tmp.
	// addressof(arg,result), false _S_propagate_on_copy_assign bool, source
	// size and destination capacity, then actual nonempty/empty alternatives.
	return 4 * P + N + 2 * P + B + 2 * vector_query +
	       max_source(allocation_copy_branch<T>(), vector_query + empty_copy_branch<T>());
}
// Same typed selected vector move/default allocator/temp/swap/cleanup graph as
// independently reviewed stock14, now instantiated only for these actual
// trivial command member types. No normalize envelope is used as credit.
template <class T> constexpr size_t vector_cleanup_source() noexcept
{
	constexpr size_t destruction = 10 * P + B;
	constexpr size_t deallocation = 11 * P + 5 * N + B;
	return destruction + deallocation + 3 * P;
}
template <class T> constexpr size_t vector_move_source() noexcept
{
	constexpr size_t entry = 3 * P + 2 * B + 2 * P + 2 * P + sizeof(std::true_type) + 2 * P;
	constexpr size_t get_allocator = 7 * P + sizeof(std::allocator<T>);
	constexpr size_t const_allocator_constructor = 11 * P;
	constexpr size_t data_swap = 2 * P + 3 * sizeof(T *) + P + 3 * (2 * P) + P;
	constexpr size_t allocator_move = 10 * P + 2 * (2 * P);
	return entry + sizeof(std::vector<T>) + get_allocator + 2 * P +
	       const_allocator_constructor + 2 * data_swap + allocator_move +
	       vector_cleanup_source<T>();
}
// Generated command assignment, id assignment and std::array assignment each
// owns this/source/returned ref. Scalar fields invoke no member function.
constexpr size_t command_generated_assignment = 3 * (3 * P);
constexpr size_t command_copy = command_generated_assignment +
				fresh_vector_const_assignment<critical_entity_key>() +
				fresh_vector_const_assignment<critical_expected_revision>() +
				2 * fresh_vector_const_assignment<uint8_t>() + 7 * (2 * P);
constexpr size_t command_move = command_generated_assignment +
				vector_move_source<critical_entity_key>() +
				vector_move_source<critical_expected_revision>() +
				2 * vector_move_source<uint8_t>() + 2 * P + 7 * (2 * P);
// state->native->attachment destination has two real unique arrow/get chains.
constexpr size_t attachment_copy = fresh_vector_const_assignment<uint8_t>() + 2 * (7 * (2 * P));
#if defined(__STDCPP_DEFAULT_NEW_ALIGNMENT__)
constexpr bool layout =
	sizeof(long) == 8 && sizeof(bool) == 1 && sizeof(std::allocator<uint8_t>) == 1 &&
	sizeof(std::allocator<critical_entity_key>) == 1 &&
	sizeof(std::allocator<critical_expected_revision>) == 1 &&
	std::is_trivial_v<critical_entity_key> && std::is_trivial_v<critical_expected_revision> &&
	std::is_trivial_v<uint8_t> && sizeof(std::vector<uint8_t>::iterator) == P &&
	sizeof(std::vector<uint8_t>::const_iterator) == P &&
	sizeof(std::vector<critical_entity_key>::iterator) == P &&
	sizeof(std::vector<critical_entity_key>::const_iterator) == P &&
	sizeof(std::vector<critical_expected_revision>::iterator) == P &&
	sizeof(std::vector<critical_expected_revision>::const_iterator) == P &&
	alignof(critical_entity_key) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__ &&
	alignof(critical_expected_revision) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__;
#else
constexpr bool layout = false;
#endif
} // namespace mixed_replay_command_assignment_source
} // namespace

namespace
{
namespace mixed_replay_fence_queue_construction_source
{
using namespace mixed_replay_command_assignment_source;
// Actual inherited/default constructors: native queue, deque, base, impl,
// allocator, new_allocator, impl_data and two real deque iterator members.
constexpr size_t defaults = 9 * P;
// _M_get_map_allocator: this and returned allocator object; true const
// tp-allocator getter(this,ref), rebind allocator converting ctor(this,ref)
// and its default new_allocator base receiver. Local/return value objects
// are summed across genuine selected source scopes, no retained heap here.
constexpr size_t map_allocator_value =
	P + sizeof(std::allocator<std::string *>) + 2 * P + 2 * P + P;
constexpr size_t allocator_cleanup = 2 * P;
// Same original std allocator_traits/std allocator/new_allocator graph, with
// the vector-specific _M_allocate/_M_deallocate wrapper explicitly excluded.
constexpr size_t allocator_allocate = allocate_source - (2 * P + N);
constexpr size_t allocator_deallocate = deallocate_source - (2 * P + N);
constexpr size_t allocate_map = 2 * P + N + sizeof(std::allocator<std::string *>) +
				map_allocator_value + allocator_allocate + allocator_cleanup;
constexpr size_t deallocate_map = 2 * P + N + sizeof(std::allocator<std::string *>) +
				  map_allocator_value + allocator_deallocate + allocator_cleanup;
// _M_allocate_node this/result, true buf_size input/return, then allocator.
constexpr size_t allocate_node = 2 * P + 2 * N + allocator_allocate;
// _M_create_nodes has this/nstart/nfinish/cur. A default deque's exact one
// node means an allocation failure destroys an EMPTY [nstart,cur) range.
// _M_destroy_nodes still executes its this/nstart/nfinish/n loop controller;
// no node deallocator is invoked for that genuinely empty failure range.
constexpr size_t create_nodes = 4 * P + allocate_node;
constexpr size_t failed_node_destroy = 4 * P;
// _M_set_node(this,new_node) -> _S_buffer_size return -> __deque_buf_size
// input/return, plus actual difference_type cast. Called on start and finish.
constexpr size_t set_node = 2 * P + 3 * N + D;
// Failed base construction destroys only completed impl subobjects: impl,
// data, two iterator members, allocator and new_allocator, six receivers.
constexpr size_t failed_impl_cleanup = 6 * P;
// _M_initialize_map owns this, num_elements, num_nodes, nstart, nfinish.
// First buffer calculation, genuine max(arg refs/result + bool) and its two
// materialized size_t arguments; map + actual one-node creation. Success adds
// two set_node and final modulo buffer query. Failure follows original empty
// node cleanup, map release and implicit completed-impl cleanup.
constexpr size_t initialize =
	3 * P + 2 * N + 2 * N + (3 * P + B + 2 * N) + allocate_map + create_nodes +
	max_source(2 * set_node + 2 * N,
		   failed_node_destroy + deallocate_map + failed_impl_cleanup);
constexpr size_t window = defaults + initialize;
constexpr bool layout = sizeof(std::string) == 32 && sizeof(std::allocator<std::string>) == 1 &&
			sizeof(std::allocator<std::string *>) == 1 &&
			sizeof(std::deque<std::string>::iterator) == 4 * P &&
			sizeof(std::deque<std::string>::const_iterator) == 4 * P;
} // namespace mixed_replay_fence_queue_construction_source
} // namespace

namespace
{
namespace mixed_replay_table_forecast_source
{
using namespace mixed_replay_command_assignment_source;
constexpr size_t policy_bytes = sizeof(std::__detail::_Prime_rehash_policy);
constexpr size_t pair_bytes = sizeof(std::pair<bool, size_t>);
// Actual native table helper has this/key/output, fresh/text/extra, result,
// genuine copied policy and original returned pair. No mapped heap here.
constexpr size_t own = 3 * P + 3 * N + B + policy_bytes + pair_bytes;
constexpr size_t count = P + N;
// Hashtable -> node allocator -> EBO const getter and C++20 specialized
// allocator_traits::max_size (no C++17 allocator.max_size branch).
constexpr size_t max_size = P + N + 2 * P + 2 * P + P + N;
constexpr size_t key_size = P + N;
constexpr size_t policy_value = 2 * P + 2 * P + P; // policy getter/copy/dtor.
// Actual reference-returning std::max, including its real two prvalue arguments.
constexpr size_t scalar_max = 3 * P + B + 2 * N;
constexpr size_t floor_interface = 2 * sizeof(double);
// GCC13.3 authentic release _M_need_rehash: this + bucket/element/insertion
// parameters and its actual min_bkts double. Returned pair lives in own above.
// Genuine forwarding pair ctor owns this/first/second refs, both std::forward
// calls, and materialized bool/size_t arguments. Short false/int branches fit
// this typed maximum; no second physical returned pair is invented.
constexpr size_t pair_construct = 3 * P + 4 * P + B + N;
constexpr size_t need_rehash =
	P + 3 * N + sizeof(double) + 2 * scalar_max + floor_interface + pair_construct;
// Real raw-pointer lower_bound: named first/last/value/result; helper's
// first/last/value/middle/result, len/half and actual one-byte comparator.
// Public factory result is elided into the genuine parameter object.
constexpr size_t raw_distance =
	2 * P + D + P + 2 * P + D + sizeof(std::random_access_iterator_tag) + 8 * P;
// advance(ref,n) keeps its actual __d; category + raw RA __advance. One real
// random tag has four-level ctor/dtor receivers; builtin_constant_p result int.
constexpr size_t raw_advance =
	P + 2 * D + P + P + D + sizeof(std::random_access_iterator_tag) + 8 * P + sizeof(int);
constexpr size_t lower_bound = 4 * P + 5 * P + 2 * D + sizeof(__gnu_cxx::__ops::_Iter_less_val) +
			       raw_distance + raw_advance + (3 * P + B) + P + P;
// Actual _M_next_bkt: this/input/result, genuine n_primes constexpr scalar,
// last_prime and next_bkt pointers. Immutable release prime/fast arrays are
// library static data, not an additional live caller frame or copied heap.
constexpr size_t next_bucket = 3 * P + 3 * N + lower_bound + floor_interface;
// Original helper invokes size once in the limit check and once in rehash args;
// key.size three times on the allocating branch, bucket once. Sum genuine sequential scopes, not iteration.
constexpr size_t window = own + 2 * count + max_size + 3 * key_size + count + policy_value +
			  need_rehash + next_bucket + P; // returned pair destructor.
constexpr bool layout =
	policy_bytes == 16 && pair_bytes == 16 && sizeof(__gnu_cxx::__ops::_Iter_less_val) == 1 &&
	sizeof(std::random_access_iterator_tag) == 1 && sizeof(double) == 8 && sizeof(int) == 4;
} // namespace mixed_replay_table_forecast_source
} // namespace

namespace
{
namespace mixed_replay_string_value_source
{
using namespace mixed_replay_command_assignment_source;
using namespace mixed_replay_fence_queue_construction_source;
// Actual selected char-allocator value copy/destruction closures only.
// Table/deque controllers and complete host/native qualification remain separate.
constexpr size_t A = sizeof(std::allocator<char>), C = sizeof(char);
constexpr size_t allocator_get = 2 * P;
constexpr size_t local_data = 8 * P; // local_data/pointer_to/addressof/__addressof.
constexpr size_t is_local = P + B + max_source(2 * P, P + local_data);
constexpr size_t allocator_copy = 4 * P; // allocator/new_allocator(this,source).
// One genuine selected allocator temporary, elided through return wrappers.
constexpr size_t select_allocator = 2 * P + A + allocator_copy;
constexpr size_t hider = 3 * P + 2 * P + allocator_copy;
constexpr size_t hider_cleanup = 3 * P;
constexpr size_t max_size = P + N + allocator_get + P + N;
constexpr size_t allocate = 3 * P + N + allocator_allocate;
// Fresh-copy _M_create always receives old capacity0: genuine growth condition
// cannot enter its doubling branch. The original valid-domain max_size runs once.
constexpr size_t create = 3 * P + N + max_size + allocator_get + allocate;
constexpr size_t init_local = P + B;
constexpr size_t raw_distance =
	2 * P + D + P + 2 * P + D + sizeof(std::random_access_iterator_tag) + 8 * P;
constexpr size_t traits_copy = 3 * P + N + B + 3 * P + N;
constexpr size_t traits_assign = 2 * P + B;
constexpr size_t copy_chars =
	3 * P + D + 2 * P + N + max_source(traits_assign, traits_copy) + 2 * P;
constexpr size_t set_length = P + N + P + N + 2 * P + traits_assign + C;
// Actual forward tag has forward/input two-level ctor+destructor, not random
// access's four-level graph. Real one-pointer guard local and ctor/dtor remain.
constexpr size_t construct = 3 * P + N + sizeof(std::forward_iterator_tag) + P + 3 * P + 4 * P +
			     raw_distance + max_source(init_local, create + 2 * P + P + N) +
			     copy_chars + set_length;
// Both source _M_data invocations and length are genuine selected entry calls.
// A failing _M_create destroys completed hider subobjects; guard is constructed
// only afterward and its successful cleanup sees _M_guarded==nullptr.
constexpr size_t copy = 2 * P + local_data + allocator_get + select_allocator + hider + 2 * P +
			4 * P + P + N + construct + hider_cleanup;
constexpr size_t destruction =
	P + P + is_local + P + N + allocator_get + 2 * P + allocator_deallocate + hider_cleanup;
// Completed string destruction is phase-exclusive with its completed copy;
// copy failure already owns hider cleanup above. No populated value dies while
// the same value is still executing that constructor. Later container cleanup
// retains this same named typed lifetime window, not another object/buffer.
constexpr size_t window = max_source(copy, destruction);
constexpr bool layout = sizeof(std::allocator<char>) == 1 && sizeof(char) == 1 &&
			sizeof(std::forward_iterator_tag) == 1 &&
			std::is_same_v<std::char_traits<char>::char_type, char>;
} // namespace mixed_replay_string_value_source
} // namespace

namespace
{
namespace mixed_replay_populated_state_cleanup_source
{
using namespace mixed_replay_command_assignment_source;
// Genuine existing typed vector controller instantiations. Command's three
// actual types share the proven controller shape; compare their exact typed
// expressions rather than referring to a nonexistent untyped alias.
constexpr size_t byte_vector_cleanup = vector_cleanup_source<uint8_t>();
constexpr size_t command_vector_cleanup = max_source(
	vector_cleanup_source<critical_entity_key>(),
	max_source(vector_cleanup_source<critical_expected_revision>(), byte_vector_cleanup));
// For authority operations, the true/false Destroy auxiliary has the same
// first/last receiver shape. Its actual nontrivial descent is added explicitly
// in authority_operations_cleanup below; no element lifetime is hidden here.
constexpr size_t authority_vector_cleanup_controller =
	vector_cleanup_source<flatfile_authority_operation>();
constexpr size_t tuple_member = mixed_replay_state_construction_source::tuple_member;
// Exact unique_ptr destructor -> ptr-reference access, get_deleter wrapper and
// real mutable tuple access, std::move(ref/result), default_delete(this,ptr),
// sized delete interface, then data/impl/tuple/two impl/two heads/deleter tail.
// Physical pointee and returned pointer are already actual retained owners.
constexpr size_t unique_destruction =
	2 * P + tuple_member + 2 * P + tuple_member + 2 * P + 2 * P + P + N + 8 * P;
constexpr size_t unique_reset = 2 * P + 3 * P + 2 * tuple_member + tuple_member + 2 * P + P + N;
constexpr size_t id_cleanup = 2 * P; // id -> its actual array member.
constexpr size_t completion_cleanup = P + max_source(id_cleanup, P);
// All generated member cleanup calls are sequential under the same parent
// receiver. Reuse one true maximum: fields are not dynamic callbacks or locks.
constexpr size_t command_cleanup = P + max_source(id_cleanup, command_vector_cleanup);
constexpr size_t native_context_cleanup = P + byte_vector_cleanup;
constexpr size_t envelope_cleanup = P + max_source(command_cleanup, byte_vector_cleanup);
constexpr size_t record_cleanup = P + max_source(command_cleanup, byte_vector_cleanup);
// Actual vector<authority_operation> false-destroy branch reaches addressof,
// _Destroy(T*), destroy_at(T*) before the actual operation destructor. Unlike
// a byte/key/revision vector its destructor really invokes each element body.
constexpr size_t authority_operation_cleanup =
	P + max_source(mixed_replay_string_value_source::destruction, byte_vector_cleanup);
constexpr size_t authority_operations_cleanup =
	authority_vector_cleanup_controller + 4 * P + authority_operation_cleanup;
// Exact fixed ordinary result -> shared participant -> its coin array, or
// a digest array. Only source receivers here, never another fixed DTO/heap.
constexpr size_t cash_result_cleanup = P + max_source(2 * P, P);
// All three existing private implementations have root/original/record/
// operations. Ordinary additionally has its fixed cash result. Identity and
// authority members are BORROWED pointers and have no lock destructor here.
constexpr size_t implementation_cleanup =
	P + max_source(max_source(mixed_replay_string_value_source::destruction, envelope_cleanup),
		       max_source(record_cleanup,
				  max_source(authority_operations_cleanup, cash_result_cleanup)));
constexpr size_t transaction_cleanup = P + unique_destruction + implementation_cleanup;
constexpr size_t state_cleanup =
	P +
	max_source(command_cleanup, max_source(unique_destruction + native_context_cleanup,
					       max_source(unique_destruction + transaction_cleanup,
							  completion_cleanup)));
constexpr size_t private_reset = unique_reset + state_cleanup;
// Actual operations node pair destroys key and state-pointer sequentially.
// Table node/allocator erasure controllers remain separate Source closures.
constexpr size_t operation_pair_cleanup =
	P + max_source(mixed_replay_string_value_source::destruction,
		       unique_destruction + state_cleanup);
constexpr size_t cleanup_window = max_source(private_reset, operation_pair_cleanup);
// Source value copies and completed rollback cleanup are distinct sequential
// mutation phases; reuse their real maximum rather than add string cleanup
// again on top of the same actual descendant included above.
constexpr size_t value_lifetime_window =
	max_source(mixed_replay_string_value_source::window, cleanup_window);
} // namespace mixed_replay_populated_state_cleanup_source
} // namespace

namespace
{
namespace mixed_replay_queue_push_source
{
using namespace mixed_replay_command_assignment_source;
namespace base = mixed_replay_fence_queue_construction_source;
// Actual size(this,returned size) -> deque difference(two refs,returned diff,
// actual bool conversion) -> _S_buffer_size(return) -> buf-size(input,return).
constexpr size_t size_query = P + N + 2 * P + D + B + 3 * N;
// max_size(this,result) -> allocator getter -> _S_max_size(allocator-ref,
// diffmax,allocmax,result) -> real C++20 traits max(ref,result) -> min(refs,
// result-ref,bool). Numeric traits value is static, not a heap request.
constexpr size_t max_size_query = P + N + 2 * P + P + 3 * N + P + N + 3 * P + B;
constexpr size_t scalar_max = 3 * P + B;
// Real map copy/copy_backward uses raw string** pointers. Both selected typed
// graphs have public/copy_a/a1/a2 four pointer carriers, two raw miter and
// three raw niter input/results, raw niter_wrap(from,res,result), then genuine
// trivial leaf(_Num,expect,memmove or one builtin pointer assignment). No
// deque iterator or string element copy executes when copying this map.
constexpr size_t raw_map_copy = 4 * P + 2 * (2 * P) + 4 * P + 3 * (2 * P) + 3 * P + 2 * (4 * P) +
				B + 4 * P + D + expect_interface +
				max_source(memmove_interface, 2 * P);
// Original map growth owns this,nodes,front,old/new counts,new_nstart,
// new_map_size,new_map. Only one original relocation branch executes.
constexpr size_t map_reallocate =
	3 * P + 4 * N + B +
	max_source(raw_map_copy,
		   scalar_max + base::allocate_map + raw_map_copy + base::deallocate_map) +
	2 * base::set_node;
constexpr size_t map_reserve = P + N + map_reallocate;
constexpr size_t node_deallocate = 2 * P + 2 * N + base::allocator_deallocate;
// Actual inherited std::allocator_traits::construct(allocator,location,arg)
// -> forward(ref,result) -> construct_at(location,arg) -> forward(ref,result)
// -> placement new(size,location,result). String ctor SOURCE is credited ONCE
// by the retained value-lifetime window; its string object/buffer are heap G/P.
constexpr size_t element_construct_controller = 3 * P + 2 * P + 2 * P + 2 * P + N + 2 * P;
constexpr size_t push_slow_controller = 2 * P + size_query + max_size_query + map_reserve +
					base::allocate_node + 2 * P + element_construct_controller +
					max_source(base::set_node, node_deallocate);
constexpr size_t push_controller =
	2 * P + max_source(element_construct_controller, push_slow_controller);
// Actual custom forecast: this,identity,out,actual-ref, text/block/peak/index/
// nodes/new_map/request/old_map. Max's second argument is an actual N prvalue.
constexpr size_t forecast_controller =
	4 * P + 8 * N + B + size_query + max_size_query + 3 * (P + N) + 2 * N + scalar_max + N;
// Forecast finishes before push; one phase-exclusive maximum of their exact
// graphs is retained before the first original callback. Real fresh map/block/
// string requests still come only from the unchanged actual peak forecast.
constexpr size_t window = max_source(forecast_controller, push_controller);
} // namespace mixed_replay_queue_push_source
} // namespace

namespace
{
namespace mixed_replay_key_value_source
{
using namespace mixed_replay_command_assignment_source;
namespace text = mixed_replay_string_value_source;
// pointer/count string's CONST allocator hider differs from const string copy:
// hider(this,data,allocator-ref), allocator/new_allocator const-copy4P; no
// invented std::move in this const hider. Default allocator temp ctor/dtor2P
// each and its genuine one-byte object are not another string object/heap.
constexpr size_t pointer_count_construct =
	3 * P + N + 2 * P + 2 * P + sizeof(std::allocator<char>) + 3 * P + text::allocator_copy +
	text::local_data + text::construct + text::hider_cleanup;
constexpr size_t array_data = 2 * P, array_size = P + N, array_index = 2 * P + N;
constexpr size_t operation_key = P + array_data + array_size + pointer_count_construct;
// Original aggregate {} initialization is builtin, not a fabricated array
// default ctor. Non-NRVO return permits the real generated array move pair;
// the array payload is already in the original two-array admission.
constexpr size_t entity_bytes = P + sizeof(unsigned int) + array_index + 2 * P + P;
constexpr size_t entity_key =
	P + entity_bytes + array_data + array_size + P + pointer_count_construct;
// Actual true/equal std::allocator<char> move: __alloc_on_move(two refs),
// std::move(ref,result), generated allocator/base assignments(three refs each).
constexpr size_t allocator_move = 10 * P;
constexpr size_t use_local = 2 * P + P + B + text::local_data;
constexpr size_t short_move = expect_interface + 2 * P + (P + N) + (2 * P + N) + 4 * P + (P + N) +
			      max_source(text::traits_assign, text::traits_copy) +
			      text::set_length + (P + N);
// Complete generic equal-allocator long branch: actual optional reused data,
// receiver/source getters, setters and either source reuse or local-data reset.
constexpr size_t long_move = B + text::is_local + 2 * P + 2 * P + 2 * P + (P + N) + (P + N) +
			     (P + N) + max_source(2 * P + P + N, use_local + 2 * P);
constexpr size_t move_assignment = 3 * P + B + P + N + B + text::is_local + B +
				   2 * text::allocator_get + allocator_move + text::is_local +
				   max_source(short_move, long_move) + P + text::set_length;
constexpr size_t window = max_source(operation_key, max_source(entity_key, move_assignment));
// Existing value window1476 genuinely dominates these named complete paths
// (not an unrelated heap/controller allowance). Preserve its exact admission.
constexpr size_t admitted_window =
	max_source(mixed_replay_populated_state_cleanup_source::value_lifetime_window, window);
constexpr bool layout =
	sizeof(unsigned int) == 4 && sizeof(std::array<char, 9>) == 9 &&
	std::allocator_traits<std::allocator<char>>::is_always_equal::value &&
	std::allocator_traits<std::allocator<char>>::propagate_on_container_move_assignment::value;
} // namespace mixed_replay_key_value_source
namespace mixed_replay_queue_cleanup_source
{
using namespace mixed_replay_command_assignment_source;
namespace base = mixed_replay_fence_queue_construction_source;
constexpr size_t I = sizeof(std::deque<std::string>::iterator);
constexpr size_t iterator_parameter_lifetime = 2 * P + P;
// String element body is credited by same retained value-lifetime window once.
// Real _Destroy(allocator/first/last), _Destroy(first/last,bool), false aux
// (first/last), addressof,_Destroy(T*),destroy_at(T*), allocator getter.
constexpr size_t element_destroy_controller = 3 * P + 2 * P + B + 2 * P + 4 * P + 2 * P;
// Actual aux(this,first,last,node) owns true by-value 4-pointer iterators,
// materialized copies/destructors and buffer-size source, then raw destroy.
constexpr size_t destroy_aux_controller = 2 * P + 2 * I + 3 * N + element_destroy_controller;
constexpr size_t destroy_data_controller =
	2 * P + 2 * I + 2 * iterator_parameter_lifetime + destroy_aux_controller;
// Actual begin()/end() each owns this and returned four-pointer iterator.
// They construct that return from stored lvalues (copy2P), then the actual
// by-value Destroy parameters clean up(P); guaranteed prvalue transfer does
// not add another generated copy. Returned formal Source P+I was absent.
constexpr size_t endpoint_query = P + I;
constexpr size_t deque_body_controller =
	P + 2 * endpoint_query + 2 * iterator_parameter_lifetime + destroy_data_controller + 2 * P;
// Actual all-node loop(this,nstart,nfinish,n) -> complete node deallocate;
// after map release, completed impl/data/two iterators/allocator bases clean.
constexpr size_t destroy_nodes_controller = 4 * P + mixed_replay_queue_push_source::node_deallocate;
constexpr size_t base_cleanup_controller =
	P + destroy_nodes_controller + base::deallocate_map + base::failed_impl_cleanup;
constexpr size_t window = P + deque_body_controller + base_cleanup_controller;
// Populated deque destruction is sequential with its prior completed push.
// Reuse the actual max controllers, not double-count current map/block/text.
constexpr size_t mutation_window = max_source(mixed_replay_queue_push_source::window, window);
} // namespace mixed_replay_queue_cleanup_source
} // namespace

namespace
{
namespace mixed_replay_table_key_lookup_source
{
using namespace mixed_replay_command_assignment_source;
constexpr size_t C = sizeof(char), J = sizeof(int), V = sizeof(std::string_view);
// CONDITIONAL external definition: authentic GCC13.3 LP64 _Hash_bytes source,
// not proof of installed libstdc++14 ABI/emitted compatibility (ROOT OPEN).
constexpr size_t hash_unaligned = P + 2 * N + 3 * P + N;
constexpr size_t hash_shift = 2 * N;
constexpr size_t hash_tail = P + J + 2 * N;
constexpr size_t hash_bytes =
	4 * P + 6 * N + max_source(hash_unaligned + hash_shift, max_source(hash_tail, hash_shift));
constexpr size_t hash_impl = P + 3 * N + hash_bytes;
constexpr size_t string_hash = 2 * P + N + 4 * P + (P + N) + hash_impl;
// Actual string -> view operator: this,data->_M_data,length,ptr-count view.
// Its view object is the actual hash argument, not a second string/view heap.
constexpr size_t string_view_conversion = 8 * P + 2 * N;
constexpr size_t view_hash = P + V + N + (2 * P) + (P + N) + hash_impl;
constexpr size_t entity_hash = P + V + N + sizeof(std::hash<std::string_view>) + 2 * P + view_hash +
			       string_view_conversion; // selected declared view copy only.
constexpr size_t hash_get = 2 * P + 2 * P; // hash wrapper -> EBO cget.
constexpr size_t key_hash = 2 * P + N + hash_get + max_source(string_hash, entity_hash);
// _M_v -> _M_valptr -> actual aligned_buffer::_M_ptr -> _M_addr, four
// receiver/result/ref pairs, no aligned_membuf/launder substitute.
constexpr size_t node_value = 8 * P;
constexpr size_t select_first = sizeof(std::__detail::_Select1st) + 2 * P + 3 * P;
constexpr size_t range_hash = sizeof(std::__detail::_Mod_range_hashing) + P + 3 * N;
constexpr size_t code_bucket = P + 2 * N + P + 3 * N + range_hash;
constexpr size_t cached_node_bucket = 2 * P + N + 2 * P + 2 * N + range_hash;
constexpr size_t uncached_node_bucket =
	2 * P + N + 2 * P + 2 * N + select_first + node_value + key_hash + range_hash;
constexpr size_t node_bucket = max_source(cached_node_bucket, uncached_node_bucket);
// Actual string equality: equal_to(this,two refs,bool), string== refs/bool,
// three size queries, two data->_M_data queries, char_traits::compare and its
// actual char_traits body. Its __builtin_memcmp has no invented interface
// or source locals; the constant-evaluation loop is not selected.
constexpr size_t string_equal =
	3 * P + B + 2 * P + B + 3 * (P + N) + 2 * (4 * P) + (2 * P + N + J + B);
// True equal_to<void> adds two std::forward(ref,result) calls. No closure or
// function-pointer adapter is invented for this original typed comparator.
constexpr size_t equal = string_equal + 4 * P;
constexpr size_t eq_get = 2 * P + 2 * P;
constexpr size_t key_equal = 3 * P + B + eq_get + select_first + node_value + equal;
constexpr size_t hash_cached_equal = P + N + B;
constexpr size_t node_equal = 3 * P + N + B + hash_cached_equal + key_equal;
constexpr size_t node_next = 2 * P;
constexpr size_t find_before_bucket = 5 * P + 2 * N + node_equal + node_next + node_bucket;
constexpr size_t find_node = 4 * P + 2 * N + find_before_bucket;
constexpr size_t iterator_constructor = 4 * P; // iterator then base this/node.
// Node/base implicit trivial copy and destructor have no declared source body.
constexpr size_t iterator_copy = 0, iterator_destructor = 0;
constexpr size_t begin = 2 * P + 2 * P + iterator_constructor;
constexpr size_t end = 2 * P + iterator_constructor;
constexpr size_t iterator_compare = 2 * P + B;
constexpr size_t iterator_increment = 2 * P + P + node_next;
// Real slow-hash small-table scan or original bucketed lookup. End scopes are
// sequential repeated calls to the same genuine body; one named closure plus
// actual local/return iterator lifetime covers their controller storage.
constexpr size_t small_scan = begin + end + iterator_destructor + iterator_compare +
			      iterator_increment + key_equal + iterator_copy + iterator_destructor;
constexpr size_t bucketed_find = key_hash + code_bucket + find_node + iterator_constructor;
constexpr size_t find_window =
	4 * P + 2 * N + (P + N) + 2 * N + max_source(small_scan, bucketed_find);
// Actual Map_base::at(this,key,result-ref,local iterator) -> same find,
// iterator arrow(this,result), valptr/ptr/addr(three receiver/result pairs),
// completed iterator/base destruction. Original observer only selects valid
// retained identity; original out-of-range exception remains runtime OPEN.
constexpr size_t at_window = 4 * P + find_window + 8 * P + iterator_destructor;
constexpr size_t window = max_source(find_window, at_window);
// Only standard std::string-key find selected by both genuine enqueue bodies.
// Transparent string_view rollback lookup remains a separate missing graph.
constexpr bool layout = sizeof(int) == 4 && sizeof(std::string_view) == 2 * P &&
			sizeof(std::hash<std::string_view>) == 1 &&
			sizeof(std::__detail::_Select1st) == 1 &&
			sizeof(std::__detail::_Mod_range_hashing) == 1;
} // namespace mixed_replay_table_key_lookup_source
} // namespace

namespace
{
namespace mixed_replay_table_node_storage_source
{
using namespace mixed_replay_command_assignment_source;
// These are the two actual table node types, not a guessed generic node.
using operation_node =
	std::__detail::_Hash_node<std::pair<const std::string, std::unique_ptr<operation_state>>,
				  true>;
using fence_node =
	std::__detail::_Hash_node<std::pair<const std::string, native_identity_queue>, false>;
constexpr size_t A = sizeof(std::allocator<void *>);
// _M_node_allocator -> _Hashtable_ebo_helper::_M_get, this/ref at both levels.
constexpr size_t node_allocator_get = 4 * P;
// _M_valptr -> aligned_buffer::_M_ptr -> _M_addr, this/result at all three levels.
constexpr size_t value_pointer = 6 * P;
// Only selected DECLARED constructors: _Hash_node_base() policy313 and
// __aligned_buffer()=default aligned_buffer98. The aggregate node/value/cache,
// storage union and empty aligned_storage base have no separate declared source.
// No implicit trivial node/storage destructor body is invented.
constexpr size_t node_default = 2 * P;
constexpr size_t node_destructor = 0;
// Genuine raw __to_address(ptr,result), pointer_to(ref,result) ->
// addressof(ref,result) -> __addressof(ref,result).
constexpr size_t raw_address = 2 * P;
constexpr size_t pointer_to = 6 * P;
constexpr size_t placement = N + 2 * P;
// Both genuine selected allocators are std::allocator, same typed allocate /
// deallocate controller already authenticated by the deque/vector providers.
// Their node alignment is checked below, so the aligned-new branch is not selected.
constexpr size_t allocator_allocate =
	mixed_replay_fence_queue_construction_source::allocator_allocate;
constexpr size_t allocator_deallocate =
	mixed_replay_fence_queue_construction_source::allocator_deallocate;
template <size_t R> constexpr size_t construct_controller()
{
	// allocator_traits::construct(allocator,location,args...), forwards;
	// construct_at(location,args...,returned pointer), forwards; placement new(size,ptr,result).
	// Actual pair construction is a SEPARATE child, not this controller.
	return (2 + R) * P + 2 * R * P + (2 + R) * P + 2 * R * P + placement;
}
template <size_t R> constexpr size_t allocate_node_controller()
{
	// this, R argument references, __nptr, __n and result; exact allocator get,
	// raw address and node default; value pointer and forwards to value construct.
	// Failed value construction releases the real allocated node before rethrow.
	return (4 + R) * P + node_allocator_get + allocator_allocate + raw_address + placement +
	       node_default + value_pointer + 2 * R * P + construct_controller<R>() +
	       node_allocator_get + allocator_deallocate;
}
template <size_t R> constexpr size_t scoped_allocate_controller()
{
	// Scoped-node this, hashtable pointer and R refs; forwards to actual allocation.
	// Its TWO retained pointer fields are priced separately as actual local INLINE.
	return (2 + R) * P + 2 * R * P + allocate_node_controller<R>();
}
// ~Scoped_node(this) -> deallocate_node(this,node), allocator getter / value ptr,
// traits::destroy(allocator,location) -> destroy_at(location). Actual pair/member
// destruction is ALREADY retained by the value/queue cleanup families, not repeated.
// deallocate_node_ptr(this,node,__ptr) -> pointer_to, real generated node teardown,
// allocator getter and real typed deallocation. No copy of node/value bytes here.
constexpr size_t scoped_node_cleanup = P + 2 * P + node_allocator_get + value_pointer + 2 * P + P +
				       3 * P + pointer_to + node_destructor + node_allocator_get +
				       allocator_deallocate;
// operations.emplace has two actual value refs; fences operator[] uses the actual
// piecewise tag and two tuples (three refs). This bound maps BOTH own controller
// instantiations. The pair / tuple / unique move child is explicitly not closed here.
constexpr size_t scoped_node_fields = 2 * P;
constexpr size_t node_window =
	scoped_node_fields +
	max_source(scoped_allocate_controller<2>(),
		   max_source(scoped_allocate_controller<3>(), scoped_node_cleanup));
// Complete _Hashtable_alloc bucket allocate/deallocate primitive: local rebind
// allocator plus its converting ctor/new_allocator base and cleanup. No vector
// or deque wrappers are substituted for these actual controller locals.
constexpr size_t rebind_allocator_construct = 3 * P;
constexpr size_t allocator_cleanup = 2 * P;
constexpr size_t bucket_allocate =
	4 * P + N + A + node_allocator_get + rebind_allocator_construct + allocator_allocate +
	raw_address +
	allocator_cleanup; // __builtin_memset introduces no source interface/declared locals.
constexpr size_t bucket_deallocate = 3 * P + N + A + pointer_to + node_allocator_get +
				     rebind_allocator_construct + allocator_deallocate +
				     allocator_cleanup;
constexpr size_t window = max_source(node_window, max_source(bucket_allocate, bucket_deallocate));
#if defined(_GLIBCXX_INLINE_VERSION) && _GLIBCXX_INLINE_VERSION == 0 && \
	defined(__STDCPP_DEFAULT_NEW_ALIGNMENT__)
constexpr bool layout = A == 1 && alignof(operation_node) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__ &&
			alignof(fence_node) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__;
#else
constexpr bool layout = false;
#endif
} // namespace mixed_replay_table_node_storage_source
} // namespace

namespace
{
namespace mixed_replay_table_value_construction_source
{
using namespace mixed_replay_command_assignment_source;
constexpr size_t T = sizeof(std::tuple<std::string &&>);
constexpr size_t E = sizeof(std::tuple<>);
constexpr size_t Z = sizeof(std::piecewise_construct_t);
constexpr size_t K = sizeof(std::_Index_tuple<>);
// Genuine unique_ptr MOVE CONSTRUCTION, not null-destination move assignment.
// unique/data/impl(this,source); one std::move; tuple and both impl/head defaulted
// move receiver/source pairs; no undeclared default_delete move; source impl pointer get.
constexpr size_t unique_move = 3 * (2 * P) + 2 * P + 2 * P + 2 * (2 * P) + 2 * (2 * P) +
			       mixed_replay_state_construction_source::tuple_member;
// Operation pair(this,x,y), two forward(ref,result). Concepts-mode empty
// __pair_base has no declared constructor or destructor in this selected mode.
// Both original member constructors are sequential under this controller. Existing
// value lifetime1476 dominates string-copy1023 and unique-move224 separately; no
// second string/unique graph or their actual object bytes are added here.
constexpr size_t operation_pair_controller = 3 * P + 2 * (2 * P);
// Existing nontrivial member cleanup is retained by the reviewed value window.
// Empty concepts-mode pair base has no separate declared cleanup source.
constexpr size_t pair_base_cleanup = 0;
// Actual fence argument expressions: std::move(ref,result), forward_as_tuple
// (arg/result tuple), its forward, tuple forwarding ctor+forward, impl ctor+forward,
// head ctor+forward; selected declared tuple<> default(receiver and one-byte object).
// The prvalue tuple<string&&> materializes once for the outer Scoped_node argument.
constexpr size_t fence_arguments = 2 * P + (P + T + 2 * P) + 3 * (2 * P + 2 * P) + P + E;
// Piecewise wrapper owns this and actual by-value tag/first tuple/empty tuple.
// The one-reference tuple selects declared tuple/impl/head moves6P. Empty tuple
// has no declared copy/move, and none of these trivial tag/tuple types declares
// a destructor. Their object storage stays owned, with no invented source calls.
constexpr size_t piecewise_wrapper = P + Z + T + E + 6 * P;
// Delegating pair(tuple1&,tuple2&,index1,index2): this/two refs, two actual empty
// index objects only (no declared special members). get<0> -> __get_helper -> impl
// head -> head-base head (four ref/result pairs), one forward; no empty-base call.
constexpr size_t piecewise_delegate = 3 * P + 2 * K + 4 * (2 * P) + 2 * P;
// Exact string MOVE CONSTRUCTION from basic_string.h, not its move assignment:
// own receiver/source, local-data and source allocator getters, std::move/hider,
// source is_local; actual short init/copy/length or long pointer/capacity setters;
// common source length/destination length, source use-local/data/set-length.
constexpr size_t string_move =
	2 * P + mixed_replay_string_value_source::local_data + 2 * P +
	mixed_replay_string_value_source::allocator_get + mixed_replay_string_value_source::hider +
	mixed_replay_string_value_source::is_local +
	max_source(mixed_replay_string_value_source::init_local +
			   mixed_replay_string_value_source::traits_copy + P + N,
		   2 * P + 2 * P + P + N) +
	(P + N) + (P + N) + mixed_replay_key_value_source::use_local + 2 * P +
	mixed_replay_string_value_source::set_length;
// First member string move completes before second deque default. If second fails,
// the original completed string destructor runs. Both branches are dominated by
// already-retained value1476: max(string move558, deque856 + string destructor258).
// Actual deque856 still appears in the unchanged original absent-fence extra
// forecast/admission. Do not remove that existing credit/request or add it again.
constexpr size_t fence_member_child =
	max_source(string_move, mixed_replay_fence_queue_construction_source::window +
					mixed_replay_string_value_source::destruction);
constexpr size_t operation_member_child =
	max_source(mixed_replay_string_value_source::copy, unique_move);
constexpr size_t operation_window = operation_pair_controller + pair_base_cleanup;
constexpr size_t fence_window =
	fence_arguments + piecewise_wrapper + piecewise_delegate + pair_base_cleanup;
constexpr size_t window = max_source(operation_window, fence_window);
constexpr bool layout =
	T == P && E == 1 && Z == 1 && K == 1 && sizeof(std::_Index_tuple<0>) == K &&
	operation_member_child <=
		mixed_replay_populated_state_cleanup_source::value_lifetime_window &&
	fence_member_child <= mixed_replay_populated_state_cleanup_source::value_lifetime_window;
} // namespace mixed_replay_table_value_construction_source
} // namespace

namespace
{
namespace mixed_replay_table_insert_source
{
using namespace mixed_replay_command_assignment_source;
namespace lookup = mixed_replay_table_key_lookup_source;
constexpr size_t Q = sizeof(std::pair<bool, size_t>);
constexpr size_t R = sizeof(std::pair<decltype(operations)::iterator, bool>);
// The genuine data-free native_admission_string_table inherits __umap_hashtable
// DIRECTLY. There is no std::unordered_map wrapper call on either selected table.
// Actual _Hashtable.emplace has this/two refs, two forwards and real unique-tag
// object parameter only; no integral_constant aggregate ctor/dtor call.
// Its returned prvalue initializes ONE elided result below.
constexpr size_t table_emplace = 3 * P + 2 * (2 * P);
// _M_emplace: this/two arg refs/key ref/loop iterator/found node/pos iterator,
// code/bucket locals, true tag parameter and two forwards into Scoped_node.
// The scoped-node physical fields and node/value construction are parent-owned.
constexpr size_t emplace_body = 7 * P + 2 * N + B + 2 * (2 * P);
// Actual returned pair(iterator,bool): declared pair forwarding ctor and two
// forwards; max branch selects iterator(node), with pointer/bool materializations.
// Undeclared trivial iterator copy, pair/base/iterator cleanup has no source call.
// The final pair object has ONE elided lifetime through actual table dispatch.
constexpr size_t returned_pair = R + (3 * P + 2 * (2 * P)) + lookup::iterator_constructor + P + B;
// Shared actual lookup descendants for this body: initial key extraction/value,
// sequential size/threshold probes; real small scan OR code/hash/bucket/find.
// The already-retained genuine lookup1904 dominates the complete selected child
// set below. Do not add another full lookup envelope for the same descendants.
constexpr size_t emplace_lookup_child =
	2 * ((P + N) + 2 * N) +
	max_source(lookup::select_first + lookup::node_value,
		   max_source(lookup::small_scan,
			      lookup::key_hash + lookup::code_bucket + lookup::find_node));
// _M_insert_unique_node own this/node/saved-state reference, bkt/code/n_elt,
// actual do_rehash pair object. _M_state(this,returned _State) materializes the
// genuine saved-state value bound by const reference; no second saved-state DTO.
// _M_store_code has this/node/code formals in BOTH true and false cache lanes.
constexpr size_t insert_unique_body = 3 * P + 3 * N + Q + (P + N) + (2 * P + N);
// _M_rehash(this,count,state-ref), actual unique-tag parameter storage only;
// failure calls real _M_reset(this,state). No alternate growth policy/source.
// The unique tag prvalue initializes the ONE aux parameter, whose B is below.
constexpr size_t rehash_body = 2 * P + N + (P + N);
// _M_rehash_aux true: this/new_buckets/p/next, count/bbegin/bkt, actual tag;
// _M_begin(this,result), _M_next(this,result). Original node bucket hash graph
// is lookup-owned; bucket allocation/free primitives are node-storage-owned.
constexpr size_t rehash_aux_body = 4 * P + 3 * N + B + 2 * P + 2 * P;
// Actual hashtable wrappers, not duplicate _Hashtable_alloc primitive frames:
// allocate(this,count,result); __builtin_expect has no declared Source interface;
// zero-arg deallocate(this) -> deallocate(this,buckets,count) ->
// uses_single_bucket(this,buckets,bool). Single-bucket branch is preserved.
constexpr size_t allocate_bucket_wrapper = 2 * P + N;
constexpr size_t deallocate_bucket_wrapper = P + (2 * P + N) + (2 * P + B);
// _M_insert_bucket_begin(this,node,bucket); its actual next-node/bucket child
// is again lookup-owned. Returned iterator construction is genuine4P.
constexpr size_t insert_bucket_body = 2 * P + N;
constexpr size_t insert_unique_window =
	insert_unique_body +
	max_source(rehash_body + rehash_aux_body + allocate_bucket_wrapper +
			   deallocate_bucket_wrapper,
		   insert_bucket_body) +
	lookup::iterator_constructor;
// Actual inherited Map_base body this/key/h/found-node/pos/result-ref, code/bucket.
// The original direct hashtable owner has NO std::unordered_map operator[] wrapper.
// Its private
// Scoped_node/tuple fields, value ctor and selected lookup descendants are already
// separately owned; no duplicated map/node/tuple/deque/string object or heap.
constexpr size_t subscript_body = 6 * P + 2 * N;
constexpr size_t emplace_window =
	table_emplace + emplace_body + returned_pair + insert_unique_window;
constexpr size_t subscript_window = subscript_body + insert_unique_window;
// Original coordinator call expression std::move(work.state) or
// std::move(work.fence_key) is outside the STL wrapper's own std::move.
constexpr size_t caller_move = 2 * P;
constexpr size_t window = caller_move + max_source(emplace_window, subscript_window);
constexpr bool layout = Q == 2 * P && R == 2 * P && sizeof(std::true_type) == B &&
			emplace_lookup_child <= lookup::window &&
			mixed_replay_table_node_storage_source::layout &&
			mixed_replay_table_value_construction_source::layout;
} // namespace mixed_replay_table_insert_source
} // namespace

namespace
{
namespace mixed_replay_table_iterator_erase_source
{
using namespace mixed_replay_command_assignment_source;
namespace lookup = mixed_replay_table_key_lookup_source;
// Direct inherited Hashtable::erase(iterator), not an unordered_map wrapper:
// receiver; by-value iterator parameter; converted const-iterator parameter;
// actual ignored output iterator (separate from non-NRVO _M_erase local result).
// Const conversion selects declared const-node/base constructors4P. Trivial
// implicit parameter copy/destructors have no declared source; receiver and actual
// parameter/output iterator values remain retained4P.
constexpr size_t overload_lifetimes = 4 * P + 4 * P;
// erase(const_iterator) own this/node/previous-node and actual bucket local;
// its by-value parameter object is already owned once by the converter above.
constexpr size_t const_erase_body = 3 * P + N;
// _M_get_previous_node(this,bucket,node,previous,result) follows raw _M_nxt;
// there is no invented node-next method call in that actual loop.
constexpr size_t previous_node = 4 * P + N;
// _M_erase(this,prev,node,result-iterator,bucket,next-bucket); actual bucket-
// removal controller, result iterator(node-next), permitted non-NRVO generated
// iterator/base move and cleanup of that real named local. The output is already
// owned once above. Actual node allocator/value destructor is a separate child.
constexpr size_t remove_bucket_begin = 2 * P + 2 * N;
constexpr size_t erase_body = 4 * P + 2 * N + remove_bucket_begin + lookup::iterator_constructor +
			      lookup::iterator_copy + lookup::iterator_destructor;
constexpr size_t window = overload_lifetimes + const_erase_body + previous_node + erase_body;
// Erasure's actual cached/uncached bucket and at most two sequential next() calls
// are subsets of already retained lookup1904. Node storage802 and member value1476
// plus queue1533 own their genuine teardown separately, not counted again here.
constexpr size_t lookup_child = lookup::node_bucket + 2 * lookup::node_next;
constexpr bool layout = sizeof(decltype(operations)::iterator) == P &&
			sizeof(decltype(operations)::const_iterator) == P &&
			sizeof(decltype(fences)::iterator) == P &&
			sizeof(decltype(fences)::const_iterator) == P &&
			lookup_child <= lookup::window && mixed_replay_table_insert_source::layout;
} // namespace mixed_replay_table_iterator_erase_source
} // namespace

namespace
{
namespace mixed_replay_transparent_table_lookup_source
{
using namespace mixed_replay_command_assignment_source;
namespace old = mixed_replay_table_key_lookup_source;
constexpr size_t V = sizeof(std::string_view), J = sizeof(int);
// Captured string_view copy ctor is declared; its trivial destructor is not.
constexpr size_t view_copy = 2 * P, view_destructor = 0;
constexpr size_t traits_compare = 2 * P + N + J + B;
// string_view::_S_compare actual n1/n2, difference_type __diff, returned int.
constexpr size_t length_compare = 2 * N + D + J;
// compare(this, by-value view, rlen, __ret, returned int), min(two refs,result-ref),
// char_traits/memcmp and S_compare; real view parameter copy/cleanup.
constexpr size_t view_compare =
	P + V + N + 2 * J + 3 * P + traits_compare + length_compare + view_copy + view_destructor;
// equality's two actual by-value views/bool, two size queries, compare,
// left parameter copy and both parameter cleanup. RHS is original string's
// actual data/size/ptr-count conversion, not an invented alternative byte check.
constexpr size_t view_equal = 2 * V + B + 2 * (P + N) + view_compare + view_copy +
			      2 * view_destructor + old::string_view_conversion;
constexpr size_t equal = 3 * P + B + 2 * (2 * P) + view_equal;
constexpr size_t key_equal = 3 * P + B + old::eq_get + old::select_first + old::node_value + equal;
constexpr size_t node_equal = 3 * P + N + B + old::hash_cached_equal + key_equal;
constexpr size_t find_before = 5 * P + 2 * N + node_equal + old::node_next + old::node_bucket;
constexpr size_t find_node = 4 * P + 2 * N + find_before;
// Actual view-key hash replaces string->view conversion with genuine initial
// entity parameter copy+cleanup. All other named source scopes are identical.
// This authentic typed substitution remains below existing old key_hash457.
constexpr size_t entity_hash =
	old::entity_hash - old::string_view_conversion + view_copy + view_destructor;
constexpr size_t key_hash = 2 * P + N + old::hash_get + entity_hash;
// Original remove_fences forms actual view temporary from its encoded array;
// that array itself remains an explicitly separate rollback local owner.
constexpr size_t view_argument = V + (2 * P + N) + 2 * P + (P + N) + view_destructor;
constexpr size_t forwarding_owner = 2 * P; // native table find(this,key const-ref).
constexpr size_t find_body = 3 * P + 2 * N; // actual this/key/output iterator, code/bucket.
constexpr size_t full_window = view_argument + forwarding_owner + find_body + key_hash +
			       old::code_bucket + find_node + old::iterator_constructor;
// Existing lookup credit owns the same selected controller/forward and traits
// leaves. Recompute the exact predicate difference from the corrected old and
// transparent expressions together; actual new forwarding16 and view argument72
// remain uncovered. Original find also has an actual small-scan iterator local
// that is absent from the transparent body (3P+2N, not4P+2N).
// Retain ONLY this genuine missing supplement, not a second full lookup envelope.
constexpr size_t predicate_supplement = equal - old::equal;
constexpr size_t supplement = predicate_supplement + forwarding_owner + view_argument;
constexpr size_t window = supplement;
constexpr bool layout = V == 2 * P && J == 4 && equal >= old::equal && key_hash <= old::key_hash &&
			full_window <= old::window + supplement &&
			mixed_replay_table_iterator_erase_source::layout;
} // namespace mixed_replay_transparent_table_lookup_source
} // namespace

namespace
{
namespace mixed_replay_deque_tail_erase_source
{
using namespace mixed_replay_command_assignment_source;
constexpr size_t I = sizeof(native_identity_queue::iterator);
// Captured declared _Deque_iterator copy constructor owns this/source.
// The selected defaulted assignment is declared separately; no implicit dtor body.
constexpr size_t iterator_copy = 2 * P;
constexpr size_t buffer_query = 3 * N; // _S_buffer_size return; deque_buf_size input/return.
constexpr size_t iterator_pointer_construct = 3 * P + buffer_query;
constexpr size_t const_iterator_conversion = P + I + iterator_pointer_construct;
constexpr size_t const_cast_without_result = const_iterator_conversion - I;
constexpr size_t endpoint = P + I + iterator_copy;
constexpr size_t endpoint_without_result = endpoint - I;
constexpr size_t equal = 2 * P + B; // C++20 != rewrites selected ==, no second helper.
constexpr size_t difference = 2 * P + D + buffer_query;
// operator+= actual receiver/n/result reference/offset/node_offset; the real
// _M_set_node source is the existing captured receiver/node/buffer/cast chain.
constexpr size_t set_node = 2 * P + buffer_query;
constexpr size_t plus_assign = 2 * P + 3 * D + buffer_query + set_node;
// Both nonmember operators own input reference/n/local iterator/result object.
// Actual non-NRVO return selects declared copy; implicit iterator dtor is absent.
constexpr size_t arithmetic_own = P + D + 2 * I;
constexpr size_t plus = arithmetic_own + iterator_copy + plus_assign + iterator_copy;
constexpr size_t minus_assign = 2 * P + D;
constexpr size_t minus =
	arithmetic_own + iterator_copy + minus_assign + plus_assign + iterator_copy;
// _M_erase_at_end(this,by-value pos); endpoint and real allocator reference query.
// The _M_destroy_data / _M_destroy_nodes children remain separately covered by
// the already retained named queue destruction source, never charged twice.
// Finish=pos selects the DECLARED defaulted assignment (this,source,result-ref)
// at captured stl_deque172, so its genuine3P belongs here.
// end() output is the same elided second destroy_data parameter already child-owned.
// Named __pos -> first destroy_data parameter selects a real copy2P.
constexpr size_t erase_end_own = P + I + endpoint_without_result + 2 * P + 3 * P + iterator_copy;
// begin() output initializes the exact at_end parameter: only one I retained.
constexpr size_t clear_own = P + endpoint_without_result + erase_end_own;
// Genuine _M_erase first/last/result iterator values + this + n/elems_before.
constexpr size_t erase_body_own = P + 3 * I;
constexpr size_t common_equal = equal;
constexpr size_t full_branch = 2 * (endpoint + equal) + clear_own + endpoint_without_result;
// Partial first==begin is false (kept>0), so full conjunction short-circuits;
// no second endpoint/comparison at that cut. Both difference calls are named
// occurrences, a conservative explicit sequential overestimate of shared48.
// Arithmetic output aliases at_end parameter (minus) or final output (plus),
// already retained in erase_end_own/erase_body_own; subtract exactly that I.
constexpr size_t partial_own = endpoint + 2 * difference +
			       mixed_replay_queue_push_source::size_query + endpoint + equal +
			       endpoint + (minus - I) + erase_end_own + endpoint + (plus - I);
constexpr size_t partial_branch = endpoint + equal + 2 * D + partial_own;
constexpr size_t erase_body = erase_body_own + common_equal +
			      max_source(iterator_copy, max_source(full_branch, partial_branch));
// Public erase(this,two const-iterator params,result), two selected converting
// const-iterator constructors, and both genuine _M_const_cast child calls.
// Public return prvalue aliases _M_erase return, and both const_cast outputs
// alias the two _M_erase parameters. Each physical I appears exactly once below.
constexpr size_t public_own = P + 2 * I + 2 * (2 * P) + 2 * const_cast_without_result;
constexpr size_t supplement = public_own + erase_body;
constexpr size_t destroy_children = mixed_replay_queue_cleanup_source::destroy_data_controller +
				    mixed_replay_queue_cleanup_source::destroy_nodes_controller;
constexpr size_t full = supplement + destroy_children;
constexpr size_t window = supplement, full_window = full;
// Exact ORIGINAL lexical domain: erase(remove(begin,end,identity),end).
// Empty removal returns first. Full removal selects clear. Partial removal has
// kept>0, elems_before=kept and size-n=kept, so kept<=kept/2 is false; last==end
// prevents MOVE3. No deque shift/backward branch is priced or substituted.
constexpr bool layout = I == 4 * P && sizeof(native_identity_queue::const_iterator) == I &&
			destroy_children <= mixed_replay_queue_cleanup_source::mutation_window;
} // namespace mixed_replay_deque_tail_erase_source
} // namespace

namespace
{
namespace mixed_replay_std_remove_source
{
using namespace mixed_replay_command_assignment_source;
namespace tail = mixed_replay_deque_tail_erase_source;
constexpr size_t I = sizeof(native_identity_queue::iterator);
using predicate_type = __gnu_cxx::__ops::_Iter_equals_val<const std::string>;
constexpr size_t O = sizeof(predicate_type), T = sizeof(std::random_access_iterator_tag);
// Factory's prvalue adapter initializes remove_if's exact by-value parameter.
// O is owned by remove_if, so only value-ref and declared ctor formals here.
constexpr size_t predicate_factory = P + 2 * P;
// Declared adapter operator(this,iterator param,bool), actual iterator copy
// from lvalue and dereference(this,result-ref). String equality is a named child.
constexpr size_t predicate_controller = P + I + B + tail::iterator_copy + 2 * P;
constexpr size_t increment = 2 * P + tail::set_node;
// Category return tag aliases RA parameter; only actual const-ref formal here.
// The aggregate tag has no invented constructor/destructor Source interfaces.
constexpr size_t iterator_category = P;
// RA first/last/pred/trip_count/tag. Its output aliases wrapper's actual return
// temporary; declared lvalue-return copy remains charged once here. Both actual
// named iterator arguments select the captured user-written copy constructor.
constexpr size_t random_find_controller = 2 * I + O + D + T + 2 * tail::iterator_copy +
					  tail::iterator_copy + tail::difference + increment +
					  predicate_controller;
// Wrapper owns first/last/pred and actual returned iterator temporary assigned
// into remove_if first. RA prvalue return aliases that ONE iterator object.
constexpr size_t find_wrapper_controller =
	3 * I + O + 2 * tail::iterator_copy + iterator_category + random_find_controller;
// remove_if first/last/pred/local __result; final output aliases remove output.
// Initial find assignment uses declared/defaulted iterator assignment3P.
// Empty return-copy and populated __result construction/loop/return are separate
// branches. Repeated increment/equality helper calls reuse their named closure;
// their actual receiver targets remain in first/result fields already owned.
constexpr size_t populated_loop = tail::iterator_copy + increment + predicate_controller +
				  2 * (2 * P) + 2 * P + tail::equal + tail::iterator_copy;
constexpr size_t remove_if_controller = 3 * I + O + find_wrapper_controller + 3 * P + tail::equal +
					max_source(tail::iterator_copy, populated_loop);
// Public remove first/last/value-ref/final output. Adapter output is already the
// remove_if parameter; every returned prvalue up this chain is the ONE output.
constexpr size_t supplement =
	3 * I + P + 2 * tail::iterator_copy + predicate_factory + remove_if_controller;
// Actual string== has two const refs/bool, three size queries, two data/Mdata
// chains and char_traits compare. It selects no equal_to<void> wrapper. These
// exact descendants are already retained once by table predicate source.
constexpr size_t equality_child =
	2 * P + B + 3 * (P + N) + 2 * (4 * P) + (2 * P + N + sizeof(int) + B);
constexpr size_t move_child = mixed_replay_key_value_source::move_assignment;
constexpr size_t full = supplement + max_source(equality_child, move_child);
constexpr size_t window = supplement, full_window = full;
constexpr bool layout =
	O == P && T == 1 && I == 4 * P &&
	equality_child <= mixed_replay_table_key_lookup_source::equal &&
	move_child <= mixed_replay_populated_state_cleanup_source::value_lifetime_window &&
	tail::layout;
// Caller begin/end objects alias public remove parameters. Their genuine
// receiver/copy source belongs the separate rollback caller controller, not an
// extra I in this callee. No iterator/adaptor implicit trivial dtor is invented.
} // namespace mixed_replay_std_remove_source
} // namespace

namespace
{
namespace mixed_replay_rollback_controller_source
{
using namespace mixed_replay_command_assignment_source;
namespace tail = mixed_replay_deque_tail_erase_source;
// Actual command.keys const range: hidden range reference, two iterators and
// named key reference and identities reference. The actual local found node
// aliases the transparent find returned iterator ALREADY owned by that profile;
// do not charge it again. identity/command formals are separate from five locals.
constexpr size_t fence_own = 7 * P;
// Original entity_key_bytes(key): key ref, unsigned index and direct array[]
// (this/index/result-ref). The captured GNU13 array indexes its _M_elems
// directly: no invented __array_traits::_S_ref call. Its local/output arrays
// stay in the original separately admitted 2*sizeof(array<char,9>) credit.
constexpr size_t encode_key = P + sizeof(unsigned int) + (2 * P + N);
// This exact encoded.data()/size()/view constructor occurrence is ALREADY
// owned by transparent::view_argument retained in the lookup supplement.
// Its actual view object stays in the original rollback fixed-inline credit.
// Keep the named correspondence but do not add this56 to the new window.
constexpr size_t encoded_view = 2 * P + (P + N) + (2 * P + N);
// const vector begin/end returns initialize the hidden iterators directly.
// Each const endpoint materializes a const-pointer P from its stored mutable
// pointer: receiverP + conversionP + constructor2P =4P. The return object is
// its existing hidden field, not another resultP in addition to that conversion.
// != rewrites ==: two refs/result plus both base(this,result-ref) calls.
// Prefix ++ and dereference each have this plus result-reference formals.
constexpr size_t key_range_queries =
	2 * (2 * P + 2 * P) + (2 * P + B + 2 * (2 * P)) + 2 * P + 2 * P;
// Genuine data-free table end(this,returned node iterator), captured node/base
// pointer constructors(two this/arg pairs); == has two refs/result bool.
constexpr size_t table_end = 2 * P + 4 * P;
constexpr size_t table_equal = 2 * P + B;
// -> -> value_base::_M_valptr -> aligned_buffer::_M_ptr -> _M_addr.
// Each has the captured receiver and pointer return; builtin &storage adds no
// synthetic interface. Actual aligned-buffer source has no addressof call.
constexpr size_t node_arrow = 4 * (2 * P);
// Caller begin/end/end outputs initialize the remove/tail child parameters.
// Their physical returned iterators are already owned by those exact child
// profiles. Only receiver and genuine declared copy-constructor formals here.
constexpr size_t deque_callers = 3 * tail::endpoint_without_result;
// deque::empty compares stored finish/start directly (stl_deque1347). It does
// NOT call begin/end and materializes no iterator: this/bool and two refs/bool.
constexpr size_t deque_empty = P + B + tail::equal;
constexpr size_t table_size = P + N;
constexpr size_t remove_fences_own = fence_own + encode_key + key_range_queries + table_end +
				     table_equal + node_arrow + deque_callers + deque_empty +
				     table_size;
// work.found->second->command: real node arrow plus unique arrow/get/const tuple
// chain. Six actual const accessors each have receiver/ref and returned ptr/ref.
constexpr size_t unique_arrow = 2 * P + 6 * (2 * P);
constexpr size_t operations_callers = table_end + table_equal + node_arrow + unique_arrow;
// Complete legacy/native rollback controller(work ref,bool result), fence
// helper and pending remove/erase caller endpoint scopes. Genuine lookup,
// node/iterator erasure, stdremove/tail and populated state reset children are
// retained separately by their named existing windows before the first callback.
// encode_key explicitly represents the separate real rollback invocation,
// versus the parent key window's earlier pre-insertion entity-key occurrence.
// Table end/compare below find and arrow after find are genuine OUTSIDE-callee
// occurrences, distinct from those inside the old lookup/small-scan/at graph.
// These sequential occurrences are conservatively summed, never numerical
// slack substituted for a missing descendant. update_depth remains a separate
// actual child; this provider does not pretend to close its clock/thread graph.
constexpr size_t window = P + B + operations_callers + remove_fences_own + deque_callers;
constexpr bool layout = sizeof(unsigned int) == 4 &&
			sizeof(std::vector<critical_entity_key>::const_iterator) == P &&
			sizeof(decltype(operations)::iterator) == P &&
			sizeof(decltype(fences)::iterator) == P && tail::layout;
} // namespace mixed_replay_rollback_controller_source
} // namespace

namespace
{
namespace mixed_replay_update_depth_header_source
{
using namespace mixed_replay_command_assignment_source;
// Actual update_depth retains oldest/now, range reference, two node iterators,
// pair-binding reference and the named identity/state member bindings.
// pair's captured tuple_size specialization selects tuple protocol; each
// get<const pair>(ref)->member-ref calls __pair_get::__const_get(ref)->ref.
constexpr size_t own = 2 * N + 6 * P;
// Direct selected native table mutable begin/end (global table is nonconst,
// even though each pair binding is const). Iterator output objects are
// the two hidden loop fields above: price only receiver/_M_begin and captured
// iterator/base pointer-constructor formals here, not second output objects.
constexpr size_t node_begin = P + 2 * P + 4 * P;
constexpr size_t node_end = P + 4 * P;
constexpr size_t node_equal = 2 * P + B;
constexpr size_t node_increment = 2 * P + P + 2 * P;
// Node * -> _M_v -> _M_valptr -> buffer::_M_ptr -> buffer::_M_addr.
constexpr size_t node_dereference = 5 * (2 * P);
constexpr size_t range_queries =
	node_begin + node_end + node_equal + node_increment + node_dereference;
constexpr size_t pair_binding = 2 * (2 * P + 2 * P);
// Repeated actual unique arrow/dereference/get/const tuple accesses reuse their
// genuine selected closure. Each operation_is_* has one const ref and bool.
constexpr size_t state_access = 2 * P + 6 * (2 * P);
constexpr size_t phase_predicate = P + B;
constexpr size_t state_queries = state_access + phase_predicate;
// Actual now_usec header-owned graph only. One steady time_point prvalue,
// one time_since_epoch nanosecond duration, one returned microsecond duration
// and the cast rep materialized for duration(const Rep&) are distinct objects.
// duration_cast/__cast/rep-constructor prvalues alias ONE microsecond object.
// steady_clock::now's OUT-OF-LINE implementation is not in these headers and
// remains an explicit separate external leaf, never an invented frame margin.
constexpr size_t clock_objects = 4 * N;
constexpr size_t epoch_query = P + 2 * P; // receiver plus declared duration copy.
constexpr size_t cast_query = P + P + (P + N) + 2 * P;
constexpr size_t final_count = P + N;
// duration declares ~duration()=default. Its actual selected receivers are
// epoch duration, microseconds and the time_point's duration member. No implicit
// time_point destructor interface is invented in addition to those members.
constexpr size_t clock_cleanup = 3 * P;
constexpr size_t clock_headers =
	N + clock_objects + epoch_query + cast_query + final_count + clock_cleanup;
// Two genuine max(ref,ref)->ref calls. The operation-count RHS sum is one
// actual uint64 temporary; the retained-byte RHS is an existing lvalue.
constexpr size_t high_water = (3 * P + N) + 3 * P;
// completed_cache is actual std::unordered_map, unlike native operations.
constexpr size_t completed_size = 2 * (P + N);
// joinable(this,bool) -> id() -> operator==(id,id)->bool. The prvalue default id
// initializes the right by-value equality parameter, counted ONCE. Left member
// copy is implicit/undeclared and adds no invented constructor Source body.
constexpr size_t thread_joinable = P + B + P + 2 * sizeof(std::thread::id) + B;
constexpr size_t window = own + range_queries + pair_binding + state_queries + clock_headers +
			  high_water + completed_size + thread_joinable;
constexpr bool layout = sizeof(std::chrono::steady_clock::time_point) == N &&
			sizeof(std::chrono::steady_clock::duration) == N &&
			sizeof(std::chrono::steady_clock::rep) == N &&
			std::chrono::steady_clock::period::num == 1 &&
			std::chrono::steady_clock::period::den == 1000000000 &&
			sizeof(std::chrono::microseconds) == N &&
			sizeof(std::chrono::microseconds::rep) == N &&
			sizeof(std::thread::id) == P && sizeof(uint64_t) == N &&
			sizeof(decltype(operations)::const_iterator) == P;
} // namespace mixed_replay_update_depth_header_source
} // namespace

namespace
{
namespace mixed_replay_workspace_entry_source
{
using namespace mixed_replay_command_assignment_source;
namespace text = mixed_replay_string_value_source;
// Actual default-string ctor(this) -> local_data, rvalue-allocator hider,
// one default allocator temporary, local-buffer init and set_length(0).
// GNU13 allocator has NO declared move ctor: std::move selects its real const
// copy ctor and __new_allocator const copy, exactly text::allocator_copy.
// Temporary allocator owns one byte, explicit allocator/base defaults2P and
// explicit ~allocator receiverP. C++20 __new_allocator has NO declared dtor.
// No implicit _Alloc_hider destructor interface is added to this ctor chain.
constexpr size_t default_allocator = sizeof(std::allocator<char>) + 2 * P + P;
constexpr size_t default_string = P + text::local_data + 3 * P + 2 * P + text::allocator_copy +
				  default_allocator + text::init_local + text::set_length;
// Null unique default: unique_ptr, explicitly defaulted __uniq_ptr_impl,
// tuple, both declared Tuple_impl and Head_base default ctors, default_delete.
// __uniq_ptr_data has no declared no-arg ctor body: no extra receiver invented.
constexpr size_t null_unique_default = 8 * P;
constexpr size_t vector_default = mixed_replay_state_construction_source::vector_default;
// Node_iterator()=default and its declared Node_iterator_base() each thisP.
// Iterator physical fields themselves are already in sizeof(actual workspace).
constexpr size_t node_iterator_default = 2 * P;
// These empty/default member constructors run sequentially, under one actual
// workspace ctor. No allocation or reserve callback overlaps them; use their
// named full maximum, not a multiplier for the two string members.
constexpr size_t member_construction =
	max_source(default_string, max_source(null_unique_default,
					      max_source(vector_default, node_iterator_default)));
// ROOM ctor: this/lock-ref/callback/opaque/caller_outer.
// Legacy ctor: this/input-ref/replay/observer/lock-ref/callback/opaque/outer.
constexpr size_t room_constructor = 4 * P + N;
constexpr size_t legacy_constructor = 7 * P + N;
constexpr size_t construction =
	max_source(room_constructor, legacy_constructor) + member_construction;
// Before the initial input-heap refusal, members are genuinely empty and no
// callback or original mutation has happened. The selecting caller's preentry
// capacity covers this entire call until SOURCE becomes self-retained. Price
// the actual no-deallocator paths, rather than borrowing later populated1476.
// Local string: dtor/dispose/is_local (data+const_local_data)/allocator dtor.
constexpr size_t empty_string_cleanup = P + P + (P + B + 2 * P + text::local_data) + P;
// Empty byte vector: dtor, allocator getter, allocator/range Destroy and true
// aux, base dtor, NULL _M_deallocate and explicit allocator dtor. No nonnull
// allocation-traits/deallocator branch is reached on that early return.
constexpr size_t empty_vector_cleanup = P + 2 * P + 3 * P + 2 * P + B + 2 * P + P + (2 * P + N) + P;
// Null unique: destructor/ptr-ref and exact five mutable accessor pairs.
// It calls no deleter and has no declared data/aggregate destructor body.
constexpr size_t null_unique_cleanup = 2 * P + mixed_replay_state_construction_source::tuple_member;
constexpr size_t early_cleanup =
	max_source(empty_string_cleanup, max_source(empty_vector_cleanup, null_unique_cleanup));
constexpr size_t window = max_source(construction, early_cleanup);
// Completed member destruction is already owned by the genuine persistent
// populated value lifetime: workspace unique destructor reaches exactly the
// same unique_destruction+state_cleanup child as operation_pair_cleanup,
// without its pair receiver. Strings/encoded vector run sequentially and use
// the same genuine destruction children, not a new workspace destructor body.
constexpr size_t cleanup_child =
	max_source(text::destruction,
		   max_source(mixed_replay_populated_state_cleanup_source::byte_vector_cleanup,
			      mixed_replay_populated_state_cleanup_source::unique_destruction +
				      mixed_replay_populated_state_cleanup_source::state_cleanup));
constexpr bool layout =
	sizeof(std::allocator<char>) == 1 && default_string == 244 && null_unique_default == 64 &&
	cleanup_child <= mixed_replay_populated_state_cleanup_source::value_lifetime_window;
// Selecting caller must preadmit this SOURCE and actual initial-inline query
// before invoking the entry. Actual entry self-retains SOURCE308 in work.outer
// through callbacks; sizeof(workspace), strings/vector/pointee actual heap
// remain in the original private CURRENT prefix, not a second Source DTO copy.
} // namespace mixed_replay_workspace_entry_source
} // namespace

namespace
{
namespace native_birth_submit_workspace_source
{
using namespace mixed_replay_command_assignment_source;
namespace members = mixed_replay_workspace_entry_source;
// Actual room_submit_workspace ctor: this/callback/opaque/output and caller N.
// Envelope's vector members, both local strings, null state and three default
// node iterators construct sequentially. Reuse their exact selected member
// declaration maxima; physical members remain in sizeof(room_submit_workspace).
constexpr size_t constructor = 4 * P + N;
// unique_lock(mutex&,defer_lock_t): this/mutex and actual by-value empty tag;
// std::__addressof(ref)->pointer is the ONLY reached address helper. The global
// defer_lock object is not newly constructed and has no declared copy/dtor call.
constexpr size_t defer_lock_constructor = 2 * P + sizeof(std::defer_lock_t) + 2 * P;
constexpr size_t member_construction =
	max_source(members::member_construction, defer_lock_constructor);
constexpr size_t construction = constructor + member_construction;
// On the overflow return before lock(), all owning members are still empty
// and unique_lock::_M_owns is false. Its actual destructor has only this;
// the unlock branch is unselected. No external mutex body is invented here.
constexpr size_t early_cleanup = max_source(members::early_cleanup, P);
constexpr size_t window = max_source(construction, early_cleanup);
constexpr bool layout = members::layout && sizeof(std::defer_lock_t) == 1 &&
			sizeof(std::unique_lock<std::mutex>) == 2 * P && construction == 284 &&
			early_cleanup == 121;
// This entry window is retained through the real first callback. Later populated
// ownership/cleanup, held-lock destruction, actual table/queue and external
// callbacks still require their own genuine caller/submit profiles.
} // namespace native_birth_submit_workspace_source
} // namespace

namespace
{
namespace native_birth_submit_clone_state_source
{
using namespace mixed_replay_command_assignment_source;
// Existing command_copy names one seven-pair unique arrow/get/tuple destination
// chain for state->command. Direct work.envelope.command does not call it.
// Keep all genuine typed fresh assignment scopes and the original independently
// reviewed conservative generated-command/id/array credits unchanged.
constexpr size_t destination_unique_access = 7 * (2 * P);
constexpr size_t direct_command_copy = command_copy - destination_unique_access;
// The direct envelope attachment invokes exactly fresh byte-vector assignment;
// neither state nor native unique pointer is accessed at this direct destination.
constexpr size_t direct_attachment_copy = fresh_vector_const_assignment<uint8_t>();
// These two genuine member-copy occurrences are conservatively summed under
// envelope assignment. No undeclared aggregate receiver/dtor is invented.
constexpr size_t envelope_copy = direct_command_copy + direct_attachment_copy;
constexpr size_t state_construction = mixed_replay_state_construction_source::state_window;
constexpr size_t context_construction = mixed_replay_state_construction_source::context_window;
// state->command = std::move(envelope.command) is the exact earlier typed move,
// including its real caller move(ref,result) and one destination unique chain.
constexpr size_t command_transfer = command_move;
// state->native->attachment = std::move(envelope.attachment): actual byte move,
// caller move(ref,result), and two destination unique arrow/get/tuple chains.
constexpr size_t attachment_transfer =
	vector_move_source<uint8_t>() + 2 * P + 2 * destination_unique_access;
constexpr size_t populated_cleanup =
	mixed_replay_populated_state_cleanup_source::value_lifetime_window;
// All these operations and later private-member cleanup are sequential phases.
// Persistent Source stays live across every recensus; member/pointee/capacity
// bytes remain the real existing private CURRENT/explicit allocation requests.
constexpr size_t window = max_source(
	envelope_copy,
	max_source(state_construction,
		   max_source(context_construction,
			      max_source(command_transfer,
					 max_source(attachment_transfer, populated_cleanup)))));
constexpr bool layout = mixed_replay_command_assignment_source::layout &&
			native_birth_submit_workspace_source::layout;
} // namespace native_birth_submit_clone_state_source
} // namespace

namespace
{
namespace native_birth_submit_mutation_children_source
{
using namespace mixed_replay_command_assignment_source;
namespace clone = native_birth_submit_clone_state_source;
namespace rollback = mixed_replay_rollback_controller_source;
// Native submit uses the SAME genuine operations/fences custom tables,
// std::string-key find/at, unique-state emplace, string-rvalue fence subscript,
// node allocation/deallocation and original Prime forecast as replay. No
// completed_cache unordered_map wrapper is included by this table child map.
constexpr size_t tables = mixed_replay_table_forecast_source::window +
			  mixed_replay_table_key_lookup_source::window +
			  mixed_replay_table_node_storage_source::window +
			  mixed_replay_table_value_construction_source::window +
			  mixed_replay_table_insert_source::window +
			  mixed_replay_table_iterator_erase_source::window +
			  mixed_replay_transparent_table_lookup_source::window;
// Both pending_admission and each genuine fence value use the exact existing
// native_identity_queue<string> forecast/push and populated cleanup children.
// The rollback pending remove + tail erase selects the same typed string moves,
// predicates/iterators and true tail-only domain, not prefix shifting/MOVE3.
constexpr size_t queues = mixed_replay_queue_cleanup_source::mutation_window +
			  mixed_replay_deque_tail_erase_source::window +
			  mixed_replay_std_remove_source::window;
// Actual remove_fences controller/array-key/transparent-view/range/end/arrow
// and pending begin/end expression occurrences are unchanged. Retain these
// exact descendants; the legacy rollback's enclosing work-ref/bool-return row
// is NOT the native submit disposition/controller and is excluded here.
constexpr size_t rollback_children =
	rollback::operations_callers + rollback::remove_fences_own + rollback::deque_callers;
// initial_heap_bytes has output pointer and bool result; its only selected
// child __deque_buf_size(size)->size owns input/result sizes. The static map
// constant and builtin sizeof/multiplication introduce no callable wrapper.
constexpr size_t initial_heap_query = P + B + 2 * N;
// The clone slice already retains the actual shared value-lifetime maximum:
// same key string construction/assignment, queue string copy/destruction,
// populated state cleanup, operation string-copy/unique-move and the genuine
// fence deque default+completed-string failure cleanup. Prove their named
// subsets rather than charging that same descendant lifetime a second time.
constexpr size_t shared_value = clone::populated_cleanup;
constexpr size_t window = tables + queues + rollback_children + initial_heap_query;
constexpr bool layout =
	clone::layout && mixed_replay_fence_queue_construction_source::layout &&
	mixed_replay_table_forecast_source::layout && mixed_replay_string_value_source::layout &&
	mixed_replay_key_value_source::layout && mixed_replay_table_key_lookup_source::layout &&
	mixed_replay_table_node_storage_source::layout &&
	mixed_replay_table_value_construction_source::layout &&
	mixed_replay_table_insert_source::layout &&
	mixed_replay_table_iterator_erase_source::layout &&
	mixed_replay_transparent_table_lookup_source::layout &&
	mixed_replay_deque_tail_erase_source::layout && mixed_replay_std_remove_source::layout &&
	rollback::layout && mixed_replay_key_value_source::window <= shared_value &&
	mixed_replay_string_value_source::window <= shared_value &&
	mixed_replay_table_value_construction_source::operation_member_child <= shared_value &&
	mixed_replay_table_value_construction_source::fence_member_child <= shared_value;
} // namespace native_birth_submit_mutation_children_source
} // namespace

namespace
{
namespace mixed_replay_enqueue_entry_controller_source
{
using namespace mixed_replay_command_assignment_source;
// Actual borrowed entry arguments and returned bool. Workspace fields belong
// its existing INLINE. The validation profile local/returned size_t are Source.
constexpr size_t legacy_entry = 6 * P + N + B;
constexpr size_t native_entry = 5 * P + N + B;
constexpr size_t validation_profile =
	2 * N + P + B; // own_source/validation_source and fixed retained query
// The genuine critical_command_valid_frame_bytes() return N is already priced
// by that preserved getter's +sizeof(size_t), not a third new enclosing local.
// unique_lock::mutex(this,returned mutex*) and owns_lock(this,bool).
constexpr size_t lock_queries = 2 * P + P + B;
// Actual observer selection locals: borrowed, observer_reserve, observer_context.
constexpr size_t observer_selection = B + 2 * P;
// Each _admit has work reference, request and bool, around the unchanged prefix
// and actual reserve relay. The full prefix and forwarding are separately held.
constexpr size_t admit_controller = P + N + B;
constexpr size_t observer_readiness = B; // actual no-argument ready predicates
constexpr size_t scalar_field_classifier = P + B;
constexpr size_t vector_size = P + N + sizeof(std::ptrdiff_t);
constexpr size_t vector_index = 2 * P + N;
constexpr size_t array_size = P + N;
constexpr size_t table_size = P + N;
// Full held_retirement_transport_command: command/bytes references, actual
// reference-capturing integer closure, blob_size and item_section/tail/terms.
// integer(this,offset,count) has i/result plus its declared uint64 return.
// No undeclared lambda destructor is invented. Repeated sequential wire reads
// reuse exactly this selected integer/size/index closure, not a wire-size margin.
constexpr size_t held_classifier = 3 * P + 3 * N + sizeof(uint64_t) + B +
				   (P + 3 * N + 2 * sizeof(uint64_t)) + vector_size + vector_index;
constexpr size_t transport_classifier =
	scalar_field_classifier + 4 * scalar_field_classifier + held_classifier;

// Genuine SHOP any_of -> none_of -> find_if -> THREE-argument __find_if ->
// random-access __find_if. Ordinary GNU13 stateless lambda and _Iter_pred are
// each one byte. Actual const-vector endpoints convert stored key* to const
// key* before binding normal_iterator(const Iterator&); the returned iterator
// aliases the by-value any_of parameter. No implicit trivial copy/dtor body.
constexpr size_t predicate_value = sizeof(char);
constexpr size_t adapter_value = sizeof(char);
constexpr size_t random_tag = sizeof(std::random_access_iterator_tag);
constexpr size_t const_endpoint = P + P + 2 * P;
constexpr size_t any_of_controller = 2 * P + predicate_value + B;
constexpr size_t none_of_controller = 2 * P + predicate_value + B;
constexpr size_t find_if_controller = 3 * P + predicate_value;
// Both forwarding returns alias the one find_if result object already above.
constexpr size_t find_dispatch = 2 * P + adapter_value;
constexpr size_t random_find = 2 * P + adapter_value + sizeof(std::ptrdiff_t) + random_tag;
// Returned category tag aliases the random-find by-value tag parameter.
constexpr size_t iterator_category = P;
// Returned adapter aliases the three-arg dispatch parameter above.
constexpr size_t adapter_factory = predicate_value;
constexpr size_t adapter_constructor = P + predicate_value;
constexpr size_t adapter_moves = 2 * (2 * P);
constexpr size_t predicate_call = (2 * P + B) + // adapter(this,iterator,bool)
				  (2 * P + B) + // actual lambda(this,key reference,bool)
				  2 * P; // normal_iterator::operator*(this,returned key reference)
constexpr size_t iterator_increment = 2 * P;
constexpr size_t iterator_difference = 2 * P + sizeof(std::ptrdiff_t) + 2 * (2 * P);
constexpr size_t iterator_equal = 2 * P + B + 2 * (2 * P);
constexpr size_t shared_shop_classifier =
	2 * scalar_field_classifier + 2 * const_endpoint + any_of_controller + none_of_controller +
	find_if_controller + find_dispatch + random_find + iterator_category + adapter_factory +
	adapter_constructor + adapter_moves + predicate_call + iterator_increment +
	iterator_difference + iterator_equal;
// Transport and SHOP tests are distinct actual call occurrences. Later typed
// birth/zone/auction/origin field tests reuse the named field-only closure.
constexpr size_t classification = transport_classifier + shared_shop_classifier;
constexpr size_t own_callers = max_source(legacy_entry, native_entry) + validation_profile +
			       lock_queries + observer_selection + admit_controller +
			       observer_readiness + classification + vector_size + vector_index +
			       array_size + table_size;
// New genuine legacy codec preflight helper: work ref/bool plus source, initial,
// supplement, live and the actual automatic constexpr query local (five N).
// The query value is compile-time initialized, so no accessor call is invented;
// its declared local storage remains Source. Query formals/return and checked
// add are separate. The returned codec status stays in its genuine child Source.
constexpr size_t codec_preflight = P + B + 5 * N + (P + B) + (P + N + B);
constexpr bool layout = sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
			predicate_value == 1 && adapter_value == 1 && random_tag == 1 &&
			sizeof(std::vector<critical_entity_key>::const_iterator) == P;
// One automatic constexpr query local survives each native validity
// preflight/callback block. Those four real occurrences are sequential;
// their named scalar lifetime maximum is one N, not four inline DTOs.
constexpr size_t native_validator_query_local = N;
constexpr size_t supplement = own_callers + codec_preflight + native_validator_query_local;
} // namespace mixed_replay_enqueue_entry_controller_source
} // namespace

namespace
{
namespace native_birth_submit_header_controller_source
{
using namespace mixed_replay_command_assignment_source;
namespace replay = mixed_replay_enqueue_entry_controller_source;
namespace lookup = mixed_replay_table_key_lookup_source;
constexpr size_t E = sizeof(critical_submit_result), J = sizeof(int);
// Actual room_submit_admit(work&, request)->bool and locked reserve relay
// (partial,opaque)->bool with its real work reference. The local-prefix graph
// is already retained by submit_private_current_source_frames; the actual
// CURRENT-C/lender/ROOT callbacks are separate whole-operation dependencies.
constexpr size_t admit = P + N + B;
constexpr size_t relay = 2 * P + N + B;
// room_submit_return(work&, disposition): work reference and the by-value enum
// parameter are distinct. Its returned prvalue is the SAME outer submit result
// already priced by native_birth_submit_source_frames, not another enum object.
// room_submit_rollback(work&) likewise returns that same outer disposition.
constexpr size_t returned_disposition_controller = P + E;
constexpr size_t rollback_controller = P;
// Unlike operations/fences, completed_cache is a real std::unordered_map.
// Its find(this,key&)->iterator and end(this)->iterator wrappers forward to the
// exact same selected string-key Hashtable children already retained by the
// mutation lookup. Each wrapper return aliases its underlying return, rather
// than creating an additional result object. The actual end temporary and
// comparison outside find are separately owned by their named typed rows.
constexpr size_t completed_wrappers = 2 * P + P + lookup::end + lookup::iterator_compare;
// Actual direct array size for the original 17-byte identity request, key-vector
// size/index for the accepting fence loop, and operations/fences size limits.
// Same selected typed declarations execute sequentially; their genuine source
// is kept separately from existing workspace fields and original heap requests.
constexpr size_t direct_accessors = (P + N) + (P + N) + (2 * P + N) + (P + N);
// The unchanged shared-family classifier uses the SAME reviewed any_of/none_of/
// find_if/random-adapter graph, including its separately preserved original
// conservative endpoint credit. It also names typed-command field predicates.
// Initial dispatch's formals/return are already in the original native submit
// source query; the no-argument original validators_ready bool is distinct.
constexpr size_t classification = replay::shared_shop_classifier + B;
// Only exact captured unique_lock/mutex HEADER controllers are named here:
// lock receiver; mutex::lock receiver+__e; destructor receiver -> unlock receiver
// -> mutex::unlock receiver; catch's owns_lock receiver+bool. No fabricated
// __gthread/pthread/throw_system_error body is supplied by these direct rows.
constexpr size_t direct_lock_headers = P + (P + J) + P + P + P + (P + B);
// Actual update_depth declaration/header graph and its now_usec chrono subtree
// are unchanged. The earlier queued_at now_usec invocation is sequential and
// reuses that exact named clock header subtree; no second out-of-line body.
constexpr size_t depth_headers = mixed_replay_update_depth_header_source::window;
constexpr size_t window = admit + relay + returned_disposition_controller + rollback_controller +
			  completed_wrappers + direct_accessors + classification +
			  direct_lock_headers + depth_headers;
constexpr bool layout = replay::layout && native_birth_submit_mutation_children_source::layout &&
			mixed_replay_update_depth_header_source::layout &&
			sizeof(decltype(completed_cache)::iterator) == P && E == 4 && J == 4;
} // namespace native_birth_submit_header_controller_source
} // namespace

namespace
{
bool mixed_replay_enqueue_retained_source(size_t *) noexcept;
}
namespace
{
// Real journal callback parameter is borrowed, not cloned. Its inline and all
// remaining journal/caller frames remain in outer throughout this real scope.
struct legacy_replay_workspace
{
	critical_command &input;
	const replay_observer_context *replay;
	critical_replay_observer_bounded_fn bounded_observer;
	const std::unique_lock<std::mutex> &lock;
	std::vector<uint8_t> encoded;
	std::string identity;
	std::string fence_key;
	std::unique_ptr<operation_state> state;
	decltype(operations)::iterator found;
	decltype(fences)::iterator fence_found;
	native_identity_queue *fence_queue = nullptr;
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer = 0, input_heap = 0, partial = 0, extra = 0, fresh = 0, key_index = 0;
	bool input_moved = false, inserted = false, rollback_cleanup_reserved = false;
	bool observer_started = false;
	legacy_replay_workspace(critical_command &actual_input,
				const replay_observer_context *actual_replay,
				critical_replay_observer_bounded_fn actual_observer,
				const std::unique_lock<std::mutex> &actual_lock,
				bool (*callback)(size_t, void *) noexcept, void *opaque,
				size_t caller_outer) noexcept
		: input(actual_input)
		, replay(actual_replay)
		, bounded_observer(actual_observer)
		, lock(actual_lock)
		, reserve(callback)
		, context(opaque)
		, outer(caller_outer)
	{
	}
};

bool legacy_replay_reserve_locked(size_t exclusive_live, void *opaque) noexcept
{
	auto &work = *static_cast<legacy_replay_workspace *>(opaque);
	// A genuine whole-operation callback already refreshes CURRENT C under
	// this exact lock. Forward its actual relay/context, never reborrow using
	// the relay itself as if it were the registered ROOT callback/guard pair.
	if (critical_room_shared_budget_lender::callback_matches(work.lock, work.reserve,
								 work.context))
		return work.reserve(exclusive_live, work.context);
	// Preserve the original single-callback path outside a matching operation.
	return critical_room_shared_budget_lender::reserve(work.lock, work.reserve, work.context,
							   exclusive_live);
}

bool legacy_replay_local_prefix(legacy_replay_workspace &work, size_t request = 0) noexcept
{
	work.partial = work.outer;
	// Incoming outer owned the authentic input's INITIAL heap. After the original
	// allocation-free move remove precisely that term, then measure the actual
	// moved-from input. The same heap is now private state, later CURRENT C.
	if (work.input_moved)
	{
		if (work.input_heap > work.partial)
			return false;
		work.partial -= work.input_heap;
		if (!room_storage_command(work.partial, work.input))
			return false;
	}
	return room_storage_add(work.partial, sizeof(legacy_replay_workspace)) &&
	       room_storage_add(work.partial, work.encoded.capacity()) &&
	       (!work.rollback_cleanup_reserved ||
		room_storage_add(work.partial,
				 2 * sizeof(std::array<char, 9>) + sizeof(std::string_view))) &&
	       room_storage_string(work.partial, work.identity) &&
	       room_storage_string(work.partial, work.fence_key) &&
	       (!work.state || (room_storage_add(work.partial, sizeof(operation_state)) &&
				room_storage_command(work.partial, work.state->command))) &&
	       room_storage_add(work.partial, request);
}

bool legacy_replay_admit(legacy_replay_workspace &work, size_t request = 0) noexcept
{
	return legacy_replay_local_prefix(work, request) &&
	       legacy_replay_reserve_locked(work.partial, &work);
}

bool legacy_replay_encode_complete_source(legacy_replay_workspace &work) noexcept
{
	size_t source = 0, initial = 0, supplement = 0, live = 0;
	constexpr size_t query = critical_command_startup_codec_complete_source_query_frame_bytes();
	if (!legacy_replay_admit(work, query) ||
	    !critical_command_startup_codec_complete_source_frame_bytes(&source) ||
	    !critical_command_startup_codec_initial_inline_bytes(&initial) ||
	    !critical_command_startup_codec_source_supplement_frame_bytes(&supplement) ||
	    !room_storage_add(source, initial) || !legacy_replay_admit(work, source) ||
	    !legacy_replay_local_prefix(work))
		return false;
	// Full Source/initial preentry is transient; the unchanged codec owns its
	// existing INLINE/heap requests, but requires its genuine Source supplement
	// through every callback. Actual input and old encoded capacity stay prefix.
	live = work.partial;
	if (!room_storage_add(live, supplement))
		return false;
	return critical_command_encode_bounded(work.input, &work.encoded,
					       legacy_replay_reserve_locked, &work,
					       live) == critical_command_codec_result::ok;
}

bool legacy_replay_rollback(legacy_replay_workspace &work) noexcept
{
	// Complete original table/fences/pending rollback. Fixed remove_fences
	// carriers were admitted BEFORE insertion; no callback or synthetic throw
	// runs during cleanup after refusal. Original construction failure does not
	// update_depth; original failed observer does. Keep that distinction.
	if (work.inserted)
	{
		work.found = operations.find(work.identity);
		if (work.found != operations.end())
		{
			remove_fences(work.identity, work.found->second->command);
			operations.erase(work.found);
		}
	}
	work.state.reset();
	pending.erase(std::remove(pending.begin(), pending.end(), work.identity), pending.end());
	if (work.observer_started)
		update_depth();
	return false;
}

// Complete genuine legacy insertion, unselected until its real mixed caller and
// host observer exist. Missing paired observer cannot select an allocating
// original fallback. Original absence of any passive observer remains valid.
[[maybe_unused]] bool
enqueue_legacy_replayed_bounded(critical_command &actual_journal_parameter,
				const replay_observer_context *replay,
				critical_replay_observer_bounded_fn actual_bounded_observer,
				const std::unique_lock<std::mutex> &actual_init_lock,
				bool (*reserve)(size_t, void *) noexcept, void *budget_context,
				size_t exclusive_outer) noexcept
{
	if (!reserve || actual_init_lock.mutex() != &coordinator_mutex ||
	    !actual_init_lock.owns_lock() ||
	    (replay && replay->observer && !actual_bounded_observer))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	if constexpr (!mixed_replay_state_construction_source::supported ||
		      !mixed_replay_command_assignment_source::layout ||
		      !mixed_replay_fence_queue_construction_source::layout ||
		      !mixed_replay_table_forecast_source::layout ||
		      !mixed_replay_string_value_source::layout ||
		      !mixed_replay_key_value_source::layout ||
		      !mixed_replay_table_key_lookup_source::layout ||
		      !mixed_replay_table_node_storage_source::layout ||
		      !mixed_replay_table_value_construction_source::layout ||
		      !mixed_replay_table_insert_source::layout ||
		      !mixed_replay_table_iterator_erase_source::layout ||
		      !mixed_replay_transparent_table_lookup_source::layout ||
		      !mixed_replay_deque_tail_erase_source::layout ||
		      !mixed_replay_std_remove_source::layout ||
		      !mixed_replay_rollback_controller_source::layout ||
		      !mixed_replay_update_depth_header_source::layout ||
		      !mixed_replay_workspace_entry_source::layout ||
		      !mixed_replay_enqueue_entry_controller_source::layout)
		return false;
	size_t own_source = 0;
	const size_t validation_source = critical_command_valid_frame_bytes();
	// Selecting caller preadmits the complete owned entry/query/INLINE before
	// this pure profile and construction. Source remains internally held below.
	if (!mixed_replay_enqueue_retained_source(&own_source))
		return false;
	legacy_replay_workspace work(actual_journal_parameter, replay, actual_bounded_observer,
				     actual_init_lock, reserve, budget_context, exclusive_outer);
	try
	{
		if (!room_storage_command(work.input_heap, work.input) ||
		    work.input_heap > work.outer ||
		    !room_storage_add(work.outer, mixed_replay_table_forecast_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_key_value_source::admitted_window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_queue_cleanup_source::mutation_window) ||
		    !room_storage_add(work.outer, mixed_replay_table_key_lookup_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_table_node_storage_source::window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_table_value_construction_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_table_insert_source::window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_table_iterator_erase_source::window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_transparent_table_lookup_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_deque_tail_erase_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_std_remove_source::window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_rollback_controller_source::window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_update_depth_header_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_workspace_entry_source::window) ||
		    !room_storage_add(work.outer, own_source) ||
		    !room_storage_add(work.outer, validation_source) || !legacy_replay_admit(work))
			return false;
		if (native_transport_command(work.input))
			return false;
		// Exact original execution_supported, with the actual paired selected
		// extension proof. Legacy-valid commands require no extension callback.
		if (!legacy_replay_local_prefix(work) ||
		    !(critical_command_valid(work.input) ||
		      (work.input.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		       critical_command_envelope_valid(work.input) &&
		       extension_validator_callback && extension_validator_bounded_callback &&
		       extension_validator_bounded_callback(
			       work.input, legacy_replay_reserve_locked, &work, work.partial))))
			return false;
		// Pure validation has returned; release precisely its phase allowance
		// before the encoder's full Source replaces the same predicate child.
		work.outer -= validation_source;
		if (!legacy_replay_encode_complete_source(work))
			return false;
		if (!legacy_replay_admit(work))
			return false;
		work.extra = sizeof(std::string);
		if (!room_storage_add(work.extra, work.input.operation_id.bytes.size() + 1) ||
		    !legacy_replay_admit(work, work.extra))
			return false;
		work.identity = operation_key(work.input.operation_id);
		if (!legacy_replay_admit(work))
			return false;
		if (operations.find(work.identity) != operations.end() ||
		    operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
		    work.encoded.size() > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
			return false;
		if (!legacy_replay_admit(
			    work, sizeof(operation_state) +
					  sizeof(std::unique_ptr<operation_state>) +
					  mixed_replay_state_construction_source::state_window))
			return legacy_replay_rollback(work);
		work.state = std::make_unique<operation_state>();
		if (!legacy_replay_admit(work,
					 mixed_replay_command_assignment_source::command_move))
			return legacy_replay_rollback(work);
		// This is the original MOVE, not a deep copy. Scalar ownership selection
		// is immediate and nonfallible before the next complete CURRENT prefix.
		work.state->command = std::move(work.input);
		work.input_moved = true;
		work.state->retained_bytes = work.encoded.size();
		work.state->queued_at_usec = now_usec();
		work.state->attempt = 1;
		work.state->attachments = 0;
		work.state->phase = critical_operation_phase::queued;
		work.state->retain_until_publication = work.state->command.publication_required;
		work.state->admission_failure_queued = false;
		work.rollback_cleanup_reserved = true;
		if (!operations.next_unique_insert_extra_peak(work.identity, 0, &work.extra) ||
		    !legacy_replay_admit(work, work.extra))
			return legacy_replay_rollback(work);
		operations.emplace(work.identity, std::move(work.state));
		work.inserted = true;
		// Private unique_ptr is now null. All transferred command/state heap is
		// in same-lock CURRENT C once; input inline/remaining capacities stay P.
		if (!legacy_replay_admit(work) ||
		    !pending.push_back_extra_peak(work.identity, &work.extra) ||
		    !legacy_replay_admit(work, work.extra))
			return legacy_replay_rollback(work);
		pending.push_back(work.identity);
		if (!legacy_replay_admit(work))
			return legacy_replay_rollback(work);
		work.found = operations.find(work.identity);
		for (work.key_index = 0; work.key_index < work.found->second->command.keys.size();
		     ++work.key_index)
		{
			if (!legacy_replay_admit(work, sizeof(std::string) +
							       2 * sizeof(std::array<char, 9>)))
				return legacy_replay_rollback(work);
			work.fence_key =
				entity_key(work.found->second->command.keys[work.key_index]);
			work.fence_found = fences.find(work.fence_key);
			if (work.fence_found == fences.end())
			{
				if (!native_identity_queue::initial_heap_bytes(&work.fresh) ||
				    !fences.next_unique_insert_extra_peak(
					    work.fence_key, work.fresh, &work.extra) ||
				    !room_storage_add(
					    work.extra,
					    mixed_replay_fence_queue_construction_source::window) ||
				    !legacy_replay_admit(work, work.extra))
					return legacy_replay_rollback(work);
				work.fence_queue = &fences[std::move(work.fence_key)];
				if (!legacy_replay_admit(work))
					return legacy_replay_rollback(work);
			}
			else
				work.fence_queue = &work.fence_found->second;
			if (!work.fence_queue->push_back_extra_peak(work.identity, &work.extra) ||
			    !legacy_replay_admit(work, work.extra))
				return legacy_replay_rollback(work);
			work.fence_queue->push_back(work.identity);
			if (!legacy_replay_admit(work))
				return legacy_replay_rollback(work);
		}
		health.fenced_keys = fences.size();
		if (work.replay && work.replay->observer)
		{
			if (!legacy_replay_local_prefix(work))
				return legacy_replay_rollback(work);
			work.observer_started = true;
			const bool borrowed = critical_room_shared_budget_lender::callback_matches(
				work.lock, work.reserve, work.context);
			const auto observer_reserve = borrowed ? work.reserve :
								 legacy_replay_reserve_locked;
			void *const observer_context = borrowed ? work.context : &work;
			// work.partial retains the real workspace and moved input census;
			// exact callback identity survives into the mandatory SAME-scope owner.
			if (!work.bounded_observer(operations.at(work.identity)->command,
						   work.replay->context, observer_reserve,
						   observer_context, work.partial))
				return legacy_replay_rollback(work);
		}
		// Original no-observer branch remains accepted. Successful real observer
		// has no subsequent fallible callback or new allocation before return.
		update_depth();
		return true;
	}
	catch (...)
	{
		// Actual standard-library exceptions only. Full original rollback is
		// nonallocating; observer-started law preserves original metric timing.
		return legacy_replay_rollback(work);
	}
#endif
}
} // namespace: complete private bounded legacy passive replay insertion

namespace
{
// Complete private accepted shared NMB4 passive replay insertion only. The
// original mixed initializer, replay callbacks, other families and selectors
// remain unchanged. Existing family-neutral ROOM-named owning helpers below
// retain their real state/lock/current census; no authority follows from names.
// Caller owns original envelope, replay context, held coordinator unique_lock,
// full journal frame/metadata and ROOT prefix in exclusive_outer, excluding
// CURRENT coordinator (the authentic same-lock lender adds that exactly once).
// Root must install the actual paired host observer whose shared branch invokes
// quest_mobile_native_birth_restore_shared_shop_bounded. Registration and actual
// initializer selection are separate; this helper remains unselected.
[[maybe_unused]] bool
enqueue_shared_shop_native_replayed_bounded(const critical_native_recovery_envelope &original,
					    const replay_observer_context &replay,
					    const std::unique_lock<std::mutex> &actual_init_lock,
					    bool (*reserve)(size_t, void *) noexcept,
					    void *budget_context, size_t exclusive_outer) noexcept
{
	if (!reserve || !replay.native_observer || !replay.native_bounded_observer ||
	    !native_birth_shared_shop_command(original.command) ||
	    actual_init_lock.mutex() != &coordinator_mutex || !actual_init_lock.owns_lock())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	room_replay_workspace work(actual_init_lock, reserve, budget_context, exclusive_outer);
	// Genuine still-live value carriers coexist with the real workspace through
	// each owning provider. Referred envelope/context/lock storage stays caller-owned.
	if (!room_storage_add(work.outer,
			      sizeof(reserve) + sizeof(budget_context) + sizeof(exclusive_outer)))
		return false;
	try
	{
		if (!room_replay_admit(work))
			return false;
		// Exact original shared native-envelope execution branch: the genuine
		// shared executor must be installed and full original shared recovery
		// validity passes. This original branch bypasses selected extension
		// predicates; no new extension support or command-domain shortcut is added.
		if (!room_replay_local_prefix(work) || !shared_native_apply_callback ||
		    !native_mobile_birth_shared_shop_recovery_valid_bounded(
			    original, room_replay_reserve_locked, &work, work.partial))
			return false;
		if (!room_replay_admit(work) ||
		    !room_checkpoint_size_bounded(original, &work.retained,
						  room_replay_reserve_locked, &work, work.partial))
			return false;
		// Preserve the second original typed-birth proof and all five registered
		// historical validator readiness requirements. Shared NMB4 selects the
		// genuine shared valid codec, exactly as native_birth_recovery_valid.
		// Full phase/revision/receipt/progress/terminal validity accepts both
		// execution and continuation; initial-only validation is not substituted.
		if (!room_replay_admit(work) || !native_birth_validators_ready() ||
		    !native_mobile_birth_shared_shop_recovery_valid_bounded(
			    original, room_replay_reserve_locked, &work, work.partial))
			return false;
		if (!room_replay_admit(work))
			return false;
		// Original operation_key return/local coexistence. Its actual 16-byte
		// GCC13 string requests 17 bytes; assignment consumes the returned string.
		work.extra = sizeof(std::string);
		if (!room_storage_add(work.extra, original.command.operation_id.bytes.size() + 1) ||
		    !room_replay_admit(work, work.extra))
			return false;
		work.identity = operation_key(original.command.operation_id);
		if (!room_replay_admit(work))
			return false;
		if (operations.find(work.identity) != operations.end() ||
		    operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
		    work.retained > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
			return false;
		work.continuation = original.phase ==
				    critical_native_recovery_phase::continuation_pending;
		work.holds_fences = !work.continuation ||
				    native_birth_origin_command(original.command) ||
				    zone_reset_typed_command(original.command);
		if (!room_replay_admit(work, sizeof(operation_state) +
						     sizeof(std::unique_ptr<operation_state>)))
			return room_replay_rollback(work);
		work.state = std::make_unique<operation_state>();
		if (!room_replay_admit(work))
			return room_replay_rollback(work);
		// Fresh default state owns no vector capacity. This genuine source profile
		// reserves ALL command/attachment copy requests and the original native
		// allocation before that complete original copy sequence. There is no
		// callback inside the sequence, and its resulting private state is censused
		// at the next cut using actual vector capacities, not encoded byte length.
		work.fresh = 0;
		if (!room_checkpoint_heap(original, true, work.fresh) ||
		    !room_storage_add(work.fresh, sizeof(native_operation_context)) ||
		    !room_storage_add(work.fresh,
				      sizeof(std::unique_ptr<native_operation_context>)) ||
		    !room_replay_admit(work, work.fresh))
			return room_replay_rollback(work);
		work.state->command = original.command;
		work.state->native = std::make_unique<native_operation_context>();
		work.state->native->revision = original.revision;
		work.state->native->phase = original.phase;
		work.state->native->attachment = original.attachment;
		work.state->retained_bytes = work.retained;
		work.state->queued_at_usec = now_usec();
		work.state->attempt = 1;
		work.state->attachments = 0;
		work.state->phase = work.continuation ?
					    critical_operation_phase::native_continuation_pending :
					    critical_operation_phase::queued;
		work.state->retain_until_publication = true;
		work.state->admission_failure_queued = false;
		work.state->native_physical_released = work.continuation;
		work.rollback_cleanup_reserved = true;
		if (!operations.next_unique_insert_extra_peak(work.identity, 0, &work.extra) ||
		    !room_replay_admit(work, work.extra))
			return room_replay_rollback(work);
		operations.emplace(work.identity, std::move(work.state));
		work.inserted = true;
		// state is now genuinely owned by CURRENT C; the moved-from pointer is
		// excluded from private scratch before any pending/fence request.
		if (!room_replay_admit(work))
			return room_replay_rollback(work);
		if (!work.continuation)
		{
			if (!pending.push_back_extra_peak(work.identity, &work.extra) ||
			    !room_replay_admit(work, work.extra))
				return room_replay_rollback(work);
			pending.push_back(work.identity);
			if (!room_replay_admit(work))
				return room_replay_rollback(work);
		}
		if (work.holds_fences)
		{
			work.found = operations.find(work.identity);
			for (work.key_index = 0;
			     work.key_index < work.found->second->command.keys.size();
			     ++work.key_index)
			{
				if (!room_replay_admit(work,
						       sizeof(std::string) +
							       2 * sizeof(std::array<char, 9>)))
					return room_replay_rollback(work);
				work.fence_key = entity_key(
					work.found->second->command.keys[work.key_index]);
				work.fence_found = fences.find(work.fence_key);
				if (work.fence_found == fences.end())
				{
					if (!native_identity_queue::initial_heap_bytes(
						    &work.fresh) ||
					    !fences.next_unique_insert_extra_peak(
						    work.fence_key, work.fresh, &work.extra) ||
					    !room_replay_admit(work, work.extra))
						return room_replay_rollback(work);
					work.fence_queue = &fences[std::move(work.fence_key)];
					if (!room_replay_admit(work))
						return room_replay_rollback(work);
				}
				else
					work.fence_queue = &work.fence_found->second;
				if (!work.fence_queue->push_back_extra_peak(work.identity,
									    &work.extra) ||
				    !room_replay_admit(work, work.extra))
					return room_replay_rollback(work);
				work.fence_queue->push_back(work.identity);
				if (!room_replay_admit(work))
					return room_replay_rollback(work);
			}
			health.fenced_keys = fences.size();
		}
		// The actual paired shared NMB4 host observer forwards this full prefix
		// and authentic reserve/context to the genuine passive birth owner.
		// Original mandatory observer remains installed. The reviewed birth
		// companion owns exact duplicate/new-attachment rollback and current
		// census; no unbounded domain/observer fallback runs on this capability.
		if (!room_replay_local_prefix(work) ||
		    !replay.native_bounded_observer(original, replay.context,
						    room_replay_reserve_locked, &work,
						    work.partial))
			return room_replay_rollback(work);
		// No fallible callback after successful actual birth registry attachment.
		// Exact original successful tail is nonallocating.
		update_depth();
		return true;
	}
	catch (...)
	{
		// Genuine allocation exceptions only; actual init lock remains owned.
		// Partially copied private state and actual inserted nodes are removed,
		// while real surviving bucket/deque capacity stays in CURRENT C.
		return room_replay_rollback(work);
	}
#endif
}
} // namespace: complete private bounded shared NMB4 passive replay insertion

player_save_prepared_startup_lifecycle_owner
critical_mixed_startup_replay_owner::acquire_lifecycle()
{
	return player_save_prepared_startup_lifecycle_owner();
}

critical_mixed_startup_replay_owner::critical_mixed_startup_replay_owner(
	const std::unique_lock<std::mutex> &actual_init_lock,
	const player_save_prepared_startup_lifecycle_owner &actual_lifecycle) noexcept
	: init_lock_(actual_init_lock)
	, lifecycle_(actual_lifecycle)
{
	// Constructor is private to the original initializer, AFTER its unchanged
	// init guard. Identity is the real live stack owner, not a readiness flag.
	if (!active_mixed_startup_owner && init_lock_.mutex() == &coordinator_mutex &&
	    init_lock_.owns_lock() && lifecycle_.held())
		active_mixed_startup_owner = this;
}

critical_mixed_startup_replay_owner::~critical_mixed_startup_replay_owner() noexcept
{
	if (active_mixed_startup_owner == this)
		active_mixed_startup_owner = nullptr;
}

const critical_mixed_startup_replay_owner *critical_mixed_startup_replay_owner::current() noexcept
{
	// Called only by the paired original-coordinator replay callback. The
	// real init lock is held; this is not an unlocked outside host observer.
	return active_mixed_startup_owner && active_mixed_startup_owner->held() ?
		       active_mixed_startup_owner :
		       nullptr;
}

bool critical_mixed_startup_replay_owner::held() const noexcept
{
	return active_mixed_startup_owner == this && init_lock_.mutex() == &coordinator_mutex &&
	       init_lock_.owns_lock() && lifecycle_.held();
}

bool critical_mixed_startup_replay_owner::reserve_coordinator_cut(
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) const noexcept
{
	if (!reserve || !held())
		return false;
	// Same actual init lock and complete existing CURRENT-C lender. The real
	// caller charges this bridge/identity frame before entering this method.
	return critical_room_shared_budget_lender::reserve(init_lock_, reserve, context, outer);
}

namespace
{
// These are real nullable paired companions of the original full initial
// dispatch. Historical callbacks/readiness stay mandatory and unchanged; no
// readiness flag, raw accept result or old allocating predicate is substituted.
[[maybe_unused]] bool
native_birth_submit_initial_bounded(const critical_native_recovery_envelope &envelope,
				    bool (*reserve)(size_t, void *) noexcept, void *context,
				    size_t outer) noexcept
{
	if (native_birth_shared_shop_command(envelope.command))
		return native_birth_validators.shared_shop_initial_bounded &&
		       native_birth_validators.shared_shop_initial_bounded(envelope, reserve,
									   context, outer);
	return envelope.command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ?
		       (native_birth_validators.cash_role_initial_bounded &&
			native_birth_validators.cash_role_initial_bounded(envelope, reserve,
									  context, outer)) :
		       (native_birth_validators.initial_bounded &&
			native_birth_validators.initial_bounded(envelope, reserve, context, outer));
}

// New complete provider only. Old three callbacks/readiness and the old owning
// initial dispatcher above remain unchanged. All predicate and callback order
// follows shared-any-SHOP (including malformed), then cash-v4, then historical.
// Fixed profiles are queried ONLY after exact registered target identity.
bool native_birth_submit_initial_complete_bounded(room_submit_workspace &work) noexcept
{
	if (native_birth_shared_shop_command(work.envelope.command))
	{
		if (native_birth_validators.shared_shop_initial_fixed_bounded !=
		    native_mobile_birth_shared_shop_recovery_initial_fixed_bounded)
			return false;
		// Required constant evaluation: one real local N, no runtime accessor.
		constexpr size_t query =
			native_mobile_birth_shared_shop_recovery_initial_fixed_bounded_source_query_frame_bytes();
		if (!room_submit_admit(work, query) ||
		    !native_mobile_birth_shared_shop_recovery_initial_fixed_bounded_source_frame_bytes(
			    &work.extra) ||
		    !native_mobile_birth_shared_shop_recovery_initial_fixed_bounded_initial_inline_bytes(
			    &work.fresh) ||
		    !room_storage_add(work.extra, work.fresh) ||
		    !room_submit_admit(work, work.extra) || !room_submit_local_prefix(work))
			return false;
		// Exact child owns Source/INITIAL internally; the real caller-only
		// prefix excludes completed transient preflight supplements.
		return native_birth_validators.shared_shop_initial_fixed_bounded(
			work.envelope, room_submit_reserve_locked, &work, work.partial);
	}
	else if (work.envelope.command.payload_version ==
		 NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION)
	{
		if (native_birth_validators.cash_role_initial_fixed_bounded !=
		    native_mobile_birth_cash_role_recovery_initial_fixed_bounded)
			return false;
		// Required constant evaluation: one real local N, no runtime accessor.
		constexpr size_t query =
			native_mobile_birth_cash_role_recovery_initial_fixed_bounded_source_query_frame_bytes();
		if (!room_submit_admit(work, query) ||
		    !native_mobile_birth_cash_role_recovery_initial_fixed_bounded_source_frame_bytes(
			    &work.extra) ||
		    !native_mobile_birth_cash_role_recovery_initial_fixed_bounded_initial_inline_bytes(
			    &work.fresh) ||
		    !room_storage_add(work.extra, work.fresh) ||
		    !room_submit_admit(work, work.extra) || !room_submit_local_prefix(work))
			return false;
		// Exact child owns Source/INITIAL internally; the real caller-only
		// prefix excludes completed transient preflight supplements.
		return native_birth_validators.cash_role_initial_fixed_bounded(
			work.envelope, room_submit_reserve_locked, &work, work.partial);
	}
	else
	{
		if (native_birth_validators.historical_initial_fixed_bounded !=
		    native_mobile_birth_recovery_initial_fixed_bounded)
			return false;
		// Required constant evaluation: one real local N, no runtime accessor.
		constexpr size_t query =
			native_mobile_birth_recovery_initial_fixed_bounded_source_query_frame_bytes();
		if (!room_submit_admit(work, query) ||
		    !native_mobile_birth_recovery_initial_fixed_bounded_source_frame_bytes(
			    &work.extra) ||
		    !native_mobile_birth_recovery_initial_fixed_bounded_initial_inline_bytes(
			    &work.fresh) ||
		    !room_storage_add(work.extra, work.fresh) ||
		    !room_submit_admit(work, work.extra) || !room_submit_local_prefix(work))
			return false;
		// Exact child owns Source/INITIAL internally; the real caller-only
		// prefix excludes completed transient preflight supplements.
		return native_birth_validators.historical_initial_fixed_bounded(
			work.envelope, room_submit_reserve_locked, &work, work.partial);
	}
}

// Complete original native_matches + critical_command_equal algorithms. Both
// original command encodes now pass through their genuine bounded encoders;
// the first actual encoded capacity remains owned through the second call.
// No reserve around the original allocating equality function is used.
bool native_birth_submit_matches_complete_bounded(const operation_state &,
						  const critical_native_recovery_envelope &,
						  bool (*)(size_t, void *) noexcept, void *,
						  size_t) noexcept;

bool native_birth_submit_matches_bounded(const operation_state &state,
					 const critical_native_recovery_envelope &envelope,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer) noexcept
{
	if (!state.native || state.native->revision != envelope.revision ||
	    state.native->phase != envelope.phase ||
	    state.native->attachment != envelope.attachment)
		return false;
	std::vector<uint8_t> left_encoded;
	std::vector<uint8_t> right_encoded;
	size_t live = outer;
	// Own the two output vector objects and this helper's params/local/return.
	// Existing nested encoder frame profiles remain caller-owned and unqualified.
	if (!room_storage_add(live, 2 * sizeof(std::vector<uint8_t>)) ||
	    !room_storage_add(live, 4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) ||
	    !reserve || !reserve(live, context))
		return false;
	if (critical_command_encode_bounded(state.command, &left_encoded, reserve, context, live) !=
	    critical_command_codec_result::ok)
		return false;
	if (!room_storage_add(live, left_encoded.capacity()))
		return false;
	return critical_command_encode_bounded(envelope.command, &right_encoded, reserve, context,
					       live) == critical_command_codec_result::ok &&
	       left_encoded == right_encoded;
}

size_t native_birth_submit_source_frames() noexcept
{
	// Actual new submit parameters, returned disposition and source-frame
	// query. Existing full room_* workspace/lender/census/codec call profiles
	// stay caller-owned; this function explicitly grants no first-call closure.
	// New initial dispatch parameters/return coexist with its real selected
	// bounded companion, whose own complete profile is that callback's duty.
	return 4 * sizeof(void *) + sizeof(size_t) + sizeof(critical_submit_result) +
	       sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
}
} // namespace: original mobile birth full bounded submit companions

namespace
{
namespace native_birth_submit_admitted_trace_source
{
using namespace mixed_replay_command_assignment_source;
constexpr size_t J = sizeof(int), U = sizeof(uint64_t);
// Only persistence_command_trace(command,command_admitted): actual DMI event
// default construction has a generated nontrivial receiver. Its event object
// and optional non-NRVO return/recorder parameter are the original TWO E heap-
// free objects admitted at the tail. No implicit array/id/event copy, assignment
// or destructor callable is invented. Nested brace initialization is storage.
constexpr size_t builder_own = P + sizeof(persistence_trace_stage) + N + P;
constexpr size_t vector_size = P + N;
constexpr size_t vector_key_index = 2 * P + N;
constexpr size_t array_key_index = 2 * P + N;
constexpr size_t array_key_size = P + N;
constexpr size_t key_min = 3 * P;
// min's array.size() prvalue is the same returned size_t bound to its const
// reference. No second temporary N is introduced for that expression.
constexpr size_t builder =
	builder_own + vector_size + vector_key_index + array_key_index + array_key_size + key_min;
// unique_lock(mutex,try_to_lock_t) directly invokes mutex::try_lock. It does
// NOT call unique_lock::try_lock. Static tag has only its true by-value formal;
// lock's device/owns fields are in the original physical lock request.
constexpr size_t try_constructor = 2 * P + sizeof(std::try_to_lock_t);
constexpr size_t address_of = 2 * P;
constexpr size_t mutex_try = P + B;
constexpr size_t owns_query = P + B;
constexpr size_t lock_cleanup = 3 * P; // ~unique_lock -> unlock -> mutex::unlock.
constexpr size_t lock_headers =
	try_constructor + address_of + mutex_try + owns_query + lock_cleanup;
// Two actual sequential gthread trylock/unlock occurrences, each owns its
// pointer formal/int return and active_p's int return. active_p's pointer is
// STATIC storage. pthread implementations remain separate out-of-line leaves.
constexpr size_t gthread_headers = 2 * (P + J + J);
// Selected event-array ring index and size declarations; repeated key-array
// size uses the named builder declaration above. incident=false makes the
// original incident ring/equality/loop subtree unreachable on this route.
constexpr size_t ring_headers = (2 * P + N) + (P + N);
// uint64_t specialization inherits this exact __atomic_base::fetch_add.
// No additional atomic wrapper, static constructor or builtin callee body.
constexpr size_t drop_atomic = P + U + sizeof(std::memory_order) + U;
// trace_now is the exact now_usec chrono-header expression, executed
// sequentially with queued_at/update_depth. Its distinct physical durations,
// declared header calls reuse depth's genuine clock helper/type-object144.
// Its OWN wrapper return uint64_t is distinct from now_usec's return and is
// retained here. steady_clock::now's body remains OPEN.
constexpr size_t shared_clock_helpers = mixed_replay_update_depth_header_source::clock_headers - N;
constexpr size_t trace_now_return = U;
constexpr size_t window =
	builder + lock_headers + gthread_headers + ring_headers + drop_atomic + trace_now_return;
constexpr bool layout = mixed_replay_update_depth_header_source::layout &&
			native_birth_submit_header_controller_source::layout &&
			sizeof(persistence_trace_stage) == 1 && sizeof(std::try_to_lock_t) == 1 &&
			sizeof(std::memory_order) == 4 && J == 4 && U == N;
} // namespace native_birth_submit_admitted_trace_source
} // namespace

critical_submit_result critical_native_mobile_birth_publication_owner::submit_bounded(
	const critical_native_recovery_envelope &original, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer, size_t *current_coordinator_bytes) noexcept
{
	if (!reserve || !current_coordinator_bytes ||
	    !native_birth_typed_command(original.command) || original.revision != 1 ||
	    original.phase != critical_native_recovery_phase::execution_pending)
		return critical_submit_result::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return critical_submit_result::overloaded;
#else
	room_submit_workspace work(reserve, context, outer, current_coordinator_bytes);
	// Caller owns its complete input/current/journal/registry prefix. Real new
	// source parameter/helper carriers are additive; workspace itself and its
	// current private heaps remain owned by the existing complete local prefix.
	if (!submit_private_current_source_frames(&work.extra) ||
	    !room_storage_add(work.outer, work.extra) ||
	    !room_storage_add(work.outer, native_birth_submit_workspace_source::window) ||
	    !room_storage_add(work.outer, native_birth_submit_source_frames()))
		return critical_submit_result::overloaded;
	if (!submit_clone_state_source_frames(&work.extra) ||
	    !room_storage_add(work.outer, work.extra))
		return critical_submit_result::overloaded;
	if (!submit_mutation_child_source_frames(&work.extra) ||
	    !room_storage_add(work.outer, work.extra))
		return critical_submit_result::overloaded;
	if (!submit_header_controller_source_frames(&work.extra) ||
	    !room_storage_add(work.outer, work.extra))
		return critical_submit_result::overloaded;
	// The SAME complete pure CURRENT-C census runs on the first locked
	// lender call and every later callback/return. Retain its actual Source
	// before that first observation; C bytes themselves stay lender-owned.
	if (!submit_coordinator_current_source_frames(&work.extra) ||
	    !room_storage_add(work.outer, work.extra))
		return critical_submit_result::overloaded;
	// Actual command_admitted diagnostics Source; original two event objects
	// and recorder lock stay in the unchanged tail request. Exact chrono header
	// Source is shared with the persistently retained depth profile, once.
	if (!submit_admitted_trace_source_frames(&work.extra) ||
	    !room_storage_add(work.outer, work.extra))
		return critical_submit_result::overloaded;
	// New helper owns its work reference, bool return, and one real constexpr
	// local query N across mutually exclusive branches. Original conservative
	// old-dispatch carrier credits stay unchanged, never repurposed as slack.
	if (!room_storage_add(work.outer, sizeof(void *) + sizeof(bool) + sizeof(size_t)))
		return critical_submit_result::overloaded;
	// Actual automatic constexpr query local in the later shared-only branch.
	// Retain before first callback; no runtime nested accessor is invoked.
	if (!room_storage_add(work.outer, sizeof(size_t)))
		return critical_submit_result::overloaded;
	// Exact original allocation-free critical valid/envelope/legacy closure.
	// Its published allowance already includes its runtime getter return N.
	// Store that returned allowance in existing scratch, without another local
	// or returned-size charge. All original predicates/evaluation order below
	// remain byte-identical; the Source persists through the real callbacks.
	if (!submit_command_validation_source_frames(&work.extra) ||
	    !room_storage_add(work.outer, work.extra))
		return critical_submit_result::overloaded;
	// Local/header lender and actual scalar loan descendants persist before
	// the first callback. CURRENT-C Source is separately retained above; the
	// actual configured ROOT callback and external pthread body stay separate.
	if (!submit_lender_header_source_frames(&work.extra) ||
	    !room_storage_add(work.outer, work.extra))
		return critical_submit_result::overloaded;
	// Actual submit lock/unlock gthread header and pthread declarations are
	// distinct from the already retained unique/mutex header controllers.
	if (!submit_lock_gthread_header_source_frames(&work.extra) ||
	    !room_storage_add(work.outer, work.extra))
		return critical_submit_result::overloaded;
	// Existing inline scratch returns to its original empty value before
	// locking/callbacks; only Source in outer survives the pure query.
	work.extra = 0;
	try
	{
		// All complete canonical proofs and genuine clone allocations now run
		// under the original mutex so the FIRST request contains actual current
		// coordinator ownership. Original pure predicate order remains unchanged.
		work.lock.lock();
		if (!room_submit_admit(work))
			return room_submit_rollback(work);
		if (!room_submit_local_prefix(work) ||
		    !mixed_startup_checkpoint_size_bounded(original, &work.retained,
							   room_submit_reserve_locked, &work,
							   work.partial))
			return work.denied ?
				       room_submit_rollback(work) :
				       room_submit_return(work, critical_submit_result::invalid);
		// Complete original carrier copy, all vector request sizes genuine. The
		// authentic original belongs to caller outer and remains live throughout.
		work.fresh = 0;
		if (!room_checkpoint_heap(original, true, work.fresh) ||
		    !room_submit_admit(work, work.fresh))
			return room_submit_rollback(work);
		work.envelope = original;
		if (!room_submit_admit(work))
			return room_submit_rollback(work);
		// Preserve the complete original native execution family dispatch.
		// Shared SHOP bypasses extension support only through its original
		// installed shared executor and complete shared recovery validity.
		// Ordinary birth STILL needs the actually selected legacy/extension
		// support; an ordinary worker callback alone grants no admission.
		if (!room_submit_local_prefix(work))
			return room_submit_rollback(work);
		if (native_birth_shared_shop_command(work.envelope.command))
		{
			// Same original shared occurrence; fixed Source belongs ONLY to this
			// exact target. Missing/custom identities refuse only the new provider.
			if (!shared_native_apply_callback ||
			    native_birth_validators.shared_shop_valid_bounded !=
				    native_mobile_birth_shared_shop_recovery_valid_fixed_bounded)
				return room_submit_return(work, critical_submit_result::invalid);
			// Required compile-time accessor evaluation, with one actual local N.
			constexpr size_t shared_query =
				native_mobile_birth_shared_shop_recovery_fixed_valid_source_query_frame_bytes();
			if (!room_submit_admit(work, shared_query) ||
			    !native_mobile_birth_shared_shop_recovery_fixed_valid_source_frame_bytes(
				    &work.extra) ||
			    !native_mobile_birth_shared_shop_recovery_fixed_valid_initial_inline_bytes(
				    &work.fresh) ||
			    !room_storage_add(work.extra, work.fresh) ||
			    !room_submit_admit(work, work.extra) ||
			    !room_submit_local_prefix(work) ||
			    !native_birth_validators.shared_shop_valid_bounded(
				    work.envelope, room_submit_reserve_locked, &work, work.partial))
				return work.denied ?
					       room_submit_rollback(work) :
					       room_submit_return(work,
								  critical_submit_result::invalid);
		}
		else if (!(critical_command_valid(work.envelope.command) ||
			   (work.envelope.command.schema_version ==
				    CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			    critical_command_envelope_valid(work.envelope.command) &&
			    extension_validator_callback && extension_validator_bounded_callback &&
			    extension_validator_bounded_callback(work.envelope.command,
								 room_submit_reserve_locked, &work,
								 work.partial))))
			return work.denied ?
				       room_submit_rollback(work) :
				       room_submit_return(work, critical_submit_result::invalid);
		// Exact original typed-birth readiness then full initial dispatch.
		// The mutually exclusive zone/auction predicates are false for this
		// original private typed-birth wrapper, just as in native_recovery_submit.
		if (!room_submit_local_prefix(work) || !native_birth_validators_ready() ||
		    !native_birth_submit_initial_complete_bounded(work))
			return work.denied ?
				       room_submit_rollback(work) :
				       room_submit_return(work, critical_submit_result::invalid);
		if (!health.initialized || !health.accepting || stop_requested ||
		    !native_replay_observer_callback)
			return room_submit_return(work, critical_submit_result::unavailable);
		// operation_key's 16 bytes require an actual 17-byte GCC13 string request;
		// assignment also owns its returned temporary string until the full expression.
		work.extra = sizeof(std::string);
		if (!room_storage_add(work.extra, original.command.operation_id.bytes.size() + 1) ||
		    !room_submit_admit(work, work.extra))
			return room_submit_rollback(work);
		work.identity = operation_key(work.envelope.command.operation_id);
		if (!room_submit_admit(work))
			return room_submit_rollback(work);
		work.completed = completed_cache.find(work.identity);
		if (work.completed != completed_cache.end())
			return room_submit_return(work, critical_submit_result::identity_conflict);
		work.found = operations.find(work.identity);
		if (work.found != operations.end())
		{
			constexpr size_t match_query = submit_matches_source_query_frames();
			size_t match_source = 0, match_initial = 0;
			// These three actual caller locals remain Source until this branch
			// returns. Full helper Source/INITIAL preentry is transient; the
			// helper owns its own Source and real vector capacities internally.
			if (!room_storage_add(work.outer, 3 * sizeof(size_t)) ||
			    !room_submit_admit(work, match_query) ||
			    !submit_matches_source_frames(&match_source) ||
			    !submit_matches_initial_inline_bytes(&match_initial) ||
			    !room_storage_add(match_source, match_initial) ||
			    !room_submit_admit(work, match_source))
				return room_submit_rollback(work);
			if (!room_submit_local_prefix(work) ||
			    !native_birth_submit_matches_complete_bounded(
				    *work.found->second, work.envelope, room_submit_reserve_locked,
				    &work, work.partial))
				return work.denied ?
					       room_submit_rollback(work) :
					       room_submit_return(
						       work,
						       critical_submit_result::identity_conflict);
			++work.found->second->attachments;
			++health.attached;
			return room_submit_return(work, critical_submit_result::attached);
		}
		if (operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
		    work.retained > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
		{
			++health.overloads;
			return room_submit_return(work, critical_submit_result::overloaded);
		}
		if (!room_submit_admit(work, sizeof(operation_state) +
						     sizeof(std::unique_ptr<operation_state>)))
			return room_submit_rollback(work);
		work.state = std::make_unique<operation_state>();
		if (!room_submit_admit(work))
			return room_submit_rollback(work);
		work.state->command = std::move(work.envelope.command);
		if (!room_submit_admit(work,
				       sizeof(native_operation_context) +
					       sizeof(std::unique_ptr<native_operation_context>)))
			return room_submit_rollback(work);
		work.state->native = std::make_unique<native_operation_context>();
		work.state->native->revision = work.envelope.revision;
		work.state->native->phase = work.envelope.phase;
		work.state->native->attachment = std::move(work.envelope.attachment);
		work.state->retained_bytes = work.retained;
		work.state->queued_at_usec = now_usec();
		work.state->attempt = 1;
		work.state->attachments = 0;
		work.state->phase = critical_operation_phase::awaiting_durability;
		work.state->retain_until_publication = true;
		work.state->admission_failure_queued = false;
		// Genuine remove_fences owns both optional-NRVO array objects and its
		// string_view. Admit cleanup BEFORE insertion can create rollback work;
		// retain this allowance through every later prefix. Never call reserve
		// while cleaning up after denial or a genuine allocation exception.
		work.rollback_cleanup_reserved = true;
		if (!operations.next_unique_insert_extra_peak(work.identity, 0, &work.extra) ||
		    !room_submit_admit(work, work.extra))
			return room_submit_rollback(work);
		operations.emplace(work.identity, std::move(work.state));
		work.inserted = true;
		if (!room_submit_admit(work) ||
		    !pending_admission.push_back_extra_peak(work.identity, &work.extra) ||
		    !room_submit_admit(work, work.extra))
			return room_submit_rollback(work);
		pending_admission.push_back(work.identity);
		pending_admission_bytes += work.retained;
		work.admission_queued = true;
		if (!room_submit_admit(work))
			return room_submit_rollback(work);
		// Genuine original add_fences, with admission between the real map's
		// absent-key/default queue construction and that real queue's push.
		work.found = operations.find(work.identity);
		for (work.key_index = 0; work.key_index < work.found->second->command.keys.size();
		     ++work.key_index)
		{
			if (!room_submit_admit(work, sizeof(std::string) +
							     2 * sizeof(std::array<char, 9>)))
				return room_submit_rollback(work);
			work.fence_key =
				entity_key(work.found->second->command.keys[work.key_index]);
			work.fence_found = fences.find(work.fence_key);
			if (work.fence_found == fences.end())
			{
				if (!native_identity_queue::initial_heap_bytes(&work.fresh) ||
				    !fences.next_unique_insert_extra_peak(
					    work.fence_key, work.fresh, &work.extra) ||
				    !room_submit_admit(work, work.extra))
					return room_submit_rollback(work);
				// Original entity_key result is an rvalue and invokes this exact
				// genuine table/default-mapped construction path.
				work.fence_queue = &fences[std::move(work.fence_key)];
				if (!room_submit_admit(work))
					return room_submit_rollback(work);
			}
			else
				work.fence_queue = &work.fence_found->second;
			if (!work.fence_queue->push_back_extra_peak(work.identity, &work.extra) ||
			    !room_submit_admit(work, work.extra))
				return room_submit_rollback(work);
			work.fence_queue->push_back(work.identity);
			if (!room_submit_admit(work))
				return room_submit_rollback(work);
		}
		health.fenced_keys = fences.size();
		// Actual fixed diagnostic return/argument objects (NRVO optional) and
		// recorder's unique_lock, before the original nonallocating accepted tail.
		if (!room_submit_admit(work, 2 * sizeof(persistence_trace_event) +
						     sizeof(std::unique_lock<std::mutex>)))
			return room_submit_rollback(work);
		persistence_trace_record(
			persistence_command_trace(operations.at(work.identity)->command,
						  persistence_trace_stage::command_admitted));
		++health.accepted;
		update_depth();
		admission_available.notify_one();
		return room_submit_return(work, critical_submit_result::awaiting_durability);
	}
	catch (...)
	{
		// Only genuine standard-library exceptions enter here. A failed lock owns
		// no coordinator state and must not perform an unlocked census or rollback.
		if (!work.lock.owns_lock())
			return critical_submit_result::unavailable;
		return room_submit_rollback(work);
	}
#endif
}

namespace
{
// Genuine ordinary birth observations. The original shared SHOP copier and
// ROOM owner are untouched. Every prospective request uses the actual lock,
// registered callback and common guard, adding fresh CURRENT C exactly once.
struct ordinary_birth_observation_workspace
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, live = 0, current = 0, extra = 0;
	std::unique_lock<std::mutex> lock{ coordinator_mutex, std::defer_lock };
	std::string identity = {};
	critical_native_recovery_envelope copy = {};
	decltype(operations)::iterator operation = {};
	decltype(completed_cache)::iterator completed = {};
	const operation_state *state = nullptr;
	critical_native_recovery_phase phase = critical_native_recovery_phase::execution_pending;

	static bool relay(size_t exclusive_live, void *opaque) noexcept
	{
		auto &work = *static_cast<ordinary_birth_observation_workspace *>(opaque);
		return critical_room_shared_budget_lender::reserve(
			work.lock, work.reserve, work.context, exclusive_live, &work.current);
	}
	bool admit(size_t prospective = 0) noexcept
	{
		live = outer;
		// Whole actual workspace plus these simultaneous source carriers:
		// provider: input/output/reserve/context/current-output pointers,
		//           outer size and bool return (5 pointers + size + bool);
		// admit:    implicit this pointer, prospective size and bool return;
		// relay:    exclusive size, opaque pointer, work reference and bool return.
		// Total: 8 pointers + 3 sizes + 3 bools, outside the workspace itself.
		// Existing full lender/census/allocator/codec native profiles remain
		// caller-owned and unqualified; no first-admission closure is claimed.
		if (!room_storage_add(live, sizeof(*this)) ||
		    !room_storage_add(live,
				      8 * sizeof(void *) + 3 * sizeof(size_t) + 3 * sizeof(bool)) ||
		    (identity.capacity() > 15 &&
		     !room_storage_add(live, identity.capacity() + 1)) ||
		    !room_checkpoint_heap(copy, false, live) ||
		    !room_storage_add(live, prospective))
			return false;
		return relay(live, this);
	}
};

bool ordinary_birth_observation_command(const critical_command &command) noexcept
{
	return native_birth_typed_command(command) && !native_birth_shared_shop_command(command);
}

// Complete original critical_command_equal algorithm, using both genuine
// bounded encoders. First actual capacity stays live through the second encode.
bool ordinary_birth_observation_equal(const critical_command &left, const critical_command &right,
				      ordinary_birth_observation_workspace &work) noexcept
{
	std::vector<uint8_t> left_encoded;
	std::vector<uint8_t> right_encoded;
	size_t live = work.live;
	if (!room_storage_add(live, 2 * sizeof(std::vector<uint8_t>)) ||
	    !room_storage_add(live, 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) ||
	    !ordinary_birth_observation_workspace::relay(live, &work))
		return false;
	if (critical_command_encode_bounded(left, &left_encoded,
					    ordinary_birth_observation_workspace::relay, &work,
					    live) != critical_command_codec_result::ok ||
	    !room_storage_add(live, left_encoded.capacity()))
		return false;
	return critical_command_encode_bounded(right, &right_encoded,
					       ordinary_birth_observation_workspace::relay, &work,
					       live) == critical_command_codec_result::ok &&
	       left_encoded == right_encoded;
}
} // namespace: ordinary birth full bounded observations

bool critical_native_mobile_birth_publication_owner::observe_generation_bounded(
	const critical_native_recovery_envelope &expected, uint64_t *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	size_t *current_coordinator_bytes) noexcept
{
	if (!output || !reserve || !current_coordinator_bytes ||
	    !ordinary_birth_observation_command(expected.command))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	ordinary_birth_observation_workspace work{ reserve, context, outer };
	try
	{
		work.lock.lock();
		// Assignment owns an actual return string beside the stored key; the
		// supported binary identity requests 17 chars, before construction.
		if (!work.admit(sizeof(std::string) + expected.command.operation_id.bytes.size() +
				1))
			return false;
		work.identity = operation_key(expected.command.operation_id);
		if (!work.admit())
			return false;
		work.operation = operations.find(work.identity);
		// Complete original typed-birth generation observation. No receipt,
		// native-phase, uncertainty or readiness gate is invented here.
		if (work.operation == operations.end() ||
		    !work.operation->second->retain_until_publication ||
		    !native_birth_submit_matches_bounded(
			    *work.operation->second, expected,
			    ordinary_birth_observation_workspace::relay, &work, work.live) ||
		    !health.initialized || !coordinator_generation ||
		    coordinator_generation_exhausted || stop_requested ||
		    work.operation->second->publication_checkpointing ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !work.admit())
			return false;
		*output = coordinator_generation;
		*current_coordinator_bytes = work.current;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool critical_native_mobile_birth_publication_owner::completion_bounded(
	const critical_operation_id &operation_id, critical_completion *completion,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	size_t *current_coordinator_bytes) noexcept
{
	if (!reserve || !completion || !current_coordinator_bytes ||
	    critical_operation_id_is_zero(operation_id))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	ordinary_birth_observation_workspace work{ reserve, context, outer };
	try
	{
		work.lock.lock();
		if (!work.admit(sizeof(std::string) + operation_id.bytes.size() + 1))
			return false;
		work.identity = operation_key(operation_id);
		if (!work.admit())
			return false;
		work.operation = operations.find(work.identity);
		// Original pending-publication-first, completed-cache-second lookup.
		// Family selection is proved from the genuine retained/cache command;
		// the operation ID creates no native/world/receipt or ACK authority.
		if (work.operation != operations.end() &&
		    operation_is_publication_pending(*work.operation->second))
		{
			if (!ordinary_birth_observation_command(work.operation->second->command))
				return false;
			*completion = work.operation->second->publication_completion;
			*current_coordinator_bytes = work.current;
			return true;
		}
		work.completed = completed_cache.find(work.identity);
		if (work.completed == completed_cache.end() ||
		    !ordinary_birth_observation_command(work.completed->second.command))
			return false;
		*completion = work.completed->second.completion;
		*current_coordinator_bytes = work.current;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool critical_native_mobile_birth_publication_owner::copy_context_ordinary_bounded(
	const critical_command &command, critical_native_recovery_envelope *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	size_t *current_coordinator_bytes) noexcept
{
	if (!output || !reserve || !current_coordinator_bytes ||
	    !ordinary_birth_observation_command(command))
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	ordinary_birth_observation_workspace work{ reserve, context, outer };
	try
	{
		work.lock.lock();
		if (!work.admit(sizeof(std::string) + command.operation_id.bytes.size() + 1))
			return false;
		work.identity = operation_key(command.operation_id);
		if (!work.admit())
			return false;
		work.operation = operations.find(work.identity);
		// Original native_context_copy predicates and the actual phase1/phase2
		// union. No generation, retain, lifecycle or validator gate is added.
		if (!health.initialized || stop_requested || work.operation == operations.end() ||
		    !work.operation->second->native ||
		    work.operation->second->publication_checkpointing ||
		    work.operation->second->native_context_uncertain ||
		    work.operation->second->native_ack_uncertain ||
		    !ordinary_birth_observation_equal(command, work.operation->second->command,
						      work))
			return false;
		work.state = work.operation->second.get();
		work.phase = work.state->native->phase;
		if (work.phase == critical_native_recovery_phase::execution_pending ?
			    !operation_is_publication_pending(*work.state) :
			    (work.phase != critical_native_recovery_phase::continuation_pending ||
			     work.state->phase !=
				     critical_operation_phase::native_continuation_pending ||
			     !work.state->native_physical_released))
			return false;
		// Full original native_envelope field copy from genuine current vectors.
		// Admit each real fresh request before allocation, then census actual
		// capacities. Old caller output stays live in outer until final success.
		work.extra = 0;
		if (work.state->command.keys.size() > SIZE_MAX / sizeof(critical_entity_key) ||
		    work.state->command.expected_revisions.size() >
			    SIZE_MAX / sizeof(critical_expected_revision) ||
		    !room_storage_add(work.extra, work.state->command.keys.size() *
							  sizeof(critical_entity_key)) ||
		    !room_storage_add(work.extra, work.state->command.expected_revisions.size() *
							  sizeof(critical_expected_revision)) ||
		    !room_storage_add(work.extra, work.state->command.payload.size()) ||
		    !room_storage_add(work.extra, work.state->command.accounting_intent.size()) ||
		    !room_storage_add(work.extra, work.state->native->attachment.size()) ||
		    !work.admit(work.extra))
			return false;
		work.copy.command = work.state->command;
		work.copy.revision = work.state->native->revision;
		work.copy.phase = work.phase;
		work.copy.attachment = work.state->native->attachment;
		if (!work.admit())
			return false;
		*output = std::move(work.copy);
		*current_coordinator_bytes = work.current;
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

namespace
{
// Full ordinary NMB4 publication, distinct from historical/shared and ROOM.
// Every allocating request below is lent fresh CURRENT coordinator storage
// under its genuine mutex and the registered ROOT callback/common guard.
bool ordinary_birth_publication_command(const critical_command &command) noexcept
{
	return native_birth_typed_command(command) &&
	       command.payload_version == NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION &&
	       !native_birth_shared_shop_command(command);
}

// Conservative full new helper source-carrier bundle. Summing all named
// allocation-free policy/helper frames retains every overlapping caller frame
// across nested callbacks; this is source accounting, not a native stack profile.
constexpr size_t ordinary_birth_publication_helper_source_frames =
	sizeof(void *) + sizeof(uint8_t) + sizeof(void *) + sizeof(uint8_t) + sizeof(void *) +
	sizeof(uint8_t) + sizeof(void *) + sizeof(bool) + sizeof(void *) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(bool) + sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) +
	sizeof(bool) + sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(void *) +
	sizeof(bool) + sizeof(void *) + sizeof(bool) + 5 * sizeof(void *) +
	2 * sizeof(std::vector<native_mobile_birth_recovery_item>::const_iterator) +
	2 * sizeof(std::vector<native_mobile_birth_recovery_effect>::const_iterator) +
	sizeof(bool) + 2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(bool) +
	2 * sizeof(void *) + 2 * sizeof(native_mobile_birth_recovery_action) + sizeof(bool) +
	4 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + 4 * sizeof(void *) +
	2 * sizeof(std::vector<native_mobile_birth_recovery_item>::const_iterator) + sizeof(bool) +
	3 * sizeof(void *) + sizeof(bool) + 3 * sizeof(void *) + sizeof(bool) +
	sizeof(critical_command_codec_result) + 3 * sizeof(void *) + sizeof(bool) +
	3 * sizeof(void *) + sizeof(bool) + sizeof(critical_command_codec_result) +
	3 * sizeof(void *) + sizeof(bool) + sizeof(economic_accounting_error) + 3 * sizeof(void *) +
	sizeof(bool) + 2 * sizeof(void *) + sizeof(bool) + 3 * sizeof(void *) + sizeof(bool) +
	sizeof(void *) + sizeof(bool) + sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(bool) + sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(bool) +
	2 * sizeof(std::array<char, 9>) + sizeof(std::string_view);

struct ordinary_birth_publication_workspace
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, provider_frames, live = 0, current = 0, extra = 0, prospective = 0;
	size_t original_retained = 0, successor_retained = 0, reserved = 0;
	uint64_t generation = 0;
	bool prior_uncertain = false, cleaned = false, same = false;
	std::unique_lock<std::mutex> lock{ coordinator_mutex, std::defer_lock };
	std::string identity = {};
	critical_native_recovery_envelope frozen = {}, prepared = {};
	critical_completion receipt{};
	native_mobile_birth_recovery_context before = {}, after = {};
	std::span<const uint8_t> attachment = {};
	decltype(operations)::iterator found = {};
	operation_state *pinned = nullptr;
	critical_command_journal_result result = critical_command_journal_result::io_failure;

	static bool context_heap(const native_mobile_birth_recovery_context &value,
				 size_t &bytes) noexcept
	{
		if (!room_storage_array(bytes, value.items.capacity(),
					sizeof(native_mobile_birth_recovery_item)))
			return false;
		for (const auto &item : value.items)
			if (!room_storage_array(bytes, item.effects.capacity(),
						sizeof(native_mobile_birth_recovery_effect)))
				return false;
		return true;
	}
	static bool relay(size_t exclusive_live, void *opaque) noexcept
	{
		auto &work = *static_cast<ordinary_birth_publication_workspace *>(opaque);
		return critical_room_shared_budget_lender::reserve(
			work.lock, work.reserve, work.context, exclusive_live, &work.current);
	}
	static bool outside_relay(size_t exclusive_live, void *opaque) noexcept
	{
		auto &work = *static_cast<ordinary_birth_publication_workspace *>(opaque);
		// The native callback is outside the mutex. Each nested budget request
		// acquires the REAL mutex solely for its fresh same-lock census/lender,
		// then releases it before returning to the native owner. No stored
		// current snapshot, thread flag or lockless zero acts as a loan.
		size_t full = exclusive_live;
		if (!room_storage_add(full, sizeof(std::unique_lock<std::mutex>)) ||
		    !room_storage_add(full, 2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)))
			return false;
		try
		{
			std::unique_lock<std::mutex> actual_lock(coordinator_mutex);
			return critical_room_shared_budget_lender::reserve(
				actual_lock, work.reserve, work.context, full, &work.current);
		}
		catch (...)
		{
			return false;
		}
	}
	bool admit(size_t request = 0) noexcept
	{
		live = outer;
		// Actual complete workspace and provider argument/return carriers. The
		// admit and relay frames own their this/reference/opaque pointers,
		// two size parameters and two bool return objects separately. Full
		// underlying native/library/census profiles remain caller-owned.
		if (!room_storage_add(live, sizeof(*this)) ||
		    !room_storage_add(live, provider_frames) ||
		    !room_storage_add(live, ordinary_birth_publication_helper_source_frames) ||
		    !room_storage_add(live,
				      3 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool)) ||
		    !room_storage_string(live, identity) ||
		    !room_checkpoint_heap(frozen, false, live) ||
		    !room_checkpoint_heap(prepared, false, live) || !context_heap(before, live) ||
		    !context_heap(after, live) || !room_storage_add(live, request))
			return false;
		return relay(live, this);
	}
	bool clone(const critical_native_recovery_envelope &source,
		   critical_native_recovery_envelope &target) noexcept
	{
		// GCC13 vector copies request their full source size. The complete
		// old target capacity and any other live clone remain in admit().
		prospective = 3 * sizeof(void *) + sizeof(bool);
		if (!room_checkpoint_heap(source, true, prospective) || !admit(prospective))
			return false;
		try
		{
			target = source;
			return admit(3 * sizeof(void *) + sizeof(bool));
		}
		catch (...)
		{
			return false;
		}
	}
};

bool ordinary_birth_publication_equal(const critical_command &left, const critical_command &right,
				      ordinary_birth_publication_workspace &work) noexcept
{
	struct equal_workspace
	{
		std::vector<uint8_t> left, right;
		size_t live = 0;
	} proof;
	proof.live = work.live;
	if (!room_storage_add(proof.live, sizeof(proof)) ||
	    !room_storage_add(proof.live, 3 * sizeof(void *) + sizeof(bool)) ||
	    !ordinary_birth_publication_workspace::relay(proof.live, &work))
		return false;
	if (critical_command_encode_bounded(left, &proof.left,
					    ordinary_birth_publication_workspace::relay, &work,
					    proof.live) != critical_command_codec_result::ok ||
	    !room_storage_add(proof.live, proof.left.capacity()))
		return false;
	return critical_command_encode_bounded(right, &proof.right,
					       ordinary_birth_publication_workspace::relay, &work,
					       proof.live) == critical_command_codec_result::ok &&
	       proof.left == proof.right;
}

bool ordinary_birth_publication_matches(const operation_state &state,
					const critical_native_recovery_envelope &expected,
					ordinary_birth_publication_workspace &work) noexcept
{
	return state.native && state.native->revision == expected.revision &&
	       state.native->phase == expected.phase &&
	       state.native->attachment == expected.attachment &&
	       work.admit(3 * sizeof(void *) + sizeof(bool)) &&
	       ordinary_birth_publication_equal(state.command, expected.command, work);
}

bool ordinary_birth_publication_size(const critical_native_recovery_envelope &envelope,
				     size_t *retained,
				     ordinary_birth_publication_workspace &work) noexcept
{
	// Complete native_envelope_size domain and canonical hard-size policy.
	if (!retained || !native_transport_command(envelope.command) ||
	    (held_retirement_transport_command(envelope.command) &&
	     envelope.phase != critical_native_recovery_phase::execution_pending) ||
	    !envelope.command.publication_required || !envelope.revision ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     envelope.phase != critical_native_recovery_phase::continuation_pending) ||
	    envelope.attachment.empty() ||
	    envelope.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES)
		return false;
	struct size_workspace
	{
		std::vector<uint8_t> encoded;
		size_t live = 0;
	} proof;
	proof.live = work.live;
	if (!room_storage_add(proof.live, sizeof(proof)) ||
	    !room_storage_add(proof.live, 3 * sizeof(void *) + sizeof(bool)) ||
	    !ordinary_birth_publication_workspace::relay(proof.live, &work) ||
	    critical_command_encode_bounded(envelope.command, &proof.encoded,
					    ordinary_birth_publication_workspace::relay, &work,
					    proof.live) != critical_command_codec_result::ok ||
	    proof.encoded.size() >
		    CRITICAL_COORDINATOR_MAX_BYTES - NATIVE_COORDINATOR_ENVELOPE_OVERHEAD ||
	    envelope.attachment.size() > CRITICAL_COORDINATOR_MAX_BYTES -
						 NATIVE_COORDINATOR_ENVELOPE_OVERHEAD -
						 proof.encoded.size())
		return false;
	*retained = proof.encoded.size() + envelope.attachment.size() +
		    NATIVE_COORDINATOR_ENVELOPE_OVERHEAD;
	return true;
}

uint8_t ordinary_publication_bits(const native_mobile_birth_recovery_action &a) noexcept
{
	return static_cast<uint8_t>((a.started ? 1 : 0) | (a.returned ? 2 : 0) |
				    (a.succeeded ? 4 : 0));
}

uint8_t ordinary_publication_bits(const native_mobile_birth_recovery_mobile &a) noexcept
{
	return static_cast<uint8_t>((a.started ? 1 : 0) | (a.returned ? 2 : 0) |
				    (a.consumed ? 4 : 0));
}

uint8_t ordinary_publication_bits(const native_mobile_birth_recovery_effect &a) noexcept
{
	return static_cast<uint8_t>((a.started ? 1 : 0) | (a.returned ? 2 : 0) |
				    (a.succeeded ? 4 : 0) | (a.periodic ? 8 : 0));
}

bool ordinary_publication_complete(const native_mobile_birth_recovery_action &a) noexcept
{
	return a.started && a.returned && a.succeeded;
}

bool ordinary_publication_complete(const native_mobile_birth_recovery_mobile &a) noexcept
{
	return a.started && a.returned && a.consumed;
}

bool ordinary_publication_completion_equal(const critical_completion &a,
					   const critical_completion &b) noexcept
{
	return a.operation_id.bytes == b.operation_id.bytes && a.outcome == b.outcome &&
	       a.durable_revision == b.durable_revision && a.error_code == b.error_code &&
	       a.attempt == b.attempt && a.queued_at_usec == b.queued_at_usec &&
	       a.started_at_usec == b.started_at_usec &&
	       a.completed_at_usec == b.completed_at_usec && a.failure_stage == b.failure_stage &&
	       a.result_size == b.result_size && a.result_payload == b.result_payload &&
	       a.recovery_correlation == b.recovery_correlation && a.disposition == b.disposition;
}

bool ordinary_publication_successful(const critical_completion &receipt) noexcept
{
	return receipt.outcome == critical_apply_outcome::applied ||
	       receipt.outcome == critical_apply_outcome::already_applied;
}

bool ordinary_publication_receipt_core_equal(const critical_completion &a,
					     const critical_completion &b) noexcept
{
	return ordinary_publication_successful(a) && ordinary_publication_successful(b) &&
	       a.operation_id.bytes == b.operation_id.bytes &&
	       a.durable_revision == b.durable_revision && a.error_code == b.error_code &&
	       a.failure_stage == b.failure_stage && a.result_size == b.result_size &&
	       a.result_payload == b.result_payload &&
	       a.disposition == critical_completion_disposition::execution &&
	       b.disposition == critical_completion_disposition::execution;
}

bool ordinary_publication_item_done_fields(const native_mobile_birth_recovery_item &item,
					   size_t effect_count) noexcept
{
	return item.admitted && item.published && ordinary_publication_complete(item.publication) &&
	       ordinary_publication_complete(item.enrollment) && !item.current_step_started &&
	       item.next_step == effect_count;
}

bool ordinary_publication_item_done(const native_mobile_birth_recovery_item &item) noexcept
{
	return ordinary_publication_item_done_fields(item, item.effects.size());
}

bool ordinary_publication_body_terminal(const native_mobile_birth_recovery_context &value) noexcept
{
	return value.receipt_present && ordinary_publication_successful(value.receipt) &&
	       value.stage == native_mobile_birth_recovery_stage::physically_proven &&
	       ordinary_publication_complete(value.whole_binding) &&
	       ordinary_publication_complete(value.reference_install) &&
	       ordinary_publication_complete(value.mobile_publication) && value.runtime_applied &&
	       std::all_of(value.items.begin(), value.items.end(), ordinary_publication_item_done);
}

bool ordinary_publication_no_progress(const native_mobile_birth_recovery_context &value) noexcept
{
	if (value.stage != native_mobile_birth_recovery_stage::captured ||
	    ordinary_publication_bits(value.whole_binding) ||
	    ordinary_publication_bits(value.reference_install) ||
	    ordinary_publication_bits(value.mobile_publication) || value.runtime_applied)
		return false;
	for (const auto &e : value.mobile_effects)
		if (ordinary_publication_bits(e))
			return false;
	for (const auto &choice : value.mobile_choices)
		if (choice.chosen || choice.requested || choice.delay)
			return false;
	for (const auto &item : value.items)
	{
		if (item.next_step || item.current_step_started || item.admitted ||
		    item.published || ordinary_publication_bits(item.publication) ||
		    ordinary_publication_bits(item.enrollment))
			return false;
		for (const auto &e : item.effects)
			if (ordinary_publication_bits(e))
				return false;
	}
	return true;
}

bool ordinary_publication_successor_action(const native_mobile_birth_recovery_action &a,
					   const native_mobile_birth_recovery_action &b) noexcept
{
	if (a == b)
		return true;
	if (!a.started)
		return b.started && !b.returned && !b.succeeded;
	if (!a.returned)
		return b.started && b.returned;
	return false;
}

bool ordinary_publication_successor_mobile(const native_mobile_birth_recovery_mobile &a,
					   const native_mobile_birth_recovery_mobile &b) noexcept
{
	if (a == b)
		return true;
	if (!a.started)
		return b.started && !b.returned && !b.consumed;
	if (a.returned || (a.consumed && !b.consumed) || !b.started)
		return false;
	return !b.returned || b.consumed == a.consumed;
}

bool ordinary_publication_successor_effect(const native_mobile_birth_recovery_effect &a,
					   const native_mobile_birth_recovery_effect &b) noexcept
{
	if (a == b)
		return true;
	return ordinary_publication_successor_action({ a.started, a.returned, a.succeeded },
						     { b.started, b.returned, b.succeeded }) &&
	       (!a.returned || a.periodic == b.periodic);
}

bool ordinary_publication_successor_context(const native_mobile_birth_recovery_context &a,
					    const native_mobile_birth_recovery_context &b) noexcept
{
	if ((a.receipt_present && !b.receipt_present) ||
	    (a.receipt_present && !ordinary_publication_completion_equal(a.receipt, b.receipt) &&
	     !ordinary_publication_receipt_core_equal(a.receipt, b.receipt)) ||
	    b.stage < a.stage ||
	    static_cast<unsigned>(b.stage) > static_cast<unsigned>(a.stage) + 1 ||
	    (a.runtime_applied && !b.runtime_applied) ||
	    !ordinary_publication_successor_action(a.whole_binding, b.whole_binding) ||
	    !ordinary_publication_successor_action(a.reference_install, b.reference_install) ||
	    !ordinary_publication_successor_mobile(a.mobile_publication, b.mobile_publication) ||
	    a.items.size() != b.items.size())
		return false;
	for (size_t i = 0; i < a.mobile_effects.size(); ++i)
		if (!ordinary_publication_successor_effect(a.mobile_effects[i],
							   b.mobile_effects[i]))
			return false;
	for (size_t i = 0; i < a.mobile_choices.size(); ++i)
		if (a.mobile_choices[i].chosen && a.mobile_choices[i] != b.mobile_choices[i])
			return false;
	for (size_t i = 0; i < a.items.size(); ++i)
	{
		const auto &left = a.items[i];
		const auto &right = b.items[i];
		if (left.object_uid != right.object_uid || right.next_step < left.next_step ||
		    right.next_step > static_cast<uint64_t>(left.next_step) + 1 ||
		    (left.admitted && !right.admitted) || (left.published && !right.published) ||
		    !ordinary_publication_successor_action(left.publication, right.publication) ||
		    !ordinary_publication_successor_action(left.enrollment, right.enrollment) ||
		    left.effects.size() != right.effects.size())
			return false;
		for (size_t step = 0; step < left.effects.size(); ++step)
			if (!ordinary_publication_successor_effect(left.effects[step],
								   right.effects[step]))
				return false;
		if (left.current_step_started && !right.current_step_started &&
		    right.next_step != left.next_step + 1)
			return false;
	}
	return true;
}

bool ordinary_birth_publication_decode(const critical_native_recovery_envelope &envelope,
				       native_mobile_birth_recovery_context &output,
				       ordinary_birth_publication_workspace &work) noexcept
{
	if (!envelope.revision ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     envelope.phase != critical_native_recovery_phase::continuation_pending) ||
	    !work.admit(3 * sizeof(void *) + sizeof(bool)))
		return false;
	work.attachment = envelope.attachment;
	return native_mobile_birth_cash_role_recovery_decode_status_bounded(
		       envelope.command, work.attachment, &output,
		       ordinary_birth_publication_workspace::relay, &work,
		       work.live) == economic_accounting_error::ok &&
	       (envelope.revision != 1 ||
		(envelope.phase == critical_native_recovery_phase::execution_pending &&
		 !output.receipt_present && ordinary_publication_no_progress(output))) &&
	       (envelope.phase == critical_native_recovery_phase::execution_pending ||
		ordinary_publication_body_terminal(output)) &&
	       work.admit(3 * sizeof(void *) + sizeof(bool));
}

bool ordinary_birth_publication_successor(const critical_native_recovery_envelope &expected,
					  const critical_native_recovery_envelope &successor,
					  ordinary_birth_publication_workspace &work) noexcept
{
	if (expected.revision == UINT64_MAX || successor.revision != expected.revision + 1 ||
	    !work.admit(3 * sizeof(void *) + sizeof(bool)) ||
	    !ordinary_birth_publication_equal(expected.command, successor.command, work) ||
	    !ordinary_birth_publication_decode(expected, work.before, work) ||
	    !ordinary_birth_publication_decode(successor, work.after, work))
		return false;
	if (expected.phase == critical_native_recovery_phase::continuation_pending ||
	    successor.phase == critical_native_recovery_phase::continuation_pending)
		return successor.phase == critical_native_recovery_phase::continuation_pending &&
		       expected.attachment == successor.attachment &&
		       ordinary_publication_body_terminal(work.after);
	return ordinary_publication_successor_context(work.before, work.after);
}

bool ordinary_birth_publication_terminal(const critical_native_recovery_envelope &expected,
					 ordinary_birth_publication_workspace &work) noexcept
{
	return expected.revision > 1 &&
	       expected.phase == critical_native_recovery_phase::continuation_pending &&
	       work.admit(2 * sizeof(void *) + sizeof(bool)) &&
	       ordinary_birth_publication_decode(expected, work.before, work) &&
	       ordinary_publication_body_terminal(work.before);
}

bool ordinary_birth_publication_receipt(const critical_native_recovery_envelope &expected,
					const critical_completion &receipt,
					ordinary_birth_publication_workspace &work) noexcept
{
	return work.admit(3 * sizeof(void *) + sizeof(bool)) &&
	       ordinary_birth_publication_decode(expected, work.before, work) &&
	       ordinary_publication_body_terminal(work.before) &&
	       ordinary_publication_receipt_core_equal(work.before.receipt, receipt);
}

bool ordinary_birth_publication_charge(ordinary_birth_publication_workspace &work) noexcept
{
	// Exactly the original native_context_checkpoint charge policy. No ROOM
	// role predicate or partial transaction stand-in establishes identity.
	if (work.found->second->ordinary_flat_transaction)
	{
		work.extra = work.found->second->ordinary_flat_transaction_bytes;
		if (!work.extra || work.successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
		    work.extra > CRITICAL_COORDINATOR_MAX_BYTES - work.successor_retained)
			return false;
		work.original_retained = work.found->second->retained_bytes;
		work.successor_retained += work.extra;
	}
	if (work.found->second->flat_transaction)
	{
		work.extra = work.found->second->flat_transaction_bytes;
		if (!work.extra || work.successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
		    work.extra > CRITICAL_COORDINATOR_MAX_BYTES - work.successor_retained)
			return false;
		work.original_retained = work.found->second->retained_bytes;
		work.successor_retained += work.extra;
	}
	if (work.found->second->room_flat_transaction)
	{
		work.extra = work.found->second->room_flat_transaction_bytes;
		if (!work.extra || work.successor_retained > CRITICAL_COORDINATOR_MAX_BYTES ||
		    work.extra > CRITICAL_COORDINATOR_MAX_BYTES - work.successor_retained)
			return false;
		work.original_retained = work.found->second->retained_bytes;
		work.successor_retained += work.extra;
	}
	work.reserved = std::max(work.found->second->retained_bytes, work.successor_retained);
	return work.reserved <=
	       CRITICAL_COORDINATOR_MAX_BYTES -
		       (health.retained_bytes - work.found->second->retained_bytes);
}

void ordinary_birth_publication_pin(ordinary_birth_publication_workspace &work) noexcept
{
	work.pinned = work.found->second.get();
	work.pinned->publication_checkpointing = true;
	++publication_checkpoints_inflight;
	++guarded_publications_inflight;
}

void ordinary_birth_publication_unpin(ordinary_birth_publication_workspace &work) noexcept
{
	--guarded_publications_inflight;
	--publication_checkpoints_inflight;
	publication_checkpoint_finished.notify_all();
	work.found = operations.find(work.identity);
}

bool ordinary_birth_publication_pinned(const ordinary_birth_publication_workspace &work) noexcept
{
	return work.found != operations.end() && coordinator_generation == work.generation &&
	       work.found->second.get() == work.pinned &&
	       work.found->second->publication_checkpointing && work.found->second->native &&
	       work.found->second->native->revision == work.frozen.revision &&
	       work.found->second->native->phase == work.frozen.phase &&
	       work.found->second->native->attachment == work.frozen.attachment;
}

void ordinary_birth_publication_journal(ordinary_birth_publication_workspace &work,
					bool retire) noexcept
{
	try
	{
		// Genuine coor->journal order only for the complete bounded provider.
		// Native transfer/cleanup callbacks remain outside coordinator ownership.
		work.lock.lock();
		if (!work.admit(2 * sizeof(void *) + sizeof(bool) +
				sizeof(std::lock_guard<std::mutex>)))
			work.result = critical_command_journal_result::quota_exceeded;
		else if (retire)
			work.result = critical_command_journal_retire_native_recovery_bounded(
				work.frozen, ordinary_birth_publication_workspace::relay, &work,
				work.live);
		else
			work.result = critical_command_journal_replace_native_recovery_bounded(
				work.frozen, work.prepared,
				ordinary_birth_publication_workspace::relay, &work, work.live);
		work.lock.unlock();
	}
	catch (...)
	{
		// The pinned operation must reach its original uncertainty/unpin tail
		// even when the new journal ownership acquisition fails.
		if (work.lock.owns_lock())
			work.lock.unlock();
	}
}
} // namespace: full bounded ordinary publication helpers

bool critical_native_mobile_birth_publication_owner::checkpoint_context_ordinary_bounded(
	const critical_native_recovery_envelope &expected,
	const critical_native_recovery_envelope &successor,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	uint64_t expected_generation) noexcept
{
	if (!reserve || !ordinary_birth_publication_command(expected.command) ||
	    successor.phase != expected.phase || expected.revision == UINT64_MAX ||
	    successor.revision != expected.revision + 1)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	ordinary_birth_publication_workspace work{ reserve, context, outer,
						   4 * sizeof(void *) + sizeof(size_t) +
							   sizeof(uint64_t) + sizeof(bool) };
	try
	{
		work.lock.lock();
		if (!work.admit(sizeof(std::string) + expected.command.operation_id.bytes.size() +
				1) ||
		    !ordinary_birth_publication_size(expected, &work.original_retained, work) ||
		    !ordinary_birth_publication_equal(expected.command, successor.command, work) ||
		    !ordinary_birth_publication_size(successor, &work.successor_retained, work) ||
		    !work.clone(expected, work.frozen) || !work.clone(successor, work.prepared))
			return false;
		if (!work.admit(sizeof(std::string) +
				work.frozen.command.operation_id.bytes.size() + 1))
			return false;
		work.identity = operation_key(work.frozen.command.operation_id);
		if (!work.admit())
			return false;
		work.found = operations.find(work.identity);
		if (!health.initialized || stop_requested || work.found == operations.end() ||
		    !ordinary_birth_publication_matches(*work.found->second, work.frozen, work) ||
		    work.found->second->publication_checkpointing ||
		    work.found->second->native_ack_uncertain || !coordinator_generation ||
		    coordinator_generation_exhausted ||
		    (expected_generation && coordinator_generation != expected_generation) ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    (work.frozen.phase == critical_native_recovery_phase::execution_pending ?
			     !operation_is_publication_pending(*work.found->second) :
			     (work.found->second->phase !=
				      critical_operation_phase::native_continuation_pending ||
			      !work.found->second->native_physical_released)) ||
		    !native_birth_validators_ready() ||
		    !ordinary_birth_publication_decode(work.frozen, work.before, work) ||
		    !ordinary_birth_publication_successor(work.frozen, work.prepared, work) ||
		    !ordinary_birth_publication_charge(work) ||
		    !work.admit(sizeof(std::array<char, 9>) * 2 + sizeof(std::string_view)))
			return false;
		work.prior_uncertain = work.found->second->native_context_uncertain;
		work.found->second->retained_bytes = work.reserved;
		work.generation = coordinator_generation;
		ordinary_birth_publication_pin(work);
		update_depth();
		work.lock.unlock();
		ordinary_birth_publication_journal(work, false);
		work.lock.lock();
		ordinary_birth_publication_unpin(work);
		if (!ordinary_birth_publication_pinned(work))
			return false;
		work.found->second->publication_checkpointing = false;
		if (work.result != critical_command_journal_result::ok)
		{
			work.found->second->native_context_uncertain =
				work.prior_uncertain ||
				work.result == critical_command_journal_result::append_uncertain;
			if (!work.found->second->native_context_uncertain)
				work.found->second->retained_bytes = work.original_retained;
			update_depth();
			return false;
		}
		// Original scalar/body move only: no callback, codec or allocation after CAS.
		work.found->second->native->revision = work.prepared.revision;
		work.found->second->native->attachment = std::move(work.prepared.attachment);
		work.found->second->retained_bytes = work.successor_retained;
		work.found->second->native_context_uncertain = false;
		update_depth();
		work_available.notify_all();
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool critical_native_mobile_birth_publication_owner::acknowledge_ordinary_bounded(
	const critical_native_recovery_envelope &expected, const critical_completion &receipt,
	uint64_t generation, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer) noexcept
{
	if (!reserve || !generation || !ordinary_birth_publication_command(expected.command) ||
	    expected.phase != critical_native_recovery_phase::execution_pending ||
	    expected.revision == UINT64_MAX ||
	    receipt.operation_id.bytes != expected.command.operation_id.bytes ||
	    (receipt.outcome != critical_apply_outcome::applied &&
	     receipt.outcome != critical_apply_outcome::already_applied) ||
	    receipt.disposition != critical_completion_disposition::execution ||
	    receipt.durable_revision != 1 || receipt.error_code ||
	    receipt.failure_stage != critical_failure_stage::none ||
	    receipt.result_size != NATIVE_MOBILE_BIRTH_CASH_ROLE_RESULT_BYTES)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	ordinary_birth_publication_workspace work{ reserve, context, outer,
						   4 * sizeof(void *) + sizeof(size_t) +
							   sizeof(uint64_t) + sizeof(bool) };
	try
	{
		work.lock.lock();
		if (!work.admit(sizeof(std::string) + expected.command.operation_id.bytes.size() +
				1) ||
		    !work.clone(expected, work.frozen) || !work.clone(work.frozen, work.prepared))
			return false;
		++work.prepared.revision;
		work.prepared.phase = critical_native_recovery_phase::continuation_pending;
		work.receipt = receipt;
		work.generation = generation;
		if (!work.admit(sizeof(std::string) +
				work.frozen.command.operation_id.bytes.size() + 1))
			return false;
		work.identity = operation_key(work.frozen.command.operation_id);
		if (!work.admit())
			return false;
		work.found = operations.find(work.identity);
		if (work.found == operations.end() || !health.initialized || stop_requested ||
		    coordinator_generation != generation || coordinator_generation_exhausted ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !operation_is_publication_pending(*work.found->second) ||
		    !work.found->second->retain_until_publication ||
		    work.found->second->publication_checkpointing ||
		    work.found->second->native_context_uncertain ||
		    !ordinary_birth_publication_matches(*work.found->second, work.frozen, work) ||
		    !native_receipt_equal(work.found->second->publication_completion,
					  work.receipt) ||
		    !native_birth_validators_ready() ||
		    !ordinary_birth_publication_receipt(work.frozen, work.receipt, work) ||
		    !ordinary_birth_publication_successor(work.frozen, work.prepared, work) ||
		    !work.admit(2 * sizeof(std::array<char, 9>) + sizeof(std::string_view)))
			return false;
		work.prior_uncertain = work.found->second->native_ack_uncertain;
		ordinary_birth_publication_pin(work);
		work.lock.unlock();
		ordinary_birth_publication_journal(work, false);
		work.lock.lock();
		ordinary_birth_publication_unpin(work);
		if (!ordinary_birth_publication_pinned(work) ||
		    !operation_is_publication_pending(*work.found->second))
			return false;
		work.found->second->publication_checkpointing = false;
		if (work.result != critical_command_journal_result::ok)
		{
			work.found->second->native_ack_uncertain =
				work.prior_uncertain ||
				work.result == critical_command_journal_result::append_uncertain;
			return false;
		}
		work.found->second->native->revision = work.prepared.revision;
		work.found->second->native->phase = work.prepared.phase;
		work.found->second->phase = critical_operation_phase::native_continuation_pending;
		work.found->second->native_ack_uncertain = false;
		work.found->second->native_physical_released = true;
		// Genuine NMB4 constructor fences survive physical ACK. No command cache
		// or save hold can replace its later original origin transfer/retirement.
		update_depth();
		work_available.notify_all();
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

bool critical_native_mobile_birth_publication_owner::retire_ordinary_bounded(
	const critical_native_recovery_envelope &expected, uint64_t expected_generation,
	bool (*durable_transfer)(const critical_native_recovery_envelope &, void *,
				 bool (*)(size_t, void *) noexcept, void *, size_t) noexcept,
	void *transfer_context, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer) noexcept
{
	if (!reserve || !durable_transfer || !expected_generation ||
	    !ordinary_birth_publication_command(expected.command) ||
	    !native_birth_origin_command(expected.command) ||
	    expected.phase != critical_native_recovery_phase::continuation_pending)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	ordinary_birth_publication_workspace work{ reserve, context, outer,
						   5 * sizeof(void *) + sizeof(uint64_t) +
							   sizeof(size_t) + sizeof(bool) };
	try
	{
		work.lock.lock();
		if (!work.admit() ||
		    !ordinary_birth_publication_size(expected, &work.original_retained, work) ||
		    !work.clone(expected, work.frozen) ||
		    !work.admit(sizeof(std::string) +
				work.frozen.command.operation_id.bytes.size() + 1))
			return false;
		// Freeze complete caller BODY/command before transfer can release it.
		work.identity = operation_key(work.frozen.command.operation_id);
		if (!work.admit())
			return false;
		work.found = operations.find(work.identity);
		if (!health.initialized || stop_requested || work.found == operations.end() ||
		    !ordinary_birth_publication_matches(*work.found->second, work.frozen, work) ||
		    work.found->second->publication_checkpointing ||
		    work.found->second->native_ack_uncertain || !coordinator_generation ||
		    coordinator_generation_exhausted ||
		    coordinator_generation != expected_generation ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    work.found->second->phase !=
			    critical_operation_phase::native_continuation_pending ||
		    !work.found->second->native_physical_released ||
		    !native_birth_validators_ready() ||
		    !ordinary_birth_publication_decode(work.frozen, work.before, work) ||
		    !ordinary_birth_publication_terminal(work.frozen, work) ||
		    !ordinary_birth_publication_charge(work) ||
		    !work.admit(2 * sizeof(std::array<char, 9>) + sizeof(std::string_view)))
			return false;
		work.prior_uncertain = work.found->second->native_context_uncertain;
		work.found->second->retained_bytes = work.reserved;
		work.generation = coordinator_generation;
		ordinary_birth_publication_pin(work);
		update_depth();
		work.lock.unlock();
		// Mandatory genuine original terminal transfer is outside the mutex.
		// Lost/failed transfer retains original context and advancement fences.
		// The callback owns its full live prefix and fresh coordinator admission.
		if (durable_transfer(work.frozen, transfer_context,
				     ordinary_birth_publication_workspace::outside_relay, &work,
				     work.live))
			ordinary_birth_publication_journal(work, true);
		work.lock.lock();
		ordinary_birth_publication_unpin(work);
		if (!ordinary_birth_publication_pinned(work))
			return false;
		work.found->second->publication_checkpointing = false;
		if (work.result != critical_command_journal_result::ok)
		{
			work.found->second->native_context_uncertain =
				work.prior_uncertain ||
				work.result == critical_command_journal_result::append_uncertain;
			if (!work.found->second->native_context_uncertain)
				work.found->second->retained_bytes = work.original_retained;
			update_depth();
			return false;
		}
		// All fallible work precedes confirmed retirement. Original fence
		// release and operation destruction require no reserve/encode/callback.
		remove_fences(work.identity, work.found->second->command);
		operations.erase(work.found);
		++health.completed;
		update_depth();
		work_available.notify_all();
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

namespace
{
bool ordinary_birth_publication_refusal_matches(const operation_state &state,
						ordinary_birth_publication_workspace &work) noexcept
{
	return operation_is_admission_failed(state) && state.owned_refusal_delivered &&
	       state.retain_until_publication && state.native &&
	       state.native->revision == work.frozen.revision &&
	       state.native->phase == work.frozen.phase &&
	       state.native->attachment == work.frozen.attachment &&
	       native_receipt_equal(state.admission_failure_completion, work.receipt);
}
}

bool critical_native_mobile_birth_publication_owner::cancel_refusal_ordinary_bounded(
	const critical_native_recovery_envelope &original, const critical_completion &expected,
	uint64_t generation,
	bool (*native_cleanup)(const critical_command &, const critical_completion &, void *,
			       bool (*)(size_t, void *) noexcept, void *, size_t) noexcept,
	void *cleanup_context, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer, bool *cleanup_called, bool *cleanup_succeeded) noexcept
{
	if (cleanup_called)
		*cleanup_called = false;
	if (cleanup_succeeded)
		*cleanup_succeeded = false;
	if (!native_cleanup || !reserve || !cleanup_called || !cleanup_succeeded ||
	    cleanup_called == cleanup_succeeded || !generation ||
	    !ordinary_birth_publication_command(original.command) ||
	    expected.operation_id.bytes != original.command.operation_id.bytes ||
	    !critical_completion_disposition_valid(expected) ||
	    expected.disposition != critical_completion_disposition::never_admitted)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	ordinary_birth_publication_workspace work{ reserve, context, outer,
						   8 * sizeof(void *) + sizeof(uint64_t) +
							   sizeof(size_t) + sizeof(bool) };
	try
	{
		work.lock.lock();
		work.receipt = expected;
		work.generation = generation;
		if (!work.admit() || !work.clone(original, work.frozen) ||
		    !work.admit(sizeof(std::string) +
				work.frozen.command.operation_id.bytes.size() + 1))
			return false;
		work.identity = operation_key(work.frozen.command.operation_id);
		if (!work.admit())
			return false;
		work.found = operations.find(work.identity);
		if (work.found == operations.end() ||
		    work.found->second->publication_checkpointing || !health.initialized ||
		    coordinator_generation != generation || coordinator_generation_exhausted ||
		    stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !ordinary_birth_publication_refusal_matches(*work.found->second, work) ||
		    !ordinary_birth_publication_matches(*work.found->second, work.frozen, work) ||
		    !work.admit(2 * sizeof(std::array<char, 9>) + sizeof(std::string_view)))
			return false;
		ordinary_birth_publication_pin(work);
		work.lock.unlock();
		// Caller markers and both callback contexts survive genuine owner cleanup.
		// Record effects before later locking/rechecks; false must not replay them.
		*cleanup_called = true;
		work.cleaned = native_cleanup(work.frozen.command, work.receipt, cleanup_context,
					      ordinary_birth_publication_workspace::outside_relay,
					      &work, work.live);
		*cleanup_succeeded = work.cleaned;
		work.lock.lock();
		ordinary_birth_publication_unpin(work);
		if (work.found == operations.end() || coordinator_generation != generation ||
		    work.found->second.get() != work.pinned)
			return false;
		work.same = work.found->second->publication_checkpointing &&
			    ordinary_birth_publication_refusal_matches(*work.found->second, work);
		work.found->second->publication_checkpointing = false;
		if (!work.cleaned || !work.same || !health.initialized ||
		    coordinator_generation_exhausted || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()))
			return false;
		// Exact delivered never-admitted owner has no durable frame to retire.
		// Original cleanup tail performs no allocation, budget or codec callback.
		remove_fences(work.identity, work.found->second->command);
		operations.erase(work.found);
		update_depth();
		work_available.notify_all();
		return true;
	}
	catch (...)
	{
		return false;
	}
#endif
}

// Genuine unlocked caller bridge. Each nested request obtains CURRENT C under
// the actual mutex and the existing registered ROOT/common-guard lender, then
// releases the mutex before any caller effect. This is never a stored lease.
bool critical_native_mobile_birth_publication_owner::reserve_ordinary_bounded(
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t exclusive_live) noexcept
{
	if (!reserve)
		return false;
	size_t live = exclusive_live;
	if (!room_storage_add(live, sizeof(std::unique_lock<std::mutex>)) ||
	    !room_storage_add(live, 3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)))
		return false;
	try
	{
		std::unique_lock<std::mutex> actual_lock(coordinator_mutex);
		return critical_room_shared_budget_lender::reserve(actual_lock, reserve, context,
								   live);
	}
	catch (...)
	{
		return false;
	}
}

namespace
{
// Complete passive ordinary NMB4 replay insertion under the REAL init mutex.
// The actual startup selector/host must supply the original configured recovery
// root to its bounded passive birth observer. A warm factory source pin cannot
// be fabricated by replay. This companion grants no construction/publication,
// activation or release authority and remains unselected until that paired host.
[[maybe_unused]] bool
enqueue_ordinary_native_replayed_bounded(const critical_native_recovery_envelope &original,
					 const replay_observer_context &replay,
					 const std::unique_lock<std::mutex> &actual_init_lock,
					 bool (*reserve)(size_t, void *) noexcept,
					 void *budget_context, size_t exclusive_outer) noexcept
{
	if (!reserve || !replay.native_observer || !replay.native_bounded_observer ||
	    !ordinary_birth_publication_command(original.command) ||
	    actual_init_lock.mutex() != &coordinator_mutex || !actual_init_lock.owns_lock())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	room_replay_workspace work(actual_init_lock, reserve, budget_context, exclusive_outer);
	// Genuine still-live value carriers coexist with the real workspace through
	// each owning provider. Referred envelope/context/lock storage stays caller-owned.
	if (!room_storage_add(work.outer,
			      sizeof(reserve) + sizeof(budget_context) + sizeof(exclusive_outer) +
				      sizeof(const critical_native_recovery_envelope *) +
				      sizeof(const replay_observer_context *) +
				      sizeof(const std::unique_lock<std::mutex> *) + sizeof(bool)))
		return false;
	try
	{
		if (!room_replay_admit(work))
			return false;
		// The original ordinary route uses execution_supported, unlike shared
		// shop replay. Preserve its full legacy-or-accounting extension decision
		// using the genuine installed bounded companion for the allocating leaf.
		if (!room_replay_local_prefix(work) ||
		    !(critical_command_valid(original.command) ||
		      (original.command.schema_version ==
			       CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		       critical_command_envelope_valid(original.command) &&
		       extension_validator_callback && extension_validator_bounded_callback &&
		       extension_validator_bounded_callback(
			       original.command, room_replay_reserve_locked, &work, work.partial))))
			return false;
		if (!room_replay_admit(work) ||
		    !room_checkpoint_size_bounded(original, &work.retained,
						  room_replay_reserve_locked, &work, work.partial))
			return false;
		// Preserve the original typed-birth proof and all five registered
		// validator readiness requirements. Ordinary NMB4 selects the genuine
		// cash-role recovery codec, exactly as native_birth_recovery_valid.
		// Both execution and continuation retain the complete receipt/progress
		// proof. An INITIAL-only decoder cannot replace this original domain.
		if (!room_replay_admit(work) || !native_birth_validators_ready() ||
		    !native_mobile_birth_cash_role_recovery_valid_bounded(
			    original, room_replay_reserve_locked, &work, work.partial))
			return false;
		if (!room_replay_admit(work))
			return false;
		// Original operation_key return/local coexistence. Its actual 16-byte
		// GCC13 string requests 17 bytes; assignment consumes the returned string.
		work.extra = sizeof(std::string);
		if (!room_storage_add(work.extra, original.command.operation_id.bytes.size() + 1) ||
		    !room_replay_admit(work, work.extra))
			return false;
		work.identity = operation_key(original.command.operation_id);
		if (!room_replay_admit(work))
			return false;
		if (operations.find(work.identity) != operations.end() ||
		    operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
		    work.retained > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
			return false;
		work.continuation = original.phase ==
				    critical_native_recovery_phase::continuation_pending;
		work.holds_fences = !work.continuation ||
				    native_birth_origin_command(original.command) ||
				    zone_reset_typed_command(original.command);
		if (!room_replay_admit(work, sizeof(operation_state) +
						     sizeof(std::unique_ptr<operation_state>)))
			return room_replay_rollback(work);
		work.state = std::make_unique<operation_state>();
		if (!room_replay_admit(work))
			return room_replay_rollback(work);
		// Fresh default state owns no vector capacity. This genuine source profile
		// reserves ALL command/attachment copy requests and the original native
		// allocation before that complete original copy sequence. There is no
		// callback inside the sequence, and its resulting private state is censused
		// at the next cut using actual vector capacities, not encoded byte length.
		work.fresh = 0;
		if (!room_checkpoint_heap(original, true, work.fresh) ||
		    !room_storage_add(work.fresh, sizeof(native_operation_context)) ||
		    !room_storage_add(work.fresh,
				      sizeof(std::unique_ptr<native_operation_context>)) ||
		    !room_replay_admit(work, work.fresh))
			return room_replay_rollback(work);
		work.state->command = original.command;
		work.state->native = std::make_unique<native_operation_context>();
		work.state->native->revision = original.revision;
		work.state->native->phase = original.phase;
		work.state->native->attachment = original.attachment;
		work.state->retained_bytes = work.retained;
		work.state->queued_at_usec = now_usec();
		work.state->attempt = 1;
		work.state->attachments = 0;
		work.state->phase = work.continuation ?
					    critical_operation_phase::native_continuation_pending :
					    critical_operation_phase::queued;
		work.state->retain_until_publication = true;
		work.state->admission_failure_queued = false;
		work.state->native_physical_released = work.continuation;
		work.rollback_cleanup_reserved = true;
		if (!operations.next_unique_insert_extra_peak(work.identity, 0, &work.extra) ||
		    !room_replay_admit(work, work.extra))
			return room_replay_rollback(work);
		operations.emplace(work.identity, std::move(work.state));
		work.inserted = true;
		// state is now genuinely owned by CURRENT C; the moved-from pointer is
		// excluded from private scratch before any pending/fence request.
		if (!room_replay_admit(work))
			return room_replay_rollback(work);
		if (!work.continuation)
		{
			if (!pending.push_back_extra_peak(work.identity, &work.extra) ||
			    !room_replay_admit(work, work.extra))
				return room_replay_rollback(work);
			pending.push_back(work.identity);
			if (!room_replay_admit(work))
				return room_replay_rollback(work);
		}
		if (work.holds_fences)
		{
			work.found = operations.find(work.identity);
			for (work.key_index = 0;
			     work.key_index < work.found->second->command.keys.size();
			     ++work.key_index)
			{
				if (!room_replay_admit(work,
						       sizeof(std::string) +
							       2 * sizeof(std::array<char, 9>)))
					return room_replay_rollback(work);
				work.fence_key = entity_key(
					work.found->second->command.keys[work.key_index]);
				work.fence_found = fences.find(work.fence_key);
				if (work.fence_found == fences.end())
				{
					if (!native_identity_queue::initial_heap_bytes(
						    &work.fresh) ||
					    !fences.next_unique_insert_extra_peak(
						    work.fence_key, work.fresh, &work.extra) ||
					    !room_replay_admit(work, work.extra))
						return room_replay_rollback(work);
					work.fence_queue = &fences[std::move(work.fence_key)];
					if (!room_replay_admit(work))
						return room_replay_rollback(work);
				}
				else
					work.fence_queue = &work.fence_found->second;
				if (!work.fence_queue->push_back_extra_peak(work.identity,
									    &work.extra) ||
				    !room_replay_admit(work, work.extra))
					return room_replay_rollback(work);
				work.fence_queue->push_back(work.identity);
				if (!room_replay_admit(work))
					return room_replay_rollback(work);
			}
			health.fenced_keys = fences.size();
		}
		// The actual paired ordinary NMB4 host observer forwards this full prefix
		// and authentic reserve/context to the genuine passive birth owner.
		// Original mandatory observer remains installed. The reviewed birth
		// companion owns exact duplicate/new-attachment rollback and current
		// census; no unbounded domain/observer fallback runs on this capability.
		if (!room_replay_local_prefix(work) ||
		    !replay.native_bounded_observer(original, replay.context,
						    room_replay_reserve_locked, &work,
						    work.partial))
			return room_replay_rollback(work);
		// No fallible callback after successful actual birth registry attachment.
		// Exact original successful tail is nonallocating.
		update_depth();
		return true;
	}
	catch (...)
	{
		// Genuine allocation exceptions only; actual init lock remains owned.
		// Partially copied private state and actual inserted nodes are removed,
		// while real surviving bucket/deque capacity stays in CURRENT C.
		return room_replay_rollback(work);
	}
#endif
}
} // namespace

// This profile is additional to the actual CURRENT-C getter's independent
// closure. Selecting ROOT preadmits it before entering the first retained scan.
size_t critical_room_shared_budget_lender::operation_frame_bytes() noexcept
{
	// Real operation scope; invoke formals and bool return; relay formals/local
	// reference/result; exact scalar begin/refresh/end formal and result carriers.
	return sizeof(operation_borrow_frame) + sizeof(callback_borrow_frame) +
	       // Nested reserve formals/result, genuine lock/registered getters and
	       // both original checked additions. CURRENT-C lower closure is separate.
	       4 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + 3 * sizeof(void *) +
	       2 * sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	       5 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + 2 * sizeof(void *) +
	       sizeof(size_t) + sizeof(bool) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	       2 * sizeof(void *) + sizeof(bool);
}

bool critical_room_shared_budget_lender::invoke(const std::unique_lock<std::mutex> &actual_lock,
						bool (*actual_reserve)(size_t, void *) noexcept,
						void *actual_guard, size_t outer,
						operation_callback operation,
						void *operation_context) noexcept
{
	if (!actual_reserve || !actual_guard || !operation || active_operation_ ||
	    actual_lock.mutex() != &coordinator_mutex || !actual_lock.owns_lock() ||
	    !item_native_quest_coordinator_budget_scope_owner::registered())
		return false;
	operation_borrow_frame scope{ actual_lock, actual_reserve,    actual_guard,
				      operation,   operation_context, outer };
	if (!room_coordinator_current_storage_bytes_locked(&scope.current) ||
	    !item_native_quest_coordinator_budget_scope_owner::begin_borrow(
		    &scope, scope.reserve, scope.guard, scope.current))
		return false;
	active_operation_ = &scope;
	// The real noexcept provider and all its native capacity checks execute while
	// the loan remains installed. There is no early exit between begin and end.
	scope.accepted = scope.operation(operation_borrow_frame::relay, &scope, scope.outer,
					 scope.operation_context);
	active_operation_ = nullptr;
	scope.ended = item_native_quest_coordinator_budget_scope_owner::end_borrow(&scope);
	return scope.accepted && scope.ended;
}

size_t critical_mixed_startup_replay_owner::coordinator_operation_frame_bytes() noexcept
{
	return critical_room_shared_budget_lender::operation_frame_bytes() +
	       // Actual pure profile return, plus the invoking owner method's this,
	       // callback/context/outer/provider/provider-context formals and result.
	       6 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool);
}

bool critical_mixed_startup_replay_owner::invoke_coordinator_operation(
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	bool (*operation)(bool (*)(size_t, void *) noexcept, void *, size_t, void *) noexcept,
	void *operation_context) const noexcept
{
	return held() && critical_room_shared_budget_lender::invoke(
				 init_lock_, reserve, context, outer, operation, operation_context);
}

bool critical_room_shared_budget_lender::callback_matches(
	const std::unique_lock<std::mutex> &actual_lock, bool (*callback)(size_t, void *) noexcept,
	void *context) noexcept
{
	// Read only after the authentic mixed owner proves the held initializer.
	// The callback/context are the actual loan's relay and stack identity;
	// an ordinary physical-guard callback or fabricated context never matches.
	return active_operation_ && callback == operation_borrow_frame::relay &&
	       context == active_operation_ && &actual_lock == &active_operation_->lock &&
	       actual_lock.mutex() == &coordinator_mutex && actual_lock.owns_lock() &&
	       item_native_quest_coordinator_budget_scope_owner::borrowed_for(
		       active_operation_->guard, active_operation_->reserve);
}

bool critical_mixed_startup_replay_owner::coordinator_operation_callback_matches(
	bool (*reserve)(size_t, void *) noexcept, void *context) const noexcept
{
	return held() &&
	       critical_room_shared_budget_lender::callback_matches(init_lock_, reserve, context);
}

namespace
{
// ROOT complete native startup insertion; unselected until full host joins.
[[maybe_unused]] bool
enqueue_native_replayed_bounded(const critical_native_recovery_envelope &original,
				const replay_observer_context &replay,
				const std::unique_lock<std::mutex> &actual_init_lock,
				bool (*reserve)(size_t, void *) noexcept, void *budget_context,
				size_t exclusive_outer) noexcept
{
	if (!reserve || !replay.native_observer || !replay.native_bounded_observer ||
	    actual_init_lock.mutex() != &coordinator_mutex || !actual_init_lock.owns_lock())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	if constexpr (!mixed_replay_state_construction_source::supported ||
		      !mixed_replay_command_assignment_source::layout ||
		      !mixed_replay_fence_queue_construction_source::layout ||
		      !mixed_replay_table_forecast_source::layout ||
		      !mixed_replay_string_value_source::layout ||
		      !mixed_replay_key_value_source::layout ||
		      !mixed_replay_table_key_lookup_source::layout ||
		      !mixed_replay_table_node_storage_source::layout ||
		      !mixed_replay_table_value_construction_source::layout ||
		      !mixed_replay_table_insert_source::layout ||
		      !mixed_replay_table_iterator_erase_source::layout ||
		      !mixed_replay_transparent_table_lookup_source::layout ||
		      !mixed_replay_deque_tail_erase_source::layout ||
		      !mixed_replay_std_remove_source::layout ||
		      !mixed_replay_rollback_controller_source::layout ||
		      !mixed_replay_update_depth_header_source::layout ||
		      !mixed_replay_workspace_entry_source::layout ||
		      !mixed_replay_enqueue_entry_controller_source::layout)
		return false;
	size_t own_source = 0;
	const size_t validation_source = critical_command_valid_frame_bytes();
	// Selecting caller preadmits the complete owned entry/query/INLINE before
	// this pure profile and construction. Source remains internally held below.
	if (!mixed_replay_enqueue_retained_source(&own_source))
		return false;
	room_replay_workspace work(actual_init_lock, reserve, budget_context, exclusive_outer);
	try
	{
		if (!room_storage_add(work.outer, mixed_replay_table_forecast_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_key_value_source::admitted_window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_queue_cleanup_source::mutation_window) ||
		    !room_storage_add(work.outer, mixed_replay_table_key_lookup_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_table_node_storage_source::window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_table_value_construction_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_table_insert_source::window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_table_iterator_erase_source::window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_transparent_table_lookup_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_deque_tail_erase_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_std_remove_source::window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_rollback_controller_source::window) ||
		    !room_storage_add(work.outer,
				      mixed_replay_update_depth_header_source::window) ||
		    !room_storage_add(work.outer, mixed_replay_workspace_entry_source::window) ||
		    !room_storage_add(work.outer, own_source) ||
		    !room_storage_add(work.outer, validation_source) || !room_replay_admit(work))
			return false;
		// Complete original execution decision. Conservative SHOP dispatch
		// never falls through ordinary, and requires its actual shared worker.
		// The original shared validity proof occurs here AND below after size;
		// neither original validation is replaced by an INITIAL-only proof.
		if (!room_replay_local_prefix(work))
			return false;
		if (native_birth_shared_shop_command(original.command))
		{
			if (!shared_native_apply_callback ||
			    !native_birth_validators.shared_shop_valid_bounded ||
			    native_birth_validators.shared_shop_valid_bounded !=
				    native_mobile_birth_shared_shop_recovery_valid_fixed_bounded)
				return false;
			constexpr size_t validator_query =
				native_mobile_birth_shared_shop_recovery_fixed_valid_source_query_frame_bytes();
			// Exact target was proved before these genuine pure queries.
			// Full child Source + INITIAL is transient preentry; reset
			// the prefix before entering its self-owning validator.
			if (!room_replay_admit(work, validator_query) ||
			    !native_mobile_birth_shared_shop_recovery_fixed_valid_source_frame_bytes(
				    &work.extra) ||
			    !native_mobile_birth_shared_shop_recovery_fixed_valid_initial_inline_bytes(
				    &work.fresh) ||
			    !room_storage_add(work.extra, work.fresh) ||
			    !room_replay_admit(work, work.extra) ||
			    !room_replay_local_prefix(work) ||
			    !native_birth_validators.shared_shop_valid_bounded(
				    original, room_replay_reserve_locked, &work, work.partial))
				return false;
		}
		else if (!(critical_command_valid(original.command) ||
			   (original.command.schema_version ==
				    CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			    critical_command_envelope_valid(original.command) &&
			    extension_validator_callback && extension_validator_bounded_callback &&
			    extension_validator_bounded_callback(original.command,
								 room_replay_reserve_locked, &work,
								 work.partial))))
			return false;
		// Completed pure execution proof is no longer live during checkpoint
		// encoding. The genuine child owns its full codec Source itself.
		work.outer -= validation_source;
		if (!room_replay_admit(work) ||
		    !mixed_startup_checkpoint_size_bounded(original, &work.retained,
							   room_replay_reserve_locked, &work,
							   work.partial))
			return false;
		// Preserve original family ordering/readiness and full historical
		// execution, progressed and terminal continuation validation. Held
		// retirement execution-only remains enforced by the full size helper.
		if (native_birth_typed_command(original.command))
		{
			if (!room_replay_admit(work) || !native_birth_validators_ready())
				return false;
			if (native_birth_shared_shop_command(original.command))
			{
				if (!native_birth_validators.shared_shop_valid_bounded ||
				    native_birth_validators.shared_shop_valid_bounded !=
					    native_mobile_birth_shared_shop_recovery_valid_fixed_bounded)
					return false;
				constexpr size_t validator_query =
					native_mobile_birth_shared_shop_recovery_fixed_valid_source_query_frame_bytes();
				// Exact target was proved before these genuine pure queries.
				// Full child Source + INITIAL is transient preentry; reset
				// the prefix before entering its self-owning validator.
				if (!room_replay_admit(work, validator_query) ||
				    !native_mobile_birth_shared_shop_recovery_fixed_valid_source_frame_bytes(
					    &work.extra) ||
				    !native_mobile_birth_shared_shop_recovery_fixed_valid_initial_inline_bytes(
					    &work.fresh) ||
				    !room_storage_add(work.extra, work.fresh) ||
				    !room_replay_admit(work, work.extra) ||
				    !room_replay_local_prefix(work) ||
				    !native_birth_validators.shared_shop_valid_bounded(
					    original, room_replay_reserve_locked, &work,
					    work.partial))
					return false;
			}
			else if (original.command.payload_version ==
				 NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION)
			{
				if (!native_birth_validators.cash_role_valid_bounded ||
				    native_birth_validators.cash_role_valid_bounded !=
					    native_mobile_birth_cash_role_recovery_valid_fixed_bounded)
					return false;
				constexpr size_t validator_query =
					native_mobile_birth_cash_role_recovery_fixed_valid_source_query_frame_bytes();
				// Exact target was proved before these genuine pure queries.
				// Full child Source + INITIAL is transient preentry; reset
				// the prefix before entering its self-owning validator.
				if (!room_replay_admit(work, validator_query) ||
				    !native_mobile_birth_cash_role_recovery_fixed_valid_source_frame_bytes(
					    &work.extra) ||
				    !native_mobile_birth_cash_role_recovery_fixed_valid_initial_inline_bytes(
					    &work.fresh) ||
				    !room_storage_add(work.extra, work.fresh) ||
				    !room_replay_admit(work, work.extra) ||
				    !room_replay_local_prefix(work) ||
				    !native_birth_validators.cash_role_valid_bounded(
					    original, room_replay_reserve_locked, &work,
					    work.partial))
					return false;
			}
			else
			{
				if (!native_birth_validators.valid_bounded ||
				    native_birth_validators.valid_bounded !=
					    native_mobile_birth_recovery_valid_bounded)
					return false;
				constexpr size_t validator_query =
					native_mobile_birth_recovery_historical_valid_source_query_frame_bytes();
				// Exact target was proved before these genuine pure queries.
				// Full child Source + INITIAL is transient preentry; reset
				// the prefix before entering its self-owning validator.
				if (!room_replay_admit(work, validator_query) ||
				    !native_mobile_birth_recovery_historical_valid_source_frame_bytes(
					    &work.extra) ||
				    !native_mobile_birth_recovery_historical_valid_initial_inline_bytes(
					    &work.fresh) ||
				    !room_storage_add(work.extra, work.fresh) ||
				    !room_replay_admit(work, work.extra) ||
				    !room_replay_local_prefix(work) ||
				    !native_birth_validators.valid_bounded(
					    original, room_replay_reserve_locked, &work,
					    work.partial))
					return false;
			}
		}
		if (zone_reset_typed_command(original.command) &&
		    (!room_replay_admit(work) || !zone_reset_validators_ready() ||
		     !zone_reset_validators.valid_bounded ||
		     !zone_reset_validators.valid_bounded(original, room_replay_reserve_locked,
							  &work, work.partial)))
			return false;
		if (native_auction_typed_command(original.command) &&
		    (!room_replay_admit(work) || !native_auction_validators_ready() ||
		     !native_auction_validators.valid_bounded ||
		     !native_auction_validators.valid_bounded(original, room_replay_reserve_locked,
							      &work, work.partial)))
			return false;
		if (!room_replay_admit(work))
			return false;
		// Original operation_key return/local coexistence. Its actual 16-byte
		// GCC13 string requests 17 bytes; assignment consumes the returned string.
		work.extra = sizeof(std::string);
		if (!room_storage_add(work.extra, original.command.operation_id.bytes.size() + 1) ||
		    !room_replay_admit(work, work.extra))
			return false;
		work.identity = operation_key(original.command.operation_id);
		if (!room_replay_admit(work))
			return false;
		if (operations.find(work.identity) != operations.end() ||
		    operations.size() >= CRITICAL_COORDINATOR_MAX_OPERATIONS ||
		    work.retained > CRITICAL_COORDINATOR_MAX_BYTES - health.retained_bytes)
			return false;
		work.continuation = original.phase ==
				    critical_native_recovery_phase::continuation_pending;
		work.holds_fences = !work.continuation ||
				    native_birth_origin_command(original.command) ||
				    zone_reset_typed_command(original.command);
		if (!room_replay_admit(
			    work, sizeof(operation_state) +
					  sizeof(std::unique_ptr<operation_state>) +
					  mixed_replay_state_construction_source::state_window))
			return room_replay_rollback(work);
		work.state = std::make_unique<operation_state>();
		if (!room_replay_admit(work))
			return room_replay_rollback(work);
		// Fresh default state owns no vector capacity. This genuine source profile
		// reserves ALL command/attachment copy requests and the original native
		// allocation before that complete original copy sequence. There is no
		// callback inside the sequence, and its resulting private state is censused
		// at the next cut using actual vector capacities, not encoded byte length.
		work.fresh = 0;
		if (!room_checkpoint_heap(original, true, work.fresh) ||
		    !room_storage_add(work.fresh, sizeof(native_operation_context)) ||
		    !room_storage_add(work.fresh,
				      sizeof(std::unique_ptr<native_operation_context>)) ||
		    !room_storage_add(work.fresh,
				      mixed_replay_state_construction_source::context_window) ||
		    !room_storage_add(work.fresh,
				      mixed_replay_command_assignment_source::command_copy) ||
		    !room_storage_add(work.fresh,
				      mixed_replay_command_assignment_source::attachment_copy) ||
		    !room_replay_admit(work, work.fresh))
			return room_replay_rollback(work);
		work.state->command = original.command;
		work.state->native = std::make_unique<native_operation_context>();
		work.state->native->revision = original.revision;
		work.state->native->phase = original.phase;
		work.state->native->attachment = original.attachment;
		work.state->retained_bytes = work.retained;
		work.state->queued_at_usec = now_usec();
		work.state->attempt = 1;
		work.state->attachments = 0;
		work.state->phase = work.continuation ?
					    critical_operation_phase::native_continuation_pending :
					    critical_operation_phase::queued;
		work.state->retain_until_publication = true;
		work.state->admission_failure_queued = false;
		work.state->native_physical_released = work.continuation;
		work.rollback_cleanup_reserved = true;
		if (!operations.next_unique_insert_extra_peak(work.identity, 0, &work.extra) ||
		    !room_replay_admit(work, work.extra))
			return room_replay_rollback(work);
		operations.emplace(work.identity, std::move(work.state));
		work.inserted = true;
		// state is now genuinely owned by CURRENT C; the moved-from pointer is
		// excluded from private scratch before any pending/fence request.
		if (!room_replay_admit(work))
			return room_replay_rollback(work);
		if (!work.continuation)
		{
			if (!pending.push_back_extra_peak(work.identity, &work.extra) ||
			    !room_replay_admit(work, work.extra))
				return room_replay_rollback(work);
			pending.push_back(work.identity);
			if (!room_replay_admit(work))
				return room_replay_rollback(work);
		}
		if (work.holds_fences)
		{
			work.found = operations.find(work.identity);
			for (work.key_index = 0;
			     work.key_index < work.found->second->command.keys.size();
			     ++work.key_index)
			{
				if (!room_replay_admit(work,
						       sizeof(std::string) +
							       2 * sizeof(std::array<char, 9>)))
					return room_replay_rollback(work);
				work.fence_key = entity_key(
					work.found->second->command.keys[work.key_index]);
				work.fence_found = fences.find(work.fence_key);
				if (work.fence_found == fences.end())
				{
					if (!native_identity_queue::initial_heap_bytes(
						    &work.fresh) ||
					    !fences.next_unique_insert_extra_peak(
						    work.fence_key, work.fresh, &work.extra) ||
					    !room_storage_add(
						    work.extra,
						    mixed_replay_fence_queue_construction_source::
							    window) ||
					    !room_replay_admit(work, work.extra))
						return room_replay_rollback(work);
					work.fence_queue = &fences[std::move(work.fence_key)];
					if (!room_replay_admit(work))
						return room_replay_rollback(work);
				}
				else
					work.fence_queue = &work.fence_found->second;
				if (!work.fence_queue->push_back_extra_peak(work.identity,
									    &work.extra) ||
				    !room_replay_admit(work, work.extra))
					return room_replay_rollback(work);
				work.fence_queue->push_back(work.identity);
				if (!room_replay_admit(work))
					return room_replay_rollback(work);
			}
			health.fenced_keys = fences.size();
		}
		// The complete actual native dispatcher is mandatory, exactly as the
		// original observer. It registers every original native family under
		// its genuine SAME scope, with domain partial holds left untouched.
		if (!room_replay_local_prefix(work))
			return room_replay_rollback(work);
		const bool borrowed = critical_room_shared_budget_lender::callback_matches(
			work.lock, work.reserve, work.context);
		const auto observer_reserve = borrowed ? work.reserve : room_replay_reserve_locked;
		void *const observer_context = borrowed ? work.context : &work;
		// Full journal input and every real private/copy capacity remain in
		// work.partial. The operation relay itself supplies fresh CURRENT C once.
		if (!replay.native_bounded_observer(original, replay.context, observer_reserve,
						    observer_context, work.partial))
			return room_replay_rollback(work);
		// No fallible callback after successful new ROOT registry attachment.
		// Exact original successful tail is nonallocating.
		update_depth();
		return true;
	}
	catch (...)
	{
		// Genuine allocation exceptions only; actual init lock remains owned.
		// Partially copied private state and actual inserted nodes are removed,
		// while real surviving bucket/deque capacity stays in CURRENT C.
		return room_replay_rollback(work);
	}
#endif
}
} // namespace: complete original native passive replay insertion

namespace
{
// Complete ADDITIONAL source of the actual callback_matches branch. Existing
// replay workspace/formals/census/child/lender sources remain selecting-owner
// responsibilities. Physical retained C/G/J and input heaps are not SOURCE.
constexpr size_t mixed_replay_forwarding_match_source =
	// callback_matches: lock/reserve/context and bool result.
	3 * sizeof(void *) + sizeof(bool) +
	// Genuine unique_lock::mutex(this,result), owns_lock(this,bool).
	2 * sizeof(void *) + sizeof(void *) + sizeof(bool) +
	// Actual borrowed_for(guard,reserve,bool), before its scheduler query.
	2 * sizeof(void *) + sizeof(bool) +
	// nevent_is_game_thread bool; get_id returns the genuine id; operator==
	// receives two by-value ids. Include the optional temporary id distinctly.
	3 * sizeof(std::thread::id) +
	// id(native_handle) this; both real generated id copies(this,source), and
	// cleanup receivers of all three ids. No duplicate id storage is in P.
	8 * sizeof(void *) +
	// __gthread_self / pthread_self native results + id native-handle formal;
	// actual scheduler and thread-id equality returned booleans.
	3 * sizeof(std::thread::native_handle_type) + 2 * sizeof(bool);
// Both observer forwarding cuts have genuine selected bool and two named
// callback/context locals. Match queries execute sequentially, so one complete
// typed match closure dominates each invocation without inventing heap/state.
constexpr size_t mixed_replay_forwarding_source_supplement =
	mixed_replay_forwarding_match_source + 2 * sizeof(void *) + sizeof(bool);
}

bool critical_mixed_startup_replay_owner::replay_forwarding_source_supplement(
	size_t *output) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) && defined(__linux__) && \
	defined(__GLIBC__) && defined(__x86_64__)
	if (!output)
	{
		errno = EINVAL;
		return false;
	}
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::thread::id) != 8 ||
	    sizeof(std::thread::native_handle_type) != 8)
	{
		errno = ENOTSUP;
		return false;
	}
	*output = mixed_replay_forwarding_source_supplement;
	return true;
#else
	(void)output;
	errno = ENOTSUP;
	return false;
#endif
}

namespace
{
// Actual size-helper entry, local source/live scalars and returned bool.
// The encoded vector is INLINE, not part of this source subtotal.
constexpr size_t mixed_checkpoint_entry_source =
	4 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool);
constexpr size_t mixed_checkpoint_checked_add_source =
	sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t mixed_checkpoint_vector_size_source =
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t);
constexpr size_t mixed_checkpoint_vector_index_source = 2 * sizeof(void *) + sizeof(size_t);
// Actual const vector::empty -> const begin/end -> normal_iterator(const
// iterator_type&). Each stored uint8_t* converts to const uint8_t*, with its
// distinct materialized pointer bound to the constructor's const reference.
// Two returned iterator objects survive equality, then their implicit dtors.
constexpr size_t mixed_checkpoint_vector_empty_source =
	sizeof(void *) + sizeof(bool) +
	2 * (2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *)) +
	// Equality lhs/rhs references and bool, each genuine base(this/ref result).
	2 * sizeof(void *) + sizeof(bool) + 2 * (2 * sizeof(void *)) + 2 * sizeof(void *);

// Full held_retirement_transport.h classification: command and bytes refs,
// actual one-reference lambda object, blob_size, item_section/tail/terms and bool result.
// The direct integer call has this/offset/count/i, uint64 local and result.
// It does not copy its closure. Closure destruction owns its actual receiver.
constexpr size_t mixed_checkpoint_held_classification_source =
	2 * sizeof(void *) + sizeof(void *) + sizeof(uint64_t) + 3 * sizeof(size_t) + sizeof(bool) +
	sizeof(void *) + (sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t)) +
	mixed_checkpoint_vector_size_source + mixed_checkpoint_vector_index_source;
constexpr size_t mixed_checkpoint_classification_source =
	// native_transport_command plus the four remaining real field-only
	// predicates (quest, birth, auction, zone). The subsequent held-phase
	// check invokes the same full held classifier sequentially; summing both
	// genuine call sites conservatively preserves the enclosing transport call.
	5 * (sizeof(void *) + sizeof(bool)) + 2 * mixed_checkpoint_held_classification_source;

constexpr size_t mixed_checkpoint_encoded_vector_cleanup_source =
	// Exact uint8 vector/~base/get-allocator/_Destroy range/aux chain.
	sizeof(std::vector<uint8_t> *) + sizeof(std::vector<uint8_t> *) +
	sizeof(std::allocator<uint8_t> *) + 2 * sizeof(uint8_t *) +
	sizeof(std::allocator<uint8_t> *) + 2 * sizeof(uint8_t *) + 2 * sizeof(uint8_t *) +
	sizeof(bool) +
	// _Vector_base/_M_deallocate -> traits/allocator/new_allocator, sized
	// delete, C++20 constant-evaluation bool, allocator and implicit impl/data/
	// new_allocator cleanup. uint8 trivial _Destroy has no element destructor.
	sizeof(void *) + sizeof(std::vector<uint8_t> *) + sizeof(uint8_t *) + sizeof(size_t) +
	sizeof(std::allocator<uint8_t> *) + sizeof(uint8_t *) + sizeof(size_t) +
	sizeof(std::allocator<uint8_t> *) + sizeof(uint8_t *) + sizeof(size_t) +
	sizeof(std::allocator<uint8_t> *) + sizeof(uint8_t *) + sizeof(size_t) + sizeof(void *) +
	sizeof(size_t) + sizeof(bool) + sizeof(std::allocator<uint8_t> *) + 3 * sizeof(void *);
constexpr size_t mixed_checkpoint_own_source =
	mixed_checkpoint_entry_source + mixed_checkpoint_classification_source +
	// Own encoded default construction reaches six real this receivers.
	6 * sizeof(void *) + mixed_checkpoint_encoded_vector_cleanup_source +
	mixed_checkpoint_vector_size_source + mixed_checkpoint_vector_empty_source +
	mixed_checkpoint_checked_add_source;

constexpr size_t mixed_checkpoint_profile_query_source =
	// Export wrapper(output,bool), internal profile(output,bool,codec N),
	// genuine lower query(output,bool), and checked-add(ref,amount,bool).
	// initial_inline's named source N is the larger genuine query alternative.
	2 * (sizeof(void *) + sizeof(bool)) + 2 * sizeof(size_t) +
	critical_command_startup_codec_complete_source_query_frame_bytes() +
	mixed_checkpoint_checked_add_source;

bool mixed_checkpoint_source(size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(__linux__) && defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) &&               \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                         \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                      \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                   \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8 ||
		      sizeof(uint64_t) != 8 || sizeof(std::allocator<uint8_t>) != 1)
		return false;
	size_t codec = 0;
	// Published full startup codec source is explicitly the genuine bounded
	// encode/decode envelope. Its documented disjoint codec branch excess is
	// retained; no old DTO/heap/working-vector request is charged as source.
	if (!critical_command_startup_codec_complete_source_frame_bytes(&codec) ||
	    !room_storage_add(codec, mixed_checkpoint_own_source))
		return false;
	*output = codec;
	return true;
#else
	return false;
#endif
}

// Complete original retained-size algorithm, with SOURCE admitted before any
// actual predicate/codec/vector lifetime. Caller owns authentic envelope,
// callback/context descendants and old output. The same-lock replay relay still
// supplies fresh CURRENT C once. No lock, borrowed scope or authority is minted.
bool mixed_startup_checkpoint_size_bounded(const critical_native_recovery_envelope &envelope,
					   size_t *retained,
					   bool (*reserve)(size_t, void *) noexcept, void *context,
					   size_t outer) noexcept
{
	size_t source = 0, live = outer;
	if (!reserve || !retained || !room_storage_add(live, mixed_checkpoint_entry_source) ||
	    !room_storage_add(live, mixed_checkpoint_profile_query_source) ||
	    !room_storage_add(live, mixed_checkpoint_checked_add_source) ||
	    !reserve(live, context) || !mixed_checkpoint_source(&source))
		return false;
	live = outer;
	if (!room_storage_add(live, source) ||
	    !room_storage_add(live, sizeof(std::vector<uint8_t>)) || !reserve(live, context))
		return false;
	if (!native_transport_command(envelope.command) ||
	    (held_retirement_transport_command(envelope.command) &&
	     envelope.phase != critical_native_recovery_phase::execution_pending) ||
	    !envelope.command.publication_required || !envelope.revision ||
	    (envelope.phase != critical_native_recovery_phase::execution_pending &&
	     envelope.phase != critical_native_recovery_phase::continuation_pending) ||
	    envelope.attachment.empty() ||
	    envelope.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES)
		return false;
	std::vector<uint8_t> encoded;
	if (critical_command_encode_bounded(envelope.command, &encoded, reserve, context, live) !=
		    critical_command_codec_result::ok ||
	    encoded.size() >
		    CRITICAL_COORDINATOR_MAX_BYTES - NATIVE_COORDINATOR_ENVELOPE_OVERHEAD ||
	    envelope.attachment.size() > CRITICAL_COORDINATOR_MAX_BYTES -
						 NATIVE_COORDINATOR_ENVELOPE_OVERHEAD -
						 encoded.size())
		return false;
	// Last reservation retained the original fresh encoder result capacity;
	// move transfer merely changes ownership to encoded. That local ownership
	// ends at scope cleanup; no encoded buffer survives the scalar output, and
	// no new callback or retained-capacity baseline is required here.
	*retained =
		encoded.size() + envelope.attachment.size() + NATIVE_COORDINATOR_ENVELOPE_OVERHEAD;
	return true;
}
} // namespace

bool critical_mixed_startup_replay_owner::native_checkpoint_size_source_frames(
	size_t *output) noexcept
{
	return mixed_checkpoint_source(output);
}
bool critical_mixed_startup_replay_owner::native_checkpoint_size_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output)
		return false;
	size_t source = 0;
	if (!mixed_checkpoint_source(&source))
		return false;
	*output = sizeof(std::vector<uint8_t>);
	return true;
}

namespace
{
namespace mixed_coordinator_current_source
{
constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
constexpr size_t D = sizeof(std::ptrdiff_t);
constexpr size_t maximum(size_t a, size_t b) noexcept
{
	return a > b ? a : b;
}

// The unchanged pure census calls no allocator, codec, mutex or user callback.
// All values below price actual signatures/locals/returns and range lifetime,
// not the persistent C bytes reported by that census. Sequential alternatives
// share an explicit maximum; genuine nested parent carriers persist.
constexpr size_t add = P + N + B;
constexpr size_t array = P + 2 * N + B + add;
constexpr size_t vector_capacity = P + N + D;
// capacity -> _M_is_local -> _M_data/_M_local_data -> pointer_to ->
// addressof -> __addressof. The left pointer result survives the right chain.
constexpr size_t string_capacity =
	P + N + P + B + maximum(2 * P, P + 2 * P + 2 * P + 2 * P + 2 * P);
constexpr size_t string = 2 * P + B + maximum(string_capacity, add);
constexpr size_t command = 2 * P + B + maximum(array + vector_capacity, add + vector_capacity);

// Actual _Node_const_iterator -> _Node_iterator_base construction and cleanup.
// Direct _Hashtable const begin calls _M_begin, end materializes nullptr.
constexpr size_t node_begin = 2 * P + 2 * P + 2 * P + 2 * P;
constexpr size_t node_end = 2 * P + 2 * P + 2 * P;
constexpr size_t node_equal = 2 * P + B;
constexpr size_t node_increment = 2 * P + P + 2 * P;
// * -> _M_v -> _M_valptr -> __aligned_buffer::_M_ptr -> _M_addr.
// The actual aligned buffer uses builtin &storage, with no addressof call.
constexpr size_t node_dereference = 5 * (2 * P);
constexpr size_t node_cleanup = 2 * (P + P);
constexpr size_t node_range_live = 2 * P + 2 * sizeof(decltype(operations)::const_iterator);
// Sum the actual sequential begin/end/comparison/increment/dereference/dtor
// scopes conservatively; no generic iterator envelope or iteration multiplier.
constexpr size_t node_range = node_range_live + node_begin + node_end + node_equal +
			      node_increment + node_dereference + node_cleanup;
// Only genuine std::unordered_map adds these two begin/end wrapper signatures.
constexpr size_t unordered_range = node_range + 2 * (2 * P);
constexpr size_t table_count = P + N;
constexpr size_t unordered_count = 2 * (P + N);
constexpr size_t table_map = 2 * P + B + maximum(array + table_count, node_range + string);
constexpr size_t unordered_map =
	2 * P + B + maximum(array + unordered_count, unordered_range + string);

// Native identity queue: this/output/actual refs; bytes/blocks/block_bytes;
// actual node pointer-difference value; returned bool; __deque_buf_size arg/N.
constexpr size_t identity_own = 3 * P + 3 * N + D + B;
constexpr size_t deque_buffer = 2 * N;
constexpr size_t identity_range_live = 2 * P + 2 * sizeof(native_identity_queue::const_iterator);
// Each deque const begin/end converts the actual stored iterator by const-ref
// constructor. It returns its genuine four-pointer const_iterator.
constexpr size_t deque_endpoint = P + sizeof(native_identity_queue::const_iterator) + 2 * P;
constexpr size_t deque_equal = 2 * P + B;
// ++ -> _M_set_node(this/newnode) -> _S_buffer_size(result N) ->
// __deque_buf_size(arg/result N), including the actual difference_type cast.
constexpr size_t deque_increment = 2 * P + 2 * P + N + 2 * N + D;
constexpr size_t deque_dereference = 2 * P;
constexpr size_t deque_cleanup = 2 * P;
constexpr size_t identity_range_queries =
	2 * deque_endpoint + deque_equal + deque_increment + deque_dereference + deque_cleanup;
constexpr size_t identity = identity_own + deque_buffer + identity_range_live +
			    identity_range_queries + string_capacity;

// player_retained_deque<critical_completion> has no element scan. Its genuine
// current getter declares impl ref and six size_t locals, calls max()/buffer,
// and computes the real node pointer difference. No element-owned heap here.
constexpr size_t completion = 3 * P + 6 * N + B + N + deque_buffer + D;

// const unique_ptr get -> __uniq_ptr_impl::_M_ptr -> const tuple get<0> ->
// __get_helper -> _Tuple_impl::_M_head -> _Head_base::_M_head, each real
// receiver/reference and returned pointer/reference. No deleter runs in C scan.
constexpr size_t unique_get = 6 * (2 * P);
constexpr size_t unique_present = P + B + unique_get;
constexpr size_t unique_access = 2 * P + unique_get;
// Actual operations loop retains state ref and three genuine branch-local
// slots definitions (conservative sum of those sequential source alternatives).
// sizeof(*transaction) is unevaluated: it invokes no transaction dereference.
constexpr size_t operations_body =
	P + 3 * N +
	maximum(unique_present,
		maximum(unique_access + command, maximum(unique_access + vector_capacity, add)));

// Root output/total/heap/result plus the actual retained_storage_bytes return.
// Each whole map helper and later range is a genuine sequential root branch.
constexpr size_t own = P + 2 * N + B + N;
constexpr size_t scalar_scans =
	maximum(maximum(table_map, unordered_map),
		maximum(array + vector_capacity, maximum(identity, completion)));
constexpr size_t range_scans =
	maximum(unordered_range + string,
		maximum(node_range + maximum(identity, add),
			maximum(unordered_range + command, node_range + operations_body)));
constexpr size_t selected = own + maximum(scalar_scans, range_scans);
} // namespace mixed_coordinator_current_source
} // namespace

bool critical_mixed_startup_replay_owner::coordinator_current_source_frames(size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(__linux__) && defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) &&               \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                         \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                      \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                   \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&             \
	(!defined(_GLIBCXX_INLINE_VERSION) || _GLIBCXX_INLINE_VERSION == 0)
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8 ||
		      sizeof(decltype(operations)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(fences)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(active_keys)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(completed_cache)::const_iterator) != sizeof(void *) ||
		      sizeof(native_identity_queue::const_iterator) != 4 * sizeof(void *))
		return false;
	*output = mixed_coordinator_current_source::selected;
	return true;
#else
	return false;
#endif
}

namespace
{
namespace mixed_replay_private_current_source
{
using namespace mixed_coordinator_current_source;
// Each prefix has actual workspace ref, request value and bool result. All
// workspace fields/object storage remain existing INLINE, not this SOURCE.
constexpr size_t prefix_entry = P + N + B;
// Native private state accesses have two real nested unique_ptr receivers.
// The pointers are read, not copied or destroyed by either census.
constexpr size_t native_prefix =
	prefix_entry +
	maximum(string, maximum(unique_present,
				maximum(unique_access + command,
					maximum(unique_access + unique_present,
						2 * unique_access + vector_capacity + add))));
constexpr size_t legacy_prefix =
	prefix_entry + maximum(string, maximum(unique_present, unique_access + command));
constexpr size_t prefixes = maximum(native_prefix, legacy_prefix);
// room_checkpoint_heap has envelope/bytes/command refs, fresh flag, keys and
// revisions locals and returned bool. Actual size and capacity alternatives
// have identical typed scalar/difference query carriers; only real heap request
// counts differ. room_retire_add has the same P+N+B source signature as add.
constexpr size_t fresh_request = 3 * P + 2 * N + 2 * B + vector_capacity + add;
} // namespace mixed_replay_private_current_source
} // namespace

bool critical_mixed_startup_replay_owner::replay_private_current_source_frames(
	size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(__linux__) && defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) &&               \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                         \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                      \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                   \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8)
		return false;
	*output = mixed_replay_private_current_source::prefixes;
	return true;
#else
	return false;
#endif
}

bool critical_mixed_startup_replay_owner::replay_fresh_copy_request_source_frames(
	size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(__linux__) && defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) &&               \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                         \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                      \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                   \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8)
		return false;
	*output = mixed_replay_private_current_source::fresh_request;
	return true;
#else
	return false;
#endif
}

bool critical_mixed_startup_replay_owner::replay_state_construction_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported)
		return false;
	*output = mixed_replay_state_construction_source::state_window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_native_context_construction_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported)
		return false;
	*output = mixed_replay_state_construction_source::context_window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_command_copy_source_frames(size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout)
		return false;
	*output = mixed_replay_command_assignment_source::command_copy;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_command_move_source_frames(size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout)
		return false;
	*output = mixed_replay_command_assignment_source::command_move;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_attachment_copy_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout)
		return false;
	*output = mixed_replay_command_assignment_source::attachment_copy;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_fence_queue_construction_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_fence_queue_construction_source::layout)
		return false;
	*output = mixed_replay_fence_queue_construction_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_table_forecast_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_table_forecast_source::layout)
		return false;
	*output = mixed_replay_table_forecast_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_string_value_source_frames(size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout)
		return false;
	*output = mixed_replay_string_value_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_operation_state_cleanup_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout)
		return false;
	*output = mixed_replay_populated_state_cleanup_source::cleanup_window;
	return true;
}
bool critical_mixed_startup_replay_owner::replay_value_lifetime_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout)
		return false;
	*output = mixed_replay_populated_state_cleanup_source::value_lifetime_window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_queue_push_source_frames(size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_fence_queue_construction_source::layout ||
	    !mixed_replay_string_value_source::layout)
		return false;
	*output = mixed_replay_queue_push_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_key_value_source_frames(size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout || !mixed_replay_key_value_source::layout)
		return false;
	*output = mixed_replay_key_value_source::window;
	return true;
}
bool critical_mixed_startup_replay_owner::replay_queue_cleanup_source_frames(size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_fence_queue_construction_source::layout)
		return false;
	*output = mixed_replay_queue_cleanup_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_table_key_lookup_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout ||
	    !mixed_replay_table_key_lookup_source::layout)
		return false;
	*output = mixed_replay_table_key_lookup_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_table_node_storage_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout ||
	    !mixed_replay_table_node_storage_source::layout)
		return false;
	*output = mixed_replay_table_node_storage_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_table_value_construction_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout ||
	    !mixed_replay_table_node_storage_source::layout ||
	    !mixed_replay_table_value_construction_source::layout)
		return false;
	*output = mixed_replay_table_value_construction_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_table_insert_source_frames(size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout || !mixed_replay_table_insert_source::layout)
		return false;
	*output = mixed_replay_table_insert_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_table_iterator_erase_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout ||
	    !mixed_replay_table_iterator_erase_source::layout)
		return false;
	*output = mixed_replay_table_iterator_erase_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_transparent_lookup_source_supplement_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout ||
	    !mixed_replay_transparent_table_lookup_source::layout)
		return false;
	*output = mixed_replay_transparent_table_lookup_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_transparent_lookup_full_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout ||
	    !mixed_replay_transparent_table_lookup_source::layout)
		return false;
	*output = mixed_replay_transparent_table_lookup_source::full_window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_deque_tail_erase_source_supplement_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout ||
	    !mixed_replay_deque_tail_erase_source::layout)
		return false;
	*output = mixed_replay_deque_tail_erase_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_deque_tail_erase_full_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout ||
	    !mixed_replay_deque_tail_erase_source::layout)
		return false;
	*output = mixed_replay_deque_tail_erase_source::full_window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_std_remove_source_supplement_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout || !mixed_replay_std_remove_source::layout)
		return false;
	*output = mixed_replay_std_remove_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_std_remove_full_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_string_value_source::layout || !mixed_replay_std_remove_source::layout)
		return false;
	*output = mixed_replay_std_remove_source::full_window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_rollback_controller_source_supplement_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_std_remove_source::layout ||
	    !mixed_replay_rollback_controller_source::layout)
		return false;
	*output = mixed_replay_rollback_controller_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_update_depth_header_source_supplement_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_std_remove_source::layout ||
	    !mixed_replay_update_depth_header_source::layout)
		return false;
	*output = mixed_replay_update_depth_header_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_workspace_entry_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !mixed_replay_std_remove_source::layout || !mixed_replay_workspace_entry_source::layout)
		return false;
	*output = mixed_replay_workspace_entry_source::window;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_workspace_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_workspace_entry_source::layout)
		return false;
	*output = sizeof(room_replay_workspace) > sizeof(legacy_replay_workspace) ?
			  sizeof(room_replay_workspace) :
			  sizeof(legacy_replay_workspace);
	return true;
}

namespace
{
namespace mixed_replay_enqueue_owned_source
{
using namespace mixed_replay_command_assignment_source;
// Exact Source families already retained by both complete replay entries.
// Overlapping string/copy/cleanup and queue push/cleanup queries select their
// authenticated maxima, not sums of the overlapping public family getters.
constexpr size_t persistent = mixed_replay_table_forecast_source::window +
			      mixed_replay_key_value_source::admitted_window +
			      mixed_replay_queue_cleanup_source::mutation_window +
			      mixed_replay_table_key_lookup_source::window +
			      mixed_replay_table_node_storage_source::window +
			      mixed_replay_table_value_construction_source::window +
			      mixed_replay_table_insert_source::window +
			      mixed_replay_table_iterator_erase_source::window +
			      mixed_replay_transparent_table_lookup_source::window +
			      mixed_replay_deque_tail_erase_source::window +
			      mixed_replay_std_remove_source::window +
			      mixed_replay_rollback_controller_source::window +
			      mixed_replay_update_depth_header_source::window +
			      mixed_replay_workspace_entry_source::window;
// Newly retained disjoint actual enclosing/relay/current scopes. CURRENT C and
// the user's callback implementation remain external to this private scratch.
constexpr size_t supplement = mixed_replay_enqueue_entry_controller_source::supplement +
			      mixed_replay_private_current_source::prefixes +
			      mixed_replay_private_current_source::fresh_request +
			      mixed_replay_forwarding_source_supplement;
// Original make_unique state, legacy command move, and native command/context/
// attachment copy are separate phases. Each real transient was already admitted
// at its precise cut; the selecting preentry needs their genuine maximum too.
constexpr size_t transient =
	max_source(mixed_replay_state_construction_source::state_window,
		   max_source(mixed_replay_command_assignment_source::command_move,
			      mixed_replay_state_construction_source::context_window +
				      mixed_replay_command_assignment_source::command_copy +
				      mixed_replay_command_assignment_source::attachment_copy));
constexpr size_t own_without_validation = persistent + supplement + transient;
constexpr bool layout =
	mixed_replay_state_construction_source::supported &&
	mixed_replay_command_assignment_source::layout &&
	mixed_replay_fence_queue_construction_source::layout &&
	mixed_replay_table_forecast_source::layout && mixed_replay_string_value_source::layout &&
	mixed_replay_key_value_source::layout && mixed_replay_table_key_lookup_source::layout &&
	mixed_replay_table_node_storage_source::layout &&
	mixed_replay_table_value_construction_source::layout &&
	mixed_replay_table_insert_source::layout &&
	mixed_replay_table_iterator_erase_source::layout &&
	mixed_replay_transparent_table_lookup_source::layout &&
	mixed_replay_deque_tail_erase_source::layout && mixed_replay_std_remove_source::layout &&
	mixed_replay_rollback_controller_source::layout &&
	mixed_replay_update_depth_header_source::layout &&
	mixed_replay_workspace_entry_source::layout &&
	mixed_replay_enqueue_entry_controller_source::layout;
} // namespace mixed_replay_enqueue_owned_source

bool mixed_replay_enqueue_retained_source(size_t *output) noexcept
{
	if (!output || !mixed_replay_enqueue_owned_source::layout)
		return false;
	*output = mixed_replay_enqueue_owned_source::supplement;
	return true;
}
} // namespace

bool critical_mixed_startup_replay_owner::replay_enqueue_owned_source_frames(size_t *output) noexcept
{
	if (!output || !mixed_replay_enqueue_owned_source::layout)
		return false;
	size_t total = mixed_replay_enqueue_owned_source::own_without_validation;
	// This existing actual lower getter prices exactly the unchanged pure
	// envelope/legacy/zero/key/binary-search predicate graph, not a codec heap.
	// Its size_t return and this checked add are owned by the query contract.
	if (!room_storage_add(total, critical_command_valid_frame_bytes()))
		return false;
	*output = total;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_enqueue_owned_source_supplement_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_enqueue_owned_source::layout)
		return false;
	size_t total = mixed_replay_enqueue_owned_source::supplement;
	if (!room_storage_add(total, critical_command_valid_frame_bytes()))
		return false;
	*output = total;
	return true;
}

bool critical_mixed_startup_replay_owner::replay_enqueue_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_enqueue_owned_source::layout)
		return false;
	*output = sizeof(room_replay_workspace) > sizeof(legacy_replay_workspace) ?
			  sizeof(room_replay_workspace) :
			  sizeof(legacy_replay_workspace);
	return true;
}

namespace
{
namespace native_birth_duplicate_matches_source
{
using namespace mixed_replay_command_assignment_source;
// New helper has state/envelope/reserve/context, outer and returned bool;
// live/source/initial/supplement/child_outer and its automatic constexpr query.
constexpr size_t helper = 4 * P + 7 * N + B;
constexpr size_t checked_add = P + N + B;
constexpr size_t capacity = P + N;
constexpr size_t child_getter = P + B;
// Existing independently reviewed selected get/ptr/tuple chain. Distinct
// native presence and -> uses are sequential; the access maximum covers both.
constexpr size_t native_access = mixed_coordinator_current_source::unique_access;
// C++20 vector<uint8_t>::operator== has two vector refs/returned bool, two
// genuine size(this,returned size) calls, and three const endpoints. The
// returned normal_iterator objects alias equal's three by-value parameters.
// Stored pointer const lvalues bind through reference-compatible qualification
// directly to the constructor's const pointer reference, without a pointer
// prvalue materialization. Each endpoint owns this and its declared constructor.
constexpr size_t vector_equal = 2 * P + B;
constexpr size_t vector_sizes = 2 * (P + N);
constexpr size_t const_endpoints = 3 * (P + 2 * P);
constexpr size_t equal_entry = 3 * P + B;
constexpr size_t equal_aux = 3 * P + B;
// Actual __niter_base(normal_iterator by value)->pointer, then base(this)->ref.
// The first 2P are the by-value CI object and declared returned pointer.
constexpr size_t unwrap = 3 * (2 * P + 2 * P);
// __equal_aux1 has three raw pointers, real automatic __simple bool and result.
constexpr size_t equal_aux1 = 3 * P + 2 * B;
// Actual selected __equal<true>::equal: three raw pointers, __len and bool.
constexpr size_t equal_memcmp_controller = 3 * P + N + B;
// std::__memcmp is a captured HEADER function: pointer/pointer/count/int result
// and is_constant_evaluated bool. Its __builtin_memcmp implementation is the
// existing shared external-library/runtime qualification, not an invented row.
constexpr size_t header_memcmp = 2 * P + N + sizeof(int) + B;
constexpr size_t byte_equal = vector_equal + vector_sizes + const_endpoints + equal_entry +
			      equal_aux + unwrap + equal_aux1 + equal_memcmp_controller +
			      header_memcmp;
#if __cpp_lib_three_way_comparison
// The selected C++20 != expression rewrites operator==, with no separate
// declared operator!=. No implicit trivial iterator special member is invented.
constexpr size_t byte_compare = byte_equal;
#else
// Actual alternative declaration has two vector refs and bool before ==.
constexpr size_t byte_compare = 2 * P + B + byte_equal;
#endif
// Preserve the genuine previously reviewed vector cleanup Source credit;
// each actual output vector selects six default-construction receivers and
// that typed cleanup chain. The two real sequential lifetimes are explicitly
// conservatively summed. Their object storage belongs INITIAL, not SOURCE.
constexpr size_t outputs_lifetime = 2 * (6 * P + vector_cleanup_source<uint8_t>());
constexpr size_t local_source = helper + checked_add + capacity + child_getter + native_access +
				byte_compare + outputs_lifetime;
constexpr bool layout = mixed_replay_command_assignment_source::layout &&
			sizeof(std::vector<uint8_t>::const_iterator) == P && sizeof(int) == 4;

bool complete_source(size_t *output) noexcept
{
	if (!output || !layout)
		return false;
	size_t codec = 0;
	if (!critical_command_startup_codec_complete_source_frame_bytes(&codec))
		return false;
	size_t total = local_source;
	if (!room_storage_add(total, codec))
		return false;
	*output = total;
	return true;
}
} // namespace native_birth_duplicate_matches_source

bool native_birth_submit_matches_complete_bounded(const operation_state &state,
						  const critical_native_recovery_envelope &envelope,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer) noexcept
{
	// The selecting submit cut preadmits the complete Source/actual INITIAL
	// before this helper. Preserve the original pure mismatch decision BEFORE
	// any vector construction or reserve call inside the helper.
	if (!state.native || state.native->revision != envelope.revision ||
	    state.native->phase != envelope.phase ||
	    state.native->attachment != envelope.attachment)
		return false;
	std::vector<uint8_t> left_encoded;
	std::vector<uint8_t> right_encoded;
	size_t live = outer, source = 0, initial = 0, supplement = 0, child_outer = 0;
	constexpr size_t query = critical_command_startup_codec_complete_source_query_frame_bytes();
	if (!room_storage_add(live, 2 * sizeof(std::vector<uint8_t>)) ||
	    !room_storage_add(live, native_birth_duplicate_matches_source::local_source) ||
	    !reserve || !reserve(live, context))
		return false;
	// Pure query scopes and full codec Source+initial are transient. Only the
	// unchanged codec's genuine missing supplement enters its callback outer;
	// its original DTO/writer Source and actual private heaps remain its own.
	if (!room_storage_add(child_outer = live, query) || !reserve(child_outer, context) ||
	    !critical_command_startup_codec_complete_source_frame_bytes(&source) ||
	    !critical_command_startup_codec_initial_inline_bytes(&initial) ||
	    !critical_command_startup_codec_source_supplement_frame_bytes(&supplement) ||
	    !room_storage_add(source, initial) || !room_storage_add(child_outer = live, source) ||
	    !reserve(child_outer, context) || !room_storage_add(child_outer = live, supplement))
		return false;
	if (critical_command_encode_bounded(state.command, &left_encoded, reserve, context,
					    child_outer) != critical_command_codec_result::ok)
		return false;
	if (!room_storage_add(live, left_encoded.capacity()))
		return false;
	// Genuine first encoded capacity remains live beside both output objects
	// through the complete second encode. Requery the same real provider, not
	// a stale recorded peak or an encoded-size approximation of retained heap.
	if (!room_storage_add(child_outer = live, query) || !reserve(child_outer, context) ||
	    !critical_command_startup_codec_complete_source_frame_bytes(&source) ||
	    !critical_command_startup_codec_initial_inline_bytes(&initial) ||
	    !critical_command_startup_codec_source_supplement_frame_bytes(&supplement) ||
	    !room_storage_add(source, initial) || !room_storage_add(child_outer = live, source) ||
	    !reserve(child_outer, context) || !room_storage_add(child_outer = live, supplement))
		return false;
	return critical_command_encode_bounded(envelope.command, &right_encoded, reserve, context,
					       child_outer) == critical_command_codec_result::ok &&
	       left_encoded == right_encoded;
}
} // namespace

bool critical_native_mobile_birth_publication_owner::submit_matches_source_frames(
	size_t *output) noexcept
{
	return native_birth_duplicate_matches_source::complete_source(output);
}

bool critical_native_mobile_birth_publication_owner::submit_matches_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output || !native_birth_duplicate_matches_source::layout)
		return false;
	*output = 2 * sizeof(std::vector<uint8_t>);
	return true;
}

bool critical_native_mobile_birth_publication_owner::submit_workspace_entry_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !native_birth_submit_workspace_source::layout)
		return false;
	*output = native_birth_submit_workspace_source::window;
	return true;
}

bool critical_native_mobile_birth_publication_owner::submit_workspace_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !native_birth_submit_workspace_source::layout)
		return false;
	*output = sizeof(room_submit_workspace);
	return true;
}

bool critical_native_mobile_birth_publication_owner::submit_private_current_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !mixed_replay_command_assignment_source::layout ||
	    !native_birth_submit_workspace_source::layout)
		return false;
	// The real submit and replay prefixes have the same work/ref/request/result,
	// string, mutable unique/state/native, command, capacity and checked-add graph.
	// Submit additionally scans its private envelope command and attachment,
	// sequentially before the same nested native state branch. The existing
	// native_prefix explicitly dominates those direct command/capacity/add paths.
	// No callback/lender/C scan belongs to this private local owner getter.
	*output = mixed_replay_private_current_source::native_prefix;
	return true;
}

bool critical_native_mobile_birth_publication_owner::submit_clone_state_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !native_birth_submit_clone_state_source::layout)
		return false;
	*output = native_birth_submit_clone_state_source::window;
	return true;
}

bool critical_native_mobile_birth_publication_owner::submit_mutation_child_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !native_birth_submit_mutation_children_source::layout)
		return false;
	*output = native_birth_submit_mutation_children_source::window;
	return true;
}

bool critical_native_mobile_birth_publication_owner::submit_header_controller_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !native_birth_submit_header_controller_source::layout)
		return false;
	*output = native_birth_submit_header_controller_source::window;
	return true;
}

bool critical_native_mobile_birth_publication_owner::submit_coordinator_current_source_frames(
	size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(__linux__) && defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) &&               \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                         \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                      \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                   \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&             \
	(!defined(_GLIBCXX_INLINE_VERSION) || _GLIBCXX_INLINE_VERSION == 0)
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8 ||
		      sizeof(decltype(operations)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(fences)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(active_keys)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(completed_cache)::const_iterator) != sizeof(void *) ||
		      sizeof(native_identity_queue::const_iterator) != 4 * sizeof(void *))
		return false;
	*output = mixed_coordinator_current_source::selected;
	return true;
#else
	return false;
#endif
}

bool critical_native_mobile_birth_publication_owner::submit_admitted_trace_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported ||
	    !native_birth_submit_admitted_trace_source::layout)
		return false;
	*output = native_birth_submit_admitted_trace_source::window;
	return true;
}

bool critical_native_mobile_birth_publication_owner::submit_command_validation_source_frames(
	size_t *output) noexcept
{
	if (!output || !mixed_replay_state_construction_source::supported)
		return false;
	// Exact existing published entire allocation-free validation contract.
	// It includes its getter's size_t return; no second result/local N here.
	*output = critical_command_valid_frame_bytes();
	return true;
}

namespace native_birth_submit_lender_header_source
{
constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
constexpr size_t H = sizeof(std::thread::native_handle_type);
// Exact ordinary captured get_id overload: the right by-value comparison id
// is the get_id return and thread::id construction output (one object). The
// stored-id left comparison argument is distinct. No implicit id copy/dtor
// Source is invented. The explicit native-handle constructor owns this P/H.
// Captured Linux/glibc >= 2.27 get_id constructs directly from pthread_self.
// That declared H return and the constructor H formal remain distinct.
constexpr size_t thread_identity = 2 * sizeof(std::thread::id) + P + 2 * H + 2 * B;
// reserve(actual_lock,callback,guard,exclusive,current_output)->bool.
// callback_borrow_frame is actual existing inline ownership, not Source here.
constexpr size_t lender_own = 4 * P + N + B;
// Exact unique_lock::mutex(this)->mutex* and owns_lock(this)->bool.
constexpr size_t lock_identity = 2 * P + P + B;
// The genuine scalar registered getter's bool is a separate sequential call.
constexpr size_t registered = B;
// begin and refresh: lender/callback/guard refs, current size, bool return.
// end: lender ref and bool return. They run sequentially, so the named full
// 33-byte begin/refresh window dominates end's 9 before/after ROOT callback.
constexpr size_t loan = 3 * P + N + B;
static_assert(loan >= P + B);
constexpr size_t selected = lender_own + lock_identity + registered + loan + thread_identity;
#if defined(__linux__) && defined(__GLIBC__) && defined(__GLIBC_MINOR__) && \
	(__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 27)) &&     \
	defined(_GLIBCXX_HAS_GTHREADS) && defined(_GLIBCXX_NATIVE_THREAD_ID)
constexpr bool supported = mixed_replay_state_construction_source::supported &&
			   sizeof(std::thread::id) == 8 && H == 8;
#else
constexpr bool supported = false;
#endif
// Existing retained CURRENT-C Source names room_storage_add P+N+B and owns
// its exact sequential scope through these lender calls; no second child or
// physical C is added here. The loan parent's carriers coexist with thread
// descendants; registered/lock scopes are explicit conservative occurrences.
// Direct get_id/pthread declarations authenticate the selected header graph,
// not the external pthread_self implementation or runtime ABI qualification.
}

bool critical_native_mobile_birth_publication_owner::submit_lender_header_source_frames(
	size_t *output) noexcept
{
	if (!output || !native_birth_submit_lender_header_source::supported)
		return false;
	*output = native_birth_submit_lender_header_source::selected;
	return true;
}

namespace coordinator_current_storage_lock_header_source
{
constexpr size_t P = sizeof(void *), I = sizeof(int), B = sizeof(bool);
// Original current_storage_bytes(output)->bool root signature. Physical lock
// object is INITIAL, independent of the global coordinator mutex in CURRENT.
constexpr size_t root = P + B;
// lock_guard(this,mutex&); actual direct _M_device reference initialization
// calls no addressof. mutex::lock owns this and actual local int __e.
constexpr size_t construct = 2 * P;
constexpr size_t lock = P + I;
// Genuine gthread mutex helper pointer and int return, active_p int return;
// actual static active pointer is not a per-call automatic object. Declared
// pthread_mutex_lock pointer/int return are selected, external body excluded.
constexpr size_t gthread = P + 2 * I;
constexpr size_t pthread_declaration = P + I;
// Actual destructor receiver -> mutex::unlock(receiver), then same selected
// gthread/pthread declarations. No try_lock/adopt tag/mutex constructor path.
constexpr size_t destroy = P;
constexpr size_t unlock = P;
constexpr size_t local = root + construct + lock + gthread + pthread_declaration + destroy +
			 unlock + gthread + pthread_declaration;
// __throw_system_error is reached only after real mutex error. Its actual
// local __e is already lock's I. Callee declaration/body and exception-owned
// storage remain shared external Source cuts; no unproved formals invented.
}

bool critical_zone_reset_item_publication_owner::current_storage_header_source_frames(
	size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(__linux__) && defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) &&               \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                         \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                      \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                   \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&             \
	(!defined(_GLIBCXX_INLINE_VERSION) || _GLIBCXX_INLINE_VERSION == 0)
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8 ||
		      sizeof(decltype(operations)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(fences)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(active_keys)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(completed_cache)::const_iterator) != sizeof(void *) ||
		      sizeof(native_identity_queue::const_iterator) != 4 * sizeof(void *))
		return false;
	if constexpr (sizeof(int) != 4 || sizeof(std::lock_guard<std::mutex>) != sizeof(void *))
		return false;
	*output = mixed_coordinator_current_source::selected +
		  coordinator_current_storage_lock_header_source::local;
	return true;
#else
	return false;
#endif
}

bool critical_zone_reset_item_publication_owner::current_storage_initial_inline_bytes(
	size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(__linux__) && defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) &&               \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                         \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                      \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                   \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&             \
	(!defined(_GLIBCXX_INLINE_VERSION) || _GLIBCXX_INLINE_VERSION == 0)
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8 ||
		      sizeof(decltype(operations)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(fences)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(active_keys)::const_iterator) != sizeof(void *) ||
		      sizeof(decltype(completed_cache)::const_iterator) != sizeof(void *) ||
		      sizeof(native_identity_queue::const_iterator) != 4 * sizeof(void *))
		return false;
	if constexpr (sizeof(int) != 4 || sizeof(std::lock_guard<std::mutex>) != sizeof(void *))
		return false;
	*output = sizeof(std::lock_guard<std::mutex>);
	return true;
#else
	return false;
#endif
}

namespace native_birth_submit_lock_gthread_header_source
{
// Genuine two sequential submit lock/unlock occurrences, not the original
// already-priced mutex receiver/error local/unique-lock controller scopes.
// Existing matching observer namespace authenticates the exact same typed
// __gthread_mutex_lock/unlock pointer/int-return/active_p int and declared
// pthread_mutex_lock/unlock pointer/int-return. Actual static active pointer
// is not a per-call object. Do not invent callable builtin wrappers.
constexpr size_t selected =
	2 * (coordinator_current_storage_lock_header_source::gthread +
	     coordinator_current_storage_lock_header_source::pthread_declaration);
constexpr bool supported = native_birth_submit_lender_header_source::supported && sizeof(int) == 4;
}

bool critical_native_mobile_birth_publication_owner::submit_lock_gthread_header_source_frames(
	size_t *output) noexcept
{
	if (!output || !native_birth_submit_lock_gthread_header_source::supported)
		return false;
	*output = native_birth_submit_lock_gthread_header_source::selected;
	return true;
}

bool critical_native_mobile_birth_publication_owner::submit_owned_source_frames(
	size_t *output) noexcept
{
	if (!output)
		return false;
	size_t total = 0, part = 0;
	// EXACT same Source values retained in submit's actual work.outer before
	// its first callback. Checked sequential queries introduce no heap/DTO.
	// Input/current/heap/registry and actual external callbacks are separate.
	if (!submit_private_current_source_frames(&part) || !room_storage_add(total, part) ||
	    !submit_workspace_entry_source_frames(&part) || !room_storage_add(total, part) ||
	    !room_storage_add(total, native_birth_submit_source_frames()) ||
	    !submit_clone_state_source_frames(&part) || !room_storage_add(total, part) ||
	    !submit_mutation_child_source_frames(&part) || !room_storage_add(total, part) ||
	    !submit_header_controller_source_frames(&part) || !room_storage_add(total, part) ||
	    !submit_coordinator_current_source_frames(&part) || !room_storage_add(total, part) ||
	    !submit_admitted_trace_source_frames(&part) || !room_storage_add(total, part) ||
	    // Actual NEW complete INITIAL helper workP/resultB/querylocalN17,
	    // plus the later shared validity constexpr query localN8.
	    !room_storage_add(total, sizeof(void *) + sizeof(bool) + sizeof(size_t)) ||
	    !room_storage_add(total, sizeof(size_t)) ||
	    !submit_command_validation_source_frames(&part) || !room_storage_add(total, part) ||
	    !submit_lender_header_source_frames(&part) || !room_storage_add(total, part) ||
	    !submit_lock_gthread_header_source_frames(&part) || !room_storage_add(total, part))
		return false;
	*output = total;
	return true;
}

bool critical_native_mobile_birth_publication_owner::submit_owned_initial_inline_bytes(
	size_t *output) noexcept
{
	// Exact original room_submit_workspace, never a fabricated whole Source.
	return submit_workspace_initial_inline_bytes(output);
}

bool critical_native_mobile_birth_publication_owner::submit_owned_source_supplement_frames(
	size_t *output) noexcept
{
	if (!output)
		return false;
	size_t full = 0;
	if (!submit_owned_source_frames(&full))
		return false;
	// Genuine zero: the original owned entry now retains ALL those same
	// checked components internally before first callback, exactly once.
	// Caller first preadmits full owned Source+INITIAL prospectively, then
	// excludes that ended preflight from its real outer passed to submit.
	*output = 0;
	return true;
}
