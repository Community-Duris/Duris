#!/usr/bin/env python3
"""Execute the shipped shrug lookup and resistance, including playable vampires."""
import os
import re
import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, HARNESS_STUBS, extract_function, source

innates = source("classes/innates.c").read_text()
macros = innates[innates.index("#define ADD_RACIAL_INNATE"):innates.index("float regen_factor")]
arrays = innates[innates.index("char racial_innates["):innates.index("extern const struct race_names")]
race_table = re.search(r"const struct race_names race_names_table\[[^\n]+ = .*?\n};",
                       source("core/common.c").read_text(), re.S).group()
production = "\n".join(extract_function(file, signature) for file, signature in [
    ("utility.c", "int flag2idx("),
    ("affects.c", "struct affected_type *get_spell_from_char("),
    ("net/sparser.c", "void update_racial_shrug_data("),
    ("classes/innates.c", "static int innate_char_race("),
    ("classes/innates.c", "static int innate_unlock_level("),
    ("classes/innates.c", "bool has_innate("),
    ("classes/innates.c", "void assign_innates("),
    ("classes/innates.c", "int get_innate_resistance("),
])
prelude = r'''
#include "world/properties.c"
#include "magic/spells.h"
#include "classes/necromancy.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int racial_shrug_data[LAST_RACE + 1];
int GET_CLASS(P_char ch, unsigned int cls) { return (ch->player.m_class & cls) != 0; }
char *str_dup(const char *text) { return ::strdup(text); }
void __free(void *p, const char *, int) { std::free(p); }
'''
driver = r'''
int main(int argc, char **argv) {
    assert(argc == 2);
    properties_count = load_properties(duris_properties);
    assert(properties_count > 0);
    qsort(duris_properties, properties_count, sizeof(struct property), property_comp);
    update_racial_shrug_data();
    assign_innates();
    char_data ch{}, peer{};
    ch.player.level = peer.player.level = 50;
    SET_POS(&ch, STAT_NORMAL + POS_STANDING);
    SET_POS(&peer, STAT_NORMAL + POS_STANDING);
    if (!strcmp(argv[1], "vampire")) {
        const int races[] = {RACE_DEMON, RACE_DEVIL, RACE_ILLITHID, RACE_BEHOLDER,
                            RACE_DRAGON, RACE_SHADOW, RACE_VAMPIRE, RACE_PVAMPIRE};
        const int values[] = {70, 70, 70, 85, 60, 60, 60, 40};
        for (int i = 0; i < 8; ++i) {
            ch.player.race = races[i];
            printf("race %d: shrug=%d resistance=%d\n", races[i],
                   racial_shrug_data[races[i]], get_innate_resistance(&ch));
            fflush(stdout);
            assert(racial_shrug_data[races[i]] == values[i]);
            assert(get_innate_resistance(&ch) == values[i] - 6);
        }
        // Shared keys for other vampire settings must retain their existing meaning.
        assert(!strcmp(race_names_table[RACE_PVAMPIRE].no_spaces, "Vampire"));
        assert(!strcmp(race_names_table[RACE_VAMPIRE].no_spaces, "Vampire"));
        assert(racial_shrug_data[RACE_PILLITHID] == 50);
    } else {
        ch.player.race = peer.player.race = RACE_GITHZERAI;
        group_list group{}; group.ch = &peer; ch.group = &group;
        assert(get_innate_resistance(&ch) == 19);
        auto *step = (struct property *)bsearch("innate.rrakkma.stepPct", duris_properties,
                         properties_count, sizeof(struct property), key_property_comp);
        assert(step);
        step->value = 0;
        printf("rrakkma with stepPct=0: resistance=%d\n", get_innate_resistance(&ch));
        fflush(stdout);
        assert(get_innate_resistance(&ch) == 14);
        step->value = 3;
        assert(get_innate_resistance(&ch) == 17);
        peer.in_room = 1;
        assert(get_innate_resistance(&ch) == 14);
    }
    for (int i = 0; i < properties_count; ++i) FREE(duris_properties[i].key);
}
'''
build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="racial-shrug-", dir=build) as directory:
    cpp, binary = Path(directory) / "harness.cpp", Path(directory) / "harness"
    cpp.write_text(prelude + macros + arrays + race_table + production + driver)
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-g", "-ffunction-sections",
                    "-fdata-sections", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-fno-pie", "-no-pie", "-I" + str(ROOT / "src"),
                    "-I/usr/include/libxml2", "-I/usr/include/mysql", str(cpp),
                    str(HARNESS_STUBS), "-Wl,--gc-sections", "-o", str(binary)], check=True)
    results = [subprocess.run([str(binary), case], cwd=ROOT).returncode
               for case in ("vampire", "rrakkma")]
    assert results == [0, 0], results
print("racial shrug and Rrakkma resistance passed")
