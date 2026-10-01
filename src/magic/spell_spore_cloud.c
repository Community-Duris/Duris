#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "world/vnum.obj.h"
#include "core/utility.h"
#include "core/utils.h"
#include "economy/economic_gameplay_authority.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"
#include "magic/spell_item_lifecycle.h"

void spell_spore_cloud(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"&+ySickly spores&+c impale $N causing $m to writhe in pain!",
		"&+ySickly spores&+c rain down on you!",
		"&+cA cloud of &+yspores&+c engulfs $N!",
		"&+c$N is caught amidst a &+ycloud of spores&+c and is punctured to &+Rdeath!",
		"&+c$N is caught amidst a &+ycloud of spores&+c and is punctured to &+Rdeath!",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim))
		return;

	if (!NewSaves(victim, SAVING_BREATH, 0))
	{
		send_to_char("&+CYou scream in pain as &+yspores&+C lance into your flesh!\n",
			     victim);
		act("$n &+Cscreams in pain as &+yhundreds of spores&+C lance into $m.", TRUE,
		    victim, 0, 0, TO_ROOM);
		spell_disease(level, ch, NULL, SPELL_TYPE_SPELL, victim, 0);
	}

	dam = 100 + level * 4 + number(1, 20);
	// Meteorswarm damage is: 100 + level * 6 + number(1, 40);
	if (IS_PC(ch) && IS_PC(victim))
	{
		dam = dam * get_property("spell.area.damage.to.pc", 0.5);
	}

	switch (world[ch->in_room].sector_type)
	{
	case SECT_UNDERWATER:
	case SECT_UNDERWATER_GR:
	case SECT_UNDRWLD_WATER:
	case SECT_UNDRWLD_NOSWIM:
	case SECT_WATER_SWIM:
	case SECT_WATER_NOSWIM:
	case SECT_OCEAN:
	case SECT_FIREPLANE:
	case SECT_WATER_PLANE:
	case SECT_UNDRWLD_INSIDE:
	case SECT_INSIDE:
	case SECT_LAVA:
		dam = (int)(dam * 0.7);
		break;
	case SECT_CITY:
	case SECT_DESERT:
	case SECT_ROAD:
	case SECT_MOUNTAIN:
	case SECT_UNDRWLD_CITY:
	case SECT_EARTH_PLANE:
		dam = (int)(dam * 0.85);
		break;
	case SECT_UNDRWLD_WILD:
	case SECT_SWAMP:
	case SECT_FOREST:
	case SECT_AIR_PLANE:
		dam = (int)(dam * 1.15);
		break;
	default:
		dam = dam;
		break;
	}
	if (spell_damage(ch, victim, dam, SPLDAM_GAS, 0, &messages) != DAM_NONEDEAD)
		return;
}

static void event_spore_burst(P_char, P_char, P_obj, void *);

struct sb_data
{
	int room;
	int spores;
};

spell_component_effect_status spell_spore_burst_initial_components_completed(
	const critical_operation_id & /*operation_id*/, P_char ch, bool committed,
	const item_transfer_result &, unsigned int /*error_code*/, const uint8_t *encoded,
	size_t encoded_size)
{
	spell_component_context_reader reader(encoded, encoded_size);
	int32_t room = 0;
	int32_t spores = 0;
	if (!ch || !reader.get_i32(&room) || !reader.get_i32(&spores) || !reader.finished())
		return spell_component_effect_status::retry;
	if (!committed || ch->in_room != room)
	{
		send_to_char("Your spell fizzles before the sphere of spores can form.\r\n", ch);
		return spell_component_effect_status::complete;
	}
	send_to_room(
		"&+cA &+ysickly yellow &+ysphere &+cstarts to grow in the center of the room..\n",
		room);
	sb_data context = { room, spores };
	add_event(event_spore_burst, (int)(PULSE_VIOLENCE * 1.5), ch, 0, 0, 0, &context,
		  sizeof(context));
	CharWait(ch, (int)2.5 * PULSE_VIOLENCE);
	return spell_component_effect_status::complete;
}

spell_component_effect_status spell_spore_burst_repeat_components_completed(
	const critical_operation_id & /*operation_id*/, P_char ch, bool committed,
	const item_transfer_result &, unsigned int /*error_code*/, const uint8_t *encoded,
	size_t encoded_size)
{
	spell_component_context_reader reader(encoded, encoded_size);
	int32_t room = 0;
	int32_t spores = 0;
	if (!ch || !reader.get_i32(&room) || !reader.get_i32(&spores) || !reader.finished())
		return spell_component_effect_status::retry;
	if (!committed || ch->in_room != room)
	{
		send_to_char("The growing sphere of spores collapses.\r\n", ch);
		return spell_component_effect_status::complete;
	}
	send_to_room("&+CA mass of &+Yspores&+C coalesce into a growing sphere...\n", room);
	sb_data context = { room, spores };
	add_event(event_spore_burst, static_cast<int>(PULSE_VIOLENCE), ch, 0, 0, 0, &context,
		  sizeof(context));
	return spell_component_effect_status::complete;
}

void spell_spore_burst(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char /*victim*/,
		       P_obj /*obj*/)
{
	struct sb_data sbdata;
	int garlic;

	switch (world[ch->in_room].sector_type)
	{
	case SECT_UNDRWLD_WILD:
	case SECT_SWAMP:
	case SECT_FOREST:
	case SECT_AIR_PLANE:
		break;
	default:
		send_to_char("You instinctively know this isn't ideal terrain...\n", ch);
		break;
	}

	if (economic_gameplay_authority::active() && IS_PC(ch))
	{
		sbdata.room = ch->in_room;
		sbdata.spores = 1;
		spell_component_context_writer context;
		if (!context.put_i32(sbdata.room) || !context.put_i32(sbdata.spores))
			return;
		if (!spell_consume_components(ch, VOBJ_FORAGE_GARLIC, 1, SPELL_SPORE_BURST,
					      item_spell_component_effect::spore_burst_initial,
					      spell_spore_burst_initial_components_completed,
					      context.data(), context.size))
		{
			send_to_char("You must have &+Wsome garlic&n in your inventory.\n", ch);
			act("&+W$n's&+G spell fizzles and dies before any growth can begin.\n",
			    TRUE, ch, 0, 0, TO_ROOM);
		}
		return;
	}
	garlic = get_spell_component(ch, VOBJ_FORAGE_GARLIC, 1);
	if (!garlic)
	{
		send_to_char("You must have &+Wsome garlic&n in your inventory.\n", ch);
		act("&+W$n's&+G spell fizzles and dies before any growth can begin.\n", TRUE, ch, 0,
		    0, TO_ROOM);
		return;
	}

	send_to_room(
		"&+cA &+ysickly yellow &+ysphere &+cstarts to grow in the center of the room..\n",
		ch->in_room);

	sbdata.room = ch->in_room;
	sbdata.spores = 1;

	add_event(event_spore_burst, (int)(PULSE_VIOLENCE * 1.5), ch, 0, 0, 0, &sbdata,
		  sizeof(struct sb_data));
	CharWait(ch, (int)2.5 * PULSE_VIOLENCE);
}

static void event_spore_burst(P_char ch, P_char /*victim*/, P_obj /*obj*/, void *data)
{
	struct sb_data *sbdata = (struct sb_data *)data;
	int garlic;

	if (!sbdata)
	{
		debug("Passed null pointer to event_spore_burst.");
		return;
	}

	if (sbdata->room != ch->in_room)
	{
		send_to_char("&+GYour spell fizzles and the sphere of &+yspores &+Gcollapses!\n",
			     ch);
		;
		act("&+W$n's&+G spell fizzles and the sphere of &+yspores &+Gcollapses!", TRUE, ch,
		    0, 0, TO_ROOM);
		return;
	}

	garlic = 0;

	if (number(0, 1) && (sbdata->spores < 3))
	{
		if (economic_gameplay_authority::active() && IS_PC(ch))
		{
			sb_data next_spore = *sbdata;
			++next_spore.spores;
			spell_component_context_writer context;
			if (!context.put_i32(next_spore.room) ||
			    !context.put_i32(next_spore.spores))
				return;
			if (spell_consume_components(ch, VOBJ_FORAGE_GARLIC, 1, SPELL_SPORE_BURST,
						     item_spell_component_effect::spore_burst_repeat,
						     spell_spore_burst_repeat_components_completed,
						     context.data(), context.size))
				return;
		}
		else
		{
			garlic = get_spell_component(ch, VOBJ_FORAGE_GARLIC, 1);
			sbdata->spores++;
		}
	}

	if (garlic)
	{
		send_to_room("&+CA mass of &+Yspores&+C coalesce into a growing sphere...\n",
			     sbdata->room);
		add_event(event_spore_burst, (int)(PULSE_VIOLENCE), ch, 0, 0, 0, sbdata,
			  sizeof(struct sb_data));
		return;
	}
	else
	{
		send_to_room("&+gThe&+G immense &+ysphere of spores&+R erupts!\n", sbdata->room);
		cast_as_damage_area(ch, spell_spore_cloud,
				    (int)sbdata->spores - 1 * 4 + GET_LEVEL(ch), NULL,
				    get_property("spell.area.minChance.spore", 90),
				    get_property("spell.area.chanceStep.spore", 10));
		return;
	}
}
