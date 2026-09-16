#!/usr/bin/env python3
"""#252: measure the racial spell-pulse factor through the real server.

Each case creates a disposable human MindFlayer in the minimal flat-file world.
Chaos mode is used only to put the player at level 56 (the mortal cap), which
makes the first-circle self spell avoid the high-circle abort branch. MindFlayer
uses the real ``will``/``do_will`` spellcast path and mana, so no spell-slot or
spellbook setup is needed. Quickchant is explicitly disabled before measuring.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import signal
import subprocess
import tempfile
import time

import test_flatfile_combat_journey as journey


ROOT = pathlib.Path(__file__).resolve().parents[2]
ACCOUNT = "Spellacct"
CHARACTER = "Spellcaster"
EMAIL = "spellcast@example.invalid"
SPELL = "adrenaline control"
START = "You begin to focus your will..."
COMPLETE = "Your mental manipulations become a reality..."
ABORT_MESSAGES = (
    "You lost your concentration!",
    "You abort your spell before it's done!",
    "You abort your prayer before it's done!",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def create_mindflayer(client: journey.MudClient) -> None:
    """Create the disposable player using the existing account journey."""
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
    client.send(journey.PASSWORD)
    client.expect("Please re-enter the same password to confirm:  ")
    client.send(journey.PASSWORD)
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
    client.send("mindflayer")
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
    client.expect("PRESS RETURN")
    client.send("")
    client.expect("The Regression Arena", timeout=30)
    # The minimal fixture can report the optional Chaos kit failure because its
    # object set intentionally omits the full-world Chaos bag. The player still
    # enters normally; consume the first gameplay prompt as the login boundary.
    client.expect("Pos: standing >", timeout=30)


def configure_fixture(run_root: pathlib.Path, factor: float) -> None:
    (run_root / "logs/log").mkdir(parents=True)
    (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
    journey.make_fixture(run_root)
    properties = run_root / "lib/duris.properties"
    text = properties.read_text()
    updated, count = re.subn(
        r"(?m)^spellcast\.pulse\.racial\.All=.*$",
        f"spellcast.pulse.racial.All={factor:.3f}",
        text,
        count=1,
    )
    require(count == 1, "racial All property fixture changed")
    properties.write_text(updated)


def run_case(binary: pathlib.Path, factor: float, inspector: pathlib.Path) -> float:
    with tempfile.TemporaryDirectory(prefix=f"duris-252-state-{factor}-") as state_tmp:
        with tempfile.TemporaryDirectory(prefix=f"duris-252-run-{factor}-") as run_tmp:
            state_root = pathlib.Path(state_tmp)
            run_root = pathlib.Path(run_tmp)
            state_root.chmod(0o700)
            (state_root / "domains").mkdir(mode=0o700)
            subprocess.run(
                [str(inspector), str(state_root), "seed-combat"],
                check=True,
                timeout=30,
            )
            configure_fixture(run_root, factor)
            journey.generate_certificate(run_root)
            journal_root = run_root / "journals"
            (journal_root / "players").mkdir(parents=True, mode=0o700)
            (journal_root / "critical").mkdir(mode=0o700)
            plain_port, tls_port, websocket_port = journey.available_ports()
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
                "CREATION_ALL_CLASSES": "TRUE",
                # Curated grants exclude Psionicist-only spatial focus from
                # this MindFlayer. FALSE would enable the legacy blanket epic
                # grant in update_skills(), introducing random acceleration.
                "CHAOS_STARTER_EPIC_SKILLS": "TRUE",
                "DURIS_NEVENT_TRACE_PLAYER": "1",
            }
            if runtime_library_path := os.environ.get("LD_LIBRARY_PATH"):
                environment["LD_LIBRARY_PATH"] = runtime_library_path

            with output_path.open("w", encoding="utf-8") as output:
                process = subprocess.Popen(
                    [str(binary), "--minimal", "-s", "-d", str(run_root), str(plain_port)],
                    cwd=run_root,
                    env=environment,
                    text=True,
                    stdout=output,
                    stderr=subprocess.STDOUT,
                )
                client: journey.MudClient | None = None
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
                        f"factor {factor} server did not boot:\n{boot_output[-8000:]}",
                    )

                    client = journey.MudClient(plain_port)
                    create_mindflayer(client)

                    client.send("score")
                    score = client.expect("Pos: standing >", timeout=15)
                    require("Level: 56" in score, f"factor {factor} was not mortal level 56")
                    require("Level: 57" not in score, "factor case entered an immortal level")

                    client.send("toggle quickchant")
                    quickchant = client.expect("Quickchant is disabled.", timeout=10)
                    require("Quickchant is enabled." not in quickchant,
                            "quickchant remained enabled")
                    client.expect("Pos: standing >", timeout=10)

                    client.send(f"will '{SPELL}'")
                    outcome, start_output = client.expect_any(
                        (START, COMPLETE, *ABORT_MESSAGES), timeout=15
                    )
                    require(outcome == START, f"factor {factor} did not start a normal will cast: {outcome!r}")
                    require(COMPLETE not in start_output, "will cast completed in its start response")
                    started_at = time.monotonic()
                    outcome, completion_output = client.expect_any(
                        (COMPLETE, *ABORT_MESSAGES), timeout=30
                    )
                    completed_at = time.monotonic()
                    require(
                        outcome == COMPLETE,
                        f"factor {factor} cast did not complete: {outcome!r}; "
                        f"output={completion_output!r}",
                    )
                    cast_output = start_output + completion_output
                    require(
                        not any(message in cast_output for message in ABORT_MESSAGES),
                        f"factor {factor} used an abort path: {cast_output!r}",
                    )
                    require("Focusing your mind" not in cast_output,
                            f"factor {factor} used spatial-focus acceleration: {cast_output!r}")
                    elapsed = completed_at - started_at
                    require(elapsed > 0.35, f"factor {factor} was an insta-cast ({elapsed:.3f}s)")

                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=30)
                    client.send("0")
                    client.close()
                    client = None

                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                    output.flush()
                    server_output = output_path.read_text(errors="replace")
                    require(process.returncode == 0,
                            f"factor {factor} server shutdown failed: {process.returncode}")
                    require("Normal termination of game." in server_output,
                            f"factor {factor} did not reach normal shutdown")
                    require("FATAL:" not in server_output and "assert:" not in server_output,
                            f"factor {factor} logged a fatal/assertion failure")
                    print(f"factor={factor:.1f} completion_seconds={elapsed:.3f} output={cast_output!r}", flush=True)
                    return elapsed
                except Exception as error:
                    output.flush()
                    server_output = output_path.read_text(errors="replace")
                    raise AssertionError(
                        f"factor {factor}: {error}\n\n"
                        f"--- isolated server output ---\n{server_output[-12000:]}"
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


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=pathlib.Path, required=True,
                        help="Fresh flat-file binary built from this branch")
    binary = parser.parse_args().server.resolve()
    require(binary.is_file() and os.access(binary, os.X_OK),
            f"server binary is not executable: {binary}")
    ROOT.joinpath("bin/tests").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="duris-252-inspector-", dir=ROOT / "bin/tests") as temporary:
        inspector = pathlib.Path(temporary) / "inspector"
        subprocess.run(
            ["python3", "tests/async/test_flatfile_player_repository.py",
             "--build-inspector", str(inspector)],
            cwd=ROOT, check=True, timeout=180,
        )
        timings = {factor: run_case(binary, factor, inspector)
                   for factor in (0.5, 1.0, 2.0)}
    require(timings[1.0] > timings[0.5] * 1.35,
            f"All=1.0 was not discriminably slower than All=0.5: {timings}")
    require(timings[2.0] > timings[1.0] * 1.35,
            f"All=2.0 was not discriminably slower than All=1.0: {timings}")
    print(
        "[PASS] #252 real flat-file will journey: "
        + ", ".join(f"All={factor:.1f} {timings[factor]:.3f}s" for factor in timings),
        flush=True,
    )


if __name__ == "__main__":
    main()
