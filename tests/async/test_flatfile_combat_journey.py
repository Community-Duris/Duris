#!/usr/bin/env python3
"""End-to-end combat, death, corpse-loot, save, and reconnect regression.

The journey boots the real server with an isolated flat-file authority and a
small deterministic world fixture.  It deliberately crosses protocol parsing,
account and character creation, command dispatch, combat pulses, NPC and player
death, both corpse types, item movement, terminal saves, and player reload.
"""

from __future__ import annotations

import os
import pathlib
import re
import shutil
import signal
import socket
import subprocess
import tempfile
import time


ROOT = pathlib.Path(__file__).resolve().parents[2]
ACCOUNT = "Journeyacct"
PASSWORD = "Qz7!mN4@"
CHARACTER = "Taverek"
EMAIL = "journey@example.invalid"
ANSI = re.compile(rb"\x1b\[[0-?]*[ -/]*[@-~]")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def available_ports() -> tuple[int, int, int]:
    """Reserve a non-privileged plain/TLS/WebSocket port set."""
    for _ in range(200):
        probes = [socket.socket() for _ in range(3)]
        try:
            probes[0].bind(("127.0.0.1", 0))
            plain = probes[0].getsockname()[1]
            if plain <= 1024 or plain >= 65533:
                continue
            probes[1].bind(("127.0.0.1", plain + 1))
            probes[2].bind(("127.0.0.1", plain + 2))
            return plain, plain + 1, plain + 2
        except OSError:
            continue
        finally:
            for probe in probes:
                probe.close()
    raise AssertionError("could not reserve isolated listener ports")


class MudClient:
    def __init__(self, port: int) -> None:
        deadline = time.monotonic() + 30
        while True:
            try:
                self.socket = socket.create_connection(
                    ("127.0.0.1", port), timeout=1
                )
                break
            except OSError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(0.05)
        self.socket.settimeout(0.25)
        self.pending = bytearray()
        self.transcript = bytearray()

    def close(self) -> None:
        self.socket.close()

    def send(self, line: str) -> None:
        self.socket.sendall(line.encode("ascii") + b"\n")

    def _receive(self) -> bool:
        try:
            chunk = self.socket.recv(65536)
        except socket.timeout:
            return False
        if not chunk:
            raise AssertionError("server closed the gameplay connection")
        cleaned = ANSI.sub(b"", chunk)
        self.pending.extend(cleaned)
        self.transcript.extend(cleaned)
        return True

    def expect(self, needle: str, timeout: float = 15) -> str:
        matched, output = self.expect_any((needle,), timeout)
        require(matched == needle, f"unexpected match {matched!r}")
        return output

    def expect_any(self, needles: tuple[str, ...], timeout: float = 15) -> tuple[str, str]:
        targets = tuple((needle, needle.encode("ascii")) for needle in needles)
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            cleaned = bytes(self.pending)
            for needle, target in targets:
                position = cleaned.find(target)
                if position >= 0:
                    # Login output is ASCII apart from Telnet negotiation. Consume
                    # through the last target byte while retaining a readable slice.
                    consumed = cleaned[: position + len(target)]
                    del self.pending[: position + len(target)]
                    return needle, consumed.decode("utf-8", errors="replace")
            self._receive()
        readable = bytes(self.pending).decode("utf-8", errors="replace")
        raise AssertionError(f"timed out waiting for {needles!r}; received:\n{readable[-6000:]}")

def make_fixture(run_root: pathlib.Path) -> None:
    for directory in ("areas", "docs"):
        (run_root / directory).symlink_to(ROOT / directory, target_is_directory=True)

    runtime_lib = run_root / "lib"
    shutil.copytree(ROOT / "lib", runtime_lib)
    properties_path = runtime_lib / "duris.properties"
    properties = properties_path.read_text()
    require("camp.timer=9.000" in properties, "camp timer fixture changed")
    properties_path.write_text(properties.replace("camp.timer=9.000", "camp.timer=2.000"))

    mini = run_root / "areas_mini"
    shutil.copytree(ROOT / "areas_mini", mini)

    mobile_path = mini / "mini.mob"
    mobiles = mobile_path.read_text()
    executioner = """#22800
regression executioner~
the regression executioner~
The regression executioner waits here to test the player death path.
~
~
10 0 0 0 0 0 0 0 S
PH 0 0 -1
60 0 -250 1000d1+50000 10d10+200
0.0.0.0 0
8 8 0
"""
    require(mobiles.count("$~") == 1, "minimal mobile terminator changed")
    mobile_path.write_text(mobiles.replace("$~", executioner + "$~"))

    world_path = mini / "mini.wld"
    world = world_path.read_text()
    fixture_room = """#22800
The Regression Arena~
This quiet stone arena exists to prove the complete combat journey.\n~
1 0 0
S
"""
    require(world.count("$~") == 1, "minimal world terminator changed")
    world_path.write_text(world.replace("$~", fixture_room + "$~"))

    zone_path = mini / "mini.zon"
    zone = zone_path.read_text()
    require("1299 0 0 6 11 1" in zone, "minimal zone header changed")
    require(zone.count("\nS\n") == 1, "minimal zone terminator changed")
    zone = zone.replace("1299 0 0 6 11 1", "29999 0 0 6 11 1")
    reset = (
        "M 0 11 1 22800 100 0 0 0 * deterministic combat target\n"
        "G 1 15 1 0 100 0 0 0 * corpse-loot marker\n"
        "M 0 22800 1 22800 100 0 0 0 * deterministic player-death target\n"
    )
    zone_path.write_text(zone.replace("\nS\n", "\n" + reset + "S\n"))


def generate_certificate(run_root: pathlib.Path) -> None:
    generated = subprocess.run(
        [
            "openssl",
            "req",
            "-x509",
            "-newkey",
            "rsa:2048",
            "-sha256",
            "-nodes",
            "-days",
            "1",
            "-subj",
            "/CN=localhost",
            "-keyout",
            str(run_root / "duris.key"),
            "-out",
            str(run_root / "duris.crt"),
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=30,
    )
    require(generated.returncode == 0, "certificate generation failed:\n" + generated.stdout)
    (run_root / "duris.key").chmod(0o600)


def runtime_logs(run_root: pathlib.Path) -> str:
    """Collect bounded synthetic-server logs for actionable journey failures."""
    sections = []
    for path in sorted((run_root / "logs/log").glob("*")):
        if not path.is_file() or path.name == ".gitignore":
            continue
        content = path.read_text(encoding="utf-8", errors="replace").strip()
        if content:
            sections.append(f"--- {path.name} ---\n{content[-8000:]}")
    return "\n".join(sections)[-30000:]


def create_character(client: MudClient, expected_room: str | None = "The Regression Arena") -> None:
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
    client.expect("Hardcore")
    client.send("n")
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
    if expected_room:
        client.expect(expected_room, timeout=30)
    client.expect("Your starter kit is ready", timeout=30)


def complete_npc_combat_journey(client: MudClient) -> None:
    client.send("wield mace")
    client.expect("You wield", timeout=10)
    client.send("drop all")
    client.expect("You drop a steel long sword.", timeout=20)
    client.send("look")
    room = client.expect("The Regression Arena", timeout=10)
    occupants = client.expect("Pos: standing >", timeout=10)
    require("Regression Arena" in room, "look did not render the fixture room")
    require("Raoul" in occupants, "combat target was not in the fixture room")
    require(
        "regression executioner" in occupants,
        "player-death target was not in the fixture room",
    )

    client.send("kill raoul")
    client.expect("is dead! R.I.P.", timeout=30)
    client.send("look")
    client.expect("corpse of raoul", timeout=10)
    client.send("look in corpse")
    corpse = client.expect("banana", timeout=10)
    require("corpse" in corpse.lower(), "loot marker was not inside the corpse")

    client.send("get banana corpse")
    client.expect("get a banana", timeout=10)
    client.send("inventory")
    client.expect("a banana", timeout=10)

    client.send("save")
    client.expect(f"Save complete for {CHARACTER}.", timeout=15)
    client.send("quit")
    client.expect("ACCOUNT MENU", timeout=30)
    client.send("0")


def reconnect_character(
    port: int,
    return_message: str | None = None,
    expected_room: str | None = "The Regression Arena",
) -> MudClient:
    client = MudClient(port)
    try:
        entry, _ = client.expect_any(("term type", "account name"))
        if entry == "term type":
            client.send("9")
            client.expect("account name")
        client.send(ACCOUNT)
        client.expect("enter your password")
        client.send(PASSWORD)
        client.expect("PRESS RETURN")
        client.send("")
        client.expect("Please select an option")
        client.send("1")
        client.expect(CHARACTER)
        client.send("1")
        client.expect("Play as")
        client.send("y")
        if return_message:
            client.expect(return_message, timeout=30)
        if expected_room:
            client.expect(expected_room, timeout=30)
        return client
    except Exception:
        client.close()
        raise


def verify_npc_loot_and_die(port: int) -> None:
    client = reconnect_character(port)
    try:
        client.send("inventory")
        client.expect("a banana", timeout=15)
        client.send("hit executioner")
        client.expect("Your wounds claim you at last", timeout=30)
        client.expect("ACCOUNT MENU", timeout=45)
        client.send("0")
    finally:
        client.close()


def recover_player_corpse(port: int) -> None:
    client = reconnect_character(port, "You rejoin the land of the living")
    try:
        client.send("look")
        client.expect("The corpse of a Human is lying here.", timeout=10)
        client.send(f"look in {CHARACTER}")
        corpse = client.expect("a banana", timeout=10)
        require(CHARACTER in corpse, "player corpse did not contain the saved loot marker")

        client.send(f"get banana {CHARACTER}")
        client.expect("get a banana", timeout=15)
        client.send("save")
        client.expect(f"Save complete for {CHARACTER}.", timeout=15)
        client.send("quit")
        client.expect("ACCOUNT MENU", timeout=30)
        client.send("0")
    finally:
        client.close()


def verify_recovered_loot(port: int) -> None:
    client = reconnect_character(port)
    try:
        client.send("inventory")
        client.expect("a banana", timeout=15)
        client.send("quit")
        client.expect("ACCOUNT MENU", timeout=30)
        client.send("0")
    finally:
        client.close()


def build_flatfile_server() -> pathlib.Path:
    build_root = ROOT / "bin/tests/flatfile-combat"
    binary = build_root / "server/dms_new"
    build = subprocess.run(
        [
            "make",
            "-C",
            "src",
            "PERSISTENCE_BACKEND=flatfile",
            f"BIN_ROOT={build_root}",
            f"OBJDIR={build_root / 'objects' / 'server'}",
            f"SERVER_BIN_DIR={binary.parent}",
            f"DMS_BINARY={binary}",
            "-j2",
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=600,
    )
    require(build.returncode == 0, "flat-file server build failed:\n" + build.stdout[-8000:])
    return binary


def run_journey(binary: pathlib.Path) -> None:
    with tempfile.TemporaryDirectory(prefix="duris-combat-state-") as state_tmp:
        with tempfile.TemporaryDirectory(prefix="duris-combat-run-") as run_tmp:
            state_root = pathlib.Path(state_tmp)
            run_root = pathlib.Path(run_tmp)
            state_root.chmod(0o700)
            (run_root / "logs/log").mkdir(parents=True)
            (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
            make_fixture(run_root)
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
                "CHAOS_MUD": "FALSE",
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
                        if "Entering game loop." in boot_output:
                            break
                        if process.poll() is not None:
                            break
                        time.sleep(0.1)
                    require(
                        "Entering game loop." in boot_output,
                        "isolated combat server did not boot:\n" + boot_output[-8000:],
                    )

                    client = MudClient(plain_port)
                    create_character(client)
                    complete_npc_combat_journey(client)
                    client.close()
                    client = None
                    verify_npc_loot_and_die(plain_port)
                    recover_player_corpse(plain_port)
                    verify_recovered_loot(plain_port)

                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                    output.flush()
                    server_output = output_path.read_text(errors="replace")
                    require(
                        process.returncode == 0,
                        f"server shutdown failed ({process.returncode}):\n{server_output[-8000:]}",
                    )
                    require(
                        "Normal termination of game." in server_output,
                        "combat journey did not reach normal server shutdown",
                    )
                    require(
                        "FATAL:" not in server_output and "assert:" not in server_output,
                        "combat journey logged a fatal/assertion failure:\n" + server_output[-8000:],
                    )
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
    run_journey(build_flatfile_server())
    print("flat-file combat, player death, corpse recovery, save, and reconnect journey passed")
