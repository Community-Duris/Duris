#include "world/epic_task_catalog.h"

#include "core/prototypes.h"
#include "sql/sql.h"
#include "core/utils.h"
#include "core/files.h"
#include "persistence/maintenance_scheduler.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

namespace
{
std::vector<int32_t> task_zones;
bool ready = false;
std::vector<int32_t> refresh_zones;
bool refresh_invalid = false;
const char *last_refresh = "not_loaded";
size_t last_candidate_count = 0;

bool report_refresh(const char *result, size_t count)
{
	if (strcmp(last_refresh, result) || last_candidate_count != count)
		logit(LOG_STATUS,
		      "Epic task catalog: result=%s candidate_zones=%zu catalog_ready=%d "
		      "active_zones=%zu; see docs/operations/EPIC_TASK_ZONES.md",
		      result, count, ready, task_zones.size());
	last_refresh = result;
	last_candidate_count = count;
	return !strcmp(result, "ready");
}

#ifndef __NO_MYSQL__
bool parse_zone(const char *value, int32_t *zone_number)
{
	if (!value || !*value || !zone_number)
		return false;
	errno = 0;
	char *end = nullptr;
	const long parsed = strtol(value, &end, 10);
	if (errno || !end || *end || parsed <= 0 || parsed > INT32_MAX)
		return false;
	*zone_number = static_cast<int32_t>(parsed);
	return true;
}
#endif
}

bool epic_task_catalog_publish(const int32_t *zone_numbers, size_t count)
{
	if (!count)
		return report_refresh("empty_task_flags", count);
	if (count > EPIC_TASK_CATALOG_MAX)
		return report_refresh("too_many_zones", count);
	if (!zone_numbers)
		return report_refresh("missing_zone_values", count);
	std::vector<int32_t> candidate;
	try
	{
		candidate.reserve(count);
		for (size_t index = 0; index < count; ++index)
		{
			if (zone_numbers[index] <= 0)
				return report_refresh("invalid_zone_number", count);
			if (index && zone_numbers[index - 1] >= zone_numbers[index])
				return report_refresh("duplicate_or_unordered_zone", count);
			if (real_zone0(zone_numbers[index]) < 0)
				return report_refresh("unloaded_zone", count);
			candidate.push_back(zone_numbers[index]);
		}
	}
	catch (const std::bad_alloc &)
	{
		return report_refresh("allocation_failure", count);
	}
	task_zones.swap(candidate);
	ready = true;
	return report_refresh("ready", count);
}

bool epic_task_catalog_refresh(void)
{
#ifdef __NO_MYSQL__
	return report_refresh("database_disabled", 0);
#else
	if (!qry("SELECT number FROM zones WHERE task_zone=1 ORDER BY number"))
		return report_refresh("database_query_failed", 0);
	MYSQL_RES *rows = mysql_store_result(DB);
	if (!rows)
		return report_refresh("database_result_failed", 0);
	std::vector<int32_t> candidate;
	const char *error = nullptr;
	const my_ulonglong row_count = mysql_num_rows(rows);
	try
	{
		if (row_count > EPIC_TASK_CATALOG_MAX)
			error = "too_many_zones";
		else
			candidate.reserve(static_cast<size_t>(row_count));
		MYSQL_ROW row = nullptr;
		while (!error && (row = mysql_fetch_row(rows)))
		{
			int32_t zone_number = 0;
			if (!parse_zone(row[0], &zone_number))
				error = "invalid_zone_number";
			else
				candidate.push_back(zone_number);
		}
	}
	catch (const std::bad_alloc &)
	{
		error = "allocation_failure";
	}
	mysql_free_result(rows);
	return error ? report_refresh(error, static_cast<size_t>(row_count)) :
		       epic_task_catalog_publish(candidate.data(), candidate.size());
#endif
}

void epic_task_catalog_complete(const maintenance_result &result)
{
	if (result.job_id != maintenance_job_id::epic_task_catalog)
		return;
	if (result.outcome == maintenance_outcome::retryable_failure)
	{
		// The scheduler retries the same cursor; retain all preceding pages.
		report_refresh("scheduled_read_failed", refresh_zones.size());
		return;
	}
	if (result.outcome != maintenance_outcome::complete &&
	    result.outcome != maintenance_outcome::more)
	{
		report_refresh(result.error_code == EINVAL ? "invalid_scheduled_rows" :
							     "scheduled_read_abandoned",
			       refresh_zones.size());
		refresh_zones.clear();
		refresh_invalid = false;
		return;
	}
	if (!refresh_invalid)
	{
		if (result.value_count > result.values.size() ||
		    refresh_zones.size() + result.value_count > EPIC_TASK_CATALOG_MAX)
			refresh_invalid = true;
		else
			try
			{
				for (size_t index = 0; index < result.value_count; ++index)
				{
					const auto value = result.values[index];
					if (value <= 0 || value > INT32_MAX ||
					    (!refresh_zones.empty() &&
					     refresh_zones.back() >= value))
					{
						refresh_invalid = true;
						break;
					}
					refresh_zones.push_back(static_cast<int32_t>(value));
				}
			}
			catch (const std::bad_alloc &)
			{
				refresh_invalid = true;
			}
		if (refresh_invalid)
			report_refresh("invalid_scheduled_page", refresh_zones.size());
	}
	if (result.outcome == maintenance_outcome::complete)
	{
		// A rejected prefix must never be replaced by a valid-looking suffix.
		if (!refresh_invalid)
			epic_task_catalog_publish(refresh_zones.data(), refresh_zones.size());
		refresh_zones.clear();
		refresh_invalid = false;
	}
}

const char *epic_task_catalog_last_refresh(void)
{
	return last_refresh;
}

int epic_task_catalog_select(const gameplay_read_state *state, epic_task_selection_status *status)
{
	if (status)
		*status = epic_task_selection_status::unavailable;
	if (!ready || !state || state->status != gameplay_read_status::ready)
		return -1;
	int selected = -1;
	int eligible = 0;
	for (int32_t zone_number : task_zones)
		if (!gameplay_read_state_zone_completed(state, zone_number))
		{
			++eligible;
			if (number(1, eligible) == 1)
				selected = zone_number;
		}
	if (status)
		*status = selected < 0 ? epic_task_selection_status::exhausted :
					 epic_task_selection_status::selected;
	return selected;
}

size_t epic_task_catalog_size(void)
{
	return task_zones.size();
}

bool epic_task_catalog_ready(void)
{
	return ready;
}

void epic_task_catalog_reset_for_tests(void)
{
	task_zones.clear();
	refresh_zones.clear();
	refresh_invalid = false;
	last_refresh = "not_loaded";
	last_candidate_count = 0;
	ready = false;
}
