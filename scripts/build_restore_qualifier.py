#!/usr/bin/env python3
"""Build the listener-free native flat-file restore verifier."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCES = """
flatfile_player_repository player_load_topology flatfile_identity_repository
flatfile_account_repository flatfile_item_repository coin_transfer_command
flatfile_player_snapshot_file flatfile_corpse_repository flatfile_locker_repository
flatfile_world_item_repository flatfile_artifact_repository flatfile_shop_trade_repository
flatfile_shop_trade_materialization flatfile_shopkeeper_repository flatfile_auction_repository
flatfile_boon_repository flatfile_player_domain_repository flatfile_authority_transaction
locker_receipt player_save_journal critical_command_journal player_snapshot_codec flatfile_store item_transfer_command corpse_lifecycle_command
shop_trade_command critical_command epic_command currency_command auction_command
combat_outcome_command boon_reward_command boon_shop_command persistence_observability
persistence_mode flatfile_ip_activity_repository flatfile_ship_repository
flatfile_association_repository flatfile_nexus_repository kingdom_db kingdom_geometry
""".split()


def build(destination=None):
    destination = destination or ROOT / "bin/tools/qualify_flatfile_restore"
    destination.parent.mkdir(parents=True, exist_ok=True)
    sources = []
    for name in SOURCES:
        matches = list((ROOT / "src").rglob(name + ".c"))
        if len(matches) != 1:
            raise RuntimeError("ambiguous native verifier source")
        sources.append(str(matches[0]))
    subprocess.run(["g++", "-std=c++20", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-D__NO_MYSQL__", "-Isrc", "-Isrc/no_mysql",
                    "scripts/qualify_flatfile_restore.cpp", *sources, "-lcrypto", "-lz", "-pthread",
                    "-o", str(destination)], cwd=ROOT, check=True)
    return destination


if __name__ == "__main__":
    build()
