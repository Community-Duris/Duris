#!/usr/bin/env python3
"""Regression coverage for scroll source classification during scribing."""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, extract_function, source


MEMORIZE = source("memorize.c").read_text(encoding="utf-8")


def test_source_contracts() -> None:
    """Keep source assignment and event validation ahead of scroll dereferences."""
    assignment = MEMORIZE.index("tmp.source.obj = obj;")
    classification = MEMORIZE.index("tmp.flag = (flag == 1 ?", assignment)
    assert assignment < classification
    event = extract_function("memorize.c", "void event_scribe(P_char ch, P_char /*victim*/")
    assert "s_data && s_data->flag == 2 && !s_data->source.obj" in event


def test_scroll_source_is_captured_as_scroll() -> None:
    """Exercise the production scheduler payload rather than duplicating its logic."""
    prelude = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "world/events.h"
#include "magic/spells.h"
#include <cassert>
#include <cstring>

static scribing_data_type captured{};
static int scheduled = 0;

void event_scribe(P_char, P_char, P_obj, void *) {}
void AddScribingAffect(P_char) {}
int GET_CHAR_SKILL_P(P_char, int) { return 100; }
nevent_schedule_result add_event(event_func, int, P_char, P_char, P_obj, int,
                                 const void *data, int size)
{
    assert(size == sizeof(captured));
    captured = *static_cast<const scribing_data_type *>(data);
    ++scheduled;
    return {};
}
'''
    production = extract_function("memorize.c", "void add_scribe_data(")
    driver = r'''
int main()
{
    char_data character{};
    obj_data book{};
    obj_data scroll{};
    book.type = ITEM_SPELLBOOK;
    scroll.type = ITEM_SCROLL;

    add_scribe_data(1, &character, &book, 1, &scroll, nullptr, nullptr);
    assert(scheduled == 1);
    assert(captured.flag == 2 && captured.source.obj == &scroll);
    assert(captured.book == &book);

    add_scribe_data(1, &character, &book, 1, &book, nullptr, nullptr);
    assert(scheduled == 2);
    assert(captured.flag == 1 && captured.source.obj == &book);
}
'''
    with tempfile.TemporaryDirectory(prefix="scribe-scroll-") as directory:
        directory_path = Path(directory)
        source_path = directory_path / "harness.cpp"
        binary_path = directory_path / "harness"
        source_path.write_text(prelude + production + driver, encoding="utf-8")
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                f"-I{ROOT / 'src'}",
                str(source_path),
                "-o",
                str(binary_path),
            ],
            cwd=ROOT,
            check=True,
            timeout=120,
        )
        subprocess.run([str(binary_path)], cwd=ROOT, check=True, timeout=30)


if __name__ == "__main__":
    test_source_contracts()
    test_scroll_source_is_captured_as_scroll()
    print("Scribing scroll regression passed")
