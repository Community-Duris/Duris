#!/usr/bin/env python3
"""Source checks now; emit the actual-function C++ unit for a granted build later.

No compiler, subprocess, game, database or service is started by this script.
--emit-harness PATH writes a standalone unit containing the unmodified production
load_one_mine definition and explicit dependency doubles. Its eventual result is
a focused unit result, never normal-world/native gameplay qualification.
"""
import argparse
from pathlib import Path
import re


def actual_function(source):
    match = re.search(r"^bool load_one_mine\(int map\)\n\{", source, re.M)
    assert match, "production load_one_mine definition missing"
    start = source.index("{", match.start())
    depth = 0
    for index in range(start, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if not depth:
                return source[match.start():index + 1]
    raise AssertionError("unterminated production definition")


def contract(function):
    compact = re.sub(r"\s+", "", function)
    guard = ("if(start<0||end<0||start>end||start>top_of_world||end>top_of_world)")
    assert guard in compact, "missing complete resolved-room bounds guard"
    prefix = compact[:compact.index("P_objmine=read_object(")]
    assert prefix.index("economic_gameplay_authority::active()") < prefix.index("start_vnum=")
    for lookup in (
        'constintstart_vnum=mining_config_region_value(map,"start",mine_data[map].start);',
        'constintend_vnum=mining_config_region_value(map,"end",mine_data[map].end);',
        "constintstart=real_room(start_vnum);",
        "constintend=real_room(end_vnum);",
    ):
        assert lookup in prefix, lookup
    guarded = prefix[prefix.index(guard):]
    assert 'logit(LOG_DEBUG,"mines:region%sunavailable:' in guarded
    assert "mine_data[map].name,start_vnum,end_vnum,start,end,top_of_world);returnFALSE;}" in guarded
    for effect in ("read_object(", "number(", "world[", "invalid_mine_room(", "mine_friendly(",
                   "extract_obj(", "obj_to_room("):
        assert effect not in prefix, effect
    assert compact.index(guard) < compact.index("P_objmine=read_object(")
    assert compact.index(guard) < compact.index("to_room=number(start,end);")


def source_checks(function):
    contract(function)
    controls = []
    for clause in ("start < 0 || ", "end < 0 || ", "start > end || ",
                   "start > top_of_world || ", " || end > top_of_world"):
        assert function.count(clause) == 1
        controls.append(function.replace(clause, "", 1))
    controls.append(function.replace("\t\treturn FALSE;\n\t}\n\tP_obj mine", "\t\treturn TRUE;\n\t}\n\tP_obj mine", 1))
    controls.append(function.replace("\tconst int start_vnum", "\t(void)number(0, 1);\n\tconst int start_vnum", 1))
    controls.append(function.replace("\tconst int start_vnum", "\t(void)read_object(193, VIRTUAL);\n\tconst int start_vnum", 1))
    # A constructor inserted before the guard is rejected independently of
    # the ordinary constructor spelling used as the contract's boundary.
    for control in controls:
        assert control != function
        try:
            contract(control)
        except (AssertionError, ValueError):
            continue
        raise AssertionError("unsafe source negative control was accepted")
    return len(controls)


PREFIX = r'''
#include <array>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#define TRUE true
#define FALSE false
constexpr int VIRTUAL = 0, VOBJ_MINE = 193, VOBJ_GEMMINE = 434, LOG_DEBUG = 0;
struct object { int value[3]; const char *description; };
using P_obj = object *;
struct room { const char *name; int number; };
room room_rows[] = { {"first", 100}, {"middle", 101}, {"last", 102} };
room *world = room_rows;
const int top_of_world = 2;
#define ROOM_VNUM(index) (world[index].number)
struct region { const char *name; int start, end, type, mine_duration; };
region mine_data[] = { {"Surface Map", 100, 102, VOBJ_MINE, 9} };
int resolved_start, resolved_end, config_calls, lookup_calls, constructs, randoms;
int room_reads, placements, extractions, logs, wizard_logs;
bool accounting_active, missing_prototype;
object created;
char diagnostic[512];
void check(bool ok) { if (!ok) throw std::runtime_error("mining region guard regression"); }
namespace economic_gameplay_authority { bool active() { return accounting_active; } }
int mining_config_region_value(int map, const char *field, int fallback)
{
    check(map == 0); ++config_calls;
    if (!strcmp(field, "start")) return 100;
    if (!strcmp(field, "end")) return 102;
    return fallback;
}
int real_room(int vnum)
{
    ++lookup_calls;
    check(vnum == 100 || vnum == 102);
    return vnum == 100 ? resolved_start : resolved_end;
}
P_obj read_object(int type, int mode)
{
    check(mode == VIRTUAL && type == mine_data[0].type); ++constructs;
    return missing_prototype ? nullptr : &created;
}
void wizlog(int, const char *, ...) { ++wizard_logs; }
void logit(int kind, const char *format, ...)
{
    check(kind == LOG_DEBUG); ++logs;
    va_list args; va_start(args, format);
    vsnprintf(diagnostic, sizeof(diagnostic), format, args); va_end(args);
}
int number(int from, int to)
{
    ++randoms; check(from <= to);
    return from == 0 && to == 99 ? 75 : from;
}
bool invalid_mine_room(int index)
{
    ++room_reads; check(index >= 0 && index <= top_of_world); return false;
}
bool mine_friendly(int index)
{
    ++room_reads; check(index >= 0 && index <= top_of_world); return true;
}
void extract_obj(P_obj item) { check(item == &created); ++extractions; }
const char *str_dup(const char *text) { return text; }
void obj_to_room(P_obj item, int index)
{
    check(item == &created && index >= 0 && index <= top_of_world); ++placements;
}
'''


SUFFIX = r'''
void reset(int start, int end, int type = VOBJ_MINE)
{
    resolved_start = start; resolved_end = end; mine_data[0].type = type;
    config_calls = lookup_calls = constructs = randoms = room_reads = 0;
    placements = extractions = logs = wizard_logs = 0;
    accounting_active = missing_prototype = false;
    created = {{71, 72, 73}, "untouched"}; diagnostic[0] = 0;
}
int main()
{
    // Actual377 both-missing cut, either missing end, reversed, and either
    // out-of-range index must refuse without construction/RNG/room/publication.
    for (const auto &cut : std::array<std::array<int, 2>, 9>{{
             {{-1, -1}}, {{-1, 2}}, {{0, -1}}, {{2, 1}}, {{3, 3}},
             {{0, 3}}, {{-2, 2}}, {{INT_MIN, 2}}, {{0, INT_MAX}} }})
    {
        reset(cut[0], cut[1]);
        check(!load_one_mine(0));
        check(config_calls == 2 && lookup_calls == 2 && logs == 1);
        check(!constructs && !randoms && !room_reads && !placements && !extractions && !wizard_logs);
        check(created.value[0] == 71 && created.value[1] == 72 && created.value[2] == 73 &&
              !strcmp(created.description, "untouched"));
        char expected[512];
        snprintf(expected, sizeof(expected),
                 "mines: region Surface Map unavailable: room range 100-102 resolves to %d-%d (top 2).",
                 cut[0], cut[1]);
        check(!strcmp(diagnostic, expected));
    }
    // Zero and top are valid inclusive bounds; singleton and ordinary ranges
    // retain the actual production placement/quality/duration path.
    for (const auto &cut : std::array<std::array<int, 2>, 3>{{ {{0, 2}}, {{0, 0}}, {{2, 2}} }})
    {
        reset(cut[0], cut[1]); check(load_one_mine(0));
        check(config_calls == 3 && lookup_calls == 2 && constructs == 1 && randoms == 3 &&
              room_reads == 3 && placements == 1 && !extractions && logs == 1 && !wizard_logs);
        check(created.value[0] == 8 && created.value[1] == 0 && created.value[2] == 9);
    }
    reset(0, 2, VOBJ_GEMMINE); check(load_one_mine(0));
    check(constructs == 1 && randoms == 4 && placements == 1 && !extractions &&
          created.value[0] == 12 && created.value[1] == 100 && created.value[2] == 9);
    reset(0, 2); missing_prototype = true;
    check(!load_one_mine(0) && constructs == 1 && !randoms && !room_reads &&
          !placements && !extractions && wizard_logs == 1);
    reset(-1, -1); accounting_active = true;
    check(!load_one_mine(0) && !config_calls && !lookup_calls && !constructs &&
          !randoms && !room_reads && !placements && !extractions && !logs && !wizard_logs);
    puts("15 actual-function mining region cases passed");
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emit-harness", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    function = actual_function((root / "src/economy/mining.c").read_text())
    controls = source_checks(function)
    print(f"mining region source-order contract and {controls} negative controls passed; C++ UNRUN")
    if args.emit_harness:
        assert args.emit_harness.is_absolute(), "choose an absolute task output path"
        with args.emit_harness.open("x", encoding="utf-8", newline="\n") as output:
            output.write(PREFIX + "\n" + function + "\n" + SUFFIX)
        print(f"actual-function unit emitted: {args.emit_harness}; no compiler or executable run")


if __name__ == "__main__":
    main()
