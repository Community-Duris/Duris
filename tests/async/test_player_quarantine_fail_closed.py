#!/usr/bin/env python3
"""Synthetic corruption must preserve bytes and fail all admission, including restart."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "player/player_save_journal.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <new>
#include <fcntl.h>
#include <unistd.h>
#include <vector>
int fail_allocation_after = -1;
bool allocation_failed = false;
extern "C" void *__real__Znwm(size_t);
extern "C" void *__wrap__Znwm(size_t size) {
    if (fail_allocation_after >= 0 && fail_allocation_after-- == 0) {
        allocation_failed = true;
        throw std::bad_alloc();
    }
    return __real__Znwm(size);
}
std::vector<char> bytes(const std::string &p) {
    std::ifstream f(p, std::ios::binary); assert(f.good());
    return std::vector<char>(std::istreambuf_iterator<char>(f), {});
}
int main(int argc,char **argv) {
    assert(argc==2);
    std::string dir=argv[1], path=dir+"/player-save.journal";
    assert(player_save_journal_init(dir.c_str()));
    player_snapshot s{};
    s.schema_version=PLAYER_SNAPSHOT_SCHEMA_VERSION;
    s.pid=9001; s.revision=1; s.components=PLAYER_CHECKPOINT_COMPONENT_ALL;
    s.save_intent=4; s.room_vnum=1201; s.encoded_size_bound=8192;
    s.status_integers.push_back({player_status_field::level,50,0,false});
    s.status_strings.push_back({player_status_string_field::name,"synthetic-probe"});
    assert(player_save_journal_append(s)==player_save_journal_result::ok);
    player_save_journal_shutdown();
    const auto before_allocation = bytes(path);
    unsigned allocation_cases = 0;
    for (int failure_point = 0; failure_point < 100; ++failure_point) {
        fail_allocation_after = failure_point;
        allocation_failed = false;
        const bool initialized = player_save_journal_init(dir.c_str());
        fail_allocation_after = -1;
        if (!allocation_failed) {
            assert(initialized);
            player_save_journal_shutdown();
            break;
        }
        ++allocation_cases;
        assert(!initialized && !player_save_journal_health_copy().initialized);
        assert(player_save_journal_pid_quarantined(9001));
        assert(player_save_journal_pid_quarantined(9002));
        assert(bytes(path) == before_allocation);
        assert(access((dir + "/player-save.journal.quarantine.archive").c_str(), F_OK) != 0);
        player_save_journal_shutdown();
    }
    assert(allocation_cases >= 3);
    int fd=open(path.c_str(),O_RDWR); assert(fd>=0);
    unsigned char b=0; assert(pread(fd,&b,1,80)==1); b^=1;
    assert(pwrite(fd,&b,1,80)==1); assert(fsync(fd)==0); close(fd);
    const auto original=bytes(path);
    assert(!player_save_journal_init(dir.c_str()));
    assert(player_save_journal_pid_quarantined(9001));
    assert(player_save_journal_pid_quarantined(9002));
    assert(bytes(path)==original);
    player_save_journal_shutdown();
    assert(!player_save_journal_init(dir.c_str()));
    assert(bytes(path)==original);
    player_save_journal_shutdown();
    // Removing the active copy in this disposable fixture must not release the
    // fence: the persistent archive still contains unassigned corrupt evidence.
    assert(truncate(path.c_str(),0)==0);
    assert(!player_save_journal_init(dir.c_str()));
    assert(player_save_journal_pid_quarantined(9001));
    assert(player_save_journal_pid_quarantined(9002));
}
'''
with tempfile.TemporaryDirectory(prefix='duris-quarantine-refusal-') as td:
    source=Path(td)/'probe.cpp'; binary=Path(td)/'probe'
    source.write_text(HARNESS)
    subprocess.run(['g++','-std=c++20','-O2','-pthread','-Wl,--wrap=_Znwm','-Isrc',str(source),
                    'src/player/player_snapshot_codec.c','src/player/player_save_journal.c',
                    '-o',str(binary)],cwd=ROOT,check=True,capture_output=True)
    subprocess.run([str(binary),str(Path(td)/'journal')],cwd=ROOT,check=True,timeout=30)
print('[PASS] allocation failures retain valid bytes without corruption archives; unassigned corrupt evidence fences every PID across restart')
