#!/usr/bin/env python3

from _paths import SRC, rel
import pathlib
import shutil
import sys
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

(ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="flat-player-test-", dir=ROOT / "bin/tests") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_player_test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-Isrc",
            "-Isrc/no_mysql",
            "tests/async/flatfile_player_repository_harness.cpp",
            rel("flatfile_player_repository.c"),
            rel("player_load_topology.c"),
            rel("flatfile_identity_repository.c"),
            rel("flatfile_item_repository.c"),
            rel("coin_transfer_command.c"),
            rel("flatfile_player_snapshot_file.c"),
            rel("flatfile_corpse_repository.c"),
            rel("flatfile_locker_repository.c"),
            rel("flatfile_world_item_repository.c"),
            rel("flatfile_artifact_repository.c"),
            rel("flatfile_shop_trade_repository.c"),
            rel("flatfile_shop_trade_materialization.c"),
            rel("flatfile_shopkeeper_repository.c"),
            rel("flatfile_auction_repository.c"),
            rel("flatfile_boon_repository.c"),
            rel("flatfile_player_domain_repository.c"),
            rel("flatfile_authority_transaction.c"),
            rel("player_snapshot_codec.c"),
            rel("flatfile_store.c"),
            rel("item_transfer_command.c"),
            rel("corpse_lifecycle_command.c"),
            rel("shop_trade_command.c"),
            rel("critical_command.c"),
            rel("epic_command.c"),
            rel("currency_command.c"),
            rel("auction_command.c"),
            rel("combat_outcome_command.c"),
            rel("boon_reward_command.c"),
            rel("boon_shop_command.c"),
            rel("persistence_observability.c"),
            rel("persistence_mode.c"),
            rel("flatfile_ip_activity_repository.c"),
            "-lcrypto",
            "-pthread",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if compile_result.returncode:
        raise SystemExit(compile_result.stdout)

    if len(sys.argv) == 3 and sys.argv[1] == "--build-inspector":
        destination = pathlib.Path(sys.argv[2]).resolve()
        if not destination.is_relative_to((ROOT / "bin").resolve()):
            raise SystemExit("inspector must be placed below bin/")
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(binary, destination)
        raise SystemExit(0)

    state_root = temporary_path / "state"
    run_result = subprocess.run(
        [str(binary), str(state_root)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)

    domain_source = (SRC / "flatfile_player_domain_repository.c").read_text()
    player_source = (SRC / "flatfile_player_repository.c").read_text()
    materialize_source = (SRC / "player_load_materialize.c").read_text()
    for token in (
        "constexpr uint32_t domain_format_version = 3",
        "base_stat_revision",
        "record.domains.base_stats",
        "format_version >= 3",
    ):
        if token not in domain_source:
            raise SystemExit(f"flat player stat authority is missing {token}")
    if "record.domains.base_stat_revision = 1" not in player_source:
        raise SystemExit("first player baseline does not establish stat authority")
    for token in (
        "result.domains.base_stat_revision",
        "ch->base_stats.Str = result.domains.base_stats[0]",
        "ch->base_stats.Luk = result.domains.base_stats[9]",
    ):
        if token not in materialize_source:
            raise SystemExit(f"player load does not publish authoritative stats: {token}")
    print(run_result.stdout.strip())
