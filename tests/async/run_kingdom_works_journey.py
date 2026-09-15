#!/usr/bin/env python3
"""Drive a kingdom's workshops and guild store through a real flat-file server.

This boots the FULL world (a kingdom needs the overhead map and the guildhall
zone, neither of which the minimal world has), so `make world` must have been
run first. A disposable character named Tyrus is created; character creation
makes that name an OVERLORD, which lets the journey found a guild, go where it
needs to, and load the harvest nodes a realm would otherwise take days to find.
Its coins come from a pile a zone reset places in a room, because coins a god
conjures are refused by the currency path.

The journey founds a guild with Tyrus as leader, finds a legal realm seat with
`kingdom prospect`, raises a main hall there and converts the guild to a
kingdom. It funds the treasury, is refused a store before any workshop and a
second forge, builds a forge and a store (checking the treasury paid for both),
harvests mineral and wood into the realm, then in the store lists and buys a
helm. It checks the helm was made at level 56 (Tyrus is above the cap) with
the level-56 armour class and hit points, that it is NOSELL, SOULBIND, CRAFTED
and STOREITEM with Tyrus's name in its keywords, that the realm's stores fell
by exactly the bill, and that Tyrus's platinum fell by the price while the
treasury did not rise.

The journey's own copy of lib/kingdom.cfg sets the build costs to 1,000
platinum and the material scale to a quarter, so the realm needs a minute of
harvesting rather than several; the price scale stays as designed.

    make world
    python3 tests/async/run_kingdom_works_journey.py --server bin/server/dms_new
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
ACCOUNT = "Worksacct"
CHARACTER = "Tyrus"
EMAIL = "works@example.invalid"
PROMPTS = ("Pos: standing >", "<>")
GUILD = "Ringfort"
SURFACE_START = 545733  # WH_MAP_VNUM, a surface map square near a hometown
MAP_WIDTH = 400  # the surface map is 400 squares on a side; +x east, +y south

STATION_COST_P = 1000
STORE_COST_P = 1000
RESOURCE_PERMILLE = 250
PRICE_PERMILLE = 1000


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def expected_bill(weight_tenths: int, level: int) -> tuple[int, int, int]:
    """(platinum, primary units, secondary units), the same curve as
    src/kingdom/kingdom_craft_math.h at the journey's scales."""
    level = max(1, min(56, level))
    platinum = (weight_tenths * (2 + level) * PRICE_PERMILLE + 5000) // 10000
    units = (weight_tenths * (8 + level) * RESOURCE_PERMILLE + 10000) // 20000
    primary = (units * 7 + 9) // 10 if units > 0 else 0
    return platinum, primary, units - primary


def create_god(client: journey.MudClient) -> None:
    entry, _ = client.expect_any(("term type", "account name"), timeout=60)
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
    client.expect("re-enter the same password to confirm")
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
    client.expect("PRESS RETURN")
    client.send("")
    client.expect_any(PROMPTS, timeout=60)


def command_any(client: journey.MudClient, line: str, untils: tuple[str, ...],
                timeout: float = 20) -> tuple[str, str]:
    """Send one command; return (the first of `untils` seen, everything it
    printed through the prompt after that), pressing Return through the pager.

    Reads the transcript from the moment the command is sent, never the
    client's pending buffer: every prompt is two lines that both count as a
    prompt, so the buffer always holds a stale one left by the command before,
    and waiting on it ended the next command before its own output arrived."""
    client.pending.clear()
    start = len(client.transcript)
    client.send(line)
    deadline = time.monotonic() + timeout
    pages = 0
    while True:
        text = bytes(client.transcript[start:]).decode("utf-8", errors="replace")
        while text.count("Return to continue") > pages:
            client.send("")
            pages += 1
        for until in untils:
            at = text.find(until)
            if at >= 0 and any(prompt in text[at + len(until):] for prompt in PROMPTS):
                return until, text
        require(time.monotonic() < deadline,
                f"'{line}' never said any of {untils!r}:\n{plain(text)[-3000:]}")
        client._receive()


def command(client: journey.MudClient, line: str, until: str, timeout: float = 20) -> str:
    """Send one command; return everything through the prompt after `until`."""
    return command_any(client, line, (until,), timeout)[1]


def plain(text: str) -> str:
    """Text with the MUD's &+x colour codes and ANSI escapes removed."""
    text = re.sub(r"\x1b\[[0-9;]*m", "", text)
    return re.sub(r"&[+\-]?[A-Za-z]|&[nN]", "", text)


def resources(client: journey.MudClient) -> dict[str, int]:
    text = plain(command(client, "kingdom status", "Resources"))
    found = dict((name, int(count)) for name, count in
                 re.findall(r"\b(mineral|wood|fibre|water)\s+(\d+)", text.split("Resources", 1)[1]))
    require(set(found) == {"mineral", "wood", "fibre", "water"}, f"no resource line in:\n{text}")
    return found


def purse_platinum(client: journey.MudClient) -> int:
    text = plain(command(client, "score", "Coins carried"))
    match = re.search(r"Coins carried:\s*(\d+)\s+platinum", text)
    require(match is not None, f"no coin line in score:\n{text}")
    return int(match.group(1))


def treasury_platinum(client: journey.MudClient) -> int:
    text = plain(command(client, "society", "Cash:"))
    match = re.search(r"Cash:\s*(\d+)\s+platinum", text)
    require(match is not None, f"no Cash line in society:\n{text}")
    return int(match.group(1))


def wait_for(probe, predicate, what: str, timeout: float = 20):
    """Poll `probe()` until `predicate(value)`; coin and item moves settle
    through the transaction coordinators, not instantly."""
    deadline = time.monotonic() + timeout
    value = probe()
    while not predicate(value):
        require(time.monotonic() < deadline, f"{what}: last saw {value!r}")
        time.sleep(0.5)
        value = probe()
    return value


def harvest_until(client: journey.MudClient, node_vnum: int, resource: str, want: int) -> None:
    """Load a node where Tyrus stands and work it until the realm holds `want`
    of `resource`, loading a fresh node whenever one is worked out."""
    command(client, f"load obj {node_vnum}", "")
    for _ in range(40):
        if resources(client)[resource] >= want:
            return
        _, raw = command_any(client, "kingdom harvest",
                             ("to your realm's stores", "exhausted", "nothing here",
                              "too exhausted", "concentration breaks"), timeout=60)
        text = plain(raw)
        require("too exhausted" not in text and "concentration breaks" not in text,
                f"harvest stopped:\n{text}")
        if "exhausted" in text or "nothing here" in text:
            command(client, f"load obj {node_vnum}", "")
    require(resources(client)[resource] >= want, f"could not harvest {want} {resource}")


def find_seat(client: journey.MudClient) -> int:
    """Let `kingdom prospect` find a legal seat, walking a widening grid of
    surface squares around SURFACE_START until it takes one or names one.

    One square is not enough: next to a hometown the survey can spend its
    whole work limit without a match, and says to try again nearby. Steps of
    12 squares, out to 72, keep every origin inside the 400-wide map, so no
    offset wraps onto another row."""
    origins = [(0, 0)]
    for ring in range(1, 7):
        step = 12 * ring
        origins += [(dx, dy) for dx in (-step, 0, step) for dy in (-step, 0, step)
                    if max(abs(dx), abs(dy)) == step]
    for dx, dy in origins:
        origin = SURFACE_START + dx + dy * MAP_WIDTH
        command(client, f"goto {origin}", "")
        here = plain(command(client, "kingdom prospect", "Prospecting", timeout=60))
        if "This square would take a realm" in here:
            return origin
        match = re.search(r"offset ([+-]\d+),([+-]\d+)", here)
        if match:
            return origin + int(match.group(1)) + int(match.group(2)) * MAP_WIDTH
    raise AssertionError(f"no legal realm seat within {12 * 6} squares of {SURFACE_START}")


def enter_hall(client: journey.MudClient) -> str:
    text = plain(command(client, "enter guildhall", ""))
    if "High-Arched Foyer" not in text:
        text = plain(command(client, "north", ""))
    require("High-Arched Foyer" in text, f"could not enter the hall:\n{text}")
    return text


def run(binary: pathlib.Path) -> None:
    ROOT.joinpath("bin/tests").mkdir(parents=True, exist_ok=True)
    require((ROOT / "areas/world.wld").is_file(), "run `make world` first: areas/world.wld is missing")
    with tempfile.TemporaryDirectory(prefix="duris-works-inspector-", dir=ROOT / "bin/tests") as tools, \
            tempfile.TemporaryDirectory(prefix="duris-works-state-") as state_tmp, \
            tempfile.TemporaryDirectory(prefix="duris-works-run-") as run_tmp:
        inspector = pathlib.Path(tools) / "inspector"
        subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                        "--build-inspector", str(inspector)], cwd=ROOT, check=True, timeout=300)
        state_root, run_root = pathlib.Path(state_tmp), pathlib.Path(run_tmp)
        state_root.chmod(0o700)
        (state_root / "domains").mkdir(mode=0o700)
        subprocess.run([str(inspector), str(state_root), "seed-combat"], check=True, timeout=30)
        (run_root / "logs/log").mkdir(parents=True)
        (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
        journey.make_fixture(run_root)
        journey.generate_certificate(run_root)
        (run_root / "journals/players").mkdir(parents=True, mode=0o700)
        (run_root / "journals/critical").mkdir(mode=0o700)

        # The journey's own settings, on its own copy of the file.
        cfg = run_root / "lib/kingdom.cfg"
        text = cfg.read_text()
        for key, value in (("kingdom.enabled", "1"),
                           ("kingdom.station.cost", str(STATION_COST_P * 1000)),
                           ("kingdom.store.cost", str(STORE_COST_P * 1000)),
                           ("kingdom.craft.price.permille", str(PRICE_PERMILLE)),
                           ("kingdom.craft.resource.permille", str(RESOURCE_PERMILLE))):
            text, count = re.subn(rf"(?m)^{re.escape(key)}\s*=.*$", f"{key} = {value}", text)
            require(count == 1, f"{key} not in lib/kingdom.cfg")
        cfg.write_text(text)

        plain_port, tls_port, websocket_port = journey.available_ports()
        environment = {
            "PATH": os.environ.get("PATH", "/usr/bin:/bin"), "ENVIRONMENT": "local",
            "PERSISTENCE_MODE": "flatfile-primary", "FLATFILE_STATE_DIR": str(state_root),
            "PLAYER_SAVE_JOURNAL_DIR": str(run_root / "journals/players"),
            "CRITICAL_COMMAND_JOURNAL_DIR": str(run_root / "journals/critical"),
            "LISTEN_ADDRESS": "127.0.0.1", "DURIS_TLS_PORT": str(tls_port),
            "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1", "DURIS_WEBSOCKET_PORT": str(websocket_port),
            "REDIS": "FALSE", "CHAOS_MUD": "FALSE", "CREATION_ALL_CLASSES": "TRUE",
        }
        if os.environ.get("LD_LIBRARY_PATH"):
            environment["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
        output_path = run_root / "server.out"
        with output_path.open("w", encoding="utf-8") as output:
            # NO -s here, unlike the other journeys: -s suppresses special
            # routines, and the guild store IS one -- its room proc answers
            # `list` and `buy`, and under -s they fall through to "you cannot
            # do that here". The guildhall door and the node procs want them too.
            process = subprocess.Popen([str(binary), "-d", str(run_root), str(plain_port)],
                                       cwd=run_root, env=environment, text=True,
                                       stdout=output, stderr=subprocess.STDOUT)
            client = None
            try:
                deadline = time.monotonic() + 900
                while time.monotonic() < deadline and "Entering game loop." not in output_path.read_text(errors="replace"):
                    require(process.poll() is None, "server exited during boot:\n" + output_path.read_text(errors="replace")[-6000:])
                    time.sleep(0.5)
                require("Entering game loop." in output_path.read_text(errors="replace"), "server did not boot in time")
                print("booted the full world", flush=True)
                client = journey.MudClient(plain_port)
                create_god(client)

                # A guild, led by Tyrus.
                command(client, f"supervise found {CHARACTER} n {GUILD}", "")
                require(GUILD in plain(command(client, "society", GUILD)), "the guild was not founded")

                # A realm seat, a main hall on it, and the conversion.
                seat = find_seat(client)
                command(client, f"goto {seat}", "")
                require("This square would take a realm" in plain(command(client, "kingdom prospect", "Prospecting", timeout=60)),
                        f"prospect's seat {seat} does not take a realm")
                built = plain(command(client, "construct guildhall", ""))
                require("guildhall" in built.lower(), f"construct guildhall:\n{built}")
                command(client, "kingdom convert", "")
                status = plain(command(client, "kingdom status", "Resources"))
                require("The realm of" in status and "Works      : none" in status, f"not a realm:\n{status}")
                print(f"realm founded on map vnum {seat}", flush=True)

                # Coins for Tyrus, and a treasury funded through a bank.
                # Coins a god conjures with `load` are refused by the currency
                # path ("The coin transfer did not commit"), so the purse is
                # filled the way a player's is: from the pile a zone reset
                # places in a room. Room 58449 holds #58423, "a huge pile of
                # platinum coins", 5,000 platinum.
                command(client, "goto 58449", "")
                reply = plain(command(client, "get pile", ""))
                require("You get" in reply, f"get pile in room 58449:\n{reply}")
                purse = wait_for(lambda: purse_platinum(client), lambda p: p >= 5000,
                                 "the coin pile never reached the purse")
                command(client, f"goto {seat}", "")
                enter_hall(client)
                command(client, "load obj 3097", "")
                command(client, "kingdom deposit 3000 platinum", "")
                treasury = wait_for(lambda: treasury_platinum(client), lambda t: t >= 3000, "the deposit never reached the treasury")
                print(f"purse {purse}p, treasury {treasury}p", flush=True)

                # The build: refusals first, then a forge and a store, each paid for.
                reply = plain(command(client, "kingdom build store west", "store"))
                require("needs something to sell" in reply, f"store before a workshop was not refused:\n{reply}")
                reply = plain(command(client, "kingdom build forge east", "forge"))
                require("raise your realm's forge east" in reply, f"forge was not built:\n{reply}")
                reply = plain(command(client, "kingdom build forge up", "forge"))
                require("already has a forge" in reply, f"a second forge was not refused:\n{reply}")
                reply = plain(command(client, "kingdom build store west", "store"))
                require("raise your realm's store west" in reply, f"store was not built:\n{reply}")
                after_builds = treasury_platinum(client)
                require(after_builds == treasury - STATION_COST_P - STORE_COST_P,
                        f"treasury {treasury}p -> {after_builds}p after two builds")
                status = plain(command(client, "kingdom status", "Resources"))
                require("Works      : forge and store" in status, f"status does not list the works:\n{status}")
                society = plain(command(client, "society", "Realm works"))
                require(re.search(r"Realm works:\s*forge and store", society) is not None,
                        f"society does not list the works:\n{society}")
                forge = plain(command(client, "east", ""))
                require("The Forge" in forge and "blackened fieldstone" in forge, f"forge room:\n{forge}")
                command(client, "west", "")
                print(f"forge and store built; treasury {treasury}p -> {after_builds}p", flush=True)

                # Material for a level-56 helm (weight 1.0) at the journey's scale.
                # Nodes are never worked on ground a realm holds, and the seat
                # square is the realm's, so gather ten squares east: clear of
                # the whole 9x9 footprint.
                price, mineral_bill, wood_bill = expected_bill(10, 56)
                command(client, f"goto {seat + 10}", "")
                harvest_until(client, 477, "mineral", mineral_bill)
                harvest_until(client, 478, "wood", wood_bill)
                command(client, f"goto {seat}", "")
                enter_hall(client)
                before = resources(client)
                purse_before = purse_platinum(client)
                treasury_before = treasury_platinum(client)
                print(f"stores {before}, purse {purse_before}p, treasury {treasury_before}p", flush=True)

                # The store: list, a refusal, and a purchase.
                room = plain(command(client, "west", ""))
                require("The Guild Store" in room and "Stout shelves" in room, f"store room:\n{room}")
                listing = plain(command(client, "list", "buy <item>"))
                require("The Guild Store of" in listing and "at your level: 56" in listing,
                        f"list:\n{listing}")
                helm = re.search(r"^\s*helm\s+(\d+) platinum\s+(\d+) mineral, (\d+) wood", listing, re.M)
                require(helm is not None and (int(helm.group(1)), int(helm.group(2)), int(helm.group(3)))
                        == (price, mineral_bill, wood_bill),
                        f"helm line wanted {price}p {mineral_bill} mineral {wood_bill} wood:\n{listing}")
                # Rows only: the footer's own example is 'buy ring health'.
                require("From the jeweller" not in listing and "From the loom" not in listing
                        and re.search(r"^\s*(ring|bracelet|necklace|cloak|robe)\s+\d+ platinum",
                                      listing, re.M) is None,
                        f"a forge-only store lists another workshop's work:\n{listing}")
                reply = plain(command(client, "buy ring health", "ring"))
                require("make no 'ring'" in reply, f"a ring was sold without a jeweller:\n{reply}")
                reply = plain(command(client, "buy helm", "platinum"))
                require(f"You pay {price} platinum" in reply, f"buy helm:\n{reply}")
                wait_for(lambda: plain(command(client, "inventory", "")),
                         lambda text: "steel helm" in text, "the helm never arrived")

                # The piece itself.
                stat = plain(command(client, "stat obj helm", "Extra2", timeout=30))
                for flag in ("SOULBIND", "CRAFTED", "STOREITEM"):
                    require(flag in stat, f"helm is not {flag}:\n{stat}")
                require("NOSELL" in stat, f"helm is not NOSELL:\n{stat}")
                require(re.search(r"AC-apply:\s*10\b", stat) is not None, f"level-56 helm AC is not 10:\n{stat}")
                require(re.search(r"Affects:\s*\S+\s+By\s+8\b", stat) is not None, f"level-56 helm HP is not +8:\n{stat}")
                require("tyrus" in stat.lower(), f"helm keywords lack the buyer:\n{stat}")
                for effect in ("HASTE", "SANCTUARY", "FIRESHIELD"):
                    require(effect not in stat, f"helm carries {effect}:\n{stat}")

                # What it cost: the realm's material, the buyer's coin, and nothing to the treasury.
                after = resources(client)
                require(after["mineral"] == before["mineral"] - mineral_bill and
                        after["wood"] == before["wood"] - wood_bill and
                        after["fibre"] == before["fibre"] and after["water"] == before["water"],
                        f"stores {before} -> {after}, bill {mineral_bill} mineral {wood_bill} wood")
                purse_after = wait_for(lambda: purse_platinum(client), lambda p: p <= purse_before - price,
                                       "the purse never paid")
                require(purse_after == purse_before - price, f"purse {purse_before}p -> {purse_after}p, price {price}p")
                require(treasury_platinum(client) == treasury_before, "the treasury moved on a store purchase")
                print(f"bought a level-56 helm: {price}p destroyed, {mineral_bill} mineral + {wood_bill} wood "
                      f"drawn; purse {purse_before}p -> {purse_after}p; treasury unchanged at {treasury_before}p",
                      flush=True)

                client.send("quit")
                client.expect_any(("ACCOUNT MENU", "Goodbye", "account menu"), timeout=30)
                client.close()
                client = None
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=60)
                log = output_path.read_text(errors="replace")
                require("FATAL:" not in log and "assert:" not in log, "server logged a fatal or assertion")
                print("[PASS] kingdom works journey: realm, builds and refusals, harvest, list, buy, "
                      "level-56 stats, flags, binding, material draw, destroyed platinum", flush=True)
            except Exception as error:
                transcript = bytes(client.transcript).decode("utf-8", errors="replace") if client else ""
                raise AssertionError(f"{error}\n--- transcript tail ---\n{plain(transcript)[-6000:]}"
                                     f"\n--- server output ---\n"
                                     + output_path.read_text(errors="replace")[-6000:]) from error
            finally:
                if client is not None:
                    client.close()
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=20)
                    except subprocess.TimeoutExpired:
                        process.kill()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=pathlib.Path, required=True)
    binary = parser.parse_args().server.resolve()
    require(binary.is_file() and os.access(binary, os.X_OK), f"not executable: {binary}")
    run(binary)


if __name__ == "__main__":
    main()
