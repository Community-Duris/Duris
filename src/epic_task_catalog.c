#include "epic_task_catalog.h"

#include "prototypes.h"
#include "sql.h"
#include "utils.h"

#include <cerrno>
#include <cstdlib>
#include <new>
#include <vector>

namespace
{
std::vector<int32_t> task_zones;
bool ready = false;

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
	if (count > EPIC_TASK_CATALOG_MAX || (count && !zone_numbers))
		return false;
	std::vector<int32_t> candidate;
	try
	{
		candidate.reserve(count);
		for (size_t index = 0; index < count; ++index)
		{
			if (zone_numbers[index] <= 0 ||
			    (index && zone_numbers[index - 1] >= zone_numbers[index]) ||
			    real_zone0(zone_numbers[index]) < 0)
				return false;
			candidate.push_back(zone_numbers[index]);
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	task_zones.swap(candidate);
	ready = true;
	return true;
}

bool epic_task_catalog_refresh(void)
{
#ifdef __NO_MYSQL__
	return false;
#else
	if (!qry("SELECT number FROM zones WHERE task_zone=1 ORDER BY number"))
		return false;
	MYSQL_RES *rows = mysql_store_result(DB);
	if (!rows)
		return false;
	std::vector<int32_t> candidate;
	bool valid = true;
	try
	{
		const my_ulonglong row_count = mysql_num_rows(rows);
		if (row_count > EPIC_TASK_CATALOG_MAX)
			valid = false;
		else
			candidate.reserve(static_cast<size_t>(row_count));
		MYSQL_ROW row = nullptr;
		while (valid && (row = mysql_fetch_row(rows)))
		{
			int32_t zone_number = 0;
			if (!parse_zone(row[0], &zone_number) ||
			    (!candidate.empty() && candidate.back() >= zone_number) ||
			    real_zone0(zone_number) < 0)
				valid = false;
			else
				candidate.push_back(zone_number);
		}
	}
	catch (const std::bad_alloc &)
	{
		valid = false;
	}
	mysql_free_result(rows);
	return valid && epic_task_catalog_publish(candidate.data(), candidate.size());
#endif
}

int epic_task_catalog_select(const gameplay_read_state *state)
{
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
	ready = false;
}
