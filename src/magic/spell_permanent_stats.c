#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
void spell_perm_increase_str(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n",
		     ch->in_room);

	if (victim->base_stats.Str >= 95)
	{
		send_to_char("&+BNothing seem to happen..\n", victim);
		return;
	}

	send_to_char("&+BYou feel your strength improve..\n", victim);
	victim->base_stats.Str = BOUNDED(1, victim->base_stats.Str + 1, 95);
	victim->curr_stats.Str = BOUNDED(1, victim->curr_stats.Str + 1, 95);
	balance_affects(victim);
}

void spell_perm_increase_agi(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n",
		     ch->in_room);

	if (victim->base_stats.Agi >= 95)
	{
		send_to_char("&+BNothing seem to happen..\n", victim);
		return;
	}

	send_to_char("&+BYou feel your agility improve..\n", victim);
	victim->base_stats.Agi = BOUNDED(1, victim->base_stats.Agi + 1, 95);
	victim->curr_stats.Agi = BOUNDED(1, victim->curr_stats.Agi + 1, 95);
	balance_affects(victim);
}

void spell_perm_increase_dex(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n",
		     ch->in_room);

	if (victim->base_stats.Dex >= 95)
	{
		send_to_char("&+BNothing seem to happen..\n", victim);
		return;
	}

	send_to_char("&+BYou feel your dexterity grow..\n", victim);
	victim->base_stats.Dex = BOUNDED(1, victim->base_stats.Dex + 1, 95);
	victim->curr_stats.Dex = BOUNDED(1, victim->curr_stats.Dex + 1, 95);
	balance_affects(victim);
}

void spell_perm_increase_con(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n",
		     ch->in_room);

	if (victim->base_stats.Con >= 95)
	{
		send_to_char("&+BNothing seem to happen..\n", victim);
		return;
	}

	send_to_char("&+BYou feel your constitition grow..\n", victim);
	victim->base_stats.Con = BOUNDED(1, victim->base_stats.Con + 1, 95);
	victim->curr_stats.Con = BOUNDED(1, victim->curr_stats.Con + 1, 95);
	balance_affects(victim);
}

void spell_perm_increase_luck(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			      P_obj /*obj*/)
{
	send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n",
		     ch->in_room);

	if (victim->base_stats.Luk >= 95)
	{
		send_to_char("&+BNothing seem to happen..\n", victim);
		return;
	}

	send_to_char("&+BYou feel your luck grow..\n", victim);
	victim->base_stats.Luk = BOUNDED(1, victim->base_stats.Luk + 1, 95);
	victim->curr_stats.Luk = BOUNDED(1, victim->curr_stats.Luk + 1, 95);
	balance_affects(victim);
}
void spell_perm_increase_pow(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n",
		     ch->in_room);

	if (victim->base_stats.Pow >= 95)
	{
		send_to_char("&+BNothing seem to happen..\n", victim);
		return;
	}

	send_to_char("&+BYou feel your power grow..\n", victim);
	victim->base_stats.Pow = BOUNDED(1, victim->base_stats.Pow + 1, 95);
	victim->curr_stats.Pow = BOUNDED(1, victim->curr_stats.Pow + 1, 95);
	balance_affects(victim);
}

void spell_perm_increase_int(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n",
		     ch->in_room);

	if (victim->base_stats.Int >= 95)
	{
		send_to_char("&+BNothing seem to happen..\n", victim);
		return;
	}

	send_to_char("&+BYou feel your intelligence grow..\n", victim);
	victim->base_stats.Int = BOUNDED(1, victim->base_stats.Int + 1, 95);
	victim->curr_stats.Int = BOUNDED(1, victim->curr_stats.Int + 1, 95);
	balance_affects(victim);
}
void spell_perm_increase_wis(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n",
		     ch->in_room);

	if (victim->base_stats.Wis >= 95)
	{
		send_to_char("&+BNothing seem to happen..\n", victim);
		return;
	}

	send_to_char("&+BYou feel your wisdom grow..\n", victim);
	victim->base_stats.Wis = BOUNDED(1, victim->base_stats.Wis + 1, 95);
	victim->curr_stats.Wis = BOUNDED(1, victim->curr_stats.Wis + 1, 95);
	balance_affects(victim);
}
void spell_perm_increase_cha(int /*level*/, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
			     P_obj /*obj*/)
{
	send_to_room("&+WThe room lights up as a &+Ymagical&+W light fills the room.&n\n",
		     ch->in_room);

	if (victim->base_stats.Cha >= 95)
	{
		send_to_char("&+BNothing seem to happen..\n", victim);
		return;
	}

	send_to_char("&+BYou feel your charisma grow..\n", victim);
	victim->base_stats.Cha = BOUNDED(1, victim->base_stats.Cha + 1, 95);
	victim->curr_stats.Cha = BOUNDED(1, victim->curr_stats.Cha + 1, 95);
	balance_affects(victim);
}
