#!/usr/bin/env python3
"""Execute full-name innate dispatch and plane shift's cooldown on bad plane names."""
import re
import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, HARNESS_STUBS, extract_function, source

innates = source('innates.c').read_text()
table = re.search(r'const struct innate_data innates_data\[LAST_INNATE \+ 1\] = .*?\n};', innates, re.S).group()
table = re.sub(r'(\{ "[^"]+", )(\w+)( \})',
               lambda m: m[1] + (m[2] if m[2] in
                                 ('do_plane_shift', 'do_summon_orc', 'do_summon_warg')
                                 else 'NULL') + m[3], table)
macros = innates[innates.index('#define ADD_RACIAL_INNATE'):innates.index('float regen_factor')]
arrays = innates[innates.index('char racial_innates['):innates.index('extern const struct race_names')]
planes = re.search(r'const char \*planes_name\[\] = \{.*?\};', source('spells.c').read_text(), re.S).group()
prelude = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "cmd/interp.h"
#include "magic/spells.h"
#include "classes/necromancy.h"
#include <cassert>
#include <cstring>
#include <string>
Skill skills[MAX_SKILLS];
static int casts;
static std::string output, cast_arg;
static std::string summon_handler, summon_arg;
static bool epic_handled;
bool SanityCheck(P_char ch, const char *) { return ch != nullptr; }
int fill_word(char *) { return false; }
int checked_appendf(char *, size_t, const char *, ...) { return 0; }
int GET_CLASS(P_char ch, unsigned int cls) { return (ch->player.m_class & cls) != 0; }
bool epic_summon(P_char, char *) { return epic_handled; }
void do_list_innates(P_char, char *) {}
void do_plane_shift(P_char, char *, int);
void do_summon_orc(P_char, char *arg, int) { summon_handler = "horde"; summon_arg = arg; }
void do_summon_warg(P_char, char *arg, int) { summon_handler = "warg"; summon_arg = arg; }
void send_to_char(const char *text, P_char) { output += text; }
void CharWait(P_char, int) {}
int get_property(const char *, int fallback) { return fallback; }
affected_type *affect_to_char(P_char ch, affected_type *af) {
    auto *copy = new affected_type(*af);
    copy->next = ch->affected; ch->affected = copy; return copy;
}
void cast_plane_shift(int, P_char, char *arg, int, P_char, P_obj) { ++casts; cast_arg = arg ? arg : ""; }
'''
functions = [
    ('utility.c', 'int flag2idx('), ('utility.c', 'int strn_cmp('), ('sparser.c', 'char *skip_spaces('),
    ('interp.c', 'void argument_interpreter('), ('interp.c', 'bool is_abbrev('),
    ('interp.c', 'char *one_argument('), ('interp.c', 'int search_block('),
    ('affects.c', 'struct affected_type *get_spell_from_char('),
    ('affects.c', 'bool affected_by_spell('),
    ('innates.c', 'void get_property_format('), ('innates.c', 'int get_innate_property('),
    ('innates.c', 'static int innate_char_race('), ('innates.c', 'static int innate_unlock_level('),
    ('innates.c', 'bool has_innate('), ('innates.c', 'void assign_innates('),
    ('innates.c', 'bool can_use_innate('), ('innates.c', 'bool check_innate_time('),
    ('innates.c', 'void do_plane_shift(P_char ch'), ('innates.c', 'void do_innate(P_char ch,'),
]
production = '\n'.join(extract_function(file, signature) for file, signature in functions)
driver = r'''
static void innate(P_char ch, const char *line, int command = CMD_INNATE) {
    char args[256]; strcpy(args, line); output.clear(); summon_handler.clear();
    do_innate(ch, args, command);
}
int main() {
    assign_innates();
    char_data orc{};
    SET_POS(&orc, STAT_NORMAL + POS_STANDING);
    orc.player.race = RACE_ORC; orc.player.m_class = CLASS_WARRIOR; orc.player.level = 16;
    assert(has_innate(&orc, INNATE_SUMMON_HORDE) && has_innate(&orc, INNATE_SUMMON_WARG));
    // Both owned names start with summon; the complete name chooses the handler.
    innate(&orc, "summon warg");
    assert(summon_handler == "warg" && summon_arg.empty());
    innate(&orc, "summon horde");
    assert(summon_handler == "horde" && summon_arg.empty());
    innate(&orc, "  SuMmOn WaRg   Target words");
    assert(summon_handler == "warg" && summon_arg == "Target words");
    // Full names accept repeated spaces and tabs without changing trailing arguments.
    for (const char *line : {"summon  warg Target  words", "summon\twarg\tTarget  words",
                             "summon warg\t Target  words"}) {
        innate(&orc, line);
        assert(summon_handler == "warg" && summon_arg == "Target  words");
    }
    innate(&orc, "summon \t horde\t");
    assert(summon_handler == "horde" && summon_arg.empty());
    // Partial names and first-word abbreviations retain their original fallback.
    innate(&orc, "summon wargling");
    assert(summon_handler == "horde" && summon_arg == "wargling");
    innate(&orc, "sum target");
    assert(summon_handler == "horde" && summon_arg == "target");
    // The standalone summon command and its epic handler keep their existing routing.
    innate(&orc, "warg", CMD_SUMMON);
    assert(summon_handler == "warg" && summon_arg.empty());
    innate(&orc, "horde target", CMD_SUMMON);
    assert(summon_handler == "horde" && summon_arg == "target");
    epic_handled = true;
    innate(&orc, "warg", CMD_SUMMON);
    assert(summon_handler.empty());
    epic_handled = false;
    // An unavailable full name must not bypass ownership/unlock checks.
    orc.player.level = 15;
    assert(!has_innate(&orc, INNATE_SUMMON_WARG));
    innate(&orc, "summon warg");
    assert(summon_handler == "horde");
    orc.player.race = RACE_HUMAN;
    innate(&orc, "summon warg");
    assert(summon_handler.empty() && output.find("no such ability") != std::string::npos);
    char_data ch{};
    SET_POS(&ch, STAT_NORMAL + POS_STANDING);
    ch.player.race = RACE_HUMAN; ch.player.m_class = CLASS_DRUID; ch.player.level = 56;
    assert(has_innate(&ch, INNATE_PLANE_SHIFT));
    innate(&ch, "plane shift Air", CMD_SUMMON);
    assert(casts == 0 && !ch.affected && output.find("no such ability") != std::string::npos);
    // A mistyped plane is refused and does not start the cooldown.
    innate(&ch, "plane shift aiir");
    assert(casts == 0 && !ch.affected && output.find("Hell") != std::string::npos);
    for (const char *line : {"plane  shift aiir", "plane\tshift\taiir"}) {
        innate(&ch, line);
        assert(casts == 0 && !ch.affected && output.find("Hell") != std::string::npos);
    }
    // The full innate name passes the plane, not the name's second word, to the spell.
    innate(&ch, "plane shift Air");
    assert(casts == 1 && cast_arg == "Air" && ch.affected);
    // The first-word form still works and the cooldown now applies.
    innate(&ch, "plane hell");
    assert(casts == 1 && output.find("too tired") != std::string::npos);
    delete ch.affected;
    ch.affected = nullptr;
    innate(&ch, "plane hell");
    assert(casts == 2 && cast_arg == "hell" && ch.affected);
    delete ch.affected;
    ch.affected = nullptr;
    for (const char *line : {"plane  shift Air", "plane\tshift\tAir", "plane shift\t Air"}) {
        int previous_casts = casts;
        innate(&ch, line);
        assert(casts == previous_casts + 1 && cast_arg == "Air" && ch.affected);
        delete ch.affected;
        ch.affected = nullptr;
    }
    puts("Innates: full-name selection, abbreviation/summon fallback, ownership and plane cooldown passed.");
}
'''
build = ROOT / 'bin/tests'
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='plane-shift-', dir=build) as directory:
    cpp, binary = Path(directory) / 'harness.cpp', Path(directory) / 'harness'
    cpp.write_text(prelude + planes + '\n' + macros + arrays + table + production + driver)
    subprocess.run(['g++', '-std=c++20', '-g', '-ffunction-sections', '-fdata-sections',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                    '-I' + str(ROOT / 'src'), str(cpp), str(HARNESS_STUBS), '-Wl,--gc-sections',
                    '-lbsd', '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
