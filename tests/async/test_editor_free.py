#!/usr/bin/env python3
"""The line editor frees every line it holds.

edit_free() and edit_insert_data()'s refusal freed `lines[i++]` through FREE(), which
names its argument twice: each call freed one line, nulled the next and skipped two, so
every second line leaked, and an odd count read past the terminator. The harness runs the
production editor under LeakSanitizer: three lines, then freed; and three lines added to an
editor that holds two, which it refuses.
"""
from pathlib import Path
import os
import subprocess
import tempfile

from _paths import HARNESS_STUBS, ROOT

HARNESS = r'''
#include "core/structs.h"
#include "core/prototypes.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

void *__malloc(size_t size, const char *, const char *, int) { return calloc(1, size); }
void __free(void *memory, const char *, int) { free(memory); }
char *str_dup(const char *text) { return strdup(text); }
void write_to_q(const char *, txt_q *, int) {}
static void done(P_desc, int, char *) {}

int main()
{
	char_data ch{};
	ch.player.level = MINLVLIMMORTAL; // edit_has_ansi() says yes to all, and only gods may
	descriptor_data d{};
	d.character = &ch;
	char three[] = "first line\nsecond line\nthird line\n";

	edit_start(&d, three, 0, done, 0);
	assert(d.editor && d.editor->lines[2] && !d.editor->lines[3]);
	edit_free(d.editor);
	assert(!d.editor);

	char two[] = "first line\nsecond line\n";
	edit_start(&d, two, 2, done, 0);
	edit_string_add(d.editor, three);
	assert(d.editor->lines[1] && !d.editor->lines[2]);
	edit_free(d.editor);
}
'''

(ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="editor-free-", dir=ROOT / "bin/tests") as build:
    source = Path(build) / "harness.cpp"
    binary = Path(build) / "harness"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-g", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", "-Isrc", str(source), "src/net/editor.c",
                    str(HARNESS_STUBS), "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, env={
        **os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1"})
print("the line editor frees every line")
