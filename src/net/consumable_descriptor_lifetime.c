#include "net/consumable_descriptor_lifetime.h"
#include "net/consumable_prepared_output_queue.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/mm.h"
#include "account/character_identity.h"
#include "net/mccp.h"
#include "net/transport.h"
#include <atomic>
#include <limits>
#include <mutex>
#include <new>
#include <thread>
#include <utility>

extern mm_ds *dead_desc_pool;

struct consumable_descriptor_lifetime_loan::control
{
	enum class phase
	{
		created,
		invalidated
	};
	control *previous = nullptr, *next = nullptr;
	std::atomic<size_t> references{ 1 }; // Actual creator/registry reference.
	std::atomic<descriptor_data *> descriptor;
	std::atomic<phase> lifecycle{ phase::created };
	const uint64_t incarnation;
	const std::thread::id creator_thread;
	mm_ds *const pool;
	const unsigned origin;
	control(descriptor_data *d, mm_ds *p, uint64_t id, unsigned source) noexcept
		: descriptor(d)
		, incarnation(id)
		, creator_thread(std::this_thread::get_id())
		, pool(p)
		, origin(source)
	{
	}
};

// Actual control objects stay in this census while ANY loan retains them,
// including invalidated objects whose original pool address has been reused.
class consumable_descriptor_lifetime_registry final
{
    public:
	using control = consumable_descriptor_lifetime_loan::control;
	static std::mutex mutex;
	static control *head;
	static size_t objects;
	static uint64_t next_incarnation;
	static std::atomic<bool> poisoned;
	static bool retain(control *value) noexcept
	{
		size_t current = value->references.load(std::memory_order_relaxed);
		while (current && current != SIZE_MAX)
			if (value->references.compare_exchange_weak(current, current + 1,
								    std::memory_order_relaxed,
								    std::memory_order_relaxed))
				return true;
		return false;
	}
	static void release_locked(control *value) noexcept
	{
		if (value->references.fetch_sub(1, std::memory_order_acq_rel) != 1)
			return;
		if (value->previous)
			value->previous->next = value->next;
		else
			head = value->next;
		if (value->next)
			value->next->previous = value->previous;
		--objects;
		delete value;
	}
	static void release(control *value) noexcept
	{
		if (!value)
			return;
		try
		{
			std::lock_guard<std::mutex> lock(mutex);
			release_locked(value);
		}
		catch (...)
		{
			// A failed metadata lock cannot leave a route capable of reading a
			// descriptor that may already have closed. Retain for investigation.
			poisoned.store(true, std::memory_order_release);
		}
	}
	static void invalidate_locked(control *value) noexcept
	{
		value->lifecycle.store(control::phase::invalidated, std::memory_order_release);
		value->descriptor.store(nullptr, std::memory_order_release);
		release_locked(value); // Creator reference only; loans do not delay close.
	}
};

std::mutex consumable_descriptor_lifetime_registry::mutex;
consumable_descriptor_lifetime_registry::control *consumable_descriptor_lifetime_registry::head =
	nullptr;
size_t consumable_descriptor_lifetime_registry::objects = 0;
uint64_t consumable_descriptor_lifetime_registry::next_incarnation = 0;
std::atomic<bool> consumable_descriptor_lifetime_registry::poisoned{ false };

void consumable_descriptor_lifetime_hooks::created(P_desc descriptor, mm_ds *pool,
						   origin source) noexcept
{
	if (!descriptor || !pool)
		return;
	using registry = consumable_descriptor_lifetime_registry;
	try
	{
		std::lock_guard<std::mutex> lock(registry::mutex);
		// The actual allocator has issued this address again. A missing old
		// invalidation cannot redirect an older loan into the new incarnation.
		for (auto *entry = registry::head; entry;)
		{
			auto *next = entry->next;
			if (entry->descriptor.load(std::memory_order_acquire) == descriptor)
				registry::invalidate_locked(entry);
			entry = next;
		}
		if (registry::poisoned.load(std::memory_order_acquire) ||
		    registry::next_incarnation == UINT64_MAX || registry::objects == SIZE_MAX)
			return;
		const uint64_t id = ++registry::next_incarnation; // Never reset or recycled.
		auto *entry = new (std::nothrow)
			registry::control(descriptor, pool, id, static_cast<unsigned>(source));
		if (!entry)
			return; // Original connection continues; this new route is unavailable.
		entry->next = registry::head;
		if (registry::head)
			registry::head->previous = entry;
		registry::head = entry;
		++registry::objects;
	}
	catch (...)
	{
		registry::poisoned.store(true, std::memory_order_release);
	}
}

void consumable_descriptor_lifetime_hooks::invalidated(P_desc descriptor) noexcept
{
	if (!descriptor)
		return;
	using registry = consumable_descriptor_lifetime_registry;
	try
	{
		std::lock_guard<std::mutex> lock(registry::mutex);
		for (auto *entry = registry::head; entry;)
		{
			auto *next = entry->next;
			if (entry->descriptor.load(std::memory_order_acquire) == descriptor)
				registry::invalidate_locked(entry);
			entry = next;
		}
	}
	catch (...)
	{
		registry::poisoned.store(true, std::memory_order_release);
	}
}

uint64_t consumable_descriptor_lifetime_hooks::recovery_begin() noexcept
{
	using registry = consumable_descriptor_lifetime_registry;
	try
	{
		std::lock_guard<std::mutex> lock(registry::mutex);
		return registry::next_incarnation;
	}
	catch (...)
	{
		registry::poisoned.store(true, std::memory_order_release);
		return UINT64_MAX;
	}
}

void consumable_descriptor_lifetime_hooks::recovery_failed(uint64_t batch_before) noexcept
{
	using registry = consumable_descriptor_lifetime_registry;
	try
	{
		std::lock_guard<std::mutex> lock(registry::mutex);
		for (auto *entry = registry::head; entry;)
		{
			auto *next = entry->next;
			if (entry->origin == static_cast<unsigned>(origin::copyover) &&
			    entry->incarnation > batch_before &&
			    entry->lifecycle.load(std::memory_order_acquire) ==
				    registry::control::phase::created)
				registry::invalidate_locked(entry);
			entry = next;
		}
	}
	catch (...)
	{
		registry::poisoned.store(true, std::memory_order_release);
	}
}

consumable_descriptor_lifetime_loan::consumable_descriptor_lifetime_loan(control *value) noexcept
	: control_(value)
{
}
consumable_descriptor_lifetime_loan::~consumable_descriptor_lifetime_loan() noexcept
{
	consumable_descriptor_lifetime_registry::release(control_);
}

std::unique_ptr<consumable_descriptor_lifetime_loan>
consumable_descriptor_lifetime_loan::borrow(P_desc descriptor,
					    bool (*admit)(size_t, void *) noexcept, void *context,
					    size_t outer) noexcept
{
	using registry = consumable_descriptor_lifetime_registry;
	if (!descriptor || !admit || !nevent_is_game_thread() ||
	    registry::poisoned.load(std::memory_order_acquire) ||
	    sizeof(consumable_descriptor_lifetime_loan) > SIZE_MAX - outer ||
	    !admit(outer + sizeof(consumable_descriptor_lifetime_loan), context))
		return nullptr;
	control *retained = nullptr;
	try
	{
		{
			std::lock_guard<std::mutex> lock(registry::mutex);
			for (auto *entry = registry::head; entry; entry = entry->next)
				if (entry->lifecycle.load(std::memory_order_acquire) ==
					    control::phase::created &&
				    entry->descriptor.load(std::memory_order_acquire) ==
					    descriptor &&
				    entry->creator_thread == std::this_thread::get_id() &&
				    entry->pool == dead_desc_pool && registry::retain(entry))
				{
					retained = entry;
					break;
				}
		}
		if (!retained)
			return nullptr; // Unknown address: no raw descriptor read occurred.
		return std::unique_ptr<consumable_descriptor_lifetime_loan>(
			new consumable_descriptor_lifetime_loan(retained));
	}
	catch (...)
	{
		registry::release(retained);
		return nullptr;
	}
}

consumable_descriptor_lifetime_loan::scope::scope(control *value, P_desc descriptor) noexcept
	: control_(value)
	, descriptor_(descriptor)
{
}
consumable_descriptor_lifetime_loan::scope::~scope() noexcept
{
	consumable_descriptor_lifetime_registry::release(control_);
}

consumable_descriptor_lifetime_loan::scope
consumable_descriptor_lifetime_loan::enter() const noexcept
{
	using registry = consumable_descriptor_lifetime_registry;
	// Metadata is safe across async completion; test thread BEFORE loading a raw
	// pointer. Original creators/closers and supported writers run on this thread.
	if (!control_ || !nevent_is_game_thread() ||
	    control_->creator_thread != std::this_thread::get_id() ||
	    registry::poisoned.load(std::memory_order_acquire))
		return scope(nullptr, nullptr);
	try
	{
		std::lock_guard<std::mutex> lock(registry::mutex);
		if (control_->lifecycle.load(std::memory_order_acquire) !=
			    control::phase::created ||
		    control_->pool != dead_desc_pool || !registry::retain(control_))
			return scope(nullptr, nullptr);
		P_desc d = control_->descriptor.load(std::memory_order_acquire);
		return scope(control_, d);
	}
	catch (...)
	{
		return scope(nullptr, nullptr);
	}
}

P_desc consumable_descriptor_lifetime_loan::scope::descriptor() const noexcept
{
	using registry = consumable_descriptor_lifetime_registry;
	if (!control_ || !nevent_is_game_thread() ||
	    control_->creator_thread != std::this_thread::get_id() ||
	    registry::poisoned.load(std::memory_order_acquire) ||
	    control_->lifecycle.load(std::memory_order_acquire) != control::phase::created ||
	    control_->descriptor.load(std::memory_order_acquire) != descriptor_)
		return nullptr;
	return descriptor_;
}

bool consumable_descriptor_lifetime_loan::registry_storage_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
	using registry = consumable_descriptor_lifetime_registry;
	try
	{
		std::lock_guard<std::mutex> lock(registry::mutex);
		const size_t globals = sizeof(registry::mutex) + sizeof(registry::head) +
				       sizeof(registry::objects) +
				       sizeof(registry::next_incarnation) +
				       sizeof(registry::poisoned);
		if (registry::objects > (SIZE_MAX - globals) / sizeof(control))
			return false;
		*output = globals + registry::objects * sizeof(control);
		return true; // Actual retained records; allocator/Source/library are separate.
	}
	catch (...)
	{
		return false;
	}
}

namespace duris_consumable_detail
{
// Observed original fields only; no descriptor/actor clone or substitute writer.
struct consumable_plain_recipient_image
{
	int fd = -1, connected = 0, gmcp = 0;
	P_char character = nullptr, original = nullptr;
	uint64_t character_runtime_id = 0, original_runtime_id = 0;
	bool capture(P_desc d) noexcept
	{
		if (!d || !is_desc_valid(d) || d->descriptor < 0 || d->write_failed ||
		    d->network_close_pending || d->transport_session || transport_world_active() ||
		    transport_frontend_active() || d->websocket || d->sslses ||
		    d->out_compress != MCCP_NONE || d->z_str || d->out_compress_buf ||
		    d->ws_compress || d->ws_deflate_stream || d->telnet_tls_retry)
			return false;
		fd = d->descriptor;
		connected = d->connected;
		gmcp = d->gmcp_enabled;
		character = d->character;
		original = d->original;
		character_runtime_id = character ? character->runtime_id : 0;
		original_runtime_id = original ? original->runtime_id : 0;
		return (!character ||
			(character_runtime_id &&
			 find_character_by_runtime_id(character_runtime_id) == character)) &&
		       (!original ||
			(original_runtime_id &&
			 find_character_by_runtime_id(original_runtime_id) == original));
	}
	bool matches(P_desc d) const noexcept
	{
		consumable_plain_recipient_image now;
		return now.capture(d) && now.fd == fd && now.connected == connected &&
		       now.gmcp == gmcp && now.character == character && now.original == original &&
		       now.character_runtime_id == character_runtime_id &&
		       now.original_runtime_id == original_runtime_id;
	}
};
}

struct consumable_prepared_output_owner::state
{
	std::unique_ptr<consumable_descriptor_lifetime_loan> loan;
	std::unique_ptr<consumable_prepared_output_queue> queue;
	duris_consumable_detail::consumable_plain_recipient_image recipient;
	enum class phase
	{
		prepared,
		transferred
	} stage = phase::prepared;
};

consumable_prepared_output_owner::consumable_prepared_output_owner(
	std::unique_ptr<state> value) noexcept
	: state_(std::move(value))
{
}
consumable_prepared_output_owner::~consumable_prepared_output_owner() noexcept
{
	// The metadata loan may be released asynchronously. Prepared original tagged
	// queue buffers use the game's MEMCHK counters and must drain on its thread.
	if (state_ && state_->stage == state::phase::prepared && !nevent_is_game_thread())
		std::abort();
}

std::unique_ptr<consumable_prepared_output_owner> consumable_prepared_output_owner::prepare(
	P_desc descriptor, std::span<const std::string_view> messages,
	bool (*admit)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!admit || !nevent_is_game_thread())
		return nullptr;
	const size_t own = sizeof(consumable_prepared_output_owner) + sizeof(state);
	if (own > SIZE_MAX - outer || !admit(outer + own, context))
		return nullptr;
	try
	{
		auto loan = consumable_descriptor_lifetime_loan::borrow(descriptor, admit, context,
									outer + own);
		if (!loan)
			return nullptr;
		auto scope = loan->enter();
		P_desc live = scope.descriptor();
		auto value = std::make_unique<state>();
		if (!value->recipient.capture(live))
			return nullptr;
		value->loan = std::move(loan);
		value->queue = consumable_prepared_output_queue::prepare(live->output, messages);
		if (!value->queue)
			return nullptr;
		size_t queued = 0;
		if (!value->queue->owned_allocation_request_bytes(&queued) ||
		    sizeof(consumable_descriptor_lifetime_loan) > SIZE_MAX - own ||
		    queued > SIZE_MAX - own - sizeof(consumable_descriptor_lifetime_loan))
			return nullptr;
		const size_t retained = own + sizeof(consumable_descriptor_lifetime_loan) + queued;
		// Actual proposal allocations are inventoried before this route is
		// admitted. Root's separate original Source/current-G/library/peak loan
		// is still required; this declared retained request is not that proof.
		if (retained > SIZE_MAX - outer || !admit(outer + retained, context) ||
		    !value->recipient.matches(scope.descriptor()) ||
		    !value->queue->retained_exact(live->output))
			return nullptr;
		return std::unique_ptr<consumable_prepared_output_owner>(
			new consumable_prepared_output_owner(std::move(value)));
	}
	catch (...)
	{
		return nullptr;
	}
}

bool consumable_prepared_output_owner::current() const noexcept
{
	if (!state_ || state_->stage != state::phase::prepared || !state_->loan || !state_->queue)
		return false;
	auto scope = state_->loan->enter();
	P_desc live = scope.descriptor();
	return state_->recipient.matches(live) && state_->queue->retained_exact(live->output);
}

bool consumable_prepared_output_owner::transfer() noexcept
{
	if (!state_ || state_->stage != state::phase::prepared || !state_->loan || !state_->queue)
		return false;
	// One actual game-thread no-yield scope: no await, event/IPC pump, allocator,
	// recipient callback or arbitrary callable exists through this queue writer.
	auto scope = state_->loan->enter();
	P_desc live = scope.descriptor();
	if (!state_->recipient.matches(live) || !state_->queue->retained_exact(live->output))
		return false;
	if (!state_->queue->transfer(live->output))
		return false;
	state_->stage = state::phase::transferred;
	return true;
}

bool consumable_prepared_output_owner::current_storage_bytes(size_t *output) const noexcept
{
	if (!output || !state_ || !state_->loan || !state_->queue)
		return false;
	size_t queued = 0;
	const size_t own =
		sizeof(*this) + sizeof(*state_) + sizeof(consumable_descriptor_lifetime_loan);
	if (!state_->queue->owned_allocation_request_bytes(&queued) || queued > SIZE_MAX - own)
		return false;
	*output = own + queued;
	// Actual retained control records remain in the separate registry census,
	// including closed records retained by other async completions. Count once.
	return true;
}
