#include "account/character_identity.h"
#include "net/comm.h"
#include <memory>
#include <new>
#include <type_traits>

namespace
{
struct runtime_identity_admission
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t base;
	bool in_callback = false;
};
// Nonowning game-thread borrow, present ONLY during bounded registration.
// This same original map's allocator owns no callback, token or capability.
runtime_identity_admission *runtime_identity_active_admission = nullptr;
size_t runtime_identity_retained_heap = 0;
template <class T> struct runtime_identity_allocator
{
	using value_type = T;
	using is_always_equal = std::true_type;
	using propagate_on_container_move_assignment = std::true_type;
	runtime_identity_allocator() noexcept = default;
	template <class U>
	runtime_identity_allocator(const runtime_identity_allocator<U> &) noexcept
	{
	}
	T *allocate(size_t count)
	{
		if (count > SIZE_MAX / sizeof(T))
			throw std::bad_alloc();
		const size_t bytes = count * sizeof(T);
		if (bytes > SIZE_MAX - runtime_identity_retained_heap)
			throw std::bad_alloc();
		if (auto *owner = runtime_identity_active_admission)
		{
			if (owner->in_callback)
				throw std::bad_alloc();
			size_t current;
			if (!character_runtime_identity_storage_bytes(&current) ||
			    current > SIZE_MAX - owner->base ||
			    bytes > SIZE_MAX - owner->base - current)
				throw std::bad_alloc();
			owner->in_callback = true;
			const bool admitted =
				owner->reserve(owner->base + current + bytes, owner->context);
			owner->in_callback = false;
			if (!admitted)
				throw std::bad_alloc();
		}
		// Every actual rebound node and bucket allocation, including duplicate
		// emplace's transient node, delegates the original installed allocator.
		T *result = std::allocator<T>{}.allocate(count);
		runtime_identity_retained_heap += bytes;
		return result;
	}
	void deallocate(T *pointer, size_t count) noexcept
	{
		const size_t bytes = count * sizeof(T);
		std::allocator<T>{}.deallocate(pointer, count);
		runtime_identity_retained_heap -= bytes;
	}
	template <class U> bool operator==(const runtime_identity_allocator<U> &) const noexcept
	{
		return true;
	}
};
struct runtime_identity_borrow
{
	runtime_identity_admission &owner;
	explicit runtime_identity_borrow(runtime_identity_admission &value) noexcept
		: owner(value)
	{
		runtime_identity_active_admission = &owner;
	}
	~runtime_identity_borrow() { runtime_identity_active_admission = nullptr; }
	runtime_identity_borrow(const runtime_identity_borrow &) = delete;
	runtime_identity_borrow &operator=(const runtime_identity_borrow &) = delete;
};
}

#include "core/prototypes.h"

#include <unordered_map>

extern P_char character_list;

static uint64_t next_runtime_id = 0;
// Non-owning; publication, lookup and retirement all run on the game thread.
static std::unordered_map<uint64_t, P_char, std::hash<uint64_t>, std::equal_to<uint64_t>,
			  runtime_identity_allocator<std::pair<const uint64_t, P_char>>>
	live_characters_by_runtime_id;

uint64_t allocate_character_runtime_id()
{
	const uint64_t runtime_id = ++next_runtime_id;

	if (!runtime_id)
		panic_corruption("character", "process-local character identity exhausted");
	return runtime_id;
}

P_char find_character_by_runtime_id(uint64_t runtime_id)
{
	if (!nevent_require_game_thread("find_character_by_runtime_id") || !runtime_id)
		return NULL;
	const auto found = live_characters_by_runtime_id.find(runtime_id);
	return found == live_characters_by_runtime_id.end() ? NULL : found->second;
}

void register_character_runtime_id(P_char character)
{
	if (!nevent_require_game_thread("register_character_runtime_id"))
		return;
	if (!character || !character->runtime_id)
	{
		panic_corruption("character", "publishing a character without a runtime identity");
		return;
	}
	const auto [entry, inserted] =
		live_characters_by_runtime_id.emplace(character->runtime_id, character);
	if (!inserted && entry->second != character)
		panic_corruption("character", "duplicate live character runtime identity");
}

void unregister_character_runtime_id(P_char character)
{
	if (!nevent_require_game_thread("unregister_character_runtime_id") || !character)
		return;
	const auto found = live_characters_by_runtime_id.find(character->runtime_id);
	if (found != live_characters_by_runtime_id.end() && found->second == character)
		live_characters_by_runtime_id.erase(found);
}

// Observation-only, at a quiescent lifecycle boundary (never during construction
// or extraction). Bound the walk so a corrupt/cyclic list cannot hang diagnostics.
bool character_runtime_index_is_consistent()
{
	if (!nevent_require_game_thread("character_runtime_index_is_consistent"))
		return false;
	size_t count = 0;
	for (P_char character = character_list; character; character = character->next)
	{
		if (++count > live_characters_by_runtime_id.size() ||
		    find_character_by_runtime_id(character->runtime_id) != character)
			return false;
	}
	return count == live_characters_by_runtime_id.size();
}

bool character_runtime_identity_storage_bytes(size_t *output) noexcept
{
	if (!output || !nevent_is_game_thread())
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	return false;
#else
	// Real same-map requests, counted on every original and bounded allocation
	// and deallocation. Inline single bucket is part of sizeof(map), not heap.
	constexpr size_t own = sizeof(live_characters_by_runtime_id) + sizeof(next_runtime_id) +
			       sizeof(runtime_identity_active_admission) +
			       sizeof(runtime_identity_retained_heap);
	if (runtime_identity_retained_heap > SIZE_MAX - own)
		return false;
	*output = own + runtime_identity_retained_heap;
	return true;
#endif
}

bool register_character_runtime_id_bounded(P_char character, bool *returned, bool *registered,
					   bool (*reserve)(size_t, void *) noexcept, void *context,
					   size_t outer_live) noexcept
{
	if (!returned || !registered || returned == registered || *returned || !reserve ||
	    !nevent_is_game_thread() || runtime_identity_active_admission)
		return false;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)character;
	(void)context;
	(void)outer_live;
	return false;
#else
	struct workspace
	{
		size_t current = 0;
		runtime_identity_admission admission;
		workspace(bool (*r)(size_t, void *) noexcept, void *c) noexcept
			: admission{ r, c, 0 }
		{
		}
	};
	constexpr size_t frame = sizeof(workspace) + sizeof(runtime_identity_borrow) +
				 7 * sizeof(void *) + 4 * sizeof(size_t) + 4 * sizeof(bool);
	if (frame > SIZE_MAX - outer_live || !reserve(outer_live + frame, context))
		return false;
	workspace work(reserve, context);
	if (!character_runtime_identity_storage_bytes(&work.current) || work.current > outer_live)
		return false;
	work.admission.base = outer_live - work.current + frame;
	try
	{
		runtime_identity_borrow borrow(work.admission);
		// Complete original publication method: same thread/zero-ID/duplicate
		// panic and actual emplace. Its real allocator requests are intercepted.
		register_character_runtime_id(character);
		*returned = true;
		*registered = true;
		return true; // No fallible diagnostic after actual registration handoff.
	}
	catch (...)
	{
		return false;
	}
#endif
}
