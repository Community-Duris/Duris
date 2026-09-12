#!/usr/bin/env python3
"""Create a real CHAOS character and persist its generated class kit."""

from __future__ import annotations

import os
from functools import cache
import pathlib
import re
import signal
import struct
import subprocess
import sys
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
    reconnect_character,
    require,
    runtime_logs,
)


ROOT = pathlib.Path(__file__).resolve().parents[2]
DATA_HEADER = ROOT / "src/account/chaos_eq_data.h"
STARTER_BAG_VNUM = 96443
JOURNEY_CLASSES = ("Warrior", "Monk", "Thief", "Sorcerer", "Dragoon")


@cache
def world_object_text() -> str:
    """Use active AREA inputs so a fresh checkout needs no generated world.obj."""
    sys.path.insert(0, str(ROOT / "scripts"))
    from chaos_eq_analyze import area_file_names
    return "\n".join(path.read_text(errors="replace") for path in
                     area_file_names(ROOT / "areas/obj", ROOT / "areas/AREA"))


def kit_entries(class_name: str) -> list[tuple[int, int]]:
    """Read declared placement from the generated standard class profile."""
    data = DATA_HEADER.read_text(encoding="utf-8")
    array_name = f"chaos_eq_standard_{class_name.lower()}"
    body = re.search(
        rf"static const chaos_kit_item {array_name}\[\] = \{{(.*?)\}};",
        data, re.S,
    )
    require(body is not None, f"missing generated {class_name} profile")
    return [
        (-1 if slot == "WEAR_NONE" else int(slot), int(vnum))
        for slot, vnum in re.findall(r"\{\s*(-?\d+|WEAR_NONE),\s*(\d+)\s*\}", body.group(1))
        if int(vnum)
    ]


def wearable_fixture(class_name: str) -> tuple[int, str, str]:
    """Use body armor to exercise immediate wear with a unique inventory root."""
    vnum = next(vnum for slot, vnum in kit_entries(class_name) if slot == 5)
    objects = world_object_text()
    entry = re.search(rf"(?ms)^#{vnum}$\n(.*?)(?=^#\d+$|^\$~$)", objects)
    require(entry is not None, f"missing wearable prototype {vnum}")
    lines = entry.group(1).splitlines()
    keyword = "chaos_journey_wearable"
    description = re.sub(r"&(?:\+.|.)", "", lines[1].rstrip("~")).lower()
    return vnum, keyword, description


def read_item_ownership(state_root: pathlib.Path) -> dict[int, list[dict[str, int]]]:
    """Read the isolated flatfile ownership catalog grouped by object VNUM."""
    data = (state_root / "domains/item_ownership").read_bytes()
    header_size = 8 + 4 + 4 + 8 + 32
    require(data[:8] == b"DUROWN\0\0", "flatfile item ownership magic changed")
    version, payload_size, revision = struct.unpack_from("<IIQ", data, 8)
    require(version == 3 and revision > 0, "flatfile item ownership header is invalid")
    require(payload_size == len(data) - header_size, "flatfile item ownership size is invalid")
    payload = data[header_size:]
    owner_count, item_count, _ = struct.unpack_from("<III", payload)
    offset = struct.calcsize("<III") + owner_count * struct.calcsize("<BQQQ")
    item_format = "<QQQBQQQiB"
    item_size = struct.calcsize(item_format)
    require(offset + item_count * item_size <= len(payload), "flatfile item ownership rows are truncated")
    by_vnum: dict[int, list[dict[str, int]]] = {}
    for _ in range(item_count):
        (
            item_uid,
            root_item_uid,
            parent_item_uid,
            owner_type,
            owner_id,
            owner_context_id,
            item_revision,
            vnum,
            state,
        ) = struct.unpack_from(item_format, payload, offset)
        offset += item_size
        require(offset + 4 <= len(payload), "flatfile coin payload length is truncated")
        coin_size, = struct.unpack_from("<I", payload, offset)
        offset += 4
        require(offset + coin_size <= len(payload), "flatfile coin payload is truncated")
        offset += coin_size
        by_vnum.setdefault(vnum, []).append(
            {
                "item_uid": item_uid,
                "root_item_uid": root_item_uid,
                "parent_item_uid": parent_item_uid,
                "owner_type": owner_type,
                "owner_id": owner_id,
                "owner_context_id": owner_context_id,
                "item_revision": item_revision,
                "state": state,
            }
        )
    return by_vnum


def one_owned_item(state_root: pathlib.Path, vnum: int) -> dict[str, int]:
    """Return the sole authoritative ownership row for an expected fixture item."""
    matches = read_item_ownership(state_root).get(vnum, [])
    require(len(matches) == 1, f"expected exactly one ownership row for VNUM {vnum}")
    return matches[0]


def expected_utilities(class_name: str) -> dict[int, int]:
    """Independent policy oracle: all chosen classes fish/salvage; only Thief picks/traps."""
    return {336: 1, 400227: 3, 412: int(class_name == "Thief"),
            73: 3 if class_name == "Thief" else 0}


def assert_kit_ownership(state_root: pathlib.Path, class_name: str) -> dict[int, list[dict[str, int]]]:
    """Assert direct wearables and support-only bag contents at grant completion."""
    ownership = read_item_ownership(state_root)
    bag = one_owned_item(state_root, STARTER_BAG_VNUM)
    # Human Sorcerer, standalone Thief and unspecialized Dragoon have no dual wield.
    # Monk profiles omit weapon slots entirely; Warrior has dual wield at level 1.
    equipment = {vnum for slot, vnum in kit_entries(class_name)
                 if slot >= 0 and not (class_name in ("Sorcerer", "Thief", "Dragoon") and slot == 17)}
    if class_name in ("Sorcerer", "Thief", "Dragoon"):
        unavailable = {vnum for slot, vnum in kit_entries(class_name) if slot == 17} - equipment
        require(not any(ownership.get(vnum) for vnum in unavailable),
                f"{class_name} received secondary gear without dual-wield capability")
    for vnum in equipment:
        rows = ownership.get(vnum, [])
        require(bool(rows), f"{class_name}: missing wearable {vnum}")
        for row in rows:
            require(row["root_item_uid"] == row["item_uid"] and row["parent_item_uid"] == 0
                    and row["owner_type"] == 1 and row["owner_id"] == 1 and row["state"] == 1,
                    f"{class_name}: wearable {vnum} did not arrive as a durable inventory root")
    for vnum, count in expected_utilities(class_name).items():
        rows = ownership.get(vnum, [])
        require(len(rows) == count, f"{class_name}: utility {vnum} expected {count}, found {len(rows)}")
        for row in rows:
            require(row["parent_item_uid"] == bag["item_uid"]
                    and row["root_item_uid"] == bag["item_uid"]
                    and row["owner_type"] == 1 and row["owner_id"] == 1 and row["state"] == 1,
                    f"{class_name}: utility {vnum} was not a durable bag child")
    # The generated profile labels support separately; no equipment declaration
    # may be nested merely because its prototype also supports container use.
    require(not any(row["parent_item_uid"] == bag["item_uid"]
                    for vnum in equipment for row in ownership.get(vnum, [])),
            f"{class_name}: starter bag contains profile equipment")
    if class_name == "Monk":
        world_objects = world_object_text()
        for vnum, rows in ownership.items():
            if not any(row["owner_type"] == 1 and row["owner_id"] == 1 for row in rows):
                continue
            match = re.search(rf"(?ms)^#{vnum}$\n(.*?)(?=^#\d+$|^\$~$)", world_objects)
            require(match is not None, f"cannot inspect Monk prototype {vnum}")
            fields = match.group(1).split("~", 4)[4].split()
            require(int(fields[0]) not in (5, 6, 7) and not int(fields[2]) & (1 << 13),
                    f"Monk was granted weapon-bearing prototype {vnum}")
    return ownership


def assert_utility_durability(state_root: pathlib.Path, class_name: str,
                             original: dict[int, list[dict[str, int]]]) -> None:
    """Relog must retain each exact utility UID and quantity, without replay grants."""
    reloaded = read_item_ownership(state_root)
    for vnum, count in expected_utilities(class_name).items():
        before = original.get(vnum, [])
        after = reloaded.get(vnum, [])
        require(len(after) == count and {row["item_uid"] for row in before}
                == {row["item_uid"] for row in after},
                f"{class_name}: utility {vnum} disappeared or duplicated on restart")
        require(all(row["parent_item_uid"] == before[0]["parent_item_uid"]
                    and row["root_item_uid"] == before[0]["root_item_uid"]
                    and row["owner_type"] == 1 and row["owner_id"] == 1
                    and row["state"] == 1 for row in after),
                f"{class_name}: utility {vnum} ownership changed on restart")


def build_snapshot_inspector(run_root: pathlib.Path) -> pathlib.Path:
    """Compile a read-only inspector using the production snapshot file decoder."""
    source, binary = run_root / "kit_snapshot.cpp", run_root / "kit_snapshot"
    source.write_text(r'''
#include "flatfile/flatfile_player_snapshot_file.h"
#include "core/defines.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
int main(int argc, char **argv) {
    assert(argc == 3);
    player_snapshot snapshot;
    std::string error;
    if (flatfile_player_snapshot_read(argv[1], 1, &snapshot, &error) != flatfile_player_load_result::ok) {
        std::cerr << "synthetic kit snapshot decode failed: " << error << '\n';
        return 1;
    }
    const int globe_vnum = std::atoi(argv[2]);
    bool globe = globe_vnum == 0;
    assert(!snapshot.items.empty());
    for (const auto &item : snapshot.items) {
        if (item.extra_flags & (ITEM_TRANSIENT | ITEM_NODROP | ITEM_INVISIBLE | ITEM_SECRET |
                               ITEM_NOSHOW | ITEM_BURIED | ITEM_NORENT)) {
            std::cerr << "unsafe persisted starter flags vnum=" << item.vnum << '\n';
            return 1;
        }
        assert(!(item.extra2_flags & ITEM2_CRUMBLELOOT));
        for (const auto &affect : item.affects) assert(affect[0] != APPLY_CURSE);
        if (item.vnum == globe_vnum && (item.bitvectors[1] & AFF2_GLOBE)) globe = true;
        for (const auto &description : item.extra_descriptions) {
            if (!description.spellbook) continue;
            assert(description.keyword == "SPELLBOOK" && description.description.empty());
            auto spell_ids = description.spell_ids;
            std::sort(spell_ids.begin(), spell_ids.end());
            assert(std::adjacent_find(spell_ids.begin(), spell_ids.end()) == spell_ids.end());
            std::cout << "SPELLBOOK " << item.object_uid << ':';
            for (int spell : spell_ids) std::cout << ' ' << spell;
            std::cout << '\n';
        }
    }
    assert(globe);
    std::cout << "persisted kit flags, curse removal and globe passed\n";
}
''')
    subprocess.run(["g++", "-std=c++20", "-Isrc", str(source),
                    "src/flatfile/flatfile_player_snapshot_file.c", "src/flatfile/flatfile_store.c",
                    "src/player/player_snapshot_codec.c", "-lcrypto", "-o", str(binary)],
                   cwd=ROOT, check=True)
    return binary


def assert_saved_kit(inspector: pathlib.Path, state_root: pathlib.Path, class_name: str) -> tuple[str, ...]:
    """Read freshly saved instances, never prototype flags or reconstructed values."""
    globe_vnum = next(vnum for slot, vnum in kit_entries(class_name) if slot == 3)
    result = subprocess.run([str(inspector), str(state_root),
                             str(globe_vnum if class_name != "Sorcerer" else 0)],
                            check=True, capture_output=True, text=True)
    books = tuple(sorted(line for line in result.stdout.splitlines() if line.startswith("SPELLBOOK ")))
    if class_name == "Sorcerer":
        require(len(books) == 1 and len(books[0].split(":", 1)[1].split()) > 0,
                "Sorcerer snapshot lost its populated native spellbook")
    print("persisted kit flags, curse removal, globe and native spellbooks passed", flush=True)
    return books


def class_kit_vnums(class_name: str) -> set[int]:
    """Return the class profile plus shared support fixture prototypes."""
    data = DATA_HEADER.read_text(encoding="utf-8", errors="replace")
    values: set[int] = {96443}
    for array_name in (f"chaos_eq_standard_{class_name.lower()}", "chaos_eq_standard_optional_slots", "chaos_eq_support_consumables"):
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
    values.add(400300)
    values.add(400000)
    values.add(400001)
    values.add(400291)
    values.add(18000)
    values.update((336, 412, 73, 400227))
    require(1252 not in values, "placeholder VNUM remains in the runtime class kit")
    return values


def install_chaos_objects(run_root: pathlib.Path, class_name: str) -> None:
    """Install the selected Chaos kit into a disposable minimal world."""
    world_objects = world_object_text()
    mini_path = run_root / "areas_mini/mini.obj"
    mini_objects = mini_path.read_text(errors="replace")
    entries = {
        int(vnum): entry.rstrip() + "\n"
        for entry, vnum in re.findall(
            r"(?ms)(^#(\d+)$\n.*?)(?=^#\d+$|^\$~$)", mini_objects
        )
    }
    entries.update(
        {
            22801: """#22801
chaos_test_ring~
a plain Chaos test ring~
A plain Chaos test ring lies here for the legacy enhancement regression.~
~
11 1 3 0 0 0 0 3 0 0 0
0 0 0 0 0 0 0 0
1 1 100
""",
            22802: """#22802
chaos_test_ring_upgrade~
an improved plain Chaos test ring~
An improved plain Chaos test ring lies here.~
~
11 1 3 0 0 0 0 3 0 0 0
0 0 0 0 0 0 0 0
1 1 100
A
1 1
""",
        }
    )
    for vnum in sorted(class_kit_vnums(class_name)):
        if vnum in entries:
            continue
        entry = re.search(
            rf"(?ms)^#{vnum}$\n.*?(?=^#\d+$|^\$~$)", world_objects
        )
        require(entry is not None, f"world object {vnum} is unavailable")
        entries[vnum] = entry.group(0).rstrip() + "\n"
    # Give the selected test wearable a unique keyword so immediate wear does
    # not depend on ambiguous shared names such as armor, black, or leather.
    wearable_vnum, _, _ = wearable_fixture(class_name)
    wearable_lines = entries[wearable_vnum].splitlines()
    wearable_lines[1] = wearable_lines[1].rstrip("~") + " chaos_journey_wearable~"
    entries[wearable_vnum] = "\n".join(wearable_lines) + "\n"
    require(mini_objects.count("$~") == 1, "minimal object terminator changed")
    mini_path.write_text("".join(entries[vnum] for vnum in sorted(entries)) + "$~\n")


def create_chaos_character(client: MudClient, class_name: str = "Warrior") -> None:
    """Drive synthetic character creation through the selected standard kit."""
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
    client.send(class_name.lower())
    selection, _ = client.expect_any(("Alignment only affects", "Hometown Selection"))
    if selection == "Alignment only affects":
        client.send("g")
        client.expect("Hometown Selection")
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
    prepared = "Your Chaos Equipment has been prepared!!"
    result, output = client.expect_any(
        (prepared, "Your CHAOS equipment kit could not", "Your CHAOS equipment bag could not",
         "The ownership authority could not", "The ownership authority did not commit"),
        timeout=300,
    )
    require(result == prepared, f"{class_name}: starter grant failed immediately: {output}")
    transcript = client.transcript.decode("utf-8", errors="replace")
    require("A free frigate!" in transcript, "Chaos tattoo reward did not advertise a Frigate")
    require(
        "Your CHAOS equipment kit is being prepared" not in client.transcript.decode("utf-8", errors="replace"),
        "Chaos creation still exposed the old blocking preparation message",
    )


def expect_paged(client: MudClient, needle: str) -> None:
    """Advance paged output until the requested text appears or the prompt returns."""
    while True:
        matched, _ = client.expect_any((needle, "[Return to continue", "Pos: standing >"), timeout=30)
        if matched == needle:
            return
        if matched == "[Return to continue":
            client.send("")
            continue
        raise AssertionError(f"{needle!r} was not found before the command prompt")


def finish_paged(client: MudClient) -> None:
    """Advance through the remaining pages until the standing prompt appears."""
    while True:
        matched, _ = client.expect_any(("[Return to continue", "Pos: standing >"), timeout=30)
        if matched == "Pos: standing >":
            return
        client.send("")


def inspect_chaos_material_pouch(client: MudClient, retrieve: bool = True) -> None:
    """Exercise the material pouch and optionally retrieve its test inputs first."""
    if retrieve:
        client.transcript.clear()
        client.send("look in bottomless")
        expect_paged(client, "a compact Chaos craft pouch")
        finish_paged(client)

        client.send("get pouch bag")
        client.expect("You get", timeout=30)
        client.expect("Pos: standing >", timeout=30)

        client.send("chaos pouchseed")
        client.expect("Chaos pouch test materials prepared", timeout=30)
        client.expect("Your starter kit is ready.", timeout=30)
        client.expect("Pos: standing >", timeout=30)
        for _ in range(20):
            client.transcript.clear()
            client.send("inventory")
            inventory = client.expect("Pos: standing >", timeout=30)
            if "small feather" in inventory and "strange green stone" in inventory:
                break
            time.sleep(0.25)
        else:
            raise AssertionError("pouch test materials were not granted within 20 retries")
        client.send("put all pouch")
        client.expect("You record 3 collected materials", timeout=30)
        client.expect("Pos: standing >", timeout=30)
        client.transcript.clear()
        client.send("inventory")
        post_collection_inventory = client.expect("Pos: standing >", timeout=30)
        require(
            "small feather" not in post_collection_inventory
            and "strange green stone" not in post_collection_inventory,
            "put all pouch left collected materials in the player inventory:\n"
            + post_collection_inventory,
        )

        client.transcript.clear()
        client.send("look in pouch")
        client.expect("Inside the compact Chaos craft pouch scoreboard", timeout=30)
        client.expect("Generated /", timeout=30)
        client.expect("0 generated / 1 collected", timeout=30)
        client.expect("400000", timeout=30)
        scoreboard = bytes(client.transcript).decode("utf-8", errors="replace")
        positions = [scoreboard.find(f"[{vnum}]") for vnum in (400000, 400001, 400291)]
        require(
            all(position >= 0 for position in positions) and positions == sorted(positions),
            "pouch scoreboard did not sort collected materials by VNUM:\n" + scoreboard,
        )
        client.expect("Pos: standing >", timeout=30)

        client.transcript.clear()
        client.send("encrust answerer 400291")
        client.expect("The Chaos craft pouch generated", timeout=30)
        client.expect("1 x a strange green stone", timeout=30)
        client.expect("400291", timeout=30)
        client.expect("Pos: standing >", timeout=30)

        client.transcript.clear()
        client.send("look in pouch")
        client.expect("1 generated / 1 collected", timeout=30)
        client.expect("400291", timeout=30)
        client.expect("Pos: standing >", timeout=30)

        client.send("chaos pouchgenerate 30")
        client.expect("The Chaos craft pouch generated", timeout=30)
        client.expect("30 x a small feather", timeout=30)
        client.expect("400000", timeout=30)
        client.expect("Pos: standing >", timeout=30)

        client.send("chaos platinum")
        client.expect("Here's", timeout=30)
        client.expect("Pos: standing >", timeout=30)
        for _ in range(120):
            client.transcript.clear()
            client.send("score")
            score = client.expect("Pos: standing >", timeout=30)
            if "10000 platinum" in score:
                break
            time.sleep(0.25)
        else:
            raise AssertionError("Chaos platinum test credit was not published")
        client.send("enhance chaos_test_ring pouch")
        client.expect("Your enhancement is a success!", timeout=60)
        client.expect("Final item value", timeout=60)
        client.expect("Pos: standing >", timeout=30)
        client.transcript.clear()
        client.send("examine pouch")
        client.expect("Inside the compact Chaos craft pouch", timeout=30)
        client.expect("Material requirements are supplied without consuming the pouch", timeout=30)
        client.expect("Pos: standing >", timeout=30)

    client.transcript.clear()
    client.send("look in pouch")
    client.expect("Inside the compact Chaos craft pouch", timeout=30)
    client.expect("Catalog: salvage", timeout=30)
    client.expect("encrust 400291-400299", timeout=30)
    client.expect("Pos: standing >", timeout=30)

    client.transcript.clear()
    client.send("examine pouch")
    client.expect("Inside the compact Chaos craft pouch", timeout=30)
    client.expect("Material requirements are supplied without consuming the pouch", timeout=30)
    client.expect("Pos: standing >", timeout=30)

    if retrieve:
        client.send("wear pouch")
        client.expect("attach", timeout=30)
        client.expect("Pos: standing >", timeout=30)


def run_chaos_kit_journey(binary: pathlib.Path, class_name: str = "Warrior") -> None:
    """Verify Chaos starter ownership and equipment across an isolated restart."""
    wearable_vnum, wearable_keyword, wearable_description = wearable_fixture(class_name)
    with tempfile.TemporaryDirectory(prefix="duris-chaos-kit-state-") as state_tmp:
        with tempfile.TemporaryDirectory(prefix="duris-chaos-kit-run-") as run_tmp:
            state_root = pathlib.Path(state_tmp)
            run_root = pathlib.Path(run_tmp)
            state_root.chmod(0o700)
            (run_root / "logs/log").mkdir(parents=True)
            (run_root / "logs/log/.gitignore").write_text("*\n!.gitignore\n")
            make_fixture(run_root)
            install_chaos_objects(run_root, class_name)
            inspector = build_snapshot_inspector(run_root)
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
                "CREATION_ALL_CLASSES": "TRUE",
                "CHAOS_TEST_COMMANDS": "TRUE",
                "CHAOS_EQ_PROFILE": "standard",
                "CHAOS_STARTER_BONUSES": "TRUE",
                "CHAOS_STARTER_MATERIALS": "TRUE",
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
                        if "Entering game loop." in boot_output or process.poll() is not None:
                            break
                        time.sleep(0.1)
                    require(
                        "Entering game loop." in boot_output,
                        "isolated CHAOS server did not boot:\n" + boot_output[-8000:],
                    )

                    client = MudClient(plain_port)
                    create_chaos_character(client, class_name)
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
                    client.expect("No, you can't have pony.  Not yours.", timeout=15)
                    client.expect("Pos: standing >", timeout=15)
                    client.transcript.clear()
                    client.send("score")
                    score = client.expect("Pos: standing >", timeout=30)
                    require(
                        "Level: 56" in score,
                        "denied Chaos level command changed the character level:\n" + score,
                    )

                    initial_authority = assert_kit_ownership(state_root, class_name)
                    wearable_before_wear = one_owned_item(state_root, wearable_vnum)
                    client.send(f"wear {wearable_keyword}")
                    client.expect("Pos: standing >", timeout=30)
                    client.send("equipment")
                    equipment = client.expect("Pos: standing >", timeout=30).lower()
                    require(wearable_description in equipment,
                            "direct starter wearable could not be worn immediately:\n" + equipment)

                    # Direct inventory intentionally starts with the whole kit.
                    # Wear it before unpacking support so carried-count limits do
                    # not prevent retrieving the pouch. Keep the individually
                    # tested body item worn through the later save/restart checks.
                    client.send("wear all")
                    client.expect("Pos: standing >", timeout=30)
                    client.send("equipment")
                    equipment = client.expect("Pos: standing >", timeout=30).lower()
                    require(wearable_description in equipment,
                            "wear all displaced the directly worn starter body item:\n" + equipment)

                    client.send("look in bottomless")
                    expect_paged(client, "a compact Chaos craft pouch")
                    finish_paged(client)
                    client.send("get pouch bag")
                    client.expect("You get", timeout=30)
                    client.expect("Pos: standing >", timeout=30)
                    inspect_chaos_material_pouch(client, retrieve=False)
                    # The generated class kit does not always leave the same items in
                    # hand, so read the carried count instead of assuming one.  Every
                    # carried item except the bag itself moves into the bag.
                    client.transcript.clear()
                    client.send("inventory")
                    inventory_before_put = client.expect("Pos: standing >", timeout=30)
                    carried = re.search(
                        r"You are carrying: \((\d+)/", inventory_before_put
                    )
                    require(
                        carried is not None,
                        "inventory did not report a carried count:\n"
                        + inventory_before_put,
                    )
                    expected_put = int(carried.group(1)) - 1
                    require(
                        expected_put >= 1,
                        "expected the Chaos kit to leave at least one item to put away:\n"
                        + inventory_before_put,
                    )
                    client.send("put all bottomless")
                    client.expect(f"You put {expected_put} items", timeout=30)
                    client.expect("Pos: standing >", timeout=30)
                    client.transcript.clear()
                    client.send("equipment")
                    equipment = client.expect("Pos: standing >", timeout=30).lower()
                    require(
                        wearable_description in equipment,
                        "put all bottomless moved worn Chaos equipment:\n" + equipment,
                    )
                    client.send("save")
                    client.expect(f"Save complete for {CHARACTER}.", timeout=120)
                    saved_spellbooks = assert_saved_kit(inspector, state_root, class_name)

                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=120)
                    output.flush()
                    server_output = output_path.read_text(errors="replace")
                    logs = runtime_logs(run_root)
                    epic_grant = re.search(
                        r"CHAOS starter granted ([0-9]+) no-specialization epic skills to pid 1", logs
                    )
                    require(
                        # The legacy standalone Thief has no entries in the
                        # epic reward table; its currency grants are still required.
                        epic_grant is not None
                        and (class_name == "Thief" or int(epic_grant.group(1)) > 0),
                        "Chaos starter granted no eligible epic skills:\\n" + logs,
                    )
                    require(process.returncode == 0, "CHAOS server shutdown failed")
                    require(
                        "Cannot load CHAOS kit item" not in logs
                        and "item creation grant did not commit" not in logs
                        and "Skipping unusable CHAOS kit item" not in logs
                        and "CHAOS starter epic grant could not be queued" not in logs
                        and "CHAOS starter bank grant could not be queued" not in logs
                        and "CHAOS starter material reserve committed" not in logs
                        and "CHAOS starter epic grant committed for pid 1 balance=20000" in logs
                        and "CHAOS starter bank grant committed for pid 1 platinum=1000000" in logs
                        and "CHAOS starter granted " in logs,
                        "CHAOS kit logged an incomplete or unusable grant:\n" + logs,
                    )
                    require("1252" not in server_output + logs, "placeholder VNUM reached the runtime journey")

                    if client is not None:
                        client.close()
                        client = None
                    reload_plain_port, reload_tls_port, reload_websocket_port = available_ports()
                    reload_environment = environment.copy()
                    reload_environment.update(
                        {
                            "DURIS_TLS_PORT": str(reload_tls_port),
                            "DURIS_WEBSOCKET_PORT": str(reload_websocket_port),
                        }
                    )
                    reload_output_path = run_root / "server-reload.out"
                    with reload_output_path.open("w", encoding="utf-8") as reload_output:
                        reload_process = subprocess.Popen(
                            [str(binary), "--minimal", "-s", "-d", str(run_root), str(reload_plain_port)],
                            cwd=run_root,
                            env=reload_environment,
                            text=True,
                            stdout=reload_output,
                            stderr=subprocess.STDOUT,
                        )
                        reload_client = None
                        try:
                            reload_deadline = time.monotonic() + 120
                            reload_boot = ""
                            while time.monotonic() < reload_deadline:
                                reload_output.flush()
                                reload_boot = reload_output_path.read_text(errors="replace")
                                if "Entering game loop." in reload_boot or reload_process.poll() is not None:
                                    break
                                time.sleep(0.1)
                            require(
                                "Entering game loop." in reload_boot,
                                "flat-file reload server did not boot:\n" + reload_boot[-8000:],
                            )
                            reload_client = reconnect_character(reload_plain_port)
                            assert_utility_durability(state_root, class_name, initial_authority)
                            reload_client.send("get pouch bottomless")
                            reload_client.expect("You get", timeout=30)
                            reload_client.expect("Pos: standing >", timeout=30)
                            inspect_chaos_material_pouch(reload_client, retrieve=False)
                            reload_client.transcript.clear()
                            reload_client.send("equipment")
                            reload_equipment = reload_client.expect(
                                "Pos: standing >", timeout=30
                            ).lower()
                            require(
                                wearable_description in reload_equipment,
                                "Chaos equipment did not survive restart in its worn slots:\n"
                                + reload_equipment,
                            )
                            reload_client.transcript.clear()
                            reload_client.send("look in bottomless")
                            finish_paged(reload_client)
                            reload_bag = bytes(reload_client.transcript).decode(
                                "utf-8", errors="replace"
                            ).lower()
                            require(
                                wearable_description not in reload_bag,
                                "worn starter wearable rematerialized in the bag:\n"
                                + reload_bag,
                            )
                            wearable_after_reload = one_owned_item(
                                state_root, wearable_vnum
                            )
                            require(
                                wearable_after_reload["item_uid"]
                                == wearable_before_wear["item_uid"]
                                and wearable_after_reload["root_item_uid"]
                                == wearable_after_reload["item_uid"]
                                and wearable_after_reload["parent_item_uid"] == 0
                                and wearable_after_reload["owner_type"] == 1
                                and wearable_after_reload["owner_id"] == 1
                                and wearable_after_reload["state"] == 1,
                                "reloaded starter wearable authority was not a player root",
                            )
                            reload_client.send("save")
                            reload_client.expect(f"Save complete for {CHARACTER}.", timeout=120)
                            reloaded_spellbooks = assert_saved_kit(inspector, state_root, class_name)
                            require(reloaded_spellbooks == saved_spellbooks,
                                    f"{class_name}: spellbook UIDs or spell IDs changed across reload")
                            reload_client.send("quit")
                            reload_client.expect("ACCOUNT MENU", timeout=60)
                            reload_client.send("0")
                        finally:
                            if reload_client is not None:
                                reload_client.close()
                            if reload_process.poll() is None:
                                reload_process.terminate()
                                try:
                                    reload_process.wait(timeout=120)
                                except subprocess.TimeoutExpired:
                                    reload_process.kill()
                                    reload_process.wait(timeout=5)
                        require(
                            reload_process.returncode == 0,
                            "flat-file reload server shutdown failed",
                        )
                        reload_output.flush()
                        reload_boot = reload_output_path.read_text(errors="replace")
                        require(
                            "limit_exceeded" not in reload_boot
                            and "component_failure" not in reload_boot,
                            "flat-file reload rejected the Chaos material inventory snapshot:\n" + reload_boot[-8000:],
                        )
                        reload_logs = runtime_logs(run_root)
                        require(
                            "player_load_materialize: component=items pid=1 outcome=topology_repaired"
                            not in reload_logs,
                            "correctly moved Chaos equipment triggered topology repair:\n"
                            + reload_logs,
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
    with tempfile.TemporaryDirectory(prefix=f"flatfile-combat-{os.getpid()}-") as build_tmp:
        binary = build_flatfile_server(pathlib.Path(build_tmp))
        selected_classes = sys.argv[1:] or JOURNEY_CLASSES
        require(all(name in JOURNEY_CLASSES for name in selected_classes), "unknown journey class")
        for class_name in selected_classes:
            run_chaos_kit_journey(binary, class_name)
            print(f"flat-file CHAOS {class_name} kit create/wear/save/restart passed")
    print("flat-file CHAOS new-character bag and generated class kit journey passed")
