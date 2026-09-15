#!/usr/bin/env python3
"""Keep explicit hashed name-set selection deterministic."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source


NAME_GEN = source("mob/name_gen.c").read_text(encoding="utf-8")
GET_NAME = extract_function("mob/name_gen.c", "int get_name(")

# The hashed encrusted-item path relies on selector 9 rather than a fresh random
# name-file choice on each description regeneration.
assert "get_name(name, 9, id);" in source("item/objmisc.c").read_text(encoding="utf-8")
assert "if (SEX < 0 || SEX > 9)" in NAME_GEN
assert "\t\tSEX = number(0, 9);" in NAME_GEN

HARNESS = f"""
#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

static int forced_selector = 0;

#define SYLLABLES_PER_SECTION 100
#define SYLLABLE_LENGTH 100
#define NAME_LENGTH 20

int number(int, int)
{{
    return forced_selector;
}}

std::size_t strlcpy(char *destination, const char *source, std::size_t size)
{{
    const std::size_t length = std::strlen(source);
    if (size)
    {{
        const std::size_t copy_length = length < size - 1 ? length : size - 1;
        std::memcpy(destination, source, copy_length);
        destination[copy_length] = '\\0';
    }}
    return length;
}}

int checked_snprintf(char *buffer, std::size_t size, const char *format, ...)
{{
    va_list arguments;
    va_start(arguments, format);
    const int result = std::vsnprintf(buffer, size, format, arguments);
    va_end(arguments);
    return result;
}}

{GET_NAME}

int main()
{{
    char first[256]{{}}, second[256]{{}}, random_name[256]{{}};

    // A valid explicit selector must not be overwritten by the RNG.
    forced_selector = 0;
    assert(get_name(first, 9, 123456789ULL) == 9);
    assert(get_name(second, 9, 123456789ULL) == 9);
    assert(first[0] && !std::strcmp(first, second));

    // The default selector remains random for generated player/NPC names.
    forced_selector = 1;
    assert(get_name(random_name, -1, 123456789ULL) == 1);
    assert(random_name[0]);
    return 0;
}}
"""

with tempfile.TemporaryDirectory(prefix="duris-name-generation-") as temporary:
    root = Path(temporary)
    harness = root / "name_generation.cpp"
    binary = root / "name_generation"
    harness.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            str(harness),
            "-o",
            str(binary),
        ],
        cwd=Path(__file__).resolve().parents[2],
        check=True,
    )
    subprocess.run([str(binary)], cwd=Path(__file__).resolve().parents[2], check=True)

print("explicit hashed name-set selector regression passed")
