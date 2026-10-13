void original_update_skills(P_char ch)
{
	int skl, spec, skllvl, minlearn;
	bool chaos_mode;

	spec = ch->player.spec;
	minlearn = MIN(40, ((3 * GET_LEVEL(ch)) / 2));

	if (!ch || !IS_PC(ch))
		return;
	chaos_mode = chaos_mud_enabled();

	for (skl = FIRST_SKILL; skl <= LAST_SKILL; skl++)
	{
		skllvl = GET_LVL_FOR_SKILL(ch, skl);
		if (IS_EPIC_SKILL(skl))
		{
			/* Curated Chaos starter skills are granted in enter_game; keep the
			 * legacy blanket update path disabled while that feature is enabled. */
			if (chaos_mode && !chaos_starter_epic_skills_enabled())
			{
				ch->only.pc->skills[skl].taught = 100;
				ch->only.pc->skills[skl].learned = 100;
			}
		}
		// If they get th skill and they're high enough level for it.
		else if ((skllvl > 0) && (GET_LEVEL(ch) >= skllvl))
		{
			/* Why is there 3 possibilitie for .taught?  Either you lose a lvl or spec out of the skill and it goes down,
			 *   or you gain a level or spec to a higher skill max and it goes up.  There should be no MAX( ... ).
			      ch->only.pc->skills[s].taught = MAX(ch->only.pc->skills[s].taught, MAX(SKILL_DATA_ALL(ch, s).maxlearn[0],
			        SKILL_DATA_ALL(ch, s).maxlearn[ch->player.spec]));
			 */
			// debug( "Taught: %d, maxlearn: %d", ch->only.pc->skills[skl].taught, skills[skl].m_class[cls].maxlearn[spec] );
			ch->only.pc->skills[skl].taught = SKILL_DATA_ALL(ch, skl).maxlearn[spec];

			/* Dunno if we need any of this when not debugging.
			      lastlvl = ch->only.pc->skills[s].learned;
			      shouldbe = (GET_LEVEL(ch) * 3 / 2);
			      notched = lastlvl-shouldbe;
			      debug("should be: %d, learned so far: %d, notched above: %d", shouldbe, lastlvl, notched);
			*/

			if (chaos_mode)
			{
				ch->only.pc->skills[skl].learned = 100;
				ch->only.pc->skills[skl].taught = 100;
			}
			else
			{
				if (ch->only.pc->skills[skl].taught < minlearn)
					ch->only.pc->skills[skl].taught = minlearn;
				if (ch->only.pc->skills[skl].learned < minlearn)
					ch->only.pc->skills[skl].learned = minlearn;

				// debug("new learned: %d", ch->only.pc->skills[s].learned);
			}
		}
		// If they never get the skill, or aren't high enough level for it.
		else
		{
			ch->only.pc->skills[skl].taught = ch->only.pc->skills[skl].learned = 0;
		}
		if (!IS_EPIC_SKILL(skl) &&
		    (ch->only.pc->skills[skl].taught < ch->only.pc->skills[skl].learned))
		{
			if (chaos_mode)
				ch->only.pc->skills[skl].taught = ch->only.pc->skills[skl].learned =
					100;
			else
				ch->only.pc->skills[skl].learned = ch->only.pc->skills[skl].taught;
		}
	}

	// assumes that the instruments and songs are back to back in spells.h
	for (skl = FIRST_INSTRUMENT; skl <= LAST_SONG; skl++)
	{
		if (GET_LVL_FOR_SKILL(ch, skl) > 0 && GET_LEVEL(ch) >= GET_LVL_FOR_SKILL(ch, skl))
		{
			if (chaos_mode)
				ch->only.pc->skills[skl].taught = ch->only.pc->skills[skl].learned =
					100;
			else
			{
				ch->only.pc->skills[skl].taught =
					MAX(ch->only.pc->skills[skl].taught,
					    MAX(SKILL_DATA_ALL(ch, skl).maxlearn[0],
						SKILL_DATA_ALL(ch, skl).maxlearn[ch->player.spec]));
				ch->only.pc->skills[skl].learned =
					MAX(MIN(40, GET_LEVEL(ch) * 3 / 2),
					    ch->only.pc->skills[skl].learned);
			}
		}
		else
		{
			ch->only.pc->skills[skl].taught = ch->only.pc->skills[skl].learned = 0;
		}
		if (SKILL_DATA_ALL(ch, skl).maxlearn[0] < ch->only.pc->skills[skl].taught &&
		    SKILL_DATA_ALL(ch, skl).maxlearn[ch->player.spec] <
			    ch->only.pc->skills[skl].taught)
		{
			if (chaos_mode)
				ch->only.pc->skills[skl].taught = ch->only.pc->skills[skl].learned =
					100;
			else
				ch->only.pc->skills[skl].taught = MAX(
					0, MAX(SKILL_DATA_ALL(ch, skl).maxlearn[0],
					       SKILL_DATA_ALL(ch, skl).maxlearn[ch->player.spec]));
		}
	}
}
