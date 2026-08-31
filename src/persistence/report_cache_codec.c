#include "persistence/report_cache_codec.h"

#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace
{
constexpr const char *schema = "FRC1|";
constexpr size_t maximum_component_bytes = 65535;
constexpr size_t maximum_payload_bytes = 1024 * 1024;

uint64_t content_revision(const char *prefix, size_t prefix_size, const char *suffix,
			  size_t suffix_size)
{
	uint64_t hash = 14695981039346656037ULL;
	for (size_t index = 0; index < prefix_size; ++index)
	{
		hash ^= static_cast<unsigned char>(prefix[index]);
		hash *= 1099511628211ULL;
	}
	hash ^= 0xff;
	hash *= 1099511628211ULL;
	for (size_t index = 0; index < suffix_size; ++index)
	{
		hash ^= static_cast<unsigned char>(suffix[index]);
		hash *= 1099511628211ULL;
	}
	return hash;
}

bool parse_number(const char **cursor, const char *end, char delimiter, uint64_t *value)
{
	if (!cursor || !*cursor || !value || *cursor >= end)
		return false;
	const char *field_end = static_cast<const char *>(
		memchr(*cursor, delimiter, static_cast<size_t>(end - *cursor)));
	if (!field_end || field_end == *cursor)
		return false;
	uint64_t parsed = 0;
	const auto result = std::from_chars(*cursor, field_end, parsed);
	if (result.ec != std::errc() || result.ptr != field_end)
		return false;
	*cursor = field_end + 1;
	*value = parsed;
	return true;
}
} // namespace

char *report_cache_countdown_encode(const char *prefix, const char *suffix, uint64_t generated_at,
				    uint64_t deadline)
{
	if (!prefix || !suffix || !generated_at || !deadline)
		return nullptr;
	const size_t prefix_size = strnlen(prefix, maximum_component_bytes + 1);
	const size_t suffix_size = strnlen(suffix, maximum_component_bytes + 1);
	if (prefix_size > maximum_component_bytes || suffix_size > maximum_component_bytes)
		return nullptr;
	const uint64_t revision = content_revision(prefix, prefix_size, suffix, suffix_size);
	char header[160];
	const int header_size = snprintf(header, sizeof header, "FRC1|%llu|%llu|%llu|%zu|%zu\n",
					 static_cast<unsigned long long>(generated_at),
					 static_cast<unsigned long long>(revision),
					 static_cast<unsigned long long>(deadline), prefix_size,
					 suffix_size);
	if (header_size <= 0 || static_cast<size_t>(header_size) >= sizeof header ||
	    static_cast<size_t>(header_size) > maximum_payload_bytes - prefix_size ||
	    static_cast<size_t>(header_size) + prefix_size > maximum_payload_bytes - suffix_size)
		return nullptr;
	const size_t total_size = static_cast<size_t>(header_size) + prefix_size + suffix_size;
	char *payload = static_cast<char *>(malloc(total_size + 1));
	if (!payload)
		return nullptr;
	memcpy(payload, header, static_cast<size_t>(header_size));
	memcpy(payload + header_size, prefix, prefix_size);
	memcpy(payload + header_size + prefix_size, suffix, suffix_size);
	payload[total_size] = '\0';
	return payload;
}

char *report_cache_countdown_render(const char *payload, uint64_t now, uint64_t maximum_age_seconds)
{
	if (!payload || !now || !maximum_age_seconds)
		return nullptr;
	const size_t payload_size = strnlen(payload, maximum_payload_bytes + 1);
	if (payload_size > maximum_payload_bytes || payload_size < strlen(schema) ||
	    strncmp(payload, schema, strlen(schema)))
		return nullptr;
	const char *cursor = payload + strlen(schema);
	const char *end = payload + payload_size;
	uint64_t generated_at = 0;
	uint64_t revision = 0;
	uint64_t deadline = 0;
	uint64_t prefix_size = 0;
	uint64_t suffix_size = 0;
	if (!parse_number(&cursor, end, '|', &generated_at) ||
	    !parse_number(&cursor, end, '|', &revision) ||
	    !parse_number(&cursor, end, '|', &deadline) ||
	    !parse_number(&cursor, end, '|', &prefix_size) ||
	    !parse_number(&cursor, end, '\n', &suffix_size) ||
	    (generated_at > now && generated_at - now > REPORT_CACHE_CLOCK_SKEW_SECONDS) ||
	    (now > generated_at && now - generated_at > maximum_age_seconds) ||
	    prefix_size > maximum_component_bytes || suffix_size > maximum_component_bytes ||
	    prefix_size > static_cast<uint64_t>(end - cursor) ||
	    suffix_size != static_cast<uint64_t>(end - cursor) - prefix_size)
		return nullptr;
	const char *prefix = cursor;
	const char *suffix = cursor + prefix_size;
	if (content_revision(prefix, static_cast<size_t>(prefix_size), suffix,
			     static_cast<size_t>(suffix_size)) != revision)
		return nullptr;
	const uint64_t remaining = deadline > now ? deadline - now : 0;
	const uint64_t days = remaining / 86400;
	const uint64_t hours = remaining / 3600 % 24;
	const uint64_t minutes = remaining / 60 % 60;
	const uint64_t seconds = remaining % 60;
	char countdown[64];
	const int countdown_size = snprintf(
		countdown, sizeof countdown, "%02llu:%02llu:%02llu:%02llu",
		static_cast<unsigned long long>(days), static_cast<unsigned long long>(hours),
		static_cast<unsigned long long>(minutes), static_cast<unsigned long long>(seconds));
	if (countdown_size <= 0 || static_cast<size_t>(countdown_size) >= sizeof countdown ||
	    prefix_size >
		    std::numeric_limits<size_t>::max() - static_cast<size_t>(countdown_size) ||
	    static_cast<size_t>(prefix_size) + static_cast<size_t>(countdown_size) >
		    std::numeric_limits<size_t>::max() - static_cast<size_t>(suffix_size))
		return nullptr;
	const size_t rendered_size = static_cast<size_t>(prefix_size) +
				     static_cast<size_t>(countdown_size) +
				     static_cast<size_t>(suffix_size);
	char *rendered = static_cast<char *>(malloc(rendered_size + 1));
	if (!rendered)
		return nullptr;
	memcpy(rendered, prefix, static_cast<size_t>(prefix_size));
	memcpy(rendered + prefix_size, countdown, static_cast<size_t>(countdown_size));
	memcpy(rendered + prefix_size + countdown_size, suffix, static_cast<size_t>(suffix_size));
	rendered[rendered_size] = '\0';
	return rendered;
}
