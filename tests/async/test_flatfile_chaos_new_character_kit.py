#!/usr/bin/env python3
"""Create a real CHAOS character and persist its generated class kit."""

from __future__ import annotations

import os
import pathlib
import re
import signal
import subprocess
import tempfile
import time

from test_flatfile_combat_journey import (
    ACCOUNT,
    CHARACTER,
    EMAIL,
    PASSWORD,
    MudClient,
    available_ports,
    build_flatfile_server,
    generate_certificate,
    make_fixture,
    require,
    runtime_logs,
)


ROOT = pathlib.Path(__file__).resolve().parents[2]
DATA_HEADER = ROOT / "src/account/chaos_eq_data.h"


def warrior_kit_vnums() -> set[int]:
    """Return the Warrior standard profile plus the shared consumable pool."""
    data = DATA_HEADER.read_text(encoding="utf-8", errors="replace")
    values: set[int] = {96443}
    for array_name in ("chaos_eq_standard_warrior", "chaos_eq_standard_optional_slots", "chaos_eq_support_consumables"):
        body = re.search(
            rf"static const chaos_kit_item {array_name}\[\] = \{{(.*?)\}};", data, re.S
        )
        require(body is not None, f"missing {array_name}")
        assert body is not None
        values.update(
            int(vnum)
            for _, vnum in re.findall(r"\{\s*(-?\d+|WEAR_NONE),\s*(\d+)\s*\}", body.group(1))
            if int(vnum) != 0
        )
    require(1252 not in values, "placeholder VNUM remains in the runtime Warrior kit")
    return values


def install_chaos_objects(run_root: pathlib.Path) -> None:
    world_objects = (ROOT / "areas/world.obj").read_text(errors="replace")
    mini_path = run_root / "areas_mini/mini.obj"
    mini_objects = mini_path.read_text(errors="replace")
    entries = {
        int(vnum): entry.rstrip() + "\n"
        for entry, vnum in re.findall(
            r"(?ms)(^#(\d+)$\n.*?)(?=^#\d+$|^\$~$)", mini_objects
        )
    }
    for vnum in sorted(warrior_kit_vnums()):
        if vnum in entries:
            continue
        entry = re.search(
            rf"(?ms)^#{vnum}$\n.*?(?=^#\d+$|^\$~$)", world_objects
        )
        require(entry is not None, f"world object {vnum} is unavailable")
        entries[vnum] = entry.group(0).rstrip() + "\n"
    require(mini_objects.count("$~") == 1, "minimal object terminator changed")
    mini_path.write_text("".join(entries[vnum] for vnum in sorted(entries)) + "$~\n")


def create_chaos_character(client: MudClient) -> None:
    entry, _ = client.expect_any(("term type", "account name"))
    if entry == "term type":
        client.send("9")
        client.expect("account name")
    client.send(ACCOUNT)
    client.expect("is this correct?")
    client.send("y")
    client.expect("email address")
    client.send(EMAIL)
    client.expect("is this correct?")
    client.send("y")
    client.expect("enter your password")
    client.send(PASSWORD)
    client.expect("verify your password")
    client.send(PASSWORD)
    client.expect("information correct?")
    client.send("y")
    client.expect("PRESS RETURN")
    client.send("")
    client.expect("Please select an option")
    client.send("2")
    client.expect("Enter your new name")
    client.send(CHARACTER)
    client.expect("Is this correct?")
    client.send("y")
    client.expect("meet these criteria?")
    client.send("y")
    client.expect("Your selection")
    client.send("h")
    client.expect("Male or Female")
    client.send("m")
    client.expect("Class Selection")
    client.send("w")
    client.expect("Alignment only affects")
    client.send("g")
    client.expect("Your selection")
    client.send("p")
    client.expect("Press return to continue")
    client.send("")
    for label in ("first bonus", "second bonus", "third bonus", "fourth bonus"):
        client.expect(label)
        client.send("s")
    client.expect("swap stats")
    client.send("n")
    client.expect("keep this character")
    client.send("y")
    client.expect("PRESS RETURN to read Duris rules")
    client.send("")
    client.expect("official and legal response")
    client.send("y")
    client.expect("PRESS RETURN")
    client.send("")
    client.expect("Your Chaos Equipment has been prepared!!", timeout=60)
    require(
        "Your CHAOS equipment kit is being prepared" not in client.transcript.decode("utf-8", errors="replace"),
        "Chaos creation still exposed the old blocking preparation message",
    )


def run_chaos_kit_journey(binary: pathlib.Path) -> None:
    with tempfile.TemporaryDirectory(prefix="duris-chaos-kit-state-") as state_tmp:
        with tempfile.TemporaryDirectory(prefix="duris-chaos-kit-run-") as run_tmp:
            state_root = pathlib.Path(state_tmp)
            run_root = pathlib.Path(run_tmp)
            state_root.chmod(0o700)
            (run_root / "logs/log").mkdir(parents=True)
            (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
            make_fixture(run_root)
            install_chaos_objects(run_root)
            generate_certificate(run_root)

            journal_root = run_root / "journals"
            (journal_root / "players").mkdir(parents=True, mode=0o700)
            (journal_root / "critical").mkdir(mode=0o700)
            plain_port, tls_port, websocket_port = available_ports()
            output_path = run_root / "server.out"
            environment = {
                "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                "ENVIRONMENT": "local",
                "PERSISTENCE_MODE": "flatfile-primary",
                "FLATFILE_STATE_DIR": str(state_root),
                "PLAYER_SAVE_JOURNAL_DIR": str(journal_root / "players"),
                "CRITICAL_COMMAND_JOURNAL_DIR": str(journal_root / "critical"),
                "LISTEN_ADDRESS": "127.0.0.1",
                "DURIS_TLS_PORT": str(tls_port),
                "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
                "DURIS_WEBSOCKET_PORT": str(websocket_port),
                "REDIS": "FALSE",
                "CHAOS_MUD": "TRUE",
                "CHAOS_EQ_PROFILE": "standard",
            }

            with output_path.open("w", encoding="utf-8") as output:
                process = subprocess.Popen(
                    [str(binary), "--minimal", "-s", "-d", str(run_root), str(plain_port)],
                    cwd=run_root,
                    env=environment,
                    text=True,
                    stdout=output,
                    stderr=subprocess.STDOUT,
                )
                client: MudClient | None = None
                try:
                    deadline = time.monotonic() + 120
                    boot_output = ""
                    while time.monotonic() < deadline:
                        output.flush()
                        boot_output = output_path.read_text(errors="replace")
                        if "Entering game loop." in boot_output or process.poll() is not None:
                            break
                        time.sleep(0.1)
                    require(
                        "Entering game loop." in boot_output,
                        "isolated CHAOS server did not boot:\n" + boot_output[-8000:],
                    )

                    client = MudClient(plain_port)
                    create_chaos_character(client)
                    creation_transcript = bytes(client.transcript).decode(
                        "utf-8", errors="replace"
                    )
                    require(
                        creation_transcript.count("You advance to level 56.") == 1,
                        "CHAOS catch-up did not emit one final-level notification:\n"
                        + creation_transcript[-8000:],
                    )
                    require(
                        "You raise a level!" not in creation_transcript,
                        "CHAOS catch-up still emitted per-level notifications:\n"
                        + creation_transcript[-8000:],
                    )

                    client.transcript.clear()
                    client.send("chaos level 55")
                    client.expect("You lose a level!", timeout=15)
                    client.expect("Pos: standing >", timeout=15)
                    client.transcript.clear()
                    client.send("chaos level 56")
                    client.expect("You raise a level!", timeout=15)
                    client.expect("Pos: standing >", timeout=15)
                    single_level_transcript = bytes(client.transcript).decode(
                        "utf-8", errors="replace"
                    )
                    require(
                        single_level_transcript.count("You raise a level!") == 1,
                        "single-level advancement did not retain one notification:\n"
                        + single_level_transcript,
                    )
                    require(
                        "You advance to level" not in single_level_transcript,
                        "single-level advancement used the batch notification:\n"
                        + single_level_transcript,
                    )

                    client.send("inventory")
                    client.expect("bottomless bag of", timeout=15)
                    client.expect("Pos: standing >", timeout=15)
                    client.send("look in bottomless")
                    bag_contents = client.expect("dark misty potion", timeout=15)
                    while True:
                        matched, page = client.expect_any(
                            ("[Return to continue", "Pos: standing >"), timeout=15
                        )
                        bag_contents += page
                        if matched == "Pos: standing >":
                            break
                        client.send("")
                    require(
                        "new random object" not in bag_contents,
                        "CHAOS warrior kit still contains VNUM 1252",
                    )
                    client.send("inventory")
                    client.expect("a bottomless bag of", timeout=15)
                    client.send("save")
                    client.expect(f"Save complete for {CHARACTER}.", timeout=30)

                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                    output.flush()
                    server_output = output_path.read_text(errors="replace")
                    logs = runtime_logs(run_root)
                    require(process.returncode == 0, "CHAOS server shutdown failed")
                    require(
                        "Cannot load CHAOS kit item" not in logs
                        and "item creation grant did not commit" not in logs
                        and "Skipping unusable CHAOS kit item" not in logs,
                        "CHAOS kit logged an incomplete or unusable grant:\n" + logs,
                    )
                    require("1252" not in server_output + logs, "placeholder VNUM reached the runtime journey")
                except Exception as error:
                    output.flush()
                    server_output = output_path.read_text(errors="replace")
                    logs = runtime_logs(run_root)
                    raise AssertionError(
                        f"{error}\n\n--- isolated server output ---\n{server_output[-12000:]}"
                        f"\n\n--- isolated runtime logs ---\n{logs}"
                    ) from error
                finally:
                    if client is not None:
                        client.close()
                    if process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=5)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            process.wait(timeout=5)


if __name__ == "__main__":
    run_chaos_kit_journey(build_flatfile_server())
    print("flat-file CHAOS new-character bag and generated class kit journey passed")
