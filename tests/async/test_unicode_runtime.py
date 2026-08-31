#!/usr/bin/env python3
"""Runtime regression coverage for untrusted Unicode conversion paths."""

from _paths import SRC
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "net/unicode.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}

static void require(bool condition, int code)
{
	if (!condition)
		std::exit(code);
}

static void require_bytes(const char *actual, const char *expected, int code)
{
	require(std::strcmp(actual, expected) == 0, code);
}

int main()
{
	int tests = 0;

	// 1. Every UTF-8 width round-trips at its scalar-value boundaries.
	for (int value : { 0x01, 0x7f, 0x80, 0x7ff, 0x800, 0xd7ff, 0xe000, 0xffff,
			  0x10000, 0x10ffff })
	{
		char encoded[8] = {};
		char *out = encoded;
		put_utf8(out, value);
		*out = '\0';
		const char *in = encoded;
		require(get_utf8(in) == value && *in == '\0', 1);
	}
	tests++;

	// 2. A stray continuation run is one invalid unit and parsing recovers.
	{
		const char input[] = { char(0x80), char(0xbf), 'A', 0 };
		const char *in = input;
		require(get_utf8(in) == UNI_BAD && *in == 'A', 2);
	}
	tests++;

	// 3. Overlong encodings are rejected.
	{
		const char input[] = { char(0xc0), char(0xaf), 0 };
		const char *in = input;
		require(get_utf8(in) == UNI_BAD, 3);
	}
	tests++;

	// 4. Truncated sequences fail without consuming the following ASCII byte.
	{
		const char input[] = { char(0xe2), char(0x82), 'A', 0 };
		const char *in = input;
		require(get_utf8(in) == UNI_BAD && *in == 'A', 4);
	}
	tests++;

	// 5. Extra continuation bytes cannot be folded into an oversized integer.
	{
		const char input[] = { char(0xf0), char(0x90), char(0x80), char(0x80),
				       char(0x80), 'A', 0 };
		const char *in = input;
		require(get_utf8(in) == 0x10000, 5);
		require(get_utf8(in) == UNI_BAD && *in == 'A', 6);
	}
	tests++;

	// 6. UTF-16 surrogates are not valid UTF-8 scalar values.
	{
		const char input[] = { char(0xed), char(0xa0), char(0x80), 0 };
		const char *in = input;
		require(get_utf8(in) == UNI_BAD, 7);
	}
	tests++;

	// 7. Values above Unicode's U+10FFFF ceiling are rejected.
	{
		const char input[] = { char(0xf4), char(0x90), char(0x80), char(0x80), 0 };
		const char *in = input;
		require(get_utf8(in) == UNI_BAD, 8);
	}
	tests++;

	// 8. The encoder replaces negative, surrogate, and out-of-range values.
	for (int value : { -1, 0xd800, 0xdfff, 0x110000 })
	{
		char encoded[8] = {};
		char *out = encoded;
		put_utf8(out, value);
		*out = '\0';
		require_bytes(encoded, "\xef\xbf\xbd", 9);
	}
	tests++;

	// 9. CP437 input filters controls, upgrades glyphs, and quotes dollars.
	{
		const char input[] = { 'A', '$', '\n', char(0x80), 0 };
		char output[32];
		upgrade_cp437_and_dollars(output, input);
		require_bytes(output, "A$$\xc3\x87", 10);
	}
	tests++;

	// 10. Valid map-safe UTF-8 survives and dollars are quoted exactly once.
	{
		char output[32];
		require(!validate_utf8_and_dollars(output, "A$\xc3\xa9"), 11);
		require_bytes(output, "A$$\xc3\xa9", 12);
	}
	tests++;

	// 11. Invalid sequences and unsafe zero-width glyphs are rejected, not echoed.
	{
		const char input[] = { 'A', char(0xed), char(0xa0), char(0x80),
				       char(0xe2), char(0x80), char(0x8b), 'B', 0 };
		char output[32];
		require(validate_utf8_and_dollars(output, input), 13);
		require_bytes(output, "AB", 14);
	}
	tests++;

	// 12. ASCII downgrade uses transliteration before falling back to '?'.
	{
		unimap direct;
		char output[32];
		downgrade_string(output, "\xc3\xa9\xe2\x82\xac\xf0\x9f\x99\x82", direct);
		require_bytes(output, "eE?", 15);
	}
	tests++;

	// 13. Sparse maps merge and subtract without disturbing unrelated entries.
	{
		unimap left;
		unimap right;
		left.set('A', 1);
		left.set(0x1f642, 2);
		right.set('A', 3);
		right.set('B', 4);
		left += right;
		require(left['A'] == 3 && left['B'] == 4 && left[0x1f642] == 2, 16);
		left -= right;
		require(left['A'] == 0 && left['B'] == 0 && left[0x1f642] == 2, 17);
		require(left[-1] == 0 && left[1 << 21] == 0, 18);
	}
	tests++;

	std::printf("%d Unicode runtime cases passed\n", tests);
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-unicode-runtime-") as directory:
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
            str(SRC / "unicode.c"),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("Unicode conversion runtime regressions passed")
