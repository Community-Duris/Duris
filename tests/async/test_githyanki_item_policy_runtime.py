#!/usr/bin/env python3
"""Compare the actual canonical Gith wrapper/policy with the original whole body.

Native construction/placement are controls; no factory, custody or Source proof.
"""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
from _paths import extract_function


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--reference-source-root", type=Path)
    parser.add_argument("--build-root", type=Path)
    args = parser.parse_args()
    root = args.source_root.resolve()
    reference = args.reference_source_root.resolve() if args.reference_source_root else None
    build = (args.build_root or Path(tempfile.gettempdir())).resolve()
    if build.is_relative_to(root) or (reference and build.is_relative_to(reference)):
        raise AssertionError("test scratch must be outside source")
    build.mkdir(parents=True, exist_ok=True)
    if reference:
        original = extract_function(reference / "src/world/limits.c", "void githyanki_weapon(")
    else:
        fixture = Path(__file__).parent / "fixtures/githyanki_weapon_dd215.inc"
        raw = fixture.read_bytes()
        if hashlib.sha256(raw).hexdigest() != "536db1782b9bc7d65b40abbf1d5ec1d2c073c774e88e525f20ba17b60037463e":
            raise AssertionError("original dd215 whole-wrapper fixture identity changed")
        original = raw.decode("utf-8")
    original = original.replace("void githyanki_weapon(", "void original_githyanki_weapon(", 1)
    wrapper = extract_function(root / "src/world/limits.c", "void githyanki_weapon(")
    reader = extract_function(root / "src/core/utility.c", "int GET_CLASS(").replace(
        "int GET_CLASS(", "int original_GET_CLASS(", 1)
    program = r'''
#include "world/githyanki_item_policy.h"
#include "core/structs.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "net/comm.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
std::array<unsigned,16> queries{};size_t query_count=0;
std::string effects,keywords,player_message,room_message;
bool succeeds=false;int requested=0;unsigned string_mask=0;obj_data native{};
CLASS_READER
int GET_CLASS(P_char actor,unsigned value){assert(query_count<queries.size());queries[query_count++]=value;return original_GET_CLASS(actor,value);}
P_obj read_object(int vnum,int mode){assert(mode==VIRTUAL);requested=vnum;effects+='C';return succeeds?&native:nullptr;}
char*str_dup(const char*text){effects+='D';keywords=text;return const_cast<char*>("controlled native name");}
void obj_to_char(P_obj object,P_char){assert(object==&native);effects+='T';string_mask=object->str_mask;}
void send_to_char(const char*text,P_char){effects+='S';player_message=text;}
void act(const char*text,int hide,P_char,P_obj object,void*victim,int kind){assert(hide==FALSE&&!object&&!victim&&kind==TO_ROOM);effects+='A';room_message=text;}
void reset(){effects.clear();keywords.clear();player_message.clear();room_message.clear();requested=0;string_mask=0;query_count=0;queries.fill(0);native.str_mask=STRUNG_DESC1;}
''' .replace("CLASS_READER", reader) + wrapper + "\n" + original
    program += r'''
int main(){
 char_data actor{};pc_only_data pc{};actor.only.pc=&pc;
 const unsigned bits[]={CLASS_NECROMANCER,CLASS_SORCERER,CLASS_PSIONICIST,CLASS_CONJURER,CLASS_ILLUSIONIST,CLASS_SUMMONER,CLASS_WARRIOR,CLASS_REAVER,CLASS_ANTIPALADIN};
 size_t cases=0;
 for(const auto&name:std::vector<std::string>{"Taverek",std::string(500,'x')}) {
  actor.player.name=const_cast<char*>(name.c_str());
  for(unsigned mask=0;mask<512;++mask)for(unsigned partition=0;partition<3;++partition){
   unsigned classes=0;for(unsigned i=0;i<9;++i)if(mask&(1U<<i))classes|=bits[i];
   actor.player.m_class=partition==0?classes:partition==1?0:classes&0xaaaaaaaaU;
   actor.player.secondary_class=partition==0?0:partition==1?classes:classes&0x55555555U;
   const int expected=(mask&63)?19:(mask&192)?418:(mask&256)?18:0;
   for(bool success:{false,true}){
    succeeds=success;reset();githyanki_weapon(&actor);
    assert(requested==expected&&effects==(expected?(success?"CDTSA":"C"):"S"));
    const auto new_effects=effects,new_keys=keywords,new_player=player_message,new_room=room_message;
    const auto new_queries=queries;const auto count=query_count;const auto new_mask=string_mask;
    reset();original_githyanki_weapon(&actor);
    assert(requested==expected&&effects==new_effects&&keywords==new_keys&&player_message==new_player&&room_message==new_room&&string_mask==new_mask&&queries==new_queries&&query_count==count);
    ++cases;
   }
  }
 }
 // Compare formatter output with names produced by the original whole body,
 // including percent directives as data and the exact 256-byte truncation cut.
 size_t formats=0;
 for(unsigned index:{0U,6U,8U})for(const auto&name:std::vector<std::string>{"","percent%s%n",std::string(255,'x'),std::string(500,'x')}){
  actor.player.name=const_cast<char*>("");actor.player.m_class=bits[index];actor.player.secondary_class=0;succeeds=true;
  reset();original_githyanki_weapon(&actor);const size_t original_fixed_length=keywords.size();
  actor.player.name=const_cast<char*>(name.c_str());actor.player.m_class=bits[index];actor.player.secondary_class=0;succeeds=true;
  reset();original_githyanki_weapon(&actor);const auto full_keys=keywords;
  const int vnum=index==0?19:index==6?418:18;
  for(size_t capacity:{size_t(1),size_t(2),size_t(255),size_t(256)}){
   std::array<char,256> output{};output.fill('!');
   const int length=githyanki_level_item_format_keywords(output.data(),capacity,vnum,name.c_str());
   const size_t full_length=original_fixed_length+name.size();
   assert(length==static_cast<int>(full_length));
   assert(std::string(output.data())==full_keys.substr(0,capacity-1));
   for(size_t i=capacity;i<output.size();++i)assert(output[i]=='!');
   ++formats;
  }
 }
 std::array<char,256> unchanged{};unchanged.fill('!');const auto before=unchanged;
 for(int bad:{0,999})assert(githyanki_level_item_format_keywords(unchanged.data(),unchanged.size(),bad,"Taverek")==-1&&unchanged==before);
 printf("PASS: %zu actual whole-wrapper cases, canonical GET_CLASS primary/secondary/fallback order, original templates/names/messages/mask/effect order; controlled native inputs\n",cases);
 printf("PASS: %zu literal formatter length/truncation/percent-as-data cases and invalid-choice strong outputs; canonical-only headers/providers\n",formats);
}
'''
    with tempfile.TemporaryDirectory(prefix="gith-policy-", dir=build) as directory:
        cpp, binary = Path(directory) / "control.cpp", Path(directory) / "control"
        cpp.write_text(program)
        command = [os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Wpedantic",
                   "-Werror", "-Wformat=2", "-I" + str(root / "src"), "-x", "c++",
                   str(root / "src/world/githyanki_item_policy.c"), str(cpp), "-o", str(binary)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        print(result.stdout, end="")


if __name__ == "__main__":
    main()
