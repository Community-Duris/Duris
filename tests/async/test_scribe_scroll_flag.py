#!/usr/bin/env python3
"""Regression contract for scroll detection flag in add_scribe_data."""

from pathlib import Path
import shutil
import subprocess
import tempfile

from _paths import ROOT, SRC, extract_function
from _source_contract import function_body

source = (SRC / "classes" / "memorize.c").read_text(encoding="utf-8", errors="replace")
body = function_body(source, r"\bvoid\s+add_scribe_data\s*\(")
assert body is not None, "add_scribe_data definition is missing"

# Verify source assignment happens before tmp.flag evaluation
assign_obj_pos = body.index("tmp.source.obj = obj;")
flag_eval_pos = body.index("tmp.flag = (flag == 1 ?")
assert assign_obj_pos < flag_eval_pos, (
    "tmp.source.obj must be assigned before evaluating tmp.flag"
)
assert "tmp.source.obj->type == ITEM_SCROLL" in body, (
    "ITEM_SCROLL check must evaluate tmp.source.obj"
)

# Verify event_scribe guards s_data->source.obj
scribe_event_body = function_body(source, r"\bvoid\s+event_scribe\s*\(")
assert scribe_event_body is not None, "event_scribe definition is missing"
assert "if (s_data->flag == 2 && s_data->source.obj)" in scribe_event_body, (
    "event_scribe must guard s_data->source.obj before dereferencing"
)

print("scribe scroll flag source contract passed")

PRELUDE = r'''
#include "core/structs.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static scribing_data_type captured_data;
static bool event_added = false;

constexpr int SKILL_SCRIBE = 1;

void AddScribingAffect(P_char) {}
void event_scribe(P_char, P_char, P_obj, void *) {}

int add_event(void (*)(P_char, P_char, P_obj, void *), int, P_char, P_char, P_obj, void *,
              void *data, size_t size)
{
    assert(size == sizeof(scribing_data_type));
    std::memcpy(&captured_data, data, sizeof(scribing_data_type));
    event_added = true;
    return 1;
}

int GET_CHAR_SKILL(P_char, int) { return 100; }
'''

DRIVER = r'''
int main()
{
    char_data ch{};
    char_data teacher{};
    obj_data scroll{};
    scroll.type = ITEM_SCROLL;

    obj_data book{};
    book.type = ITEM_SPELLBOOK;

    // Test 1: Scribing from a scroll sets flag to 2
    event_added = false;
    std::memset(&captured_data, 0, sizeof(captured_data));
    add_scribe_data(1, &ch, &book, 1, &scroll, nullptr, nullptr);
    assert(event_added);
    assert(captured_data.flag == 2);
    assert(captured_data.source.obj == &scroll);

    // Test 2: Scribing from a spellbook sets flag to 1
    event_added = false;
    std::memset(&captured_data, 0, sizeof(captured_data));
    add_scribe_data(1, &ch, &book, 1, &book, nullptr, nullptr);
    assert(event_added);
    assert(captured_data.flag == 1);
    assert(captured_data.source.obj == &book);

    // Test 3: Learning from a teacher sets flag to 0
    event_added = false;
    std::memset(&captured_data, 0, sizeof(captured_data));
    add_scribe_data(1, &ch, &book, 0, nullptr, &teacher, nullptr);
    assert(event_added);
    assert(captured_data.flag == 0);
    assert(captured_data.source.teacher == &teacher);

    // Test 4: Scribing with NULL object sets flag to 1 without crashing
    event_added = false;
    std::memset(&captured_data, 0, sizeof(captured_data));
    add_scribe_data(1, &ch, &book, 1, nullptr, nullptr, nullptr);
    assert(event_added);
    assert(captured_data.flag == 1);
    assert(captured_data.source.obj == nullptr);

    std::puts("scribe scroll flag runtime harness passed");
    return 0;
}
'''

if shutil.which("g++"):
    with tempfile.TemporaryDirectory(prefix="duris-scribe-") as directory:
        directory_path = Path(directory)
        harness = directory_path / "harness.cpp"
        binary = directory_path / "harness"
        func_code = extract_function("classes/memorize.c", "void add_scribe_data(int spl,")
        harness.write_text(PRELUDE + func_code + DRIVER, encoding="utf-8")
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I" + str(ROOT / "src"),
                str(harness),
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True)
