#!/usr/bin/env python3
"""Real worker/journal terminal failures fence admission before completion consumption.

Repository results are synthetic; no DB or live service is accessed. Admission
functions and hook registration come from production; the unrelated target-save
fence lookup is empty in this isolated harness.
"""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PIPELINE = (ROOT / "src/player/player_save_pipeline.c").read_text()
LOAD = (ROOT / "src/player/player_load_pipeline.c").read_text()
registration = re.search(r"player_save_worker_set_journal_hooks\([^;]*?\)", PIPELINE)
assert registration is not None
HOOKS = registration.group()


def function(source, signature):
    first = source.index(signature)
    return source[first : source.index("\n}", first) + 2]


ADMISSION = function(LOAD, "bool player_load_pipeline_login_admit(int pid)") + "\n" + function(
    PIPELINE, "bool player_save_pipeline_save_admitted(int pid)"
)
HARNESS = r'''
#include "player/player_save_journal.h"
#include "player/player_revision_state.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <fstream>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

std::mutex pipeline_mutex;
void *find_target_save_login_fence_locked(int) { return nullptr; }
struct terminal_fence { bool death_pinned = false; };
terminal_fence *find_terminal_fence_locked(int) { return nullptr; }
@ADMISSION@
std::atomic<bool> fail_directory_sync{false};
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    struct stat status{};
    if (fstat(fd, &status) == 0 && S_ISDIR(status.st_mode) &&
        fail_directory_sync.exchange(false)) { errno = EIO; return -1; }
    return __real_fsync(fd);
}
std::vector<char> bytes(const std::string &path) {
    std::ifstream file(path, std::ios::binary); assert(file.good());
    return std::vector<char>(std::istreambuf_iterator<char>(file), {});
}
struct result_state { unsigned error; bool throws; bool retry; };
player_save_apply_result apply(const player_snapshot &s, void *raw) {
    const auto &state = *static_cast<result_state *>(raw);
    if (s.pid != 9001) return {player_save_apply_outcome::applied, s.revision, 0};
    if (state.throws) throw std::runtime_error("synthetic repository exception");
    return {state.retry ? player_save_apply_outcome::retryable_failure :
                         player_save_apply_outcome::terminal_failure, 0, state.error};
}
player_snapshot snapshot(int pid) {
    assert(player_revision_hydrate(pid, 0));
    player_snapshot s{};
    s.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    s.pid = pid;
    assert(player_revision_mark(pid, PLAYER_COMPONENT_STATUS, &s.revision));
    assert(player_revision_queue(pid, &s.revision, &s.components));
    s.save_intent = 4; s.room_vnum = 1201; s.encoded_size_bound = 8192;
    s.status_strings.push_back({player_status_string_field::name, "synthetic-live-failure"});
    return s;
}
void run(const std::string &directory, result_state state, bool sync_failure) {
    player_save_worker_reset_for_tests();
    player_revision_reset_for_tests();
    assert(player_save_journal_init(directory.c_str()));
    auto failed = snapshot(9001);
    auto healthy = snapshot(9002);
    assert(player_save_journal_append(failed) == player_save_journal_result::ok);
    const auto failed_frame = bytes(directory + "/player-save.journal");
    if (!sync_failure)
        assert(player_save_journal_append(healthy) == player_save_journal_result::ok);
    const auto original = bytes(directory + "/player-save.journal");
    assert(@HOOKS@);
    assert(player_save_worker_init(apply, &state, 1));
    fail_directory_sync = sync_failure;
    assert(player_save_worker_submit(failed) == player_save_submit_result::accepted);
    if (state.retry) {
        // Consume only the first eight retryable completions. The final worker
        // attempt must fence before its completion is consumed.
        for (unsigned index = 0; index < PLAYER_SAVE_WORKER_MAX_RETRIES; ++index) {
            player_save_completion completion{};
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (!player_save_worker_pulse(&completion, 1)) {
                assert(std::chrono::steady_clock::now() < deadline);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            assert(completion.outcome == player_save_apply_outcome::retryable_failure);
        }
    }
    if (!sync_failure)
        assert(player_save_worker_submit(healthy) == player_save_submit_result::accepted);
    player_save_worker_shutdown();

    // No game-thread completion has consumed the terminal result yet.
    assert(player_save_journal_pid_quarantined(9001));
    const auto diagnosis = player_save_journal_diagnostic_copy(9001);
    assert(diagnosis.available && !diagnosis.policy_fence);
    assert(diagnosis.pid_fence == !sync_failure);
    assert(diagnosis.global_fence == sync_failure);
    assert(diagnosis.archived_frames == (sync_failure ? 0U : 1U));
    assert(diagnosis.archived_bytes == (sync_failure ? 0U : failed_frame.size()));
    assert(!diagnosis.recovery_prepared);
    assert(!player_load_pipeline_login_admit(9001));
    assert(!player_save_pipeline_save_admitted(9001));
    player_revision_snapshot revision{};
    assert(player_revision_snapshot_copy(9001, &revision));
    assert(revision.acknowledged_revision == 0); // no fabricated receipt
    assert(player_save_journal_append(failed) != player_save_journal_result::ok);
    if (sync_failure) {
        assert(!player_load_pipeline_login_admit(9002));
        assert(!player_save_pipeline_save_admitted(9002));
        assert(bytes(directory + "/player-save.journal") == original);
    } else {
        assert(player_load_pipeline_login_admit(9002));
        assert(player_save_pipeline_save_admitted(9002));
        assert(player_save_journal_health_copy().records == 0);
    }
    const auto archive = bytes(directory + "/player-save.journal.quarantine.archive");
    assert(std::search(archive.begin(), archive.end(), failed_frame.begin(), failed_frame.end())
           != archive.end());
    player_save_worker_reset_for_tests();
    player_save_journal_shutdown();
    assert(player_save_journal_init(directory.c_str()));
    assert(!player_load_pipeline_login_admit(9001));
    assert(!player_save_pipeline_save_admitted(9001));
    assert(player_load_pipeline_login_admit(9002));
    const auto restored_diagnosis = player_save_journal_diagnostic_copy(9001);
    assert(restored_diagnosis.available && restored_diagnosis.pid_fence);
    assert(restored_diagnosis.archived_frames >= 1);
    assert(restored_diagnosis.archived_bytes >= failed_frame.size());
    if (!sync_failure) {
        assert(restored_diagnosis.archived_frames == diagnosis.archived_frames);
        assert(restored_diagnosis.archived_bytes == diagnosis.archived_bytes);
    }
    player_save_journal_shutdown();
}
int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string root = argv[1];
    run(root + "/terminal", {EINVAL, false, false}, false);
    run(root + "/missing", {ENOENT, false, false}, false);
    run(root + "/custody", {PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH, false, false}, false);
    run(root + "/exception", {EFAULT, true, false}, false);
    run(root + "/sync-failure", {EINVAL, false, false}, true);
    run(root + "/exhaustion", {1205, false, true}, false);
}
'''.replace("@ADMISSION@", ADMISSION).replace("@HOOKS@", HOOKS)

with tempfile.TemporaryDirectory(prefix="duris-live-failure-quarantine-") as temp:
    source = Path(temp) / "probe.cpp"
    binary = Path(temp) / "probe"
    source.write_text(HARNESS)
    command = [
        "g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror", "-pthread",
        "-Isrc", str(source), "src/player/player_save_worker.c",
        "src/player/player_save_journal.c", "src/player/player_revision_state.c",
        "src/player/player_snapshot_codec.c", "src/persistence/persistence_observability.c",
        "-Wl,--wrap=fsync", "-lmysqlclient", "-o", str(binary),
    ]
    compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if compiled.returncode:
        raise RuntimeError(compiled.stderr)
    subprocess.run([str(binary), temp], cwd=ROOT, check=True, timeout=40)
print("[PASS] live terminal/exception/exhaustion failures fence login and save before completion; "
      "exact bytes survive restart, healthy PID proceeds, sync failure stays fail-closed")
