#!/usr/bin/env python3
"""Compile production dispatcher/shutdown: a quarantined backlog must not hang.

The real journal and terminal-quarantine hook are used. Worker shutdown is a
no-op fixture; no SQL/game service runs, and replay starts with an empty journal.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (ROOT / 'src/player/player_save_pipeline.c').read_text()


def section(start, end):
    first = PIPELINE.index(start)
    return PIPELINE[first:PIPELINE.index(end, first)]


GLOBALS = section('std::mutex pipeline_mutex;', 'player_save_apply_fn selected_snapshot_apply()')
assert GLOBALS.count('std::deque<player_snapshot> pending_append;') == 1
GLOBALS = GLOBALS.replace('std::deque<player_snapshot> pending_append;',
                          'std::deque<player_snapshot, pending_allocator<player_snapshot>> pending_append;')
FENCES = section('struct terminal_fence', '/** Refresh queue depth')
UPDATE = section('void update_depth_locked()', '/** Replay the journal')
DISPATCH = section('void dispatcher_main()', '/** Check retained queues for an exact')
SHUTDOWN = section('void player_save_pipeline_shutdown(void)', '/** Mark player components dirty')
HARNESS = r'''
#include "core/defines.h"
#include "player/player_save_pipeline.h"
#include "player/player_save_journal.h"
#include "player/player_snapshot_codec.h"
#include "net/network_wakeup.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <new>
#include <poll.h>
#include <set>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

bool player_revision_unpin_terminal_death(int, player_revision_t) { return true; }

std::atomic<bool> fail_pending_allocation{false};
std::atomic<unsigned> observed_requeue_failures{0};
// Fault the real deque allocation, not journal encoding or snapshot movement.
// This fixture uses g++/libstdc++, whose deque has one slot for this large type.
static_assert(sizeof(player_snapshot) > 256);
template<class T> struct pending_allocator {
    using value_type = T;
    pending_allocator() = default;
    template<class U> pending_allocator(const pending_allocator<U> &) {}
    T *allocate(size_t count) {
        if (fail_pending_allocation.exchange(false)) {
            ++observed_requeue_failures;
            throw std::bad_alloc();
        }
        return std::allocator<T>{}.allocate(count);
    }
    void deallocate(T *data, size_t count) { std::allocator<T>{}.deallocate(data, count); }
    template<class U> bool operator==(const pending_allocator<U> &) const { return true; }
};
@GLOBALS@
@FENCES@
@UPDATE@
player_save_apply_result unused_apply(const player_snapshot &s, void *) {
    return {player_save_apply_outcome::applied, s.revision, 0};
}
player_save_apply_fn selected_snapshot_apply() { return unused_apply; }
void player_save_worker_shutdown() {} // Only dispatcher join is under test.
@DISPATCH@
@SHUTDOWN@
std::atomic<bool> fail_directory_sync{false};
std::atomic<bool> fail_archive_data_sync{false};
extern "C" int __real_fdatasync(int);
extern "C" int __wrap_fdatasync(int fd) {
    if (fail_archive_data_sync.exchange(false)) {
        fail_pending_allocation=true;
        errno=EIO;
        return -1;
    }
    return __real_fdatasync(fd);
}
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    struct stat status{};
    if (fstat(fd, &status) == 0 && S_ISDIR(status.st_mode) &&
        fail_directory_sync.exchange(false)) { errno=EIO; return -1; }
    return __real_fsync(fd);
}
std::vector<unsigned char> bytes(const std::string &path) {
    std::ifstream f(path, std::ios::binary); assert(f.good());
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(f), {});
}
player_snapshot snapshot(int pid, unsigned revision) {
    player_snapshot s{};
    s.schema_version=PLAYER_SNAPSHOT_SCHEMA_VERSION;
    s.pid=pid; s.revision=revision; s.components=PLAYER_COMPONENT_STATUS;
    s.save_intent=4; s.room_vnum=1201; s.encoded_size_bound=8192;
    s.status_strings.push_back({player_status_string_field::name, "synthetic-pending-"+std::to_string(revision)});
    return s;
}
void run(const std::string &directory, bool inject_sync_failure, bool inject_requeue_failure=false) {
    assert(network_wakeup_fd() >= 0);
    network_wakeup_drain();
    stop_requested=false; health={};
    observed_requeue_failures=0;
    assert(player_save_journal_init(directory.c_str()));
    auto first=snapshot(9001,1), pending=snapshot(9001,2), healthy=snapshot(9002,1);
    assert(player_save_journal_append(first)==player_save_journal_result::ok);
    const auto first_frame=bytes(directory+"/player-save.journal");
    player_save_journal_worker_terminal(first,nullptr);
    assert(player_save_journal_pid_quarantined(9001));
    assert(player_save_journal_health_copy().records==0);
    std::vector<uint8_t> pending_payload;
    assert(player_snapshot_encode(pending,&pending_payload)==player_snapshot_codec_result::ok);
    dispatcher=std::thread(dispatcher_main);
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
    for (;;) {
        { std::lock_guard<std::mutex> lock(pipeline_mutex); if (health.replay_complete) break; }
        assert(std::chrono::steady_clock::now()<deadline);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    {
        std::lock_guard<std::mutex> lock(pipeline_mutex);
        fail_directory_sync=inject_sync_failure;
        pending_append.push_back(pending); pending_append.push_back(healthy);
        fail_archive_data_sync=inject_requeue_failure;
        retained_bytes=pending.encoded_size_bound+healthy.encoded_size_bound;
    }
    append_available.notify_all();
    // The former implementation requeues the rejected first entry forever and
    // hangs this production join. The subprocess timeout makes that observable.
    player_save_pipeline_shutdown();
    pollfd wakeup{network_wakeup_fd(), POLLIN, 0};
    assert(poll(&wakeup, 1, 0) == 1 && (wakeup.revents & POLLIN));
    network_wakeup_drain();
    wakeup.revents = 0;
    assert(poll(&wakeup, 1, 0) == 0);
    assert(observed_requeue_failures==(inject_requeue_failure ? 1U : 0U));
    assert(retained_bytes==0 && pending_append.empty());
    const auto archive=bytes(directory+"/player-save.journal.quarantine.archive");
    assert(std::search(archive.begin(),archive.end(),first_frame.begin(),first_frame.end())!=archive.end());
    assert(std::search(archive.begin(),archive.end(),pending_payload.begin(),pending_payload.end())!=archive.end());
    assert(player_save_journal_init(directory.c_str()));
    assert(player_save_journal_pid_quarantined(9001));
    assert(!player_save_journal_pid_quarantined(9002));
    assert(player_save_journal_health_copy().records==1); // healthy snapshot not starved
    assert(player_save_journal_health_copy().checkpoints==0); // no fake ACK
    assert(player_save_journal_append(pending)==player_save_journal_result::quarantined_pid);
    player_save_journal_shutdown();
}
int main(int argc,char **argv) {
    assert(argc==2);
    run(std::string(argv[1])+"/ordinary",false);
    run(std::string(argv[1])+"/sync-retry",true);
    run(std::string(argv[1])+"/archive-and-allocation-retry",false,true);
}
'''
for name, value in [('GLOBALS', GLOBALS), ('FENCES', FENCES), ('UPDATE', UPDATE),
                    ('DISPATCH', DISPATCH), ('SHUTDOWN', SHUTDOWN)]:
    HARNESS = HARNESS.replace('@' + name + '@', value)

with tempfile.TemporaryDirectory(prefix='duris-quarantined-dispatcher-') as temp:
    source = Path(temp) / 'probe.cpp'
    binary = Path(temp) / 'probe'
    source.write_text(HARNESS)
    result = subprocess.run(['g++','-std=c++20','-g','-Og','-Wall','-Wextra','-Werror','-pthread',
                             '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie',
                             '-D__NO_MYSQL__','-Isrc',str(source),
                             'src/player/player_save_journal.c','src/player/player_snapshot_codec.c',
                             '-Wl,--wrap=fsync','-Wl,--wrap=fdatasync','-o',str(binary)],
                            cwd=ROOT,capture_output=True,text=True)
    if result.returncode:
        raise RuntimeError(result.stderr)
    subprocess.run([str(binary), temp], cwd=ROOT, check=True, timeout=8)
print('[PASS] quarantined pending snapshot is archived without ACK; shutdown returns, '
      'healthy backlog proceeds, original bytes survive, archive sync retry preserves capture; '
      'archive failure plus deque allocation failure never drops a capture')
