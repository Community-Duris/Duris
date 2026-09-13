#include "item/native_artifact_actions.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/artifact_mana.h"
#include "magic/spells.h"
#include "net/comm.h"
#include "world/handler.h"

extern P_index obj_index;
extern P_char character_list;

namespace
{
constexpr int NECROPLASM = 67243;
bool necroplasm_form(int spell)
{
	return spell == SPELL_VAMPIRE || spell == SPELL_ANGELIC_COUNTENANCE;
}

class necroplasm_adapter final : public item_action_adapter
{
	const native_artifact_config settings;

    public:
	explicit necroplasm_adapter(native_artifact_config config)
		: settings(config)
	{
	}
	bool validate(const item_action_context &c) const noexcept override
	{
		if (!native_artifact_actor(c.actor) || OBJ_VNUM(c.source) != NECROPLASM ||
		    c.actor->equipment[HOLD] == c.source ||
		    !can_prime_class_use_item(c.actor, c.source) ||
		    affected_by_spell(c.actor, SPELL_VAMPIRE) ||
		    affected_by_spell(c.actor, SPELL_ANGELIC_COUNTENANCE) ||
		    (IS_OBJ_STAT2(c.source, ITEM2_ACCOUNT_BOUND) &&
		     !account_bound_reward_owner(c.actor, c.source)))
			return false;
		for (P_obj worn : c.actor->equipment)
			if (worn && worn != c.source && IS_ARTIFACT(worn))
				return false;
		return true;
	}
	item_action_consumption commit(const item_action_context &c) const noexcept override
	{
		return artifact_mana_debit(c.source, settings.cost, true, c.identity.action_id) ?
			       item_action_consumption::committed :
			       item_action_consumption::rejected;
	}
	void announce(const item_action_context &c) const noexcept override
	{
		act("$p gathers dark energy, preparing to transform you.", FALSE, c.actor, c.source,
		    nullptr, TO_CHAR);
		act("$n's $p gathers dark energy, preparing to transform $m.", FALSE, c.actor,
		    c.source, nullptr, TO_ROOM);
	}
	void resolve(const item_action_context &c,
		     const item_action_effect &) const noexcept override
	{
		// Retain the spell's pet/order and memorized-slot rules as well as its stats.
		spell_vampire(55, c.actor, nullptr, 0, c.actor, nullptr);
		P_char actor = find_character_by_runtime_id(c.identity.actor_id);
		if (!actor)
			return;
		P_obj source = nullptr;
		P_char current_actor = nullptr;
		if (!native_artifact_current(c.identity, current_actor, source))
		{
			// Neither form existed before this invocation. A transition inside the
			// native spell cannot leave its newly-created grant detached from custody.
			affect_from_char(actor, SPELL_VAMPIRE);
			actor = find_character_by_runtime_id(c.identity.actor_id);
			if (actor)
				affect_from_char(actor, SPELL_ANGELIC_COUNTENANCE);
			return;
		}
		for (auto *af = actor->affected; af; af = af->next)
			if (necroplasm_form(af->type))
			{
				af->flags |= AFFTYPE_LINKED_OBJ | AFFTYPE_NOSAVE;
				link_char_obj_with_affect(actor, source, LNK_CHAR_OBJ_AFF, af);
			}
	}
	void finish(const item_action_identity &id, item_action_consumption,
		    item_action_outcome outcome) const noexcept override
	{
		if (outcome != item_action_outcome::completed)
			if (P_char actor = find_character_by_runtime_id(id.actor_id))
				send_to_char("The necroplasm's gathering energy fades.\r\n", actor);
	}
};
}

void release_necroplasm_forms()
{
	for (P_char actor = character_list; actor; actor = actor->next)
	{
		// Removing the affect unlinks its object link. Restart after each removal;
		// no saved affect/link pointer survives the native removal operation.
		for (;;)
		{
			affected_type *owned = nullptr;
			for (auto *link = actor->obj_linked; link; link = link->next)
				if (link->type == LNK_CHAR_OBJ_AFF && link->obj && link->affect &&
				    OBJ_VNUM(link->obj) == NECROPLASM &&
				    necroplasm_form(link->affect->type))
				{
					owned = link->affect;
					break;
				}
			if (!owned)
				break;
			affect_remove(actor, owned);
		}
	}
}

item_action_start begin_necroplasm_form(P_obj source, P_char actor)
{
	if (!native_artifact_owns(NECROPLASM))
		return item_action_start::legacy;
	const auto config = native_artifact_settings(NECROPLASM);
	if (!source || !native_artifact_actor(actor) ||
	    !native_artifact_prepare(NECROPLASM, config))
		return item_action_start::suppressed;
	item_action_definition definition;
	definition.id = native_artifact_ability(NECROPLASM, 1);
	definition.revision = config.revision;
	definition.windup_pulses = config.windup;
	definition.effect_count = 1;
	definition.effects[0].id = 1;
	return start_item_action_instance(definition, std::make_unique<necroplasm_adapter>(config),
					  actor, actor, source);
}
