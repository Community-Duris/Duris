static void chat_presentation_matrix()
{
	char_data alice{}, bob{};
	pc_only_data a{}, b{};
	descriptor_data da{}, db{};
	Guild guild;
	a.pid = 100;
	b.pid = 101;
	char an[] = "Alice", bn[] = "Bob";
	alice.only.pc = &a;
	bob.only.pc = &b;
	alice.desc = &da;
	bob.desc = &db;
	alice.player.name = an;
	bob.player.name = bn;
	alice.player.level = bob.player.level = 30;
	alice.specials.position = bob.specials.position = POS_STANDING + STAT_NORMAL;
	alice.specials.guild = bob.specials.guild = &guild;
	alice.specials.guild_status = bob.specials.guild_status = A_SF2 | A_NORMAL;
	SET_BIT(alice.specials.act, PLR_ECHO | PLR_GCC);
	SET_BIT(bob.specials.act, PLR_ECHO | PLR_GCC);
	da.character = &alice;
	db.character = &bob;
	da.next = &db;
	da.connected = db.connected = CON_PLAYING;
	da.gmcp_enabled = db.gmcp_enabled = true;
	descriptor_list = &da;
	world[0].people = &alice;
	alice.next_in_room = &bob;
	const char *permitted = "translated &+gforest&n && <tag> 🌲";
	language_override = permitted;
	cJSON *fixtures = cJSON_CreateArray();
	auto verify = [&](const char *label, OutputChannel channel, const char *legacy,
			  int color_id, bool reset = false)
	{
		assert(chat_packets.size() == 2);
		for (auto &[d, json] : chat_packets)
		{
			auto terminal = drain(d);
			cJSON *packet = cJSON_Parse(json.c_str());
			assert(packet);
			auto field = [&](const char *name)
			{ return cJSON_GetObjectItem(packet, name); };
			assert(std::string(cJSON_GetStringValue(field("channel"))) == legacy);
			assert(cJSON_IsNumber(field("timestamp")));
			const std::string body = cJSON_GetStringValue(field("text"));
			if (d == &db)
				assert(body == permitted &&
				       body.find("SECRET") == std::string::npos);
			cJSON *p = field("presentation");
			assert(p && cJSON_GetObjectItem(p, "version")->valueint == 1);
			assert(cJSON_GetObjectItem(p, "channelId")->valueint == (int)channel);
			assert(std::string(cJSON_GetStringValue(cJSON_GetObjectItem(
				       p, "policy"))) == (reset ? "preserve" : "static"));
			AnsiString rendered(terminal.c_str());
			while (!rendered.empty() && (rendered.ch(rendered.size() - 1) == '\r' ||
						     rendered.ch(rendered.size() - 1) == '\n'))
				rendered.pop_back();
			char plain[MAX_STRING_LENGTH];
			rendered.plain(plain);
			assert(std::string(cJSON_GetStringValue(cJSON_GetObjectItem(p, "text"))) ==
			       plain);
			size_t position = 0;
			cJSON *run = nullptr;
			bool found_base = false, found_authored = false;
			cJSON_ArrayForEach(run, cJSON_GetObjectItem(p, "runs"))
			{
				assert(cJSON_GetArraySize(run) == 4);
				const int begin = cJSON_GetArrayItem(run, 0)->valueint;
				const int end = cJSON_GetArrayItem(run, 1)->valueint;
				const int fg = cJSON_GetArrayItem(run, 2)->valueint;
				const int bg = cJSON_GetArrayItem(run, 3)->valueint;
				assert((size_t)begin == position && end > begin);
				for (int i = begin; i < end; ++i)
					assert(GET_FG(rendered.attr(i)) == fg &&
					       GET_BG(rendered.attr(i)) == bg);
				position = end;
				found_base |= fg == (d == &da ? 27 : color_id);
				found_authored |= fg == 18;
			}
			assert(position == rendered.size() && found_authored);
			if (!reset)
				assert(found_base);
			cJSON_DeleteItemFromObject(packet, "timestamp");
			auto fixture = cJSON_CreateObject();
			cJSON_AddStringToObject(fixture, "case", label);
			cJSON_AddStringToObject(fixture, "recipient", d == &da ? "Alice" : "Bob");
			cJSON_AddStringToObject(fixture, "terminalMarkup", terminal.c_str());
			cJSON_AddItemToObject(fixture, "packet", packet);
			cJSON_AddItemToArray(fixtures, fixture);
		}
		chat_packets.clear();
	};
	for (auto [channel, legacy] : { std::pair{ OutputChannel::ChatSay, "say" },
					{ OutputChannel::ChatTell, "tell" },
					{ OutputChannel::ChatGuild, "gcc" } })
	{
		OutputProfilePreferences pa, pb;
		pa.set_color(channel, ATTR_FG(27));
		pb.set_color(channel, ATTR_FG(20));
		assert(update_player_output_preferences(&alice, pa) ==
		       OutputPreferenceUpdate::PendingSave);
		assert(update_player_output_preferences(&bob, pb) ==
		       OutputPreferenceUpdate::PendingSave);
		auto send = [&]
		{
			char original[] = "SECRET &+gforest&n && <tag> 🌲";
			if (channel == OutputChannel::ChatSay)
				say(&alice, original);
			else if (channel == OutputChannel::ChatGuild)
				do_gcc(&alice, original, 0);
			else
			{
				char tell[] = "Bob SECRET &+gforest&n && <tag> 🌲";
				do_tell(&alice, tell, 0);
			}
		};
		const auto before = language_calls;
		send();
		verify("selected", channel, legacy, 20);
		assert(language_calls == before + 1);
		// A new descriptor keeps the character's persisted preference state.
		db = {};
		db.character = &bob;
		db.connected = CON_PLAYING;
		db.gmcp_enabled = true;
		send();
		verify("reconnect", channel, legacy, 20);
		b.ignored = &alice;
		send();
		assert(std::none_of(chat_packets.begin(), chat_packets.end(),
				    [&](auto &v) { return v.first == &db; }));
		assert(drain(&db).empty());
		drain(&da);
		chat_packets.clear();
		b.ignored = nullptr;
		REMOVE_BIT(alice.specials.act, PLR_ECHO);
		send();
		assert(chat_packets.size() == 1 && chat_packets[0].first == &db);
		drain(&da);
		drain(&db);
		chat_packets.clear();
		SET_BIT(alice.specials.act, PLR_ECHO);
		if (channel == OutputChannel::ChatSay)
		{
			SET_BIT(bob.specials.affected_by4, AFF4_DEAF);
			send();
			assert(chat_packets.size() == 1 && chat_packets[0].first == &da);
			assert(drain(&db).empty());
			drain(&da);
			chat_packets.clear();
			REMOVE_BIT(bob.specials.affected_by4, AFF4_DEAF);
		}
		pa.reset_all();
		pb.reset_all();
		assert(update_player_output_preferences(&alice, pa) ==
		       OutputPreferenceUpdate::PendingSave);
		assert(update_player_output_preferences(&bob, pb) ==
		       OutputPreferenceUpdate::PendingSave);
		send();
		verify("reset", channel, legacy, 0, true);
	}

	// A known template containing literal ampersands must keep its visible text.
	OutputProfilePreferences literal_pref;
	literal_pref.set_color(OutputChannel::ChatTell, ATTR_FG(27));
	update_player_output_preferences(&alice, literal_pref);
	PlayerOutputMessage(&alice, OutputChannel::ChatTell)
		.literal("Fish & chips && sauce")
		.send(LOG_PRIVATE);
	char literal_plain[MAX_STRING_LENGTH];
	AnsiString(drain(&da).c_str()).plain(literal_plain);
	assert(std::string(literal_plain) == "Fish & chips && sauce");

	OutputStyleRecipe recipe;
	recipe.kind = OutputRecipeKind::Flow;
	recipe.palette[0] = ATTR_FG(17);
	recipe.palette[1] = ATTR_FG(19);
	recipe.palette_size = 2;
	WordColorDictionary words{ { "water", ATTR_FG(17) } };
	WordRecipeDictionary recipes{ { "water", &recipe } };
	OutputChatMessage chat{ "say", "Alice", "water" };
	OutputContext animated{ OutputChannel::ChatSay, OutputPolicy::Animated, &words };
	animated.recipes = &recipes;
	animated.chat = &chat;
	da.output_sequences[(size_t)OutputChannel::ChatSay] = 0;
	send_to_char("water", &alice, animated);
	assert(chat_packets.size() == 1 &&
	       da.output_sequences[(size_t)OutputChannel::ChatSay] == 1);
	drain(&da);
	chat_packets.clear();
	// Supplemental serialization never consumes a second animation phase.
	gmcp_comm_channel_output(&alice, chat, animated, "&+bwater");
	assert(da.output_sequences[(size_t)OutputChannel::ChatSay] == 1);
	chat_packets.clear();
	for (const auto &oversize : { std::string(16385, 'x'), std::string("\x1b[31mraw") })
	{
		gmcp_comm_channel_output(&alice, chat, animated, oversize.c_str());
		assert(chat_packets.size() == 1);
		cJSON *packet = cJSON_Parse(chat_packets[0].second.c_str());
		assert(packet && !cJSON_GetObjectItem(packet, "presentation"));
		assert(std::string(cJSON_GetStringValue(cJSON_GetObjectItem(packet, "text"))) ==
		       "water");
		cJSON_Delete(packet);
		chat_packets.clear();
	}
	da.gmcp_enabled = false;
	gmcp_comm_channel_output(&alice, chat, animated, "water");
	assert(chat_packets.empty());
	da.gmcp_enabled = true;
	SET_BIT(alice.specials.act3, PLR3_NOGMCP);
	gmcp_comm_channel_output(&alice, chat, animated, "water");
	assert(chat_packets.empty());
	REMOVE_BIT(alice.specials.act3, PLR3_NOGMCP);
	language_override = nullptr;
	descriptor_list = nullptr;
	world[0].people = nullptr;
	if (const char *file = std::getenv("CHAT_PRESENTATION_FIXTURE"))
	{
		char *json = cJSON_Print(fixtures);
		std::ofstream(file) << json << '\n';
		std::free(json);
	}
	cJSON_Delete(fixtures);
	puts("Structured chat: real say/tell/guild, transformed privacy, authored runs, reset, connection state and no duplicate deliveries passed");
}
