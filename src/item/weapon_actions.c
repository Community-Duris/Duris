#include "item/weapon_actions.h"
#include "item/native_artifact_actions.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "item/artifact_mana.h"
#include "magic/spells.h"
#include "net/comm.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <string>

extern P_index obj_index;
extern P_obj object_list;
extern Skill skills[];
extern P_room world;
extern const int top_of_world;

namespace
{
enum class weapon_kind : uint32_t
{
	avernus = 1,
	packed = 2,
	random = 3
};

struct weapon_config
{
	bool valid = false, enabled = false;
	int windup = 8, cost = 0, capacity = 0, regeneration = 0, mana_revision = 1;
	bool operator==(const weapon_config &) const = default;
};

struct binding
{
	weapon_kind kind;
	int vnum;
	uint32_t id;
	uint64_t revision = 0;
	weapon_config config;
};

std::map<uint32_t, binding> bindings;

bool integer(const std::string &key, int fallback, int minimum, int maximum, int &out)
{
	const float raw = get_property(key.c_str(), static_cast<double>(fallback), false);
	if (!std::isfinite(raw) || raw < minimum || raw > maximum || std::floor(raw) != raw)
		return false;
	out = static_cast<int>(raw);
	return true;
}

weapon_config configuration(weapon_kind kind, int vnum)
{
	const std::string category = kind == weapon_kind::avernus ? "itemActions.avernus" :
				     kind == weapon_kind::packed  ? "itemActions.weapons" :
								    "itemActions.randomWeapons";
	const std::string item = "itemActions.weapon." + std::to_string(vnum);
	weapon_config result;
	int enabled = 0, individual = 1;
	result.valid = integer(category + ".enabled", 0, 0, 1, enabled) &&
		       integer(item + ".enabled", 1, 0, 1, individual) &&
		       integer(category + ".windupPulses", 8, 4, 600, result.windup) &&
		       integer(item + ".manaCost", 0, 0, 10000000, result.cost) &&
		       integer(item + ".manaCapacity", 0, 0, 10000000, result.capacity) &&
		       integer(item + ".manaRegen", 0, 0, 10000000, result.regeneration) &&
		       integer(item + ".manaRevision", 1, 1, 10000000, result.mana_revision);
	result.enabled = enabled == 1 && individual == 1;
	result.valid = result.valid && (!result.cost || result.capacity >= result.cost);
	return result;
}

bool valid_spell(uint32_t spell)
{
	return spell > 0 && spell < MAX_SKILLS && skills[spell].spell_pointer;
}

void packed_messages(const item_action_context &context)
{
	for (auto *extra = context.source->ex_description; extra; extra = extra->next)
		if (isname("_char_msg", extra->keyword))
			act(extra->description, FALSE, context.actor, context.source,
			    context.target, TO_CHAR | ACT_NOEOL);
		else if (isname("_victim_msg", extra->keyword))
			act(extra->description, FALSE, context.actor, context.source,
			    context.target, TO_VICT | ACT_NOEOL);
		else if (isname("_room_msg", extra->keyword))
			act(extra->description, FALSE, context.actor, context.source,
			    context.target, TO_NOTVICT | ACT_NOEOL);
}

class weapon_adapter final : public item_action_adapter
{
	const weapon_kind kind;
	const int vnum;
	const weapon_config config;

    public:
	weapon_adapter(weapon_kind type, int object_vnum, weapon_config settings)
		: kind(type)
		, vnum(object_vnum)
		, config(settings)
	{
	}
	bool validate(const item_action_context &context) const noexcept override
	{
		if (OBJ_VNUM(context.source) != vnum || !context.definition.effect_count)
			return false;
		bool harmful = kind == weapon_kind::avernus;
		for (size_t i = 0; i < context.definition.effect_count; ++i)
		{
			const auto &effect = context.definition.effects[i];
			if (kind == weapon_kind::avernus)
			{
				if (effect.id != 1 || effect.power > 200 ||
				    effect.target != item_action_effect_target::original)
					return false;
			}
			else
			{
				if (!valid_spell(effect.id) ||
				    effect.call != item_action_call::spell)
					return false;
				harmful = harmful || IS_AGG_SPELL(effect.id);
			}
		}
		// CanDoFightMove checks the original target's reach/visibility and the
		// actor's current offensive eligibility without selecting an opponent.
		return !harmful || CanDoFightMove(context.actor, context.target);
	}
	item_action_consumption commit(const item_action_context &context) const noexcept override
	{
		if (config.cost && !artifact_mana_debit(context.source, config.cost, true,
							context.identity.action_id))
			return item_action_consumption::rejected;
		if (kind == weapon_kind::random)
			context.source->timer[0] = context.definition.effects[0].auxiliary;
		return config.cost || kind == weapon_kind::random ?
			       item_action_consumption::committed :
			       item_action_consumption::none;
	}
	void announce(const item_action_context &context) const noexcept override
	{
		bool harmful = kind == weapon_kind::avernus;
		for (size_t i = 0; i < context.definition.effect_count; ++i)
			harmful = harmful || context.definition.effects[i].target ==
						     item_action_effect_target::original;
		act(kind == weapon_kind::avernus ?
			    "&+LAvernus begins gathering a hungry light toward $N!&n" :
		    harmful ? "&+W$p begins gathering magic toward $N!&n" :
			      "&+W$p begins gathering magic around you!&n",
		    FALSE, context.actor, context.source, context.target, TO_CHAR);
		act(kind == weapon_kind::avernus ?
			    "&+L$n's Avernus begins drawing a hungry light toward YOU!&n" :
		    harmful ? "&+W$n's $p begins gathering magic toward YOU!&n" :
			      "&+W$n's $p begins gathering magic around $m!&n",
		    FALSE, context.actor, context.source, context.target, TO_VICT);
		act(kind == weapon_kind::avernus ?
			    "&+L$n's Avernus begins gathering a hungry light toward $N!&n" :
		    harmful ? "&+W$n's $p begins gathering magic toward $N!&n" :
			      "&+W$n's $p begins gathering magic around $m!&n",
		    FALSE, context.actor, context.source, context.target, TO_NOTVICT);
	}
	void progress(const item_action_context &context) const noexcept override
	{
		for (int audience : { TO_CHAR, TO_VICT, TO_NOTVICT })
			act("&+WThe energy gathering in $p intensifies!&n", FALSE, context.actor,
			    context.source, context.target, audience);
	}
	void resolve(const item_action_context &context,
		     const item_action_effect &effect) const noexcept override
	{
		if (kind == weapon_kind::avernus)
		{
			resolve_avernus_drain(context.source, context.actor, context.target,
					      effect.power, effect.auxiliary);
			return;
		}
		if (kind == weapon_kind::packed && effect.auxiliary == 0)
			packed_messages(context);
		P_char target = effect.target == item_action_effect_target::actor ? context.actor :
										    context.target;
		if (effect.target == item_action_effect_target::actor &&
		    affected_by_spell(target, effect.id))
			return;
		for (int audience : { TO_CHAR, TO_VICT, TO_NOTVICT })
			act("&+W$p releases its gathered magic!&n", FALSE, context.actor,
			    context.source, context.target, audience);
		skills[effect.id].spell_pointer(effect.power, context.actor, nullptr,
						SPELL_TYPE_SPELL, target, context.source);
	}
	void finish(const item_action_identity &identity, item_action_consumption,
		    item_action_outcome outcome) const noexcept override
	{
		if (outcome == item_action_outcome::completed)
			return;
		const char *name = kind == weapon_kind::avernus ? "Avernus" : "the weapon";
		for (P_obj source = object_list; source; source = source->next)
			if (source->obj_uid == identity.source_uid && OBJ_SHORT(source))
			{
				name = OBJ_SHORT(source);
				break;
			}
		const std::string message =
			"The unfinished energy gathering in " + std::string(name) + "&n fades.\r\n";
		if (world && identity.origin_room >= 0 && identity.origin_room <= top_of_world)
			send_to_room(message.c_str(), identity.origin_room);
		if (P_char actor = find_character_by_runtime_id(identity.actor_id); IS_ALIVE(actor))
			if (actor->in_room != identity.origin_room)
				send_to_char(message.c_str(), actor);
		if (P_char target = find_character_by_runtime_id(identity.target_id);
		    IS_ALIVE(target))
			if (target->in_room != identity.origin_room)
				send_to_char(message.c_str(), target);
		// Cost/cooldown are retained, including partial resolution and rollback.
	}
};

bool publish(binding &entry, const weapon_config &config)
{
	if (!config.valid || !config.enabled)
	{
		item_actions_disable(entry.id);
		entry.config = config;
		return false;
	}
	if (entry.config == config && item_actions_definition_revision(entry.id) == entry.revision)
		return true;
	if (entry.revision == std::numeric_limits<uint64_t>::max())
		return false;
	if (config.cost &&
	    !artifact_mana_publish(entry.vnum, { static_cast<uint64_t>(entry.vnum),
						 static_cast<uint64_t>(config.mana_revision),
						 static_cast<uint64_t>(config.capacity),
						 static_cast<uint64_t>(config.regeneration), 0 }))
	{
		item_actions_disable(entry.id);
		return false;
	}
	item_action_definition definition;
	definition.id = entry.id;
	definition.revision = entry.revision + 1;
	definition.windup_pulses = config.windup;
	definition.progress_pulses = config.windup / 2;
	definition.selected_effects = true;
	if (!item_actions_publish(definition,
				  std::make_unique<weapon_adapter>(entry.kind, entry.vnum, config)))
		return false;
	entry.revision = definition.revision;
	entry.config = config;
	return true;
}

item_action_start prepare(weapon_kind kind, P_obj source, uint32_t &id)
{
	if (!item_actions_enabled())
		return item_action_start::legacy;
	if (!source || source->R_num < 0)
		return item_action_start::suppressed;
	const int vnum = OBJ_VNUM(source);
	if (vnum <= 0 || vnum >= (1 << 28))
		return item_action_start::suppressed;
	const auto config = configuration(kind, vnum);
	if (config.valid && !config.enabled)
		return item_action_start::legacy;
	id = (static_cast<uint32_t>(kind) << 28) | static_cast<uint32_t>(vnum);
	auto found = bindings.try_emplace(id, binding{ kind, vnum, id, 0, {} }).first;
	return publish(found->second, config) ? item_action_start::scheduled :
						item_action_start::suppressed;
}
} // namespace

void update_weapon_action_properties()
{
	for (auto &[id, entry] : bindings)
	{
		(void)id;
		publish(entry, configuration(entry.kind, entry.vnum));
	}
}

item_action_start selected_avernus_action(P_obj source, P_char actor, P_char target, int damage,
					  int cap)
{
	if (!native_artifact_control_enabled(19730))
		return item_action_start::suppressed;
	if (!native_artifact_variant_enabled(19730, actor, "telegraphic"))
		return item_action_start::legacy;
	if (!native_artifact_power_enabled(19730, "drain"))
		return item_action_start::suppressed;
	uint32_t id = 0;
	const auto routing = prepare(weapon_kind::avernus, source, id);
	if (routing != item_action_start::scheduled)
		return routing;
	item_action_selection selection;
	selection.effect_count = 1;
	selection.effects[0] = {
		1, std::min(damage, native_artifact_power_level(19730, "drain", damage)),
		item_action_call::weapon, item_action_effect_target::original, cap
	};
	return start_selected_item_action(id, actor, target, source, selection);
}

item_action_start selected_packed_weapon_action(P_obj source, P_char actor, P_char target)
{
	if (!item_actions_enabled())
		return item_action_start::legacy;
	if (!source)
		return item_action_start::suppressed;
	if (source->value[5] < 0)
	{
		uint32_t invalid_id = 0;
		return prepare(weapon_kind::packed, source, invalid_id) ==
				       item_action_start::legacy ?
			       item_action_start::legacy :
			       item_action_start::suppressed;
	}
	int spells[3] = { static_cast<int>(source->value[5] % 1000),
			  static_cast<int>(source->value[5] % 1000000 / 1000),
			  static_cast<int>(source->value[5] % 1000000000 / 1000000) };
	int count = 0;
	for (int spell : spells)
		if (spell)
			++count;
	bool offensive = false;
	for (int i = 0; i < count; ++i)
		if (valid_spell(spells[i]) && IS_AGG_SPELL(spells[i]))
			offensive = true;
	if (!offensive)
		return item_action_start::
			legacy; // No additional RNG consumed for a pure beneficial bundle.
	uint32_t id = 0;
	const auto routing = prepare(weapon_kind::packed, source, id);
	if (routing != item_action_start::scheduled)
		return routing;
	if (source->value[5] < 0 || source->value[6] < 0)
		return item_action_start::suppressed;
	const bool random = source->value[5] > 999999999;
	int index = random ? number(0, count - 1) : count - 1;
	item_action_selection selection;
	for (; index >= 0; --index)
	{
		const int spell = spells[index];
		if (valid_spell(spell) && (IS_AGG_SPELL(spell) || !affected_by_spell(actor, spell)))
		{
			const auto target_kind = IS_AGG_SPELL(spell) ?
							 item_action_effect_target::original :
							 item_action_effect_target::actor;
			const int ordinal = static_cast<int>(selection.effect_count);
			selection.effects[selection.effect_count++] = {
				static_cast<uint32_t>(spell), static_cast<int>(source->value[6]),
				item_action_call::spell, target_kind, ordinal
			};
		}
		if (random)
			break;
	}
	if (!selection.effect_count)
		return item_action_start::suppressed;
	return start_selected_item_action(id, actor, target, source, selection);
}

item_action_start selected_random_weapon_action(P_obj source, P_char actor, P_char target,
						int spell, int selected_at)
{
	if (valid_spell(spell) && !IS_AGG_SPELL(spell))
		return item_action_start::legacy;
	uint32_t id = 0;
	const auto routing = prepare(weapon_kind::random, source, id);
	if (routing != item_action_start::scheduled)
		return routing;
	if (!valid_spell(spell))
		return item_action_start::suppressed;
	item_action_selection selection;
	selection.effect_count = 1;
	selection.effects[0] = { static_cast<uint32_t>(spell), 50, item_action_call::spell,
				 item_action_effect_target::original, selected_at };
	return start_selected_item_action(id, actor, target, source, selection);
}
