#!/usr/bin/env python3
"""First-connection currency with empty and populated bank authority, retry and reload."""

import pathlib
import subprocess
import tempfile

from test_flatfile_combat_journey import (
    ROOT, INSPECTOR, CHARACTER, build_flatfile_server, inspect_authority,
    reconnect_character, require, run_journey,
)


def verify_first_session(client, port, state_root, populated_bank):
    # The full starter kit fills the level-one inventory slot allowance.
    client.send("drop all")
    client.expect("You drop a steel long sword.", timeout=20)
    opening = inspect_authority(state_root)
    bank = [19, 23, 31, 47] if populated_bank else [0, 0, 0, 0]
    revision = 2 if populated_bank else 1
    require(opening["wallet"] == [0, 0, 0, 0] and opening["wallet_revision"] == 0,
            f"unexpected opening wallet: {opening}")
    require(opening["bank"] == bank and opening["bank_revision"] == revision,
            f"creation changed the selected account bank: {opening}")
    # This command submits both expected live revisions, before any reconnect.
    client.send("get coins")
    client.expect("You get 3s.", timeout=20)
    credited = inspect_authority(state_root)
    require(credited["wallet"] == [0, 3, 0, 0] and credited["wallet_revision"] == 1,
            f"first-session credit did not commit exactly once: {credited}")
    require(credited["bank"] == bank and credited["bank_revision"] == revision + 1,
            f"first-session bank revision did not advance once: {credited}")
    # Repeating the pickup and saving cannot credit the absent pile again.
    client.send("get coins")
    client.send("save")
    client.expect(f"Save complete for {CHARACTER}.", timeout=20)
    client.send("quit")
    client.expect("ACCOUNT MENU", timeout=30)
    client.send("0")
    client.close()
    reloaded = reconnect_character(port)
    try:
        reloaded.send("get coins")
        reloaded.send("save")
        reloaded.expect(f"Save complete for {CHARACTER}.", timeout=20)
        after = inspect_authority(state_root)
        for key in ("wallet", "wallet_revision", "bank", "bank_revision"):
            require(after[key] == credited[key], f"retry/reload changed {key}: {after}")
        # Spending uses the bank revision published to the live character on reload.
        reloaded.send("drop 1 silver")
        reloaded.expect("OK.", timeout=20)
        spent = inspect_authority(state_root)
        require(spent["wallet"] == [0, 2, 0, 0] and spent["wallet_revision"] == 2
                and spent["bank"] == bank and spent["bank_revision"] == revision + 2,
                f"reloaded currency state was not usable: {spent}")
    finally:
        reloaded.close()


if __name__ == "__main__":
    subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                    "--build-inspector", str(INSPECTOR)], cwd=ROOT, check=True, timeout=180)
    with tempfile.TemporaryDirectory(prefix="flatfile-first-session-",
                                     dir=ROOT / "bin/tests") as temporary:
        binary = build_flatfile_server(pathlib.Path(temporary))
        for populated in (False, True):
            run_journey(binary, reset_coins=True, first_session_only=True,
                        populated_bank=populated)
    print("flat-file first-session currency, populated bank, retry and reload passed")
