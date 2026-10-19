#!/usr/bin/env python3
"""Regression contract for Guild's fixed-size default-title buffers."""

from _paths import ROOT, SRC, extract_function
from pathlib import Path
import subprocess
import tempfile


root = Path(__file__).resolve().parents[2]
source = (SRC / "assocs.c").read_text()

constructors_start = source.index("Guild::Guild(char *_name")
constructors_end = source.index("\nvoid Guild::initialize()", constructors_start)
constructors = source[constructors_start:constructors_end]

safe_copy = 'snprintf(titles[i], sizeof(titles[i]), "%s", guild_default_titles[i]);'
assert constructors.count(safe_copy) == 2, (
    "both Guild constructors must use each rank title's actual buffer capacity"
)
assert "snprintf(titles[i], MAX_STRING_LENGTH" not in constructors, (
    "Guild constructors must not claim MAX_STRING_LENGTH for 81-byte rank titles"
)

print("guild title buffer bound contract passed")

# Run the production command as a mortal officer, including its color parser.
functions = '\n'.join(extract_function(file, signature) for file, signature in (
    ('utility.c', 'bool is_ansi_char('), ('utility.c', 'string strip_ansi('),
    ('assocs.c', 'void Guild::title('), ('assocs.c', 'void Guild::title_trim(')))
harness = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "core/files.h"
#include "guild/assocs.h"
#include <cassert>
#include <string>
static std::string sent;
static int saves;
Guild::Guild() { strcpy(name, "Fixture"); bits = 0; }
Guild::~Guild() {}
void send_to_char(const char *text, P_char) { sent += text; }
void send_to_char_f(P_char, const char *, ...) {}
void clear_title(P_char ch) { free(GET_TITLE(ch)); GET_TITLE(ch) = nullptr; }
char *str_dup(const char *text) { return strdup(text); }
int writeCharacter(P_char, int, int) { ++saves; return 1; }
''' + functions + r'''
int main()
{
    Guild guild;
    char_data officer{};
    officer.player.level = 25;
    GET_A_BITS(&officer) = A_OFFICER;
    GET_TITLE(&officer) = strdup("Original");
    auto run = [&](std::string text) {
        sent.clear();
        guild.title(&officer, &officer, text.data());
    };
    run("&+WOfficer");
    assert(std::string(GET_TITLE(&officer)) == "&+WOfficer&N Fixture");
    assert(saves == 1);
    std::string codes;
    for (int i = 0; i < 167; ++i) codes += "&+W";
    run(codes + "X"); // 501 code bytes + 1 visible byte + 10-byte suffix
    assert(strlen(GET_TITLE(&officer)) == 512);
    assert(saves == 2);
    const std::string accepted = GET_TITLE(&officer);
    for (const auto &invalid : {codes + "XX", codes + "&+W"}) {
        run(invalid);
        assert(std::string(GET_TITLE(&officer)) == accepted);
        assert(saves == 2);
        assert(sent.find("512 bytes") != std::string::npos);
    }
    std::string oversized;
    for (int i = 0; i < 200; ++i) oversized += "&+W";
    run(oversized);
    assert(std::string(GET_TITLE(&officer)) == accepted && saves == 2);
    run("Ordinary");
    assert(std::string(GET_TITLE(&officer)) == "Ordinary Fixture" && saves == 3);
    clear_title(&officer);
    puts("Guild titles: 512-byte boundary accepted; oversized color text rejected without mutation or save");
}
'''
with tempfile.TemporaryDirectory(prefix='duris-guild-title-') as td:
    test_source = Path(td) / 'test.cpp'
    test_source.write_text(harness)
    binary = ROOT / 'bin/tests/guild_title_bounds'
    binary.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-parameter', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', '-g', '-Isrc', '-I/usr/include/mysql',
                    str(test_source), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
