#!/usr/bin/env python3
"""Production generated-NPC runtime state, tagged file bytes and Redis wire round trips."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

HARNESS=r'''
#include "world/generated_npc_state.h"
#include "world/world_recovery_codec.h"
#include "persistence/copyover.h"
#include "core/utils.h"
#include "core/prototypes.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
index_data indexes[3]={};
P_index mob_index=indexes;
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
char *str_dup(const char *s) { return strdup(s); }
void str_free(const char *s) { free(const_cast<char *>(s)); }
'''+extract_function('copyover.c','static bool write_generated_npc_state(')+'\n'+extract_function('copyover.c','static bool read_generated_npc_state(')+r'''
static void clear_strings(P_char mob) {
 str_free(mob->player.name); str_free(mob->player.short_descr); str_free(mob->player.long_descr);
 mob->player.name=mob->player.short_descr=mob->player.long_descr=nullptr;
}
int main() {
 indexes[0].virtual_number=1255; indexes[1].virtual_number=1256; indexes[2].virtual_number=11;
 for(int type : {0,1}) {
  char_data original={}, restored={}; npc_only_data npc={}, next={};
  original.only.npc=&npc; restored.only.npc=&next;
  npc.R_num=next.R_num=type; original.specials.act=restored.specials.act=ACT_ISNPC;
  original.player.name=str_dup("Theran generated guardian");
  original.player.short_descr=str_dup("Theran the wandering guardian");
  original.player.long_descr=str_dup("Theran the wandering guardian stands here.\r\n");
  npc.str_mask=STRUNG_KEYS|STRUNG_DESC1|STRUNG_DESC2;
  original.player.level=61; original.player.race=1; original.player.sex=1; original.player.size=2;
  original.player.m_class=4; original.player.secondary_class=8;
  original.specials.alignment=-350;
  original.points.base_hit=5421; original.points.base_mana=52; original.points.base_vitality=83;
  original.points.base_armor=-73; original.points.base_hitroll=61; original.points.base_damroll=62;
  original.points.damnodice=12; original.points.damsizedice=13;
  for(size_t i=0;i<10;++i) original.base_stats[i]=90+i;
  original.specials.affected_by=AFF_HASTE|AFF_AWARE;
  original.specials.affected_by4=AFF4_DETECT_ILLUSION;
  npc.aggro_flags=7; npc.aggro2_flags=8; npc.aggro3_flags=9;
  GET_COPPER(&original)=21; GET_SILVER(&original)=22; GET_GOLD(&original)=23; GET_PLATINUM(&original)=24;
  std::string encoded; assert(generated_npc_capture(&original,&encoded) && !encoded.empty());
  auto negative_wallet=encoded; negative_wallet[3]=char(255);
  assert(!generated_npc_state_valid(indexes[type].virtual_number,negative_wallet));
  std::string fuzzy; assert(generated_npc_capture(&original,&fuzzy,false));
  pet_restore_state fuzzy_state; std::array<int32_t,4> fuzzy_wallet;
  assert(generated_npc_state_decode(fuzzy,&fuzzy_state,&fuzzy_wallet));
  assert((fuzzy_wallet == std::array<int32_t,4>{}));
  FILE *file=tmpfile(); assert(file);
  assert(write_generated_npc_state(file,&original)); rewind(file);
  std::string from_file; assert(read_generated_npc_state(file,indexes[type].virtual_number,&from_file));
  assert(from_file==encoded); fclose(file);
  std::string extension; assert(generated_npc_extension_encode(indexes[type].virtual_number,encoded,&extension));
  for(size_t size=0;size<extension.size();++size) {
   std::string bad; assert(!generated_npc_extension_decode(indexes[type].virtual_number,extension.data(),size,&bad));
  }
  auto oversized=extension; oversized[4]=oversized[5]=oversized[6]=oversized[7]=char(255);
  std::string bad; assert(!generated_npc_extension_decode(1255,oversized.data(),oversized.size(),&bad));
  assert(!generated_npc_extension_decode(11,extension.data(),extension.size(),&bad));
  copyover_mob entry={}; entry.shopkeeper_shop_id=-1; entry.vnum=indexes[type].virtual_number; entry.idnum=123+type; entry.room=22800;
  entry.hit=4321; entry.max_hit=5421;
  for(auto &v:entry.equipment_vnums) v=-1;
  std::vector<unsigned char> native(sizeof(entry)+extension.size());
  memcpy(native.data(),&entry,sizeof(entry)); memcpy(native.data()+sizeof(entry),extension.data(),extension.size());
  std::vector<unsigned char> wire(40000), decoded;
  size_t size=0;
  for(int round=0;round<5;++round) {
   assert(world_recovery_encode_record(world_recovery_record_type::mob,native.data(),native.size(),wire.data(),wire.size(),&size));
   assert(world_recovery_decode_record(world_recovery_record_type::mob,wire.data(),size,&decoded));
   assert(decoded==native);
   std::string value;
   assert(generated_npc_extension_decode(entry.vnum,(const char*)decoded.data()+sizeof(entry),decoded.size()-sizeof(entry),&value));
   assert(generated_npc_apply(&restored,value));
   assert(!next.summoned_instance && !next.summon_kind && !next.pet_charm_expires_at && !next.pet_death_expires_at);
   assert(restored.points.base_hit==5421 && restored.points.base_damroll==62);
   assert(GET_COPPER(&restored)==21 && GET_SILVER(&restored)==22 && GET_GOLD(&restored)==23 && GET_PLATINUM(&restored)==24);
   assert(restored.player.m_class==4 && restored.player.secondary_class==8 && restored.player.level==61);
   assert(!strcmp(restored.player.short_descr,"Theran the wandering guardian"));
   std::string again; assert(generated_npc_capture(&restored,&again) && again==encoded);
  }
  // Current file version has a bounded extension. The old native layout stays
  // intact; ordinary/legacy wire records remain accepted without an extension.
  assert(generated_npc_apply(&restored,""));
  assert(world_recovery_encode_record(world_recovery_record_type::mob,(const unsigned char*)&entry,sizeof(entry),wire.data(),wire.size(),&size));
  assert(size==286 && world_recovery_decode_record(world_recovery_record_type::mob,wire.data(),size,&decoded));
  assert(decoded.size()==sizeof(entry));
  // Legacy instances remain explicit empty extensions; they must not stop all
  // world persistence. No generated identity is invented for discarded state.
  npc.str_mask=0; assert(generated_npc_capture(&original,&bad) && bad.empty());
  npc.str_mask=STRUNG_KEYS|STRUNG_DESC1|STRUNG_DESC2;
  clear_strings(&original); clear_strings(&restored);
 }
 char_data ordinary={}; npc_only_data npc={}; ordinary.only.npc=&npc; npc.R_num=2; ordinary.specials.act=ACT_ISNPC;
 std::string empty="old"; assert(generated_npc_capture(&ordinary,&empty) && empty.empty());
 puts("generated NPC state: file extension, Redis wire, repeated identity/stats/flags/owned strings, ordinary control and malformed/legacy handling passed");
}
'''
with tempfile.TemporaryDirectory(prefix='generated-npc-') as directory:
    cpp=Path(directory)/'test.cpp'; binary=Path(directory)/'test'
    cpp.write_text(HARNESS)
    subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-D__NO_MYSQL__',
        '-Isrc','-Isrc/no_mysql','-fsanitize=address,undefined','-g',str(cpp),
        'src/world/generated_npc_state.c','src/world/generated_npc_runtime.c',
        'src/player/pet_restore_state.c','src/world/world_recovery_codec.c','-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True)
