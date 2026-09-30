#!/usr/bin/env python3
"""Native restore qualification preserves verified archives and admission policy."""

import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("restore_builder", ROOT / "scripts/build_restore_qualifier.py")
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)

HARNESS = r'''
#include "player/player_save_journal.h"
#include <cassert>
#include <string>
int main(int argc, char **argv) {
    assert(argc == 2);
    const std::string directory = argv[1];
    assert(player_save_journal_init(directory.c_str()));
    player_snapshot snapshot{};
    snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
    snapshot.pid = 9001; snapshot.revision = 1;
    snapshot.components = PLAYER_COMPONENT_STATUS;
    snapshot.save_intent = 4; snapshot.room_vnum = 1201;
    snapshot.encoded_size_bound = 8192;
    snapshot.status_strings.push_back({player_status_string_field::name, "synthetic-restore"});
    assert(player_save_journal_append(snapshot) == player_save_journal_result::ok);
    player_save_journal_worker_terminal(snapshot, nullptr);
    assert(player_save_journal_health_copy().records == 0);
    assert(player_save_journal_pid_quarantined(9001));
    player_save_journal_shutdown();
    assert(player_save_journal_init(directory.c_str()));
    assert(player_save_journal_pid_quarantined(9001));
    assert(!player_save_journal_pid_quarantined(9002));
    player_save_journal_shutdown();
}
'''

with tempfile.TemporaryDirectory(prefix="duris-quarantine-restore-") as temporary:
    root = Path(temporary)
    qualifier = builder.build(root / "qualifier")
    source = root / "seed.cpp"
    seed = root / "seed"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-pthread", "-Isrc", str(source), "src/player/player_save_journal.c",
                    "src/player/player_snapshot_codec.c", "-o", str(seed)], cwd=ROOT, check=True)
    valid = root / "valid"
    players = valid / "journals/players"
    players.mkdir(parents=True, mode=0o700)
    (valid / "journals/critical").mkdir(mode=0o700)
    (valid / "ISOLATED_RESTORE").write_text("synthetic fixture\n")
    subprocess.run([str(seed), str(players)], check=True)
    policy = players / "player-save.quarantine-pids"
    policy.write_text("9001\n")
    policy.chmod(0o600)
    archive = players / "player-save.journal.quarantine.archive"
    protected = {path.name: path.read_bytes() for path in (archive, policy)}
    for phase in ("--journals-preflight", "--journals-drained"):
        result = subprocess.run([str(qualifier), phase, str(valid)], capture_output=True, text=True, check=True)
        report = json.loads(result.stdout)
        assert report["player_records"] == report["critical_records"] == 0
        assert report["protected_player_bytes"] > 0
        assert all((players / name).read_bytes() == original for name, original in protected.items())
    for case in ("bad-hash", "invalid-policy", "oversized-policy", "unsafe-policy", "policy-wrong-root", "archive-symlink"):
        candidate = root / case
        shutil.copytree(valid, candidate)
        candidate_players = candidate / "journals/players"
        candidate_archive = candidate_players / archive.name
        candidate_policy = candidate_players / policy.name
        if case == "bad-hash":
            payload = bytearray(candidate_archive.read_bytes())
            payload[-1] ^= 1
            candidate_archive.write_bytes(payload)
        elif case == "invalid-policy":
            candidate_policy.write_text("9001\nnot-a-pid\n")
        elif case == "oversized-policy":
            candidate_policy.write_text("9001\n" * 1000)
        elif case == "unsafe-policy":
            candidate_policy.chmod(0o644)
        elif case == "policy-wrong-root":
            candidate_policy.rename(candidate / "journals/critical" / policy.name)
        else:
            target = root / "archive-target"
            candidate_archive.rename(target)
            candidate_archive.symlink_to(target)
        original = {path: path.read_bytes() for path in candidate_players.iterdir() if path.is_file()}
        result = subprocess.run([str(qualifier), "--journals-preflight", str(candidate)], capture_output=True)
        assert result.returncode != 0, case
        assert all(path.read_bytes() == content for path, content in original.items()), case

print("[PASS] native archive and PID policy qualify unchanged; damaged hashes, invalid/unsafe policy, wrong root, and symlinks refuse restore")
