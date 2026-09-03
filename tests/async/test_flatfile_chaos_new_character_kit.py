#!/usr/bin/env python3
"""Create a real CHAOS character and persist its generated class kit."""

from __future__ import annotations

import os
import pathlib
import re
import signal
import struct
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
    reconnect_character,
    require,
    runtime_logs,
)


ROOT = pathlib.Path(__file__).resolve().parents[2]
DATA_HEADER = ROOT / "src/account/chaos_eq_data.h"
TRANSIENT_RING_VNUM = 88317
STARTER_BAG_VNUM = 96443


def read_item_ownership(state_root: pathlib.Path) -> dict[int, list[dict[str, int]]]:
    """Read the isolated flatfile ownership catalog grouped by object VNUM."""
    data = (state_root / "domains/item_ownership").read_bytes()
    header_size = 8 + 4 + 4 + 8 + 32
    require(data[:8] == b"DUROWN\0\0", "flatfile item ownership magic changed")
    version, payload_size, revision = struct.unpack_from("<IIQ", data, 8)
    require(version == 2 and revision > 0, "flatfile item ownership header is invalid")
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
    values.add(400300)
    values.add(400000)
    values.add(400001)
    values.add(400291)
    values.add(18000)
    require(1252 not in values, "placeholder VNUM remains in the runtime Warrior kit")
    return values


def install_chaos_objects(run_root: pathlib.Path) -> None:
    """Install the Warrior Chaos kit objects into the isolated minimal world."""
    world_objects = (ROOT / "areas/world.obj").read_text(errors="replace")
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
    """Drive character creation through a standard Warrior Chaos starter grant."""
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
    client.expect("Your Chaos Equipment has been prepared!!", timeout=300)
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


def run_chaos_kit_journey(binary: pathlib.Path) -> None:
    """Verify Chaos starter ownership and equipment across an isolated restart."""
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
                    client.expect("No, you can't have pony.  Not yours.", timeout=15)
                    client.expect("Pos: standing >", timeout=15)
                    client.transcript.clear()
                    client.send("score")
                    score = client.expect("Pos: standing >", timeout=30)
                    require(
                        "Level: 56" in score,
                        "denied Chaos level command changed the character level:\n" + score,
                    )

                    bag_authority = one_owned_item(state_root, STARTER_BAG_VNUM)
                    ring_before_get = one_owned_item(state_root, TRANSIENT_RING_VNUM)
                    require(
                        ring_before_get["root_item_uid"] == bag_authority["item_uid"]
                        and ring_before_get["parent_item_uid"] == bag_authority["item_uid"]
                        and ring_before_get["owner_type"] == 1
                        and ring_before_get["owner_id"] == 1
                        and ring_before_get["state"] == 1,
                        "transient starter ring was not granted as a durable bag child",
                    )

                    client.send("get liquid bottomless")
                    client.expect("You get", timeout=30)
                    client.expect("Pos: standing >", timeout=30)
                    ring_after_get = one_owned_item(state_root, TRANSIENT_RING_VNUM)
                    require(
                        ring_after_get["item_uid"] == ring_before_get["item_uid"]
                        and ring_after_get["root_item_uid"] == ring_after_get["item_uid"]
                        and ring_after_get["parent_item_uid"] == 0
                        and ring_after_get["owner_type"] == 1
                        and ring_after_get["owner_id"] == 1
                        and ring_after_get["state"] == 1
                        and ring_after_get["item_revision"]
                        > ring_before_get["item_revision"],
                        "transient starter ring authority did not move from the bag to a player root",
                    )

                    client.send("wear liquid")
                    client.expect("ring finger", timeout=30)
                    client.expect("Pos: standing >", timeout=30)

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
                        "ring of liquid rock" in equipment,
                        "put all bottomless moved worn Chaos equipment:\n" + equipment,
                    )
                    client.send("save")
                    client.expect(f"Save complete for {CHARACTER}.", timeout=120)

                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=120)
                    output.flush()
                    server_output = output_path.read_text(errors="replace")
                    logs = runtime_logs(run_root)
                    epic_grant = re.search(
                        r"CHAOS starter granted ([0-9]+) no-specialization epic skills to pid 1", logs
                    )
                    require(
                        epic_grant is not None and int(epic_grant.group(1)) > 0,
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
                                "ring of liquid rock" in reload_equipment,
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
                                "ring of liquid rock" not in reload_bag,
                                "worn transient starter ring rematerialized in the bag:\n"
                                + reload_bag,
                            )
                            ring_after_reload = one_owned_item(
                                state_root, TRANSIENT_RING_VNUM
                            )
                            require(
                                ring_after_reload["item_uid"]
                                == ring_after_get["item_uid"]
                                and ring_after_reload["root_item_uid"]
                                == ring_after_reload["item_uid"]
                                and ring_after_reload["parent_item_uid"] == 0
                                and ring_after_reload["owner_type"] == 1
                                and ring_after_reload["owner_id"] == 1
                                and ring_after_reload["state"] == 1,
                                "reloaded transient starter ring authority was not a player root",
                            )
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
        run_chaos_kit_journey(build_flatfile_server(pathlib.Path(build_tmp)))
    print("flat-file CHAOS new-character bag and generated class kit journey passed")
