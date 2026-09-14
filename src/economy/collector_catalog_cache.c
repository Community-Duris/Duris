#include "economy/collector_catalog_cache.h"

#include "core/refresh_cache.h"
#include "economy/collector_catalog_source.h"
#include "economy/collector_runtime.h"

#include <chrono>
#include <string>

namespace
{
using clock_type = std::chrono::steady_clock;
constexpr auto REFRESH_INTERVAL = std::chrono::minutes(5);
constexpr auto STALE_RETRY_INTERVAL = std::chrono::seconds(1);

refresh_cache<collector_bootstrap_snapshot> cache;
clock_type::time_point next_refresh = {};
unsigned long applied_generation = 0;
uint64_t applied_revision = 0;
size_t applied_held_items = 0;
uint64_t stale_snapshots = 0;
uint64_t publication_failures = 0;
bool ready = false;

void schedule_after(std::chrono::steady_clock::duration interval)
{
	next_refresh = clock_type::now() + interval;
}

void publish_completed_generation()
{
	const unsigned long generation = cache.generation_value();
	if (!generation || generation == applied_generation)
		return;
	const collector_bootstrap_snapshot *snapshot = cache.get();
	if (!snapshot)
		return;
	if (snapshot->catalog.revision < collector_runtime_catalog_revision())
	{
		++stale_snapshots;
		applied_generation = generation;
		schedule_after(STALE_RETRY_INTERVAL);
		return;
	}
	if (!collector_runtime_rebuild_authoritative(snapshot->catalog, snapshot->held_items.data(),
						     snapshot->held_items.size()))
	{
		++publication_failures;
		applied_generation = generation;
		return;
	}
	applied_generation = generation;
	applied_revision = snapshot->catalog.revision;
	applied_held_items = snapshot->held_items.size();
	ready = true;
}
}

bool collector_catalog_cache_refresh(void)
{
	const bool requested = cache.request(collector_catalog_source_load);
	if (requested)
		schedule_after(REFRESH_INTERVAL);
	return requested;
}

void collector_catalog_cache_pulse(void)
{
	cache.poll();
	publish_completed_generation();
	const auto now = clock_type::now();
	if (next_refresh == clock_type::time_point{})
		next_refresh = now + REFRESH_INTERVAL;
	if (now >= next_refresh && !cache.busy())
		collector_catalog_cache_refresh();
}

void collector_catalog_cache_shutdown(void)
{
	cache.shutdown();
	next_refresh = {};
	applied_generation = cache.generation_value();
	applied_revision = 0;
	applied_held_items = 0;
	stale_snapshots = 0;
	publication_failures = 0;
	ready = false;
}

bool collector_catalog_cache_ready(void)
{
	return ready;
}

bool collector_catalog_cache_busy(void)
{
	return cache.busy();
}

std::string collector_catalog_cache_status(void)
{
	return cache.status() + ", runtime " + (ready ? "ready" : "unavailable") +
	       ", loaded revision " + std::to_string(applied_revision) + ", runtime revision " +
	       std::to_string(collector_runtime_catalog_revision()) + ", held items " +
	       std::to_string(applied_held_items) + ", stale " + std::to_string(stale_snapshots) +
	       ", publication failures " + std::to_string(publication_failures);
}
