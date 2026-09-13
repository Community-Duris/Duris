#include <chrono>
#include <ctime>
#include <new>
#include <iostream>

static bool benchmark_counting = false;
static uint64_t benchmark_allocations = 0, benchmark_allocated_bytes = 0;
extern "C" void *__real_malloc(size_t);
extern "C" void *__real_calloc(size_t, size_t);
extern "C" void *__real_realloc(void *, size_t);
static void record_allocation(size_t bytes)
{
	if (benchmark_counting)
	{
		++benchmark_allocations;
		benchmark_allocated_bytes += bytes;
	}
}
extern "C" void *__wrap_malloc(size_t bytes)
{
	record_allocation(bytes);
	return __real_malloc(bytes);
}
extern "C" void *__wrap_calloc(size_t n, size_t bytes)
{
	record_allocation(n * bytes);
	return __real_calloc(n, bytes);
}
extern "C" void *__wrap_realloc(void *ptr, size_t bytes)
{
	record_allocation(bytes);
	return __real_realloc(ptr, bytes);
}
void *operator new(size_t bytes)
{
	record_allocation(bytes);
	if (void *ptr = __real_malloc(bytes))
		return ptr;
	throw std::bad_alloc();
}
void *operator new[](size_t bytes)
{
	return ::operator new(bytes);
}
void operator delete(void *ptr) noexcept
{
	std::free(ptr);
}
void operator delete[](void *ptr) noexcept
{
	std::free(ptr);
}
void operator delete(void *ptr, size_t) noexcept
{
	std::free(ptr);
}
void operator delete[](void *ptr, size_t) noexcept
{
	std::free(ptr);
}
static void *benchmark_json_malloc(size_t bytes)
{
	record_allocation(bytes);
	return __real_malloc(bytes);
}
