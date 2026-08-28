#!/usr/bin/env python3
"""Boot restore orchestration regression for flat-file player corpses."""

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "flatfile_corpse_restore.h"
#include "flatfile_corpse_ownership.h"
#include "flatfile_world_item_repository.h"
#include "necromancy.h"
#include "player_load_items.h"
#include "prototypes.h"
#include "structs.h"

#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

int skip_corpse_save = 0;
static P_obj published = nullptr;
static uint64_t hydrated_revision = 0;
static int refreshes = 0;

char *str_dup(const char *source)
{
	const size_t size = std::strlen(source) + 1;
	char *copy = static_cast<char *>(std::malloc(size));
	assert(copy);
	std::memcpy(copy, source, size);
	return copy;
}

void __free(void *pointer, const char *, int)
{
	std::free(pointer);
}

int checked_snprintf(char *destination, size_t size, const char *format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	const int result = std::vsnprintf(destination, size, format, arguments);
	va_end(arguments);
	return result;
}

void set_keywords(P_obj object, const char *value)
{
	object->name = str_dup(value);
	object->str_mask |= STRUNG_KEYS;
}

void set_short_description(P_obj object, const char *value)
{
	object->short_description = str_dup(value);
	object->str_mask |= STRUNG_DESC2;
}

void set_long_description(P_obj object, const char *value)
{
	object->description = str_dup(value);
	object->str_mask |= STRUNG_DESC1;
}

int real_room(int vnum)
{
	return vnum == 500 ? 5 : NOWHERE;
}

int real_object(int vnum)
{
	return vnum == 2 ? 2 : -1;
}

P_obj read_object(int rnum, int)
{
	P_obj object = static_cast<P_obj>(std::calloc(1, sizeof(obj_data)));
	assert(object);
	object->R_num = rnum;
	object->loc_p = LOC_NOWHERE;
	object->loc.room = NOWHERE;
	return object;
}

P_obj create_money(int copper, int silver, int gold, int platinum)
{
	P_obj object = read_object(99, REAL);
	object->type = ITEM_MONEY;
	object->value[0] = copper;
	object->value[1] = silver;
	object->value[2] = gold;
	object->value[3] = platinum;
	return object;
}

void obj_to_room(P_obj object, int room)
{
	object->loc_p = LOC_ROOM;
	object->loc.room = room;
	published = object;
}

static void release(P_obj object)
{
	while (object->contains)
	{
		P_obj child = object->contains;
		object->contains = child->next_content;
		release(child);
	}
	if (object->str_mask & STRUNG_KEYS)
		std::free(object->name);
	if (object->str_mask & STRUNG_DESC1)
		std::free(object->description);
	if (object->str_mask & STRUNG_DESC2)
		std::free(object->short_description);
	if (object->str_mask & STRUNG_DESC3)
		std::free(object->action_description);
	std::free(object);
}

void extract_obj(P_obj object, int)
{
	release(object);
}

void persistence_refresh_restored_corpse(P_obj object, const char *)
{
	assert(object == published);
	++refreshes;
}

flatfile_world_item_result
flatfile_world_item_list(const std::string &, std::vector<flatfile_corpse_record> *corpses,
			 std::vector<flatfile_saved_world_item_record> *saved_items,
			 std::string *)
{
	flatfile_corpse_record record = {};
	record.owner_pid = 42;
	record.owner_name = "hero";
	record.save_id = 20;
	record.room_vnum = 500;
	record.short_description = "the corpse of Hero";
	record.description = "The corpse of Hero is lying here.";
	record.keywords = "hero corpse _pcorpse_";
	record.weight = 75;
	record.values[CORPSE_FLAGS] = PC_CORPSE;
	record.values[CORPSE_PID] = 42;
	record.values[CORPSE_RACEWAR] = 1;
	record.values[CORPSE_SAVEID] = 20;
	record.money = { 1, 2, 3, 4 };
	record.revision = 7;
	corpses->push_back(record);
	saved_items->clear();
	return flatfile_world_item_result::ok;
}

item_owner_identity flatfile_corpse_item_owner(uint32_t owner_pid, uint32_t save_id)
{
	return { item_owner_type::corpse,
		 (static_cast<uint64_t>(owner_pid) << 32) | save_id, 0 };
}

flatfile_corpse_ownership_result flatfile_corpse_load_item_ownership(
	const std::string &, const flatfile_corpse_record &, uint64_t *owner_revision,
	std::vector<player_load_item_identity> *identities, std::string *)
{
	*owner_revision = 0;
	identities->clear();
	return flatfile_corpse_ownership_result::ok;
}

bool player_load_item_graph_materialize_detached(
	const std::vector<player_item_snapshot> &,
	const std::vector<player_load_item_identity> &, const item_owner_identity &, uint64_t, bool,
	bool, std::vector<P_obj> *, player_load_item_materialize_metrics *)
{
	return false;
}

bool item_ownership_runtime_hydrate_owner(const item_owner_identity &, uint64_t)
{
	return true;
}

void item_ownership_runtime_forget(uint64_t)
{
}

void item_ownership_runtime_forget_owner(const item_owner_identity &)
{
}

bool corpse_lifecycle_transaction_hydrate(uint32_t owner_pid, uint32_t save_id,
					  uint64_t revision)
{
	assert(owner_pid == 42 && save_id == 20);
	hydrated_revision = revision;
	return true;
}

bool corpse_lifecycle_transaction_forget(uint32_t, uint32_t)
{
	return true;
}

int main()
{
	std::string error;
	assert(flatfile_corpse_restore_catalog("state", &error) ==
	       flatfile_corpse_restore_result::ok);
	assert(skip_corpse_save == 0 && published && refreshes == 1 && hydrated_revision == 7);
	assert(published->type == ITEM_CORPSE && published->weight == 75 &&
	       published->value[CORPSE_PID] == 42 && published->loc.room == 5);
	assert(std::strcmp(published->action_description, "hero") == 0 &&
	       std::strcmp(published->name, "hero corpse _pcorpse_") == 0);
	assert(published->contains && published->contains->type == ITEM_MONEY &&
	       published->contains->value[0] == 1 && published->contains->value[3] == 4 &&
	       published->contains->loc.inside == published);
	release(published);
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-flatfile-corpse-restore-") as temporary:
    temporary_path = pathlib.Path(temporary)
    source = temporary_path / "harness.cpp"
    binary = temporary_path / "harness"
    source.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            str(source),
            "src/flatfile_corpse_restore.c",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("[PASS] flat-file corpse boot restore publishes metadata and money")
