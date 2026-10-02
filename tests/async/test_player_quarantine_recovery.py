#!/usr/bin/env python3
"""Execute the archive recovery protocol; backend proof is deliberately mocked here."""

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "player/player_save_journal.h"
#include "player/player_snapshot_codec.h"
#include <cassert>
#include <cerrno>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int fault = 0;
extern "C" int __real_fdatasync(int);
extern "C" int __wrap_fdatasync(int fd) {
    if (fault == 1) { fault = 0; errno = ENOSPC; return -1; }
    return __real_fdatasync(fd);
}
extern "C" int __real_fsync(int);
extern "C" int __wrap_fsync(int fd) {
    if (fault == 2) { fault = 0; errno = EIO; return -1; }
    return __real_fsync(fd);
}
extern "C" int __real_rename(const char *, const char *);
extern "C" int __wrap_rename(const char *from, const char *to) {
    if (fault == 3) { kill(getpid(), SIGKILL); }
    int result = __real_rename(from, to);
    if (fault == 4 && result == 0) { kill(getpid(), SIGKILL); }
    return result;
}
static std::vector<uint8_t> persisted;
static bool exact_result(const player_save_recovery_record &record, void *) {
    std::vector<uint8_t> expected;
    return player_snapshot_encode(record.replacement, &expected) == player_snapshot_codec_result::ok &&
        persisted == expected;
}
static bool throwing_proof(const player_save_recovery_record &, void *) { throw 1; }
static player_save_apply_result healthy_apply(const player_snapshot &value, void *context) {
    assert(value.pid == 9002);
    ++*static_cast<int *>(context);
    return {player_save_apply_outcome::applied, value.revision, 0};
}
static std::vector<char> bytes(const std::string &path) {
    std::ifstream in(path, std::ios::binary); assert(in.good());
    return {std::istreambuf_iterator<char>(in), {}};
}
static player_snapshot snapshot(int pid, uint64_t revision, uint64_t components) {
    player_snapshot result{};
    result.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    result.pid = pid; result.revision = revision; result.components = components;
    result.encoded_size_bound = 8192;
    result.status_strings.push_back({player_status_string_field::name, "synthetic-recovery"});
    return result;
}
static player_save_recovery_record seed(const std::string &directory) {
    std::filesystem::create_directory(directory); chmod(directory.c_str(), 0700);
    assert(player_save_journal_init(directory.c_str()));
    auto first = snapshot(9001, 2, PLAYER_COMPONENT_STATUS);
    auto later = snapshot(9001, 3, PLAYER_COMPONENT_SKILLS);
    later.skills.push_back({42, 75, 50});
    assert(player_save_journal_append(first) == player_save_journal_result::ok);
    assert(player_save_journal_append(later) == player_save_journal_result::ok);
    const auto raw = bytes(directory + "/player-save.journal");
    player_save_journal_worker_terminal(first, nullptr);
    const auto archive = bytes(directory + "/player-save.journal.quarantine.archive");
    assert(std::search(archive.begin(), archive.end(), raw.begin(), raw.end()) != archive.end());
    std::vector<player_snapshot> frames;
    player_save_recovery_record record;
    assert(player_save_journal_recovery_inspect(9001, &frames, &record.archive_digest) == player_save_journal_result::ok);
    assert(frames.size() == 2 && frames[1].skills.front().learned == 75);
    record.identity[0] = 1; record.backend = 1; record.backend_identity = "synthetic-lineage";
    record.account_name = "synthetic-account";
    record.baseline = snapshot(9001, 1, PLAYER_CHECKPOINT_COMPONENT_ALL);
    record.replacement = snapshot(9001, 4, PLAYER_CHECKPOINT_COMPONENT_ALL);
    record.replacement.skills = later.skills;
    // Opaque input here; the native backend owner must validate actual commands.
    record.creation_commands = {{1, 2, 3}}; record.authority_evidence = {4, 5, 6};
    assert(!player_save_journal_pid_quarantined(9002));
    return record;
}
static void reopen(const std::string &directory) {
    player_save_journal_shutdown();
    assert(player_save_journal_init(directory.c_str()));
    assert(player_save_journal_pid_quarantined(9001));
    assert(!player_save_journal_pid_quarantined(9002));
}
int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string root = argv[1];
    auto record = seed(root + "/normal");
    auto wrong = record; wrong.archive_digest[0] ^= 1;
    assert(player_save_journal_recovery_prepare(wrong) == player_save_journal_result::replay_blocked);
    assert(player_save_journal_recovery_prepare(record) == player_save_journal_result::ok);
    wrong = record; wrong.creation_commands.assign(65, {1});
    assert(player_save_journal_recovery_prepare(wrong) == player_save_journal_result::replay_blocked);
    assert(player_save_journal_recovery_resolve(record, throwing_proof, nullptr) == player_save_journal_result::replay_blocked);
    assert(player_save_journal_recovery_prepare(record) == player_save_journal_result::ok);
    wrong = record; wrong.replacement.skills.front().learned = 76;
    assert(!player_save_journal_recovery_matches(wrong));
    assert(player_save_journal_recovery_prepare(wrong) == player_save_journal_result::replay_blocked);
    assert(player_save_journal_recovery_resolve(record, exact_result, nullptr) == player_save_journal_result::replay_blocked);
    assert(player_snapshot_encode(wrong.replacement, &persisted) == player_snapshot_codec_result::ok);
    assert(player_save_journal_recovery_resolve(record, exact_result, nullptr) == player_save_journal_result::replay_blocked);
    assert(player_snapshot_encode(record.replacement, &persisted) == player_snapshot_codec_result::ok);
    reopen(root + "/normal");
    player_save_recovery_record reloaded; bool resolved = true;
    assert(player_save_journal_recovery_read(9001, &reloaded, &resolved) == player_save_journal_result::ok && !resolved);
    assert(player_save_journal_recovery_resolve(reloaded, exact_result, nullptr) == player_save_journal_result::ok);
    assert(!player_save_journal_pid_quarantined(9001));
    auto pending = snapshot(9001, 5, PLAYER_COMPONENT_SKILLS);
    auto healthy = snapshot(9002, 1, PLAYER_COMPONENT_SKILLS);
    assert(player_save_journal_append(pending) == player_save_journal_result::ok);
    assert(player_save_journal_append(healthy) == player_save_journal_result::ok);
    reopen(root + "/normal");
    int healthy_calls = 0;
    assert(player_save_journal_replay(healthy_apply, &healthy_calls) == player_save_journal_result::ok);
    assert(healthy_calls == 1 && player_save_journal_health_copy().records == 1);
    assert(player_save_journal_recovery_read(9001, &reloaded, &resolved) == player_save_journal_result::ok && resolved);
    persisted.clear(); // Simulate restoring the old backend beside the new archive.
    assert(player_save_journal_recovery_resolve(reloaded, exact_result, nullptr) == player_save_journal_result::replay_blocked);
    assert(player_save_journal_pid_quarantined(9001));
    assert(player_snapshot_encode(record.replacement, &persisted) == player_snapshot_codec_result::ok);
    assert(player_save_journal_recovery_resolve(reloaded, exact_result, nullptr) == player_save_journal_result::ok);
    assert(player_save_journal_checkpoint(9001, 5) == player_save_journal_result::ok);
    auto fresh = snapshot(9001, 5, PLAYER_COMPONENT_STATUS);
    assert(player_save_journal_append(fresh) == player_save_journal_result::ok);
    player_save_journal_worker_terminal(fresh, nullptr);
    assert(!player_save_journal_recovery_matches(record));
    reopen(root + "/normal");
    assert(player_save_journal_recovery_read(9001, &reloaded, &resolved) == player_save_journal_result::replay_blocked);
    player_save_journal_shutdown();

    for (int stage : {1, 2}) {
        const std::string directory = root + "/io-" + std::to_string(stage);
        record = seed(directory);
        fault = stage;
        assert(player_save_journal_recovery_prepare(record) == player_save_journal_result::io_failure);
        assert(player_save_journal_pid_quarantined(9001));
        reopen(directory);
        auto read = player_save_journal_recovery_read(9001, &reloaded, &resolved);
        assert(read == player_save_journal_result::ok || read == player_save_journal_result::replay_blocked);
        assert(player_save_journal_recovery_prepare(record) == player_save_journal_result::ok);
        assert(player_snapshot_encode(record.replacement, &persisted) == player_snapshot_codec_result::ok);
        fault = stage;
        assert(player_save_journal_recovery_resolve(record, exact_result, nullptr) == player_save_journal_result::io_failure);
        assert(player_save_journal_pid_quarantined(9001));
        reopen(directory);
        assert(player_save_journal_recovery_resolve(record, exact_result, nullptr) == player_save_journal_result::ok);
        player_save_journal_shutdown();
    }
    for (int stage : {3, 4}) {
        const std::string directory = root + "/crash-" + std::to_string(stage);
        record = seed(directory);
        player_save_journal_shutdown();
        pid_t child = fork(); assert(child >= 0);
        if (!child) {
            assert(player_save_journal_init(directory.c_str()));
            fault = stage;
            player_save_journal_recovery_prepare(record);
            _exit(2);
        }
        int status = 0; assert(waitpid(child, &status, 0) == child);
        assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL);
        assert(player_save_journal_init(directory.c_str()));
        assert(player_save_journal_pid_quarantined(9001));
        assert(player_save_journal_recovery_prepare(record) == player_save_journal_result::ok);
        assert(player_snapshot_encode(record.replacement, &persisted) == player_snapshot_codec_result::ok);
        player_save_journal_shutdown();
        child = fork(); assert(child >= 0);
        if (!child) {
            assert(player_save_journal_init(directory.c_str()));
            fault = stage;
            player_save_journal_recovery_resolve(record, exact_result, nullptr);
            _exit(2);
        }
        assert(waitpid(child, &status, 0) == child && WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL);
        reopen(directory);
        assert(player_save_journal_recovery_resolve(record, exact_result, nullptr) == player_save_journal_result::ok);
        player_save_journal_shutdown();
    }
    puts("PASS: durable prepare/resolve, exact proof refusal, restart revalidation, revocation, disk faults and actual SIGKILL rename boundaries");
}
'''

with tempfile.TemporaryDirectory(prefix="duris-quarantine-recovery-") as temporary:
    folder = Path(temporary)
    program = folder / "recovery.cpp"
    binary = folder / "recovery"
    program.write_text("#include <algorithm>\n" + HARNESS)
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread",
        "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-Isrc", str(program), "src/player/player_save_journal.c", "src/player/player_snapshot_codec.c",
        "-Wl,--wrap=fdatasync", "-Wl,--wrap=fsync", "-Wl,--wrap=rename", "-o", str(binary),
    ], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(folder)], cwd=ROOT, check=True, timeout=45)
