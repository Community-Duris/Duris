#!/usr/bin/env python3
"""Whole-TU auction world-observer acceptance; no SQL/publication qualification.

Run on Linux/WSL with C++20, OpenSSL, ASan and UBSan available. --source-root
allows an exact maintained candidate to be qualified without altering a checkout.
BIN_ROOT (or --build-dir) controls retained outputs; TMPDIR controls compiler temp.
Each invocation requires a fresh build directory and retains commands, dependency
pins, the generated fixture, objects, link map, ELF and per-case transcripts.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "src/economy/auction_native_publication.c",
    "src/player/player_snapshot_capture.c",
    "src/player/player_snapshot_codec.c",
    "src/item/item_ownership_runtime.c",
    "src/item/item_transfer_command.c",
    "src/account/character_identity.c",
    "src/player/inert_item_stage.c",
    "src/core/mm.c",
    "src/core/memory.c",
    "src/core/utility.c",
    "src/mob/studioproclib.c",
)

# Only synthetic graphs/globals, thread detection and fail-stop diagnostics are
# fixture support. Every observer, capture, codec, census and ownership provider
# comes from a complete production translation unit below, never text extraction.
HARNESS = r'''
#include "economy/auction_native_publication.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "item/item_ownership_runtime.h"
#include "core/utils.h"
#define DURIS_CHARACTER_IDENTITY_TEST_PANIC_STUB
#include "tests/async/character_identity_test_fixture.h"
#include <algorithm>
#include <cstdio>
#include <deque>
#include <string>

// Synthetic input graph and diagnostic/thread scaffolding, never SQL authority.
P_obj object_list = nullptr;
P_char character_list = nullptr;
P_desc descriptor_list = nullptr;
room_data rooms[2]{};
P_room world = rooms;
extern const int top_of_world;
const int top_of_world = 1;
index_data indexes[3]{};
P_index obj_index = indexes;
int top_of_objt = 2;
bool nevent_is_game_thread()
{
	return std::this_thread::get_id() == fixture_game_thread;
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
void fatal_boot_error(const char *, const char *, ...)
{
	std::abort();
}

using forest = std::vector<player_item_snapshot>;
using location = auction_native_world_location;
static std::vector<uint8_t> bytes(const forest &items)
{
	std::vector<uint8_t> value;
	assert(player_item_snapshot_list_encode(items, &value) == player_snapshot_codec_result::ok);
	return value;
}
static void equal(const forest &actual, const forest &expected)
{
	assert(bytes(actual) == bytes(expected));
}
// Independent, manually specified rows. Never read a live object/capture result.
static player_item_snapshot row(uint64_t uid, int vnum, int parent = -1, int slot = 0,
				bool literal = false, uint32_t flags = 0)
{
	player_item_snapshot r{};
	r.object_uid = uid;
	r.vnum = vnum;
	r.parent_index = parent;
	r.equipment_slot = slot;
	r.generated_key = 17;
	r.type = ITEM_CONTAINER;
	r.string_mask = literal ? 15 : 0;
	r.wear_flags = 2;
	r.extra_flags = flags;
	r.anti_flags = 3;
	r.anti2_flags = 4;
	r.extra2_flags = 5;
	r.weight = 13;
	r.material = 1;
	r.cost = 97;
	r.condition = 21;
	r.craftsmanship = 3;
	r.bitvectors = { 1, 2, 3, 4, 5 };
	r.values = { 101, 102, 103, 104, 105, 106, 107, 108 };
	r.timers = { 201, 202, 203, 204, 205, 206 };
	r.affects = { { { 1, 31 }, { 2, 32 }, { 3, 33 }, { 4, 34 } } };
	if (literal)
	{
		r.name = "literal keys";
		r.short_description = "literal short";
		r.description = "literal room\r\n";
		r.action_description = "literal action";
	}
	if (uid == 200)
		r.extra_descriptions.push_back({ "inscription", "original detail", false, {} });
	if (uid == 201)
		r.dynamic_affects.push_back({ 5, 77, 8 });
	return r;
}
struct fixture
{
	pc_only_data pc{}, other_pc{};
	char_data actor{}, other{};
	descriptor_data descriptor{};
	obj_affect affect{};
	extra_descr_data detail{};
	std::deque<obj_data> objects;
	P_obj hat, bag, child, spare;
	forest expected;
	std::vector<auction_native_world_uid> selected{ { 200, location::carried } };
	fixture()
	{
		item_ownership_runtime_reset();
		rooms[0] = {};
		rooms[1] = {};
		for (int i = 0; i < 3; ++i)
		{
			indexes[i] = {};
			indexes[i].virtual_number = 7000 + i;
		}
		actor.only.pc = &pc;
		pc.pid = 42;
		actor.player.level = 56;
		other.only.pc = &other_pc;
		other_pc.pid = 43;
		fixture_register_character(&actor);
		fixture_register_character(&other);
		actor.next = &other;
		character_list = &actor;
		rooms[0].people = &actor;
		hat = object(100, 0);
		hat->loc_p = LOC_WORN;
		hat->loc.wearing = &actor;
		actor.equipment[0] = hat;
		bag = object(200, 1);
		bag->loc_p = LOC_CARRIED;
		bag->loc.carrying = &actor;
		child = object(201, 2);
		child->loc_p = LOC_INSIDE;
		child->loc.inside = bag;
		bag->contains = child;
		spare = object(300, 0);
		spare->loc_p = LOC_CARRIED;
		spare->loc.carrying = &actor;
		actor.carrying = bag;
		bag->next_content = spare;
		expected = { row(100, 7000, -1, 1), row(200, 7001, -1, 0, true),
			     row(201, 7002, 1, 0, true), row(300, 7000) };
	}
	~fixture()
	{
		fixture_retire_character(&actor);
		fixture_retire_character(&other);
		character_list = nullptr;
		object_list = nullptr;
		descriptor_list = nullptr;
		rooms[0] = {};
		rooms[1] = {};
		item_ownership_runtime_reset();
	}
	P_obj object(uint64_t uid, int number)
	{
		objects.emplace_back();
		P_obj o = &objects.back();
		o->obj_uid = uid;
		o->R_num = number;
		o->g_key = 17;
		o->type = ITEM_CONTAINER;
		o->loc_p = LOC_NOWHERE;
		if (uid == 200)
		{
			detail.keyword = const_cast<char *>("inscription");
			detail.description = const_cast<char *>("original detail");
			o->ex_description = &detail;
		}
		if (uid == 201)
		{
			affect.type = 5;
			affect.data = 77;
			affect.extra2 = 8;
			o->affects = &affect;
		}
		o->name = const_cast<char *>("literal keys");
		o->short_description = const_cast<char *>("literal short");
		o->description = const_cast<char *>("literal room\r\n");
		o->action_description = const_cast<char *>("literal action");
		o->wear_flags = 2;
		o->anti_flags = 3;
		o->anti2_flags = 4;
		o->extra2_flags = 5;
		o->weight = 13;
		o->material = 1;
		o->cost = 97;
		o->condition = 21;
		o->craftsmanship = 3;
		o->bitvector = 1;
		o->bitvector2 = 2;
		o->bitvector3 = 3;
		o->bitvector4 = 4;
		o->bitvector5 = 5;
		for (int i = 0; i < 8; ++i)
			o->value[i] = 101 + i;
		for (int i = 0; i < 6; ++i)
			o->timer[i] = 201 + i;
		for (int i = 0; i < 4; ++i)
		{
			o->affected[i].location = 1 + i;
			o->affected[i].modifier = 31 + i;
		}
		o->next = object_list;
		if (object_list)
			object_list->prev = o;
		object_list = o;
		return o;
	}
	auction_native_world_observation observe()
	{
		auction_native_world_observation result;
		assert(auction_native_world_observe(42, actor.runtime_id, expected, selected,
						    &result));
		assert(result.actor == &actor);
		equal(result.player_items, expected);
		assert(result.selected.size() == selected.size());
		assert(result.selected_trees.size() == selected.size());
		return result;
	}
	void baseline()
	{
		auto value = observe();
		assert(value.selected[0] == bag);
		equal(value.selected_trees[0],
		      { row(200, 7001, -1, 0, true), row(201, 7002, 0, 0, true) });
	}
	void refuse(uint32_t pid = 42, uint64_t runtime = 0, bool explicit_zero = false)
	{
		auction_native_world_observation output;
		output.actor = &other;
		output.player_items = { row(999, 7000) };
		output.selected = { spare };
		output.selected_trees = { { row(998, 7001) } };
		const auto before = bytes(output.player_items),
			   tree_before = bytes(output.selected_trees[0]);
		assert(!auction_native_world_observe(pid,
						     explicit_zero ? runtime : actor.runtime_id,
						     expected, selected, &output));
		assert(output.actor == &other && output.selected == std::vector<P_obj>{ spare });
		assert(bytes(output.player_items) == before && output.selected_trees.size() == 1);
		assert(bytes(output.selected_trees[0]) == tree_before);
	}
	void detach()
	{
		actor.carrying = spare;
		bag->next_content = nullptr;
		bag->loc_p = LOC_NOWHERE;
		bag->loc.room = 0;
		expected = { row(100, 7000, -1, 1), row(300, 7000) };
		selected = { { 200, location::detached } };
	}
};

static void projection()
{
	const forest before{ row(100, 7000, -1, 1), row(300, 7000), row(310, 7001),
			     row(311, 7002, 2) };
	auto a = row(200, 7001, -1, 0, true);
	a.generated_key = 0;
	auto b = row(201, 7002, 0, 0, true);
	b.generated_key = 0;
	auto c = row(400, 7002, -1, 0, true);
	c.generated_key = 0;
	const forest source{ a, b, c };
	auction_command_payload payload{};
	payload.actor_pid = 42;
	payload.action = auction_action::claim_item;
	payload.item_count = 2;
	payload.items[0] = { 200, 1, 7001 };
	payload.items[1] = { 400, 1, 7002 };
	assert(auction_native_selected_forest_valid(payload, source));
	auto placed_a = a;
	placed_a.generated_key = 1;
	auto placed_c = c;
	placed_c.generated_key = 1;
	auto placed_b = b;
	placed_b.parent_index = 3;
	// Second root with no matching VNUM goes at inventory head, after equipment.
	const forest golden{
		row(100, 7000, -1, 1), placed_c,	 row(300, 7000), placed_a, placed_b,
		row(310, 7001),	       row(311, 7002, 5)
	};
	forest output;
	assert(auction_native_expected_player_forest(payload, before, source, false, 56, &output));
	equal(output, golden);
	for (uint32_t level : { 57U, 58U })
	{
		auto high = golden;
		high[1].generated_key = high[3].generated_key = 0;
		assert(auction_native_expected_player_forest(payload, before, source, false, level,
							     &output));
		equal(output, high);
	}
	payload.actor_pid = 10000000;
	auto high_pid = golden;
	high_pid[1].generated_key = high_pid[3].generated_key = 0;
	assert(auction_native_expected_player_forest(payload, before, source, false, 56, &output));
	equal(output, high_pid);
	payload.actor_pid = 42;
	assert(auction_native_expected_player_forest(payload, before, source, true, 56, &output));
	equal(output, before);
	payload.action = auction_action::bid;
	assert(auction_native_expected_player_forest(payload, before, {}, false, 56, &output));
	equal(output, before);
	payload.action = auction_action::list;
	assert(auction_native_expected_player_forest(
		payload, golden, forest{ placed_a, b, placed_c }, false, 56, &output));
	equal(output, before);
	auto damaged = source;
	damaged[0].vnum++;
	assert(!auction_native_selected_forest_valid(payload, damaged));
	damaged = source;
	std::swap(payload.items[0], payload.items[1]);
	assert(!auction_native_selected_forest_valid(payload, damaged));
	std::swap(payload.items[0], payload.items[1]);
	damaged = source;
	damaged[1].string_mask = 0;
	assert(!auction_native_selected_forest_valid(payload, damaged));
	damaged = source;
	damaged[1].object_uid = 200;
	assert(!auction_native_selected_forest_valid(payload, damaged));
	output = { row(999, 7000) };
	const auto sentinel = bytes(output);
	payload.item_count = AUCTION_COMMAND_MAX_ITEMS + 1;
	assert(!auction_native_expected_player_forest(payload, before, source, false, 56, &output));
	assert(bytes(output) == sentinel);
	payload.item_count = 2;
	payload.action = auction_action::claim_item;
	assert(!auction_native_expected_player_forest(payload, before, source, false, 0, &output));
	assert(bytes(output) == sentinel);
	auto collision = before;
	collision.push_back(row(200, 7001));
	assert(!auction_native_expected_player_forest(payload, collision, source, false, 56,
						      &output));
	assert(bytes(output) == sentinel);
}

static void claim_world()
{
	fixture f;
	f.baseline();
	auto existing = f.object(310, 1);
	existing->loc_p = LOC_CARRIED;
	existing->loc.carrying = &f.actor;
	auto existing_child = f.object(311, 2);
	existing_child->loc_p = LOC_INSIDE;
	existing_child->loc.inside = existing;
	existing->contains = existing_child;
	auto second = f.object(400, 2);
	second->loc_p = LOC_CARRIED;
	second->loc.carrying = &f.actor;
	second->g_key = f.bag->g_key = 1;
	f.child->g_key = 0;
	f.actor.carrying = second;
	second->next_content = f.spare;
	f.spare->next_content = f.bag;
	f.bag->next_content = existing;
	auto bag = row(200, 7001, -1, 0, true);
	bag.generated_key = 1;
	auto child = row(201, 7002, 3, 0, true);
	child.generated_key = 0;
	auto other = row(400, 7002, -1, 0, true);
	other.generated_key = 1;
	// Manual physical claim result: matching-VNUM root before the old root;
	// later unmatched root at inventory head, with all equipment preserved.
	f.expected = { row(100, 7000, -1, 1), other, row(300, 7000), bag, child, row(310, 7001),
		       row(311, 7002, 5) };
	f.selected.push_back({ 400, location::carried });
	auto value = f.observe();
	assert(value.selected == std::vector<P_obj>({ f.bag, second }));
	child.parent_index = 0;
	equal(value.selected_trees[0], { bag, child });
	equal(value.selected_trees[1], { other });
	second->next_content = f.bag;
	f.bag->next_content = f.spare;
	f.spare->next_content = existing;
	f.refuse();
}

static void bounds()
{
	fixture f;
	f.baseline();
	const auto literal_expected = f.expected;
	f.expected = { row(100, 7000, -1, 1), row(200, 7001), row(201, 7002, 1), row(300, 7000) };
	// The public selected root bound permits nine absent roots, refuses ten.
	std::vector<auction_native_world_uid> requirements;
	for (uint64_t id = 1000; id < 1009; ++id)
		requirements.push_back({ id, location::absent });
	f.selected = requirements;
	auto value = f.observe();
	assert(value.selected.size() == 9);
	for (auto object : value.selected)
		assert(!object);
	for (const auto &tree : value.selected_trees)
		assert(tree.empty());
	f.selected = requirements;
	f.selected.push_back({ 1009, location::absent });
	f.refuse();
	f.selected = { { 200, location::carried } };
	f.expected = literal_expected;
	auto original = f.expected;
	f.expected[1].name.assign(PLAYER_SNAPSHOT_MAX_STRING_BYTES, 'x');
	std::string text(PLAYER_SNAPSHOT_MAX_STRING_BYTES, 'x');
	f.bag->name = text.data();
	f.observe();
	text.push_back('x');
	f.bag->name = text.data();
	// Keep the expected row valid so this refusal must inspect live capture.
	f.refuse();
	f.expected[1].name = text;
	f.refuse();
	f.bag->name = const_cast<char *>("literal keys");
	f.expected = original;
	forest deep;
	auction_command_payload payload{};
	payload.item_count = 1;
	payload.items[0] = { 1000, 1, 7001 };
	for (size_t i = 0; i < PLAYER_SNAPSHOT_MAX_DEPTH; ++i)
		deep.push_back(row(1000 + i, 7001, i ? static_cast<int>(i - 1) : -1, 0, true));
	assert(auction_native_selected_forest_valid(payload, deep));
	deep.push_back(row(2000, 7001, static_cast<int>(deep.size() - 1), 0, true));
	assert(!auction_native_selected_forest_valid(payload, deep));
	P_obj tail = f.spare;
	for (size_t i = f.expected.size(); i < PLAYER_SNAPSHOT_MAX_OBJECTS; ++i)
	{
		auto o = f.object(10000 + i, 0);
		o->loc_p = LOC_CARRIED;
		o->loc.carrying = &f.actor;
		tail->next_content = o;
		tail = o;
		f.expected.push_back(row(10000 + i, 7000));
	}
	f.observe();
	auto o = f.object(20000, 0);
	o->loc_p = LOC_CARRIED;
	o->loc.carrying = &f.actor;
	tail->next_content = o;
	// Live graph first: do not let an oversized expected forest mask capture.
	f.refuse();
	f.expected.push_back(row(20000, 7000));
	f.refuse();
}

static void depth()
{
	fixture f;
	f.baseline();
	f.detach();
	forest tree{ row(200, 7001, -1, 0, true), row(201, 7002, 0, 0, true) };
	P_obj parent = f.child;
	for (size_t i = 2; i < PLAYER_SNAPSHOT_MAX_DEPTH; ++i)
	{
		auto o = f.object(1000 + i, 2);
		o->loc_p = LOC_INSIDE;
		o->loc.inside = parent;
		parent->contains = o;
		parent = o;
		tree.push_back(row(1000 + i, 7002, static_cast<int>(i - 1), 0, true));
	}
	auto value = f.observe();
	equal(value.selected_trees[0], tree);
	auto o = f.object(2000, 2);
	o->loc_p = LOC_INSIDE;
	o->loc.inside = parent;
	parent->contains = o;
	f.refuse();
}

static void byte_budget()
{
	fixture f;
	f.baseline();
	f.detach();
	constexpr size_t nodes = 300;
	assert(nodes < PLAYER_SNAPSHOT_MAX_OBJECTS);
	assert(nodes * 4 * PLAYER_SNAPSHOT_MAX_STRING_BYTES > PLAYER_SNAPSHOT_MAX_BYTES);
	std::string text(PLAYER_SNAPSHOT_MAX_STRING_BYTES, 'x');
	auto strings = [&](P_obj o)
	{
		o->name = text.data();
		o->short_description = text.data();
		o->description = text.data();
		o->action_description = text.data();
	};
	strings(f.bag);
	strings(f.child);
	P_obj tail = f.child;
	for (size_t i = 2; i < nodes; ++i)
	{
		auto o = f.object(10000 + i, 2);
		o->loc_p = LOC_INSIDE;
		o->loc.inside = f.bag;
		strings(o);
		tail->next_content = o;
		tail = o;
	}
	// Every string and the shallow graph are individually valid; their combined
	// literal payload exceeds the real capture budget and must preserve output.
	f.refuse();
}

static void norent()
{
	fixture f;
	f.baseline();
	f.bag->extra_flags = ITEM_NORENT;
	f.child->extra_flags = ITEM_NORENT;
	f.expected[1].extra_flags = f.expected[2].extra_flags = ITEM_NORENT;
	auto literal = f.observe();
	equal(literal.selected_trees[0], { row(200, 7001, -1, 0, true, ITEM_NORENT),
					   row(201, 7002, 0, 0, true, ITEM_NORENT) });
	forest ordinary;
	assert(player_item_snapshot_list_capture(&f.actor, true, true, true, &ordinary, nullptr) ==
	       player_snapshot_capture_result::ok);
	equal(ordinary, { row(100, 7000, -1, 1), row(300, 7000) });
	// Runtime rows are synthetic component facts, never native SQL proof.
	assert(item_ownership_runtime_hydrate({ 200,
						200,
						0,
						{ item_owner_type::player, 42, 0 },
						1,
						1,
						7001,
						item_custody_state::active }));
	assert(item_ownership_runtime_hydrate({ 201,
						200,
						200,
						{ item_owner_type::player, 42, 0 },
						1,
						1,
						7002,
						item_custody_state::active }));
	assert(player_item_snapshot_list_capture(&f.actor, true, true, true, &ordinary, nullptr) ==
	       player_snapshot_capture_result::ok);
	equal(ordinary, { row(100, 7000, -1, 1), row(200, 7001, -1, 0, false, ITEM_NORENT),
			  row(201, 7002, 1, 0, false, ITEM_NORENT), row(300, 7000) });
	f.observe();
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const std::string name = argv[1];
	fixture_check_runtime_identity_retirement();
	if (name == "projection")
		projection();
	else if (name == "claim_world")
		claim_world();
	else if (name == "bounds")
		bounds();
	else if (name == "depth")
		depth();
	else if (name == "byte_budget")
		byte_budget();
	else if (name == "norent")
		norent();
	else
	{
		fixture f;
		f.baseline();
		if (name == "carried")
			f.baseline();
		else if (name == "detached")
		{
			f.detach();
			auto value = f.observe();
			assert(value.selected[0] == f.bag);
			equal(value.selected_trees[0],
			      { row(200, 7001, -1, 0, true), row(201, 7002, 0, 0, true) });
		}
		else if (name == "absent")
		{
			f.selected = { { 999, location::absent } };
			f.expected = { row(100, 7000, -1, 1), row(200, 7001), row(201, 7002, 1),
				       row(300, 7000) };
			auto value = f.observe();
			assert(value.selected[0] == nullptr && value.selected_trees[0].empty());
		}
		else if (name == "descriptor_original")
		{
			character_list = &f.other;
			f.other.next = nullptr;
			rooms[0].people = nullptr;
			f.descriptor.original = &f.actor;
			f.descriptor.character = &f.other;
			descriptor_list = &f.descriptor;
			f.other.desc = &f.descriptor;
			f.observe();
		}
		else if (name == "multiple_discovery")
		{
			f.descriptor.original = &f.actor;
			f.descriptor.character = &f.actor;
			descriptor_list = &f.descriptor;
			f.actor.desc = &f.descriptor;
			f.observe();
		}
		else if (name == "runtime_zero_absent")
		{
			f.expected.clear();
			f.selected = { { 999, location::absent } };
			character_list = &f.other;
			rooms[0].people = nullptr;
			auction_native_world_observation out;
			assert(auction_native_world_observe(42, 0, {}, f.selected, &out));
			assert(!out.actor);
			assert(out.player_items.empty() && out.selected.size() == 1 &&
			       !out.selected[0] && out.selected_trees[0].empty());
		}
		else
		{
			if (name == "replacement_body")
			{
				f.other_pc.pid = 42;
				character_list = &f.other;
				rooms[0].people = &f.other;
				f.other.carrying = f.actor.carrying;
				f.other.equipment[0] = f.hat;
				f.actor.carrying = nullptr;
				f.actor.equipment[0] = nullptr;
				f.hat->loc.wearing = &f.other;
				f.bag->loc.carrying = f.spare->loc.carrying = &f.other;
				auction_native_world_observation replacement;
				assert(auction_native_world_observe(42, f.other.runtime_id,
								    f.expected, f.selected,
								    &replacement));
				assert(replacement.actor == &f.other);
				equal(replacement.player_items, f.expected);
				// The same complete forest is valid on the new body, but cannot
				// satisfy the original runtime identity passed to refuse().
			}
			else if (name == "duplicate_actor")
				f.other_pc.pid = 42;
			else if (name == "runtime_wrong_pid")
				f.pc.pid = 44;
			else if (name == "runtime_zero_present")
			{
				f.expected.clear();
				f.refuse(42, 0, true);
				goto done;
			}
			else if (name == "duplicate_selected")
				f.selected.push_back(f.selected[0]);
			else if (name == "duplicate_world_uid")
				f.object(200, 1);
			else if (name == "duplicate_child_uid")
				f.object(201, 2);
			else if (name == "norent_duplicate")
				f.object(200, 1)->extra_flags = ITEM_NORENT;
			else if (name == "dual_room_player")
				rooms[1].contents = f.bag;
			else if (name == "missing_reciprocal")
				f.bag->loc_p = LOC_NOWHERE;
			else if (name == "wrong_carrier")
				f.bag->loc.carrying = &f.other;
			else if (name == "extra_child")
			{
				auto extra = f.object(202, 2);
				extra->loc_p = LOC_INSIDE;
				extra->loc.inside = f.bag;
				f.child->next_content = extra;
			}
			else if (name == "missing_child")
			{
				f.bag->contains = nullptr;
				f.child->loc_p = LOC_NOWHERE;
			}
			else if (name == "reparented_child")
			{
				f.bag->contains = nullptr;
				f.spare->contains = f.child;
				f.child->loc.inside = f.spare;
			}
			else if (name == "wrong_child_reciprocal")
				f.child->loc.inside = f.spare;
			else if (name == "sibling_cycle")
				f.child->next_content = f.child;
			else if (name == "container_cycle")
			{
				f.child->contains = f.bag;
				f.bag->loc_p = LOC_INSIDE;
				f.bag->loc.inside = f.child;
			}
			else if (name == "global_prev")
				f.child->prev = nullptr;
			else if (name == "global_next_cycle")
				f.hat->next = f.spare;
			else if (name == "character_cycle")
				f.other.next = &f.actor;
			else if (name == "room_people_cycle")
				f.actor.next_in_room = &f.actor;
			else if (name == "descriptor_cycle")
			{
				descriptor_list = &f.descriptor;
				f.descriptor.next = &f.descriptor;
			}
			else if (name == "literal_changed")
				f.bag->cost++;
			else if (name == "dynamic_changed")
				f.affect.data++;
			else if (name == "description_changed")
				f.detail.description = const_cast<char *>("changed detail");
			else if (name == "equipment_changed")
				f.hat->weight++;
			else if (name == "unrelated_changed")
				f.spare->timer[0]++;
			else if (name == "invalid_expected_parent")
				f.expected[2].parent_index = 3;
			else if (name == "invalid_expected_mask")
				f.expected[1].string_mask = 16;
			else if (name == "carried_literal_mask")
				f.expected[1].string_mask = 0;
			else if (name == "invalid_location")
				f.selected[0].location = static_cast<location>(3);
			else if (name == "zero_selected_uid")
				f.selected[0].uid = 0;
			else if (name == "max_selected_uid")
				f.selected[0].uid = UINT64_MAX;
			else if (name == "absent_present")
			{
				f.selected[0].location = location::absent;
			}
			else if (name == "detached_linked")
				f.selected[0].location = location::detached;
			else if (name == "detached_next_content")
			{
				f.detach();
				f.bag->next_content = f.spare;
			}
			else if (name == "off_thread")
			{
				std::thread worker([&] { f.refuse(); });
				worker.join();
				goto done;
			}
			else
				assert(false && "unknown case");
			f.refuse();
		}
	}
done:
	std::printf(
		"PASS %s actual_observer_capture_codec=1 native_sql_publication_qualification=0\n",
		name.c_str());
}
'''

CASES = (
    "carried", "detached", "absent", "descriptor_original", "multiple_discovery",
    "runtime_zero_absent", "replacement_body", "duplicate_actor", "runtime_wrong_pid",
    "runtime_zero_present", "duplicate_selected", "duplicate_world_uid",
    "duplicate_child_uid", "norent_duplicate", "dual_room_player", "missing_reciprocal",
    "wrong_carrier", "extra_child", "missing_child", "reparented_child",
    "wrong_child_reciprocal", "sibling_cycle", "container_cycle", "global_prev",
    "global_next_cycle", "character_cycle", "room_people_cycle", "descriptor_cycle",
    "literal_changed", "dynamic_changed",
    "description_changed", "equipment_changed", "unrelated_changed",
    "invalid_expected_parent", "invalid_expected_mask", "carried_literal_mask",
    "invalid_location", "zero_selected_uid", "max_selected_uid", "absent_present",
    "detached_linked", "detached_next_content", "off_thread", "projection", "bounds",
    "depth", "byte_budget", "norent", "claim_world",
)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--optimization", choices=("O1", "Og"), default="O1")
    args = parser.parse_args()
    root = args.source_root.resolve()
    output = (args.build_dir or Path(os.environ.get("BIN_ROOT", ROOT / "bin")) /
              "tests" / "auction-native-world-observation" /
              (args.optimization + "-" + str(time.time_ns()))).resolve()
    output.mkdir(parents=True, exist_ok=False)
    harness = output / "driver.cpp"
    harness.write_text(HARNESS, encoding="utf-8")
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    version = subprocess.run(compiler + ["--version"], check=True, text=True,
                             capture_output=True).stdout
    (output / "compiler-version.txt").write_text(version)
    compiler_path = Path(shutil.which(compiler[0])).resolve()
    (output / "compiler-pin.json").write_text(json.dumps(
        {"path": str(compiler_path), "sha256": sha256(compiler_path)}, indent=2))
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-" + args.optimization, "-g", "-fsanitize=address,undefined",
             "-fno-omit-frame-pointer", "-fno-pie", "-ffunction-sections",
             "-fdata-sections", "-D__NO_MYSQL__", "-Isrc", "-Isrc/no_mysql", "-I."]
    commands = []
    direct = [root / source for source in SOURCES] + [harness, Path(__file__).resolve()]
    before = {str(path): sha256(path) for path in direct}
    units = [harness] + [root / source for source in SOURCES]

    def compile_unit(index_source):
        index, source = index_source
        obj = output / (str(index) + ".o")
        command = compiler + flags + ["-MD", "-MF", str(output / (str(index) + ".d")),
                                      "-c", str(source), "-o", str(obj)]
        start = time.monotonic()
        with (output / (str(index) + "-compile.log")).open("w") as log:
            result = subprocess.run(command, cwd=root, stdout=log, stderr=subprocess.STDOUT)
        return {"command": command, "returncode": result.returncode,
                "seconds": time.monotonic() - start}

    # These are independent compilation jobs, not substitute providers.
    with ThreadPoolExecutor(max_workers=3) as pool:
        commands.extend(pool.map(compile_unit, enumerate(units)))
    (output / "compile-commands.json").write_text(json.dumps(commands, indent=2))
    assert all(entry["returncode"] == 0 for entry in commands), str(output)
    dependencies = set(direct)
    for depfile in output.glob("*.d"):
        # GCC make dependencies escape spaces and wrap lines; shlex handles the
        # escaped paths after the target colon (Linux paths contain no drive colon).
        value = depfile.read_text().replace(chr(92) + chr(10), " ").split(":", 1)[1]
        dependencies.update((root / token).resolve() for token in shlex.split(value))
    pins = {str(path): sha256(path) for path in sorted(dependencies)}
    assert all(pins[path] == digest for path, digest in before.items())
    (output / "input-pins.json").write_text(json.dumps(pins, indent=2))
    executable = output / "auction-native-world-observation"
    link = compiler + flags + ["-no-pie"] + [str(output / (str(i) + ".o"))
                                            for i in range(len(units))] + [
        "-Wl,--gc-sections", "-Wl,-Map=" + str(output / "link.map"),
        "-lcrypto", "-pthread", "-o", str(executable)]
    with (output / "link.log").open("w") as log:
        result = subprocess.run(link, cwd=root, stdout=log, stderr=subprocess.STDOUT)
    (output / "link-command.json").write_text(json.dumps(
        {"command": link, "returncode": result.returncode}, indent=2))
    assert result.returncode == 0, str(output)
    libraries = subprocess.run(["ldd", str(executable)], check=True, text=True,
                               capture_output=True).stdout
    (output / "shared-libraries.txt").write_text(libraries)
    library_pins = {}
    for line in libraries.splitlines():
        fields = line.split()
        path = next((Path(field) for field in fields if field.startswith("/")), None)
        if path:
            library_pins[str(path.resolve())] = sha256(path)
    (output / "shared-library-pins.json").write_text(json.dumps(library_pins, indent=2))
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    runs = []
    for case in CASES:
        result = subprocess.run([str(executable), case], cwd=root, env=environment,
                                text=True, capture_output=True, timeout=90)
        (output / (case + ".log")).write_text(result.stdout + result.stderr)
        runs.append({"case": case, "returncode": result.returncode})
        (output / "runs.json").write_text(json.dumps(runs, indent=2))
        assert result.returncode == 0 and result.stdout.startswith("PASS " + case + " "), (
            case, result.stdout, result.stderr, str(output))
    assert all(sha256(Path(path)) == digest for path, digest in pins.items())
    artifacts = {path.name: sha256(path) for path in sorted(output.iterdir()) if path.is_file()}
    (output / "artifact-pins.json").write_text(json.dumps(artifacts, indent=2))
    print(json.dumps({"passed": len(runs), "optimization": args.optimization,
                      "output": str(output), "elf_sha256": sha256(executable),
                      "native_sql_publication_qualification": False}, indent=2))


if __name__ == "__main__":
    main()
