#!/usr/bin/env python3
"""Execute the legacy command: replace a whole short and reject malformed stat input."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

functions = '\n'.join(extract_function('actwiz.c', signature) for signature in (
    'void sa_byteCopy(', 'void sa_shortCopy(', 'void sa_intCopy(', 'void sa_ageCopy(',
    'void do_setattr('))
harness = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "sql/sql.h"
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <string>
static char_data victim{};
static std::string sent;
void send_to_char(const char *text, P_char) { sent += text; }
void wizlog(int, const char *, ...) {}
void logit(const char *, const char *, ...) {}
void sql_log(P_char, const char *, const char *, ...) {}
void do_setship(P_char, char *) {}
void balance_affects(P_char) {}
P_char get_char_vis(P_char, const char *) { return &victim; }
bool isname(const char *a, const char *b) { return !strcasecmp(a,b); }
char *one_argument(const char *argument, char *out)
{
    while (*argument == ' ') ++argument;
    while (*argument && *argument != ' ') *out++ = *argument++;
    *out = 0;
    while (*argument == ' ') ++argument;
    return const_cast<char *>(argument);
}
''' + functions + r'''
int main()
{
    char_data staff{};
    char name[] = "Staff";
    staff.player.name = name;
    const char *stats[] = {"str", "dex", "agi", "con", "pow", "int", "wis", "cha", "karma", "luck"};
    for (int index = 0; index < 10; ++index) {
        auto run = [&](const char *value) {
            char command[160];
            snprintf(command, sizeof(command), "Fixture %s %s", stats[index], value);
            sent.clear();
            do_setattr(&staff, command, 0);
        };
        victim.base_stats[index] = -1; // a byte-only store leaves the old high byte behind
        for (const char *value : {"130", "140", "-32768", "32767"}) {
            run(value);
            assert(victim.base_stats[index] == std::atoi(value));
            assert(sent == "OK.\n");
        }
        for (const char *value : {"32768", "-32769", "65536", "4294967426",
                                 "999999999999999999999999", "1x", "1.5", "--1"}) {
            run(value);
            assert(victim.base_stats[index] == 32767);
            assert(sent == "Enter a whole number from -32768 to 32767.\r\n");
        }
    }
    puts("setattr writes all ten complete signed shorts and rejects malformed/overflowing input");
}
'''

with tempfile.TemporaryDirectory(prefix='duris-setattr-') as td:
    source, binary = Path(td) / 'test.cpp', ROOT / 'bin/tests/setattr_stats'
    source.write_text(harness)
    binary.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                    '-ffunction-sections', '-fdata-sections', '-Isrc', '-I/usr/include/mysql', str(source), '-Wl,--gc-sections', '-o', str(binary)],
                   cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
