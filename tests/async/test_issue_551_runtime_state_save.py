#!/usr/bin/env python3
"""Exercise the synchronous item-state writer with multiple captured roots."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, rel
source = (ROOT / 'src/sql/sql_player.c').read_text()
start = source.index('static bool sql_save_player_item_runtime_state(')
end = source.index('bool sql_save_player_items(P_char ch)', start)
helper = source[start:end]
assert 'runtime.payload' in source[source.rindex('bool sql_load_player_items(P_char ch)'):]
body = r'''#include "core/structs.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include <cassert>
#include <string>
#include <vector>
P_obj save_equip[MAX_WEAR] = {};
static std::vector<std::string> queries;
static bool refuse = false;
bool sql_run_query(const char *query) { queries.push_back(query); return !refuse; }
player_snapshot_capture_result player_item_snapshot_tree_capture(P_obj root, std::vector<player_item_snapshot> *items, size_t *) {
 player_item_snapshot item = {};
 item.object_uid = root->obj_uid; item.vnum = 1251;
 item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
 item.craftsmanship = 4; item.timers = {1,2,3,4,5,6};
 *items = {item};
 return player_snapshot_capture_result::ok;
}
''' + helper + r'''
int main() {
 char_data actor = {}; obj_data a = {}, b = {}, worn = {};
 a.obj_uid = 1; b.obj_uid = 2; worn.obj_uid = 3;
 a.next_content = &b; actor.carrying = &a; actor.equipment[0] = &worn;
 assert(sql_save_player_item_runtime_state(551, &actor));
 assert(queries.size() == 3);
 for (unsigned uid = 1; uid <= 3; ++uid) {
  bool found = false;
  for (const auto &query : queries) if (query.find("AND obj_uid=" + std::to_string(uid) + " ") != std::string::npos) found = true;
  assert(found);
 }
 refuse = true; queries.clear();
 assert(!sql_save_player_item_runtime_state(551, &actor) && queries.size() == 1);
}
'''
with tempfile.TemporaryDirectory(prefix='duris-craft-state-save-') as directory:
 cpp = Path(directory) / 'save.cpp'; binary = Path(directory) / 'save'
 cpp.write_text(body)
 subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Isrc', str(cpp), rel('player_snapshot_codec.c'), '-o', str(binary)], cwd=ROOT, check=True)
 subprocess.run([str(binary)], check=True)
print('legacy item runtime-state writer preserves every root and refuses SQL failure')
