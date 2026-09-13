#include "item/studio_abilities.h"
#include "item/artifact_mana.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "magic/spells.h"
#include "mob/studioproc.h"
#include "net/comm.h"
#include <algorithm>
#include <fstream>
#include <map>
#include <time.h>

extern P_index obj_index;
extern Skill skills[];
extern const int top_of_world;

namespace
{
studio_ability_catalog catalog, versions;
bool studio_enabled = false;
std::map<uint32_t, bool> ability_enabled;
using cooldown_key = std::pair<uint64_t, uint32_t>;
std::map<cooldown_key, uint64_t> cooldowns;
constexpr size_t MAX_COOLDOWNS = 65536;
uint32_t engine_id(uint32_t id)
{
	return 0x90000000U | id;
}
uint64_t studio_now_ms()
{
	timespec now{};
	if (clock_gettime(CLOCK_MONOTONIC, &now) || now.tv_sec < 0)
		return 0;
	return uint64_t(now.tv_sec) * 1000 + now.tv_nsec / 1000000;
}
bool actor_valid(P_char actor, const studio_ability_definition &definition)
{
	return IS_ALIVE(actor) && IS_AWAKE(actor) && GET_STAT(actor) >= STAT_RESTING &&
	       GET_LEVEL(actor) >= definition.min_level && !IS_IMMOBILE(actor) &&
	       !IS_AFFECTED2(actor, AFF2_STUNNED) && !CHAR_IN_NO_MAGIC_ROOM(actor) &&
	       !(IS_NPC(actor) && IS_AFFECTED(actor, AFF_CHARM));
}
bool effect_supported(const studio_ability_effect &effect)
{
	if (effect.spell <= 0 || effect.spell >= MAX_SKILLS || !skills[effect.spell].spell_pointer)
		return false;
	const auto flags = skills[effect.spell].targets;
	if (IS_SET(flags, TAR_CHAR_WORLD | TAR_OBJ_WORLD))
		return false;
	switch (effect.target)
	{
	case studio_ability_target::victim:
		return IS_SET(flags, TAR_CHAR_ROOM | TAR_CHAR_RANGE) &&
		       !IS_SET(flags, TAR_SELF_ONLY);
	case studio_ability_target::activator:
	case studio_ability_target::holder:
		return IS_SET(flags, TAR_SELF_ONLY | TAR_CHAR_ROOM | TAR_CHAR_RANGE) &&
		       !IS_SET(flags, TAR_SELF_NONO);
	case studio_ability_target::item:
		return IS_SET(flags, TAR_OBJ_INV | TAR_OBJ_EQUIP);
	case studio_ability_target::room:
		return IS_SET(flags, TAR_AREA | TAR_IGNORE);
	}
	return false;
}
P_char spell_target(const studio_ability_effect &effect, const item_action_context &context)
{
	if (effect.target == studio_ability_target::victim)
		return context.target;
	if (effect.target == studio_ability_target::activator ||
	    effect.target == studio_ability_target::holder)
		return context.actor;
	return nullptr;
}
bool lawful(const studio_ability_effect &effect, const item_action_context &context)
{
	if (!effect_supported(effect))
		return false;
	P_char target = spell_target(effect, context);
	if (target && (!IS_ALIVE(target) || !CAN_SEE(context.actor, target)))
		return false;
	if (target && target != context.actor &&
	    IS_ROOM(context.identity.origin_room, ROOM_SINGLE_FILE) &&
	    !AdjacentInRoom(context.actor, target))
		return false;
	if (effect.target == studio_ability_target::room &&
	    IS_ROOM(context.identity.origin_room, ROOM_SINGLE_FILE))
		return false;
	if (!IS_AGG_SPELL(effect.spell))
		return true;
	if (is_in_safe(context.actor) || IS_AFFECTED5(context.actor, AFF5_NOT_OFFENSIVE) ||
	    affected_by_spell(context.actor, SONG_PEACE) ||
	    affected_by_spell(context.actor, SKILL_GAZE))
		return false;
	if (target && target != context.actor &&
	    (!CanDoFightMove(context.actor, target) ||
	     (IS_PC(target) && should_not_kill(context.actor, target))))
		return false;
	if (GET_MASTER(context.actor) &&
	    (target == GET_MASTER(context.actor) ||
	     (!target && should_area_hit(context.actor, GET_MASTER(context.actor)))))
		return false;
	return true;
}
class studio_adapter final : public item_action_adapter
{
	const studio_ability_definition definition;
	const std::string source_name;
	void message(const item_action_context &context, const std::string &text) const noexcept
	{
		const std::string self = "&+W$p: " + text + "&n";
		const std::string room = "&+W$n's $p: " + text + "&n";
		act(self.c_str(), FALSE, context.actor, context.source, context.target, TO_CHAR);
		act(room.c_str(), FALSE, context.actor, context.source, context.target, TO_ROOM);
	}

    public:
	studio_adapter(studio_ability_definition def, std::string name)
		: definition(std::move(def))
		, source_name(std::move(name))
	{
	}
	bool validate(const item_action_context &context) const noexcept override
	{
		if (!actor_valid(context.actor, definition) || context.source->R_num < 0 ||
		    OBJ_VNUM(context.source) != definition.vnum ||
		    (IS_OBJ_STAT2(context.source, ITEM2_ACCOUNT_BOUND) &&
		     !account_bound_reward_owner(context.actor, context.source)))
			return false;
		for (size_t i = 0; i < definition.effect_count; ++i)
			if (!lawful(definition.effects[i], context))
				return false;
		return true;
	}
	item_action_consumption commit(const item_action_context &context) const noexcept override
	{
		const uint64_t now = studio_now_ms();
		const cooldown_key key{ context.identity.source_uid, definition.id };
		const auto found = cooldowns.find(key);
		if (found != cooldowns.end() && now < found->second)
			return item_action_consumption::rejected;
		if (definition.cooldown_ms && found == cooldowns.end() &&
		    cooldowns.size() >= MAX_COOLDOWNS)
			return item_action_consumption::rejected;
		// Reserve bookkeeping before the paid debit, so rejected payment changes
		// neither cooldown nor resource. No callbacks occur in this phase.
		if (definition.cooldown_ms)
			cooldowns.try_emplace(key, 0);
		if (definition.cost &&
		    !artifact_mana_debit(context.source, definition.cost,
					 definition.trigger == studio_ability_trigger::hit,
					 context.identity.action_id))
		{
			if (found == cooldowns.end())
				cooldowns.erase(key);
			return item_action_consumption::rejected;
		}
		if (definition.cooldown_ms)
			cooldowns[key] = now + definition.cooldown_ms;
		return item_action_consumption::committed;
	}
	void announce(const item_action_context &context) const noexcept override
	{
		message(context, definition.begin);
		for (size_t i = 0; i < definition.effect_count; ++i)
			if (IS_AGG_SPELL(definition.effects[i].spell))
			{
				if (definition.effects[i].target == studio_ability_target::victim &&
				    context.target != context.actor)
					act("&+R$n's $p gathers magic aimed at YOU!&n", FALSE,
					    context.actor, context.source, context.target, TO_VICT);
				else
					act("&+RThe magic gathering in $p threatens the room!&n",
					    FALSE, context.actor, context.source, nullptr, TO_ROOM);
				break;
			}
	}
	void progress(const item_action_context &context) const noexcept override
	{
		message(context, definition.beat);
	}
	void resolve(const item_action_context &context,
		     const item_action_effect &selected) const noexcept override
	{
		const auto &effect = definition.effects[static_cast<size_t>(selected.auxiliary)];
		if (!selected.auxiliary)
			message(context, definition.complete);
		P_char target = spell_target(effect, context);
		if (IS_AGG_SPELL(effect.spell))
			appear(context.actor);
		skills[effect.spell].spell_pointer(
			effect.power, context.actor, nullptr,
			effect.call == studio_ability_call::spell ? SPELL_TYPE_SPELL :
								    SPELL_TYPE_WAND,
			target,
			effect.target == studio_ability_target::item ? context.source : nullptr);
	}
	void finish(const item_action_identity &identity, item_action_consumption,
		    item_action_outcome outcome) const noexcept override
	{
		if (outcome == item_action_outcome::completed)
			return;
		const std::string text = source_name + "&n: " + definition.cancel + "&n\r\n";
		if (identity.origin_room >= 0 && identity.origin_room <= top_of_world)
			send_to_room(text.c_str(), identity.origin_room);
		P_char actor = find_character_by_runtime_id(identity.actor_id);
		if (IS_ALIVE(actor) && actor->in_room != identity.origin_room)
			send_to_char(text.c_str(), actor);
	}
};
}

bool studio_abilities_load(const std::string &json, std::string &error)
{
	if (!nevent_require_game_thread("studio_abilities_load"))
	{
		error = "catalog: game thread required";
		return false;
	}
	studio_ability_catalog candidate;
	if (!parse_studio_ability_catalog(json, candidate, error))
		return false;
	size_t unseen = 0;
	for (const auto &[id, definition] : candidate)
	{
		const std::string prefix = "ability " + std::to_string(id) + ": ";
		const auto old = versions.find(id);
		if (old != versions.end() &&
		    (definition.revision < old->second.revision ||
		     (definition.revision == old->second.revision && definition != old->second)))
		{
			error = prefix + "changed definition requires a greater revision";
			return false;
		}
		if (old == versions.end())
			++unseen;
		if (real_object(definition.vnum) < 0)
		{
			error = prefix + "source vnum is absent";
			return false;
		}
		for (size_t i = 0; i < definition.effect_count; ++i)
			if (!effect_supported(definition.effects[i]))
			{
				error = prefix + "effects[" + std::to_string(i) +
					"] spell pointer or typed target is unsupported";
				return false;
			}
		if (definition.mana.id &&
		    !artifact_mana_can_publish(definition.vnum, definition.mana))
		{
			error = prefix +
				"mana profile conflicts with an existing binding or revision";
			return false;
		}
	}
	if (versions.size() + unseen > 4096)
	{
		error = "catalog: lifetime ability-id bound exceeded";
		return false;
	}
	// All checks precede publication. These calls run synchronously on the game
	// thread; intra-candidate mana consistency was checked by the pure parser.
	for (const auto &[id, definition] : candidate)
		if (definition.mana.id)
			artifact_mana_publish(definition.vnum, definition.mana);
	for (const auto &[id, definition] : catalog)
	{
		const auto next = candidate.find(id);
		if (next == candidate.end() || next->second != definition)
			item_actions_disable(engine_id(id));
	}
	for (const auto &[id, definition] : candidate)
		versions[id] = definition;
	catalog = std::move(candidate);
	update_studio_ability_properties();
	error.clear();
	return true;
}

bool studio_abilities_reload_file(std::string &error)
{
	std::ifstream file("lib/item_abilities.json", std::ios::binary);
	if (!file)
	{
		error = "cannot read lib/item_abilities.json; last valid catalog retained";
		return false;
	}
	std::string text(1024 * 1024 + 1, '\0');
	file.read(text.data(), text.size());
	text.resize(static_cast<size_t>(file.gcount()));
	if (file.bad())
	{
		error = "cannot read ability catalog; last valid catalog retained";
		return false;
	}
	return studio_abilities_load(text, error);
}

void update_studio_ability_properties()
{
	if (!nevent_require_game_thread("update_studio_ability_properties"))
		return;
	const bool enabled = get_property("itemActions.studio.enabled", 0.0, false) == 1;
	for (const auto &[id, definition] : catalog)
	{
		const bool individual =
			get_property(
				("itemActions.ability." + std::to_string(id) + ".enabled").c_str(),
				1.0, false) == 1;
		if (studio_enabled != enabled ||
		    (ability_enabled.contains(id) && ability_enabled[id] != individual))
			item_actions_disable(engine_id(id));
		ability_enabled[id] = individual;
	}
	studio_enabled = enabled;
}

bool studio_ability_reference(uint32_t id, int vnum, studio_ability_trigger trigger,
			      std::string &error)
{
	const auto found = catalog.find(id);
	if (found == catalog.end() || found->second.vnum != vnum ||
	    found->second.trigger != trigger)
	{
		error = "itemability must reference a loaded definition with this object vnum and event";
		return false;
	}
	error.clear();
	return true;
}

bool parse_studio_ability_action(int target_type, int vnum, int event, int command,
				 const char *argument, uint32_t &id, std::string &error)
{
	if (target_type != SP_T_OBJ ||
	    (event != SP_EV_HIT && (event != SP_EV_CMD || command != CMD_USE)))
	{
		error = "itemability is object-only and requires HIT or CMD use";
		return false;
	}
	char *end = nullptr;
	const unsigned long value = argument ? strtoul(argument, &end, 10) : 0;
	while (end && *end == ' ')
		++end;
	if (!argument || *argument < '0' || *argument > '9' || !end || *end || !value ||
	    value > 0x0fffffff)
	{
		error = "itemability requires one supported numeric ID";
		return false;
	}
	if (!studio_ability_reference(static_cast<uint32_t>(value), vnum,
				      event == SP_EV_HIT ? studio_ability_trigger::hit :
							   studio_ability_trigger::use,
				      error))
		return false;
	id = static_cast<uint32_t>(value);
	return true;
}

item_action_start begin_studio_ability(uint32_t id, studio_ability_trigger event, P_obj source,
				       P_char actor, P_char victim, const char *arguments)
{
	if (!nevent_require_game_thread("begin_studio_ability"))
		return item_action_start::suppressed;
	if (!item_actions_enabled() || !studio_enabled)
		return item_action_start::legacy;
	const auto found = catalog.find(id);
	if (found == catalog.end())
		return item_action_start::suppressed;
	if (!ability_enabled[id])
		return item_action_start::legacy;
	const auto &definition = found->second;
	if (!source || source->R_num < 0 || OBJ_VNUM(source) != definition.vnum ||
	    event != definition.trigger || !IS_ALIVE(actor))
		return item_action_start::suppressed;
	if (!OBJ_WORN_BY(source, actor) && !OBJ_CARRIED_BY(source, actor))
		return event == studio_ability_trigger::use ? item_action_start::legacy :
							      item_action_start::suppressed;
	if (event == studio_ability_trigger::use)
	{
		char name[MAX_INPUT_LENGTH]{}, target_name[MAX_INPUT_LENGTH]{};
		if (!arguments || strlen(arguments) >= MAX_INPUT_LENGTH)
			return item_action_start::suppressed;
		const char *rest = one_argument(arguments, name);
		int slot = -1;
		P_obj selected = definition.carried ?
					 get_obj_in_list_vis(actor, name, actor->carrying) :
					 get_object_in_equip_vis(actor, name, &slot);
		if (selected != source)
			return item_action_start::
				legacy; // Another item's USE is not this ability's event.
		rest = one_argument(rest, target_name);
		if (*rest)
			return item_action_start::suppressed;
		victim = *target_name ? get_char_room_vis(actor, target_name) : GET_OPPONENT(actor);
	}
	if (!actor_valid(actor, definition))
		return item_action_start::suppressed;
	bool needs_victim = false;
	for (size_t i = 0; i < definition.effect_count; ++i)
		needs_victim = needs_victim ||
			       definition.effects[i].target == studio_ability_target::victim;
	if (needs_victim && !IS_ALIVE(victim))
		return item_action_start::suppressed;
	const auto now = studio_now_ms();
	// Bounded auxiliary cooldown state, keyed by stable ability ID and physical
	// UID. Reorder, transfer, abort and catalog reload cannot reset it.
	for (auto it = cooldowns.begin(); it != cooldowns.end();)
		if (it->second <= now)
			it = cooldowns.erase(it);
		else
			++it;
	item_action_definition action;
	action.id = engine_id(id);
	action.revision = definition.revision;
	action.mode = event == studio_ability_trigger::hit ? item_action_mode::passive :
							     item_action_mode::active;
	action.source = definition.carried ? item_action_source::carried :
					     item_action_source::equipped;
	action.windup_pulses = definition.windup;
	action.progress_pulses = definition.progress;
	action.effect_count = definition.effect_count;
	for (size_t i = 0; i < definition.effect_count; ++i)
		action.effects[i] = { static_cast<uint32_t>(definition.effects[i].spell),
				      definition.effects[i].power, item_action_call::spell,
				      item_action_effect_target::original, static_cast<int>(i) };
	return start_item_action_instance(
		action,
		std::make_unique<studio_adapter>(definition, OBJ_SHORT(source) ? OBJ_SHORT(source) :
										 "the item"),
		actor, needs_victim ? victim : actor, source);
}
