#!/usr/bin/env python3
"""Exercise production Craft info/make refusals for missing material prototypes."""
from _paths import ROOT, extract_function
import os
from pathlib import Path
import subprocess
import tempfile

command = extract_function('crafting.c', 'void crafting_handle_craft_command(')
probes = {}
for action in ('info', 'make'):
    branch = command.index(f'else if (is_abbrev(first, "{action}"))')
    start = command.index('\t\tP_obj matLowest, matHighest;', branch)
    end = command.index('\t\t\treturn;\n\t\t}', start) + len('\t\t\treturn;\n\t\t}')
    probes[action] = command[start:end]
HARNESS = r'''
#include <cassert>
#include <cstdio>
#include <set>
struct object { const char* short_description; };
using P_obj = object*;
std::set<P_obj> live;
bool low_exists, high_exists;
int reads, extracted, messages, diagnostics, continued;
constexpr int VIRTUAL=1;
P_obj create_object() {
 P_obj result=new object{"material probe"};
 assert(live.insert(result).second);
 return result;
}
P_obj read_object(int vnum, int mode) {
 assert(mode==VIRTUAL && (vnum==100 || vnum==104));
 ++reads;
 return (vnum==100 ? low_exists : high_exists) ? create_object() : nullptr;
}
void extract_obj(P_obj object) {
 assert(object && live.erase(object)==1);
 ++extracted;
 delete object;
}
void send_to_char(const char*, void*) { ++messages; }
void debug(const char*, ...) { ++diagnostics; }
'''
FUNCTION = r'''
void craft_material_probes_@ACTION@(P_obj tobj) {
 void* ch=nullptr;
 int lowQualityMaterialVnum=100, selected=42;
 [[maybe_unused]] int highQualityMaterialVnum=104;
@PROBES@
 ++continued;
 // The rest of a successful command owns all three temporary objects.
 assert(live.size()==3);
 extract_obj(matLowest);
 extract_obj(matHighest);
 extract_obj(tobj);
}
'''
MAIN = r'''
int main() {
 for (auto action : {craft_material_probes_info, craft_material_probes_make}) {
  for (int mask=0; mask<4; ++mask) {
   low_exists=(mask&1)!=0; high_exists=(mask&2)!=0;
   reads=extracted=messages=diagnostics=continued=0;
   action(create_object());
   assert(reads==2);
   assert(live.empty());
   assert(extracted==1+static_cast<int>(low_exists)+static_cast<int>(high_exists));
   assert(messages==(mask==3 ? 0 : 1) && diagnostics==messages);
   assert(continued==(mask==3 ? 1 : 0));
  }
 }
}
'''
with tempfile.TemporaryDirectory(prefix='duris-craft-probe-cleanup-') as temporary:
    cpp = Path(temporary) / 'probe.cpp'
    binary = Path(temporary) / 'probe'
    cpp.write_text(HARNESS + '\n'.join(
        FUNCTION.replace('@ACTION@', action).replace('@PROBES@', body)
        for action, body in probes.items()) + MAIN)
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++20', '-Wall', '-Wextra',
                    '-Werror', '-O1', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-fno-pie', '-no-pie',
                    str(cpp), '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
print('Craft info/make material probes: all eight cases release each temporary object exactly once')
