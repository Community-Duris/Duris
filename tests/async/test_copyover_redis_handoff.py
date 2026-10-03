#!/usr/bin/env python3
"""Execute the production Redis exec handoff and failed-exec resume paths."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, extract_function

code = r'''
#include <cassert>
#include <cstdint>
#include <string>
static bool world_enabled, world_recovery_quiesced, initialized;
static bool recovery_ok, flush_ok, floor_ok, release_ok, floor_quiesced;
static int cancelled, released, retried;
static std::string world_writer_token;
static uint64_t world_writer_lease_msec, world_writer_epoch;
struct redis_world_store_config {};
struct health { bool initialized; };
health world_recovery_pipeline_health_copy() { return {initialized}; }
bool redis_world_recovery_drain(uint64_t timeout) { assert(timeout==3000); return recovery_ok; }
bool redis_flush_floor_drops() { return flush_ok; }
bool redis_floor_store_drain(uint64_t timeout) { assert(timeout==3000); return floor_ok; }
void redis_floor_runtime_set_quiesced(bool value) { floor_quiesced=value; }
void redis_world_writer_retry_cancel() {}
void world_recovery_pipeline_cancel() { ++cancelled; initialized=false; }
redis_world_store_config redis_world_store_config_copy() { return {}; }
bool redis_world_store_release_fence(const redis_world_store_config *, const char *token) {
    assert(!initialized && floor_quiesced && std::string(token)=="old-token");
    ++released; return release_ok;
}
void redis_world_writer_retry_schedule(const char *) { ++retried; }
''' + extract_function('redis_world_runtime.c', 'void redis_world_recovery_resume_after_copyover(') + '\n' + extract_function('redis_world_runtime.c', 'bool redis_world_recovery_prepare_copyover(') + r'''
void reset() {
    world_enabled=initialized=recovery_ok=flush_ok=floor_ok=release_ok=true;
    world_recovery_quiesced=floor_quiesced=false;
    cancelled=released=retried=0; world_writer_token="old-token";
    world_writer_lease_msec=600000; world_writer_epoch=1;
}
int main() {
    reset(); world_enabled=false; assert(redis_world_recovery_prepare_copyover());
    assert(!released && !cancelled);
    for (int stage=0; stage<3; ++stage) {
        reset(); if(stage==0) recovery_ok=false; if(stage==1) flush_ok=false; if(stage==2) floor_ok=false;
        assert(!redis_world_recovery_prepare_copyover());
        assert(!released && !cancelled && !world_recovery_quiesced);
    }
    reset(); release_ok=false; assert(!redis_world_recovery_prepare_copyover());
    assert(released==1 && cancelled==1 && retried==1 && !world_recovery_quiesced);
    assert(world_writer_token=="old-token");
    reset(); assert(redis_world_recovery_prepare_copyover());
    assert(released==1 && cancelled==1 && world_recovery_quiesced && world_writer_token.empty());
    assert(!world_writer_epoch && !world_writer_lease_msec);
    redis_world_recovery_resume_after_copyover();
    assert(!world_recovery_quiesced && floor_quiesced && retried==1);
    reset(); world_writer_token.clear(); assert(redis_world_recovery_prepare_copyover());
    assert(!released && cancelled==1);
}
'''
with tempfile.TemporaryDirectory(prefix='copyover-redis-handoff-') as directory:
    path = Path(directory)
    (path/'test.cpp').write_text(code)
    subprocess.run(['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    str(path/'test.cpp'), '-o', str(path/'test')], cwd=ROOT, check=True)
    subprocess.run([str(path/'test')], check=True)
print('copyover Redis drain, release, refusal and failed-exec resume passed')
