#!/usr/bin/env python3
"""#253: exercise casting-pulse policy through a real flat-file server.

The journey creates an OVERLORD plus mortal Grey Elf and Githyanki Clerics,
advances and restores both Clerics through the real staff commands, then makes
each Cleric cast ``full heal`` twice normally and twice with deterministic quick
chant.
Every cast queues a marker command so the journey proves that type-ahead remains
ordered through landing. The player-event trace separately verifies that the
cast-owned command gate and the final spell continuation expire on the same
pulse; the ordinary four-pulse post-cast recovery remains intact. Run explicitly
with a fresh flat-file binary:

    python3 tests/async/run_casting_pulse_journey.py --server bin/server/dms_new
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
PROMPTS = ("Pos: standing >", "<>")
START = "You start chanting..."
COMPLETE = "A torrent of divine energy flows into your body"
ABORT_MESSAGES = (
    "You lost your concentration!",
    "You abort your spell before it's done!",
    "You abort your prayer before it's done!",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def create_character(
    client: journey.MudClient,
    *,
    account: str,
    character: str,
    email: str,
    race: str,
    class_name: str,
    alignment: str,
    hometown: str | None = None,
) -> None:
    """Create one disposable character while tolerating fixed race choices."""
    entry, _ = client.expect_any(("term type", "account name"))
    if entry == "term type":
        client.send("9")
        client.expect("account name")
    client.send(account)
    client.expect("is this correct?")
    client.send("y")
    client.expect("email address")
    client.send(email)
    client.expect("is this correct?")
    client.send("y")
    client.expect("enter your password")
    client.send(journey.PASSWORD)
    client.expect("re-enter the same password to confirm")
    client.send(journey.PASSWORD)
    client.expect("information correct?")
    client.send("y")
    client.expect("PRESS RETURN")
    client.send("")
    client.expect("Please select an option")
    client.send("2")
    client.expect("Enter your new name")
    client.send(character)
    client.expect("Is this correct?")
    client.send("y")
    client.expect("meet these criteria?")
    client.send("y")
    client.expect("Your selection")
    client.send(race)
    client.expect("Male or Female")
    client.send("m")
    client.expect("Hardcore")
    client.send("n")
    client.expect("Class Selection")
    client.expect("Your selection")
    client.send(class_name)

    next_step, _ = client.expect_any(("Alignment only affects", "Your selection", "Press return to continue"))
    if next_step == "Alignment only affects":
        client.expect("Your selection")
        client.send(alignment)
        next_step, _ = client.expect_any(("Your selection", "Press return to continue"))
    if next_step == "Your selection":
        require(hometown is not None, f"{character} unexpectedly required a hometown choice")
        client.send(hometown)
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
    client.expect_any(PROMPTS, timeout=30)


def drain(client: journey.MudClient, quiet_seconds: float = 0.4) -> str:
    """Consume asynchronous notices and stale prompts until the socket is quiet."""
    collected = bytearray(client.pending)
    client.pending.clear()
    deadline = time.monotonic() + quiet_seconds
    while time.monotonic() < deadline:
        if client._receive():
            collected.extend(client.pending)
            client.pending.clear()
            deadline = time.monotonic() + quiet_seconds
    return collected.decode("utf-8", errors="replace")


def command(client: journey.MudClient, line: str, timeout: float = 30) -> str:
    drain(client)
    start = len(client.transcript)
    client.send(line)
    client.expect_any(PROMPTS, timeout=timeout)
    return bytes(client.transcript[start:]).decode("utf-8", errors="replace")


def configure_fixture(run_root: pathlib.Path) -> None:
    (run_root / "logs/log").mkdir(parents=True)
    (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
    journey.make_fixture(run_root)
    properties_path = run_root / "lib/duris.properties"
    properties = properties_path.read_text()
    replacements = {
        "spellcast.quickChant.tankSuccessPercent=75.000":
            "spellcast.quickChant.tankSuccessPercent=100.000",
        "spellcast.quickChant.skillBasePercent=0.000":
            "spellcast.quickChant.skillBasePercent=100.000",
        "spellcast.quickChant.skillPercentPerPoint=1.000":
            "spellcast.quickChant.skillPercentPerPoint=0.000",
        "spellcast.maxCircleAbort.capPercent=5.000":
            "spellcast.maxCircleAbort.capPercent=0.000",
    }
    for original, replacement in replacements.items():
        require(properties.count(original) == 1, f"property fixture changed: {original}")
        properties = properties.replace(original, replacement)
    properties_path.write_text(properties)
    journey.generate_certificate(run_root)


def staff_prepare(
    admin: journey.MudClient,
    player: journey.MudClient,
    name: str,
    expected_race: str,
) -> None:
    drain(admin)
    start = len(admin.transcript)
    admin.send(f"advance {name} 56")
    admin.expect("You feel generous.")
    admin.expect_any(PROMPTS)
    advanced = bytes(admin.transcript[start:]).decode("utf-8", errors="replace")
    require("You feel generous." in advanced, f"advance failed for {name}:\n{advanced}")
    restore_slots(admin, name)
    drain(player)
    start = len(player.transcript)
    player.send("score")
    player.expect("Level: 56", timeout=30)
    player.expect_any(PROMPTS, timeout=30)
    score = bytes(player.transcript[start:]).decode("utf-8", errors="replace")
    require("Level: 56" in score and "Level: 57" not in score,
            f"{name} was not a mortal level 56:\n{score}")
    require(f"Race: {expected_race}" in score and "Class: Cleric" in score,
            f"{name} did not have the expected race/class under test:\n{score}")
    command(player, "remove all")
    equipment = command(player, "equipment")
    require("You aren't wearing anything!" in equipment,
            f"{name} retained equipment during the same-gear comparison:\n{equipment}")


def restore_slots(admin: journey.MudClient, name: str) -> None:
    drain(admin)
    start = len(admin.transcript)
    admin.send(f"restore {name}")
    admin.expect("Done.")
    admin.expect_any(PROMPTS)
    reply = bytes(admin.transcript[start:]).decode("utf-8", errors="replace")
    require("Done." in reply and "Huh?" not in reply and "cannot" not in reply.lower(),
            f"restore failed for {name}:\n{reply}")


def transfer_to_fixture_room(
    admin: journey.MudClient, player: journey.MudClient, name: str
) -> None:
    reply = command(admin, f"transfer {name}")
    require("Ok." in reply and "No-one by that name" not in reply,
            f"transfer failed for {name}:\n{reply}")
    arrival = player.expect("demands your presence NOW", timeout=15)
    require("demands your presence NOW" in arrival,
            f"{name} did not receive the transfer:\n{arrival}")
    room = command(player, "look")
    require("The Regression Arena" in room,
            f"{name} was not transferred to the casting fixture:\n{room}")


def set_quick_chant(client: journey.MudClient, enabled: bool) -> None:
    expected = "Quickchant is enabled." if enabled else "Quickchant is disabled."
    drain(client)
    start = len(client.transcript)
    client.send("toggle quickchant")
    client.expect(expected)
    client.expect_any(PROMPTS)
    reply = bytes(client.transcript[start:]).decode("utf-8", errors="replace")
    require(expected in reply, f"quick chant did not become {enabled}:\n{reply}")


def prepare_prayers(client: journey.MudClient, count: int = 4) -> None:
    rested = command(client, "rest")
    require("Pos: sitting" in rested, f"character did not rest before praying:\n{rested}")
    for _ in range(count):
        drain(client)
        client.send("pray full heal")
        client.expect("You are praying for full heal", timeout=15)
        client.expect_any(PROMPTS, timeout=15)
    for _ in range(count):
        client.expect("You have finished praying for full heal.", timeout=60)
    client.expect("Your prayers are complete.", timeout=60)
    client.expect_any(PROMPTS, timeout=15)
    stood = command(client, "stand")
    require("stand" in stood.lower(), f"character did not stand after praying:\n{stood}")


def expect_next(client: journey.MudClient, needles: tuple[str, ...], timeout: float) -> tuple[str, str]:
    """Consume the earliest target in the wire stream, not tuple priority order."""
    targets = tuple((needle, needle.encode("ascii")) for needle in needles)
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        pending = bytes(client.pending)
        matches = [
            (position, needle, target)
            for needle, target in targets
            if (position := pending.find(target)) >= 0
        ]
        if matches:
            position, needle, target = min(matches, key=lambda match: match[0])
            end = position + len(target)
            consumed = pending[:end]
            del client.pending[:end]
            return needle, consumed.decode("utf-8", errors="replace")
        client._receive()
    readable = bytes(client.pending).decode("utf-8", errors="replace")
    raise AssertionError(f"timed out waiting for {needles!r}; received:\n{readable[-6000:]}")


def cast_and_measure(client: journey.MudClient, label: str) -> tuple[float, float, str]:
    marker = f"{label}-gate-open"
    drain(client)
    transcript_start = len(client.transcript)
    try:
        client.send("cast 'full heal' self")
        outcome, start_output = client.expect_any((START, *ABORT_MESSAGES), timeout=15)
        require(outcome == START, f"{label} did not start: {outcome!r}; {start_output!r}")
        started_at = time.monotonic()
        client.send(f"say {marker}")

        observed: dict[str, float] = {}
        output = start_output
        while COMPLETE not in observed or marker not in observed:
            outcome, chunk = expect_next(client, (COMPLETE, marker, *ABORT_MESSAGES), timeout=30)
            output += chunk
            require(outcome not in ABORT_MESSAGES, f"{label} aborted: {output!r}")
            observed.setdefault(outcome, time.monotonic())
    except Exception as error:
        full_output = bytes(client.transcript[transcript_start:]).decode("utf-8", errors="replace")
        raise AssertionError(f"{label} cast observation failed:\n{full_output}") from error

    completion_seconds = observed[COMPLETE] - started_at
    recovery_seconds = observed[marker] - observed[COMPLETE]
    full_output = bytes(client.transcript[transcript_start:]).decode("utf-8", errors="replace")
    require("quick chant falters" not in full_output.lower(),
            f"{label} unexpectedly failed quick chant: {full_output!r}")
    require(0.5 <= recovery_seconds < 3.0,
            f"{label} post-cast recovery was {recovery_seconds:.3f}s: {full_output!r}")
    client.expect_any(PROMPTS, timeout=15)
    return completion_seconds, recovery_seconds, full_output


def assert_cast_gate_alignment(status_output: str, expected_casts: int) -> None:
    """Match each cast-owned wait to its final continuation by player and tick."""
    pattern = re.compile(
        r"PLAYER EVENT TIMING: func=(event_spellcast|event_wait) sequence=(\d+).*?"
        r"ch_pid=(-?\d+) due_tick=(\d+) actual_tick=(\d+)"
    )
    events = [
        (function, int(sequence), int(player_id), int(due_tick), int(actual_tick))
        for function, sequence, player_id, due_tick, actual_tick
        in pattern.findall(status_output)
    ]
    spells = [event for event in events if event[0] == "event_spellcast"]
    aligned_waits = []
    for function, wait_sequence, player_id, due_tick, actual_tick in events:
        if function != "event_wait" or player_id <= 0:
            continue
        matching_spells = [
            spell for spell in spells
            if spell[1] > wait_sequence and spell[2:] == (player_id, due_tick, actual_tick)
        ]
        if matching_spells:
            aligned_waits.append((player_id, due_tick, wait_sequence, matching_spells[0][1]))
    relevant_trace = "\n".join(
        line for line in status_output.splitlines()
        if "PLAYER EVENT TIMING" in line
        and ("event_spellcast" in line or "event_wait" in line)
    )
    require(len(aligned_waits) == expected_casts,
            f"expected {expected_casts} cast gates aligned with landing, got "
            f"{aligned_waits!r}; relevant trace:\n{relevant_trace}")
    counts = {
        player_id: sum(1 for aligned in aligned_waits if aligned[0] == player_id)
        for player_id, *_ in aligned_waits
    }
    require(sorted(counts.values()) == [expected_casts // 2, expected_casts // 2],
            f"cast-gate evidence was not balanced across both players: {counts!r}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=pathlib.Path, required=True,
                        help="Fresh flat-file binary built from this branch")
    binary = parser.parse_args().server.resolve()
    require(binary.is_file() and os.access(binary, os.X_OK),
            f"server binary is not executable: {binary}")

    ROOT.joinpath("bin/tests").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="duris-253-inspector-", dir=ROOT / "bin/tests") as tools, \
            tempfile.TemporaryDirectory(prefix="duris-253-state-") as state_tmp, \
            tempfile.TemporaryDirectory(prefix="duris-253-run-") as run_tmp:
        inspector = pathlib.Path(tools) / "inspector"
        subprocess.run(
            ["python3", "tests/async/test_flatfile_player_repository.py",
             "--build-inspector", str(inspector)],
            cwd=ROOT, check=True, timeout=180,
        )
        state_root, run_root = pathlib.Path(state_tmp), pathlib.Path(run_tmp)
        state_root.chmod(0o700)
        (state_root / "domains").mkdir(mode=0o700)
        subprocess.run([str(inspector), str(state_root), "seed-combat"], check=True, timeout=30)
        configure_fixture(run_root)
        (run_root / "journals/players").mkdir(parents=True, mode=0o700)
        (run_root / "journals/critical").mkdir(mode=0o700)
        plain_port, tls_port, websocket_port = journey.available_ports()
        environment = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
            "ENVIRONMENT": "local",
            "PERSISTENCE_MODE": "flatfile-primary",
            "FLATFILE_STATE_DIR": str(state_root),
            "PLAYER_SAVE_JOURNAL_DIR": str(run_root / "journals/players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(run_root / "journals/critical"),
            "LISTEN_ADDRESS": "127.0.0.1",
            "DURIS_TLS_PORT": str(tls_port),
            "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
            "DURIS_WEBSOCKET_PORT": str(websocket_port),
            "REDIS": "FALSE",
            "CHAOS_MUD": "FALSE",
            "CREATION_ALL_CLASSES": "TRUE",
            "DURIS_NEVENT_TRACE_PLAYER": "1",
        }
        if runtime_library_path := os.environ.get("LD_LIBRARY_PATH"):
            environment["LD_LIBRARY_PATH"] = runtime_library_path

        output_path = run_root / "server.out"
        with output_path.open("w", encoding="utf-8") as output:
            process = subprocess.Popen(
                [str(binary), "--minimal", "-s", "-d", str(run_root), str(plain_port)],
                cwd=run_root, env=environment, text=True,
                stdout=output, stderr=subprocess.STDOUT,
            )
            clients: list[journey.MudClient] = []
            try:
                deadline = time.monotonic() + 120
                boot_output = ""
                while time.monotonic() < deadline:
                    output.flush()
                    boot_output = output_path.read_text(errors="replace")
                    if "Entering game loop." in boot_output or process.poll() is not None:
                        break
                    time.sleep(0.1)
                require("Entering game loop." in boot_output,
                        f"server did not boot:\n{boot_output[-8000:]}")

                admin = journey.MudClient(plain_port)
                clients.append(admin)
                create_character(admin, account="Castadmin", character="Tyrus",
                                 email="castadmin@example.invalid", race="h",
                                 class_name="w", alignment="g", hometown="p")

                grey = journey.MudClient(plain_port)
                clients.append(grey)
                create_character(grey, account="Greycast", character="Greypriest",
                                 email="greycast@example.invalid", race="e",
                                 class_name="c", alignment="g", hometown="p")

                gith = journey.MudClient(plain_port)
                clients.append(gith)
                create_character(gith, account="Githcast", character="Githpriest",
                                 email="githcast@example.invalid", race="j",
                                 class_name="c", alignment="e")

                staff_prepare(admin, grey, "Greypriest", "Grey Elf")
                staff_prepare(admin, gith, "Githpriest", "Githyanki")
                transfer_to_fixture_room(admin, gith, "Githpriest")
                prepare_prayers(grey)
                prepare_prayers(gith)

                timings: dict[str, tuple[float, float]] = {}
                for name, client in (("grey", grey), ("gith", gith)):
                    # New characters begin with quick chant enabled.
                    set_quick_chant(client, False)
                    normal_runs = [
                        cast_and_measure(client, f"{name}-normal-{attempt}")
                        for attempt in range(1, 3)
                    ]

                    set_quick_chant(client, True)
                    quick_runs = [
                        cast_and_measure(client, f"{name}-quick-{attempt}")
                        for attempt in range(1, 3)
                    ]
                    normal = sum(run[0] for run in normal_runs) / len(normal_runs)
                    quick = sum(run[0] for run in quick_runs) / len(quick_runs)
                    require(quick < normal * 0.75,
                            f"{name} quick chant was not materially faster: normal={normal:.3f}, quick={quick:.3f}")
                    timings[name] = normal, quick
                    print(
                        f"{name}: normal={normal:.3f}s quick={quick:.3f}s "
                        f"post_cast_recovery="
                        f"({','.join(f'{run[1]:.3f}s' for run in normal_runs)};"
                        f"{','.join(f'{run[1]:.3f}s' for run in quick_runs)})",
                        flush=True,
                    )

                racial_gap = max(
                    abs(timings["grey"][0] - timings["gith"][0]),
                    abs(timings["grey"][1] - timings["gith"][1]),
                )
                quick_gain = min(
                    timings["grey"][0] - timings["grey"][1],
                    timings["gith"][0] - timings["gith"][1],
                )
                require(
                    racial_gap < quick_gain,
                    "race still dominated quick chant: "
                    f"racial_gap={racial_gap:.3f}s quick_gain={quick_gain:.3f}s "
                    f"timings={timings!r}",
                )

                status_path = run_root / "logs/log/status"
                require(status_path.exists(), "player-event status trace was not written")
                assert_cast_gate_alignment(status_path.read_text(errors="replace"), expected_casts=8)

                for client in reversed(clients):
                    client.send("quit")
                    client.expect_any(("ACCOUNT MENU", "Goodbye", "account menu"), timeout=30)
                    client.close()
                clients.clear()
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=30)
                output.flush()
                server_output = output_path.read_text(errors="replace")
                require(process.returncode == 0,
                        f"server shutdown failed: {process.returncode}")
                require("Normal termination of game." in server_output,
                        "server did not reach normal shutdown")
                require("FATAL:" not in server_output and "assert:" not in server_output,
                        "server logged a fatal/assertion failure")
                print(
                    "[PASS] #253 real casting journey: four full-heal casts per race; "
                    "quick chant shortened both races, every cast gate matched landing, "
                    "and post-cast recovery remained intact",
                    flush=True,
                )
            except Exception as error:
                output.flush()
                server_output = output_path.read_text(errors="replace")
                status_path = run_root / "logs/log/status"
                status_output = status_path.read_text(errors="replace") if status_path.exists() else ""
                raise AssertionError(
                    f"{error}\n\n--- isolated server output ---\n{server_output[-12000:]}"
                    f"\n\n--- isolated status log ---\n{status_output[-12000:]}"
                ) from error
            finally:
                for client in clients:
                    client.close()
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=10)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=5)


if __name__ == "__main__":
    main()
