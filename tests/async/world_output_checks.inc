static void world_output_matrix()
{
	char_data ch{};
	pc_only_data pc{};
	descriptor_data connection{};
	ch.only.pc = &pc;
	ch.desc = &connection;
	connection.character = &ch;
	ch.specials.position = POS_STANDING + STAT_NORMAL;
	ch.player.level = 20;
	ch.in_room = 0;
	OutputProfilePreferences preferences;
	// Every adopted world channel retains exact original bytes by default,
	// adds only unstyled text on explicit override, and restores independently.
	for (auto channel :
	     { OutputChannel::RoomTitle, OutputChannel::RoomInspect, OutputChannel::RoomExits,
	       OutputChannel::RoomAuras, OutputChannel::RoomOccupants, OutputChannel::RoomItems,
	       OutputChannel::ItemsList, OutputChannel::Social, OutputChannel::Weather })
	{
		const char *text = "forest &+rc&+Yh&+Ge&+Bs&+Mt&n\n";
		auto context = recipient_output_context(channel);
		send_authored_output(text, &ch, context);
		assert(drain(&connection) == text);
		preferences.set_color(channel, ATTR_FG(27));
		pc.output_preferences = preferences.state();
		send_authored_output(text, &ch, context);
		AnsiString decorated(drain(&connection).c_str()), original(text);
		assert(decorated.attr(0) == ATTR_FG(27));
		for (size_t i = 7; i < 12; ++i)
			assert(decorated[i] == original[i]);
		for (const char *art :
		     { "+---+\n| @ |\n+---+\n", "forest   stream\n", "forest\tstream\n",
		       "\xe2\x94\x8c\xe2\x94\x80\xe2\x94\x90\n" })
		{
			send_authored_output(art, &ch, context);
			assert(drain(&connection) == art);
		}
		context.policy = OutputPolicy::Preserve;
		send_authored_output(text, &ch, context);
		assert(drain(&connection) == text);
		preferences.reset(channel);
		pc.output_preferences = preferences.state();
		send_authored_output(text, &ch, recipient_output_context(channel));
		assert(drain(&connection) == text);
	}

	OutputProfileRegistry semantic_registry;
	assert(semantic_registry
		       .reload_json(
			       R"({"version":1,"revision":1,"recipes":{"glow":{"kind":"pulse","palette":["red","blue"]}},"dictionaries":{"wrong":{"chest":"glow"}},"profiles":{"dense":{"policy":"animated","dictionary":"wrong","base":"cyan"}},"channels":{"room.title":"dense","room.exits":"dense","room.auras":"dense","room.occupants":"dense","room.items":"dense","items.list":"dense"}})")
		       .ok);
	for (auto channel :
	     { OutputChannel::RoomTitle, OutputChannel::RoomExits, OutputChannel::RoomAuras,
	       OutputChannel::RoomOccupants, OutputChannel::RoomItems, OutputChannel::ItemsList })
	{
		auto profile = resolve_output_profile(semantic_registry.snapshot(), channel,
						      OutputPolicy::Static, {});
		assert(profile.context.policy == OutputPolicy::Static && !profile.context.words &&
		       !profile.context.recipes);
	}
	room_direction_data north{}, east{};
	north.to_room = east.to_room = 1;
	north.exit_info = EX_CLOSED;
	east.exit_info = EX_SECRET;
	rooms[0].dir_option[0] = &north;
	rooms[0].dir_option[1] = &east;
	char name[] = "Secret chamber";
	rooms[1].name = name;
	show_exits_to_char(&ch, 0, 1);
	auto baseline = drain(&connection);
	assert(baseline.find("North") != std::string::npos &&
	       baseline.find("East") == std::string::npos);
	assert(baseline.find("&+g#&n") != std::string::npos);
	preferences.set_color(OutputChannel::RoomExits, ATTR_FG(27));
	pc.output_preferences = preferences.state();
	show_exits_to_char(&ch, 0, 1);
	auto colored = drain(&connection);
	AnsiString exits(colored.c_str()), normal(baseline.c_str());
	assert(exits.attr(0) == ATTR_FG(27) && exits.size() == normal.size());
	for (size_t i = 0; i < normal.size(); ++i)
	{
		assert(exits.ch(i) == normal.ch(i));
		if (normal.ch(i) == '#')
			assert(exits.attr(i) == ATTR_FG(18));
	}
	world_visibility = 5;
	show_exits_to_char(&ch, 0, 1);
	assert(drain(&connection).empty());
	world_visibility = 2;
	preferences.reset_all();
	pc.output_preferences = preferences.state();
	// Actual item metadata is revealed only through the existing perception gate.
	obj_data ring{};
	char ring_name[] = "a &+Gsilver&n ring";
	ring.short_description = ring_name;
	ring.extra2_flags = ITEM2_MAGIC;
	auto inventory = recipient_output_context(OutputChannel::ItemsList);
	auto roomitems = recipient_output_context(OutputChannel::RoomItems);
	preferences.set_color(OutputChannel::ItemsList, ATTR_FG(27));
	preferences.set_color(OutputChannel::RoomItems, ATTR_FG(20));
	pc.output_preferences = preferences.state();
	visible = true;
	list_obj_to_char(&ring, &ch, LISTOBJ_SHORTDESC | LISTOBJ_STATS, true, inventory);
	auto ordinary = drain(&connection);
	assert(ordinary.find("magic") == std::string::npos);
	AnsiString carried(ordinary.c_str());
	assert(carried.attr(0) == ATTR_FG(27));
	SET_BIT(ch.specials.affected_by2, AFF2_DETECT_MAGIC);
	list_obj_to_char(&ring, &ch, LISTOBJ_SHORTDESC | LISTOBJ_STATS, true, roomitems);
	auto detected = drain(&connection);
	assert(detected.find("magic") != std::string::npos);
	AnsiString floor(detected.c_str());
	assert(floor.attr(0) == ATTR_FG(20));
	assert(floor.attr(2) == ATTR_FG(26)); // authored name stays green
	visible = false;
	list_obj_to_char(&ring, &ch, LISTOBJ_SHORTDESC | LISTOBJ_STATS, false, inventory);
	assert(drain(&connection).empty());
	visible = true;
	list_obj_to_char(&ring, &ch, LISTOBJ_SHORTDESC | LISTOBJ_STATS, true);
	assert(drain(&connection).starts_with("a &+Gsilver&n ring"));
	// Aura eligibility and authored semantic accent are independent of base color.
	ch.specials.affected_by2 = 0;
	rooms[0].room_flags = ROOM_HEAL;
	preferences.set_color(OutputChannel::RoomAuras, ATTR_FG(27));
	pc.output_preferences = preferences.state();
	display_room_auras(&ch, 0);
	assert(drain(&connection).empty());
	SET_BIT(ch.specials.affected_by2, AFF2_DETECT_GOOD);
	display_room_auras(&ch, 0);
	AnsiString aura(drain(&connection).c_str());
	assert(aura.attr(0) == ATTR_FG(27) && aura.attr(2) == ATTR_FG(25));
	// Weather checks the real outdoors, blindness and sleeping conditions.
	descriptor_list = &connection;
	rooms[0].room_flags = 0;
	preferences.set_color(OutputChannel::Weather, ATTR_FG(19));
	pc.output_preferences = preferences.state();
	send_to_weather_sector(7, "Rain falls.\n");
	AnsiString rain(drain(&connection).c_str());
	assert(rain.attr(0) == ATTR_FG(19));
	rooms[0].room_flags = ROOM_INDOORS;
	send_to_weather_sector(7, "Rain falls.\n");
	assert(drain(&connection).empty());
	rooms[0].room_flags = 0;
	SET_BIT(ch.specials.affected_by, AFF_BLIND);
	send_to_weather_sector(7, "Rain falls.\n");
	assert(drain(&connection).empty());
	descriptor_list = nullptr;
	rooms[0].dir_option[0] = rooms[0].dir_option[1] = nullptr;
	puts("World output: profile matrix, layout, exits, perception-gated item flags, auras and weather passed");
}
