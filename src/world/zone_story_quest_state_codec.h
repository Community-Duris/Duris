#ifndef ZONE_STORY_QUEST_STATE_CODEC_H
#define ZONE_STORY_QUEST_STATE_CODEC_H

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace zone_story_quest_state
{
using records = std::map<std::string, std::string>;
struct changes
{
	records values;
	bool replace = false;
};
inline std::string hex(std::string_view value)
{
	constexpr char digits[] = "0123456789abcdef";
	std::string result;
	for (unsigned char ch : value)
	{
		result += digits[ch >> 4];
		result += digits[ch & 15];
	}
	return result;
}
inline bool unhex(std::string_view value, std::string *result)
{
	if (value.size() % 2)
		return false;
	result->clear();
	auto digit = [](char ch)
	{ return ch >= '0' && ch <= '9' ? ch - '0' :
		 ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 :
					  -1; };
	for (size_t i = 0; i < value.size(); i += 2)
	{
		const int high = digit(value[i]), low = digit(value[i + 1]);
		if (high < 0 || low < 0)
			return false;
		*result += static_cast<char>(high * 16 + low);
	}
	return true;
}
inline records split_document(std::string_view document)
{
	records result;
	size_t begin = 0;
	while (begin < document.size())
	{
		const auto end = document.find('\n', begin);
		const auto line = document.substr(begin, end == std::string_view::npos ?
								 document.size() - begin :
								 end - begin);
		begin = end == std::string_view::npos ? document.size() : end + 1;
		if (line.empty())
			continue;
		std::vector<std::string_view> fields;
		size_t at = 0;
		while (at <= line.size())
		{
			const auto separator = line.find('|', at);
			fields.push_back(line.substr(at, separator == std::string_view::npos ?
								 line.size() - at :
								 separator - at));
			if (separator == std::string_view::npos)
				break;
			at = separator + 1;
		}
		std::string key = "meta";
		if ((fields[0] == "T" || fields[0] == "E") && fields.size() >= 2)
			key = std::string(fields[0]) + ":" + std::string(fields[1]);
		else if (fields[0] != "ZSQF" && fields[0] != "K" && fields.size() >= 3)
		{
			key = "character:" + std::string(fields[1]) + ":" + std::string(fields[2]);
			if (fields.size() >= 4 && fields[0] != "N")
				key += ":" + std::string(fields[0]) + ":" + std::string(fields[3]);
		}
		result[key] += std::string(line) + "\n";
	}
	return result;
}
inline std::string document(const records &values)
{
	std::string result;
	const auto meta = values.find("meta");
	if (meta != values.end())
		result = meta->second;
	for (const auto &[key, value] : values)
		if (key != "meta")
			result += value;
	return result;
}
inline std::string encode(const records &values)
{
	std::string result = "ZSQB|1\n";
	for (const auto &[key, value] : values)
		result += hex(key) + "|" + hex(value) + "\n";
	return result;
}
inline bool decode(std::string_view encoded, records *values)
{
	if (!encoded.starts_with("ZSQB|1\n"))
		return false;
	records candidate;
	size_t begin = 7;
	while (begin < encoded.size())
	{
		const auto end = encoded.find('\n', begin), separator = encoded.find('|', begin);
		if (end == std::string_view::npos || separator == std::string_view::npos ||
		    separator >= end)
			return false;
		std::string key, value;
		if (!unhex(encoded.substr(begin, separator - begin), &key) ||
		    !unhex(encoded.substr(separator + 1, end - separator - 1), &value) ||
		    key.empty() || !candidate.emplace(key, value).second)
			return false;
		begin = end + 1;
	}
	*values = std::move(candidate);
	return true;
}
inline void apply(records *values, const changes &updates)
{
	if (updates.replace)
		values->clear();
	for (const auto &[key, value] : updates.values)
		if (value.empty())
			values->erase(key);
		else
			(*values)[key] = value;
}
// SQL rows retain complete, atomically appended record batches. Replaying a
// bucket replaces earlier values; an empty value deletes the corresponding key.
inline bool decode_journal(std::string_view encoded, records *values)
{
	records candidate;
	size_t begin = 0;
	while (begin < encoded.size())
	{
		const auto next = encoded.find("\nZSQB|1\n", begin);
		const size_t end = next == std::string_view::npos ? encoded.size() : next + 1;
		records batch;
		if (!decode(encoded.substr(begin, end - begin), &batch))
			return false;
		apply(&candidate, { std::move(batch), false });
		begin = end;
	}
	if (encoded.empty())
		return false;
	*values = std::move(candidate);
	return true;
}
inline unsigned bucket(std::string_view key)
{
	uint32_t hash = 2166136261U;
	for (unsigned char ch : key)
		hash = (hash ^ ch) * 16777619U;
	return key == "meta" ? 1 : 2 + hash % 254;
}
} // namespace zone_story_quest_state
#endif
