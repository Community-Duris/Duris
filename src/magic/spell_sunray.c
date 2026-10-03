#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/graph.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"

void spell_sunray(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim, P_obj /*obj*/)
{
	struct damage_messages messages = {
		"&+WYou unleash &+Ylight&+W in a focused, searing &+Yray&+W at&n $N!",
		"$n&+W unleashes &+Ylight&+W in a focused, searing &+Yray&+W at you!",
		"$n&+W unleashes &+Ylight&+W in a focused, searing &+Yray&+W at&n $N!",
		"$N&+Y is struck by your ray of sunlight, and is destroyed instantly!",
		"&+YYou see a fatally blinding light!",
		"$N&+Y is struck by a blinding light from&n $n, &+Yand is utterly destroyed!",
		0
	};

	if (!IS_ALIVE(ch) || !IS_ALIVE(victim) || ch->in_room != victim->in_room)
		return;

	// A little more than iceball and does less damage when not outside.
	// However, has a chance to blind victim for a while.
	int dam = dice((int)(level * 3), 6) - number(0, 40);

	if (IS_AFFECTED(victim, AFF_BLIND))
		dam = (int)(dam * 0.85);
	else if (!IS_OUTSIDE(victim->in_room))
		dam = (int)(dam * 0.95);

	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_SUNRAY);
	if (!NewSaves(victim, SAVING_SPELL, mod))
	{
		dam = (int)(dam * 1.30);
	}

	if (!NewSaves(victim, SAVING_SPELL, (int)(mod / 3)) && !IS_BLIND(victim))
		blind(ch, victim, number(4, 12) * WAIT_SEC);

	spell_damage(ch, victim, dam, SPLDAM_FIRE, 0, &messages);
}
