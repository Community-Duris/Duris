#!/usr/bin/env python3
"""Run a disposable real-server A/B journey for ordered divine refusal."""

from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile
import time

import test_flatfile_combat_journey as journey


STAFF_FIXTURE = r'''
#include "flatfile/flatfile_player_snapshot_file.h"
#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"
#include <cassert>
#include <openssl/sha.h>
template<class T> void number(std::vector<uint8_t>& out, T value) {
    for (size_t i=0; i<sizeof(T); ++i)
        out.push_back(static_cast<uint64_t>(value) >> (i*8));
}
int main(int argc, char **argv) {
    assert(argc == 2);
    player_snapshot snapshot; std::string error;
    assert(flatfile_player_snapshot_read(argv[1], 1, &snapshot, &error) ==
           flatfile_player_load_result::ok);
    for (auto &field : snapshot.status_integers) {
        if (field.field == player_status_field::level ||
            field.field == player_status_field::highest_level)
            field.signed_value = field.unsigned_value = 62;
        if (field.field == player_status_field::base_hit)
            field.signed_value = field.unsigned_value = 200000;
        if (field.field == player_status_field::hit_difference)
            field.signed_value = field.unsigned_value = 0;
    }
    std::vector<uint8_t> payload, bytes;
    snapshot.encoded_size_bound = PLAYER_SNAPSHOT_MAX_BYTES;
    assert(player_snapshot_encode(snapshot, &payload) == player_snapshot_codec_result::ok);
    using namespace flatfile_player_snapshot_file;
    bytes.insert(bytes.end(), player_magic.begin(), player_magic.end());
    number(bytes, player_file_version); number<uint32_t>(bytes, payload.size());
    number(bytes, snapshot.pid); number(bytes, snapshot.revision);
    number(bytes, snapshot.components);
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(payload.data(), payload.size(), digest);
    bytes.insert(bytes.end(), digest, digest + sizeof(digest));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    assert(flatfile_atomic_write(player_directory(argv[1]), player_filename(1), bytes,
                                 &error));
}
'''


CLERIC_MOBS = """#22801
divine healer cleric~
a bound healer~
A bound healer waits here to test ordered divine magic.
~
~
10 0 0 0 0 0 0 0 S
PH 0 32 -1
50 0 0 200d1+2000 1d1+1
0.0.0.0 0
8 8 0
#22802
divine striker cleric~
a bound striker~
A bound striker waits here to test ordered attacks.
~
~
10 0 0 0 0 0 0 0 S
PH 0 32 -1
50 0 0 200d1+2000 1d1+1
0.0.0.0 0
8 8 0
#22803
divine witness warrior~
a bound witness~
A bound witness waits here to test mixed follower orders.
~
~
10 0 0 0 0 0 0 0 S
PH 0 1 -1
50 0 0 200d1+2000 1d1+1
0.0.0.0 0
8 8 0
"""


def drain(client: journey.MudClient, duration: float = 0.6) -> str:
    deadline = time.monotonic() + duration
    while time.monotonic() < deadline:
        client._receive()
        time.sleep(0.02)
    output = client.pending.decode("utf-8", errors="replace")
    client.pending.clear()
    return output


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def configure_fixture(runtime: Path, enabled: bool) -> None:
    journey.make_fixture(runtime)
    journey.generate_certificate(runtime)

    properties_path = runtime / "lib/duris.properties"
    properties = properties_path.read_text(encoding="utf-8")
    expected = "pets.divine_refusal.enabled=0"
    require(properties.count(expected) == 1, "divine-refusal master setting changed")
    properties = properties.replace(
        expected, f"pets.divine_refusal.enabled={1 if enabled else 0}"
    )
    properties = properties.replace(
        "pets.divine_refusal.percent=10", "pets.divine_refusal.percent=100"
    )
    properties = properties.replace(
        "pets.divine_refusal.retry_lock_seconds=4",
        "pets.divine_refusal.retry_lock_seconds=8",
    )
    properties_path.write_text(properties, encoding="utf-8")

    mobile_path = runtime / "areas_mini/mini.mob"
    mobiles = mobile_path.read_text(encoding="utf-8")
    require(mobiles.count("$~") == 1, "minimal mobile terminator changed")
    mobile_path.write_text(
        mobiles.replace("$~", CLERIC_MOBS + "$~"), encoding="utf-8"
    )


def elevate_player(root: Path, state: Path) -> None:
    source = root / "staff.cpp"
    binary = root / "staff"
    source.write_text(STAFF_FIXTURE, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Isrc",
            str(source),
            "src/player/player_snapshot_codec.c",
            "src/flatfile/flatfile_player_snapshot_file.c",
            "src/flatfile/flatfile_store.c",
            "-lcrypto",
            "-o",
            str(binary),
        ],
        cwd=journey.ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(state)], check=True)


def run_scenario(server_binary: Path, enabled: bool) -> None:
    label = "enabled-100-percent" if enabled else "disabled"
    print(f"divine refusal journey: {label}", flush=True)
    with tempfile.TemporaryDirectory(prefix=f"divine-refusal-{label}-") as temporary:
        root = Path(temporary)
        state = root / "state"
        runtime = root / "runtime"
        state.mkdir(mode=0o700)
        (state / "domains").mkdir(mode=0o700)
        runtime.mkdir()
        (runtime / "logs/log").mkdir(parents=True)
        configure_fixture(runtime, enabled)
        for name in ("players", "critical"):
            (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
        (runtime / "bin/server").mkdir(parents=True)
        for name in ("dms", "dms_new"):
            shutil.copy2(server_binary, runtime / "bin/server" / name)
        subprocess.run(
            [str(journey.INSPECTOR), str(state), "seed-combat"], check=True
        )

        port, tls_port, websocket_port = journey.available_ports()
        environment = dict(
            PATH=os.environ.get("PATH", "/usr/bin:/bin"),
            ENVIRONMENT="local",
            PERSISTENCE_MODE="flatfile-primary",
            FLATFILE_STATE_DIR=str(state),
            PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
            CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
            REDIS="FALSE",
            CHAOS_MUD="FALSE",
            LISTEN_ADDRESS="127.0.0.1",
            DURIS_TLS_PORT=str(tls_port),
            DURIS_WEBSOCKET_PORT=str(websocket_port),
            DURIS_WEBSOCKET_LISTEN_ADDRESS="127.0.0.1",
        )
        process = None
        output = None
        client = None

        def stop() -> None:
            nonlocal process, output, client
            if client:
                client.close()
                client = None
            if process and process.poll() is None:
                process.terminate()
                process.wait(timeout=30)
            process = None
            if output:
                output.close()
                output = None

        def boot() -> None:
            nonlocal process, output
            output = (runtime / "server.out").open("w", encoding="utf-8")
            process = subprocess.Popen(
                [str(runtime / "bin/server/dms"), "--minimal", "-s", str(port)],
                cwd=runtime,
                env=environment,
                stdout=output,
                stderr=subprocess.STDOUT,
            )
            deadline = time.monotonic() + 90
            while "Entering game loop." not in (runtime / "server.out").read_text(
                encoding="utf-8", errors="replace"
            ):
                require(process.poll() is None, "server exited during boot")
                require(time.monotonic() < deadline, "server boot timed out")
                time.sleep(0.1)

        try:
            boot()
            client = journey.MudClient(port)
            journey.create_character(client, class_name="3")
            client.send("save")
            client.expect("Save complete for Taverek.")
            client.send("quit")
            client.expect("ACCOUNT MENU", timeout=30)
            stop()
            elevate_player(root, state)

            boot()
            client = journey.reconnect_character(port)
            drain(client)
            for vnum in (22801, 22802, 22803):
                client.send(f"givepet Taverek {vnum}")
                response = drain(client)
                require("not loadable" not in response.lower(), response)
            client.send("look")
            room = client.expect("Pos: standing >")
            for description in ("A bound healer", "A bound striker", "A bound witness"):
                require(description in room, f"missing fixture pet {description}:\n{room}")

            if not enabled:
                client.send("order healer cast 'heal' Taverek")
                heal = client.expect("Ok.", timeout=15) + drain(client, 1.0)
                require("My deity has warned me" not in heal, heal)
                client.send("order striker kill raoul")
                attack = client.expect("Ok.", timeout=15) + drain(client, 1.0)
                require("My deity has warned me" not in attack, attack)
                print(
                    "PASS disabled: ordered heal and attack accepted (2/2), no refusal",
                    flush=True,
                )
            else:
                client.send("order healer frobnicate")
                unknown = client.expect("Ok.", timeout=15) + drain(client)
                require("My deity has warned me" not in unknown, unknown)

                client.send("order healer abort")
                exempt = client.expect("Ok.", timeout=15) + drain(client)
                require("My deity has warned me" not in exempt, exempt)

                client.send("order healer cast 'heal' Taverek")
                heal = client.expect("My deity has warned me", timeout=15)
                heal += drain(client)
                require("Ok." not in heal, heal)

                # The command is sent immediately. The master's ordinary combat-round
                # wait delays dispatch, while the eight-second test lock keeps this
                # execution inside the same refusal window.
                client.send("order healer kill raoul")
                retry = client.expect("still refusing that order", timeout=15)
                retry += drain(client)
                require("My deity has warned me" not in retry, retry)
                require("Ok." not in retry, retry)

                client.send("order striker ki raoul")
                attack = client.expect("My deity has warned me", timeout=15)
                attack += drain(client)
                require("Ok." not in attack, attack)

                client.send("order followers say mixed-accepted")
                mixed = client.expect("mixed-accepted", timeout=20)
                mixed += drain(client)
                require("Ok." in mixed, mixed)
                require("None here are loyal" not in mixed, mixed)
                print(
                    "PASS enabled: ordered heal and attack refused (0/2 cleric "
                    "dispatches), retry stayed locked, mixed non-cleric acted",
                    flush=True,
                )
        except Exception:
            print((runtime / "server.out").read_text(errors="replace")[-8000:])
            print(journey.runtime_logs(runtime)[-12000:])
            if client:
                print(client.transcript.decode(errors="replace")[-10000:])
            raise
        finally:
            stop()


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: run_divine_refusal_journey.py /path/to/flatfile/dms", file=sys.stderr)
        return 2
    binary = Path(sys.argv[1]).resolve()
    require(binary.is_file(), f"server binary does not exist: {binary}")
    journey.INSPECTOR.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            "python3",
            "tests/async/test_flatfile_player_repository.py",
            "--build-inspector",
            str(journey.INSPECTOR),
        ],
        cwd=journey.ROOT,
        check=True,
        timeout=180,
    )
    run_scenario(binary, False)
    run_scenario(binary, True)
    print(
        "Divine refusal disposable A/B complete; forced boundaries are not a DPS estimate.",
        flush=True,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
