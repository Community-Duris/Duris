#include "core/prototypes.h"
#include "core/utils.h"
#include "core/files.h"
#include "net/output_preference_codec.h"
#include "player/output_preferences.h"
#include "player/player_snapshot_codec.h"
#include "player/player_save_pipeline.h"
#include <cassert>
#include <cstring>
#include <set>
#include <type_traits>

static room_data rooms[1]{};
P_room world = rooms;
static player_save_pipeline_result admission = player_save_pipeline_result::queued;
static int requests = 0;
static P_char requested_owner = nullptr;
static OutputPreferenceState captured{};
int IS_MORPH(P_char)
{
	return false;
}
void panic_corruption(const char *, const char *, ...)
{
	abort();
}
int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}
player_save_pipeline_result
player_save_pipeline_request(P_char owner, player_component_mask_t components, int intent, int room)
{
	assert(components == PLAYER_COMPONENT_STATUS && intent == RENT_CRASH && room == NOWHERE);
	++requests;
	requested_owner = owner;
	captured = owner->only.pc->output_preferences;
	return admission;
}

int main()
{
	static_assert(std::is_trivial_v<OutputPreferenceState>);
	static_assert(std::is_trivially_copyable_v<OutputPreferenceState>);
	OutputPreferenceState defaults{};
	assert(encode_output_preferences(defaults).empty());
	assert(decode_output_preferences("") == defaults);
	assert(decode_output_preferences("v2;m=1;12=27") == defaults);
	assert(decode_output_preferences(std::string(513, 'x')) == defaults);
	std::set<unsigned> palette;
	for (auto color : output_palette_choices())
		palette.insert(GET_FG(color.attr));
	for (unsigned value = 0; value <= 255; ++value)
		assert(valid_output_preference_choice(value) ==
		       (value <= 3 || palette.contains(value)));

	OutputProfilePreferences first, second;
	assert(first.set(OutputChannel::RoomDescription, OutputProfileChoice::Animated));
	assert(first.set_color(OutputChannel::ChatTell, ATTR_FG(27)));
	assert(second.set_color(OutputChannel::ChatTell, ATTR_FG(20)));
	first.motion_enabled = false;
	auto state = first.state();
	assert(encode_output_preferences(state) == "v1;m=1;1=3;12=27");
	assert(decode_output_preferences(encode_output_preferences(state)) == state);
	assert(decode_output_preferences("v1;m=1;1=3;12=27;999=31;18=99") == state);
	auto duplicate = decode_output_preferences("v1;12=27;12=20;1=3");
	assert(!duplicate.choices[12] && duplicate.choices[1] == 3);
	assert(decode_output_preferences("v1;m=1;m=1;12=27").motion_off == false);
	assert(!decode_output_preferences("v1;12=27junk;1=-1;18=&+r").choices[12]);
	assert(!first.set_color(OutputChannel::ChatTell, ATTR_FG(16)));
	assert(!first.set_color(OutputChannel::Count, ATTR_FG(27)));
	assert(first.reset(OutputChannel::ChatTell) && first.color(OutputChannel::ChatTell) == 0);
	assert(!first.motion_enabled &&
	       first.get(OutputChannel::RoomDescription) == OutputProfileChoice::Animated);
	first.reset_all();
	assert(first.state() == defaults && first.motion_enabled);
	first = OutputProfilePreferences::from_state(state);
	assert(first.state() == state);

	// Snapshot framing has explicit new versions, and old 1/3 payloads load with
	// inherited choices. The preference representation is separately versioned.
	player_snapshot snapshot{};
	snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.pid = 41;
	snapshot.revision = 7;
	snapshot.components = PLAYER_COMPONENT_STATUS;
	snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
	snapshot.output_preferences = encode_output_preferences(first.state());
	std::vector<uint8_t> bytes;
	assert(player_snapshot_encode(snapshot, &bytes) == player_snapshot_codec_result::ok);
	player_snapshot decoded{};
	assert(player_snapshot_decode(bytes.data(), bytes.size(), &decoded) ==
	       player_snapshot_codec_result::ok);
	assert(decode_output_preferences(decoded.output_preferences) == first.state());
	bytes.resize(bytes.size() - snapshot.output_preferences.size() - 4);
	for (uint8_t version : { 1, 3 })
	{
		bytes[0] = version;
		assert(player_snapshot_decode(bytes.data(), bytes.size(), &decoded) ==
		       player_snapshot_codec_result::ok);
		assert(decoded.schema_version == PLAYER_SNAPSHOT_SCHEMA_VERSION &&
		       decoded.output_preferences.empty());
	}
	snapshot.output_preferences = std::string(513, 'a');
	assert(player_snapshot_encode(snapshot, &bytes) != player_snapshot_codec_result::ok);

	char_data a{}, b{}, body{};
	pc_only_data a_pc{}, b_pc{};
	npc_only_data npc{};
	descriptor_data connection{};
	a.only.pc = &a_pc;
	b.only.pc = &b_pc;
	body.only.npc = &npc;
	a_pc.pid = 41;
	b_pc.pid = 42;
	a.in_room = b.in_room = NOWHERE;
	SET_BIT(body.specials.act, ACT_ISNPC);
	assert(player_output_preferences(&body).state() == defaults);
	assert(update_player_output_preferences(&body, first) ==
	       OutputPreferenceUpdate::Unavailable);
	assert(update_player_output_preferences(nullptr, first) ==
	       OutputPreferenceUpdate::Unavailable);
	assert(update_player_output_preferences(&a, first) == OutputPreferenceUpdate::PendingSave);
	assert(captured == state && requested_owner == &a);
	assert(update_player_output_preferences(&b, second) == OutputPreferenceUpdate::PendingSave);
	assert(player_output_preferences(&a).state() == first.state());
	assert(player_output_preferences(&b).state() == second.state());
	assert(requests == 2);
	assert(update_player_output_preferences(&a, first) == OutputPreferenceUpdate::Unchanged &&
	       requests == 2);
	for (auto failure :
	     { player_save_pipeline_result::invalid, player_save_pipeline_result::capture_failed,
	       player_save_pipeline_result::overloaded, player_save_pipeline_result::unavailable })
	{
		admission = failure;
		assert(update_player_output_preferences(&a, second) ==
		       OutputPreferenceUpdate::Unavailable);
		assert(player_output_preferences(&a).state() == first.state());
	}
	admission = player_save_pipeline_result::coalesced;
	connection.original = &a;
	connection.character = &body;
	body.desc = &connection;
	assert(player_output_preferences(&body).state() == first.state());
	assert(update_player_output_preferences(&body, second) ==
	       OutputPreferenceUpdate::PendingSave);
	assert(requested_owner == &a && player_output_preferences(&a).state() == second.state());
	// Plain preference state belongs to the character, never a descriptor's phases.
	connection.output_sequences[(size_t)OutputChannel::RoomDescription] = 33;
	descriptor_data reconnect{};
	a.desc = &reconnect;
	assert(player_output_preferences(&a).state() == second.state());
	assert(!reconnect.output_sequences[(size_t)OutputChannel::RoomDescription]);
	auto color = player_output_profile(&a, OutputChannel::ChatTell, OutputPolicy::Static);
	assert(color.context.policy == OutputPolicy::Static &&
	       color.context.base_attr == ATTR_FG(20));
	assert(player_output_profile(&a, OutputChannel::ChatTell, OutputPolicy::Preserve)
		       .context.policy == OutputPolicy::Preserve);
	assert(player_output_profile(&a, OutputChannel::ChatSay, OutputPolicy::Static)
		       .context.policy == OutputPolicy::Preserve);
	puts("Output preferences: versioned round trips, old data, validation, independent owners, reset, save admission and reconnect passed");
}
