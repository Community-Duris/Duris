#!/usr/bin/env python3
"""Qualify Tower direction clues and native magic passwords without a server."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'areas/wld/lortower.wld').read_text(encoding='utf8')
rooms = {int(m[1]): m[2] for m in re.finditer(
    r'^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)', source, re.M | re.S)}

def exit_at(room, direction):
    m = re.search(rf'\bD{direction}\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', rooms[room], re.S)
    assert m, (room, direction)
    return m

failures = []
for room, destination, expected in (
    (134011, 134014, 'To the south is a small shrine to Sargon.'),
    (134014, 134017, 'To the south is the stairs to the higher levels.'),
):
    route = exit_at(room, 2)
    assert int(route[5]) == destination and int(exit_at(destination, 0)[5]) == room
    clue = ' '.join(re.sub(r'&(?:\+.|[nN])', '', route[1]).split())
    if clue != expected:
        failures.append(f'{room}: south clue disagrees with its reciprocal route')

# Execute the maintained production functions, including exact-name matching and
# reciprocal lock/secret handling. Stubs provide only the room/message boundary.
def function(path, signature):
    text = (ROOT / path).read_text()
    start = text.index(signature)
    body = text.index('{', start)
    depth = 1
    end = body + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

prefix = r'''
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#define TRUE 1
#define FALSE 0
#define MAX_STRING_LENGTH 8192
#define MAX_INPUT_LENGTH 2048
#define NUM_EXITS 10
#define NOWHERE -1
#define EX_CLOSED 2
#define EX_LOCKED 4
#define EX_SECRET 64
#define TO_ROOM 1
#define TO_CHAR 2
#define LOWER(c) std::tolower(static_cast<unsigned char>(c))
#define IS_SET(a,b) ((a)&(b))
#define REMOVE_BIT(a,b) ((a)&=~(b))
struct room_direction_data { int key, exit_info, to_room; char *keyword; };
struct Room { room_direction_data *dir_option[NUM_EXITS] = {}; };
struct Character { int in_room; };
using P_char = Character *;
Room world[2];
const int rev_dir[] = {2,3,0,1,5,4,9,8,7,6};
#define EXIT(ch,d) world[(ch)->in_room].dir_option[d]
void half_chop(const char *in, char *first, char *rest) {
    while (*in==' ') ++in;
    while (*in && *in!=' ') *first++=*in++;
    *first=0;
    while (*in==' ') ++in;
    std::memmove(rest,in,std::strlen(in)+1);
}
const char *FirstWord(const char *) { return "door"; }
void act(const char *, int, P_char, int, int, int) {}
'''
cases = ''
for room, direction, phrase in (
    (134034, 0, 'sargon'), (134040, 1, 'sargon'),
    (134041, 3, 'sargon'), (134073, 4, 'thothrontithos'),
    (134049, 9, 'darkfather'), (134053, 6, 'darkfather'),
    (134083, 5, 'thothrontithos'),
):
    route = exit_at(room, direction)
    assert int(route[4]) == -2
    keyword = route[2].strip()
    assert re.fullmatch(r'[a-z &n]+', keyword)
    cases += f'''{{
char keyword[] = "{keyword}";
room_direction_data forward{{-2, EX_CLOSED|EX_LOCKED|EX_SECRET, 1, keyword}};
room_direction_data back{{-2, EX_CLOSED|EX_LOCKED|EX_SECRET, 0, keyword}};
world[0] = {{}}; world[1] = {{}};
world[0].dir_option[{direction}] = &forward;
world[1].dir_option[rev_dir[{direction}]] = &back;
Character ch{{0}};
check_magic_doors(&ch, "wrongword");
assert(forward.exit_info == (EX_CLOSED|EX_LOCKED|EX_SECRET));
check_magic_doors(&ch, "{phrase}");
if (forward.exit_info != EX_CLOSED || back.exit_info != EX_CLOSED) {{
    std::fprintf(stderr,"{room}: plain password fails\\n"); ++failed;
}}
}}'''
code = prefix + function('src/world/handler.c', 'bool isname(const char *str, const char *namelist)') + '\n' + function('src/cmd/actcomm.c', 'void check_magic_doors(P_char ch, const char *word)') + '\nint main() { int failed=0;\n' + cases + '\nreturn failed ? 1 : 0; }\n'
with tempfile.TemporaryDirectory(prefix='duris-lortower-password-') as temp:
    cpp = Path(temp) / 'password.cpp'
    binary = Path(temp) / 'password'
    cpp.write_text(code)
    subprocess.run(['g++-12', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)])
    if result.returncode:
        failures.append('plain native passwords do not unlock reciprocal doors')
assert not failures, '\n'.join(failures)
print('PASS: Tower south clues and production magic-password matching/reciprocal unlock; opening remains separate.')
