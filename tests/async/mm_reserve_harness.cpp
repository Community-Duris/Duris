// Focused native pool unit fixture. Every server provider is original;
// only the launcher entrypoint is renamed and mmap is fault-injected at link.
#define main duris_original_launcher_main
#include "net/comm.c"
#undef main
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <sys/mman.h>
#include <vector>

namespace
{
bool fail_mapping = false;
size_t mapping_calls = 0;
struct reserve_test_slot
{
	reserve_test_slot *next;
	std::array<unsigned char, 56> payload;
};
static_assert(sizeof(reserve_test_slot) == 64);
void require_pool(bool value, const char *message)
{
	if (!value)
		throw std::runtime_error(message);
}
std::array<unsigned char, sizeof(mm_ds)> bytes(const mm_ds &pool)
{
	std::array<unsigned char, sizeof(mm_ds)> result;
	std::memcpy(result.data(), &pool, result.size());
	return result;
}
void refused_unchanged(mm_ds &pool)
{
	const auto before = bytes(pool);
	const auto mappings = mapping_calls;
	require_pool(!mm_try_reserve_free_slot(&pool), "invalid pool accepted");
	require_pool(bytes(pool) == before, "refusal changed pool metadata");
	require_pool(mapping_calls == mappings, "invalid geometry attempted mmap");
}
}

extern "C" void *__real_mmap(void *, size_t, int, int, int, off_t);
extern "C" void *__wrap_mmap(void *address, size_t length, int protection, int flags,
			     int descriptor, off_t offset)
{
	++mapping_calls;
	if (fail_mapping)
	{
		errno = ENOMEM;
		return MAP_FAILED;
	}
	return __real_mmap(address, length, protection, flags, descriptor, offset);
}

int main()
{
	try
	{
		mm_ds *pool = mm_create("MM_TEST", sizeof(reserve_test_slot),
					offsetof(reserve_test_slot, next), 1);
		require_pool(pool && !pool->head && !pool->tail && !mm_try_get(pool),
			     "original fresh pool contract differs");
		const auto mappings = mapping_calls;
		require_pool(mm_try_reserve_free_slot(pool), "empty pool reserve refused");
		require_pool(mapping_calls == mappings + 1 && pool->pages_owned == 1 &&
				     !pool->objs_used && !pool->bytes_wasted,
			     "configured chunk statistics differ");
		const auto free_state = bytes(*pool);
		require_pool(mm_try_reserve_free_slot(pool) && bytes(*pool) == free_state &&
				     mapping_calls == mappings + 1,
			     "existing free slot grew or changed metadata");
		void *one = mm_try_get(pool);
		require_pool(one && pool->objs_used == 1, "original acquisition count differs");
		mm_release(pool, one);
		require_pool(!pool->objs_used, "original release count differs");
		std::vector<void *> held;
		const size_t capacity = 4096 / sizeof(reserve_test_slot);
		held.reserve(capacity);
		for (size_t index = 0; index < capacity; ++index)
		{
			void *value = mm_try_get(pool);
			require_pool(value != nullptr, "configured free slot absent");
			held.push_back(value);
		}
		require_pool(!pool->head && !pool->tail && !mm_try_get(pool) &&
				     pool->objs_used == capacity,
			     "exhaustion changed free-only contract");
		require_pool(mm_try_reserve_free_slot(pool) && pool->pages_owned == 2 &&
				     pool->objs_used == capacity && mapping_calls == mappings + 2,
			     "refill did not reserve exactly one configured chunk");
		for (void *value : held)
			mm_release(pool, value);
		require_pool(!pool->objs_used, "retained slots were not released exactly once");

		mm_ds empty{};
		empty.size = sizeof(reserve_test_slot);
		empty.next_off = offsetof(reserve_test_slot, next);
		empty.chunk_size = 1;
		require_pool(!mm_try_reserve_free_slot(nullptr), "null pool accepted");
		mm_ds invalid = empty;
		invalid.size = sizeof(char *) - 1;
		refused_unchanged(invalid);
		invalid = empty;
		invalid.next_off = invalid.size - sizeof(char *) + 1;
		refused_unchanged(invalid);
		invalid = empty;
		invalid.chunk_size = 0;
		refused_unchanged(invalid);
		invalid = empty;
		invalid.chunk_size = -1;
		refused_unchanged(invalid);
		invalid = empty;
		invalid.size = 4097;
		refused_unchanged(invalid);
		invalid = empty;
		invalid.tail = reinterpret_cast<char *>(pool->tail);
		refused_unchanged(invalid);
		invalid = empty;
		invalid.pages_owned = std::numeric_limits<size_t>::max();
		refused_unchanged(invalid);
		invalid = empty;
		invalid.size = 65;
		invalid.bytes_wasted = std::numeric_limits<size_t>::max();
		refused_unchanged(invalid);

		const auto failure_state = bytes(empty);
		const auto before_failure = mapping_calls;
		fail_mapping = true;
		const bool accepted = mm_try_reserve_free_slot(&empty);
		fail_mapping = false;
		require_pool(!accepted && bytes(empty) == failure_state &&
				     mapping_calls == before_failure + 1,
			     "actual injected mmap failure changed pool metadata");
		std::puts(
			"ACTUAL_MM_RESERVE_PASS existing_slot empty_chunk acquire_release refill geometry stats_overflow mmap_refusal");
		return 0;
	}
	catch (const std::exception &error)
	{
		std::fprintf(stderr, "ACTUAL_MM_RESERVE_REFUSED %s\n", error.what());
		return 2;
	}
}
