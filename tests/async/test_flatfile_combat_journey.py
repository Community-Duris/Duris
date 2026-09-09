#!/usr/bin/env python3
"""End-to-end combat, death, corpse-loot, save, and reconnect regression.

The journey boots the real server with an isolated flat-file authority and a
small deterministic world fixture.  It deliberately crosses protocol parsing,
account and character creation, command dispatch, combat pulses, NPC and player
death, both corpse types, item movement, terminal saves, and player reload.
"""

from __future__ import annotations

import os
import errno
import fcntl
import hashlib
import json
import struct
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
INSPECTOR = ROOT / "bin/tests/coin-death-inspector"
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

def make_fixture(run_root: pathlib.Path, reset_coins: bool = False) -> None:
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
    # Exercise both wallet-created and reset-created piles without prior custody.
    # A nonempty reset prototype must not add coins to later wallet conversions.
    start = mobiles.index("#11\n")
    end = mobiles.index("#12\n", start)
    raoul = mobiles[start:end]
    if reset_coins:
        raoul = raoul.replace("raoul~", "raoul _nomoney_~", 1)
    require("0.0.0.0 0" in raoul, "Raoul wallet fixture changed")
    mobiles = mobiles[:start] + raoul + mobiles[end:]
    objects_path = mini / "mini.obj"
    objects = objects_path.read_text()
    start = objects.index("#3\n")
    end = objects.index("#4\n", start)
    coins = objects[start:end]
    require("0 0 0 0 0 0 0 0" in coins, "money object fixture changed")
    objects = objects[:start] + coins.replace(
        "0 0 0 0 0 0 0 0", "0 3 0 0 0 0 0 0" if reset_coins else "0 0 0 0 0 0 0 0") + objects[end:]
    # End the low-HP NPC fight on a landed hit instead of depending on many
    # random attack/dodge rolls while Raoul is wounded. Keep real combat and
    # death handling; the executioner's 51,000 HP still safely exceeds this.
    start = objects.index("#677\n")
    end = objects.index("#678\n", start)
    mace = objects[start:end]
    require("6 1 6 7 0 0 0 0" in mace, "starter mace fixture changed")
    objects_path.write_text(objects[:start] + mace.replace(
        "6 1 6 7 0 0 0 0", "6 100 1 7 0 0 0 0") + objects[end:])
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
        + ("G 1 3 1 0 100 0 0 0 * reset-created coins\n" if reset_coins else "")
        +         "M 0 22800 1 22800 100 0 0 0 * deterministic player-death target\n"
    )
    zone_path.write_text(zone.replace("\nS\n", "\n" + reset + "S\n"))


def inspect_authority(state_root: pathlib.Path) -> dict:
    return json.loads(subprocess.check_output(
        [str(INSPECTOR), str(state_root), "inspect", "1"], text=True, timeout=15))


def add_death_conflict(state_root: pathlib.Path, parent_uid: int) -> int:
    """Add one durable-only descendant to the synthetic player's live root.

    Ordinary admission would advance the owner revision, producing ESTALE.
    This fixture preserves revisions while introducing the exact subtree-count
    disagreement that production rejects with EMSGSIZE. Never used on .env data.
    """
    path = state_root / "domains/item_ownership"
    with (state_root / "domains/.critical-authority.lock").open("r+b") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        data = bytearray(path.read_bytes())
        require(data[:8] == b"DUROWN\0\0", "custody fixture magic changed")
        version, size, _ = struct.unpack_from("<IIQ", data, 8)
        require(version == 3 and size == len(data) - 56, "custody fixture format changed")
        require(hashlib.sha256(data[56:]).digest() == data[24:56], "invalid custody fixture")
        owners, items, _ = struct.unpack_from("<III", data, 56)
        at = 68 + owners * 25
        last_uid = 0
        parent = None
        for _ in range(items):
            row = struct.unpack_from("<QQQBQQQiBI", data, at)
            last_uid = row[0]
            if row[0] == parent_uid:
                parent = row
            at += 58 + row[-1]
        require(parent is not None and parent[3:6] == (1, 1, 0), "fixture root is not player-owned")
        ghost_uid = last_uid + 10000
        ghost = struct.pack("<QQQBQQQiBI", ghost_uid, parent_uid, parent_uid,
                            1, 1, 0, 1, 15, 1, 0)
        data[at:at] = ghost
        struct.pack_into("<I", data, 12, size + len(ghost))
        struct.pack_into("<I", data, 60, items + 1)
        data[24:56] = hashlib.sha256(data[56:]).digest()
        temporary = path.with_name("item_ownership.fixture")
        with temporary.open("wb") as output:
            os.fchmod(output.fileno(), 0o600)
            output.write(data)
            output.flush()
            os.fsync(output.fileno())
        os.replace(temporary, path)
        directory = os.open(path.parent, os.O_DIRECTORY)
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    return ghost_uid


def disputed_death(port: int, state_root: pathlib.Path, run_root: pathlib.Path) -> dict:
    client = reconnect_character(port)
    try:
        client.send("save")
        client.expect(f"Save complete for {CHARACTER}.")
        before = inspect_authority(state_root)
        banana = next(item["uid"] for item in before["player_items"] if item["vnum"] == 15)
        ghost = add_death_conflict(state_root, banana)
        require(any(item["uid"] == ghost and item["parent"] == banana
                    for item in inspect_authority(state_root)["player_items"]),
                "conflicting durable custody was not installed")
        # Wait for the real repository refusal while the character remains live.
        client.send("hit executioner")
        client.expect("Your wounds claim you at last", timeout=30)
        deadline = time.monotonic() + 15
        while f"error={errno.EMSGSIZE} disputed=1" not in runtime_logs(run_root):
            require(time.monotonic() < deadline, "death did not reach EMSGSIZE refusal")
            time.sleep(0.01)
        refused_at = time.monotonic()
        client.expect("ACCOUNT MENU", timeout=30)
        elapsed = time.monotonic() - refused_at
        require(not (state_root / "domains/.critical-authority-transaction").exists(),
                "character released before death after-images completed")
        after = inspect_authority(state_root)
        require(len(after["deaths"]) == 1, "death disposition missing at release")
        death = after["deaths"][0]
        require(any(item["uid"] == banana for item in death["items"]), "refused item payload lost")
        require(any(item["uid"] == banana and item["owner_type"] == 1 and item["owner_id"] == 1
                    for item in death["custody"]), "refused custody observation lost")
        require(after["wallet"] == [0, 0, 0, 0], "death wallet was not cleared")
        before_value = sum(amount * 10 ** denomination
                           for denomination, amount in enumerate(before["wallet"]))
        evidence_value = sum(amount * 10 ** denomination for item in death["items"]
                             for denomination, amount in enumerate(item["coins"]))
        require(before_value == evidence_value,
                "refused-death wallet value was not conserved in recovery evidence")
        require(not after["player_items"], "disputed custody remained active at release")
        require(after["death_count"] == before["death_count"] + 1,
                "refused death was counted more or less than once")
        logs = runtime_logs(run_root)
        require(logs.index("death_disposition_recorded") < logs.index("death_disposition_completed"),
                "character released before disposition durability")
        print(f"flatfile-primary EMSGSIZE refusal-to-account-menu (n=1, isolated): {elapsed:.3f}s", flush=True)
        client.send("0")
        return after
    finally:
        client.close()


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
    # No rules-agreement gate: keeping the character goes straight to the motd.
    client.expect("PRESS RETURN")
    client.send("")
    if expected_room:
        client.expect(expected_room, timeout=30)
    client.expect("Your starter kit is ready", timeout=30)


def complete_npc_combat_journey(client: MudClient, reset_coins: bool = False) -> None:
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
    combat_deadline = time.monotonic() + 45
    while True:
        remaining = combat_deadline - time.monotonic()
        require(remaining > 0, "Raoul survived the combat journey timeout")
        outcome, _ = client.expect_any(
            (
                "is dead! R.I.P.",
                "You stumble, but recover in time!",
                "You stumble in your attack, and jab at",
                "You stumble in your attack, and hit yourself!",
            ),
            timeout=remaining,
        )
        if outcome == "is dead! R.I.P.":
            break
        # A fumble can intentionally stop combat before the initial hit engages
        # either participant, or during a later round. Resume the journey just as
        # a player would instead of waiting for a fight that is no longer active.
        client.send("kill raoul")
    client.send("look")
    client.expect("corpse of raoul", timeout=10)
    client.send("look in corpse")
    corpse = client.expect("banana", timeout=10)
    require("corpse" in corpse.lower(), "loot marker was not inside the corpse")

    expected_coins = ("You get 0 platinum, 0 gold, 3 silver, and 0 copper coins." if reset_coins else
                      "You get 0 platinum, 0 gold, 0 silver, and 1 copper coins.")
    deadline = time.monotonic() + 15
    while True:
        client.send("get coins corpse")
        result, _ = client.expect_any((expected_coins,
            "The coin transfer did not commit; nothing changed.",
            "The coin transfer could not start; nothing changed."), timeout=15)
        if result == expected_coins:
            break
        require(time.monotonic() < deadline, "NPC coin retry never committed")
        # Other reward commands can hold the player's currency fence while the
        # room-only admission commits. Retry must use that admission, not mint.
        time.sleep(0.1)

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


def recover_player_corpse(port: int, reset_coins: bool = False) -> None:
    client = reconnect_character(port, "You rejoin the land of the living")
    try:
        client.send("look")
        client.expect("The corpse of a Human is lying here.", timeout=10)
        client.send(f"look in {CHARACTER}")
        corpse = client.expect("a banana", timeout=10)
        require(CHARACTER in corpse, "player corpse did not contain the saved loot marker")

        client.send(f"get banana {CHARACTER}")
        client.expect("get a banana", timeout=15)
        client.send(f"get coins {CHARACTER}")
        client.expect("You get 0 platinum, 0 gold, 3 silver, and 0 copper coins." if reset_coins else
                      "You get 0 platinum, 0 gold, 0 silver, and 1 copper coins.", timeout=15)
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


def build_flatfile_server(build_root: pathlib.Path) -> pathlib.Path:
    """Build an isolated flat-file journey server under build_root."""
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


def run_journey(binary: pathlib.Path, reset_coins: bool = False) -> None:
    with tempfile.TemporaryDirectory(prefix="duris-combat-state-") as state_tmp:
        with tempfile.TemporaryDirectory(prefix="duris-combat-run-") as run_tmp:
            state_root = pathlib.Path(state_tmp)
            run_root = pathlib.Path(run_tmp)
            state_root.chmod(0o700)
            (state_root / "domains").mkdir(mode=0o700)
            subprocess.run([str(INSPECTOR), str(state_root), "seed-combat"], check=True)
            (run_root / "logs/log").mkdir(parents=True)
            (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
            make_fixture(run_root, reset_coins)
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
                "DURIS_NEVENT_TRACE_PLAYER": "1",
                "CHAOS_MUD": "FALSE",
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
                    client.send("toggle boon")
                    client.expect("You will no longer be affected by boons.")
                    # Enter with the durable account-bank revision. Fresh character
                    # creation currently leaves its live bank revision at zero.
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=30)
                    client.send("0")
                    client.close()
                    client = reconnect_character(plain_port)
                    complete_npc_combat_journey(client, reset_coins)
                    client.close()
                    client = None
                    coin_balance = [0, 3, 0, 0] if reset_coins else [1, 0, 0, 0]
                    require(inspect_authority(state_root)["wallet"] == coin_balance,
                            "NPC pickup/save did not conserve coins")
                    verify_npc_loot_and_die(plain_port)
                    recover_player_corpse(plain_port, reset_coins)
                    verify_recovered_loot(plain_port)
                    require(inspect_authority(state_root)["wallet"] == coin_balance,
                            "reconnect or corpse recovery changed the coin total")
                    disputed_death(plain_port, state_root, run_root)

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
                    # Boot again with the same authority and journals. Compare the
                    # immutable evidence and wallet after re-entry and another save.
                    before_restart = inspect_authority(state_root)
                    death_files = {path.name: path.read_bytes()
                                   for path in (state_root / "player-deaths").glob("*.death")}
                    offset = output_path.stat().st_size
                    process = subprocess.Popen(
                        [str(binary), "--minimal", "-s", "-d", str(run_root), str(plain_port)],
                        cwd=run_root, env=environment, text=True,
                        stdout=output, stderr=subprocess.STDOUT)
                    deadline = time.monotonic() + 120
                    while b"Entering game loop." not in output_path.read_bytes()[offset:]:
                        require(process.poll() is None and time.monotonic() < deadline,
                                "combat server did not restart")
                        time.sleep(0.1)
                    client = reconnect_character(plain_port, "You rejoin the land of the living")
                    client.send("save")
                    client.expect(f"Save complete for {CHARACTER}.")
                    client.send("quit")
                    client.expect("ACCOUNT MENU", timeout=30)
                    client.send("0")
                    client.close()
                    client = None
                    after_restart = inspect_authority(state_root)
                    for field in ("wallet", "wallet_revision", "deaths", "player_items",
                                  "death_count", "experience", "level"):
                        require(after_restart[field] == before_restart[field],
                                f"restart/re-entry changed {field}")
                    require(death_files == {path.name: path.read_bytes()
                            for path in (state_root / "player-deaths").glob("*.death")},
                            "restart/re-entry rewrote death evidence")
                    require(not after_restart["snapshot_uids"],
                            "re-entry restored disputed inventory without recovery")
                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                    require(process.returncode == 0, "restarted server shutdown failed")
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
    subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                    "--build-inspector", str(INSPECTOR)], cwd=ROOT, check=True, timeout=180)
    # The root regression runner executes this and the Chaos kit journey
    # concurrently. Distinct temporary build roots prevent linker/runtime races
    # and are removed after each journey, including on failure.
    with tempfile.TemporaryDirectory(prefix=f"flatfile-combat-{os.getpid()}-",
                                     dir=ROOT / "bin/tests") as build_tmp:
        binary = build_flatfile_server(pathlib.Path(build_tmp))
        run_journey(binary)
        run_journey(binary, reset_coins=True)
    print("flat-file combat, player death, corpse recovery, save, and reconnect journey passed")
