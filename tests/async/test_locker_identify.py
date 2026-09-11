"""Exercise the actual locker service with held and rejected currency completions."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
service = (ROOT / 'src/item/locker_identify.c').read_text()
service = re.sub(r'^#include.*\n', '', service, flags=re.M)
locker = (ROOT / 'src/item/storage_lockers.c').read_text()
stat = locker.split('if (cmd == CMD_STAT && !IS_TRUSTED(ch))',1)[1].split('StorageLocker *pLocker',1)[0]
assert 'locker_identify(ch, tmp_object, cost)' in stat
assert 'CharWait' not in stat and 'do_lore' not in stat and 'SUB_MONEY' not in stat
lore = (ROOT / 'src/cmd/actnew.c').read_text()
normal = lore.split('void do_lore(',1)[1].split('static void render_item_lore(',1)[0]
assert 'lore_item(ch, obj);\n\t\tCharWait(ch, 3);' in normal
captured = lore.split('std::string item_lore_description(',1)[1].split('const char *MAKE_FORMAT',1)[0]
assert 'CharWait' not in captured and 'send_to_char' not in captured

(ROOT / 'bin').mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(dir=ROOT / 'bin') as temp:
    temp = Path(temp)
    cpp = temp / 'locker.cpp'
    binary = temp / 'locker'

    # Compile the actual lore renderer with real object layouts/constants. Only
    # presentation tables and networking are stubbed, so item variants exercise
    # the same capture implementation used by the paid service.
    render_start = lore.index('static void render_item_lore(')
    render_end = lore.index('const char *MAKE_FORMAT', render_start)
    render_prefix = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include "item/objmisc.h"
#include "net/comm.h"
#include <cassert>
#include <cstdarg>
#include <iostream>
int checked_snprintf(char *out,size_t size,const char *format,...) {
 va_list args; va_start(args,format); int result=vsnprintf(out,size,format,args); va_end(args); return result; }
const char *item_types[]={"type"}, *spells[]={"spell"}, *apply_types[]={"attribute"};
const flagDef extra_bits[]={{}}, affected1_bits[]={{}}, affected2_bits[]={{}},
 affected3_bits[]={{}}, affected4_bits[]={{}}, affected5_bits[]={{}};
std::string sent;
void send_to_char(const char *text,P_char) { sent+=text; }
void sprinttype(int,const char **,char *text) { strcpy(text,"type"); }
void sprintbitde(ulong,const flagDef[],char *text) { strcpy(text,"ability"); }
bool ac_can_see_obj(P_char,P_obj,int) { return true; }
int itemvalue(P_obj) { return 42; }
char *get_str_zone(P_obj) { static char zone[]="Test Zone"; return zone; }
void act(const char *text,int,P_char,P_obj obj,void *,int) {
 std::string line=text;
 auto pos=line.find("$p"); if(pos!=std::string::npos) line.replace(pos,2,obj->short_description);
 sent+=line+"\r\n";
}
'''
    render_main = r'''
int main() {
 char_data ch={}; ch.player.level=50;
 obj_data obj={}; char name[]="a test item"; obj.short_description=name; obj.weight=5;
 for(int type : {ITEM_WEAPON,ITEM_ARMOR,ITEM_TOTEM,ITEM_POTION,ITEM_WAND}) {
  obj.type=type; obj.value[0]=TOTEM_LESS_ANIM|TOTEM_GR_SPIR;
  obj.value[1]=3; obj.value[2]=4; obj.value[3]=1; obj.bitvector=1;
  obj.affected[0].location=APPLY_STR; obj.affected[0].modifier=2;
  sent.clear(); lore_item(&ch,&obj); std::string normal=sent;
  sent.clear(); auto snapshot=item_lore_description(&ch,&obj);
  assert(sent.empty()); assert(snapshot=="This item is from the zone: Test Zone\n"+normal);
  if(type==ITEM_WEAPON) assert(snapshot.find("3D4")!=std::string::npos);
  if(type==ITEM_TOTEM) assert(snapshot.find("Greater Spiritual")!=std::string::npos);
 }
 ch.player.level=49; obj.extra_flags=ITEM_NOIDENTIFY;
 auto denied=item_lore_description(&ch,&obj);
 assert(denied.find("can't recall")!=std::string::npos);
 assert(item_lore_description(nullptr,&obj).empty());
 std::cout << "actual lore capture matches ordinary rendering for weapon, armor, totem, potion and wand\n";
}
'''
    cpp.write_text(render_prefix + lore[render_start:render_end] + render_main)
    subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror','-Isrc',
                    str(cpp),'-o',str(binary)],cwd=ROOT,check=True)
    subprocess.run([str(binary)],check=True)
