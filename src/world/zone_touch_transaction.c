#include "world/zone_touch_transaction.h"

#include "core/structs.h"
#include "world/epic.h"
#include "redis/redis_report_cache.h"

#include <new>
#include <string>
#include <unordered_map>

namespace
{
std::unordered_map<std::string, zone_touch_payload> pending;

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}
} // namespace

bool zone_touch_transaction_submit(const zone_touch_payload &payload)
{
	if (pending.size() >= ZONE_TOUCH_PENDING_MAX)
		return false;
	critical_operation_id operation_id = {};
	critical_command command = {};
	if (!critical_operation_id_generate(&operation_id) ||
	    !zone_touch_command_build(&command, operation_id, payload))
		return false;
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, payload);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const critical_submit_result submitted =
		critical_command_coordinator_submit(std::move(command));
	if (submitted != critical_submit_result::accepted &&
	    submitted != critical_submit_result::attached)
	{
		pending.erase(key);
		return false;
	}
	return true;
}

void zone_touch_transaction_handle_completions(const critical_completion *completions, size_t count)
{
	if (count && !completions)
		return;
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end())
			continue;
		zone_touch_result result = {};
		const bool committed =
			zone_touch_command_decode_result(completions[index].result_payload.data(),
							 completions[index].result_size, &result) &&
			(completions[index].outcome == critical_apply_outcome::applied ||
			 completions[index].outcome == critical_apply_outcome::already_applied);
		if (committed)
			epic_publish_zone_touch(result);
		pending.erase(found);
	}
}

critical_outbox_delivery_result
zone_touch_transaction_outbox_delivery(const critical_outbox_record &record, void *)
{
	if (record.destination != 9 || record.event_type != 1 || record.payload_version != 1)
		return critical_outbox_delivery_result::terminal_failure;
	zone_touch_result result = {};
	if (!zone_touch_command_decode_result(record.payload.data(), record.payload.size(),
					      &result))
		return critical_outbox_delivery_result::terminal_failure;
	redis_invalidate_epic_zones();
	return critical_outbox_delivery_result::delivered;
}

void zone_touch_transaction_reset_for_tests(void)
{
	pending.clear();
}
