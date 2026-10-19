#!/usr/bin/env python3
"""Exercise create_recipe() validation, rejection criteria, and delivery in isolation."""
import subprocess
import tempfile
from pathlib import Path
from _paths import ROOT, extract_function

create_recipe_body = extract_function("drannak.c", "bool create_recipe(")

FIXTURE = r'''
#include "core/structs.h"
#include "core/utils.h"
#include "economy/tradeskill.h"
#include "world/db.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static index_data obj_index_arr[2]{};
P_index obj_index = obj_index_arr;

static std::vector<P_obj> delivered;
static bool target_available = true;

bool crafting_recipe_target_is_available(P_obj)
{
    return target_available;
}

void crafting_configure_recipe_scroll(P_obj, P_obj)
{
}

void debug(const char *, ...)
{
}

int itemvalue(P_obj)
{
    return 100;
}

P_obj read_object(int vnum, int mode)
{
    assert(vnum == 400210 && mode == VIRTUAL);
    auto *obj = new obj_data{};
    obj->type = ITEM_SCROLL;
    obj->short_description = strdup("a recipe scroll for");
    obj->str_mask = STRUNG_DESC2;
    return obj;
}

void obj_to_char(P_obj obj, P_char)
{
    delivered.push_back(obj);
}

char *str_dup(const char *text)
{
    return strdup(text);
}

void __free(void *ptr, const char *, int)
{
    free(ptr);
}

int checked_snprintf_at(const char *, int, char *buffer, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int res = vsnprintf(buffer, size, format, args);
    va_end(args);
    return res;
}
#define checked_snprintf(buf, sz, fmt, ...) checked_snprintf_at(__FILE__, __LINE__, buf, sz, fmt, ##__VA_ARGS__)

''' + create_recipe_body + r'''

static obj_data make_valid_target()
{
    obj_data obj{};
    obj.R_num = 0;
    obj.type = ITEM_WEAPON;
    obj.short_description = const_cast<char *>("a sharp iron blade");
    return obj;
}

int main()
{
    char_data ch{};
    ch.player.name = const_cast<char *>("Tester");

    // 1. Valid target returns true and delivers scroll to character.
    {
        obj_data target = make_valid_target();
        target_available = true;
        delivered.clear();
        assert(create_recipe(&ch, &target));
        assert(delivered.size() == 1);
        assert(delivered[0]->type == ITEM_SCROLL);
        free(delivered[0]->short_description);
        delete delivered[0];
    }

    // 2. Non-empty container is rejected.
    {
        obj_data target = make_valid_target();
        target.type = ITEM_CONTAINER;
        obj_data inside{};
        target.contains = &inside;
        delivered.clear();
        assert(!create_recipe(&ch, &target));
        assert(delivered.empty());
    }

    // 3. Non-empty storage is rejected.
    {
        obj_data target = make_valid_target();
        target.type = ITEM_STORAGE;
        obj_data inside{};
        target.contains = &inside;
        delivered.clear();
        assert(!create_recipe(&ch, &target));
        assert(delivered.empty());
    }

    // 4. Restricted affects are rejected (e.g. sanctuary, stone skin, etc.).
    {
        for (uint64_t aff : {AFF_STONE_SKIN, AFF_BIOFEEDBACK, AFF_SNEAK, AFF_HIDE})
        {
            obj_data target = make_valid_target();
            target.bitvector = aff;
            delivered.clear();
            assert(!create_recipe(&ch, &target));
            assert(delivered.empty());
        }
        for (uint64_t aff : {AFF4_SANCTUARY, AFF4_VAMPIRE_FORM, AFF4_HOLY_SACRIFICE})
        {
            obj_data target = make_valid_target();
            target.bitvector4 = aff;
            delivered.clear();
            assert(!create_recipe(&ch, &target));
            assert(delivered.empty());
        }
    }

    // 5. Combat pulse and spell pulse modifiers are rejected.
    {
        obj_data target = make_valid_target();
        target.affected[0].location = APPLY_COMBAT_PULSE;
        delivered.clear();
        assert(!create_recipe(&ch, &target));
        assert(delivered.empty());

        target = make_valid_target();
        target.affected[1].location = APPLY_SPELL_PULSE;
        delivered.clear();
        assert(!create_recipe(&ch, &target));
        assert(delivered.empty());
    }

    // 6. Unavailable crafting target is rejected.
    {
        obj_data target = make_valid_target();
        target_available = false;
        delivered.clear();
        assert(!create_recipe(&ch, &target));
        assert(delivered.empty());
    }

    std::puts("PASS create_recipe validation, rejection criteria, and delivery");
    return 0;
}
'''


def main() -> None:
    build_dir = ROOT / "bin/tests"
    build_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="recipe-rejection-", dir=build_dir) as td:
        cpp = Path(td) / "test.cpp"
        binary = Path(td) / "test"
        cpp.write_text(FIXTURE, encoding="utf-8")
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-fsanitize=address,undefined",
                "-g",
                "-Isrc",
                str(cpp),
                "-o",
                str(binary),
            ],
            cwd=ROOT,
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
