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
kingdom. With too little in the treasury it is refused a forge, and checks
that nothing was built and nothing charged. It funds the treasury, is refused
a store before any workshop and a second forge, and builds a forge and a store
(checking the treasury paid for both). Before any harvest the store refuses a
purchase the realm's stores cannot supply, taking nothing. It harvests mineral
and wood into the realm, then in the store lists and buys a pair of
vambraces. It checks they were made at level 56 (Tyrus is above the cap) with
the level-56 armour class and strength, that they are CRAFTED and STOREITEM
but neither SOULBIND nor NOSELL, that they are worth 5,800 copper -- a tenth of
the 58p they cost -- that their keywords carry the buyer's name but NOT the
maker's mark (which lives in the action description, where no command reaches
it), that the realm's stores fell by exactly the bill, and that Tyrus's
platinum fell by the price while the treasury did not rise.

Then who may wear it. Store gear binds to nobody (ruled 2026-09-16), so it is
ordinary property: Tyrus wears the piece, takes it off, puts it in a basket and
leaves the basket for two characters on other accounts -- one NAMED
"Vambraces", a word on the piece, and one with an ordinary name. Each sheds its
starter kit (a new character starts over its carrying limit), takes the piece
out of the basket and wears it.

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
SHORT_TREASURY_P = 500  # less than a workshop costs

PIECE = "vambraces"  # weight 1.0: the same bill as a helm
PIECE_SHORT = "steel vambraces"
# "a small woven basket": an open container whose keyword no starter kit
# carries. A new character's kit includes "a small leather bag", so a bag
# would be ambiguous -- `get vambraces bag` looks in the kit's own bag.
BASKET_VNUM = 387
BASKET = "basket"
# Characters who take Tyrus's piece and wear it. Nothing binds store gear, so
# both must manage it. "Vambraces" is a word on the piece and no mob's keyword,
# so character creation allows it; it stays here as the name most likely to
# trip a stray name check, being the one the old soulbind test let through.
OTHERS = (("Worksthief", "thief@example.invalid", "Vambraces"),
          ("Workspeer", "peer@example.invalid", "Quillomen"))


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


def create_account(client: journey.MudClient, account: str, email: str) -> None:
    entry, _ = client.expect_any(("term type", "account name"), timeout=60)
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


def create_god(client: journey.MudClient) -> None:
    """Tyrus: character creation makes that name an OVERLORD."""
    create_account(client, ACCOUNT, EMAIL)
    create_character(client, CHARACTER)


def create_character(client: journey.MudClient, name: str) -> None:
    """A human warrior named `name`, from the account menu into the game."""
    client.expect("Please select an option")
    client.send("2")
    client.expect("Enter your new name")
    client.send(name)
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


def wore_the_piece(other: journey.MudClient, name: str) -> None:
    """`name` takes Tyrus's piece out of the basket at their feet, wears it,
    and puts it back.

    Store gear is ordinary property (ruled 2026-09-16): nothing binds it to the
    character who bought it, so another character on another account may take
    it and wear it.

    A new character starts over its carrying limit -- the starter kit is 28
    items against a limit of 11 -- and can pick up nothing until it sheds
    some, so the kit goes on the floor first. The kit arrives through the item
    coordinator, so an early `drop all` can find it still in flight; the drop
    is repeated until the load is under the limit, and every reply is kept for
    the failure message.

    Every step waits for words of its own. A new character is sent output of
    its own accord -- the kit arriving, a kit item crumbling -- each with a
    prompt, and a step that waited only for the next prompt could return
    before its own reply and read the one meant for the next step."""

    def inventory() -> str:
        return plain(command(other, "inventory", "You are carrying"))

    replies = []
    for _ in range(10):
        carrying = re.search(r"You are carrying: \((\d+)/(\d+)\)", inventory())
        if carrying and int(carrying.group(1)) < int(carrying.group(2)):
            break
        replies.append(plain(command(other, "drop all", "")))
        time.sleep(1)
    else:
        raise AssertionError(f"{name} never got under the carrying limit; last drops:\n"
                             + "\n----\n".join(reply[-800:] for reply in replies[-3:]))
    got = plain(command(other, f"get {PIECE} {BASKET}", ""))
    wait_for(inventory, lambda text: PIECE_SHORT in text,
             f"{name} never got the piece out of the {BASKET} (the get said: {got[-400:]!r})")
    # Either the piece's own name, which a successful wear prints, or the
    # soulbind refusal; the require then says which it was.
    _, reply = command_any(other, f"wear {PIECE}", (PIECE_SHORT, "bound to someone"))
    reply = plain(reply)
    require("bound to someone" not in reply, f"{name} was refused Tyrus's piece:\n{reply}")
    worn = plain(command(other, "equipment", ""))
    require(PIECE_SHORT in worn, f"{name} could not wear Tyrus's piece:\n{worn}")
    command(other, f"remove {PIECE}", "")
    wait_for(inventory, lambda text: PIECE_SHORT in text,
             f"{name} never took the piece off again")
    command(other, f"put {PIECE} {BASKET}", "")
    wait_for(inventory, lambda text: PIECE_SHORT not in text, f"{name} never put the piece back")


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
            others: list[journey.MudClient] = []
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

                # A build the treasury cannot pay for builds nothing and takes nothing.
                command(client, f"kingdom deposit {SHORT_TREASURY_P} platinum", "")
                short = wait_for(lambda: treasury_platinum(client), lambda t: t >= SHORT_TREASURY_P,
                                 "the first deposit never reached the treasury")
                require(short < STATION_COST_P, f"treasury {short}p already pays for a forge")
                reply = plain(command(client, "kingdom build forge east", "forge"))
                require("cannot pay" in reply, f"an unaffordable forge was not refused:\n{reply}")
                require(treasury_platinum(client) == short, "a refused build moved the treasury")
                status = plain(command(client, "kingdom status", "Resources"))
                require("Works      : none" in status, f"a refused build left works behind:\n{status}")
                require("The Forge" not in plain(command(client, "east", "")),
                        "a refused build left a room east of the foyer")

                command(client, f"kingdom deposit {3000 - short} platinum", "")
                treasury = wait_for(lambda: treasury_platinum(client), lambda t: t >= 3000, "the deposit never reached the treasury")
                print(f"purse {purse}p, treasury {treasury}p; an unaffordable forge was refused", flush=True)

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

                # A purchase the realm's stores cannot supply takes nothing.
                empty = resources(client)
                purse_empty = purse_platinum(client)
                command(client, "west", "")
                reply = plain(command(client, f"buy {PIECE}", "harvested"))
                require("cannot supply it" in reply, f"a purchase without material was not refused:\n{reply}")
                require(resources(client) == empty, "a refused purchase moved the realm's stores")
                require(purse_platinum(client) == purse_empty, "a refused purchase took platinum")
                require(PIECE_SHORT not in plain(command(client, "inventory", "")),
                        "a refused purchase made a piece")
                command(client, "east", "")
                print("a purchase the realm's stores could not supply was refused", flush=True)

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
                piece = re.search(rf"^\s*{PIECE}\s+(\d+) platinum\s+(\d+) mineral, (\d+) wood", listing, re.M)
                require(piece is not None and (int(piece.group(1)), int(piece.group(2)), int(piece.group(3)))
                        == (price, mineral_bill, wood_bill),
                        f"{PIECE} line wanted {price}p {mineral_bill} mineral {wood_bill} wood:\n{listing}")
                reply = plain(command(client, f"buy {PIECE}", "platinum"))
                require(f"You pay {price} platinum" in reply, f"buy {PIECE}:\n{reply}")
                wait_for(lambda: plain(command(client, "inventory", "")),
                         lambda text: PIECE_SHORT in text, "the vambraces never arrived")

                # The piece itself.
                stat = plain(command(client, f"stat obj {PIECE}", "Extra2", timeout=30))
                for flag in ("CRAFTED", "STOREITEM"):
                    require(flag in stat, f"the vambraces are not {flag}:\n{stat}")
                # Ruled 2026-09-16: ordinary property. A shop refuses NOSELL
                # outright, and soulbound gear cannot be given or looted at all.
                for flag in ("SOULBIND", "NOSELL"):
                    require(flag not in stat, f"the vambraces are still {flag}:\n{stat}")
                # Worth a tenth of the 58p price, in copper: `stat obj` prints
                # it as the item's Value.
                require("5,800" in stat, f"the vambraces are not worth 5,800 copper:\n{stat}")
                require(re.search(r"AC-apply:\s*10\b", stat) is not None, f"level-56 vambraces AC is not 10:\n{stat}")
                require(re.search(r"Affects:\s*\S+\s+By\s+2\b", stat) is not None,
                        f"level-56 vambraces strength is not +2:\n{stat}")
                require("tyrus" in stat.lower(), f"the keywords lack the buyer's name:\n{stat}")
                # The binding token is the piece's action description, which no
                # command targets and `stat obj` does not print. What must be
                # true here is that it is NOT among the keywords, where anyone
                # could type it. That the binding works is proven below, by who
                # can wear the piece and who cannot.
                require("kingdom-bound-" not in stat,
                        f"the binding token is among the keywords, where commands reach it:\n{stat}")
                material = re.search(r"Material[^\n]*", stat)
                if material:
                    require("steel" in material.group(0).lower(), f"the vambraces are not steel:\n{stat}")
                for effect in ("HASTE", "SANCTUARY", "FIRESHIELD"):
                    require(effect not in stat, f"the vambraces carry {effect}:\n{stat}")

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
                print(f"bought level-56 vambraces: {price}p destroyed, {mineral_bill} mineral + {wood_bill} wood "
                      f"drawn; purse {purse_before}p -> {purse_after}p; treasury unchanged at {treasury_before}p",
                      flush=True)

                # The buyer may wear it.
                command(client, f"wear {PIECE}", "")
                worn = plain(command(client, "equipment", ""))
                require(PIECE_SHORT in worn, f"the buyer could not wear the piece:\n{worn}")
                command(client, f"remove {PIECE}", "")
                wait_for(lambda: plain(command(client, "inventory", "")),
                         lambda text: PIECE_SHORT in text, "the piece never came off")

                # And so may anyone else, whatever they are called: nothing
                # binds store gear. A container is the route a looter or a
                # trade would take.
                command(client, "goto 58449", "")
                command(client, f"load obj {BASKET_VNUM}", "")
                command(client, f"put {PIECE} {BASKET}", "")
                wait_for(lambda: plain(command(client, "inventory", "")),
                         lambda text: PIECE_SHORT not in text,
                         f"the piece never went into the {BASKET}")
                command(client, f"drop {BASKET}", "")
                for account, email, name in OTHERS:
                    other = journey.MudClient(plain_port)
                    others.append(other)
                    create_account(other, account, email)
                    create_character(other, name)
                    command(client, f"transfer {name.lower()}", "")
                    wore_the_piece(other, name)
                    print(f"{name} wore Tyrus's piece", flush=True)
                for other in others:
                    other.close()
                others.clear()

                client.send("quit")
                client.expect_any(("ACCOUNT MENU", "Goodbye", "account menu"), timeout=30)
                client.close()
                client = None
                process.send_signal(signal.SIGTERM)
                process.wait(timeout=60)
                log = output_path.read_text(errors="replace")
                require("FATAL:" not in log and "assert:" not in log, "server logged a fatal or assertion")
                print("[PASS] kingdom works journey: realm, builds and refusals (an unaffordable forge "
                      "among them), a purchase refused for material, harvest, list, buy, level-56 stats, "
                      "flags and resale value, gear two other characters could wear, material "
                      "draw, destroyed platinum", flush=True)
            except Exception as error:
                transcript = bytes(client.transcript).decode("utf-8", errors="replace") if client else ""
                other_tail = ""
                if others:
                    other_text = bytes(others[-1].transcript).decode("utf-8", errors="replace")
                    other_tail = f"\n--- other character's transcript tail ---\n{plain(other_text)[-3000:]}"
                raise AssertionError(f"{error}\n--- transcript tail ---\n{plain(transcript)[-6000:]}"
                                     f"{other_tail}\n--- server output ---\n"
                                     + output_path.read_text(errors="replace")[-6000:]) from error
            finally:
                for other in others:
                    other.close()
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
