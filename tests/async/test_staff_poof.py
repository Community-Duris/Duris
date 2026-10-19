#!/usr/bin/env python3
"""Poof command boundaries and preview formatting under ASan/UBSan."""
import os
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, extract_function

HARNESS = r'''
#include "core/structs.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include "net/unicode.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
static std::string preview, response;
static int recipient;
void *__malloc(size_t n, const char *, const char *, int) { return malloc(n); }
void __free(void *p, const char *, int) { free(p); }
[[noreturn]] int panic_corruption_int(const char *, const char *, ...) { abort(); }
void send_to_char(const char *s, P_char) { response += s; }
void act(const char *s, int hide, P_char, P_obj, void *, int target) {
    assert(hide == TRUE); preview = s; recipient = target;
}
'''
for file, signature in (
    ('utility.c', 'char *str_dup('), ('utility.c', 'void str_free('),
    ('unicode.c', 'int get_utf8('), ('actwiz.c', 'static void show_poof('),
    ('actwiz.c', 'static void set_poof('), ('actwiz.c', 'void do_poofIn('),
    ('actwiz.c', 'void do_poofOut('),
):
    HARNESS += extract_function(file, signature) + '\n'
HARNESS += r'''
int main() {
    char_data ch{};
    pc_only_data pc{};
    ch.only.pc = &pc;
    ch.player.level = 57;
    for (bool entering : {true, false}) {
        char *&message = entering ? pc.poofIn : pc.poofOut;
        auto invoke = [&](std::string argument) {
            response.clear(); preview.clear();
            (entering ? do_poofIn : do_poofOut)(&ch, argument.data(), 0);
        };
        const std::string fallback = entering ? "$n appears with an ear-splitting bang." :
                                               "$n disappears in a puff of smoke.";
        invoke("?"); assert(preview == fallback && recipient == TO_CHAR);
        message = str_dup(""); // Database hydration normalizes NULL to an empty string.
        invoke("?"); assert(preview == fallback);
        show_poof(&ch, entering, TO_ROOM); assert(preview == fallback && recipient == TO_ROOM);
        invoke("&+GA mist reveals %n; %n waves.&n");
        invoke("?"); assert(preview == "&+GA mist reveals $n; $n waves.&n");
        show_poof(&ch, entering, TO_ROOM);
        assert(preview == "&+GA mist reveals $n; $n waves.&n" && recipient == TO_ROOM);
        invoke("leaves with $$5 and %s.");
        invoke("?"); assert(preview == "$n leaves with $$5 and %s.");
        invoke("  \t   "); assert(!message);
        invoke("?"); assert(preview == fallback);
        invoke("the"); assert(std::string(message) == "the"); // Free text, not a target parser.
        for (const std::string &unit : {std::string("x"), std::string("\xc3\xa9"), std::string("\xf0\x9f\x8c\xab")}) {
            std::string limit;
            for (int i = 0; i < 512; ++i) limit += unit;
            invoke(limit); assert(std::string(message) == limit && response.empty());
            invoke(limit + unit);
            assert(response.find("at most 512 characters") != std::string::npos);
            assert(std::string(message) == limit);
            invoke("?"); assert(preview == "$n " + limit);
            invoke("\t?  "); assert(preview == "$n " + limit);
        }
        ch.player.level = 56;
        const std::string before = message;
        invoke("forbidden"); assert(std::string(message) == before && response.empty());
        ch.player.level = 57;
        invoke("invalid\xff");
        assert(response.find("valid UTF-8") != std::string::npos);
        assert(std::string(message) == before);
        invoke(""); assert(!message);
    }
    puts("staff poof defaults, placeholders, whitespace, UTF-8 boundaries and permissions passed");
}
'''

with tempfile.TemporaryDirectory(prefix='staff-poof-native-') as temporary:
    source = Path(temporary) / 'probe.cpp'
    binary = Path(temporary) / 'probe'
    source.write_text(HARNESS)
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-g', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-pie', '-no-pie',
                    '-I', str(ROOT / 'src'), str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
