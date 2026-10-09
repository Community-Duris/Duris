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
};

namespace
{
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
		const auto &actual = storage_base::_M_impl;
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
		const auto &actual = storage_base::_M_impl;
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
			if (shared_native_worker_fenced(state.command))
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
		if ((shared_apply && !shared_owner.current()) ||
		    (room_apply && !room_owner.current()))
		{
			// Lost coordinator lifetime before invocation: no fabricated completion
			// or never-admitted receipt. Original durable state stays owned.
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
			if (shared_apply)
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
			applied = { (shared_apply || room_apply) ?
					    critical_apply_outcome::ambiguous_commit :
					    critical_apply_outcome::retryable_failure,
				    0, 0 };
		}
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
	critical_native_recovery_observer_bounded_fn native_replay_observer_bounded)
{
	if (!apply || !worker_count || worker_count > CRITICAL_COORDINATOR_DEFAULT_WORKERS * 4)
		return false;
	std::unique_lock<std::mutex> lock(coordinator_mutex);
	if (health.initialized || lifecycle_guard_active ||
	    active_cutover_phase != cutover_owner_phase::none)
		return false;
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
	    !room_retire_add(live, sizeof(std::lock_guard<std::mutex>)) ||
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
			std::lock_guard<std::mutex> lock(coordinator_mutex);
			auto found = operations.find(identity);
			if (!health.initialized || stop_requested || found == operations.end() ||
			    !room_retire_matches_bounded(*found->second, expected, reserve,
							 budget_context, live) ||
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
			    !zone_reset_validators.valid_bounded(expected, reserve, budget_context,
								 live) ||
			    !zone_reset_validators.terminal_bounded(expected, reserve,
								    budget_context, live))
				return false;
			// Preserve every original retained proposal/ceiling rule even though
			// this capability retires only, never constructs a successor.
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
			result = critical_command_journal_retire_native_recovery_bounded(
				expected, reserve, budget_context, live);
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
	    !room_retire_add(live, sizeof(std::lock_guard<std::mutex>)) ||
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
			std::lock_guard<std::mutex> lock(coordinator_mutex);
			auto found = operations.find(identity);
			if (!health.initialized || stop_requested || found == operations.end() ||
			    !room_retire_matches_bounded(*found->second, expected, reserve,
							 budget_context, live) ||
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
			    !zone_reset_validators.valid_bounded(expected, reserve, budget_context,
								 live) ||
			    !zone_reset_validators.successor_bounded(expected, prepared, reserve,
								     budget_context, live))
				return false;
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
		const auto result = critical_command_journal_replace_native_recovery_bounded(
			expected, prepared, reserve, budget_context, live);
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
	    !room_retire_add(live, sizeof(std::lock_guard<std::mutex>)) ||
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
		    !room_retire_matches_bounded(*found->second, frozen, reserve, budget_context,
						 live) ||
		    !native_receipt_equal(found->second->publication_completion, receipt) ||
		    !zone_reset_validators_ready() || !zone_reset_validators.publication_bounded ||
		    !zone_reset_validators.successor_bounded ||
		    !zone_reset_validators.publication_bounded(frozen, receipt, reserve,
							       budget_context, live) ||
		    !zone_reset_validators.successor_bounded(frozen, successor, reserve,
							     budget_context, live))
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
		result = critical_command_journal_replace_native_recovery_bounded(
			frozen, successor, reserve, budget_context, live);
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
	    !room_retire_add(live, sizeof(std::lock_guard<std::mutex>)) || !reserve(live, context))
		return false;
	try
	{
		// The original phase1/phase2 attempts differ only in this actual phase test.
		// One same-lock lookup preserves both complete predicates and proofs.
		const std::string identity = operation_key(command.operation_id);
		critical_native_recovery_envelope copy;
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (!health.initialized || stop_requested || found == operations.end() ||
		    !found->second->native || found->second->publication_checkpointing ||
		    found->second->native_context_uncertain ||
		    found->second->native_ack_uncertain ||
		    !room_checkpoint_equal_bounded(command, found->second->command, reserve,
						   context, live))
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
		    !reserve(live, context))
			return false;
		// Same original native_envelope field copies into one actual fresh object.
		// Direct fields avoid a separate uncharged return carrier or fabricated BODY.
		copy.command = current;
		copy.revision = state.native->revision;
		copy.phase = phase;
		copy.attachment = state.native->attachment;
		if (!zone_reset_validators_ready() || !zone_reset_validators.valid_bounded ||
		    !zone_reset_validators.valid_bounded(copy, reserve, context, live))
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
	    !room_retire_add(live, sizeof(std::lock_guard<std::mutex>)) || !reserve(live, context))
		return false;
	try
	{
		const std::string identity = operation_key(expected.command.operation_id);
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		const auto found = operations.find(identity);
		if (found == operations.end() || !found->second->retain_until_publication ||
		    !room_retire_matches_bounded(*found->second, expected, reserve, context,
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
	    !room_retire_add(live, sizeof(std::lock_guard<std::mutex>)) ||
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
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		auto found = operations.find(identity);
		if (found == operations.end() || found->second->publication_checkpointing ||
		    !health.initialized || coordinator_generation != generation ||
		    coordinator_generation_exhausted || stop_requested ||
		    (lifecycle_guard_active &&
		     lifecycle_guard_thread != std::this_thread::get_id()) ||
		    !matches(*found->second) ||
		    !room_retire_matches_bounded(*found->second, frozen, reserve, budget_context,
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
	if (!room_retire_add(live, sizeof(std::lock_guard<std::mutex>)) || !reserve(live, context))
		return false;
	try
	{
		std::lock_guard<std::mutex> lock(coordinator_mutex);
		// Same original execution_supported predicates, specialized only to ROOM.
		// The original callback remains mandatory. Never fall back to invoking its
		// unbounded allocator when the paired prospective provider is absent.
		if (critical_command_valid(command))
			return true;
		return command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		       critical_command_envelope_valid(command) && extension_validator_callback &&
		       extension_validator_bounded_callback &&
		       extension_validator_bounded_callback(command, reserve, context, live);
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
		sizeof(coordinator_mutex) + sizeof(work_available) + sizeof(result_available) +
		sizeof(admission_available) + sizeof(publication_checkpoint_finished) +
		sizeof(publication_checkpoints_inflight) + sizeof(guarded_publications_inflight) +
		sizeof(operations) + sizeof(pending) + sizeof(pending_admission) +
		sizeof(completion_delivery) + sizeof(active_keys) + sizeof(fences) +
		sizeof(completed_cache) + sizeof(completed_order) + sizeof(completed_cache_bytes) +
		sizeof(pending_admission_bytes) + sizeof(admission_inflight_bytes) +
		sizeof(workers) + sizeof(admission_worker) + sizeof(apply_callback) +
		sizeof(shared_native_apply_callback) + sizeof(zone_reset_apply_callback) +
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
	struct actual_borrow_frame
	{
		const std::unique_lock<std::mutex> &lock;
		bool (*reserve)(size_t, void *) noexcept;
		void *guard;
		size_t *output;
		size_t current = 0, full = 0;
		bool borrowed = false, accepted = false, ended = false;
	} scope{ actual_lock, actual_reserve, actual_guard, current_output };
	if (!room_coordinator_current_storage_bytes_locked(&scope.current))
		return false;
	if (scope.output)
		*scope.output = scope.current;
	scope.full = exclusive_live;
	if (!room_storage_add(scope.full, sizeof(actual_borrow_frame)) ||
	    !room_storage_add(scope.full, scope.current))
		return false;
	// Original unregistered callbacks receive the complete prefix, never a
	// guessed partial budget or new game-thread requirement. Once registered,
	// the real shared owner authenticates callback/context/exact actual guard.
	if (item_native_quest_coordinator_budget_scope_owner::registered())
	{
		if (!item_native_quest_coordinator_budget_scope_owner::begin_borrow(
			    &scope, scope.reserve, scope.guard, scope.current))
			return false;
		scope.borrowed = true;
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
