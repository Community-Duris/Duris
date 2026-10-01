#!/usr/bin/env python3
"""Exercise the game-loop session seams with a real isolated client.

This is deliberately a small wire-level journey rather than a source-only
contract.  It crosses account and character entry, prompt framing, queued
type-ahead, rendered output, quit, and reconnect against a disposable
flat-file server.  The shared fixtures own the server lifecycle and copy all
mutable runtime state into temporary directories.
"""

from __future__ import annotations

import os
import pathlib
import tempfile

from test_account_recovery_journey import (
    ACCOUNT,
    OLD_PASSWORD,
    IsolatedServer,
    MudClient,
    build_flatfile_server,
    enter_account_name,
    require,
)
from test_creation_prompt_journey import (
    finish_character_choices,
    reply,
    require_prompt_framed,
    start_character,
)


CHARACTER = "Pulsejourney"


def create_character(client: MudClient) -> None:
    enter_account_name(client)
    client.expect("is this correct?")
    client.send("y")
    client.expect("email address")
    client.send("pulse@example.invalid")
    client.expect("is this correct?")
    client.send("y")
    client.expect("enter your password")
    client.send(OLD_PASSWORD)
    client.expect("Please re-enter the same password to confirm:  ")
    client.send(OLD_PASSWORD)
    client.expect("information correct?")
    client.send("y")
    client.expect("PRESS RETURN")
    client.send("")
    client.expect("Please select an option")

    start_character(client)
    reply(client, CHARACTER, "Is this correct?")
    reply(client, "y", "meet these criteria?")
    reply(client, "y", "Your selection")
    finish_character_choices(client)
    reply(client, "y", "PRESS RETURN")
    reply(client, "", "Your starter kit is ready")
    client.expect("Pos: standing >", timeout=30)


def reconnect_character(client: MudClient) -> None:
    enter_account_name(client)
    client.expect("enter your password")
    client.send(OLD_PASSWORD)
    client.expect("PRESS RETURN")
    client.send("")
    client.expect("Please select an option")
    client.send("1")
    client.expect(CHARACTER)
    client.send("1")
    client.expect("Play as")
    client.send("y")
    client.expect("Pos: standing >", timeout=30)


def run_journey(binary: pathlib.Path) -> None:
    with IsolatedServer(binary, None) as server:
        client: MudClient | None = None
        try:
            client = MudClient(server.plain_port)
            create_character(client)

            # Two complete lines are sent before either prompt is consumed.
            # The session-input phase must preserve command FIFO, while the
            # output phase must render each prompt once and frame it for the
            # line-buffered client.  The default gameplay prompt is two-line:
            # the status line is followed by the final <> prompt line (with
            # its display space), which is the line that must be followed by
            # Telnet GA.
            client.send("score")
            client.send("look")
            score = client.expect("Pos: standing >", timeout=15)
            require(CHARACTER in score, "score output was lost or reordered")
            require_prompt_framed(client, "<> ")
            look = client.expect("Pos: standing >", timeout=15)
            require(look.strip(), "look output was empty")
            require_prompt_framed(client, "<> ")

            client.send("save")
            client.expect(f"Save complete for {CHARACTER}.", timeout=20)

            client.send("quit")
            # Normal quit camps first (TAG_CAMP: roughly 140 seconds).
            client.expect("ACCOUNT MENU", timeout=240)
            client.expect("Please select an option", timeout=10)
            client.send("0")
            client.close()
            client = None

            client = MudClient(server.plain_port)
            reconnect_character(client)
            client.send("quit")
            # Normal quit camps first (TAG_CAMP: roughly 140 seconds).
            client.expect("ACCOUNT MENU", timeout=240)
            client.expect("Please select an option", timeout=10)
            client.send("0")
            client.close()
            client = None
            server.shutdown()
        except Exception as error:
            output = server.server_output()
            raise AssertionError(
                f"{error}\n\n--- isolated server output ---\n{output[-12000:]}"
            ) from error
        finally:
            if client is not None:
                client.close()


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix=f"game-loop-session-{os.getpid()}-") as build_tmp:
        run_journey(build_flatfile_server(pathlib.Path(build_tmp)))
    print("game-loop real session journey passed")
