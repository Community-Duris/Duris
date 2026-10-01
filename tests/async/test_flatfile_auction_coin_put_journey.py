#!/usr/bin/env python3
"""Auction listing must leave a connected player's next coin put usable."""

import argparse
import os
import pathlib
import subprocess
import tempfile
from unittest.mock import patch

import test_flatfile_combat_journey as journey
import test_flatfile_first_session_currency as first_session


TRANSFER_FAILED = "The coin transfer did not commit; nothing changed."
TRANSFER_SUCCEEDED = "coins into a small leather bag."
REMAINING_WALLET = [0, 8, 9, 8]  # Ten platinum less the 1,020-copper listing fee.
original_fixture = journey.make_fixture


def make_fixture(run_root, reset_coins=False):
    original_fixture(run_root, reset_coins)
    zone = run_root / "areas_mini/mini.zon"
    zone.write_text(zone.read_text().replace(
        "\nS\n",
        "\nO 0 390 1 22800 100 0 0 0 * coin-put test bag\n"
        "O 0 391 1 22800 100 0 0 0 * auction test item\n"
        "O 0 15 1 22800 100 0 0 0 * other bag contents\nS\n"))
    objects = run_root / "areas_mini/mini.obj"
    contents = objects.read_text()
    start = contents.index("#3\n")
    end = contents.index("#4\n", start)
    contents = (contents[:start] + contents[start:end].replace(
        "0 3 0 0 0 0 0 0", "0 0 0 10 0 0 0 0") + contents[end:])
    objects.write_text(contents.replace(
        "bag leather small~", "testbag bag leather small~").replace(
        "bag leather large~", "saleitem bag leather large~"))


def command(client, text, expected):
    client.send(text)
    matched, output = client.expect_any((
        expected, TRANSFER_FAILED,
        "The coin debit could not start; nothing changed."), timeout=30)
    journey.require(matched == expected, f"{text}: {output}")
    print(f"{text}: {matched}", flush=True)


def verify(client, port, state_root, populated_bank, expect_regression=False):
    # Free starter-kit inventory slots. The target bag holds food and no coins.
    command(client, "drop all", "You drop ")
    command(client, "get testbag", "You get a small leather bag.")
    command(client, "get saleitem", "You get a large leather bag.")
    command(client, "get banana", "You get a banana.")
    command(client, "put banana testbag", "Ok.")
    command(client, "get coins", "You get 10p.")
    command(client, "auction offer saleitem 1", "is now listed as auction")
    listed = journey.inspect_authority(state_root)
    journey.require(listed["wallet"] == REMAINING_WALLET,
                    f"unexpected listing fee: {listed}")

    # No disconnect occurs before these commands.
    if expect_regression:
        for _ in range(2):
            command(client, "put all.coins testbag", TRANSFER_FAILED)
            after = journey.inspect_authority(state_root)
            for key in ("wallet", "wallet_revision", "bank", "bank_revision"):
                journey.require(after[key] == listed[key],
                                f"failed coin put changed {key}: {after}")
    else:
        command(client, "put all.coins testbag", TRANSFER_SUCCEEDED)
        journey.require(journey.inspect_authority(state_root)["wallet"] == [0, 0, 0, 0],
                        "successful coin put did not durably debit the wallet")
        command(client, "get coins testbag", "You get 8p, 9g, and 8s.")
        command(client, "put all.coins testbag", TRANSFER_SUCCEEDED)

    command(client, "save", f"Save complete for {journey.CHARACTER}.")
    command(client, "quit", "ACCOUNT MENU")
    client.send("0")
    client.close()
    reloaded = journey.reconnect_character(port)
    try:
        if not expect_regression:
            command(reloaded, "get coins testbag", "You get 8p, 9g, and 8s.")
        after = journey.inspect_authority(state_root)
        journey.require(after["wallet"] == REMAINING_WALLET,
                        f"round trip or full reload changed the coin balance: {after}")
        command(reloaded, "put all.coins testbag", TRANSFER_SUCCEEDED)
    finally:
        reloaded.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=pathlib.Path,
                        help="reuse an already built flat-file server")
    parser.add_argument("--expect-regression", action="store_true",
                        help="verify the original failure and full-reload workaround")
    args = parser.parse_args()
    (journey.ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=f"auction-coin-put-{os.getpid()}-",
                                     dir=journey.ROOT / "bin/tests") as build_tmp:
        inspector = pathlib.Path(build_tmp) / "coin-death-inspector"
        subprocess.run([
            "python3", "tests/async/test_flatfile_player_repository.py",
            "--build-inspector", str(inspector),
        ], cwd=journey.ROOT, check=True, timeout=180)
        binary = (args.server.resolve() if args.server else
                  journey.build_flatfile_server(pathlib.Path(build_tmp)))

        def verify_session(client, port, state_root, populated_bank):
            verify(client, port, state_root, populated_bank, args.expect_regression)

        with patch.object(journey, "INSPECTOR", inspector), \
                patch.object(journey, "make_fixture", make_fixture), \
                patch.object(first_session, "verify_first_session", verify_session):
            journey.run_journey(binary, reset_coins=True, first_session_only=True)
    print("Original auction/coin-put regression reproduced." if args.expect_regression else
          "Auction listing, immediate coin put, round trips, and full reload passed.")


if __name__ == "__main__":
    main()
