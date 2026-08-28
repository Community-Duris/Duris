#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
REPOSITORY = (ROOT / "src/flatfile_shop_trade_repository.c").read_text()
PLAYER = (ROOT / "src/flatfile_player_repository.c").read_text()
for token in (
    "flatfile_shop_trade_materialization_prepare(",
    "images.push_back(std::move(materialization.after_image))",
    "payload.action != shop_trade_action::discard_invalid",
):
    if token not in REPOSITORY:
        raise SystemExit(f"shop trade authority transaction is missing {token}")
locked_owner = PLAYER.index("flatfile_item_repository_load_owner_locked(")
reconcile = PLAYER.index("flatfile_shop_trade_materialization_reconcile(")
identities = PLAYER.index("build_item_identities(result->snapshot.items", reconcile)
if not locked_owner < reconcile < identities:
    raise SystemExit("player load does not reconcile shop materialization under ownership authority")

with tempfile.TemporaryDirectory(prefix="duris-flat-shop-trade-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_shop_trade_test"
    sources = [
        "tests/async/flatfile_shop_trade_repository_harness.cpp",
        "src/flatfile_shop_trade_repository.c",
        "src/flatfile_shop_trade_materialization.c",
        "src/flatfile_shopkeeper_repository.c",
        "src/flatfile_auction_repository.c",
        "src/flatfile_boon_repository.c",
        "src/flatfile_item_repository.c",
        "src/flatfile_locker_repository.c",
        "src/flatfile_player_domain_repository.c",
        "src/flatfile_authority_transaction.c",
        "src/flatfile_store.c",
        "src/player_snapshot_codec.c",
        "src/shop_trade_command.c",
        "src/item_transfer_command.c",
        "src/epic_command.c",
        "src/auction_command.c",
        "src/currency_command.c",
        "src/combat_outcome_command.c",
        "src/boon_reward_command.c",
        "src/boon_shop_command.c",
        "src/critical_command.c",
        "src/persistence_mode.c",
    ]
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST",
            "-Isrc",
            "-Isrc/no_mysql",
            *sources,
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
    run_result = subprocess.run(
        [str(binary), str(temporary_path / "state")],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    print(run_result.stdout.strip())
