#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-player-test-") as temporary:
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
            "src/flatfile_player_repository.c",
            "src/flatfile_identity_repository.c",
            "src/flatfile_item_repository.c",
            "src/flatfile_shop_trade_repository.c",
            "src/flatfile_shop_trade_materialization.c",
            "src/flatfile_shopkeeper_repository.c",
            "src/flatfile_auction_repository.c",
            "src/flatfile_boon_repository.c",
            "src/flatfile_player_domain_repository.c",
            "src/flatfile_authority_transaction.c",
            "src/player_snapshot_codec.c",
            "src/flatfile_store.c",
            "src/item_transfer_command.c",
            "src/shop_trade_command.c",
            "src/critical_command.c",
            "src/epic_command.c",
            "src/currency_command.c",
            "src/auction_command.c",
            "src/combat_outcome_command.c",
            "src/boon_reward_command.c",
            "src/boon_shop_command.c",
            "src/persistence_observability.c",
            "src/persistence_mode.c",
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

    domain_source = (ROOT / "src/flatfile_player_domain_repository.c").read_text()
    player_source = (ROOT / "src/flatfile_player_repository.c").read_text()
    materialize_source = (ROOT / "src/player_load_materialize.c").read_text()
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
