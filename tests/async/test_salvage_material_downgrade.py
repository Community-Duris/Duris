#!/usr/bin/env python3
"""Exercise the production material downgrade branch with authority failures."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/item/salvage.c").read_text()

def function(name):
    start = source.index(name)
    opening = source.index("{", start)
    depth = 1
    cursor = opening + 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[start:cursor]

branch = source[source.index("// Handle salvage materials"):source.index("\n\tif (!is_salvageable(item))", source.index("// Handle salvage materials"))]
production = function("bool grant_salvage_item(")
if "void salvage_material_completed(" in source:
    production += "\n" + function("void salvage_material_completed(")
    production += "\n" + function("bool downgrade_salvage_material(")

fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
constexpr bool FALSE=false, TRUE=true;
constexpr int VIRTUAL=1, TO_ROOM=1, TO_CHAR=2, LOG_DEBUG=3;
constexpr int LOWEST_MAT_VNUM=400000, HIGHEST_MAT_VNUM=400209;
struct object { int vnum; const char *short_description="material"; bool carried=false; bool retired=false; };
using P_obj=object*;
struct character {};
using P_char=character*;
enum class economic_source_kind { crafting };
struct item_transfer_result {};
enum class item_movement_reject { none };
using completion_fn=void (*)(P_char,bool,const item_transfer_result&,unsigned int,const uint8_t*,size_t);
std::vector<std::unique_ptr<object>> candidates;
std::vector<std::string> messages;
int reads=0, missing=0, grant_calls=0, failed_grant=0, submissions=0;
bool admit=true;
P_obj frozen_input=nullptr;
P_obj frozen_outputs[2]{};
completion_fn frozen_callback=nullptr;
int OBJ_VNUM(P_obj obj) { return obj->vnum; }
int get_matstart(P_obj) { return 400045; }
void send_to_char(const char* text,P_char) { messages.emplace_back(text); }
void act(const char*,bool,P_char,P_obj,int,int) {}
void logit(int,const char*,...) {}
P_obj read_object(int vnum,int) {
 if (++reads==missing) return nullptr;
 candidates.push_back(std::make_unique<object>());
 candidates.back()->vnum=vnum;
 return candidates.back().get();
}
void obj_from_char(P_obj obj) { assert(obj && obj->carried); obj->carried=false; }
void extract_obj(P_obj obj,bool=false) { assert(obj && !obj->retired); obj->retired=true; obj->carried=false; }
bool item_creation_grant_submit_to_player(P_char,P_obj obj,P_char,void*,economic_source_kind) {
 assert(obj && !obj->carried && !obj->retired);
 if (++grant_calls==failed_grant) return false;
 obj->carried=true;
 return true;
}
bool item_movement_transaction_submit_craft(P_char,P_obj const* inputs,size_t input_count,
 P_obj const* outputs,size_t output_count,int64_t recipe_id,completion_fn callback,
 const void* context,size_t context_size,item_movement_reject*) {
 ++submissions;
 assert(input_count==1 && output_count==2 && inputs[0]->carried && !inputs[0]->retired);
 assert(recipe_id==inputs[0]->vnum && !context && !context_size);
 assert(outputs[0] && outputs[1] && outputs[0]!=outputs[1]);
 for (size_t i=0;i<2;++i) assert(!outputs[i]->carried && !outputs[i]->retired && outputs[i]->vnum==400046);
 if (!admit) return false;
 frozen_input=inputs[0]; frozen_outputs[0]=outputs[0]; frozen_outputs[1]=outputs[1]; frozen_callback=callback;
 return true;
}
'''
fixture += production + "\nvoid downgrade(P_char ch,P_obj item) { int itemvnum=OBJ_VNUM(item), lowest=0;\n" + branch + "\n}\n"
fixture += r'''
int main() {
 character actor;
 // All refusal cases retain the original and publish no partial outputs.
 for (int absent : {0,1,2}) {
  candidates.clear(); messages.clear(); reads=grant_calls=submissions=0; missing=absent;
  failed_grant=1; admit=false; frozen_input=nullptr;
  object original{400047,"original",true,false};
  downgrade(&actor,&original);
  assert(original.carried && !original.retired);
  assert(grant_calls==0 && !frozen_input);
  for (const auto& item:candidates) assert(item->retired && !item->carried);
  assert(!messages.empty());
 }
 // Submission holds both detached outputs and the original for one native commit.
 candidates.clear(); messages.clear(); reads=grant_calls=submissions=0; missing=0; failed_grant=0; admit=true;
 object original{400047,"original",true,false}; downgrade(&actor,&original);
 assert(submissions==1 && grant_calls==0 && original.carried && !original.retired && frozen_callback);
 for (auto obj:frozen_outputs) assert(!obj->carried && !obj->retired);
 // Terminal rejection preserves input; only the authority owner cleans candidates.
 for (auto obj:frozen_outputs) extract_obj(obj);
 frozen_callback(&actor,false,{},1,nullptr,0);
 assert(original.carried && !original.retired);
 candidates.clear(); messages.clear(); reads=submissions=0; frozen_callback=nullptr;
 downgrade(&actor,&original);
 assert(submissions==1 && original.carried && !original.retired);
 extract_obj(&original);
 for (auto obj:frozen_outputs) obj->carried=true;
 frozen_callback(&actor,true,{},0,nullptr,0);
 assert(original.retired && !original.carried);
 for (auto obj:frozen_outputs) assert(obj->carried && !obj->retired);
 assert(!messages.empty());
}
'''
with tempfile.TemporaryDirectory(prefix="duris-salvage-downgrade-") as directory:
    root = Path(directory)
    harness = root / "harness.cpp"
    harness.write_text(fixture)
    binary = root / "harness"
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-g", str(harness), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS":"detect_leaks=1:abort_on_error=1"})
print("PASS: native material branch preserves inputs on missing prototypes/refused batch, queues two frozen outputs, and publishes after completion")
