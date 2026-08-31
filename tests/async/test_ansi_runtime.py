#!/usr/bin/env python3
"""Runtime regression coverage for ANSI parsing, rendering, and gradients."""

from _paths import SRC
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "net/ansi.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

int main()
{
	int tests = 0;
	char output[MAX_STRING_LENGTH];

	// 1. Plain rendering removes Duris color state while preserving Unicode.
	{
		AnsiString value("&+rRed &-bBack &=gWBoth&n \xc3\xa9");
		value.plain(output);
		require(std::strcmp(output, "Red Back Both \xc3\xa9") == 0, 1);
	}
	tests++;

	// 2. Foreground, background, combined, and reset attributes are parsed.
	{
		AnsiString value("&+rR&-bB&=gWC&nN");
		require(value.size() == 4, 2);
		require(GET_FG(value[0]) != 0 && GET_BG(value[0]) == 0, 3);
		require(GET_FG(value[1]) == 0 && GET_BG(value[1]) != 0, 4);
		require(GET_FG(value[2]) != 0 && GET_BG(value[2]) != 0, 5);
		require(GET_ATTR(value[3]) == 0, 6);
	}
	tests++;

	// 3. CR is ignored and LF resets attributes for the following line.
	{
		AnsiString value("&+rA\r\nB");
		require(value.size() == 3 && value.ch(1) == '\n', 7);
		require(GET_ATTR(value[0]) != 0 && GET_ATTR(value[2]) == 0, 8);
	}
	tests++;

	// 4. Truncated and unknown markup remains literal instead of reading past NUL.
	for (const char *input : { "&", "&+", "&=", "&=r", "&+?" })
	{
		AnsiString value(input);
		value.plain(output);
		require(std::strcmp(output, input) == 0, 9);
	}
	tests++;

	// 5. ANSI rendering round-trips visible text and defuses literal ampersands.
	{
		AnsiString original("literal &&+rred&n tail");
		original.ansi(output);
		AnsiString reparsed(output);
		char plain[MAX_STRING_LENGTH];
		reparsed.plain(plain);
		require(std::strcmp(plain, "literal &red tail") == 0, 10);
	}
	tests++;

	// 6. Terminal output normalizes newlines and closes active attributes.
	{
		AnsiString value("&+rA\nB&+gC");
		value.term(output, TL_BRIGHT_BG);
		require(std::strstr(output, "\x1b[0;31mA\x1b[m\r\nB") != nullptr, 11);
		require(std::strlen(output) >= 3 &&
			std::strcmp(output + std::strlen(output) - 3, "\x1b[m") == 0, 12);
	}
	tests++;

	// 7. Uniform colorization changes attributes but never characters.
	{
		AnsiString value("abc");
		value.colorize(ATTR_FG(20));
		require(value.ch(0) == 'a' && value.ch(2) == 'c', 13);
		require(value.attr(0) == ATTR_FG(20) && value.attr(2) == ATTR_FG(20), 14);
	}
	tests++;

	// 8. An empty gradient clears existing colors.
	{
		AnsiString value("&+rabc");
		value.colorize(Gradient(""));
		require(value.attr(0) == 0 && value.attr(2) == 0, 15);
	}
	tests++;

	// 9. Coloring an empty string is a safe no-op.
	{
		AnsiString value;
		value.colorize(Gradient("rgb"));
		require(value.empty(), 16);
	}
	tests++;

	// 10. Expanded gradients cover every character and keep ordered color bands.
	{
		Gradient colors("rg");
		AnsiString value("abcdef");
		value.colorize(colors);
		require(value.attr(0) == colors[0] && value.attr(2) == colors[0], 17);
		require(value.attr(3) == colors[1] && value.attr(5) == colors[1], 18);
	}
	tests++;

	// 11. Contracted gradients select stable endpoints without overrunning the string.
	{
		Gradient colors("rgb");
		AnsiString value("xy");
		value.colorize(colors);
		require(value.ch(0) == 'x' && value.ch(1) == 'y', 19);
		require(value.attr(0) == colors[0] && value.attr(1) == colors[2], 20);
	}
	tests++;

	std::printf("%d ANSI runtime cases passed\n", tests);
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-ansi-runtime-") as directory:
    temp = Path(directory)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            f"-I{SRC}",
            str(harness),
            str(SRC / "ansi.c"),
            str(SRC / "unicode.c"),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("ANSI runtime regressions passed")
