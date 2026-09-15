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
std::atomic<bool> rebuild_succeeds = true;
std::atomic<bool> block_load = false;
std::atomic<bool> load_entered = false;
std::atomic<bool> release_load = false;

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

bool revision_twelve_ready()
{
	return !collector_catalog_cache_busy() && runtime_revision == 12;
}

bool publication_failure_observed()
{
	const std::string status = collector_catalog_cache_status();
	return !collector_catalog_cache_busy() &&
	       status.find("publication failures ") != std::string::npos && runtime_revision == 12;
}

bool revision_thirteen_ready()
{
	return !collector_catalog_cache_busy() && runtime_revision == 13 &&
	       collector_catalog_cache_ready();
}
}

bool collector_catalog_source_load(collector_bootstrap_snapshot &snapshot, std::string &)
{
	++loads;
	const uint64_t captured_revision = source_revision.load();
	if (block_load)
	{
		load_entered = true;
		while (!release_load)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	snapshot.catalog.revision = captured_revision;
	snapshot.catalog.next_listing = 1;
	return true;
}

uint64_t collector_runtime_catalog_revision(void)
{
	return runtime_revision.load();
}

bool collector_runtime_rebuild_authoritative(const collector::catalog &catalog,
					     const item_ownership_runtime_entry *held_items,
					     size_t held_count,
					     const collector_death_snapshot *deaths,
					     size_t death_count)
{
	assert(catalog.next_listing == 1 && catalog.records.empty());
	(void)held_items;
	assert(held_count == 0);
	(void)deaths;
	assert(death_count == 0);
	++rebuilds;
	if (!rebuild_succeeds)
		return false;
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

	// Invalidating while a pre-commit read is already in flight must retain a
	// latch and begin another read after that stale generation completes.
	source_revision = 11;
	block_load = true;
	load_entered = false;
	release_load = false;
	assert(collector_catalog_cache_refresh());
	const auto entered_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!load_entered && std::chrono::steady_clock::now() < entered_deadline)
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	assert(load_entered);
	source_revision = 12;
	collector_catalog_cache_invalidate();
	assert(!collector_catalog_cache_ready());
	block_load = false;
	release_load = true;
	assert(wait_until(revision_twelve_ready));
	assert(loads == 5 && rebuilds == 4 && runtime_revision == 12);

	// A transient runtime publication failure retains the last good projection
	// for a normal refresh but retries promptly instead of waiting for the
	// ordinary five-minute refresh.  An invalidation still keeps commands gated
	// until a replacement projection is published.
	source_revision = 13;
	rebuild_succeeds = false;
	assert(collector_catalog_cache_refresh());
	assert(wait_until(publication_failure_observed));
	const unsigned int failed_rebuilds = rebuilds.load();
	rebuild_succeeds = true;
	assert(wait_until(revision_thirteen_ready));
	assert(rebuilds == failed_rebuilds + 1);

	collector_catalog_cache_shutdown();
	assert(!collector_catalog_cache_ready() && !collector_catalog_cache_busy());
}
