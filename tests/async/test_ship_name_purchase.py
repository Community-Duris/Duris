#!/usr/bin/env python3
"""Exercise colored ship-name encoding and the real pending purchase layout."""
from pathlib import Path
import re
import subprocess
import tempfile

from _paths import ROOT, extract_function

shop = (ROOT / 'src/ships/ship_shop.c').read_text()
header = (ROOT / 'src/world/epic_transaction.h').read_text()
context_match = re.search(r'struct ship_hull_purchase_context\s*\{.*?\n\};', shop, re.S)
limit_match = re.search(r'constexpr size_t EPIC_PENDING_CONTEXT_MAX_BYTES = \d+;', header)
assert context_match and limit_match
context = context_match.group()
limit = limit_match.group()
validator = extract_function('ships/ship_base.c', 'bool check_ship_name(')
assert 'an.size() > 20' in validator
assert 'AnsiString(arg2).ansi(normalized_name);' in shop
assert 'name_bytes >= sizeof(ship_hull_purchase_context{}.name)' in shop
assert 'memcpy(context.name, normalized_name, name_bytes + 1);' in shop

harness = r'''
#include "net/ansi.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <array>
#include <string>
#include <cstdlib>
void panic_corruption(const char *, const char *, ...) { std::abort(); }
'''+limit+'\n'+context+r'''
static_assert(sizeof(ship_hull_purchase_context) <= EPIC_PENDING_CONTEXT_MAX_BYTES);
void check(const std::string &input, size_t visible) {
    AnsiString parsed(input.c_str());
    assert(parsed.size() == visible);
    char normalized[MAX_STRING_LENGTH];
    parsed.ansi(normalized);
    ship_hull_purchase_context sent{};
    assert(strlen(normalized) < sizeof(sent.name));
    memcpy(sent.name, normalized, strlen(normalized) + 1);
    std::array<unsigned char, EPIC_PENDING_CONTEXT_MAX_BYTES> pending{};
    memcpy(pending.data(), &sent, sizeof(sent));
    ship_hull_purchase_context received{};
    memcpy(&received, pending.data(), sizeof(received));
    assert(AnsiString(received.name) == parsed); // characters AND color attributes
}
int main() {
    const char *reported = "&+rD&+Lea&+rth&+Ls &+rd&+Loo&+rr";
    assert(strlen(reported) == 32);
    char plain[MAX_STRING_LENGTH];
    AnsiString(reported).plain(plain);
    assert(!strcmp(plain, "Deaths door"));
    check(reported, 11);
    check("Deaths door", 11);
    std::string alternating, unicode, redundant;
    for (int i = 0; i < 20; ++i) {
        alternating += (i % 2 ? "&+rX" : "&+LX");
        unicode += (i % 2 ? "&+r\xF0\x9F\x9A\xA2" : "&+L\xF0\x9F\x9A\xA2");
    }
    check(alternating, 20);
    check(unicode, 20);
    for (int i = 0; i < 200; ++i) redundant += "&+r";
    check(redundant + "Deaths door&n", 11);
    assert(AnsiString((alternating + "X").c_str()).size() > 20);
    assert(AnsiString("&+r&n").empty());
    puts("ship purchase name regression passed: report, colors, UTF-8, boundaries, callback copy");
}
'''
with tempfile.TemporaryDirectory(prefix='ship-name-') as directory:
    cpp = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    cpp.write_text(harness)
    subprocess.run(['g++', '-std=c++20', '-Isrc', '-ffunction-sections',
                    '-fdata-sections', str(cpp), 'src/net/ansi.c', 'src/net/unicode.c',
                    '-Wl,--gc-sections', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
