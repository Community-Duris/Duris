#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"

void spell_bigbys_clenched_fist(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
				P_obj /*obj*/)
{
	struct damage_messages messages = {
		"Your giant &+Yfist of force&N causes $N to stagger in agony!",
		"$n's mighty &+Ymagic fist&N slams into you!",
		"$n's &+Yfist&N beats the life out of $N, &+rblood&N pours from $S body!",
		"Your &+Ymighty fist&N smashes into $N's body, killing $M instantly!",
		"$n's giant &+Yfist of force&N is the last thing you ever see.",
		"$n's &+Ymagic fist&N punches $N into so much pulp!",
		0
	};

	if (level > 50)
		level = 50;

	int dam = (10 * level) + number(1, 25);
	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_BIGBYS_CLENCHED_FIST);
	if (!NewSaves(victim, SAVING_SPELL, mod))
		dam = dam * 2;

	spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}

void spell_bigbys_crushing_hand(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
				P_obj /*obj*/)
{
	struct damage_messages messages = {
		"$N is grabbed by your giant fist, which begins &+Ycrushing&n $M.",
		"A huge fist sent by $n grabs you, and begins &+Ycrushing&n your body.",
		"$N is encircled by a huge fist sent by $n, which begins to &+Ycrush&n $S body.",
		"Your fist &+Ycrushes&n $N&n, leaving nothing but a smear of &+rblood&n on the ground.",
		"A huge fist &+Ycrushes&n you completely, leaving nothing but a wet smear.",
		"$N's head &+Yexplodes&n from the pressure of a huge fist sent by $n, and bits of $M &+Gooze&n between its fingers.",
		0
	};

	int dam = (12 * level) + number(1, 50);
	int mod = get_default_save_mod(victim, ch, SAVING_SPELL, SPELL_BIGBYS_CRUSHING_HAND);
	if (!NewSaves(victim, SAVING_SPELL, mod))
		dam = (int)(dam * (number(175, 175 + MAX(0, level - 45) * 2) / 100.0));

	spell_damage(ch, victim, dam, SPLDAM_GENERIC, 0, &messages);
}
