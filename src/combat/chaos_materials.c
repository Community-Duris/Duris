#include "core/prototypes.h"
#include "combat/chaos_materials.h"

#include "economy/tradeskill.h"
#include "item/item_ownership_runtime.h"
#include "persistence/persistence_checkpoint.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <new>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

extern P_index obj_index;

namespace
{
constexpr const char *CHAOS_POUCH_LEDGER_PREFIX = "CHAOS_POUCH_LEDGER_";
constexpr size_t CHAOS_MATERIAL_TYPES =
	static_cast<size_t>(HIGHEST_MAT_VNUM - LOWEST_MAT_VNUM + 1) +
	static_cast<size_t>(ENCRUST_VNUM_END - ENCRUST_VNUM_BEGIN + 1);

struct pouch_score
{
	uint64_t generated = 0;
	uint64_t collected = 0;
};

using score_table = std::array<pouch_score, CHAOS_MATERIAL_TYPES>;
using usage_table = std::array<chaos_material_pouch_usage, CHAOS_MATERIAL_TYPES>;

struct ledger_chunk
{
	size_t index;
	const char *payload;
};

int material_index(int vnum)
{
	if (vnum >= LOWEST_MAT_VNUM && vnum <= HIGHEST_MAT_VNUM)
		return vnum - LOWEST_MAT_VNUM;
	if (vnum >= ENCRUST_VNUM_BEGIN && vnum <= ENCRUST_VNUM_END)
		return HIGHEST_MAT_VNUM - LOWEST_MAT_VNUM + 1 + vnum - ENCRUST_VNUM_BEGIN;
	return -1;
}

int material_vnum(size_t index)
{
	const size_t salvage_count = HIGHEST_MAT_VNUM - LOWEST_MAT_VNUM + 1;
	return index < salvage_count ? LOWEST_MAT_VNUM + static_cast<int>(index) :
				       ENCRUST_VNUM_BEGIN + static_cast<int>(index - salvage_count);
}

bool parse_chunk_key(const char *key, size_t *index)
{
	if (!key || !index ||
	    strncmp(key, CHAOS_POUCH_LEDGER_PREFIX, strlen(CHAOS_POUCH_LEDGER_PREFIX)) != 0)
		return false;
	const char *suffix = key + strlen(CHAOS_POUCH_LEDGER_PREFIX);
	if (!*suffix)
		return false;
	char *end = nullptr;
	errno = 0;
	const unsigned long value = strtoul(suffix, &end, 10);
	if (errno == ERANGE || end == suffix || (end && *end) ||
	    value >= CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS)
		return false;
	*index = static_cast<size_t>(value);
	return true;
}

bool parse_number(const char *cursor, char delimiter, const char **end_out, uint64_t *value)
{
	if (!cursor || !end_out || !value || !isdigit(static_cast<unsigned char>(*cursor)))
		return false;
	char *end = nullptr;
	errno = 0;
	const unsigned long long parsed = strtoull(cursor, &end, 10);
	if (errno == ERANGE || end == cursor || (delimiter && *end != delimiter))
		return false;
	*end_out = end;
	*value = static_cast<uint64_t>(parsed);
	return true;
}

bool parse_chunk(const char *payload, score_table *scores,
		 std::array<bool, CHAOS_MATERIAL_TYPES> *seen)
{
	if (!payload || !scores || !seen)
		return false;
	const char *cursor = payload;
	while (*cursor)
	{
		uint64_t raw_index = 0;
		uint64_t generated = 0;
		uint64_t collected = 0;
		if (!parse_number(cursor, ':', &cursor, &raw_index) ||
		    !parse_number(cursor + 1, ':', &cursor, &generated) ||
		    !parse_number(cursor + 1, ';', &cursor, &collected))
			return false;
		if (raw_index >= CHAOS_MATERIAL_TYPES || (*seen)[raw_index])
			return false;
		(*seen)[raw_index] = true;
		(*scores)[raw_index] = { generated, collected };
		if (*cursor == ';')
			++cursor;
		else if (*cursor)
			return false;
	}
	return true;
}

bool read_scores(P_obj pouch, score_table *scores)
{
	if (!pouch || !scores)
		return false;
	*scores = {};
	std::array<ledger_chunk, CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS> chunks = {};
	std::array<bool, CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS> found = {};
	size_t chunk_count = 0;
	for (const extra_descr_data *description = pouch->ex_description; description;
	     description = description->next)
	{
		size_t chunk_index = 0;
		if (!parse_chunk_key(description->keyword, &chunk_index))
			continue;
		if (found[chunk_index] || chunk_count >= chunks.size())
			return false;
		found[chunk_index] = true;
		chunks[chunk_count++] = { chunk_index, description->description ?
							       description->description :
							       "" };
	}
	std::sort(chunks.begin(), chunks.begin() + chunk_count,
		  [](const ledger_chunk &left, const ledger_chunk &right)
		  { return left.index < right.index; });
	std::array<bool, CHAOS_MATERIAL_TYPES> seen = {};
	for (size_t index = 0; index < chunk_count; ++index)
		if (!parse_chunk(chunks[index].payload, scores, &seen))
			return false;
	return true;
}

bool build_ledger_chunks(const score_table &scores, std::vector<std::string> *chunks)
{
	if (!chunks)
		return false;
	chunks->clear();
	std::string current;
	for (size_t index = 0; index < scores.size(); ++index)
	{
		if (!scores[index].generated && !scores[index].collected)
			continue;
		std::string record = std::to_string(index) + ":" +
				     std::to_string(scores[index].generated) + ":" +
				     std::to_string(scores[index].collected) + ";";
		if (record.size() > CHAOS_MATERIAL_POUCH_LEDGER_CHUNK_BYTES)
			return false;
		if (!current.empty() &&
		    current.size() + record.size() > CHAOS_MATERIAL_POUCH_LEDGER_CHUNK_BYTES)
		{
			chunks->push_back(std::move(current));
			current.clear();
		}
		current += record;
	}
	if (!current.empty())
		chunks->push_back(std::move(current));
	return chunks->size() <= CHAOS_MATERIAL_POUCH_LEDGER_MAX_CHUNKS;
}

void free_ledger_entry(extra_descr_data *entry)
{
	if (!entry)
		return;
	if (entry->keyword)
		str_free(entry->keyword);
	if (entry->description)
		str_free(entry->description);
	FREE(entry);
}

bool write_scores(P_obj pouch, const score_table &scores)
{
	if (!pouch)
		return false;
	std::vector<std::string> chunks;
	try
	{
		if (!build_ledger_chunks(scores, &chunks))
			return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}

	extra_descr_data **link = &pouch->ex_description;
	while (*link)
	{
		size_t chunk_index = 0;
		if (!parse_chunk_key((*link)->keyword, &chunk_index))
		{
			link = &(*link)->next;
			continue;
		}
		extra_descr_data *obsolete = *link;
		*link = obsolete->next;
		free_ledger_entry(obsolete);
	}
	for (size_t index = chunks.size(); index-- > 0;)
	{
		extra_descr_data *entry = nullptr;
		CREATE(entry, extra_descr_data, 1, MEM_TAG_EXDESCD);
		memset(entry, 0, sizeof(*entry));
		const std::string key =
			std::string(CHAOS_POUCH_LEDGER_PREFIX) + std::to_string(index);
		entry->keyword = str_dup(key.c_str());
		entry->description = str_dup(chunks[index].c_str());
		entry->next = pouch->ex_description;
		pouch->ex_description = entry;
	}
	pouch->str_mask |= STRUNG_EDESC;
	return true;
}

bool merge_usage(const chaos_material_pouch_usage *usage, size_t usage_count, usage_table *merged,
		 size_t *merged_count)
{
	if (!merged || !merged_count || usage_count > CHAOS_MATERIAL_TYPES ||
	    (usage_count && !usage))
		return false;
	*merged = {};
	*merged_count = 0;
	for (size_t index = 0; index < usage_count; ++index)
	{
		if (!usage[index].count)
			continue;
		const int slot = material_index(usage[index].vnum);
		if (slot < 0)
			return false;
		size_t existing = *merged_count;
		for (size_t candidate = 0; candidate < *merged_count; ++candidate)
			if ((*merged)[candidate].vnum == usage[index].vnum)
			{
				existing = candidate;
				break;
			}
		if (existing == *merged_count)
		{
			if (*merged_count >= merged->size())
				return false;
			(*merged)[existing] = { usage[index].vnum, 0 };
			++*merged_count;
		}
		if ((*merged)[existing].count > UINT64_MAX - usage[index].count)
			return false;
		(*merged)[existing].count += usage[index].count;
	}
	return true;
}

bool update_scores(P_obj pouch, const chaos_material_pouch_usage *usage, size_t usage_count,
		   bool generated)
{
	if (!chaos_material_pouch_is_active(pouch))
		return false;
	usage_table merged = {};
	size_t merged_count = 0;
	if (!merge_usage(usage, usage_count, &merged, &merged_count))
		return false;
	score_table scores = {};
	if (!read_scores(pouch, &scores))
	{
		logit(LOG_DEBUG, "CHAOS pouch ledger rejected for uid=%lu vnum=%d", pouch->obj_uid,
		      OBJ_VNUM(pouch));
		return false;
	}
	for (size_t index = 0; index < merged_count; ++index)
	{
		const int slot = material_index(merged[index].vnum);
		uint64_t *total = generated ? &scores[slot].generated : &scores[slot].collected;
		if (*total > UINT64_MAX - merged[index].count)
		{
			logit(LOG_DEBUG, "CHAOS pouch ledger counter overflow for uid=%lu vnum=%d",
			      pouch->obj_uid, merged[index].vnum);
			return false;
		}
		*total += merged[index].count;
	}
	if (!write_scores(pouch, scores))
	{
		logit(LOG_DEBUG, "CHAOS pouch ledger write failed for uid=%lu vnum=%d",
		      pouch->obj_uid, OBJ_VNUM(pouch));
		return false;
	}
	return true;
}

std::string material_short_description(int vnum)
{
	const int rnum = real_object(vnum);
	if (rnum >= 0 && obj_index[rnum].desc2)
		return obj_index[rnum].desc2;
	return "an unknown crafting material";
}

void echo_generated(P_char ch, const usage_table &usage, size_t usage_count)
{
	std::ostringstream output;
	output << "&+GThe Chaos craft pouch generated:&n ";
	for (size_t index = 0; index < usage_count; ++index)
	{
		if (index)
			output << ", ";
		output << "&+Y" << usage[index].count << "&n x "
		       << material_short_description(usage[index].vnum) << " &+w["
		       << usage[index].vnum << "]&n";
	}
	output << ".\r\n";
	send_to_char(output.str().c_str(), ch);
}
} // namespace

P_obj chaos_material_pouch_find(P_char ch)
{
	if (!ch || IS_NPC(ch) || !chaos_starter_materials_enabled())
		return nullptr;
	for (P_obj object = ch->carrying; object; object = object->next_content)
		if (P_obj pouch = chaos_material_pouch_find_nested(object))
			return pouch;
	for (int slot = WEAR_ATTACH_BELT_1; slot <= WEAR_ATTACH_BELT_3; ++slot)
		if (P_obj pouch = chaos_material_pouch_find_nested(ch->equipment[slot]))
			return pouch;
	return nullptr;
}

bool chaos_material_pouch_available(P_char ch)
{
	return chaos_material_pouch_find(ch) != nullptr;
}

bool chaos_material_pouch_vnum_supported(int vnum)
{
	return material_index(vnum) >= 0;
}

bool chaos_material_pouch_record_generated(P_char ch, const chaos_material_pouch_usage *usage,
					   size_t usage_count)
{
	P_obj pouch = chaos_material_pouch_find(ch);
	if (!pouch)
		return false;
	usage_table merged = {};
	size_t merged_count = 0;
	if (!merge_usage(usage, usage_count, &merged, &merged_count) ||
	    !update_scores(pouch, usage, usage_count, true))
		return false;
	if (merged_count)
		echo_generated(ch, merged, merged_count);
	if (IS_PC(ch))
		mark_player_dirty_components(GET_PID(ch), PLAYER_COMPONENT_STATUS |
								  PLAYER_COMPONENT_EQUIPMENT |
								  PLAYER_COMPONENT_INVENTORY);
	return true;
}

bool chaos_material_pouch_record_collected(P_obj pouch, const chaos_material_pouch_usage *usage,
					   size_t usage_count)
{
	return update_scores(pouch, usage, usage_count, false);
}

bool chaos_material_pouch_collect_object(P_obj pouch, P_obj material)
{
	if (!chaos_material_pouch_is_active(pouch) || !material || material == pouch ||
	    !chaos_material_pouch_vnum_supported(OBJ_VNUM(material)))
		return false;
	const chaos_material_pouch_usage usage = { OBJ_VNUM(material), 1 };
	if (!chaos_material_pouch_record_collected(pouch, &usage, 1))
		return false;
	obj_from_char(material);
	extract_obj(material);
	return true;
}

int chaos_material_pouch_collect_inventory(P_char ch, P_obj pouch, const char *filter)
{
	if (!ch || !chaos_material_pouch_is_active(pouch))
		return 0;
	usage_table usage = {};
	size_t usage_count = 0;
	std::vector<P_obj> selected;
	try
	{
		for (P_obj object = ch->carrying; object; object = object->next_content)
		{
			if (object == pouch ||
			    !chaos_material_pouch_vnum_supported(OBJ_VNUM(object)) ||
			    (filter && *filter && !isname(filter, object->name)))
				continue;
			size_t index = usage_count;
			for (size_t candidate = 0; candidate < usage_count; ++candidate)
				if (usage[candidate].vnum == OBJ_VNUM(object))
				{
					index = candidate;
					break;
				}
			if (index == usage_count)
			{
				usage[index] = { OBJ_VNUM(object), 0 };
				++usage_count;
			}
			++usage[index].count;
			selected.push_back(object);
		}
	}
	catch (const std::bad_alloc &)
	{
		return 0;
	}
	if (selected.empty() ||
	    !chaos_material_pouch_record_collected(pouch, usage.data(), usage_count))
		return 0;
	for (P_obj object : selected)
	{
		obj_from_char(object);
		extract_obj(object);
	}
	return static_cast<int>(selected.size());
}

const char *chaos_material_pouch_scoreboard(P_obj pouch)
{
	static std::string output;
	output.clear();
	output = "&+yInside the compact Chaos craft pouch scoreboard:&n\r\n";
	output += "  Catalog: salvage 400000-400209; encrust 400291-400299.\r\n";
	output += "  Material requirements are supplied without consuming the pouch.\r\n";
	if (!pouch || !chaos_material_pouch_is(pouch))
	{
		output += "  The pouch ledger is unavailable.\r\n";
		return output.c_str();
	}
	score_table scores = {};
	if (!read_scores(pouch, &scores))
	{
		output += "  The pouch ledger is unreadable; no history was changed.\r\n";
		return output.c_str();
	}
	std::vector<size_t> order;
	try
	{
		for (size_t index = 0; index < scores.size(); ++index)
			if (scores[index].generated || scores[index].collected)
				order.push_back(index);
		std::sort(order.begin(), order.end(),
			  [&](size_t left, size_t right)
			  {
				  if (scores[left].generated != scores[right].generated)
					  return scores[left].generated > scores[right].generated;
				  if (scores[left].collected != scores[right].collected)
					  return scores[left].collected > scores[right].collected;
				  return material_vnum(left) < material_vnum(right);
			  });
	}
	catch (const std::bad_alloc &)
	{
		output += "  The pouch scoreboard could not be assembled.\r\n";
		return output.c_str();
	}
	if (order.empty())
	{
		output += "  No generated or collected materials recorded yet.\r\n";
		return output.c_str();
	}
	output += "&+GGenerated / &+Ccollected&n (most-used first):\r\n";
	for (size_t index : order)
	{
		output += "  &+Y" + std::to_string(scores[index].generated) + "&n generated / &+C" +
			  std::to_string(scores[index].collected) + "&n collected  ";
		output += material_short_description(material_vnum(index));
		output += " &+w[" + std::to_string(material_vnum(index)) + "]&n\r\n";
	}
	return output.c_str();
}
