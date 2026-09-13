#include "item/native_artifact_actions.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/artifact_mana.h"
#include "combat/damage.h"
#include "net/comm.h"
#include <vector>

extern P_room world;
extern P_index obj_index;

namespace
{
constexpr int MIRRORED_IOUN = 922;
class mirrored_ioun_adapter final : public item_action_adapter
{
	const native_artifact_config settings;
	const uint64_t attacker_id;
	const int damage, attack_type;
	const uint flags;
	damage_messages *const messages; // Borrowed only during this synchronous call.
    public:
	mirrored_ioun_adapter(native_artifact_config config, const proc_data &incoming)
		: settings(config)
		, attacker_id(incoming.victim->runtime_id)
		, damage(incoming.dam)
		, attack_type(incoming.attacktype)
		, flags(incoming.flags)
		, messages(incoming.messages)
	{
	}
	bool validate(const item_action_context &c) const noexcept override
	{
		P_char attacker = find_character_by_runtime_id(attacker_id);
		return native_artifact_actor(c.actor) && IS_ALIVE(attacker) &&
		       attacker->in_room == c.identity.origin_room &&
		       OBJ_VNUM(c.source) == MIRRORED_IOUN && !IS_TRUSTED(c.target) &&
		       native_artifact_hostile(c.actor, c.target) &&
		       (!(IS_OBJ_STAT2(c.source, ITEM2_ACCOUNT_BOUND)) ||
			account_bound_reward_owner(c.actor, c.source));
	}
	bool references_character(uint64_t id) const noexcept override { return id == attacker_id; }
	item_action_consumption commit(const item_action_context &c) const noexcept override
	{
		return artifact_mana_debit(c.source, settings.cost, true, c.identity.action_id) ?
			       item_action_consumption::committed :
			       item_action_consumption::rejected;
	}
	void announce(const item_action_context &) const noexcept override {}
	void resolve(const item_action_context &c,
		     const item_action_effect &) const noexcept override
	{
		act("$n's $p flashes and deflects the incoming spell to YOU!", FALSE, c.actor,
		    c.source, c.target, TO_VICT);
		act("$n's $p flashes and deflects the incoming spell to $N!", FALSE, c.actor,
		    c.source, c.target, TO_NOTVICT);
		act("Your $p flashes and deflects the incoming spell to $N!", FALSE, c.actor,
		    c.source, c.target, TO_CHAR);
		spell_damage(c.actor, c.target, damage, attack_type, flags | SPLDAM_NODEFLECT,
			     messages);
		// No pointer use after damage, including the defender: reflection may kill either side.
	}
	void finish(const item_action_identity &, item_action_consumption,
		    item_action_outcome) const noexcept override
	{
	}
};
}

bool intercept_mirrored_ioun(P_obj source, P_char defender, const proc_data &incoming)
{
	const auto config = native_artifact_settings(MIRRORED_IOUN);
	if (!source || !native_artifact_actor(defender) || !IS_ALIVE(incoming.victim) ||
	    incoming.victim == defender || incoming.dam <= 0 ||
	    (incoming.flags & SPLDAM_NODEFLECT) || !native_artifact_prepare(MIRRORED_IOUN, config))
		return false;
	std::vector<uint64_t> targets;
	for (P_char target = world[defender->in_room].people; target; target = target->next_in_room)
	{
		if (targets.size() == 4096)
			return false;
		if (!IS_TRUSTED(target) && native_artifact_hostile(defender, target))
			targets.push_back(target->runtime_id);
	}
	if (targets.empty())
		return false;
	P_char target = find_character_by_runtime_id(targets[number(0, targets.size() - 1)]);
	item_action_definition definition;
	definition.id = native_artifact_ability(MIRRORED_IOUN, 1);
	definition.revision = config.revision;
	definition.effect_count = 1;
	definition.effects[0].id = 1;
	return resolve_item_interception(definition,
					 std::make_unique<mirrored_ioun_adapter>(config, incoming),
					 defender, target, source);
}
