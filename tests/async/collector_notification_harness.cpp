#include "core/prototypes.h"
#include "economy/collector_notification.h"
#include "economy/collector_runtime.h"
#include "economy/collector_transaction.h"
#include "sql/sql.h"

#include <cassert>
#include <string>
#include <vector>

bool collector_config_enabled(void)
{
	return true;
}

bool collector_runtime_find(uint64_t, collector::record *)
{
	return false;
}

bool collector_runtime_find_death(uint32_t, uint64_t, collector_death_snapshot *)
{
	return false;
}

bool collector_runtime_hint_candidates(uint64_t, size_t, size_t, uint64_t,
					       std::vector<collector::record> *, uint64_t *, bool *)
{
	return false;
}

bool collector_transaction_listing_busy(uint64_t)
{
	return false;
}

bool collector_transaction_submit_background(const collector_command_payload &,
					     collector_completion_fn)
{
	return false;
}

bool send_to_pid(const char *, int)
{
	return false;
}

bool send_to_pid_offline_deduplicated(const char *, int, const unsigned char *)
{
	return false;
}

int main()
{
	collector::rules policy;
	policy.enabled = true;
	collector::record entry;
	assert(collector::enroll(77, "0123456789abcdef0123456789abcdef", 42, 9001, 3, 1000,
					 policy, &entry) == collector::outcome::applied);
	entry.status = collector::state::available;
	entry.available_at = entry.sale_at;
	entry.expires_at = entry.available_at + entry.policy.holding_duration;
	entry.revision = 4;
	entry.price_value = 123456;
	entry.holding_paused = false;

	std::string message;
	assert(collector_notification_format(entry, &message));
	assert(message.find("Collector of Antiquities") != std::string::npos);
	assert(message.find("registered auction room") != std::string::npos);
	assert(message.find("collector list") != std::string::npos);
	assert(message.find("collector inspect <number>") != std::string::npos);
	assert(message.find("collector buy <number>") != std::string::npos);
	assert(message.find("Carry the fee: 123456 copper") != std::string::npos);
	assert(message.find("Holding deadline: " + std::to_string(entry.expires_at) +
					" (Unix seconds)") != std::string::npos);
	assert(message.find(entry.death_operation.data()) == std::string::npos);
	assert(message.find("death room") == std::string::npos);
	assert(message.find("other player") == std::string::npos);
	assert(message.find("artifact") == std::string::npos);
	return 0;
}
