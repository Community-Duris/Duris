// Real procedures, act(), send_to_char(), queues and PlayerLog; spell effects
// are controlled boundaries so mystra cannot damage a persistent character.
static P_char activation_actor;
static int activation_spell, activation_calls;
static bool activation_area;

void hummer(P_obj) {}
void cast_as_area(P_char ch, int spell, int level, char *argument)
{
	assert(ch == activation_actor && level == 60 && argument == nullptr);
	activation_spell = spell;
	activation_area = true;
	++activation_calls;
}

#define ACTIVATION_SPELL(name, id)                                                             \
	void name(int level, P_char ch, char *argument, int type, P_char victim, P_obj object) \
	{                                                                                      \
		assert(level == 60 && ch == activation_actor && victim == ch);                 \
		assert(argument == nullptr && type == SPELL_TYPE_SPELL && object == nullptr);  \
		activation_spell = id;                                                         \
		activation_area = false;                                                       \
		++activation_calls;                                                            \
	}
ACTIVATION_SPELL(spell_spirit_armor, SPELL_SPIRIT_ARMOR)
ACTIVATION_SPELL(spell_hawkvision, SPELL_HAWKVISION)
ACTIVATION_SPELL(spell_pantherspeed, SPELL_PANTHERSPEED)
ACTIVATION_SPELL(spell_sense_spirit, SPELL_SENSE_SPIRIT)
ACTIVATION_SPELL(spell_ravenflight, SPELL_RAVENFLIGHT)
ACTIVATION_SPELL(spell_greater_spirit_ward, SPELL_GREATER_SPIRIT_WARD)
ACTIVATION_SPELL(spell_lionrage, SPELL_LIONRAGE)
ACTIVATION_SPELL(spell_elephantstrength, SPELL_ELEPHANTSTRENGTH)
ACTIVATION_SPELL(spell_lightning_bolt, SPELL_LIGHTNING_BOLT)
ACTIVATION_SPELL(spell_mass_heal, SPELL_MASS_HEAL)
#undef ACTIVATION_SPELL

static void artifact_activation_matrix()
{
	char_data actor{}, observer{};
	pc_only_data actor_pc{}, observer_pc{};
	descriptor_data actor_desc{}, observer_desc{};
	PlayerLog actor_log, observer_log;
	group_list group{};
	obj_data item{};
	char actor_name[] = "Invoker", observer_name[] = "Observer";
	// Use the authored Totem description, including its article and colors.
	char totem_name[] = "the &+yT&+go&+yt&+ge&+ym &+Wof &+LM&nas&+Wt&ner&+Wy&n";
	char circlet_name[] = "&+WA circlet of light&n";
	actor.player.name = actor_name;
	observer.player.name = observer_name;
	actor.only.pc = &actor_pc;
	observer.only.pc = &observer_pc;
	actor_pc.log = &actor_log;
	observer_pc.log = &observer_log;
	actor.desc = &actor_desc;
	observer.desc = &observer_desc;
	actor_desc.character = &actor;
	observer_desc.character = &observer;
	actor_desc.connected = observer_desc.connected = CON_PLAYING;
	actor.player.level = observer.player.level = 50;
	actor.specials.position = observer.specials.position = STAT_NORMAL + POS_STANDING;
	actor.next_in_room = &observer;
	world[0].people = &actor;
	visible = true;
	hidden_recipient = nullptr;
	executing_ch = nullptr;
	activation_actor = &actor;
	item.loc_p = LOC_WORN;
	item.loc.wearing = &actor;
	item.short_description = totem_name;
	item.name = totem_name;

	auto invoke = [&](bool circlet, const char *word, int spell, bool grouped)
	{
		actor_log.clear();
		observer_log.clear();
		logged.clear();
		activation_calls = 0;
		actor.group = grouped ? &group : nullptr;
		char argument[32];
		strcpy(argument, word);
		const auto proc = circlet ? circlet_of_light : totem_of_mastery;
		assert(proc(&item, &actor, CMD_SAY, argument));
		assert(activation_calls == 1 && activation_spell == spell);
		assert(activation_area == (grouped && !circlet && spell != SPELL_LIGHTNING_BOLT));
		const std::string own = drain(&actor_desc), seen = drain(&observer_desc);
		assert(own.find("(NULL)") == std::string::npos);
		assert(seen.find("(NULL)") == std::string::npos);
		if (spell != SPELL_MASS_HEAL)
		{
			const std::string name = circlet ? "&+Wcirclet of light&n" : totem_name + 4;
			assert(own.find(name) != std::string::npos);
			assert(seen.find(name) != std::string::npos);
			if (spell != SPELL_LIONRAGE)
				assert(own.starts_with("You whisper '" + std::string(word) +
						       "' to your " + name + "\n\r"));
			assert(seen.find("Invoker whispers '" + std::string(word) + "' to ") !=
			       std::string::npos);
		}
		// A fresh public log contains exactly what each player received, with
		// original item markup retained for downstream combat snapshots.
		assert(std::string(actor_log.read(LOG_PUBLIC, MAX_LOG_LEN)) == own);
		assert(std::string(observer_log.read(LOG_PUBLIC, MAX_LOG_LEN)) == seen);
		assert(std::string(actor_log.read(LOG_PRIVATE, MAX_LOG_LEN)).empty());
		assert(std::string(observer_log.read(LOG_PRIVATE, MAX_LOG_LEN)).empty());
	};
	const std::pair<const char *, int> totem_spells[] = { { "spirit", SPELL_SPIRIT_ARMOR },
							      { "hawk", SPELL_HAWKVISION },
							      { "panther", SPELL_PANTHERSPEED },
							      { "sense", SPELL_SENSE_SPIRIT },
							      { "raven", SPELL_RAVENFLIGHT },
							      { "ward", SPELL_GREATER_SPIRIT_WARD },
							      { "elephant",
								SPELL_ELEPHANTSTRENGTH },
							      { "mystra", SPELL_LIGHTNING_BOLT },
							      { "lion", SPELL_LIONRAGE } };
	for (int slot : { HOLD, WIELD, SECONDARY_WEAPON, THIRD_WEAPON, FOURTH_WEAPON })
	{
		actor.equipment[slot] = &item;
		for (bool grouped : { false, true })
			for (const auto &[word, spell] : totem_spells)
				invoke(false, word, spell, grouped);
		actor.equipment[slot] = nullptr;
	}
	item.name = item.short_description = circlet_name;
	actor.equipment[WEAR_HEAD] = &item;
	for (bool grouped : { false, true })
	{
		invoke(true, "mystra", SPELL_LIGHTNING_BOLT, grouped);
		invoke(true, "beblessed", SPELL_MASS_HEAL, grouped);
	}
	actor.equipment[WEAR_HEAD] = nullptr;

	// Neither artifact activates from an unsupported equipment slot.
	char argument[] = "mystra";
	activation_calls = 0;
	assert(!totem_of_mastery(&item, &actor, CMD_SAY, argument));
	assert(!circlet_of_light(&item, &actor, CMD_SAY, argument));
	assert(!activation_calls && drain(&actor_desc).empty() && drain(&observer_desc).empty());
	world[0].people = nullptr;
	activation_actor = nullptr;
	puts("Artifact activations: 94 solo/group/slot cases, actor/observer output, public logs and spell calls passed");
}
