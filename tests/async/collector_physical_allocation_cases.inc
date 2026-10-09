// Link only with the native C++ allocation wrappers in the qualification recipe.
#ifdef DURIS_COLLECTOR_PHYSICAL_ALLOCATION_TEST
static thread_local bool collector_allocation_instrument = false;
static thread_local size_t collector_allocations = 0, collector_allocation_fault = 0;
extern "C" void *__real__Znwm(size_t);
extern "C" void *__real__Znam(size_t);
extern "C" void *__wrap__Znwm(size_t size)
{
	if (collector_allocation_instrument &&
	    ++collector_allocations == collector_allocation_fault)
		throw std::bad_alloc{};
	return __real__Znwm(size);
}
extern "C" void *__wrap__Znam(size_t size)
{
	if (collector_allocation_instrument &&
	    ++collector_allocations == collector_allocation_fault)
		throw std::bad_alloc{};
	return __real__Znam(size);
}
#endif
