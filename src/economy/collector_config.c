#include "economy/collector_config.h"

#include "core/prototypes.h"

#include <ctime>
#include <limits>

namespace
{
constexpr uint64_t DEFAULT_COLLECTION_DELAY = 12 * 60 * 60;
constexpr uint64_t DEFAULT_SALE_DELAY = 24 * 60 * 60;
constexpr uint64_t DEFAULT_HOLDING_DURATION = 7 * 24 * 60 * 60;
constexpr uint64_t DEFAULT_PRICE_PERCENT = 200;
constexpr uint64_t DEFAULT_MINIMUM_VALUE = 100;
constexpr uint64_t DEFAULT_MAINTENANCE_INTERVAL = 60;
constexpr uint64_t DEFAULT_MAINTENANCE_LEASE = 120;
constexpr size_t DEFAULT_MAINTENANCE_BATCH = 32;

collector_feature_config active = {};
bool initialized = false;

int bounded_property(const char *key, int fallback, int minimum, int maximum)
{
	const int value = get_property(key, fallback, false);
	if (value >= minimum && value <= maximum)
		return value;
	logit(LOG_STATUS, "Invalid collector property %s=%d; using %d.", key, value, fallback);
	return fallback;
}

collector_feature_config defaults()
{
	collector_feature_config result;
	result.policy.enabled = false;
	result.policy.collection_delay = DEFAULT_COLLECTION_DELAY;
	result.policy.sale_delay = DEFAULT_SALE_DELAY;
	result.policy.holding_duration = DEFAULT_HOLDING_DURATION;
	result.policy.price_percent = DEFAULT_PRICE_PERCENT;
	result.policy.minimum_value = DEFAULT_MINIMUM_VALUE;
	result.maintenance_interval_seconds = DEFAULT_MAINTENANCE_INTERVAL;
	result.maintenance_lease_seconds = DEFAULT_MAINTENANCE_LEASE;
	result.maintenance_batch_limit = DEFAULT_MAINTENANCE_BATCH;
	return result;
}

bool same_policy(const collector::rules &left, const collector::rules &right)
{
	return left.enabled == right.enabled && left.collection_delay == right.collection_delay &&
	       left.sale_delay == right.sale_delay &&
	       left.holding_duration == right.holding_duration &&
	       left.price_percent == right.price_percent &&
	       left.minimum_value == right.minimum_value;
}

bool same_config(const collector_feature_config &left, const collector_feature_config &right)
{
	return same_policy(left.policy, right.policy) &&
	       left.maintenance_interval_seconds == right.maintenance_interval_seconds &&
	       left.maintenance_lease_seconds == right.maintenance_lease_seconds &&
	       left.maintenance_batch_limit == right.maintenance_batch_limit;
}

uint64_t wall_clock_now()
{
	const std::time_t now = std::time(nullptr);
	return now > 0 ? static_cast<uint64_t>(now) : 1;
}
}

void collector_config_reload(void)
{
	collector_feature_config candidate = defaults();
	candidate.policy.enabled = bounded_property("collector.enabled", 0, 0, 1) == 1;
	candidate.policy.collection_delay = static_cast<uint64_t>(
		bounded_property("collector.collection.delay.seconds",
				 static_cast<int>(DEFAULT_COLLECTION_DELAY), 1, 30 * 24 * 60 * 60));
	candidate.policy.sale_delay = static_cast<uint64_t>(
		bounded_property("collector.sale.delay.seconds",
				 static_cast<int>(DEFAULT_SALE_DELAY), 1, 30 * 24 * 60 * 60));
	candidate.policy.holding_duration = static_cast<uint64_t>(bounded_property(
		"collector.holding.duration.seconds", static_cast<int>(DEFAULT_HOLDING_DURATION), 1,
		365 * 24 * 60 * 60));
	candidate.policy.price_percent = static_cast<uint64_t>(bounded_property(
		"collector.price.percent", static_cast<int>(DEFAULT_PRICE_PERCENT), 1, 100000));
	candidate.policy.minimum_value = static_cast<uint64_t>(
		bounded_property("collector.minimum.value.copper",
				 static_cast<int>(DEFAULT_MINIMUM_VALUE), 1, 1000000000));
	candidate.maintenance_interval_seconds = static_cast<uint64_t>(
		bounded_property("collector.maintenance.interval.seconds",
				 static_cast<int>(DEFAULT_MAINTENANCE_INTERVAL), 1, 60));
	candidate.maintenance_lease_seconds = static_cast<uint64_t>(
		bounded_property("collector.maintenance.lease.seconds",
				 static_cast<int>(DEFAULT_MAINTENANCE_LEASE), 1, 3600));
	candidate.maintenance_batch_limit = static_cast<size_t>(
		bounded_property("collector.maintenance.batch.limit",
				 static_cast<int>(DEFAULT_MAINTENANCE_BATCH), 1, 128));

	if (candidate.policy.sale_delay < candidate.policy.collection_delay)
	{
		logit(LOG_STATUS,
		      "Collector sale delay precedes collection delay; using 12h/24h defaults.");
		candidate.policy.collection_delay = DEFAULT_COLLECTION_DELAY;
		candidate.policy.sale_delay = DEFAULT_SALE_DELAY;
	}
	if (candidate.maintenance_lease_seconds < candidate.maintenance_interval_seconds)
	{
		logit(LOG_STATUS,
		      "Collector maintenance lease is shorter than its interval; using %llu seconds.",
		      static_cast<unsigned long long>(candidate.maintenance_interval_seconds));
		candidate.maintenance_lease_seconds = candidate.maintenance_interval_seconds;
	}
	if (!collector::valid_rules(candidate.policy))
	{
		const bool enabled = candidate.policy.enabled;
		candidate = defaults();
		candidate.policy.enabled = enabled;
	}

	const bool changed = !initialized || !same_config(active, candidate);
	if (!changed)
	{
		candidate.enabled_changed_at = active.enabled_changed_at;
		candidate.revision = active.revision;
		active = candidate;
		return;
	}
	candidate.enabled_changed_at = !initialized || active.policy.enabled !=
							       candidate.policy.enabled ?
					       wall_clock_now() :
					       active.enabled_changed_at;
	if (!initialized)
		candidate.revision = 1;
	else if (active.revision == std::numeric_limits<uint64_t>::max())
		candidate.revision = active.revision;
	else
		candidate.revision = active.revision + 1;
	active = candidate;
	initialized = true;
	logit(LOG_STATUS,
	      "Loaded collector config revision %llu: %s, collect %llus, sale %llus, hold %llus, price %llu%% (minimum %llu copper).",
	      static_cast<unsigned long long>(active.revision),
	      active.policy.enabled ? "enabled" : "disabled",
	      static_cast<unsigned long long>(active.policy.collection_delay),
	      static_cast<unsigned long long>(active.policy.sale_delay),
	      static_cast<unsigned long long>(active.policy.holding_duration),
	      static_cast<unsigned long long>(active.policy.price_percent),
	      static_cast<unsigned long long>(active.policy.minimum_value));
}

const collector_feature_config *collector_config_get(void)
{
	if (!initialized)
		collector_config_reload();
	return &active;
}

bool collector_config_enabled(void)
{
	return collector_config_get()->policy.enabled;
}

uint64_t collector_config_revision(void)
{
	return collector_config_get()->revision;
}

void collector_config_reset_for_tests(void)
{
	active = {};
	initialized = false;
}
