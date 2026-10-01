#!/usr/bin/env python3
"""Exercise the real progression owner through failed saves and cold recovery."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "core/files.h"
#include "classes/necromancy.h"
#include "world/vnum.obj.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "core/utils.h"
#include "magic/spells.h"
#include "player/craft_progression_hooks.h"
#include "player/player_save_pipeline.h"
#include "player/player_snapshot_codec.h"
#include <cassert>
#include <climits>
#include <cstdarg>
P_room world = nullptr;
int top_of_world = -1;
int notches = 0, xp = 0, saves = 0;
bool notch_skill(P_char, int skill, float chance) {
 assert((skill == SKILL_CRAFT || skill == SKILL_FORGE) && chance == 50); ++notches; return true;
}
int gain_exp(P_char, P_char, int amount, int kind) {
 assert(kind == EXP_BOON); xp += amount; return 0;
}
int panic_corruption_int(const char *, const char *, ...) { std::abort(); }
player_save_pipeline_result player_save_pipeline_request(P_char ch,
 player_component_mask_t mask, int, int) {
 assert(GET_PID(ch) == 7 && mask == CRAFT_PROGRESSION_COMPONENTS); ++saves;
 return player_save_pipeline_result::capture_failed;
}
critical_operation_id id(uint8_t value) { critical_operation_id op = {}; op.bytes[0]=value; return op; }
int main() {
 char_data actor = {}; pc_only_data pc = {}; actor.only.pc = &pc; pc.pid = 7; actor.in_room = -1;
 craft_recipe_continuation terms; terms.player_pid=7; terms.experience=7000;
 terms.recipe_vnum=100; terms.output_uid=80;
 for (auto discipline : {craft_recipe_discipline::craft, craft_recipe_discipline::forge}) {
  craft_progression_initialize(); terms.discipline=discipline; notches=xp=saves=0;
  assert(craft_progression_hooks.publish(id(1), &actor, terms)==craft_progression_publication_result::waiting);
  assert(notches==1 && xp==7000 && saves==1);
  for (int retry=0;retry<20;++retry)
   assert(craft_progression_hooks.publish(id(1), &actor, terms)==craft_progression_publication_result::waiting);
  assert(notches==1 && xp==7000 && saves==1);
  std::vector<player_craft_receipt_snapshot> receipts;
  assert(craft_progression_hooks.pending(7,&receipts) && receipts.size()==1);
  auto wrong=receipts[0]; ++wrong.experience;
  craft_progression_hooks.saved(7,true,&wrong,1);
  assert(craft_progression_hooks.publish(id(1), &actor, terms)==craft_progression_publication_result::waiting);
  assert(!craft_progression_hooks.recover(7,nullptr,0)); // older reconnect image
  craft_progression_hooks.saved(7,false,receipts.data(),receipts.size());
  assert(craft_progression_hooks.publish(id(1), &actor, terms)==craft_progression_publication_result::waiting);
  assert(notches==1 && xp==7000 && saves==2);
  assert(craft_progression_hooks.recover(7,receipts.data(),receipts.size()));
  assert(craft_progression_hooks.publish(id(1), &actor, terms)==craft_progression_publication_result::ready);
  craft_progression_hooks.acknowledged(id(1));
  assert(craft_progression_hooks.pending(7,&receipts) && receipts.empty());
  // A durable receipt loaded after restart suppresses all gameplay effects.
  player_craft_receipt_snapshot receipt{id(1),static_cast<uint32_t>(discipline),7000};
  craft_progression_initialize();
  assert(craft_progression_hooks.recover(7,&receipt,1));
  assert(craft_progression_hooks.publish(id(1), &actor, terms)==craft_progression_publication_result::ready);
  assert(notches==1 && xp==7000 && saves==2);
 }
 craft_progression_initialize();
 assert(craft_progression_hooks.publish({},&actor,terms)==craft_progression_publication_result::failed);
 auto receipt=player_craft_receipt_snapshot{id(3),1,10};
 auto duplicate=std::vector<player_craft_receipt_snapshot>{receipt,receipt};
 assert(!craft_progression_hooks.recover(7,duplicate.data(),duplicate.size()));
 // Freeze gameplay state and receipts in one canonical player save envelope.
 player_snapshot snapshot={}; snapshot.pid=7; snapshot.revision=5;
 snapshot.schema_version=PLAYER_SNAPSHOT_CRAFT_RECEIPT_SCHEMA_VERSION;
 snapshot.components=CRAFT_PROGRESSION_COMPONENTS;
 snapshot.encoded_size_bound=PLAYER_SNAPSHOT_MAX_BYTES;
 snapshot.craft_receipts={receipt};
 std::vector<uint8_t> bytes; player_snapshot decoded={};
 assert(player_snapshot_encode(snapshot,&bytes)==player_snapshot_codec_result::ok);
 assert(player_snapshot_decode(bytes.data(),bytes.size(),&decoded)==player_snapshot_codec_result::ok);
 assert(decoded.craft_receipts.size()==1 && decoded.craft_receipts[0].experience==10);
 snapshot.components &= ~PLAYER_COMPONENT_AFFECTS;
 assert(player_snapshot_encode(snapshot,&bytes)!=player_snapshot_codec_result::ok);
 snapshot.components=CRAFT_PROGRESSION_COMPONENTS; snapshot.craft_receipts.push_back(receipt);
 assert(player_snapshot_encode(snapshot,&bytes)!=player_snapshot_codec_result::ok);
 snapshot.craft_receipts={receipt}; snapshot.schema_version=PLAYER_SNAPSHOT_SCHEMA_VERSION;
 assert(player_snapshot_encode(snapshot,&bytes)!=player_snapshot_codec_result::ok);
 // Terminal death preserves the progression proof alongside custody and evidence.
 snapshot.schema_version=PLAYER_SNAPSHOT_DEATH_CRAFT_RECEIPT_SCHEMA_VERSION;
 snapshot.components=PLAYER_CHECKPOINT_COMPONENT_ALL; snapshot.save_intent=RENT_DEATH;
 snapshot.death.emplace(); snapshot.death->operation_id=id(4);
 snapshot.death->corpse_room_vnum=1201; snapshot.death->wallet_revision=1;
 player_item_snapshot corpse={}; corpse.object_uid=90000;
 corpse.parent_index=PLAYER_SNAPSHOT_NO_PARENT; corpse.equipment_slot=-1;
 corpse.vnum=VOBJ_CORPSE; corpse.type=ITEM_CORPSE;
 corpse.values[CORPSE_PID]=7; corpse.values[CORPSE_SAVEID]=1;
 corpse.values[CORPSE_FLAGS]=PC_CORPSE; snapshot.death->corpse={corpse};
 assert(player_snapshot_encode(snapshot,&bytes)==player_snapshot_codec_result::ok);
 assert(player_snapshot_decode(bytes.data(),bytes.size(),&decoded)==player_snapshot_codec_result::ok);
 assert(decoded.death && decoded.craft_receipts.size()==1 && decoded.craft_receipts[0].experience==10);
 snapshot.schema_version=PLAYER_SNAPSHOT_DEATH_CRAFT_EVIDENCE_SCHEMA_VERSION;
 snapshot.death->conflict_evidence.emplace(); auto &evidence=*snapshot.death->conflict_evidence;
 evidence.player_items.columns={"id","pid","obj_uid","vnum","container_id"};
 evidence.player_items.rows={{{"1"},{"7"},{"90000"},{"0"},std::nullopt}};
 evidence.player_item_affects.columns={"id","item_id","location","modifier"};
 evidence.player_item_extra_descr.columns={"id","item_id","keyword","description"};
 evidence.item_current_owner.columns={"item_uid","root_item_uid","parent_item_uid","item_revision","vnum","state","owner_type","owner_id","owner_context_id"};
 evidence.item_owner_revision.columns={"owner_type","owner_id","owner_context_id","revision"};
 assert(player_snapshot_encode(snapshot,&bytes)==player_snapshot_codec_result::ok);
 assert(player_snapshot_decode(bytes.data(),bytes.size(),&decoded)==player_snapshot_codec_result::ok);
 assert(decoded.death->conflict_evidence && decoded.craft_receipts.size()==1);
 bytes.pop_back();
 assert(player_snapshot_decode(bytes.data(),bytes.size(),&decoded)!=player_snapshot_codec_result::ok);
 assert(decoded.craft_receipts.size()==1 && decoded.death->conflict_evidence);
 // Wire rejection preserves the caller's frozen continuation.
 std::vector<uint8_t> wire; assert(craft_recipe_continuation_encode(terms,&wire));
 craft_recipe_continuation decoded_terms; assert(craft_recipe_continuation_decode(wire,&decoded_terms));
 wire[28] ^= 1; assert(!craft_recipe_continuation_decode(wire,&decoded_terms));
 assert(decoded_terms.experience==7000);
}
'''
with tempfile.TemporaryDirectory(prefix="duris-craft-progression-") as temporary:
    source = Path(temporary) / "progression.cpp"
    binary = Path(temporary) / "progression"
    source.write_text(HARNESS)
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra",
                    "-Werror", "-O1", "-g", "-fsanitize=address,undefined", "-fno-pie",
                    "-no-pie", "-Isrc", str(source), "src/economy/craft_progression.c",
                    "src/player/player_snapshot_codec.c", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)
print("craft progression: retry, exact ACK, reconnect fence, cold receipts and canonical save passed")
