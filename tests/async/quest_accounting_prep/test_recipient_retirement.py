#!/usr/bin/env python3
"""Execute current native GIVE observation for recipient incarnation and root UID.

The actual observe_give body runs; runtime lookup and world census are isolated
seams. Birth binding, D stock/cash retirement, SQL, ACK and cold recovery remain
native journey requirements. The old prototype/room lookup is not this route.
"""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

from case_data import ROOT, digest
from _paths import extract_function

PRELUDE = r'''
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
struct character;
struct object { uint64_t uid; int vnum; character *carrier=nullptr; };
struct character {
 bool npc=true; int pid=0,rnum=16006,in_room=16077; uint64_t runtime_id=0;
 struct { void *pc=nullptr; } only;
};
using P_char=character*; using P_obj=object*;
#define IS_NPC(ch) ((ch)->npc)
#define IS_PC(ch) (!(ch)->npc)
#define GET_PID(ch) ((ch)->pid)
#define OBJ_CARRIED_BY(obj,ch) ((obj)->carrier==(ch))
bool game_thread=true,scan_ok=true;
bool nevent_is_game_thread() { return game_thread; }
std::unordered_map<uint64_t,P_char> bodies;
std::unordered_map<uint64_t,P_obj> objects;
P_char find_character_by_runtime_id(uint64_t id) {
 auto found=bodies.find(id); return found==bodies.end()?nullptr:found->second;
}
namespace native_quest_world {
 struct census {
  std::unordered_set<P_char> seen_bodies;
  bool scan() {
   for(const auto &[id,body]:bodies) { (void)id; seen_bodies.insert(body); }
   return scan_ok;
  }
  P_obj unique(uint64_t uid) {
   auto found=objects.find(uid); return found==objects.end()?nullptr:found->second;
  }
 };
}
class item_native_quest_gameplay_publication_owner {
 public: static bool observe_give(uint64_t,uint32_t,uint64_t,uint64_t,P_char*,P_char*,P_obj*) noexcept;
};
'''
BODY = r'''
int main() {
 character player,original,replacement;
 player.npc=false; player.pid=42; player.runtime_id=10; player.only.pc=&player;
 original.runtime_id=20; replacement.runtime_id=21;
 object lance{101,16016,&original},replacement_lance{201,16016,&replacement};
 bodies={{10,&player},{20,&original}}; objects={{101,&lance},{201,&replacement_lance}};
 P_char observed_player=nullptr,observed_mobile=nullptr; P_obj observed_root=nullptr;
 auto observe=[&](uint64_t root=0) {
  return item_native_quest_gameplay_publication_owner::observe_give(
   10,42,20,root,&observed_player,&observed_mobile,&observed_root);
 };
 assert(observe(101) && observed_mobile==&original && observed_root==&lance);
 // Same prototype, room and stock kind do not supply original generation.
 bodies.erase(20); bodies.emplace(21,&replacement);
 assert(!observe(101)); assert(!observe());
 assert(observed_mobile==&original && observed_root==&lance); // strong failure outputs
 assert(replacement_lance.uid==201 && replacement_lance.carrier==&replacement);
 bodies.emplace(20,&original);
 assert(!observe(201)); // wrong incarnation's root, despite same VNUM
 lance.carrier=&replacement; assert(!observe(101)); lance.carrier=&original;
 objects.erase(101); assert(!observe(101)); objects.emplace(101,&lance);
 player.pid=43; assert(!observe()); player.pid=42;
 game_thread=false; assert(!observe()); game_thread=true;
 scan_ok=false; assert(!observe()); scan_ok=true;
 assert(observe(101) && observed_player==&player && observed_mobile==&original);
}
'''


def run(acceptance=False):
    function = extract_function('item/item_movement_transaction.c',
                                'bool item_native_quest_gameplay_publication_owner::observe_give(')
    # Source checks bind the isolated runtime seam to the actual native entry and
    # canonical birth-reference copier, without pretending to execute the codec.
    begin = extract_function('world/quest.c', 'bool quest_native_gameplay_owner::begin(')
    assert 'quest_mobile_native_reference_copy(mobile, mobile->runtime_id,' in begin
    binding = extract_function('world/quest_mobile_native_binding.c', 'bool quest_mobile_native_reference_copy(')
    assert 'find_character_by_runtime_id(expected_runtime_id) != character' in binding
    assert 'quest_mobile_native_reference_decode(bytes, &candidate)' in binding
    assert 'std::equal(canonical.begin(), canonical.end(), bytes.begin())' in binding
    build_root = ROOT / 'bin/tests'
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='quest-prep-retirement-', dir=build_root) as directory:
        cpp, executable = Path(directory) / 'recipient.cpp', Path(directory) / 'recipient'
        cpp.write_text(PRELUDE + function + BODY)
        subprocess.run(['g++', '-std=c++20', '-O0', '-Wall', '-Wextra', '-Werror',
                        str(cpp), '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    return dict(case='QP03', mode='acceptance' if acceptance else 'current observation',
                owner='item_native_quest_gameplay_publication_owner::observe_give',
                result='component passed; replacement generation refused; lookup/census stubbed; native retirement untested',
                source_hashes={str(p.relative_to(ROOT)): digest(p) for p in
                    (ROOT / 'src/world/quest.c', ROOT / 'src/item/item_movement_transaction.c',
                     ROOT / 'src/world/quest_mobile_native_binding.c')})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--acceptance', action='store_true', help='current owner must refuse replacement generation')
    args = parser.parse_args()
    print(json.dumps(run(args.acceptance), indent=2))
