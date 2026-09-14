#include "economy/collector_catalog_cache.h"
#include "economy/collector_catalog_source.h"
#include "economy/collector_runtime.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <string>
#include <thread>

namespace
{
std::atomic<uint64_t> source_revision = 0;
std::atomic<uint64_t> runtime_revision = 0;
std::atomic<unsigned int> loads = 0;
std::atomic<unsigned int> rebuilds = 0;

bool wait_until(bool (*predicate)())
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (std::chrono::steady_clock::now() < deadline)
	{
		collector_catalog_cache_pulse();
		if (predicate())
			return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return false;
}

bool cache_ready()
{
	return collector_catalog_cache_ready() && !collector_catalog_cache_busy();
}

bool stale_observed()
{
	return !collector_catalog_cache_busy() &&
	       collector_catalog_cache_status().find("stale 1") != std::string::npos;
}
}

bool collector_catalog_source_load(collector::catalog &catalog, std::string &)
{
	++loads;
	catalog.revision = source_revision.load();
	catalog.next_listing = 1;
	return true;
}

uint64_t collector_runtime_catalog_revision(void)
{
	return runtime_revision.load();
}

bool collector_runtime_rebuild(const collector::catalog &catalog)
{
	assert(catalog.next_listing == 1 && catalog.records.empty());
	++rebuilds;
	runtime_revision = catalog.revision;
	return true;
}

int main()
{
	source_revision = 5;
	assert(collector_catalog_cache_refresh());
	assert(wait_until(cache_ready));
	assert(loads == 1 && rebuilds == 1 && runtime_revision == 5);
	assert(collector_catalog_cache_status().find("loaded revision 5") != std::string::npos);

	// A read started before a newer outbox publication must not roll runtime back.
	runtime_revision = 9;
	source_revision = 8;
	assert(collector_catalog_cache_refresh());
	assert(wait_until(stale_observed));
	assert(loads == 2 && rebuilds == 1 && runtime_revision == 9);

	source_revision = 10;
	assert(collector_catalog_cache_refresh());
	assert(wait_until(cache_ready));
	assert(loads == 3 && rebuilds == 2 && runtime_revision == 10);

	collector_catalog_cache_shutdown();
	assert(!collector_catalog_cache_ready() && !collector_catalog_cache_busy());
}
