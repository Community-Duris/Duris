#include "economy/collector_config.h"

#include "core/prototypes.h"

#include <cassert>
#include <cstdarg>
#include <cstring>
#include <map>
#include <string>

namespace
{
std::map<std::string, int> properties;
size_t log_messages = 0;
}

int get_property(const char *key, int fallback, bool)
{
	const auto found = properties.find(key ? key : "");
	return found == properties.end() ? fallback : found->second;
}

void logit(const char *, const char *, ...)
{
	++log_messages;
}

int main()
{
	collector_config_reset_for_tests();
	const collector_feature_config *config = collector_config_get();
	assert(config);
	assert(!config->policy.enabled);
	assert(config->policy.collection_delay == 12 * 60 * 60);
	assert(config->policy.sale_delay == 24 * 60 * 60);
	assert(config->policy.holding_duration == 7 * 24 * 60 * 60);
	assert(config->policy.price_percent == 200);
	assert(config->policy.minimum_value == 100);
	assert(config->maintenance_interval_seconds == 60);
	assert(config->maintenance_lease_seconds == 120);
	assert(config->maintenance_batch_limit == 32);
	assert(config->enabled_changed_at > 0);
	assert(config->revision == 1);
	const uint64_t default_revision = config->revision;
	const uint64_t default_enabled_changed_at = config->enabled_changed_at;
	collector_config_reload();
	assert(collector_config_revision() == default_revision);
	assert(collector_config_get()->enabled_changed_at == default_enabled_changed_at);

	properties = {
		{ "collector.enabled", 1 },
		{ "collector.collection.delay.seconds", 3600 },
		{ "collector.sale.delay.seconds", 7200 },
		{ "collector.holding.duration.seconds", 9000 },
		{ "collector.price.percent", 250 },
		{ "collector.minimum.value.copper", 300 },
		{ "collector.maintenance.interval.seconds", 30 },
		{ "collector.maintenance.lease.seconds", 45 },
		{ "collector.maintenance.batch.limit", 7 },
	};
	collector_config_reload();
	config = collector_config_get();
	assert(config->policy.enabled);
	assert(config->policy.collection_delay == 3600);
	assert(config->policy.sale_delay == 7200);
	assert(config->policy.holding_duration == 9000);
	assert(config->policy.price_percent == 250);
	assert(config->policy.minimum_value == 300);
	assert(config->maintenance_interval_seconds == 30);
	assert(config->maintenance_lease_seconds == 45);
	assert(config->maintenance_batch_limit == 7);
	assert(config->revision == default_revision + 1);
	assert(config->enabled_changed_at >= default_enabled_changed_at);
	const uint64_t custom_revision = config->revision;
	const uint64_t enabled_changed_at = config->enabled_changed_at;

	properties["collector.sale.delay.seconds"] = 1800;
	properties["collector.maintenance.lease.seconds"] = 5;
	collector_config_reload();
	config = collector_config_get();
	assert(config->policy.enabled);
	assert(config->policy.collection_delay == 12 * 60 * 60);
	assert(config->policy.sale_delay == 24 * 60 * 60);
	assert(config->maintenance_interval_seconds == 30);
	assert(config->maintenance_lease_seconds == 30);
	assert(config->revision == custom_revision + 1);
	assert(config->enabled_changed_at == enabled_changed_at);

	properties["collector.enabled"] = 2;
	properties["collector.maintenance.interval.seconds"] = 90;
	collector_config_reload();
	config = collector_config_get();
	assert(!config->policy.enabled);
	assert(config->maintenance_interval_seconds == 60);
	assert(log_messages >= 5);
	collector_config_reset_for_tests();
}
