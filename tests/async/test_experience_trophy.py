#!/usr/bin/env python3
"""Exercise in-memory XP observation and its checkpoint ownership contract."""

from pathlib import Path
import subprocess
import shlex
import tempfile

ROOT = Path(__file__).resolve().parents[2]

HARNESS = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/trophy.h"
#include "player/player_revision_state.h"
#include "player/player_snapshot_codec.h"

#include <cassert>
#include <climits>
#include <cstdlib>
#include <cstring>

room_data rooms[2]{};
P_room world = rooms;
extern const int top_of_world = 1;
zone_data zones[2]{};
zone_data *zone_table = zones;
int top_of_zone_table = 1;
int observe = 0;
P_char pet = nullptr;
P_char owner = nullptr;

int get_property(const char *name, int fallback)
{
    assert(std::strcmp(name, "exp.zoneTrophy.observe") == 0);
    assert(fallback == 0);
    return observe;
}
P_char get_linked_char(P_char ch, ush_int type)
{
    assert(type == LNK_PET);
    return ch == pet ? owner : nullptr;
}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
void mark_player_dirty_components(int pid, player_component_mask_t mask)
{
    assert(player_revision_mark(pid, mask, nullptr));
}

int main()
{
    char_data ch{}, mob{}, patient{}, enemy{};
    pc_only_data pc{};
    ch.only.pc = &pc;
    pc.pid = 7;
    ch.player.level = 25;
    ch.player.race = RACE_HUMAN;
    mob.specials.act = ACT_ISNPC;
    zones[0].number = 12;
    zones[1].number = 34;
    rooms[0].zone = 0;
    rooms[1].zone = 1;
    assert(player_revision_hydrate(7, 0));

    assert(!record_zone_trophy_award(&ch, &mob, 100, EXP_KILL));
    assert(!pc.zone_trophy);
    observe = 1;
    assert(!record_zone_trophy_award(nullptr, &mob, 100, EXP_KILL));
    assert(!record_zone_trophy_award(&mob, &ch, 100, EXP_KILL));
    assert(!record_zone_trophy_award(&ch, &mob, 0, EXP_KILL));
    assert(!record_zone_trophy_award(&ch, &mob, -1, EXP_DEATH));
    ch.player.level = 24;
    assert(!record_zone_trophy_award(&ch, &mob, 100, EXP_KILL));
    ch.player.level = MAXLVLMORTAL + 1;
    assert(!record_zone_trophy_award(&ch, &mob, 100, EXP_KILL));
    ch.player.level = 25;
    ch.player.race = RACE_ILLITHID;
    assert(!record_zone_trophy_award(&ch, &mob, 100, EXP_KILL));
    ch.player.race = RACE_HUMAN;
    for (int type : {EXP_QUEST, EXP_WORLD_QUEST, EXP_RESURRECT, EXP_DEATH})
        assert(!record_zone_trophy_award(&ch, &mob, 100, type));
    for (int type : {EXP_KILL, EXP_MELEE, EXP_DAMAGE, EXP_TANKING})
    {
        assert(!record_zone_trophy_award(&ch, &enemy, 100, type));
        assert(record_zone_trophy_award(&ch, &mob, 1, type));
    }
    assert(pc.zone_trophy->size() == 1 && pc.zone_trophy->front().exp == 4);
    patient.specials.fighting = &enemy;
    assert(!record_zone_trophy_award(&ch, &patient, 10, EXP_HEALING));
    patient.specials.fighting = &mob;
    assert(record_zone_trophy_award(&ch, &patient, 10, EXP_HEALING));
    pet = &mob;
    owner = &ch;
    assert(!record_zone_trophy_award(&ch, &mob, 100, EXP_KILL));
    assert(!record_zone_trophy_award(&ch, &patient, 100, EXP_HEALING));
    pet = nullptr;
    ch.in_room = NOWHERE;
    assert(!record_zone_trophy_award(&ch, &mob, 100, EXP_KILL));
    ch.in_room = 2;
    assert(!record_zone_trophy_award(&ch, &mob, 100, EXP_KILL));
    ch.in_room = 1;
    rooms[1].zone = 2;
    assert(!record_zone_trophy_award(&ch, &mob, 100, EXP_KILL));
    rooms[1].zone = 1;
    assert(record_zone_trophy_award(&ch, &mob, 20, EXP_KILL));
    assert(pc.zone_trophy->size() == 2);
    assert(pc.zone_trophy->front().exp == 14); // No cross-zone decay writer.

    // A snapshot in flight owns a copy, and acknowledging it cannot clear a
    // subsequent trophy/XP update. This uses the real revision and codec code.
    const auto mask = PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES;
    assert(player_revision_mark(7, mask, nullptr));
    player_revision_t revision = 0;
    player_component_mask_t components = 0;
    assert(player_revision_queue(7, &revision, &components));
    assert(components == mask);
    assert(player_revision_begin_inflight(7, revision, components));
    player_snapshot snapshot{};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.encoded_size_bound = 4096;
    snapshot.pid = 7;
    snapshot.revision = revision;
    snapshot.components = components;
    for (const auto &entry : *pc.zone_trophy)
        snapshot.trophies.push_back({entry.zone_number, entry.exp});
    std::vector<uint8_t> bytes;
    assert(player_snapshot_encode(snapshot, &bytes) == player_snapshot_codec_result::ok);
    assert(record_zone_trophy_award(&ch, &mob, 7, EXP_KILL));
    assert(player_revision_mark(7, mask, nullptr));
    assert(player_revision_acknowledge(7, revision, components));
    player_revision_snapshot state{};
    assert(player_revision_snapshot_copy(7, &state));
    assert(state.dirty_components == mask && state.unacknowledged_components == mask);
    player_snapshot restored{};
    assert(player_snapshot_decode(bytes.data(), bytes.size(), &restored) ==
           player_snapshot_codec_result::ok);
    assert(restored.trophies[1].experience == 20 && pc.zone_trophy->back().exp == 27);

    assert(record_zone_trophy_award(&ch, &mob, INT_MAX, EXP_KILL));
    assert(pc.zone_trophy->back().exp == INT_MAX);
    assert(!record_zone_trophy_award(&ch, &mob, 1, EXP_KILL));
    observe = 0;
    ch.in_room = 0;
    assert(!record_zone_trophy_award(&ch, &mob, 100, EXP_KILL));
    assert(pc.zone_trophy->front().exp == 14);
    observe = 1;
    for (size_t i = pc.zone_trophy->size(); i < ZONE_TROPHY_MAX_ZONES; ++i)
        pc.zone_trophy->push_back({static_cast<int>(i) + 100, 1});
    zones[0].number = 99999;
    assert(!record_zone_trophy_award(&ch, &mob, 100, EXP_KILL));
    assert(pc.zone_trophy->size() == ZONE_TROPHY_MAX_ZONES);
    clear_zone_trophy(&ch);
    assert(pc.zone_trophy->empty());
    assert(player_revision_snapshot_copy(7, &state));
    assert(state.dirty_components & PLAYER_COMPONENT_TROPHIES);
    delete pc.zone_trophy;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-trophy-") as temporary:
    source = Path(temporary) / "trophy.cpp"
    source.write_text(HARNESS)
    for backend in ("sql", "flatfile"):
        binary = Path(temporary) / backend
        flags = (["-D__NO_MYSQL__", "-Isrc/no_mysql"] if backend == "flatfile" else
                 shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True)))
        subprocess.run(
            ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-ffunction-sections", "-fdata-sections", "-Isrc", *flags, str(source),
             "src/item/trophy.c", "src/player/player_revision_state.c",
             "src/player/player_snapshot_codec.c", "-Wl,--gc-sections", "-o", str(binary)],
            cwd=ROOT, check=True,
        )
        subprocess.run([str(binary)], check=True)

# Integration contract: observe precisely the credited amount inside the same
# acceptance branch and revision, never the intermediate pre-cap XP calculations.
limits = (ROOT / "src/world/limits.c").read_text()
assert limits.count("record_zone_trophy_award(") == 1
credit = limits.index("GET_EXP(ch) += (int)XP_final;")
observation = limits.index("record_zone_trophy_award(ch, victim, XP_final, type)")
dirty = limits.index("mark_player_dirty_components", observation)
assert credit < observation < dirty < limits.index("display_gain(ch", dirty)
assert "components |= PLAYER_COMPONENT_TROPHIES;" in limits
assert "modify_exp_by_zone_trophy" not in limits
trophy = (ROOT / "src/item/trophy.c").read_text()
for forbidden in ("qry(", "mysql_", "get_zone_info(", "save_zone_trophy", "exp_mod"):
    assert forbidden not in trophy
maintenance = (ROOT / "src/persistence/maintenance_repository.c").read_text()
dispatch = maintenance.split("maintenance_result maintenance_repository_execute(", 1)[1]
assert dispatch.index("request.job_id == maintenance_job_id::zone_trophy") < dispatch.index("sql_pool_acquire()")
assert "UPDATE zone_trophy" not in maintenance
print("experience trophy observation and checkpoint contracts passed")
