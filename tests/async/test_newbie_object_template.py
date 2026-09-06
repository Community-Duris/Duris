#!/usr/bin/env python3
"""Synthetic runtime regression for boot-only newbie object-template parsing.

Executes extracted production db.c parser/cache/publisher, fread_string, and
isname against a temporary object file. Memory pool, identity, event, conversion,
and procedure-library endpoints are fixtures; no database or game is started.
Linux g++, ASan/UBSan and the repository headers are required.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/mm.h"
#include "cmd/interp.h"
#include "world/object_template.h"
#include "item/objmisc.h"
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

FILE *obj_f = nullptr;
static index_data indexes[4]{};
P_index obj_index = indexes;
int top_of_objt = 3;
P_obj object_list = nullptr;
mm_ds *dead_obj_pool = nullptr;
static unsigned pool_allocations, uid_allocations, proc_calls, conversion_calls;
static unsigned periodic_calls, event_calls, duplicate_strings;
static std::unordered_set<void *> allocations;
static std::unordered_map<int, object_template> starter_object_templates;
void logit(const char *, const char *, ...) {}
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
void *__malloc(size_t size, const char *, const char *, int)
{
    void *result = calloc(1, size); assert(result); allocations.insert(result); return result;
}
// Match the checked allocator's non-null contract: the empty fread_string case
// must be guarded by read_template_string, not hidden by a permissive free stub.
void __free(void *ptr, const char *, int)
{
    assert(ptr && allocations.erase(ptr) == 1); free(ptr);
}
char *str_dup(const char *text)
{
    ++duplicate_strings;
    char *result = static_cast<char *>(__malloc(strlen(text) + 1, nullptr, nullptr, 0));
    strcpy(result, text); return result;
}
void *_mm_get(mm_ds *, const char *, int)
{
    ++pool_allocations; return __malloc(sizeof(obj_data), nullptr, nullptr, 0);
}
unsigned long long persistence_next_item_uid() { return 1000 + ++uid_allocations; }
void required_fscanf_impl(FILE *stream, int expected, const char *, int, const char *format, ...)
{
    va_list args; va_start(args, format);
    const int actual = vfscanf(stream, format, args); va_end(args); assert(actual == expected);
}
int real_object(int vnum)
{
    for (int nr = 0; nr <= top_of_objt; ++nr)
        if (indexes[nr].virtual_number == vnum) return nr;
    return -1;
}
int strn_cmp(const char *left, const char *right, uint count)
{
    if (!left || !right) return left == right ? 0 : left ? 1 : -1;
    return strncasecmp(left, right, count);
}
int number(int low, int high) { assert(low == -4 && high == 4); return 0; }
static void assert_published(P_obj obj)
{
    assert(obj && obj->obj_uid && obj_index[obj->R_num].number > 0);
    P_obj at = object_list;
    while (at && at != obj) at = at->next;
    assert(at == obj && OBJ_NOWHERE(obj));
}
int item_switch(P_obj obj, P_char, int cmd, char *)
{
    assert_published(obj); assert(cmd == CMD_SET_PERIODIC); ++periodic_calls; return 1;
}
int proclibObj_add(P_obj obj, char *name, char *args)
{
    assert_published(obj);
    assert(strcmp(name, "fixture") == 0 && strcmp(args, "synthetic args") == 0);
    ++proc_calls; SET_BIT(obj->extra_flags, ITEM_PROCLIB);
    obj_index[obj->R_num].func.obj = item_switch;
    return 0; // consumed procedure descriptions are not ordinary extra descriptions
}
void event_object_proc(P_char, P_char, P_obj, void *) {}
void event_random_exit(P_char, P_char, P_obj, void *) {}
nevent_schedule_result add_event(event_func fn, int delay, P_char, P_char, P_obj obj,
                                 int, const void *, int)
{
    assert_published(obj);
    assert((fn == event_object_proc && delay == PULSE_MOBILE) ||
           (fn == event_random_exit && delay == 3));
    ++event_calls;
    return {nevent_schedule_status::scheduled, {}};
}
int FillMasterSpellBook(P_obj) { abort(); }
void convertObj(P_obj obj) { assert_published(obj); ++conversion_calls; }
'''

DRIVER = r'''
static void write_record(int nr, const char *name, int type, int first_value,
                         unsigned long wear, bool rich)
{
    indexes[nr].virtual_number = 100 + nr;
    indexes[nr].pos = ftell(obj_f);
    std::ostringstream record;
    record << name << "~\nA &+Csynthetic item~\nA synthetic item lies here.~\n~\n";
    record << type << " 3 0 0 6 0 " << ITEM_PROCLIB << ' ' << wear << " 19 "
           << CLASS_NECROMANCER << " 23\n";
    record << first_value << " 2 3 4 5 6 7 8\n17 1234 88\n1 2 4 8\n";
    if (rich)
        record << "B5\n4294967296\nE\n_proclib_fixture~\nsynthetic args~\n"
                  "E\nmark~\nAn independent mark.~\nA\n4 -2\nA\n5 3\nT\n9 10 11 12\n";
    else
        record << "S\n";
    const std::string text = record.str();
    assert(fwrite(text.data(), 1, text.size(), obj_f) == text.size());
}
static void assert_inert()
{
    assert(pool_allocations == 0 && uid_allocations == 0 && object_list == nullptr);
    assert(proc_calls == 0 && periodic_calls == 0 && event_calls == 0 && conversion_calls == 0);
    assert(duplicate_strings == 0 && allocations.empty());
    for (const auto &index : indexes)
        assert(index.number == 0 && !index.keys && !index.desc1 && !index.desc2 &&
               !index.desc3 && !index.func.obj);
}
static void cleanup()
{
    while (object_list)
    {
        P_obj obj = object_list; object_list = obj->next;
        while (obj->ex_description)
        {
            auto *desc = obj->ex_description; obj->ex_description = desc->next;
            if (desc->keyword) FREE(desc->keyword);
            if (desc->description) FREE(desc->description);
            FREE(desc);
        }
        FREE(obj);
    }
    for (auto &index : indexes)
    {
        if (index.keys) FREE(index.keys);
        if (index.desc1) FREE(index.desc1);
        if (index.desc2) FREE(index.desc2);
        if (index.desc3) FREE(index.desc3);
    }
    assert(allocations.empty());
}
int main()
{
    obj_f = tmpfile(); assert(obj_f);
    write_record(0, "BAG random_exit", ITEM_CONTAINER, 12, ITEM_TAKE, true);
    write_record(1, "BOOMERANG", ITEM_WEAPON, WEAPON_2HANDSWORD, ITEM_HOLD, false);
    write_record(2, "ARMOR", ITEM_ARMOR, 0, ITEM_TAKE, false);
    write_record(3, "SWITCH", ITEM_SWITCH, 0, 0, false);
    fflush(obj_f);
    for (int nr = 0; nr < 4; ++nr) assert(cache_object_template(100 + nr));
    assert(!cache_object_template(999) && !find_object_template(999));
    assert_inert();
    const auto *bag = find_object_template(100);
    assert(bag && bag->name == "bag random_exit" && bag->action_description.empty());
    assert(bag->short_description == "A &+Csynthetic item&n");
    assert(bag->material == 3 && bag->craftsmanship == 6);
    assert(bag->weight == 17 && bag->cost == 1234 && bag->condition == 88);
    assert(bag->extra2_flags == 19 && bag->anti2_flags == 23);
    assert(IS_SET(bag->anti_flags, CLASS_NECROMANCER) && IS_SET(bag->anti_flags, CLASS_THEURGIST));
    assert(!IS_SET(bag->extra_flags, ITEM_PROCLIB));
    assert(bag->bitvector == 1 && bag->bitvector2 == 2 && bag->bitvector3 == 4 &&
           bag->bitvector4 == 8 && bag->bitvector5 == 4294967296UL);
    assert(bag->affected[0].location == 4 && bag->affected[0].modifier == -2);
    assert(bag->affected[1].location == 5 && bag->affected[1].modifier == 3);
    assert(bag->trap_eff == 9 && bag->trap_dam == 10 && bag->trap_charge == 11 && bag->trap_level == 12);
    assert(bag->descriptions.size() == 2 && bag->descriptions[0].keyword == "_proclib_fixture");
    assert(IS_SET(bag->wear_flags, ITEM_TAKE) && IS_SET(bag->wear_flags, ITEM_HOLD) &&
           IS_SET(bag->wear_flags, ITEM_ATTACH_BELT));
    const auto *weapon = find_object_template(101);
    assert(weapon && IS_SET(weapon->extra_flags, ITEM_CAN_THROW1) &&
           IS_SET(weapon->extra_flags, ITEM_CAN_THROW2) && IS_SET(weapon->extra_flags, ITEM_RETURNING) &&
           IS_SET(weapon->extra_flags, ITEM_TWOHANDS));
    assert(IS_SET(weapon->wear_flags, ITEM_HOLD) && IS_SET(weapon->wear_flags, ITEM_TAKE));
    assert(find_object_template(102)->type == ITEM_WORN);
    assert(weapon->bitvector5 == 0 && weapon->affected[0].location == 0 && weapon->trap_eff == 0);

    // No readable source remains: cached lookup, repeated cache hit and every
    // publication below must work without reparsing an object file.
    fclose(obj_f); obj_f = nullptr;
    assert(cache_object_template(100) && find_object_template(100) == bag);
    assert(!find_object_template(999));
    assert_inert();
    P_obj first = instantiate_object_template(*bag);
    assert(pool_allocations == 1 && uid_allocations == 1 && indexes[0].number == 1);
    assert(first->action_description == nullptr && indexes[0].desc3 == nullptr);
    assert(first->bitvector5 == 4294967296UL && first->value[0] == 12 && first->value[7] == 8);
    assert(first->trap_eff == 9 && first->trap_level == 12 && first->affected[0].modifier == -2);
    assert(first->ex_description && !first->ex_description->next);
    assert(strcmp(first->ex_description->keyword, "mark") == 0);
    assert(proc_calls == 1 && periodic_calls == 1 && event_calls == 2 && conversion_calls == 1);
    P_obj second = instantiate_object_template(*find_object_template(100));
    assert(first != second && first->obj_uid != second->obj_uid && indexes[0].number == 2);
    assert(object_list == second && second->next == first && first->prev == second);
    assert(first->name == second->name && first->description == second->description &&
           first->short_description == second->short_description);
    assert(second->action_description == nullptr);
    assert(first->ex_description != second->ex_description &&
           first->ex_description->keyword != second->ex_description->keyword &&
           first->ex_description->description != second->ex_description->description);
    first->ex_description->description[0] = 'X'; first->value[0] = 99;
    assert(second->ex_description->description[0] == 'A' && second->value[0] == 12);
    assert(bag->descriptions[1].description[0] == 'A' && bag->value[0] == 12);
    assert(proc_calls == 2 && periodic_calls == 2 && event_calls == 4 && conversion_calls == 2);
    assert(!IS_SET(bag->extra_flags, ITEM_PROCLIB));
    // Cold parsing preserves canonical indexed text while consuming the file's
    // four strings, and still reads fresh numeric data without publishing.
    const auto allocation_count = allocations.size();
    obj_f = tmpfile(); assert(obj_f);
    write_record(0, "replacement file keywords", ITEM_CONTAINER, 21, ITEM_TAKE, true);
    fflush(obj_f);
    const auto cold = parse_object_template(0);
    assert(cold.name == "bag random_exit" && cold.value[0] == 21);
    assert(cold.action_description.empty() && cold.bitvector5 == 4294967296UL);
    assert(allocations.size() == allocation_count && indexes[0].number == 2);
    assert(pool_allocations == 2 && uid_allocations == 2 && proc_calls == 2 && event_calls == 4);
    assert(cold.name.c_str() != indexes[0].keys);
    fclose(obj_f); obj_f = nullptr;
    for (int vnum : {101, 102, 103}) instantiate_object_template(*find_object_template(vnum));
    assert(pool_allocations == 5 && uid_allocations == 5 && conversion_calls == 5);
    assert(indexes[3].func.obj == item_switch && periodic_calls == 3 && event_calls == 5);
    cleanup(); starter_object_templates.clear();
    puts("newbie object template runtime: ok");
}
'''


def main() -> int:
    functions = [
        extract_function("db.c", "char *fread_string(FILE *fl)"),
        extract_function("handler.c", "bool isname(const char *str, const char *namelist)"),
        extract_function("db.c", "void skip_fread(FILE *fl)"),
        extract_function("db.c", "std::string read_template_string(FILE *file,"),
        extract_function("db.c", "object_template parse_object_template(int nr)"),
        extract_function("db.c", "bool cache_object_template(int vnum)"),
        extract_function("db.c", "const object_template *find_object_template(int vnum)"),
        extract_function("db.c", "P_obj instantiate_object_template(const object_template &prototype)"),
    ]
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "newbie_object_template.cpp"
        binary = Path(directory) / "newbie_object_template"
        source.write_text("\n".join([PRELUDE, *functions, DRIVER]), encoding="utf-8")
        subprocess.run([
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
            "-fsanitize=address,undefined", "-Isrc", str(source), "-o", str(binary),
        ], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)
    print("All newbie object template checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
