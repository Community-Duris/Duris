#!/usr/bin/env python3
"""Release walkthrough using a disposable world, two accounts and real TCP clients.

Run on Linux after building the flatfile server and the player repository inspector.
No environment file or existing account/state directory is read. The optional JSON
report contains only synthetic command results and rendered room frames.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import signal
import socket
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/async"))
from test_flatfile_combat_journey import (  # noqa: E402
    ANSI, MudClient, PASSWORD, available_ports, create_character,
    generate_certificate, make_fixture, require,
)


class ColorClient(MudClient):
    """Separate Telnet control packets from terminal bytes, including split packets."""

    def __init__(self, port: int, index: int):
        super().__init__(port, source_host=f"127.0.0.{index + 1}")
        self.wire = bytearray()
        self.terminal = bytearray()
        self.chat: list[dict] = []

    def _receive(self) -> bool:
        try:
            chunk = self.socket.recv(65536)
        except socket.timeout:
            return False
        require(bool(chunk), "server closed the gameplay connection")
        self.wire.extend(chunk)
        text = bytearray()
        while self.wire:
            if self.wire[0] != 255:
                text.append(self.wire.pop(0))
                continue
            if len(self.wire) < 2:
                break
            command = self.wire[1]
            if command == 255:
                text.append(255)
                del self.wire[:2]
            elif command == 250:
                end = self.wire.find(b"\xff\xf0", 2)
                if end < 0:
                    break
                packet = bytes(self.wire[2:end]).replace(b"\xff\xff", b"\xff")
                if packet.startswith(b"\xc9Comm.Channel "):
                    self.chat.append(json.loads(packet[14:]))
                del self.wire[:end + 2]
            elif command in (251, 252, 253, 254):
                if len(self.wire) < 3:
                    break
                del self.wire[:3]
            else:
                del self.wire[:2]
        self.terminal.extend(text)
        cleaned = ANSI.sub(b"", text)
        self.pending.extend(cleaned)
        self.transcript.extend(cleaned)
        return True

    def drain(self) -> None:
        # Wait for the current game pulse/output, then drain until one quiet read.
        deadline = time.monotonic() + 3
        while time.monotonic() < deadline and self._receive():
            pass

    def enable_gmcp(self) -> None:
        self.socket.sendall(b"\xff\xfd\xc9")
        self.drain()


def foregrounds(raw: bytes, word: str) -> list[int]:
    """Read actual ANSI foregrounds, in the protocol's legacy BGR color order."""
    visible: list[str] = []
    colors: list[int] = []
    base, bold, at = 0, False, 0
    text = raw.decode("utf-8")
    sgr = re.compile(r"\x1b\[([0-9;]*)m")
    for match in sgr.finditer(text):
        for char in text[at:match.start()]:
            visible.append(char)
            colors.append(base + (8 if base and bold else 0))
        for value in (int(p or 0) for p in match[1].split(";")):
            if value == 0:
                base, bold = 0, False
            elif value == 1:
                bold = True
            elif value == 22:
                bold = False
            elif 30 <= value <= 37:
                base = (16, 20, 18, 22, 17, 21, 19, 23)[value - 30]
            elif value == 39:
                base = 0
        at = match.end()
    for char in text[at:]:
        visible.append(char)
        colors.append(base + (8 if base and bold else 0))
    start = "".join(visible).find(word)
    require(start >= 0, f"missing rendered word {word!r}")
    return colors[start:start + len(word)]


def run(server: Path, inspector: Path, report_path: Path | None) -> None:
    checks: list[dict] = []
    frames: list[dict] = []
    clients: list[ColorClient] = []
    accounts = (("Journeyacct", "Taverek"), ("Coloracct", "Colorbob"))
    with tempfile.TemporaryDirectory(prefix="duris-colorization-") as temporary:
        root = Path(temporary)
        state, runtime = root / "state", root / "run"
        state.mkdir(mode=0o700)
        (state / "domains").mkdir(mode=0o700)
        runtime.mkdir()
        subprocess.run([str(inspector), str(state), "seed-combat"], check=True)
        make_fixture(runtime)
        generate_certificate(runtime)
        zone = runtime / "areas_mini/mini.zon"
        zone.write_text(re.sub(r"^[MG] .*\n", "", zone.read_text(), flags=re.M))
        world = runtime / "areas_mini/mini.wld"
        prose = "".join(f"Water crosses the forest. Gate {i:02d}.\n" for i in range(30))
        mural = ("&+WAuthored survey mural&n\n"
                 "+------------+-------+\n"
                 "| &+Gforest&n     | water |\n"
                 "+------------+-------+\n"
                 "&+BBlue&+C gradient&n, river and runes stay authored.\n")
        original = "This quiet stone arena exists to prove the complete combat journey.\n~\n1 0 0\nS\n"
        require(original in world.read_text(), "release room fixture changed")
        world.write_text(world.read_text().replace(original,
            prose + "~\n1 0 0\nE\nmural~\n" + mural + "~\nS\n"))
        (runtime / "logs/log").mkdir(parents=True)
        (runtime / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
        for name in ("players", "critical"):
            (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
        port, tls, web = available_ports()
        env = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"), "ENVIRONMENT": "local",
            "PERSISTENCE_MODE": "flatfile-primary", "FLATFILE_STATE_DIR": str(state),
            "PLAYER_SAVE_JOURNAL_DIR": str(runtime / "journals/players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(runtime / "journals/critical"),
            "LISTEN_ADDRESS": "127.0.0.1", "DURIS_TLS_PORT": str(tls),
            "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1", "DURIS_WEBSOCKET_PORT": str(web),
            "REDIS": "FALSE", "CHAOS_MUD": "FALSE",
            "DURIS_OUTPUT_PROFILES_FILE": str(ROOT / "docs/examples/scenery-profiles-v1.json"),
        }
        with (runtime / "server.out").open("w") as log:
            process = subprocess.Popen(
                [str(server), "--minimal", "-s", "-d", str(runtime), str(port)],
                cwd=runtime, env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                deadline = time.monotonic() + 120
                while "Entering game loop." not in (runtime / "server.out").read_text(errors="replace"):
                    require(process.poll() is None and time.monotonic() < deadline,
                            "isolated server did not boot")
                    time.sleep(0.1)

                def command(client: ColorClient, line: str, expected: str) -> bytes:
                    client.drain()
                    client.pending.clear()
                    start = len(client.terminal)
                    client.send(line)
                    client.expect(expected, timeout=45 if line == "quit" else 15)
                    client.drain()
                    checks.append({"command": line, "expected": expected})
                    print(f"PASS {line}: {expected}", flush=True)
                    return bytes(client.terminal[start:])

                for index, (account, character) in enumerate(accounts):
                    client = ColorClient(port, index)
                    clients.append(client)
                    create_character(client, account=account, character=character,
                                     email=f"color{index}@example.invalid")
                    client.enable_gmcp()
                    # Consume/quit a possible login page before normal commands.
                    client.send("q")
                    client.drain()
                    command(client, "toggle color", "Color settings")
                    color = "bright cyan" if index == 0 else "bright red"
                    for channel in ("say", "tell"):
                        command(client, f"toggle color {channel} {color}", "save pending")
                    command(client, "toggle color tell bright", "bright cyan")
                    command(client, "toggle color tell chartreuse", "Unknown color")
                    command(client, "toggle color preview tell yellow", "Sample (tell)")
                    command(client, "toggle color tell", f"tell: {color}.")
                    command(client, "save", f"Save complete for {character}.")

                def chat_pair(label: str) -> None:
                    for sender, receiver in ((0, 1), (1, 0)):
                        client, target = clients[sender], clients[receiver]
                        target.drain()
                        start = len(target.chat)
                        terminal_start = len(target.terminal)
                        marker = f"color-{label}-{sender}"
                        client.send(f"tell {accounts[receiver][1]} {marker}")
                        target.expect(marker)
                        target.drain()
                        messages = [p for p in target.chat[start:] if marker in p.get("text", "")]
                        require(len(messages) == 1, "tell must deliver one structured message")
                        presentation = messages[0]["presentation"]
                        expected = 27 if receiver == 0 else 28
                        require(any(r[2] == expected for r in presentation["runs"]),
                                "recipient's independent color was not applied")
                        require(foregrounds(bytes(target.terminal[terminal_start:]), marker) ==
                                [expected] * len(marker), "terminal and GMCP recipient colors differ")
                        require(presentation["channelId"] == 12, "wrong structured channel")
                        checks.append({"chat": label, "recipient": accounts[receiver][1],
                                       "foreground": expected, "packets": len(messages)})
                        print(f"PASS {label}: independent recipient {receiver}", flush=True)

                chat_pair("selected")
                for index, (_, character) in enumerate(accounts):
                    command(clients[index], "quit", "ACCOUNT MENU")
                    clients[index].send("0")
                    clients[index].close()
                clients.clear()
                for index, (account, character) in enumerate(accounts):
                    client = ColorClient(port, index)
                    clients.append(client)
                    entry, _ = client.expect_any(("term type", "account name"))
                    if entry == "term type":
                        client.send("9")
                        client.expect("account name")
                    client.send(account)
                    client.expect("enter your password")
                    client.send(PASSWORD)
                    client.expect("PRESS RETURN")
                    client.send("")
                    client.expect("Please select an option")
                    client.send("1")
                    client.expect(character)
                    client.send("1")
                    client.expect("Play as")
                    client.send("y")
                    client.expect("The Regression Arena")
                    client.enable_gmcp()
                    client.send("q")
                    client.drain()
                    for channel in ("say", "tell"):
                        color = "bright cyan" if index == 0 else "bright red"
                        command(client, f"toggle color {channel}", f"{channel}: {color}.")
                chat_pair("reconnected")
                alice, bob = clients
                command(alice, "toggle color reset tell", "save pending")
                command(alice, "toggle color tell", "tell: default.")
                command(alice, "toggle color say", "say: bright cyan.")
                command(bob, "toggle color tell", "tell: bright red.")
                command(alice, "toggle color reset all", "save pending")
                command(alice, "toggle color say", "say: default.")
                command(bob, "toggle color say", "say: bright red.")

                # Thirty repeated prose lines force paging. Refresh and back use
                # already frozen output; new looks alone advance the room frame.
                command(alice, "toggle color room animated", "save pending")
                command(alice, "toggle screensize 12", "Screen length set to 12 lines.")
                paging = command(alice, "toggle paging", "Paging")
                if b"mode off" in ANSI.sub(b"", paging):
                    command(alice, "toggle paging", "Paging")

                def room(label: str, action: str = "look room") -> tuple[list[int], list[int]]:
                    raw = command(alice, action, "Gate 00.")
                    water, forest = foregrounds(raw, "Water"), foregrounds(raw, "forest")
                    frames.append({"label": label, "terminalAnsi": raw.decode("utf-8"),
                                   "water": water, "forest": forest})
                    return water, forest

                first = room("animated first")
                require(room("pager refresh", "r") == first, "pager refresh changed frozen colors")
                alice.send("q")
                alice.drain()
                command(alice, "say unrelated-chat", "unrelated-chat")
                second = room("animated after unrelated chat")
                require(first[0] != second[0], "new room output did not advance water")
                require(all(first[0][i] == second[0][(i + 1) % 5] for i in range(5)),
                        "unrelated chat advanced the room phase or water lost coherence")
                alice.send("q")
                alice.drain()
                animated = [first, second]
                for index in range(4):
                    frame = room(f"animated continuation {index + 1}")
                    previous = animated[-1]
                    require(all(previous[0][i] == frame[0][(i + 1) % 5] for i in range(5)),
                            "water frame lost one-step coherence")
                    require(frame[1] == previous[1] or
                            all(previous[1][i] == frame[1][(i + 1) % 6] for i in range(6)),
                            "forest shimmer rerolled instead of drifting")
                    animated.append(frame)
                    alice.send("q")
                    alice.drain()
                require(len({tuple(frame[1]) for frame in animated}) > 1,
                        "forest shimmer did not advance over repeated room prose")
                command(alice, "toggle color motion off", "save pending")
                still = room("motion off first")
                alice.send("q")
                alice.drain()
                require(room("motion off repeated") == still, "motion off changed scenery")
                alice.send("q")
                alice.drain()
                command(alice, "toggle color motion on", "save pending")
                command(alice, "toggle color room static", "save pending")
                fixed = room("static first")
                alice.send("q")
                alice.drain()
                require(room("static repeated") == fixed, "static scenery changed")
                alice.send("q")
                alice.drain()
                command(alice, "toggle color title bright cyan", "save pending")
                room("separate title and room body")
                require(foregrounds(frames[-1]["terminalAnsi"].encode(), "The Regression Arena") ==
                        [27] * len("The Regression Arena"), "title did not remain independent")
                alice.send("q")
                alice.drain()
                command(alice, "toggle paging", "Paging mode off")
                command(alice, "toggle color inspect bright red", "save pending")
                art = command(alice, "look mural", "runes stay authored.")
                require(b"+------------+-------+" in ANSI.sub(b"", art), "mural layout changed")
                require(foregrounds(art, "water") == [0] * 5, "art acquired the selected base color")
                require(foregrounds(art, "forest") == [26] * 6, "authored art color changed")
                frames.append({"label": "protected survey mural", "terminalAnsi": art.decode("utf-8")})
                dense = bytearray()
                for channel, color in (("incoming", "bright red"), ("outgoing", "bright cyan"),
                                       ("observed", "yellow")):
                    dense.extend(command(alice, f"toggle color preview {channel} {color}",
                                         f"Sample ({channel})"))
                frames.append({"label": "dense combat previews", "terminalAnsi": dense.decode("utf-8")})
                low = command(alice, "toggle color preview prompt bright cyan", "10v")
                require(foregrounds(low, "40m") == [22] * 3 and foregrounds(low, "10v") == [20] * 3,
                        "selected prompt base hid the warning colors")
                frames.append({"label": "low-resource prompt preview", "terminalAnsi": low.decode("utf-8")})
                for client, (_, character) in zip(clients, accounts):
                    command(client, "save", f"Save complete for {character}.")
                    command(client, "quit", "ACCOUNT MENU")
                    client.send("0")
                    client.close()
                clients.clear()
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=30)
                require(process.returncode == 0, "isolated server shutdown failed")
                if report_path:
                    report_path.write_text(json.dumps({
                        "transport": "real server TCP Telnet with GMCP", "checks": checks,
                        "frames": frames,
                    }, indent=2), encoding="utf-8")
                print(f"PASS live colorization journey: {len(checks)} checks, {len(frames)} frames")
            except Exception:
                print((runtime / "server.out").read_text(errors="replace")[-5000:])
                raise
            finally:
                for client in clients:
                    client.close()
                if process.poll() is None:
                    process.terminate()
                    process.wait(timeout=30)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, default=ROOT / "bin/server/dms_new")
    parser.add_argument("--inspector", type=Path, default=ROOT / "bin/tests/color-inspector")
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    run(args.server.resolve(), args.inspector.resolve(), args.report)
