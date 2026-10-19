#!/usr/bin/env python3
"""Every race owns exactly its own race bit in can_char_use_item() and
can_prime_class_use_item().

anti2_flags holds one bit for each of races 1-32. A race outside them used to
shift past the 32-bit flag word, undefined behaviour that x86 wraps, so a
firbolg (36) was judged as a grey elf (4). The production functions are
compiled with UBSan stopping at the first report.
"""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

PRELUDE = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>

[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
int real_mobile(const int) { return -1; }
int GET_CLASS(P_char ch, uint cls) { return ch->player.m_class & cls; }
int GET_PRIME_CLASS(P_char ch, uint cls) { return ch->player.m_class & cls; }
'''
DRIVER = r'''
int main()
{
    pc_only_data pc{};
    char_data ch{};
    ch.only.pc = &pc;
    ch.player.m_class = CLASS_WARRIOR;
    for (int race = RACE_NONE; race <= LAST_RACE; ++race) {
        ch.player.race = race;
        const bool illithid = race == RACE_ILLITHID;
        for (int bit = 0; bit < 32; ++bit) {
            obj_data deny{}, allow{};
            deny.anti2_flags = allow.anti2_flags = 1U << bit;
            allow.extra_flags = ITEM_ALLOWED_RACES;
            const bool own = race == bit + 1;
            assert(bool(can_char_use_item(&ch, &deny)) == (illithid || !own));
            assert(bool(can_char_use_item(&ch, &allow)) == (illithid || own));
            assert(bool(can_prime_class_use_item(&ch, &deny)) == (illithid || !own));
            assert(bool(can_prime_class_use_item(&ch, &allow)) == (illithid || own));
        }
    }
    puts("item race restriction runtime passed");
}
'''


def main():
    functions = ["static bool obj_race_bit_set", "int can_char_use_item(P_char ch, P_obj obj)",
                 "int can_prime_class_use_item(P_char ch, P_obj obj)"]
    harness = "\n".join([PRELUDE, *[extract_function("handler.c", sig) for sig in functions],
                         DRIVER])
    with tempfile.TemporaryDirectory(prefix="item-race-unit-") as directory:
        source, binary = Path(directory) / "race.cpp", Path(directory) / "race"
        source.write_text(harness)
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=undefined", "-fno-sanitize-recover=undefined", "-Isrc",
                        str(source), "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
