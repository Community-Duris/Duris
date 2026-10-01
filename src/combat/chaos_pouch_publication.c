#include "core/prototypes.h"
#include "combat/chaos_pouch_publication.h"
#include "item/craft_pouch_mutation.h"
#include "core/utils.h"

#include <cstring>
#include <new>
#include <string_view>

namespace
{
constexpr std::string_view prefix = "CHAOS_POUCH_LEDGER_";

bool ledger(const char *keyword)
{
	return keyword && std::strncmp(keyword, prefix.data(), prefix.size()) == 0;
}

void free_entry(extra_descr_data *entry)
{
	if (entry->keyword)
		str_free(entry->keyword);
	if (entry->description)
		str_free(entry->description);
	FREE(entry);
}

void free_chain(extra_descr_data *entry)
{
	while (entry)
	{
		auto next = entry->next;
		free_entry(entry);
		entry = next;
	}
}
} // namespace

bool chaos_pouch_publish_committed(obj_data *pouch, const craft_pouch_mutation &mutation)
{
	if (!pouch || pouch->obj_uid != mutation.before.object_uid ||
	    OBJ_VNUM(pouch) != mutation.before.vnum)
		return false;
	extra_descr_data *replacement = nullptr;
	try
	{
		player_item_snapshot live = {};
		live.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		live.object_uid = pouch->obj_uid;
		live.vnum = mutation.before.vnum;
		size_t visited = 0;
		for (auto entry = pouch->ex_description; entry; entry = entry->next)
		{
			if (++visited > PLAYER_SNAPSHOT_MAX_ROWS)
				return false;
			if (!ledger(entry->keyword))
				continue;
			if (!entry->description ||
			    std::strlen(entry->keyword) > prefix.size() + 1 ||
			    std::strlen(entry->description) >
				    CHAOS_MATERIAL_POUCH_LEDGER_CHUNK_BYTES)
				return false;
			live.extra_descriptions.push_back(
				{ entry->keyword, entry->description, false, {} });
		}
		player_item_snapshot after;
		if (chaos_pouch_ledger_apply_native(mutation.before, mutation.after, mutation.usage,
						    mutation.mode, live,
						    &after) != chaos_pouch_ledger_result::ok)
		{
			return chaos_pouch_ledger_verify(mutation.before, mutation.after,
							 mutation.usage, mutation.mode) ==
				       chaos_pouch_ledger_result::ok &&
			       chaos_pouch_ledger_counters_equal(live, mutation.after);
		}
		extra_descr_data **tail = &replacement;
		for (const auto &description : after.extra_descriptions)
		{
			auto entry = static_cast<extra_descr_data *>(__malloc(
				sizeof(extra_descr_data), MEM_TAG_EXDESCD, __FILE__, __LINE__));
			if (!entry)
			{
				free_chain(replacement);
				return false;
			}
			std::memset(entry, 0, sizeof(*entry));
			*tail = entry;
			tail = &entry->next;
			entry->keyword = str_dup(description.keyword.c_str());
			entry->description = str_dup(description.description.c_str());
			if (!entry->keyword || !entry->description)
			{
				free_chain(replacement);
				return false;
			}
		}
		// All allocations and validation succeeded. The remaining splice cannot
		// fail, and preserves every non-ledger node and other object field.
		extra_descr_data **link = &pouch->ex_description;
		while (*link)
		{
			if (!ledger((*link)->keyword))
			{
				link = &(*link)->next;
				continue;
			}
			auto obsolete = *link;
			*link = obsolete->next;
			free_entry(obsolete);
		}
		*tail = pouch->ex_description;
		pouch->ex_description = replacement;
		pouch->str_mask |= STRUNG_EDESC;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		free_chain(replacement);
		return false;
	}
}
