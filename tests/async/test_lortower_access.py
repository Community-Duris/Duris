#!/usr/bin/env python3
"""Qualify reviewed native zone access and item names without a server."""
from pathlib import Path
import re
import subprocess
import tempfile
import json

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

# Smoke uses ordinary keys rather than magic passwords. Execute the maintained
# matching, carried/held-key and reciprocal lock functions. Visibility, command
# object selection and key destruction are boundaries, not played qualification.
smoke_rooms = {int(m[1]): m[2] for m in re.finditer(
    r'^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)',
    (ROOT / 'areas/wld/smoke.wld').read_text(encoding='utf8'), re.M | re.S)}
smoke_objects = {int(m[1]): m[2] for m in re.finditer(
    r'^#(\d+)\s*\n(.*?)(?=^#\d+|^\$|\Z)',
    (ROOT / 'areas/obj/smoke.obj').read_text(encoding='utf8'), re.M | re.S)}
def smoke_exit(room, direction):
    match = re.search(rf'\bD{direction}\s+([^~]*)~([^~]*)~\s*(-?\d+)\s+(-?\d+)\s+(-?\d+)', smoke_rooms[room], re.S)
    assert match
    return match

assert int(smoke_objects[139818].split('~')[4].split()[0]) == 18
assert int(smoke_objects[139819].split('~')[4].split()[0]) == 25
assert int(smoke_objects[139818].split('~')[4].split()[12]) == 100
smoke_prefix = r'''
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#define TRUE 1
#define FALSE 0
#define MAX_STRING_LENGTH 8192
#define NUM_EXITS 10
#define NOWHERE -1
#define HOLD 18
#define MINLVLIMMORTAL 60
#define ITEM_CONTAINER 15
#define ITEM_STORAGE 35
#define ITEM_QUIVER 30
#define CONT_CLOSED 4
#define CONT_LOCKED 8
#define EX_ISDOOR 1
#define EX_CLOSED 2
#define EX_LOCKED 4
#define EX_SECRET 64
#define EX_BLOCKED 128
#define EX_PICKPROOF 256
#define FIND_OBJ_INV 4
#define FIND_OBJ_ROOM 8
#define FIND_NO_TRACKS 256
#define VOBJ_TEMPLATE_KEY 11011
#define VNUM_TRACKS 11002
#define TO_ROOM 1
#define TO_CHAR 2
#define LOWER(c) std::tolower(static_cast<unsigned char>(c))
#define IS_SET(a,b) ((a)&(b))
#define REMOVE_BIT(a,b) ((a)&=~(b))
#define SET_BIT(a,b) ((a)|=(b))
struct Object { int R_num=0, type=18, value[8]={}; const char *name=""; Object *next_content=nullptr; };
using P_obj = Object *;
struct Character { int in_room=0, level=50; bool alive=true, immobile=false, blind=false, trusted=false, npc=false; struct { int z_cord=0; } specials; P_obj carrying=nullptr, equipment[19]={}; };
using P_char = Character *;
struct room_direction_data { int key, exit_info, to_room; char *keyword; };
struct Room { room_direction_data *dir_option[NUM_EXITS] = {}; };
Room world[2];
struct Index { int virtual_number; };
Index obj_index[] = {{139818},{139812},{139814},{139829}};
const int rev_dir[] = {2,3,0,1,5,4,9,8,7,6};
const char *dirs[] = {"north","east","south","west","up","down","northwest","southwest","northeast","southeast","\n"};
const char *short_dirs[] = {"n","e","s","w","u","d","nw","sw","ne","se","\n"};
#define EXIT(ch,d) world[(ch)->in_room].dir_option[d]
#define GET_LEVEL(ch) ((ch)->level)
#define IS_ALIVE(ch) ((ch)->alive)
#define IS_IMMOBILE(ch) ((ch)->immobile)
#define IS_BLIND(ch) ((ch)->blind)
#define IS_TRUSTED(ch) ((ch)->trusted)
#define IS_NPC(ch) ((ch)->npc)
#define IS_PC(ch) (!(ch)->npc)
#define OBJ_VNUM(obj) obj_index[(obj)->R_num].virtual_number
#define CAN_SEE_OBJ(ch,obj) ((ch)!=nullptr && (obj)!=nullptr)
#define IS_NOSHOW(obj) ((obj)==nullptr)
template<typename... Args> void act(Args...) {}
void send_to_char(const char *,P_char) {}
int generic_find(const char *,int,P_char,P_char *victim,P_obj *object) { *victim=nullptr;*object=nullptr;return 0; }
void argument_interpreter(char *arg,char *first,char *rest) { while(*arg==' ')++arg;while(*arg && *arg!=' ')*first++=*arg++;*first=0;while(*arg==' ')++arg;std::strcpy(rest,arg); }
int str_cmp(const char *a,const char *b) { return std::strcmp(a,b); }
int strn_cmp(const char *a,const char *b,unsigned n) { return std::strncmp(a,b,n); }
int get_number(char **) { return 1; }
int number(int,int) { return 0; }
int break_calls=0;
bool break_key(P_char,P_obj) { ++break_calls;return false; }
'''
smoke_code = smoke_prefix + '\n'.join(function(path, signature) for path, signature in (
    ('src/world/handler.c', 'bool isname(const char *str, const char *namelist)'),
    ('src/world/handler.c', 'P_obj get_obj_in_list_vis(P_char ch, const char *name, P_obj list, bool no_tracks)'),
    ('src/cmd/interp.c', 'int search_block(char *arg, const char **list, int exact)'),
    ('src/cmd/actmove.c', 'int find_door(P_char ch, char *type, char *dir)'),
    ('src/cmd/actmove.c', 'P_obj has_key(P_char ch, int key)'),
    ('src/cmd/actmove.c', 'void do_lock(P_char ch, char *argument, int /*cmd*/)'),
    ('src/cmd/actmove.c', 'void do_unlock(P_char ch, char *argument, int /*cmd*/)'),
)) + '\nint main() { int failed=0;\n'
for side, (room, direction, opposite) in enumerate(((139941, 0, 139942), (139942, 2, 139941))):
    forward, back = smoke_exit(room, direction), smoke_exit(opposite, (direction + 2) % 4)
    assert int(forward[5]) == opposite and int(back[5]) == room
    assert int(forward[3]) == int(back[3]) == 3
    smoke_code += f'''for (bool held : {{false,true}}) {{
char keyword[] = "portcullis";
room_direction_data forward{{{forward[4]},EX_ISDOOR|EX_PICKPROOF|EX_CLOSED|EX_LOCKED,1,keyword}};
room_direction_data back{{{back[4]},EX_ISDOOR|EX_PICKPROOF|EX_CLOSED|EX_LOCKED,0,keyword}};
world[0]={{}};world[1]={{}};
world[0].dir_option[{direction}]=&forward;world[1].dir_option[rev_dir[{direction}]]=&back;
Character ch;Object key,wrong;wrong.R_num=1;key.value[1]=100;
ch.carrying=&wrong;break_calls=0;
char command[]="portcullis";
do_unlock(&ch,command,0);
assert((forward.exit_info & EX_LOCKED) && (back.exit_info & EX_LOCKED) && break_calls==0);
if(held)ch.equipment[HOLD]=&key;else ch.carrying=&key;
do_unlock(&ch,command,0);
if(forward.exit_info!=(EX_ISDOOR|EX_PICKPROOF|EX_CLOSED) || back.exit_info!=(EX_ISDOOR|EX_PICKPROOF|EX_CLOSED) || break_calls!=1) {{
std::fprintf(stderr,"Smoke {room}: rewarded carried/held vault key fails\\n");++failed;
}} else {{
do_lock(&ch,command,0);assert((forward.exit_info & EX_LOCKED) && (back.exit_info & EX_LOCKED));
}}
}}\n'''
smoke_code += f'''Character ch;Object hate,discontent;
hate.R_num=2;discontent.R_num=3;
hate.name={json.dumps(smoke_objects[139814].split('~')[0].strip())};
discontent.name={json.dumps(smoke_objects[139829].split('~')[0].strip())};
hate.next_content=&discontent;
if(get_obj_in_list_vis(&ch,"discontent",&hate,false)!=&discontent) {{std::fprintf(stderr,"Smoke: Discontent cannot be selected by its own name\\n");++failed;}}
assert(get_obj_in_list_vis(&ch,"hate",&discontent,false)==&discontent);
assert(get_obj_in_list_vis(&ch,"hate",&hate,false)==&hate);
assert(get_obj_in_list_vis(&ch,"unknown",&hate,false)==nullptr);
return failed ? 1 : 0;
}}\n'''
with tempfile.TemporaryDirectory(prefix='duris-smoke-native-access-') as temp:
    cpp, binary = Path(temp) / 'access.cpp', Path(temp) / 'access'
    cpp.write_text(smoke_code)
    subprocess.run(['g++-12', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: Smoke production name lookup, carried/held vault key, wrong-key rejection, reciprocal locks and preserved closed/pickproof state; key settlement remains a boundary.')
