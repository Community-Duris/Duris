#!/usr/bin/env python3
"""Shopkeeper provenance requires a complete, validated compatible copyover."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/persistence/copyover.c").read_text()

def function(signature):
    start = source.index(signature)
    cursor = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[start:cursor]

program = r'''
#include "persistence/copyover_codec.h"
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
const char *copyover_state_file() { return "compat-copyover.dat"; }
void logit(const char *, const char *, ...) {}
'''+function("bool copyover_has_durable_shopkeepers(")+r'''
int main() {
    assert(copyover_codec_legacy_abi_compatible());
    for (int version=11; version<=17; ++version) {
        copyover_header h={};
        memcpy(h.magic, COPYOVER_MAGIC, 4); h.version=version;
        FILE *f=fopen(COPYOVER_FILE,"wb"); assert(f);
        assert(fwrite(&h,sizeof(h),1,f)==1);
        const int listeners[3]={-1,-1,-1};
        assert(fwrite(listeners,sizeof(listeners),1,f)==1);
        if (version>=15) {
            const unsigned char trailer[12]={'T','L','M','Y',1};
            assert(fwrite(trailer,1,12,f)==12);
        }
        assert(fclose(f)==0);
        assert(copyover_has_durable_shopkeepers() == (version>=13));
    }
    FILE *f=fopen(COPYOVER_FILE,"w+b"); assert(f);
    copyover_header h={}; const int listeners[3]={-1,-1,-1};
    assert(copyover_codec_begin(f,h,listeners));
    assert(copyover_codec_finish(f,nullptr)); assert(fclose(f)==0);
    assert(copyover_has_durable_shopkeepers());
    f=fopen(COPYOVER_FILE,"r+b"); assert(f);
    assert(fseek(f,32,SEEK_SET)==0); assert(fputc(0,f)!=EOF); assert(fclose(f)==0);
    assert(!copyover_has_durable_shopkeepers());
    assert(remove(COPYOVER_FILE)==0);
    puts("copyover versions 12-18: validated durable shopkeeper provenance PASS");
}
'''
with tempfile.TemporaryDirectory(prefix="duris-shop-copyover-") as tmp:
    p=Path(tmp); (p/"test.cpp").write_text(program)
    subprocess.run(["g++","-std=c++20","-Wall","-Wextra","-Werror","-I",str(ROOT/"src"),str(p/"test.cpp"),
                    "src/persistence/copyover_codec.c", "src/world/world_recovery_codec.c",
                    "src/world/generated_npc_state.c", "src/player/pet_restore_state.c",
                    "src/item/item_transfer_command.c", "src/world/quest_mobile_native_reference.c", "src/economy/economic_source_event.c", "-ffunction-sections", "-fdata-sections",
                    "-Wl,--gc-sections", '-lcrypto', "-o",str(p/"test")],cwd=ROOT,check=True)
    subprocess.run([str(p/"test")],cwd=p,check=True)
