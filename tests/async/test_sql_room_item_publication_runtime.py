#!/usr/bin/env python3
"""Execute exact restoration placement/cleanup helpers with native object types.

This is a bounded component regression, not SQL/world-boot qualification.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
text = (ROOT / "src/sql/sql_player.c").read_text(encoding="latin-1")
a = text.index("static void sql_room_item_clear_uids(P_obj object)")
b = text.index("class sql_room_item_stage_guard", a)
helpers = text[a:b]
program = r'''
#include <ctime>
#include "core/structs.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "world/db.h"
#include <cassert>
#include <cstring>
room_data *world;
int top_of_world;
static int light_calls;
int room_light(int, int) { return ++light_calls; }
''' + helpers + r'''
int main() {
    room_data rooms[3]{};
    world=rooms; top_of_world=2;
    obj_data root{}, child{}, second_child{}, grandchild{}, sibling{}, next_sibling{};
    root.obj_uid=100; child.obj_uid=101; second_child.obj_uid=102; grandchild.obj_uid=103;
    sibling.obj_uid=900; next_sibling.obj_uid=901;
    root.contains=&child; child.next_content=&second_child; child.contains=&grandchild;
    root.next_content=&sibling; sibling.next_content=&next_sibling;
    sql_room_item_clear_uids(&root);
    assert(!root.obj_uid && !child.obj_uid && !second_child.obj_uid && !grandchild.obj_uid);
    assert(sibling.obj_uid==900 && next_sibling.obj_uid==901);
    sql_room_item_clear_uids(nullptr);
    const int sectors[]={SECT_INSIDE,SECT_UNDERWATER,SECT_NO_GROUND};
    for(int index=0;index<3;++index) {
        root={}; root.obj_uid=100; root.type=ITEM_CONTAINER;
        root.extra_flags=ITEM_TRANSIENT|ITEM_LIT; root.value[0]=47;
        root.weight=93; root.z_cord=7;
        root.loc_p=LOC_NOWHERE; root.loc.room=NOWHERE;
        obj_affect effect{}; root.affects=&effect;
        rooms[index].contents=&sibling; rooms[index].sector_type=sectors[index];
        rooms[index].chance_fall=100;
        assert(sql_room_item_can_place_restored(&root,index));
        assert(!sql_room_item_can_place_restored(&root,-1));
        assert(!sql_room_item_can_place_restored(&root,3));
        const auto original=root;
        sql_room_item_place_restored(&root,index);
        assert(OBJ_ROOM(&root) && root.loc.room==index && rooms[index].contents==&root);
        assert(root.next_content==&sibling && sibling.next_content==&next_sibling);
        assert(root.obj_uid==original.obj_uid && root.extra_flags==original.extra_flags);
        assert(root.value[0]==original.value[0] && root.z_cord==original.z_cord);
        assert(root.weight==original.weight && root.affects==&effect);
        assert(!sql_room_item_can_place_restored(&root,index));
        assert(sibling.obj_uid==900 && next_sibling.obj_uid==901);
    }
    assert(light_calls==3);
    root.loc_p=LOC_NOWHERE;root.next_content=nullptr;
    root.type=ITEM_MONEY; assert(!sql_room_item_can_place_restored(&root,0));
    root.type=ITEM_CORPSE; assert(!sql_room_item_can_place_restored(&root,0));
    root.type=ITEM_CONTAINER; root.extra_flags|=ITEM_ARTIFACT;
    assert(!sql_room_item_can_place_restored(&root,0));
    assert(!sql_room_item_can_place_restored(nullptr,0));
}
'''
with tempfile.TemporaryDirectory(prefix="sql-room-placement-") as directory:
    source=Path(directory)/"placement.cpp"
    binary=Path(directory)/"placement"
    source.write_text(program,encoding="utf-8")
    subprocess.run(["g++","-std=c++20","-Wall","-Wextra","-Wpedantic","-Werror",
        "-fsanitize=address,undefined","-fno-omit-frame-pointer","-fno-pie","-no-pie",
        "-I"+str(ROOT/"src"),str(source),"-o",str(binary)],check=True,timeout=60)
    subprocess.run([str(binary)],check=True,timeout=10)
print("PASS: native restoration placement preserves exact state and unrelated sibling UIDs")
