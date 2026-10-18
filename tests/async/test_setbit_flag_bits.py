#!/usr/bin/env python3
"""setbit sets a flag bit only where its 32-bit field has one.

ac_bitCopy() sets bit N of a 32-bit field. `setbit obj <item> race firbolg 1` asked
it for bit 35, an undefined shift that x86 wraps to bit 3, so the item denied grey
elves. The real setbit_parseTable() and ac_bitCopy() run against an item race list
built as setbit_obj() builds it, with UBSan stopping at its first report.
"""
from pathlib import Path
import re
import subprocess
import tempfile
from _paths import ROOT, SRC, extract_function

source = (SRC / "actset.c").read_text()
# The file's own flag macros and table type, from bad_on_off to the SetBitTable typedef.
declarations = source[source.index("char bad_on_off"):source.index("/* Private Interface */")]
functions = [extract_function("actset.c", signature) for signature in (
    "static int ac_strcasecmp(const char *str1", "static void ac_bitCopy(void *where",
    "static void setbit_parseTable(P_char ch")]

character_table = source[source.index('static void setbit_char(P_char ch, char *name'):source.index('\n\tP_char ppl;', source.index('static void setbit_char(P_char ch, char *name'))]
short_fields = ('mana', 'vit', 'mxvit', 'ac', 'align', 'timer', 'prestige', 'wimpy', 'aggr',
                'weight', 'height', 'carryn', 'prompt')
short_entries = {name: re.search(r'\{ "' + name + r'",[^\n]+\}', character_table).group()
                 for name in short_fields}

HARNESS = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include <cassert>
#include <cerrno>
#include <climits>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <string>

''' + declarations + r'''
static std::string sent;
void send_to_char(const char *message, P_char) { sent += message; }
bool is_number(char *value) { return value[0] && value[strspn(value, "-0123456789")] == 0; }
int checked_snprintf_at(const char *, int, char *, size_t, const char *, ...) { return 0; }
int checked_snprintf(char *, size_t, const char *, ...) { return 0; }
static void setbit_printOutTable(P_char, SetBitTable *, int) {}
static void setbit_printOutSubTable(P_char, const char **, int) {}
static void setbit_syntax(P_char, int) {}
static void ac_tongueCopy(void *, int, char *, int, int) {}
static void ac_skillCopy(void *, int, char *, int, int) {}
''' + source[source.index('#define MAKE_COPY_FUNCTION(Type)'):source.index('/* Macro to facilitate making of general copy functions */\n// special case')] + r'''
MAKE_COPY_FUNCTION(short)
MAKE_COPY_FUNCTION(sh_int)
MAKE_COPY_FUNCTION(ush_int)
MAKE_COPY_FUNCTION(int)
static int ac_strcasecmp(const char *, const char *);
''' + "\n".join(functions) + r'''
int main()
{
    static race_names races[LAST_RACE + 2]{};
    static char names[LAST_RACE + 1][16];
    for (int race = 1; race <= LAST_RACE; ++race) {
        snprintf(names[race], sizeof(names[race]), "race%d", race);
        races[race].no_spaces = names[race];
    }
    SetBitTable table[] = {
        { "race", int(offsetof(obj_data, anti2_flags)), (const char **)&races[1], ac_bitCopy,
          sizeof(race_names), int(offsetof(race_names, no_spaces)) },
        { "extra", int(offsetof(obj_data, extra_flags)), nullptr, ac_bitCopy },
    };
    obj_data obj{};
    char race[] = "race", extra[] = "extra";
    auto set = [&](char *flag, const char *value, int on_off) {
        char text[16];
        snprintf(text, sizeof(text), "%s", value);
        sent.clear();
        setbit_parseTable(nullptr, &obj, table, 2, flag, text, on_off, SETBIT_OBJ);
    };
    // Races 1-32 own bits 0-31, on and off; a race above 32 has no bit.
    set(race, "race1", 1);
    set(race, "race32", 1);
    assert(obj.anti2_flags == (1U | 1U << 31) && sent.empty());
    set(race, "race36", 1);
    assert(obj.anti2_flags == (1U | 1U << 31) && sent == "That field has no bit for that value.\r\n");
    set(race, "race32", 0);
    assert(obj.anti2_flags == 1U && sent.empty());
    // A flag given by number reaches only bits 0-31 as well.
    for (const char *number : {"-1", "32", "40"}) {
        set(extra, number, 1);
        assert(obj.extra_flags == 0 && sent == "That field has no bit for that value.\r\n");
    }
    set(extra, "31", 1);
    assert(obj.extra_flags == 1U << 31 && sent.empty());
    // Use the production character table's ten stat entries and its actual short copy.
#define ABOFFSET(Field) int(offsetof(char_data, base_stats) + offsetof(stat_data, Field))
    SetBitTable stat_table[] = {
''' + source[source.index('\t\t{ "str", ABOFFSET'):source.index('\n\t\t{ "tstr",')] + r'''
    };
#undef ABOFFSET
    char_data ch{};
    for (auto &entry : stat_table) {
        for (const char *value : {"130", "140", "-32768", "32767"}) {
            char input[80];
            snprintf(input, sizeof(input), "%s", value);
            sent.clear();
            setbit_parseTable(nullptr, &ch, stat_table, 10,
                              const_cast<char *>(entry.sb_flag), input, 0, SETBIT_CHAR);
            short stored = 0;
            memcpy(&stored, reinterpret_cast<char *>(&ch) + entry.sb_offset, sizeof(stored));
            assert(stored == std::atoi(value) && sent.empty());
        }
        for (const char *value : {"32768", "-32769", "65536", "4294967426",
                                 "9999999999999999999999999999", "1x", "1.5", "--1", ""}) {
            char input[80];
            snprintf(input, sizeof(input), "%s", value);
            sent.clear();
            setbit_parseTable(nullptr, &ch, stat_table, 10,
                              const_cast<char *>(entry.sb_flag), input, 0, SETBIT_CHAR);
            short stored = 0;
            memcpy(&stored, reinterpret_cast<char *>(&ch) + entry.sb_offset, sizeof(stored));
            assert(stored == 32767 && sent == "Enter a whole number from -32768 to 32767.\r\n");
        }
    }
    // Additional production entries include signed typedefs and unsigned destinations.
#define PLOFFSET(Field) int(offsetof(char_data, player) + offsetof(char_player_data, Field))
#define POOFFSET(Field) int(offsetof(char_data, points) + offsetof(char_point_data, Field))
#define SPOFFSET(Field) int(offsetof(char_data, specials) + offsetof(char_special_data, Field))
#define PCOFFSET(Field) int(offsetof(pc_only_data, Field))
    SetBitTable short_table[] = {
''' + ',\n'.join(short_entries.values()) + r'''
    };
    pc_only_data pc{};
    for (auto &entry : short_table) {
        const std::string name = entry.sb_flag;
        const bool pc_field = name == "prestige" || name == "wimpy" ||
                              name == "aggr" || name == "prompt";
        const bool unsigned_field = name == "weight" || name == "height" ||
                                    name == "carryn" || name == "prompt";
        void *target = pc_field ? static_cast<void *>(&pc) : &ch;
        char *field = static_cast<char *>(target) + entry.sb_offset;
        auto write = [&](const char *value) {
            char input[80];
            snprintf(input, sizeof(input), "%s", value);
            sent.clear();
            setbit_parseTable(nullptr, target, &entry, 1,
                              const_cast<char *>(entry.sb_flag), input, 0, SETBIT_CHAR);
        };
        const auto unsigned_values = {"0", "32768", "40000", "65535"};
        const auto signed_values = {"-32768", "128", "32767"};
        for (const char *value : unsigned_field ? unsigned_values : signed_values) {
            write(value);
            if (unsigned_field) {
                ush_int stored;
                memcpy(&stored, field, sizeof(stored));
                assert(stored == std::atoi(value));
            } else {
                sh_int stored;
                memcpy(&stored, field, sizeof(stored));
                assert(stored == std::atoi(value));
            }
            assert(sent.empty());
        }
        unsigned char before[sizeof(short)];
        memcpy(before, field, sizeof(before));
        for (const char *value : {"65536", "-32769", "9999999999999999999999999999",
                                 "1x", "1.5", "--1", ""}) {
            write(value);
            assert(!memcmp(before, field, sizeof(before)) && !sent.empty());
        }
        write(unsigned_field ? "-1" : "32768");
        assert(!memcmp(before, field, sizeof(before)) && !sent.empty());
    }
    // The room zone shares the unsigned-short representation.
#undef OFFSET
#define OFFSET(Field) int(offsetof(room_data, Field))
    SetBitTable zone_entry =
''' + re.search(r'\{ "zone", OFFSET\(zone\),[^\n]+\}', source).group() + r''';
    room_data room{};
    for (const char *value : {"32768", "65535", "65536", "-1"}) {
        char input[16];
        snprintf(input, sizeof(input), "%s", value);
        sent.clear();
        setbit_parseTable(nullptr, &room, &zone_entry, 1,
                          const_cast<char *>(zone_entry.sb_flag), input, 0, SETBIT_ROOM);
        if (std::atoi(value) >= 32768 && std::atoi(value) <= 65535)
            assert(room.zone == std::atoi(value) && sent.empty());
        else
            assert(room.zone == 65535 && !sent.empty());
    }
#undef DIR
#define DIR(Direction, Field) int(offsetof(room_direction_data, Field))
    SetBitTable key_table[] = {
''' + ',\n'.join(re.findall(r'\{ "[a-z]+key", DIR\([^\n]+\}', source)) + r'''
    };
    for (auto &entry : key_table) {
        room_direction_data exit{};
        exit.key = -1;
        for (const char *value : {"123", "35824", "40000", "-2147483648", "2147483647", "-1"}) {
            char input[16];
            snprintf(input, sizeof(input), "%s", value);
            sent.clear();
            setbit_parseTable(nullptr, &exit, &entry, 1,
                              const_cast<char *>(entry.sb_flag), input, 0, SETBIT_DIR);
            assert(exit.key == std::atoi(value) && sent.empty());
        }
        for (const char *invalid : {"-", "1x", "2147483648", "-2147483649",
                                    "99999999999999999999999"}) {
            char input[80];
            snprintf(input, sizeof(input), "%s", invalid);
            sent.clear();
            setbit_parseTable(nullptr, &exit, &entry, 1,
                              const_cast<char *>(entry.sb_flag), input, 0, SETBIT_DIR);
            assert(exit.key == -1 && !sent.empty());
        }
    }
    puts("setbit validates short destinations, full-width exit keys and flag bounds");
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix="duris-setbit-bits-") as directory:
        harness, binary = Path(directory) / "harness.cpp", Path(directory) / "harness"
        harness.write_text(HARNESS)
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=undefined",
                        "-Isrc", str(harness), "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
