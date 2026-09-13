static std::string plain_text(const std::string &text)
{
	char plain[MAX_STRING_LENGTH];
	AnsiString(text.c_str()).plain(plain);
	return plain;
}
static void combat_prompt_matrix()
{
	char_data attacker{}, victim{}, observer{};
	pc_only_data pc{}, victim_pc{}, observer_pc{};
	descriptor_data desc{}, victim_desc{}, observer_desc{};
	attacker.only.pc = &pc;
	victim.only.pc = &victim_pc;
	observer.only.pc = &observer_pc;
	attacker.desc = &desc;
	victim.desc = &victim_desc;
	observer.desc = &observer_desc;
	desc.character = &attacker;
	victim_desc.character = &victim;
	observer_desc.character = &observer;
	attacker.specials.position = victim.specials.position = observer.specials.position =
		POS_STANDING + STAT_NORMAL;
	char actor_name[] = "Alice", victim_name[] = "&+GBob&n", observer_name[] = "Observer";
	attacker.player.name = actor_name;
	victim.player.name = victim_name;
	observer.player.name = observer_name;
	attacker.next_in_room = &victim;
	victim.next_in_room = &observer;
	rooms[0].people = &attacker;
	attacker.player.level = victim.player.level = observer.player.level = 20;
	attacker.points.hit = attacker.points.max_hit = victim.points.hit = victim.points.max_hit =
		100;
	attacker.points.max_mana = attacker.points.max_vitality = 100;
	OutputProfilePreferences a, b, c;
	damage_messages message{ "You strike $N%s.",
				 "$n strikes you%s.",
				 "$n strikes $N%s.",
				 "",
				 "",
				 "",
				 DAMMSG_HIT,
				 nullptr };
	for (double damage : { 0., 5., 40. })
	{
		pc.output_preferences = {};
		victim_pc.output_preferences = {};
		observer_pc.output_preferences = {};
		const int before = combat_random_calls;
		dam_message(damage, &attacker, &victim, &message);
		std::array<std::string, 3> original{ drain(&desc), drain(&victim_desc),
						     drain(&observer_desc) };
		assert(combat_random_calls == before + 1);
		a.set_color(OutputChannel::CombatOutgoing, ATTR_FG(27));
		b.set_color(OutputChannel::CombatIncoming, ATTR_FG(20));
		c.set_color(OutputChannel::CombatObserved, ATTR_FG(22));
		pc.output_preferences = a.state();
		victim_pc.output_preferences = b.state();
		observer_pc.output_preferences = c.state();
		dam_message(damage, &attacker, &victim, &message);
		std::array<std::string, 3> colored{ drain(&desc), drain(&victim_desc),
						    drain(&observer_desc) };
		assert(combat_random_calls == before + 2);
		for (size_t i = 0; i < 3; ++i)
			assert(plain_text(original[i]) == plain_text(colored[i]));
		assert(colored[0] != original[0] && colored[1] != original[1] &&
		       colored[2] != original[2]);
		assert(attacker.points.hit == 100 && victim.points.hit == 100);
		assert(colored[0].find("&+GBob") != std::string::npos);
	}
	// Real prompt resource boundaries retain exact visible numbers. The player's
	// foreground applies only to healthy resources; low warnings remain distinct.
	for (int percentage : { -1, 0, 14, 15, 32, 33, 65, 66, 100 })
	{
		attacker.points.hit = attacker.points.mana = attacker.points.vitality = percentage;
		pc.prompt = PROMPT_HIT | PROMPT_MANA | PROMPT_MOVE;
		pc.output_preferences = {};
		desc.prompt_mode = true;
		make_prompt(&desc);
		auto baseline = drain(&desc);
		a.reset_all();
		a.set_color(OutputChannel::Prompt, ATTR_FG(27));
		pc.output_preferences = a.state();
		desc.prompt_mode = true;
		make_prompt(&desc);
		auto styled = drain(&desc);
		assert(plain_text(styled) == plain_text(baseline));
		AnsiString shown(styled.c_str());
		auto hit = plain_text(styled).find(std::to_string(percentage) + "h");
		assert(shown.attr(hit) == (percentage >= 66 ? ATTR_FG(27) :
					   percentage >= 33 ? ATTR_FG(22) :
					   percentage >= 15 ? ATTR_FG(20) :
							      ATTR_FG(28)));
		assert(!desc.prompt_mode);
	}
	// The normal prompt does not replace paging/editor/confirmation prompts.
	desc.showstr_count = 2;
	desc.showstr_page = 1;
	desc.prompt_mode = true;
	make_prompt(&desc);
	assert(drain(&desc).find("Return to continue") != std::string::npos);
	desc.showstr_count = 0;
	char *edit = nullptr;
	desc.str = &edit;
	desc.prompt_mode = true;
	make_prompt(&desc);
	assert(drain(&desc) == "] ");
	desc.str = nullptr;
	desc.confirm_state = CONFIRM_AWAIT;
	desc.prompt_mode = true;
	make_prompt(&desc);
	assert(drain(&desc).empty());
	desc.confirm_state = 0;
	// Validate all semantic role fields, hard Preserve and the shared thresholds.
	OutputProfileRegistry registry;
	assert(registry.reload_json(
			       R"({"version":1,"revision":1,"recipes":{},"dictionaries":{},"profiles":{"roles":{"policy":"animated","roles":{"healthy":"cyan","caution":"yellow","low":"red","critical":"bright_red","success":"green","failure":"red","hit":"bright_cyan","miss":"gray"}}},"channels":{"prompt":"roles","combat.outgoing":"roles","system.feedback":"roles"}})")
		       .ok);
	auto configured = resolve_output_profile(registry.snapshot(), OutputChannel::Prompt,
						 OutputPolicy::Static, {});
	assert(configured.context.policy == OutputPolicy::Static && !configured.context.recipes &&
	       !configured.context.words);
	for (auto role :
	     { OutputRole::Healthy, OutputRole::Caution, OutputRole::Low, OutputRole::Critical,
	       OutputRole::Success, OutputRole::Failure, OutputRole::Hit, OutputRole::Miss })
		assert(output_role_attribute(configured, role));
	assert(prompt_resource_role(66) == OutputRole::Healthy &&
	       prompt_resource_role(65) == OutputRole::Caution &&
	       prompt_resource_role(33) == OutputRole::Caution &&
	       prompt_resource_role(32) == OutputRole::Low &&
	       prompt_resource_role(15) == OutputRole::Low &&
	       prompt_resource_role(14) == OutputRole::Critical);
	configured.context.policy = OutputPolicy::Preserve;
	assert(!output_role_attribute(configured, OutputRole::Success));

	const char *role_config =
		R"({"version":1,"revision":2,"recipes":{},"dictionaries":{},"profiles":{"roles":{"policy":"static","roles":{"critical":"bright_magenta","success":"green","failure":"red","hit":"bright_cyan","miss":"gray"}}},"channels":{"prompt":"roles","combat.outgoing":"roles","system.feedback":"roles"}})";
	assert(output_profile_registry().reload_json(role_config).ok);
	a.reset_all();
	pc.output_preferences = a.state();
	for (double damage : { 0., 5. })
	{
		dam_message(damage, &attacker, &victim, &message);
		auto text = drain(&desc);
		auto offset = plain_text(text).find("strike");
		assert(AnsiString(text.c_str()).attr(offset) ==
		       (damage > 0 ? ATTR_FG(27) : ATTR_FG(24)));
		drain(&victim_desc);
		drain(&observer_desc);
	}
	attacker.points.hit = 1;
	pc.prompt = PROMPT_HIT;
	desc.prompt_mode = true;
	make_prompt(&desc);
	auto low = drain(&desc);
	assert(AnsiString(low.c_str()).attr(plain_text(low).find("1h")) == ATTR_FG(29));
	for (auto role : { OutputRole::Success, OutputRole::Failure })
	{
		PlayerOutputMessage(&attacker, OutputChannel::SystemFeedback, role)
			.literal("Save ")
			.entity(actor_name)
			.literal(".\r\n")
			.send(LOG_PUBLIC);
		AnsiString feedback(drain(&desc).c_str());
		assert(feedback.attr(0) ==
		       (role == OutputRole::Success ? ATTR_FG(18) : ATTR_FG(20)));
		assert(feedback.attr(5) == 0); // the entity name is protected
	}
	pc.output_preferences = {};
	rooms[0].people = nullptr;
	puts("Combat and prompt: real damage recipients, unchanged RNG/text/state, resource thresholds and auxiliary prompts passed");
}
