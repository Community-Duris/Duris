#include "item/device_actions.h"
#include "item/wonder_actions.h"

#include "core/prototypes.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "net/comm.h"

#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <vector>

extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern Skill skills[];

namespace
{
constexpr size_t MAX_DEVICE_ARGUMENT = 500; // parse_spell_arguments has a 512-byte scratch buffer.
constexpr size_t MAX_SCROLL_CLEANUP = 4096;
constexpr size_t MAX_STAFF_TARGETS = 4096;
enum class device_kind : uint32_t
{
	wand = 4,
	staff = 5,
	scroll_carried = 6,
	scroll_worn = 7
};
enum class device_target
{
	none,
	character,
	object,
	area
};
struct device_config
{
	bool valid = false, enabled = false;
	int windup = 8;
	bool operator==(const device_config &) const = default;
};
struct device_binding
{
	device_kind kind;
	int vnum;
	device_config config;
	uint64_t revision = 1;
};
struct device_effect
{
	int spell = 0, power = 0;
	device_target target = device_target::none;
	uint64_t target_id = 0;
	int object_location = 0;
	std::string arguments;
};
std::map<uint32_t, device_binding> device_bindings;
// A slot is reserved at commit. false = active ink reservation; true = ready
// for physical removal after the current callback/transition has unwound.
std::map<uint64_t, bool> consumed_scrolls;

bool is_scroll(device_kind kind)
{
	return kind == device_kind::scroll_carried || kind == device_kind::scroll_worn;
}
bool device_integer(const std::string &key, int fallback, int low, int high, int &value)
{
	const float raw = get_property(key.c_str(), static_cast<double>(fallback), false);
	if (!std::isfinite(raw) || raw < low || raw > high || std::floor(raw) != raw)
		return false;
	value = static_cast<int>(raw);
	return true;
}
device_config device_configuration(device_kind kind, int vnum)
{
	device_config result;
	int enabled = 0, individual = 1;
	const std::string category = kind == device_kind::wand	? "itemActions.wands" :
				     kind == device_kind::staff ? "itemActions.staves" :
								  "itemActions.scrolls";
	result.valid = device_integer(category + ".enabled", 0, 0, 1, enabled) &&
		       device_integer(category + ".windupPulses", 8, 4, 600, result.windup) &&
		       device_integer("itemActions.device." + std::to_string(vnum) + ".enabled", 1,
				      0, 1, individual);
	result.enabled = enabled == 1 && individual == 1;
	return result;
}
P_obj device_object(uint64_t uid)
{
	if (!uid)
		return nullptr;
	P_obj found = nullptr;
	for (P_obj obj = object_list; obj; obj = obj->next)
		if (obj->obj_uid == uid)
		{
			if (found)
				return nullptr;
			found = obj;
		}
	return found;
}
bool device_spell(int spell)
{
	return spell > 0 && spell < MAX_SKILLS && skills[spell].spell_pointer;
}
bool device_actor(P_char actor, device_kind kind)
{
	return IS_ALIVE(actor) && IS_AWAKE(actor) && GET_STAT(actor) >= STAT_RESTING &&
	       !IS_IMMOBILE(actor) && !IS_AFFECTED2(actor, AFF2_STUNNED) &&
	       !CHAR_IN_NO_MAGIC_ROOM(actor) && !(IS_NPC(actor) && IS_AFFECTED(actor, AFF_CHARM)) &&
	       (!is_scroll(kind) || !is_silent(actor, false));
}
bool device_harm(P_char actor, P_char target)
{
	if (is_in_safe(actor) || IS_AFFECTED5(actor, AFF5_NOT_OFFENSIVE) ||
	    affected_by_spell(actor, SONG_PEACE) || affected_by_spell(actor, SKILL_GAZE))
		return false;
	if (target && target != actor)
		return CanDoFightMove(actor, target) &&
		       (!IS_PC(target) || !should_not_kill(actor, target));
	if (target == actor)
		return true;
	return !GET_MASTER(actor) || !should_area_hit(actor, GET_MASTER(actor));
}
bool device_target_valid(const device_effect &effect, P_char actor, int origin)
{
	if (!device_spell(effect.spell))
		return false;
	if (effect.target == device_target::character)
	{
		P_char target = find_character_by_runtime_id(effect.target_id);
		if (!IS_ALIVE(target) || target->in_room != origin || !CAN_SEE(actor, target))
			return false;
		if (IS_SET(skills[effect.spell].targets, TAR_SELF_ONLY) && target != actor)
			return false;
		if (IS_SET(skills[effect.spell].targets, TAR_SELF_NONO) && target == actor)
			return false;
		if (IS_ROOM(origin, ROOM_SINGLE_FILE) && target != actor &&
		    !AdjacentInRoom(actor, target))
			return false;
		return !IS_AGG_SPELL(effect.spell) || device_harm(actor, target);
	}
	if (effect.target == device_target::object)
	{
		P_obj target = device_object(effect.target_id);
		if (!target || target->loc_p != effect.object_location ||
		    !CAN_SEE_OBJ(actor, target))
			return false;
		if (!OBJ_CARRIED_BY(target, actor) && !OBJ_WORN_BY(target, actor) &&
		    !(OBJ_ROOM(target) && target->loc.room == origin))
			return false;
	}
	if (IS_ROOM(origin, ROOM_SINGLE_FILE) && IS_SET(skills[effect.spell].targets, TAR_AREA) &&
	    !IS_SET(skills[effect.spell].targets, TAR_CHAR_ROOM))
		return false;
	return !IS_AGG_SPELL(effect.spell) || device_harm(actor, nullptr);
}
void device_wake(uint64_t actor_id, uint64_t victim_id, int spell)
{
	P_char actor = find_character_by_runtime_id(actor_id);
	P_char victim = find_character_by_runtime_id(victim_id);
	if (!IS_ALIVE(actor) || !IS_ALIVE(victim) || actor == victim || !IS_AGG_SPELL(spell))
		return;
	if (affected_by_spell(victim, SPELL_SLEEP))
		affect_from_char(victim, SPELL_SLEEP);
	// Affect removal can alter state; reacquire before the legacy wake-up messages.
	actor = find_character_by_runtime_id(actor_id);
	victim = find_character_by_runtime_id(victim_id);
	if (!IS_ALIVE(actor) || !IS_ALIVE(victim) || GET_STAT(victim) != STAT_SLEEPING)
		return;
	send_to_char("Your rest is violently disturbed!\r\n", victim);
	act("Your spell disturbs $N's beauty sleep!", FALSE, actor, nullptr, victim, TO_CHAR);
	act("$n's spell disturbs $N's beauty sleep!", FALSE, actor, nullptr, victim, TO_NOTVICT);
	SET_POS(victim, GET_POS(victim) + STAT_NORMAL);
}

class device_adapter final : public item_action_adapter
{
	const device_kind kind;
	const int vnum;
	const std::array<device_effect, ITEM_ACTION_MAX_EFFECTS> effects;
	const size_t effect_count;
	const std::string source_name;

	bool current(const item_action_identity &identity, P_char &actor,
		     P_obj &source) const noexcept
	{
		if (!item_action_pending(identity.action_id))
			return false;
		actor = find_character_by_runtime_id(identity.actor_id);
		if (!device_actor(actor, kind) || actor->in_room != identity.origin_room)
			return false;
		source = nullptr;
		if (identity.source_slot >= 0)
		{
			source = actor->equipment[identity.source_slot];
			return source && source->obj_uid == identity.source_uid &&
			       OBJ_WORN_BY(source, actor);
		}
		for (source = actor->carrying; source; source = source->next_content)
			if (source->obj_uid == identity.source_uid && OBJ_CARRIED_BY(source, actor))
				return true;
		return false;
	}
	void area(const item_action_context &context, const device_effect &effect) const noexcept
	{
		const auto &identity = context.identity;
		P_char actor = context.actor;
		if (IS_AGG_SPELL(effect.spell))
			appear(actor);
		// Copy argument storage for callbacks that consume/modify it.
		std::string arguments = effect.arguments;
		if (IS_SET(skills[effect.spell].targets, TAR_AREA | TAR_IGNORE))
		{
			skills[effect.spell].spell_pointer(
				effect.power, actor,
				IS_AGG_SPELL(effect.spell) ? arguments.data() : nullptr,
				SPELL_TYPE_SPELL, nullptr, nullptr);
			return;
		}
		std::vector<uint64_t> targets;
		for (P_char target = world[identity.origin_room].people; target;
		     target = target->next_in_room)
		{
			if (targets.size() == MAX_STAFF_TARGETS)
				return; // Bounded area invocation; no partial overflow dispatch.
			targets.push_back(target->runtime_id);
		}
		for (uint64_t target_id : targets)
		{
			P_obj source = nullptr;
			if (!current(identity, actor, source))
				return;
			P_char target = find_character_by_runtime_id(target_id);
			if (!IS_ALIVE(target) || target->in_room != identity.origin_room)
				continue;
			if (IS_AGG_SPELL(effect.spell))
			{
				if (target == actor ||
				    (actor->group && actor->group == target->group) ||
				    !IS_SET(skills[effect.spell].targets, TAR_CHAR_ROOM) ||
				    !device_harm(actor, target))
					continue;
			}
			else if (WIZ_INVIS(actor, target))
				continue;
			arguments = effect.arguments;
			if (IS_SET(skills[effect.spell].targets, TAR_CHAR_ROOM))
				skills[effect.spell].spell_pointer(effect.power, actor,
								   arguments.data(),
								   SPELL_TYPE_SPELL, target,
								   nullptr);
			else if (IS_SET(skills[effect.spell].targets, TAR_SELF_ONLY))
				skills[effect.spell].spell_pointer(effect.power, target,
								   arguments.data(),
								   SPELL_TYPE_SPELL, target,
								   nullptr);
			// No pointer from this iteration survives a spell callback.
		}
	}

    public:
	device_adapter(device_kind type, int object_vnum,
		       std::array<device_effect, ITEM_ACTION_MAX_EFFECTS> selected, size_t count,
		       std::string name)
		: kind(type)
		, vnum(object_vnum)
		, effects(std::move(selected))
		, effect_count(count)
		, source_name(std::move(name))
	{
	}
	bool references_character(uint64_t id) const noexcept override
	{
		for (size_t i = 0; i < effect_count; ++i)
			if (effects[i].target == device_target::character &&
			    effects[i].target_id == id)
				return true;
		return false;
	}
	bool references_object(uint64_t uid) const noexcept override
	{
		for (size_t i = 0; i < effect_count; ++i)
			if (effects[i].target == device_target::object &&
			    effects[i].target_id == uid)
				return true;
		return false;
	}
	bool validate(const item_action_context &context) const noexcept override
	{
		const int type = is_scroll(kind)	   ? ITEM_SCROLL :
				 kind == device_kind::wand ? ITEM_WAND :
							     ITEM_STAFF;
		if (!device_actor(context.actor, kind) || OBJ_VNUM(context.source) != vnum ||
		    context.source->type != type ||
		    (IS_OBJ_STAT2(context.source, ITEM2_ACCOUNT_BOUND) &&
		     !account_bound_reward_owner(context.actor, context.source)))
			return false;
		for (size_t i = 0; i < effect_count; ++i)
			if (!device_target_valid(effects[i], context.actor,
						 context.identity.origin_room))
				return false;
		return true;
	}
	item_action_consumption commit(const item_action_context &context) const noexcept override
	{
		if (is_scroll(kind))
		{
			if (consumed_scrolls.size() >= MAX_SCROLL_CLEANUP ||
			    !consumed_scrolls.try_emplace(context.identity.source_uid, false).second)
				return item_action_consumption::rejected;
			// Ink is consumed before presentation. The source remains available to
			// the action; transfer/abort cannot restore any of its spell slots.
			for (int slot = 1; slot <= 3; ++slot)
				context.source->value[slot] = 0;
		}
		else
		{
			if (context.source->value[2] <= 0)
				return item_action_consumption::rejected;
			--context.source->value[2];
		}
		return item_action_consumption::committed;
	}
	void announce(const item_action_context &context) const noexcept override
	{
		act(is_scroll(kind) ? "&+WYou begin reciting $p as its ink burns away!&n" :
				      "&+WYou begin channeling magic through $p!&n",
		    FALSE, context.actor, context.source, nullptr, TO_CHAR);
		act(is_scroll(kind) ? "&+W$n begins reciting $p as its ink burns away!&n" :
				      "&+W$n begins channeling magic through $p!&n",
		    FALSE, context.actor, context.source, nullptr, TO_ROOM);
		for (size_t i = 0; i < effect_count; ++i)
		{
			if (!IS_AGG_SPELL(effects[i].spell))
				continue;
			if (effects[i].target == device_target::character)
			{
				P_char target = find_character_by_runtime_id(effects[i].target_id);
				if (target && target != context.actor)
					act("&+R$n is aiming $p's gathering magic at YOU!&n", FALSE,
					    context.actor, context.source, target, TO_VICT);
			}
			else
				act("&+RThe magic gathering in $p threatens the room!&n", FALSE,
				    context.actor, context.source, nullptr, TO_ROOM);
		}
	}
	void progress(const item_action_context &context) const noexcept override
	{
		act(is_scroll(kind) ? "&+WYou approach the final words of $p!&n" :
				      "&+WThe magic you channel through $p intensifies!&n",
		    FALSE, context.actor, context.source, nullptr, TO_CHAR);
		act(is_scroll(kind) ? "&+W$n approaches the final words of $p!&n" :
				      "&+WThe magic $n channels through $p intensifies!&n",
		    FALSE, context.actor, context.source, nullptr, TO_ROOM);
	}
	void resolve(const item_action_context &context,
		     const item_action_effect &selected) const noexcept override
	{
		const auto &effect = effects[static_cast<size_t>(selected.auxiliary)];
		if (!selected.auxiliary)
		{
			act(is_scroll(kind) ? "&+WYou finish reciting $p and release its magic!&n" :
					      "&+WYou release the magic channeled through $p!&n",
			    FALSE, context.actor, context.source, nullptr, TO_CHAR);
			act(is_scroll(kind) ?
				    "&+W$n finishes reciting $p and releases its magic!&n" :
				    "&+W$n releases the magic channeled through $p!&n",
			    FALSE, context.actor, context.source, nullptr, TO_ROOM);
		}
		if (effect.target == device_target::area)
		{
			area(context, effect);
			return;
		}
		P_char target = effect.target == device_target::character ?
					find_character_by_runtime_id(effect.target_id) :
					nullptr;
		P_obj object = effect.target == device_target::object ?
				       device_object(effect.target_id) :
				       nullptr;
		if (IS_AGG_SPELL(effect.spell) && target && target != context.actor)
			appear(context.actor);
		std::string arguments = effect.arguments;
		skills[effect.spell].spell_pointer(
			effect.power, context.actor,
			is_scroll(kind) || IS_SET(skills[effect.spell].targets, TAR_IGNORE) ?
				arguments.data() :
				nullptr,
			SPELL_TYPE_SPELL, target, object);
		if (!item_action_pending(context.identity.action_id))
			return;
		device_wake(context.identity.actor_id,
			    effect.target == device_target::character ? effect.target_id : 0,
			    effect.spell);
	}
	void finish(const item_action_identity &identity, item_action_consumption,
		    item_action_outcome outcome) const noexcept override
	{
		if (is_scroll(kind))
		{
			auto found = consumed_scrolls.find(identity.source_uid);
			if (found != consumed_scrolls.end())
				found->second = true;
		}
		if (outcome == item_action_outcome::completed)
			return;
		const std::string message =
			"The unfinished magic in " + source_name + "&n fades.\r\n";
		if (identity.origin_room >= 0 && identity.origin_room <= top_of_world)
			send_to_room(message.c_str(), identity.origin_room);
		P_char actor = find_character_by_runtime_id(identity.actor_id);
		if (IS_ALIVE(actor) && actor->in_room != identity.origin_room)
			send_to_char(message.c_str(), actor);
	}
};
} // namespace

void update_device_action_properties()
{
	if (!nevent_require_game_thread("update_device_action_properties"))
		return;
	update_wonder_action_properties();
	for (auto &[id, entry] : device_bindings)
	{
		const auto updated = device_configuration(entry.kind, entry.vnum);
		if (entry.config != updated)
		{
			item_actions_disable(id);
			entry.config = updated;
			if (entry.revision != std::numeric_limits<uint64_t>::max())
				++entry.revision;
		}
	}
}

void device_actions_pulse()
{
	if (!nevent_require_game_thread("device_actions_pulse"))
		return;
	std::vector<uint64_t> ready;
	for (auto it = consumed_scrolls.begin(); it != consumed_scrolls.end();)
		if (it->second)
		{
			ready.push_back(it->first);
			it = consumed_scrolls.erase(it);
		}
		else
			++it;
	for (uint64_t uid : ready)
		if (P_obj source = device_object(uid))
			extract_obj(source);
}

item_action_start begin_device_action(P_obj source, P_char actor, const char *arguments)
{
	if (!nevent_require_game_thread("begin_device_action"))
		return item_action_start::suppressed;
	if (!item_actions_enabled() || !source ||
	    (source->type != ITEM_WAND && source->type != ITEM_STAFF &&
	     source->type != ITEM_SCROLL))
		return item_action_start::legacy;
	if (!IS_ALIVE(actor) || source->R_num < 0)
		return item_action_start::suppressed;
	const auto kind = source->type == ITEM_WAND  ? device_kind::wand :
			  source->type == ITEM_STAFF ? device_kind::staff :
			  OBJ_WORN_BY(source, actor) ? device_kind::scroll_worn :
						       device_kind::scroll_carried;
	const int vnum = OBJ_VNUM(source);
	if (vnum <= 0 || vnum >= (1 << 28))
		return item_action_start::suppressed;
	const auto config = device_configuration(kind, vnum);
	if (config.valid && !config.enabled)
		return item_action_start::legacy;
	const uint32_t id = (static_cast<uint32_t>(kind) << 28) | static_cast<uint32_t>(vnum);
	auto &binding = device_bindings.try_emplace(id, device_binding{ kind, vnum, config, 1 })
				.first->second;
	if (binding.config != config)
	{
		item_actions_disable(id);
		binding.config = config;
		if (binding.revision != std::numeric_limits<uint64_t>::max())
			++binding.revision;
	}
	if (!config.valid || binding.revision == std::numeric_limits<uint64_t>::max() ||
	    !device_actor(actor, kind) || !arguments ||
	    std::char_traits<char>::length(arguments) > MAX_DEVICE_ARGUMENT)
		return item_action_start::suppressed;
	if (consumed_scrolls.contains(source->obj_uid))
		return item_action_start::suppressed;
	if (!is_scroll(kind) && source->value[2] <= 0)
	{
		send_to_char("There are no charges available to channel.\r\n", actor);
		return item_action_start::suppressed;
	}
	if (source->value[0] < 0)
		return item_action_start::suppressed;
	std::array<device_effect, ITEM_ACTION_MAX_EFFECTS> effects;
	size_t count = 0;
	P_char anchor = actor;
	const int first = is_scroll(kind) ? 1 : 3;
	for (int slot = first; slot <= 3; ++slot)
	{
		const int spell = source->value[slot];
		if (is_scroll(kind) && !spell)
			continue;
		if (!device_spell(spell))
		{
			send_to_char(
				"The device's spell definition is invalid; no resource was consumed.\r\n",
				actor);
			return item_action_start::suppressed;
		}
		device_effect effect;
		if (kind != device_kind::staff &&
		    IS_SET(skills[spell].targets, TAR_CHAR_WORLD | TAR_OBJ_WORLD))
		{
			send_to_char("This remote spell is not supported by device channeling.\r\n",
				     actor);
			return item_action_start::suppressed;
		}
		if (kind == device_kind::staff &&
		    !IS_SET(skills[spell].targets,
			    TAR_AREA | TAR_IGNORE | TAR_CHAR_ROOM | TAR_SELF_ONLY))
			return item_action_start::suppressed;
		effect.spell = spell;
		effect.power = source->value[0];
		effect.arguments = arguments;
		if (kind == device_kind::staff)
			effect.target = device_target::area;
		else
		{
			spell_target_data parsed{};
			parsed.ttype = spell;
			std::string copy = arguments;
			if (!parse_spell_arguments(actor, &parsed, copy.data()))
				return item_action_start::suppressed;
			if (parsed.t_char)
			{
				effect.target = device_target::character;
				effect.target_id = parsed.t_char->runtime_id;
				if (anchor == actor && parsed.t_char != actor)
					anchor = parsed.t_char;
			}
			else if (parsed.t_obj)
			{
				effect.target = device_target::object;
				effect.target_id = parsed.t_obj->obj_uid;
				effect.object_location = parsed.t_obj->loc_p;
			}
		}
		if (!device_target_valid(effect, actor, actor->in_room))
			return item_action_start::suppressed;
		effects[count++] = std::move(effect);
	}
	if (!count)
		return item_action_start::suppressed;
	item_action_definition definition;
	definition.id = id;
	definition.revision = binding.revision;
	definition.mode = item_action_mode::active;
	definition.source = kind == device_kind::scroll_carried ? item_action_source::carried :
								  item_action_source::equipped;
	definition.windup_pulses = config.windup;
	definition.progress_pulses = config.windup / 2;
	definition.effect_count = count;
	for (size_t i = 0; i < count; ++i)
		definition.effects[i] = { static_cast<uint32_t>(effects[i].spell), effects[i].power,
					  item_action_call::spell,
					  item_action_effect_target::original,
					  static_cast<int>(i) };
	return start_item_action_instance(
		definition,
		std::make_unique<device_adapter>(kind, vnum, std::move(effects), count,
						 OBJ_SHORT(source) ? OBJ_SHORT(source) :
								     "the device"),
		actor, anchor, source);
}
