#include "item/native_artifact_actions.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/artifact_mana.h"
#include "magic/spells.h"
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <string>

extern const int top_of_world;

namespace
{
constexpr std::array<int, 5> pilots = { 21, 22, 922, 31514, 67243 };
std::map<int, native_artifact_config> native_configs;
bool native_master_enabled = false;
bool setting(const std::string &prefix, const char *key, int fallback, int low, int high, int &out)
{
	const float raw =
		get_property((prefix + key).c_str(), static_cast<double>(fallback), false);
	if (!std::isfinite(raw) || raw < low || raw > high || std::floor(raw) != raw)
		return false;
	out = static_cast<int>(raw);
	return true;
}
}

uint32_t native_artifact_ability(int vnum, int power)
{
	return 0xa0000000U | (static_cast<uint32_t>(vnum) << 4) | power;
}

void update_native_artifact_properties()
{
	if (!nevent_require_game_thread("update_native_artifact_properties"))
		return;
	bool release_forms = native_master_enabled && !item_actions_enabled();
	native_master_enabled = item_actions_enabled();
	for (int vnum : pilots)
	{
		const std::string prefix = "itemActions.artifact." + std::to_string(vnum) + ".";
		auto &old = native_configs[vnum];
		native_artifact_config next;
		int enabled = 0;
		next.valid = setting(prefix, "enabled", 0, 0, 1, enabled) &&
			     setting(prefix, "manaCost", 0, 0, 10000000, next.cost) &&
			     setting(prefix, "manaCapacity", 0, 0, 10000000, next.capacity) &&
			     setting(prefix, "manaRegen", 0, 0, 10000000, next.regeneration) &&
			     setting(prefix, "passiveFloor", 0, 0, 10000000, next.floor) &&
			     setting(prefix, "manaRevision", 1, 1, 10000000, next.mana_revision) &&
			     setting(prefix, "windupPulses", 8, 4, 600, next.windup);
		next.enabled = enabled == 1;
		// Disabled defaults need no balance profile. Every enabled power pays.
		next.valid = next.valid &&
			     (!next.enabled || (next.cost > 0 && next.capacity >= next.cost &&
						next.floor <= next.capacity));
		next.revision = old.revision;
		if (next != old)
		{
			release_forms = release_forms || vnum == 67243;
			for (int power = 1; power <= 15; ++power)
				item_actions_disable(native_artifact_ability(vnum, power));
			if (next.revision != std::numeric_limits<uint64_t>::max())
				++next.revision;
			old = next;
		}
	}
	if (release_forms)
		release_necroplasm_forms();
}

bool native_artifact_owns(int vnum)
{
	update_native_artifact_properties();
	if (!item_actions_enabled())
		return false;
	const auto found = native_configs.find(vnum);
	return found != native_configs.end() && (!found->second.valid || found->second.enabled);
}

native_artifact_config native_artifact_settings(int vnum)
{
	const auto found = native_configs.find(vnum);
	return found == native_configs.end() ? native_artifact_config{} : found->second;
}

bool native_artifact_prepare(int vnum, const native_artifact_config &settings)
{
	return settings.valid && settings.enabled &&
	       settings.revision != std::numeric_limits<uint64_t>::max() &&
	       artifact_mana_publish(vnum, { static_cast<uint64_t>(vnum),
					     static_cast<uint64_t>(settings.mana_revision),
					     static_cast<uint64_t>(settings.capacity),
					     static_cast<uint64_t>(settings.regeneration),
					     static_cast<uint64_t>(settings.floor) });
}

bool native_artifact_actor(P_char actor)
{
	return IS_ALIVE(actor) && actor->in_room >= 0 && actor->in_room <= top_of_world &&
	       IS_AWAKE(actor) && GET_STAT(actor) >= STAT_RESTING && !IS_IMMOBILE(actor) &&
	       !IS_AFFECTED2(actor, AFF2_STUNNED) && !CHAR_IN_NO_MAGIC_ROOM(actor) &&
	       !(IS_NPC(actor) && IS_AFFECTED(actor, AFF_CHARM));
}

bool native_artifact_hostile(P_char actor, P_char target)
{
	return IS_ALIVE(target) && actor != target && target->in_room == actor->in_room &&
	       !is_in_safe(actor) && !IS_AFFECTED5(actor, AFF5_NOT_OFFENSIVE) &&
	       !affected_by_spell(actor, SONG_PEACE) && !affected_by_spell(actor, SKILL_GAZE) &&
	       CanDoFightMove(actor, target) &&
	       !(IS_PC(target) && should_not_kill(actor, target)) && target != GET_MASTER(actor);
}

bool native_artifact_current(const item_action_identity &identity, P_char &actor, P_obj &source)
{
	if (!item_action_pending(identity.action_id))
		return false;
	actor = find_character_by_runtime_id(identity.actor_id);
	if (!native_artifact_actor(actor) || actor->in_room != identity.origin_room ||
	    identity.source_slot < 0 || identity.source_slot >= MAX_WEAR)
		return false;
	source = actor->equipment[identity.source_slot];
	return source && source->obj_uid == identity.source_uid && OBJ_WORN_BY(source, actor);
}
