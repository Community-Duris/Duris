#!/usr/bin/env python3
"""Fail-closed copyover ordering and live-process recovery contracts."""

from _paths import SRC
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
copyover = (SRC / "copyover.c").read_text()
comm = (SRC / "comm.c").read_text()

body = copyover[copyover.index("bool copyover_save("):copyover.index(
    "static P_char copyover_load_player", copyover.index("bool copyover_save(")
)]
recover = copyover[copyover.index("static P_char copyover_load_player"):]
descriptor_capture = copyover[copyover.index("static int write_desc_entry("):
                              copyover.index("static int write_mob_entry(")]
save = body.index("persistence_save_character_terminal")
flush = body.index("persistence_flush_all_character_saves")
drain = body.index("player_save_pipeline_drain")
publish = body.index("rename(copyover_tmp, COPYOVER_FILE)")
close = body.index("close(d->descriptor)")
prepare_client = body.index("copyover_prepare_socket(d->descriptor)")
progress_notice = body.index("*** Copyover in progress... ***")
execute = body.index("execl(")

checks = {
    "copyover returns failure": body.count("return false;") >= 10,
    "ships precede characters": body.index("drain_pending_ship_saves") < save,
    "lockers precede characters": body.index("locker_async_drain") < save,
    "connected saves precede remaining flush": save < flush,
    "copyover terminal saves require database acknowledgement":
        "persistence_save_character_terminal_database_acknowledged(" in body and
        "persistence_save_character_terminal(d->character" not in body,
    "all saves precede publication": flush < publish,
    "pipeline drain precedes publication": flush < drain < publish,
    "pipeline drain is bounded": "player_save_pipeline_drain(3000)" in body,
    "copyover abort reopens pipeline": "player_save_pipeline_resume();" in
                                       copyover[copyover.index("static void notify_copyover_failure"):
                                                copyover.index("static void raw_write_to_fd", copyover.index("static void notify_copyover_failure"))],
    "publication precedes descriptor close": publish < close,
    "publication precedes client fd mutation": publish < prepare_client,
    "publication precedes progress notice": publish < progress_notice,
    "descriptor close precedes exec": close < execute,
    "failure says server remains live": "server remains live" in body,
    "copyover runs inside live game loop": "copyover_save(s, S, WS)" in
                                           comm[comm.index("void game_loop("):],
    "typed workers stop only after game loop returns": "critical_command_coordinator_shutdown();" in
                                                       comm[comm.index("game_loop(port, sslport);"):],
    "legacy raw workers are not stopped at shutdown": "persistence_stop_scalar_event_worker();" not in
                                                       comm,
    "failure resumes game loop": "goto resume_game_loop;" in comm,
    "shutdown drain is fail closed": "!player_save_pipeline_drain(3000)" in comm and
                                      "pipeline_drain_failed" in comm,
    "no destructive restart fallback": "refusing fallback exit" in comm,
    "copyover reloads durable player inventory and pets":
        "request.include_items = true;" in recover and
        "request.include_pets = true;" in recover and
        "restoreItemsOnly(ch, 0)" not in recover,
    "copyover places the materialized pet graph": "player_load_pets_place(ch);" in recover,
    "copyover does not rebuild descriptor pets as permanent prototypes":
        "setup_pet(pet, ch, -1" not in recover and
        "desc_entry.pet_vnums[p]" not in recover,
    "copyover keeps materialized inventory attached": "reset_char(ch);" not in recover,
    "descriptor capture requires the player's actual pet link":
        "GET_MASTER(f->follower) == ch" in descriptor_capture,
    "world mob capture excludes every linked pet":
        "ch->in_room >= 0 && !GET_MASTER(ch)" in body,
    "minimal copyover preserves its world dataset":
        'execl(DMS_RUNTIME_BINARY, "dms", "--minimal", "-C", exec_buf' in body,
}

for name, passed in checks.items():
    print(f"[{'PASS' if passed else 'FAIL'}] {name}")
assert all(checks.values())

HARNESS = r'''
#include "core/utils.h"
#include "persistence/copyover.h"
#include <cassert>
#include <cstdlib>
#include <cstdio>

index_data mob_indexes[2] = {};
P_index mob_index = mob_indexes;

int panic_corruption_int(const char *, const char *, ...)
{
    std::abort();
}

P_char get_linked_char(P_char ch, ush_int type)
{
    for (char_link_data *link = ch->linking; link; link = link->next_linking)
        if (link->type == type) return link->linked;
    return nullptr;
}
''' + descriptor_capture + r'''
int main()
{
    char_data owner = {}, pet = {}, unrelated = {};
    pc_only_data owner_pc = {};
    npc_only_data pet_npc = {}, unrelated_npc = {};
    owner.only.pc = &owner_pc;
    owner.player.name = const_cast<char *>("owner");
    owner.in_room = pet.in_room = unrelated.in_room = 0;
    pet.only.npc = &pet_npc;
    unrelated.only.npc = &unrelated_npc;
    SET_BIT(pet.specials.act, ACT_ISNPC);
    SET_BIT(unrelated.specials.act, ACT_ISNPC);
    pet_npc.R_num = 0;
    unrelated_npc.R_num = 1;
    mob_indexes[0].virtual_number = 1201;
    mob_indexes[1].virtual_number = 1202;
    GET_HIT(&pet) = GET_MAX_HIT(&pet) = 100;
    GET_HIT(&unrelated) = GET_MAX_HIT(&unrelated) = 100;
    char_link_data pet_link = {};
    pet_link.type = LNK_PET;
    pet_link.linking = &pet;
    pet_link.linked = &owner;
    pet.linking = &pet_link;
    follow_type unrelated_follow{&unrelated, nullptr};
    follow_type pet_follow{&pet, &unrelated_follow};
    owner.followers = &pet_follow;
    descriptor_data descriptor = {};
    descriptor.descriptor = 10;
    descriptor.character = &owner;
    FILE *file = tmpfile();
    assert(file);
    assert(write_desc_entry(file, &descriptor));
    rewind(file);
    copyover_desc saved = {};
    assert(fread(&saved, sizeof(saved), 1, file) == 1);
    fclose(file);
    assert(saved.num_pets == 1);
    assert(saved.pet_vnums[0] == 1201);
}
'''
with tempfile.TemporaryDirectory(prefix="duris-copyover-pet-owner-") as directory:
    source = Path(directory) / "copyover_pet_owner.cpp"
    binary = Path(directory) / "copyover_pet_owner"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-Isrc",
                    str(source), "-lbsd", "-o", str(binary)], cwd=root, check=True)
    subprocess.run([str(binary)], check=True)

print("copyover save guards passed")
