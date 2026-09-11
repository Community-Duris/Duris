#!/usr/bin/env python3
"""Execute score's production container-coin traversal and formatting with fixtures."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'src/cmd/actinf.c').read_text(encoding='utf-8')
start = source.index('static void score_container_coins(')
helper = source[start:source.index('\nvoid do_score(', start)]
start = source.index('unsigned long long container_coins[4] = {};', source.index('void do_score('))
end = source.index('send_to_char(buf, ch);', start) + len('send_to_char(buf, ch);')
output = source[start:end]
bank = source.index('Coins in bank:', source.index('void do_score('))
assert bank < start < source.index('get_account_name_safe(ch)', bank)
assert 'GET_PLATINUM(ch), GET_GOLD(ch), GET_SILVER(ch), GET_COPPER(ch)' in source

harness = r'''
#include <cassert>
#include <climits>
#include <cstdio>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>
constexpr int MAX_WEAR = 4, MAX_STRING_LENGTH = 4096;
enum { ITEM_CONTAINER, ITEM_MONEY, ITEM_CORPSE, ITEM_STORAGE, ITEM_OTHER };
constexpr int CONT_CLOSED = 1, ITEM2_TRANSPARENT = 2;
struct object {
    int type = ITEM_CONTAINER;
    int value[4] = {};
    int extra2_flags = 0;
    bool visible = true;
    object *contains = nullptr, *next_content = nullptr;
};
using P_obj = object *;
struct character { object *carrying = nullptr; object *equipment[MAX_WEAR] = {}; };
using P_char = character *;
#define GET_ITEM_TYPE(obj) ((obj)->type)
#define IS_SET(value, flag) ((value) & (flag))
#define CAN_SEE_OBJ(ch, obj) ((void)(ch), (obj)->visible)
''' + helper + r'''
std::string rendered;
void send_to_char(const char *buf, P_char) { rendered += buf; }
void render(P_char ch) {
    char buf[MAX_STRING_LENGTH];
''' + output + r'''
}
object money(int copper, int silver, int gold, int platinum) {
    object obj;
    obj.type = ITEM_MONEY;
    obj.value[0] = copper; obj.value[1] = silver;
    obj.value[2] = gold; obj.value[3] = platinum;
    return obj;
}
void expect(character &ch, unsigned long long c, unsigned long long s,
            unsigned long long g, unsigned long long p) {
    unsigned long long coins[4] = {};
    score_container_coins(&ch, coins);
    assert(coins[0] == c && coins[1] == s && coins[2] == g && coins[3] == p);
}
int main() {
    character ch;
    expect(ch, 0, 0, 0, 0);
    render(&ch);
    assert(rendered == "Coins in container(s): &+W   0 platinum&N  &+Y   0 gold&N  &n   0 silver&N  &+y   0 copper&N\n");
    object satchel, pouch, nested;
    object first = money(1, 2, 3, 4), second = money(5, 6, 7, 8);
    object third = money(9, 10, 11, 12);
    ch.carrying = &satchel; satchel.contains = &first;
    expect(ch, 1, 2, 3, 4);
    ch.equipment[2] = &pouch; pouch.contains = &second;
    first.next_content = &nested; nested.contains = &third;
    expect(ch, 15, 18, 21, 24);
    rendered.clear(); render(&ch);
    assert(rendered == "Coins in container(s): &+W  24 platinum&N  &+Y  21 gold&N  &n  18 silver&N  &+y  15 copper&N\n");
    // Multiple piles in one container and multiple inventory roots.
    object spare, fourth = money(1, 1, 1, 1);
    satchel.next_content = &spare; spare.contains = &fourth;
    expect(ch, 16, 19, 22, 25);
    spare.contains = nullptr; second.next_content = &fourth;
    expect(ch, 16, 19, 22, 25);
    second.next_content = nullptr;
    // Visibility applies at every container and to each money pile.
    nested.visible = false; expect(ch, 6, 8, 10, 12);
    nested.visible = true; first.visible = false; expect(ch, 14, 16, 18, 20);
    first.visible = true; satchel.visible = false; expect(ch, 5, 6, 7, 8);
    satchel.visible = true;
    // Closed opaque ancestors hide all descendants; transparent ones match look-in.
    satchel.value[1] = CONT_CLOSED; expect(ch, 5, 6, 7, 8);
    satchel.extra2_flags = ITEM2_TRANSPARENT; expect(ch, 15, 18, 21, 24);
    nested.value[1] = CONT_CLOSED; expect(ch, 6, 8, 10, 12);
    nested.extra2_flags = ITEM2_TRANSPARENT; expect(ch, 15, 18, 21, 24);
    // Ground/other-player containers have no path from this character's roots.
    object room_bag, other_bag, external = money(100, 100, 100, 100);
    room_bag.contains = &external; other_bag.contains = &external;
    character other; other.carrying = &other_bag;
    expect(ch, 15, 18, 21, 24);
    expect(other, 100, 100, 100, 100);
    // Carried corpses, storage lockers and non-container contents are excluded.
    for (int type : {ITEM_CORPSE, ITEM_STORAGE, ITEM_OTHER}) {
        spare.type = type; spare.contains = &external;
        expect(ch, 15, 18, 21, 24);
    }
    spare = money(100, 100, 100, 100); // loose inventory money is not container money
    expect(ch, 15, 18, 21, 24);
    // Shared equipment roots and malformed cycles neither repeat coins nor hang.
    ch.equipment[3] = &satchel;
    third.next_content = &first;
    expect(ch, 15, 18, 21, 24);
    spare.next_content = &satchel;
    expect(ch, 15, 18, 21, 24);
    // Preserve denominations, ignore invalid negative values, and exceed int range.
    character rich; object bag, a = money(INT_MAX, -5, 0, INT_MAX);
    object b = money(INT_MAX, 0, 0, INT_MAX);
    rich.carrying = &bag; bag.contains = &a; a.next_content = &b;
    expect(rich, 2ULL * INT_MAX, 0, 0, 2ULL * INT_MAX);
    unsigned long long totals[4] = {ULLONG_MAX - 1, 0, 0, ULLONG_MAX};
    score_container_coins(&rich, totals);
    assert(totals[0] == ULLONG_MAX && totals[3] == ULLONG_MAX);
    puts("PASS: score container coins, visibility, nesting, ownership boundaries and overflow");
}
'''
build_root = ROOT / 'bin/tests'
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='score-coins-', dir=build_root) as tmp:
    cpp = Path(tmp) / 'score_coins.cpp'
    exe = Path(tmp) / 'score_coins'
    cpp.write_text(harness, encoding='utf-8')
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-Wall', '-Wextra',
                    '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                    str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True, timeout=10)
