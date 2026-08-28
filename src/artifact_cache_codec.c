#include "artifact_cache_codec.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace
{
constexpr int MAX_ARTIFACT_CACHE_ITEMS = 4096;
constexpr size_t MAX_ARTIFACT_DESCRIPTION_BYTES = 4096;
constexpr size_t MAX_ARTIFACT_OWNER_BYTES = 256;
constexpr size_t MAX_ARTIFACT_TIMESTAMP_BYTES = 128;

bool integer_in_range(const cJSON *item, double minimum, double maximum)
{
	if (!cJSON_IsNumber(item) || !std::isfinite(item->valuedouble) ||
	    std::trunc(item->valuedouble) != item->valuedouble)
		return false;
	return item->valuedouble >= minimum && item->valuedouble <= maximum;
}

bool bounded_string(const cJSON *item, size_t maximum)
{
	return cJSON_IsString(item) && item->valuestring &&
	       strnlen(item->valuestring, maximum + 1) <= maximum;
}

bool artifact_item_valid(const cJSON *item, bool godlist)
{
	if (!cJSON_IsObject(item))
		return false;

	const cJSON *vnum = cJSON_GetObjectItemCaseSensitive(item, "vnum");
	const cJSON *location = cJSON_GetObjectItemCaseSensitive(item, "location");
	const cJSON *loc_type = cJSON_GetObjectItemCaseSensitive(item, "locType");
	const cJSON *owned = cJSON_GetObjectItemCaseSensitive(item, "owned");
	const cJSON *short_desc = cJSON_GetObjectItemCaseSensitive(item, "shortDesc");
	const cJSON *racewar = cJSON_GetObjectItemCaseSensitive(item, "racewar");
	if (!integer_in_range(vnum, 1, std::numeric_limits<int>::max()) ||
	    !integer_in_range(location, std::numeric_limits<int>::min(),
			      std::numeric_limits<int>::max()) ||
	    !integer_in_range(loc_type, 1, 5) || !cJSON_IsBool(owned) ||
	    !bounded_string(short_desc, MAX_ARTIFACT_DESCRIPTION_BYTES) ||
	    !integer_in_range(racewar, 0, 4))
		return false;

	const cJSON *owner_name = cJSON_GetObjectItemCaseSensitive(item, "ownerName");
	if (owner_name && !bounded_string(owner_name, MAX_ARTIFACT_OWNER_BYTES))
		return false;

	if (godlist)
	{
		const cJSON *timer = cJSON_GetObjectItemCaseSensitive(item, "timer");
		const cJSON *last_update = cJSON_GetObjectItemCaseSensitive(item, "lastUpdate");
		if (!integer_in_range(timer, 0,
				      static_cast<double>(std::numeric_limits<int64_t>::max())) ||
		    !bounded_string(last_update, MAX_ARTIFACT_TIMESTAMP_BYTES))
			return false;
	}

	return true;
}
} // namespace

bool artifact_cache_payload_valid(const cJSON *root, int expected_type, bool expected_godlist)
{
	if (!cJSON_IsObject(root) || expected_type < 1 || expected_type > 3)
		return false;

	const cJSON *schema = cJSON_GetObjectItemCaseSensitive(root, "schema_version");
	const cJSON *type = cJSON_GetObjectItemCaseSensitive(root, "type");
	const cJSON *godlist = cJSON_GetObjectItemCaseSensitive(root, "godlist");
	const cJSON *artifacts = cJSON_GetObjectItemCaseSensitive(root, "artifacts");
	if (!integer_in_range(schema, ARTIFACT_CACHE_SCHEMA_VERSION,
			      ARTIFACT_CACHE_SCHEMA_VERSION) ||
	    !integer_in_range(type, expected_type, expected_type) || !cJSON_IsBool(godlist) ||
	    (cJSON_IsTrue(godlist) != expected_godlist) || !cJSON_IsArray(artifacts) ||
	    cJSON_GetArraySize(artifacts) > MAX_ARTIFACT_CACHE_ITEMS)
		return false;

	const cJSON *item = nullptr;
	cJSON_ArrayForEach(item, artifacts)
	{
		if (!artifact_item_valid(item, expected_godlist))
			return false;
	}
	return true;
}
