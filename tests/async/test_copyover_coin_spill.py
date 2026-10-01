#!/usr/bin/env python3
"""Execute the actual movement coin-spill block in recovery and normal contexts."""
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
source=(subprocess.check_output(['git','show','e3ec44110:src/world/handler.c'],cwd=ROOT,text=True)
        if '--baseline' in sys.argv else (ROOT/'src/world/handler.c').read_text())
start=source.index('\ttotal_coins = GET_COPPER(ch)')
end=source.index('\n\t/*',start)
block=source[start:end]
program=r'''
#include <cassert>
#include <cstdio>
struct Character { struct { int cash[4]; } points; bool trusted; };
bool copyover=false,redis_recovery=false;
bool is_copyover_boot() { return copyover; }
bool redis_world_recovery_boot_active() { return redis_recovery; }
#define GET_COPPER(ch) ((ch)->points.cash[0])
#define GET_SILVER(ch) ((ch)->points.cash[1])
#define GET_GOLD(ch) ((ch)->points.cash[2])
#define GET_PLATINUM(ch) ((ch)->points.cash[3])
#define IS_TRUSTED(ch) ((ch)->trusted)
#define TRUE true
#define MAX_STRING_LENGTH 256
char Gbuf1[MAX_STRING_LENGTH];
const char *coin_abbrev[]={"c","s","g","p"};
int drops=0;
int number(int low,int) { return low; }
void do_drop(Character *ch,const char*,int) { ++drops; --ch->points.cash[0]; }
void movement(Character *ch) { int total_coins,x; bool worked=false;
'''+block+r'''
}
int main() {
 Character actor={{{201,0,0,0}},false};
 copyover=true;movement(&actor);assert(drops==0 && actor.points.cash[0]==201);
 copyover=false;redis_recovery=true;movement(&actor);assert(drops==0 && actor.points.cash[0]==201);
 redis_recovery=false;movement(&actor);assert(drops==1 && actor.points.cash[0]==200);
 movement(&actor);assert(drops==1);
 actor.points.cash[0]=201;actor.trusted=true;movement(&actor);assert(drops==1);
 puts("recovery placement preserves wallet/floor; normal movement spill remains active PASS");
}
'''
with tempfile.TemporaryDirectory(prefix='duris-recovery-coins-') as directory:
    p=Path(directory);(p/'test.cpp').write_text(program)
    subprocess.run(['g++','-std=c++20','-Wall','-Wextra','-Werror',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
