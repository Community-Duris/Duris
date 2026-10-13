ClassSkillInfo original_SKILL_DATA_ALL(P_char ch, int skill)
{
	ClassSkillInfo dummy;
	int required_level, new_cap;
	int pri_class, sec_class;
	int pri_rlevel, sec_rlevel, pri_cap, sec_cap;
	float pri_mod = get_property("skill.cap.multi.mod.primarySkill", 100.0);
	float sec_mod = get_property("skill.cap.multi.mod.secondarySkill", 95.0);

	pri_class = flag2idx(ch->player.m_class) - 1;
	sec_class = flag2idx(ch->player.secondary_class) - 1;

	// Note: Class 0 is warrior per above.
	if (IS_PC(ch) && (sec_class >= 0))
	{
		// Note: Currently you should not be spec'd and multi'd so ch->player.spec should be 0.
		pri_rlevel = (skills[(skill)].m_class[pri_class]).rlevel[ch->player.spec];
		sec_rlevel = (skills[(skill)].m_class[sec_class]).rlevel[ch->player.spec];
		pri_cap = skills[skill].m_class[pri_class].maxlearn[ch->player.spec];
		sec_cap = skills[skill].m_class[sec_class].maxlearn[ch->player.spec];

		if ((pri_rlevel > 0) && (sec_rlevel < 1))
		{
			new_cap = (int)(pri_cap * pri_mod) / 100;
			required_level = pri_rlevel;
		}
		else if ((sec_rlevel > 0) && (pri_rlevel < 1))
		{
			new_cap = (int)(sec_cap * sec_mod) / 100;
			required_level = sec_rlevel + 5;
		}
		else if ((pri_rlevel > 0) && (sec_rlevel > 0))
		{
			new_cap = (MAX((int)(pri_cap * pri_mod), (int)(sec_cap * sec_mod))) / 100;
			required_level = MIN(pri_rlevel, (sec_rlevel + 5));
		}
		else
		{
			new_cap = 0;
			required_level = 0;
		}

		// Debugging:
		// if( IS_SKILL(skill) ) debug( "Skill %s (%d): pri_rlevel: %d, sec_rlevel: %d, pri_cap: %d, sec_cap %d.\nnew_cap: %d, required_level: %d.",
		//  skills[skill].name, skill, pri_rlevel, sec_rlevel, pri_cap, sec_cap, new_cap, required_level );

		dummy.maxlearn[ch->player.spec] = new_cap;
		dummy.rlevel[ch->player.spec] = required_level;
	}
	else if (IS_MULTICLASS_NPC(ch))
	{
		/* because multiclass NPCs store their multiple classes in m_class, it's not possible
		to simply use SKILL_DATA(), as this just returns the skill data for the class with the
		highest order flag bit. This routine should go through and find the skill with the highest
		value for any of the set classes - Torgal 4/2009 */

		int highest = 0;
		struct ClassSkillInfo *classEntry = skills[skill].m_class;
		struct ClassSkillInfo *highestClassEntry = classEntry;

		for (int i = 0; i < CLASS_COUNT; i++, classEntry++)
		{
			if (ch->player.m_class & (1 << i))
			{
				if (classEntry->maxlearn[0] > highest)
				{
					highestClassEntry = classEntry;
					highest = classEntry->maxlearn[0];
				}
			}
		}

		dummy = *highestClassEntry;
	}
	else if (pri_class >= 0 && pri_class < CLASS_COUNT)
	{
		dummy = SKILL_DATA(ch, skill);
	}
	else
	{
		printf("Invalid m_class %x for %s\n", ch->player.m_class, J_NAME(ch));
		ch->player.m_class = 1; // warrior
		dummy = SKILL_DATA(ch, skill);
	}

	return dummy;
}

int original_GET_LVL_FOR_SKILL(P_char ch, int skill)
{
	// We don't check IS_ALIVE() because a char that just died and lost level would not be alive.
	if (!ch)
	{
		return 0;
	}

	return original_SKILL_DATA_ALL(ch, skill).rlevel[ch->player.spec];
}
