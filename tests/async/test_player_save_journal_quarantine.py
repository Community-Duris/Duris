#!/usr/bin/env python3
"""Fail-closed quarantine integration with a generated, copied synthetic journal.

The supplied alias index is private and is read only to create owner-only test
policy files in a temporary directory. No raw PID values are printed or stored
in the repository.
"""

import argparse
import binascii
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shutil
import stat
import struct
import subprocess
import tempfile
from types import SimpleNamespace
from typing import TypedDict

ROOT = Path(__file__).resolve().parents[2]
EXPECTED_CAPTURE_SHA256 = "99c7d7e555df83bbda6d092972b6915c0c3370d12efe239f22e4521a52691499"
FRAME_MAGIC = b"DPSJNL1\0"
FRAME_HEADER_SIZE = 72
ARCHIVE_MAGIC = b"DPQARC1\0"
ARCHIVE_HEADER_SIZE = 28
ARCHIVE_ENTRY_SIZE = 56
KNOWN_ACTIVE_ALIASES = {"PID-003", "PID-004"}


class Frame(TypedDict):
    pid: int
    revision: int
    components: int
    record_id: int
    payload_crc: int
    raw: bytes

HARNESS = r'''#include "player/player_save_journal.h"
#include "player/player_snapshot_codec.h"
#include "classes/necromancy.h"
#include "world/vnum.obj.h"
#include "core/defines.h"

#include <cassert>
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

constexpr size_t FRAME_HEADER_SIZE = 72;
const std::vector<uint8_t> FRAME_MAGIC = {'D', 'P', 'S', 'J', 'N', 'L', '1', 0};

bool fail_next_fdatasync = false;
extern "C" int __real_fdatasync(int fd);
extern "C" int __wrap_fdatasync(int fd)
{
    if (fail_next_fdatasync) {
        fail_next_fdatasync = false;
        errno = EIO;
        return -1;
    }
    return __real_fdatasync(fd);
}

std::vector<uint8_t> read_bytes(const std::string &path)
{
    std::ifstream input(path, std::ios::binary);
    assert(input.good());
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
}

std::vector<int> read_ids(const std::string &path)
{
    std::ifstream input(path);
    assert(input.good());
    std::vector<int> ids;
    int pid = 0;
    while (input >> pid)
        ids.push_back(pid);
    assert(input.eof() && !ids.empty());
    return ids;
}

uint32_t u32(const uint8_t *bytes, size_t offset)
{
    uint32_t value = 0;
    for (unsigned int index = 0; index < 4; ++index)
        value |= static_cast<uint32_t>(bytes[offset + index]) << (index * 8);
    return value;
}

uint64_t u64(const uint8_t *bytes, size_t offset)
{
    uint64_t value = 0;
    for (unsigned int index = 0; index < 8; ++index)
        value |= static_cast<uint64_t>(bytes[offset + index]) << (index * 8);
    return value;
}

uint32_t crc32(const std::vector<uint8_t> &bytes)
{
    uint32_t crc = UINT32_MAX;
    for (uint8_t byte : bytes) {
        crc ^= byte;
        for (unsigned int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (0U - (crc & 1U)));
    }
    return ~crc;
}

struct decoded_frame
{
    player_snapshot snapshot;
    uint64_t record_id;
    uint32_t payload_checksum;
};

std::vector<decoded_frame> decode_snapshots(const std::vector<uint8_t> &journal)
{
    std::vector<decoded_frame> snapshots;
    size_t offset = 0;
    while (offset < journal.size()) {
        assert(journal.size() - offset >= FRAME_HEADER_SIZE);
        assert(std::equal(FRAME_MAGIC.begin(), FRAME_MAGIC.end(), journal.begin() + offset));
        const uint64_t size = u64(journal.data() + offset, 16);
        const uint32_t payload_size = u32(journal.data() + offset, 64);
        assert(size == FRAME_HEADER_SIZE + payload_size && size <= journal.size() - offset);
        player_snapshot snapshot = {};
        assert(player_snapshot_decode(journal.data() + offset + FRAME_HEADER_SIZE,
                                      payload_size, &snapshot) ==
               player_snapshot_codec_result::ok);
        assert(static_cast<uint32_t>(snapshot.pid) == u32(journal.data() + offset, 40));
        std::vector<uint8_t> payload(journal.begin() + offset + FRAME_HEADER_SIZE,
                                     journal.begin() + offset + size);
        snapshots.push_back({std::move(snapshot), u64(journal.data() + offset, 24),
                             crc32(payload)});
        offset += static_cast<size_t>(size);
    }
    assert(offset == journal.size());
    return snapshots;
}

bool find_snapshot(const std::vector<uint8_t> &journal, int pid, player_snapshot *out)
{
    size_t offset = 0;
    while (offset < journal.size()) {
        const uint64_t size = u64(journal.data() + offset, 16);
        if (static_cast<int32_t>(u32(journal.data() + offset, 40)) == pid) {
            const uint32_t payload_size = u32(journal.data() + offset, 64);
            return player_snapshot_decode(journal.data() + offset + FRAME_HEADER_SIZE,
                                          payload_size, out) ==
                   player_snapshot_codec_result::ok;
        }
        offset += static_cast<size_t>(size);
    }
    return false;
}

struct mock_repository
{
    int terminal_pid = 0;
    bool terminal_seen = false;
    bool unexpected_fenced_call = false;
    bool unproven_death_seen = false;
    size_t receipts = 0;
    std::map<int, player_revision_t> durable;
    std::ofstream transcript;
};

player_save_apply_result mock_apply(const player_snapshot &snapshot, void *opaque)
{
    auto &mock = *static_cast<mock_repository *>(opaque);
    if (player_save_journal_pid_quarantined(snapshot.pid)) {
        mock.unexpected_fenced_call = true;
        return {player_save_apply_outcome::terminal_failure, 0, EPERM};
    }
    if (snapshot.pid == mock.terminal_pid && !mock.terminal_seen) {
        mock.terminal_seen = true;
        return {player_save_apply_outcome::terminal_failure, 0,
                PLAYER_SAVE_ERROR_CUSTODY_PAYLOAD_MISMATCH};
    }
    if (snapshot.death) {
        mock.unproven_death_seen = true;
        return {player_save_apply_outcome::terminal_failure, 0, ENOENT};
    }
    std::vector<uint8_t> payload;
    assert(player_snapshot_encode(snapshot, &payload) == player_snapshot_codec_result::ok);
    mock.transcript << snapshot.pid << '\t' << snapshot.revision << '\t'
                    << snapshot.components << '\t' << crc32(payload) << '\n';
    ++mock.receipts;
    player_revision_t &durable = mock.durable[snapshot.pid];
    if (snapshot.revision > durable) {
        durable = snapshot.revision;
        return {player_save_apply_outcome::applied, durable, 0};
    }
    if (snapshot.revision == durable)
        return {player_save_apply_outcome::already_applied, durable, 0};
    return {player_save_apply_outcome::stale_revision, durable, 0};
}

int main(int argc, char **argv)
{
    assert(argc >= 3);
    const std::string directory = argv[1];
    const std::string journal_path = directory + "/player-save.journal";
    const std::string archive_path = directory + "/player-save.journal.quarantine.archive";
    const std::string policy_path = directory + "/player-save.quarantine-pids";
    if (std::string(argv[2]) == "generate") {
        assert(player_save_journal_init(directory.c_str()));
        // These identities and item UIDs are synthetic. Capture through the
        // production encoder/journal, rather than reproducing its byte format.
        for (int pid = 1; pid <= 25; ++pid) {
            for (uint64_t revision : {1, 2}) {
                player_snapshot snapshot = {};
                snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
                snapshot.pid = pid;
                snapshot.revision = revision;
                snapshot.components = PLAYER_CHECKPOINT_COMPONENT_ALL;
                snapshot.encoded_size_bound = 8192;
                snapshot.save_intent = 4;
                snapshot.status_strings.push_back({player_status_string_field::name, "Synthetic"});
                if (pid == 1) {
                    snapshot.schema_version = PLAYER_SNAPSHOT_DEATH_SCHEMA_VERSION;
                    snapshot.death.emplace();
                    snapshot.death->operation_id.bytes[0] = 1;
                    snapshot.death->operation_id.bytes[15] = revision;
                    snapshot.death->corpse_room_vnum = 1201;
                    snapshot.death->wallet_revision = 1;
                    player_item_snapshot corpse = {};
                    corpse.object_uid = 1000 + revision;
                    corpse.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
                    corpse.vnum = VOBJ_CORPSE;
                    corpse.type = ITEM_CORPSE;
                    corpse.values[CORPSE_PID] = pid;
                    corpse.values[CORPSE_SAVEID] = 100 + revision;
                    corpse.values[CORPSE_FLAGS] = PC_CORPSE;
                    snapshot.death->corpse.push_back(corpse);
                }
                assert(player_save_journal_append(snapshot) == player_save_journal_result::ok);
            }
        }
        player_save_journal_shutdown();
        return 0;
    }
    const std::vector<int> policy_ids = read_ids(policy_path);

    if (std::string(argv[2]) == "fail-sync") {
        const std::vector<uint8_t> before = read_bytes(journal_path);
        fail_next_fdatasync = true;
        assert(!player_save_journal_init(directory.c_str()));
        assert(!fail_next_fdatasync);
        assert(read_bytes(journal_path) == before);
        assert(access(archive_path.c_str(), F_OK) != 0);
        for (int pid : policy_ids)
            assert(player_save_journal_pid_quarantined(pid));
        assert(player_save_journal_pid_quarantined(1));
        mock_repository mock;
        assert(player_save_journal_replay(mock_apply, &mock) ==
               player_save_journal_result::not_initialized);
        assert(mock.receipts == 0 && !mock.terminal_seen);
        player_save_journal_shutdown();
        return 0;
    }

    assert(std::string(argv[2]) == "success" && argc == 7);
    const std::vector<uint8_t> captured = read_bytes(journal_path);
    const std::vector<decoded_frame> snapshots = decode_snapshots(captured);
    std::ifstream expected_source(argv[5]);
    std::ofstream canonical_expected(argv[6], std::ios::out | std::ios::trunc);
    assert(expected_source.good() && canonical_expected.good());
    int expected_pid = 0;
    player_revision_t expected_revision = 0;
    uint64_t expected_components = 0;
    uint32_t expected_raw_checksum = 0;
    while (expected_source >> expected_pid >> expected_revision >> expected_components >>
           expected_raw_checksum) {
        const auto found = std::find_if(snapshots.begin(), snapshots.end(),
                                        [&](const decoded_frame &frame) {
            return frame.snapshot.pid == expected_pid &&
                   frame.snapshot.revision == expected_revision &&
                   frame.snapshot.components == expected_components &&
                   frame.payload_checksum == expected_raw_checksum;
        });
        assert(found != snapshots.end());
        std::vector<uint8_t> payload;
        assert(player_snapshot_encode(found->snapshot, &payload) ==
               player_snapshot_codec_result::ok);
        canonical_expected << expected_pid << '\t' << expected_revision << '\t'
                           << expected_components << '\t' << crc32(payload) << '\n';
    }
    assert(expected_source.eof());
    canonical_expected.close();
    const std::vector<int> identity_checks = read_ids(argv[3]);
    assert(identity_checks.size() == 3);
    const int pid003 = identity_checks[0];
    const int pid004 = identity_checks[1];
    const int terminal_pid = identity_checks[2];
    std::set<int> protected_ids(policy_ids.begin(), policy_ids.end());
    assert(protected_ids.size() == 22);
    assert(!protected_ids.count(pid003) && !protected_ids.count(pid004));
    assert(!protected_ids.count(terminal_pid));
    assert(player_save_journal_init(directory.c_str()));
    assert(!player_save_journal_pid_quarantined(pid003));
    assert(!player_save_journal_pid_quarantined(pid004));
    assert(!player_save_journal_pid_quarantined(terminal_pid));
    size_t active_expected = 0;
    for (const decoded_frame &frame : snapshots)
        if (!protected_ids.count(frame.snapshot.pid))
            ++active_expected;
    assert(player_save_journal_health_copy().records == active_expected);

    for (int pid : policy_ids) {
        player_snapshot snapshot = {};
        assert(find_snapshot(captured, pid, &snapshot));
        assert(player_save_journal_pid_quarantined(pid));
        assert(player_save_journal_append(snapshot) ==
               player_save_journal_result::quarantined_pid);
        assert(player_save_journal_checkpoint(pid, snapshot.revision) ==
               player_save_journal_result::quarantined_pid);
        assert(!player_save_journal_worker_append(snapshot, nullptr));
        assert(!player_save_journal_worker_ack(snapshot, snapshot.revision, nullptr));
    }

    mock_repository mock;
    mock.terminal_pid = terminal_pid;
    mock.transcript.open(argv[4], std::ios::out | std::ios::trunc);
    assert(mock.transcript.good());
    assert(player_save_journal_replay(mock_apply, &mock) == player_save_journal_result::ok);
    mock.transcript.close();
    assert(mock.terminal_seen);
    assert(!mock.unexpected_fenced_call && !mock.unproven_death_seen);
    assert(mock.receipts > 0);
    assert(player_save_journal_health_copy().records == 0);
    assert(player_save_journal_pid_quarantined(terminal_pid));
    assert(!player_save_journal_pid_quarantined(pid003));
    assert(!player_save_journal_pid_quarantined(pid004));
    player_snapshot terminal_snapshot = {};
    assert(find_snapshot(captured, terminal_pid, &terminal_snapshot));
    assert(player_save_journal_append(terminal_snapshot) ==
           player_save_journal_result::quarantined_pid);
    assert(player_save_journal_checkpoint(terminal_pid, terminal_snapshot.revision) ==
           player_save_journal_result::quarantined_pid);
    assert(!player_save_journal_worker_ack(terminal_snapshot,
                                           terminal_snapshot.revision, nullptr));
    player_save_journal_shutdown();

    // The manifest-backed fence survives restart; the unaffected PID aliases stay open.
    assert(player_save_journal_init(directory.c_str()));
    assert(player_save_journal_health_copy().records == 0);
    for (int pid : policy_ids)
        assert(player_save_journal_pid_quarantined(pid));
    assert(player_save_journal_pid_quarantined(terminal_pid));
    assert(!player_save_journal_pid_quarantined(pid003));
    assert(!player_save_journal_pid_quarantined(pid004));
    player_save_journal_shutdown();
    return 0;
}
'''


def fail(message: str) -> None:
    raise SystemExit(f"[FAIL] {message}")


def load_alias_map(path: Path) -> dict[str, int]:
    document = json.loads(path.read_text())
    alias_map: dict[str, int] = {}
    for entries in document["records"].values():
        for entry in entries:
            alias = entry.get("pid_alias")
            pid = entry.get("pid")
            if isinstance(alias, str) and isinstance(pid, int):
                alias_map[alias] = pid
    return alias_map


def parse_capture(path: Path, expected_digest=EXPECTED_CAPTURE_SHA256, expected_count=4810) -> list[Frame]:
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != expected_digest:
        fail("capture digest does not match the approved fixture")
    frames: list[Frame] = []
    offset = 0
    while offset < len(raw):
        if raw[offset : offset + 8] != FRAME_MAGIC or len(raw) - offset < FRAME_HEADER_SIZE:
            fail("capture frame header is invalid")
        size = struct.unpack_from("<Q", raw, offset + 16)[0]
        payload_size = struct.unpack_from("<I", raw, offset + 64)[0]
        if size != FRAME_HEADER_SIZE + payload_size or size > len(raw) - offset:
            fail("capture frame size is invalid")
        frame = raw[offset : offset + size]
        pid = struct.unpack_from("<i", raw, offset + 40)[0]
        revision = struct.unpack_from("<Q", raw, offset + 48)[0]
        components = struct.unpack_from("<Q", raw, offset + 56)[0]
        record_id = struct.unpack_from("<Q", raw, offset + 24)[0]
        payload = raw[offset + FRAME_HEADER_SIZE : offset + size]
        frames.append(
            {
                "pid": pid,
                "revision": revision,
                "components": components,
                "record_id": record_id,
                "payload_crc": binascii.crc32(payload) & 0xFFFFFFFF,
                "raw": frame,
            }
        )
        offset += size
    if offset != len(raw) or len(frames) != expected_count:
        fail("capture frame count is not the expected fixture count")
    return frames


def expected_replay(frames: list[Frame], skipped: set[int]) -> list[str]:
    ordered = sorted(
        (frame for frame in frames if frame["pid"] not in skipped),
        key=lambda frame: (frame["pid"], frame["revision"], frame["record_id"]),
    )
    seen: set[tuple[int, int, int, int]] = set()
    output = []
    for frame in ordered:
        identity = (
            int(frame["pid"]),
            int(frame["revision"]),
            int(frame["components"]),
            int(frame["payload_crc"]),
        )
        if identity in seen:
            continue
        seen.add(identity)
        output.append("\t".join(str(value) for value in identity) + "\n")
    return output


def write_private_ids(path: Path, ids: list[int]) -> None:
    path.write_text("".join(f"{pid}\n" for pid in ids))
    path.chmod(0o600)


def prepare_journal(directory: Path, capture: Path, ids: list[int]) -> None:
    directory.mkdir(mode=0o700)
    shutil.copyfile(capture, directory / "player-save.journal")
    (directory / "player-save.journal").chmod(0o600)
    write_private_ids(directory / "player-save.quarantine-pids", ids)


def verify_archive(
    path: Path,
    frames: list[Frame],
    expected_ids: set[int],
    explicit_ids: set[int],
    runtime_terminal_pid: int,
) -> None:
    raw = path.read_bytes()
    if stat.S_IMODE(path.stat().st_mode) != 0o600:
        fail("archive mode is not owner-only")
    if len(raw) < ARCHIVE_HEADER_SIZE or raw[:8] != ARCHIVE_MAGIC:
        fail("archive header is invalid")
    version, total_size, pid_count, frame_count = struct.unpack_from("<IQII", raw, 8)
    if version != 1 or total_size != len(raw):
        fail("archive version or total size is invalid")
    cursor = ARCHIVE_HEADER_SIZE
    pids = list(struct.unpack_from(f"<{pid_count}i", raw, cursor)) if pid_count else []
    cursor += pid_count * 4
    table_end = cursor + frame_count * ARCHIVE_ENTRY_SIZE
    if table_end > len(raw) or set(pids) != expected_ids or len(pids) != len(expected_ids):
        fail("archive PID manifest is incomplete")
    archived_frames: list[bytes] = []
    next_data_offset = table_end
    entries = []
    for _ in range(frame_count):
        pid, reason, data_offset, data_size = struct.unpack_from("<iIQQ", raw, cursor)
        digest = raw[cursor + 24 : cursor + 56]
        cursor += ARCHIVE_ENTRY_SIZE
        if data_offset != next_data_offset or data_size == 0 or data_offset + data_size > len(raw):
            fail("archive frame manifest bounds are invalid")
        frame = raw[data_offset : data_offset + data_size]
        expected_reason = 1 if pid in explicit_ids else 5
        if hashlib.sha256(frame).digest() != digest or pid not in expected_ids or reason != expected_reason:
            fail("archive frame hash, reason, or identity is invalid")
        archived_frames.append(frame)
        entries.append((pid, frame))
        next_data_offset += data_size
    if next_data_offset != len(raw):
        fail("archive contains unindexed bytes")
    expected_frames = [frame["raw"] for frame in frames if frame["pid"] in expected_ids]
    if Counter(archived_frames) != Counter(expected_frames):
        fail("archived frame bytes differ from the original capture")
    if len(entries) != len(expected_frames):
        fail("archive entry count differs from the original quarantined frames")


def run_capture(args, *, expected_digest=EXPECTED_CAPTURE_SHA256, expected_count=4810,
                expected_counts=None, expected_bytes=None) -> None:
    frames = parse_capture(args.capture, expected_digest, expected_count)
    reconciliation = json.loads(args.reconciliation.read_text())
    alias_map = load_alias_map(args.alias_index)
    split_manifest = json.loads(args.split_manifest.read_text())
    protected_aliases = split_manifest.get("protected_aliases")
    if (
        split_manifest.get("clone_only") is not True
        or split_manifest.get("all_frames_accounted_for") is not True
        or split_manifest.get("original_sha256") != expected_digest
        or not isinstance(protected_aliases, list)
        or len(protected_aliases) != 22
        or len(set(protected_aliases)) != 22
    ):
        fail("split manifest flags, original digest, or protected alias count are invalid")
    expected_counts = expected_counts or {"active": 948, "protected": 3862}
    expected_bytes = expected_bytes or {"active": 24_919_913, "protected": 83_536_054}
    if any(split_manifest.get("counts", {}).get(k) != v for k, v in expected_counts.items()):
        fail("split manifest active/protected frame counts differ from the verified rehearsal")
    if any(split_manifest.get("bytes", {}).get(k) != v for k, v in expected_bytes.items()):
        fail("split manifest active/protected byte counts differ from the verified rehearsal")

    groups = reconciliation["groups"]
    if reconciliation.get("frame_count") != len(frames):
        fail("redacted reconciliation frame count does not match capture")
    group_counts = Counter(int(frame["pid"]) for frame in frames)
    for group in groups:
        alias = group.get("pid_alias")
        if alias not in alias_map or group_counts[alias_map[alias]] != group.get("frames"):
            fail("private alias index does not match the redacted group manifest")
    if any(alias not in alias_map for alias in protected_aliases):
        fail("one or more manifest-protected aliases are missing from the private alias index")
    protected_set = set(protected_aliases)
    if not KNOWN_ACTIVE_ALIASES.issubset(alias_map) or protected_set & KNOWN_ACTIVE_ALIASES:
        fail("the known unaffected groups do not match the manifest policy")
    report_groups = {group["pid_alias"]: group for group in groups}
    if not protected_set.issubset(report_groups):
        fail("manifest aliases do not match the redacted reconciliation groups")
    death_aliases = {group["pid_alias"] for group in groups if group.get("death_frames", 0)}
    if not death_aliases.issubset(protected_set) or sum(
        group.get("death_frames", 0) for group in groups if group["pid_alias"] in death_aliases
    ) != reconciliation.get("death_frames"):
        fail("unproven death frames are not all inside the startup quarantine policy")

    protected_ids = {alias_map[alias] for alias in protected_aliases}
    configured_ids = sorted(protected_ids)
    candidates = [
        group
        for group in groups
        if group["pid_alias"] not in protected_set
        and group["pid_alias"] not in KNOWN_ACTIVE_ALIASES
        and group.get("frames", 0) > 0
        and group.get("death_frames", 0) == 0
    ]
    if not candidates:
        fail("no unaffected non-death group is available for the synthetic terminal test")
    probe_alias = min(candidates, key=lambda group: (group["frames"], group["pid_alias"]))["pid_alias"]
    probe_pid = alias_map[probe_alias]
    skipped_ids = protected_ids | {probe_pid}
    protected_bytes = b"".join(frame["raw"] for frame in frames if frame["pid"] in protected_ids)
    active_bytes = b"".join(frame["raw"] for frame in frames if frame["pid"] not in protected_ids)
    protected_copy = args.split_manifest.parent / "protected-player-save.journal"
    active_copy = args.split_manifest.parent / "player-save.journal"
    if (
        hashlib.sha256(protected_bytes).hexdigest() != split_manifest.get("protected_sha256")
        or hashlib.sha256(active_bytes).hexdigest() != split_manifest.get("active_sha256")
        or protected_copy.read_bytes() != protected_bytes
        or active_copy.read_bytes() != active_bytes
    ):
        fail("split-copy bytes do not exactly match the manifest-selected original frames")

    with tempfile.TemporaryDirectory(prefix="duris-journal-quarantine-", dir=ROOT / "bin/tests") as temporary:
        root = Path(temporary)
        failure_dir = root / "failure"
        prepare_journal(failure_dir, args.capture, configured_ids)
        source = root / "quarantine_test.cpp"
        binary = root / "harness"
        source.write_text(HARNESS)
        compiled = subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-O2",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-pthread",
                "-Wl,--wrap=fdatasync",
                "-Isrc",
                str(source),
                "src/player/player_snapshot_codec.c",
                "src/player/player_save_journal.c",
                "-o",
                str(binary),
            ],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if compiled.returncode:
            fail("quarantine C++ harness did not compile: " + compiled.stderr)
        print("JOURNAL-NATIVE compiled", flush=True)

        # Archive-sync failure must leave the copied journal byte-identical and
        # keep all login/save/replay paths fenced.
        subprocess.run(
            [str(binary), str(failure_dir), "fail-sync"],
            check=True,
            timeout=300,
            capture_output=True,
        )

        success_dir = root / "success"
        prepare_journal(success_dir, args.capture, configured_ids)
        pid_checks = root / "pid-checks"
        write_private_ids(
            pid_checks,
            [alias_map["PID-003"], alias_map["PID-004"], probe_pid],
        )
        expected_file = root / "expected.tsv"
        expected_file.write_text("".join(expected_replay(frames, skipped_ids)))
        expected_file.chmod(0o600)
        actual_file = root / "actual.tsv"
        canonical_file = root / "canonical-expected.tsv"
        subprocess.run(
            [str(binary), str(success_dir), "success", str(pid_checks), str(actual_file), str(expected_file), str(canonical_file)],
            check=True,
            timeout=600,
            capture_output=True,
        )
        if actual_file.read_bytes() != canonical_file.read_bytes():
            fail("mock replay order or canonical snapshot semantics differ")
        verify_archive(
            success_dir / "player-save.journal.quarantine.archive",
            frames,
            skipped_ids,
            protected_ids,
            probe_pid,
        )
        if (success_dir / "player-save.journal").stat().st_size != 0:
            fail("active journal still contains records after proven mock receipts")

    # The affected SQL and login boundaries must all consult the same PID fence.
    source_contracts = {
        "src/account/account.c": ("player_save_journal_pid_quarantined(c->pid)",),
        "src/core/files.c": ("player_save_journal_pid_quarantined(GET_PID(ch))",),
        "src/persistence/copyover.c": ("player_save_journal_pid_quarantined(result.pid)",),
        "src/player/player_load_materialize.c": ("player_save_journal_pid_quarantined(result.pid)",),
        "src/player/player_load_repository.c": ("player_save_journal_pid_quarantined(request.pid)",),
        "src/player/player_load_pipeline.c": (
            "player_save_journal_pid_quarantined(request.pid)",
            "player_save_journal_pid_quarantined(pid)",
        ),
        "src/sql/sql_player.c": (
            "player_save_journal_pid_quarantined(pid)",
            "player_save_journal_pid_quarantined(db_pid)",
        ),
        "src/player/player_snapshot_repository.c": (
            "player_save_journal_pid_quarantined(snapshot.pid)",
        ),
    }
    for relative, contracts in source_contracts.items():
        text = (ROOT / relative).read_text()
        if any(contract not in text for contract in contracts):
            fail("a login, load, or save bypass is missing the PID fence")

    sql_source = (ROOT / "src/sql/sql_player.c").read_text()
    guarded_sql_boundaries = (
        "sql_save_player",
        "sql_save_player_status",
        "sql_save_player_skills",
        "sql_save_player_affects",
        "sql_save_player_items",
        "sql_save_player_pets",
        "sql_save_player_shapechanges",
        "sql_player_rename",
        "sql_delete_player",
        "sql_add_player_recipe",
        "sql_delete_player_recipes",
        "sql_load_player_status",
        "sql_load_player_epic_bonus",
        "sql_load_player_skills",
        "sql_load_player_affects",
        "sql_load_player_items",
        "sql_load_player_shapechanges",
    )
    for name in guarded_sql_boundaries:
        definitions = list(re.finditer(r"^(?:static )?bool " + re.escape(name) + r"\(", sql_source, re.M))
        if not definitions:
            fail("a direct SQL save/load boundary could not be located")
        start = definitions[-1].start()
        opening = sql_source.find("{", start)
        closing = sql_source.find("\n}", opening)
        if opening < 0 or closing < 0 or "player_save_journal_pid_quarantined(" not in sql_source[opening:closing]:
            fail(name + " direct SQL save/load boundary is missing its PID fence")
    repository_source = (ROOT / "src/player/player_load_repository.c").read_text()
    repository_start = repository_source.rfind(
        "player_load_result player_load_repository_execute("
    )
    repository_body = repository_source[repository_start:]
    repository_fence = repository_body.find("player_save_journal_pid_quarantined(request.pid)")
    ordinary_delegate = repository_body.find("execute_player_load(connection, request, false)")
    if repository_start < 0 or repository_fence < 0 or ordinary_delegate < 0 or repository_fence > ordinary_delegate:
        fail("the direct repository load path can start SQL before checking the PID fence")

    print("[PASS] copied capture: byte-exact hashed quarantine, fail-closed sync failure, dynamic PID fence, unaffected replay order")


def synthetic_capture(directory):
    capture_root = directory / "capture"
    capture_root.mkdir(mode=0o700)
    source = directory / "generate.cpp"
    binary = directory / "generate"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-O2", "-pthread", "-Wl,--wrap=fdatasync", "-Isrc",
                    str(source), "src/player/player_snapshot_codec.c", "src/player/player_save_journal.c",
                    "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(capture_root), "generate"], cwd=ROOT, check=True, timeout=30)
    capture = capture_root / "player-save.journal"
    original_digest = hashlib.sha256(capture.read_bytes()).hexdigest()
    frames = parse_capture(capture, original_digest, 50)
    aliases = {f"PID-{pid:03}": pid for pid in range(1, 26)}
    protected = sorted(set(aliases) - KNOWN_ACTIVE_ALIASES - {"PID-025"})
    protected_ids = {aliases[alias] for alias in protected}
    groups = [{"pid_alias": alias, "frames": 2, "death_frames": 2 if pid == 1 else 0}
              for alias, pid in aliases.items()]
    paths = SimpleNamespace(capture=capture, alias_index=directory / "aliases.json",
                            reconciliation=directory / "reconciliation.json",
                            split_manifest=directory / "split.json")
    paths.alias_index.write_text(json.dumps({"records": {"synthetic": [
        {"pid_alias": alias, "pid": pid} for alias, pid in aliases.items()]}}))
    paths.reconciliation.write_text(json.dumps({"frame_count": 50, "death_frames": 2, "groups": groups}))
    active = b"".join(frame["raw"] for frame in frames if frame["pid"] not in protected_ids)
    fenced = b"".join(frame["raw"] for frame in frames if frame["pid"] in protected_ids)
    (directory / "player-save.journal").write_bytes(active)
    (directory / "protected-player-save.journal").write_bytes(fenced)
    counts = {"active": 6, "protected": 44}
    sizes = {"active": len(active), "protected": len(fenced)}
    paths.split_manifest.write_text(json.dumps({"clone_only": True, "all_frames_accounted_for": True,
        "original_sha256": original_digest, "protected_aliases": protected, "counts": counts,
        "bytes": sizes, "protected_sha256": hashlib.sha256(fenced).hexdigest(),
        "active_sha256": hashlib.sha256(active).hexdigest()}))
    for path in directory.rglob("*"):
        if path.is_file() and path != binary:
            path.chmod(0o600)
    print("CUSTODY-ADMISSION synthetic_capture_sha256=" + original_digest, flush=True)
    return paths, original_digest, counts, sizes


def main() -> None:
    parser = argparse.ArgumentParser()
    for argument in ("capture", "reconciliation", "alias-index", "split-manifest"):
        parser.add_argument("--" + argument, type=Path)
    args = parser.parse_args()
    supplied = [args.capture, args.reconciliation, args.alias_index, args.split_manifest]
    if any(supplied):
        if not all(supplied):
            parser.error("historical capture qualification requires all four private inputs")
        run_capture(args)
    else:
        (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="synthetic-quarantine-", dir=ROOT / "bin/tests") as temporary:
            paths, original_digest, counts, sizes = synthetic_capture(Path(temporary))
            run_capture(paths, expected_digest=original_digest, expected_count=50,
                        expected_counts=counts, expected_bytes=sizes)


if __name__ == "__main__":
    main()
