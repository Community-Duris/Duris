#include "core/prototypes.h"
#include "cmd/interp.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utility.h"
#include "core/utils.h"
#include "core/defines.h"
#include "combat/damage.h"
#include "magic/spells.h"

void spell_dispel_good(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"A &+rred aura&N surrounds you as you shoot out a &+Lblack bolt of energy&N from your hands to destroy $N.",
		"A bolt of evil energy sent by $n engulfs you, dissolving your very soul!",
		"A &+rred aura&N surrounds $n as a &+Lblack bolt of energy&N shoots out from $s hands to dispel $N.",
		"You cackle in triumph as the essence of $N is utterly destroyed by your evil power!",
		"You scream in horror as your soul is purged from existance by the evil forces of $n!",
		"$N screams in terror as $S soul is ripped to shreds by the vile power of $n's spell!"
	};

	/*  if(IS_GOOD(ch))
	      victim = ch; */
	// Removed because of bard song
	if (!IS_GOOD(victim))
	{
		act("$N basks in your ignorance.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n is clueless.", FALSE, ch, 0, victim, TO_VICT);
		act("$N chuckles at $n's cluelessness.", FALSE, ch, 0, victim, TO_NOTVICT);
		return;
	}
	dam = dice(level, 8);

	if (saves_spell(victim, SAVING_SPELL))
		dam >>= 1;

	spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_ALLGLOBES, &messages);
}

void spell_dispel_evil(int level, P_char ch, char * /*arg*/, int /*type*/, P_char victim,
		       P_obj /*obj*/)
{
	int dam;
	struct damage_messages messages = {
		"$N shivers, and suffers from $S evilness!",
		"$n makes your soul hurt and suffer!",
		"$n makes $N's evil spirit shiver and suffer!",
		"$N is dissolved by your goodness.",
		"$n dissolves you, you regret having been so evil, and die...",
		"$n completely dissolves $N."
	};

	/*  if(IS_EVIL(ch))
	      victim = ch; */
	// Removed because of bard song
	if (!IS_EVIL(victim))
	{
		act("$N chuckles at your foolishness.", FALSE, ch, 0, victim, TO_CHAR);
		act("$n is clueless.", FALSE, ch, 0, victim, TO_VICT);
		act("$N chuckles at $n's cluelessness.", FALSE, ch, 0, victim, TO_NOTVICT);
		return;
	}
	dam = dice((level + 1), 5);

	if (saves_spell(victim, SAVING_SPELL))
		dam >>= 1;

	spell_damage(ch, victim, dam, SPLDAM_HOLY, SPLDAM_ALLGLOBES, &messages);
}
