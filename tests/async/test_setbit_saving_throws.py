#!/usr/bin/env python3
"""setbit's saving throws stay inside their five bytes.

`setbit char <name> savthr "<a> <b> <c> <d> <e>"` copied five shorts over the character's
five one-byte saving throws, so it ran on into the conditions after them and stored the
low and high bytes of the first values. The real ac_savthrCopy() runs against a character
with both arrays; ASan would also catch a write past them.
"""
from _paths import SRC
import subprocess
import tempfile
from pathlib import Path

source = (SRC / "actset.c").read_text()
start = source.rindex("static void ac_savthrCopy(")
function = source[start:source.index("\n}\n", start) + 3]

HARNESS = r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#include <strings.h>

typedef signed char byte;
typedef short sh_int;
struct char_data
{
	struct
	{
		::byte apply_saving_throw[5];
		::byte conditions[5];
	} specials;
};
typedef char_data *P_char;

int BOUNDED(int low, int value, int high)
{
	return value < low ? low : value > high ? high : value;
}
''' + function + r'''
int main()
{
	char_data ch = {};
	memset(ch.specials.conditions, 7, sizeof ch.specials.conditions);
	const ::byte conditions[5] = { 7, 7, 7, 7, 7 };

	char all[] = "1 -2 3 300 -300";
	ac_savthrCopy(&ch, 0, all, 0, 0);
	const ::byte bounded[5] = { 1, -2, 3, 127, -128 };
	assert(!memcmp(ch.specials.apply_saving_throw, bounded, sizeof bounded));
	assert(!memcmp(ch.specials.conditions, conditions, sizeof conditions));

	// Values that are not given keep what the character has.
	char two[] = "5 6";
	ac_savthrCopy(&ch, 0, two, 0, 0);
	const ::byte kept[5] = { 5, 6, 3, 127, -128 };
	assert(!memcmp(ch.specials.apply_saving_throw, kept, sizeof kept));
	assert(!memcmp(ch.specials.conditions, conditions, sizeof conditions));
	puts("setbit saving throws stay in their five bytes");
	return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-setbit-savthr-") as directory:
    harness = Path(directory) / "harness.cpp"
    binary = Path(directory) / "harness"
    harness.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    str(harness), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
