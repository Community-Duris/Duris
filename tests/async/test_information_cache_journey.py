#!/usr/bin/env python3
"""Exercise information browsing with two real players and automatic refresh.

The default uses isolated flat-file state. --backend mariadb requires explicit
TEST_DB_HOST/USER/PASSWORD for a disposable loopback database server and creates
and drops only a uniquely named fixture schema. Never reads the checkout .env.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager
import json
import os
from pathlib import Path
import re
import signal
import statistics
import subprocess
import tempfile
import threading
import time
import uuid

import test_flatfile_combat_journey as journey

ROOT = journey.ROOT
PAGES = ("credits", "faq", "wizlist")
PROMPT = "Pos: standing >"
PAGER = "[Return to continue, (q)uit, (r)efresh, (b)ack, or page number"


def page_content(name, version):
    lines = 96 if name == "credits" and version == "v1" else 1
    return "".join(f"INFO-{name}-{version} line {line:03d}\n" for line in range(lines))


@contextmanager
def authority(backend, runtime):
    environment = {
        "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
        "ENVIRONMENT": "local", "REDIS": "FALSE", "CHAOS_MUD": "FALSE",
        "LISTEN_ADDRESS": "127.0.0.1", "DURIS_WEBSOCKET_LISTEN_ADDRESS": "127.0.0.1",
    }
    if "LD_LIBRARY_PATH" in os.environ:
        environment["LD_LIBRARY_PATH"] = os.environ["LD_LIBRARY_PATH"]
    if backend == "flatfile":
        state = runtime / "state"
        state.mkdir(mode=0o700)
        (state / "domains").mkdir(mode=0o700)
        subprocess.run([str(journey.INSPECTOR), str(state), "seed-combat"], check=True)
        environment.update(PERSISTENCE_MODE="flatfile-primary", FLATFILE_STATE_DIR=str(state))

        def publish(version):
            directory = runtime / "lib/information"
            directory.mkdir(exist_ok=True)
            for name in PAGES:
                temporary = directory / (name + ".new")
                temporary.write_text(page_content(name, version))
                temporary.replace(directory / name)

        yield environment, publish
        return

    host = os.environ["TEST_DB_HOST"]
    journey.require(host in ("127.0.0.1", "localhost"), "use a disposable loopback database")
    database = "information_journey_test_" + uuid.uuid4().hex[:12]
    environment.update(
        PERSISTENCE_MODE="mariadb-primary", DB_TLS="FALSE", DB_HOST=host, DB_PORT="3306",
        DB_USER=os.environ["TEST_DB_USER"], DB_PASSWD=os.environ["TEST_DB_PASSWORD"],
        DB_NAME=database, DB_ALLOWED_TARGETS=host + "/" + database,
        MYSQL_PWD=os.environ["TEST_DB_PASSWORD"],
    )
    mysql = ["mysql", "--protocol=tcp", "-h", host, "-u", environment["DB_USER"], "-N", "-B"]

    def sql(statement, selected=True):
        return subprocess.check_output(mysql + ([database] if selected else []),
                                       input=statement, text=True, env=environment)

    def publish(version):
        rows = ",".join(f"('{name}',CONVERT(0x{page_content(name, version).encode().hex()} USING utf8mb4))"
                        for name in PAGES)
        sql("START TRANSACTION; INSERT INTO mud_info(name,content) VALUES " + rows +
            " ON DUPLICATE KEY UPDATE content=VALUES(content); COMMIT;")

    sql("CREATE DATABASE " + database + " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci", False)
    try:
        sql((ROOT / "migrations/bootstrap_multithread_safe.sql").read_text())
        for action in (("adopt", "--kind", "fresh_bootstrap"), ("run",)):
            subprocess.run(["python3", "scripts/migration_runner.py", *action],
                           cwd=ROOT, env=environment, check=True)
        yield environment, publish
    finally:
        sql("DROP DATABASE " + database, False)


def response(client, command, marker, timeout=5):
    started = time.monotonic()
    client.send(command)
    # Consume the command-specific output first; a leftover prompt cannot
    # satisfy this measurement before the requested command actually executes.
    text = client.expect(marker, timeout=timeout)
    text += client.expect(PROMPT, timeout=timeout)
    journey.require("temporarily unavailable" not in text, "cache unexpectedly unavailable")
    return time.monotonic() - started


def exercise(reader, observer, publish):
    samples = []
    stop = threading.Event()
    started = threading.Event()
    refresh_requested = threading.Event()
    refreshed = threading.Event()

    def observe():
        while not stop.is_set():
            samples.append(response(observer, "score", "Score information for"))
            started.set()
            if refresh_requested.is_set() and not refreshed.is_set():
                observer.send("faq")
                marker, _ = observer.expect_any(("INFO-faq-v1", "INFO-faq-v2"), timeout=5)
                observer.expect(PROMPT, timeout=5)
                if marker == "INFO-faq-v2":
                    refreshed.set()

    groups = []
    with ThreadPoolExecutor(max_workers=1) as pool:
        future = pool.submit(observe)
        try:
            journey.require(started.wait(10), "observer did not answer its first score")
            for _ in range(12):
                began = time.monotonic()
                for name in PAGES:
                    response(reader, name, f"INFO-{name}-v1")
                groups.append(time.monotonic() - began)
            journey.require(max(groups) < 2, "ordinary three-page navigation took two seconds")
            # Hold the real pager open while the cache's backing strings are
            # replaced. The second player observes publication independently.
            reader.send("more credits")
            reader.expect("INFO-credits-v1")
            reader.expect(PAGER)
            reader.expect("]")
            # Leave the second player active while the real periodic worker
            # replaces all three pages. No test-only refresh hook is involved.
            publish("v2")
            refreshed_at = time.monotonic()
            refresh_requested.set()
            journey.require(refreshed.wait(70), "automatic refresh did not publish")
            refresh_seconds = time.monotonic() - refreshed_at
            reader.send("")
            old_page = reader.expect("INFO-credits-v1") + reader.expect(PAGER)
            reader.expect("]")
            journey.require("INFO-credits-v2" not in old_page, "active pager changed generations")
            reader.send("q")
            reader.expect(PROMPT)
            for name in PAGES:
                response(reader, name, f"INFO-{name}-v2")
        finally:
            stop.set()
            future.result(timeout=10)
    journey.require(len(samples) >= 12, "insufficient concurrent observer samples")
    journey.require(max(samples) < 2, "information browsing stalled the observer for two seconds")
    return {
        "navigation_groups": len(groups), "information_commands_before_refresh": 36,
        "three_page_group_max_seconds": round(max(groups), 3),
        "observer_score_samples": len(samples),
        "observer_score_median_seconds": round(statistics.median(samples), 3),
        "observer_score_max_seconds": round(max(samples), 3),
        "automatic_refresh_seconds": round(refresh_seconds, 3),
        "active_pager_retained_old_generation": True,
    }


def run(binary, backend):
    with tempfile.TemporaryDirectory(prefix="information-journey-") as temporary:
        runtime = Path(temporary)
        journey.make_fixture(runtime)
        # Keep both test players in a quiet room, away from combat/scavenging.
        zone = runtime / "areas_mini/mini.zon"
        zone.write_text(re.sub(r"^[MG] .*\n", "", zone.read_text(), flags=re.M))
        journey.generate_certificate(runtime)
        (runtime / "logs/log").mkdir(parents=True)
        for name in ("players", "critical"):
            (runtime / "journals" / name).mkdir(parents=True, mode=0o700)
        with authority(backend, runtime) as (environment, publish):
            publish("v1")
            plain, tls, websocket = journey.available_ports()
            environment.update(
                PLAYER_SAVE_JOURNAL_DIR=str(runtime / "journals/players"),
                CRITICAL_COMMAND_JOURNAL_DIR=str(runtime / "journals/critical"),
                DURIS_TLS_PORT=str(tls), DURIS_WEBSOCKET_PORT=str(websocket),
            )
            output_path = runtime / "server.out"
            clients = []
            with output_path.open("w") as output:
                process = subprocess.Popen(
                    [str(binary), "--minimal", "-s", "-d", str(runtime), str(plain)],
                    cwd=runtime, env=environment, stdout=output, stderr=subprocess.STDOUT)
                try:
                    deadline = time.monotonic() + 120
                    while "Entering game loop." not in output_path.read_text(errors="replace"):
                        journey.require(process.poll() is None and time.monotonic() < deadline,
                                        "information journey failed to boot")
                        time.sleep(0.1)
                    for index, (account, character) in enumerate(
                            (("Infoacct", "Inforen"), ("Watchacct", "Watchren")), start=1):
                        # Distinct loopback clients exercise normal mortal login
                        # without altering the production multiplay policy.
                        client = journey.MudClient(plain, source_host=f"127.0.0.{index}")
                        clients.append(client)
                        journey.create_character(client, account=account, character=character,
                                                 email=account.lower() + "@example.invalid")
                        client.expect(PROMPT, timeout=30)
                        response(client, "toggle paging no", "mode off.")
                    result = exercise(*clients, publish)
                    process.send_signal(signal.SIGTERM)
                    process.wait(timeout=30)
                    journey.require(process.returncode == 0, "information server shutdown failed")
                    print(json.dumps({"backend": backend, **result}, sort_keys=True), flush=True)
                except Exception as error:
                    raise AssertionError(str(error) + "\n" + output_path.read_text(errors="replace")[-8000:]
                                         + "\n" + journey.runtime_logs(runtime)) from error
                finally:
                    for client in clients:
                        client.close()
                    if process.poll() is None:
                        process.terminate()
                        try:
                            process.wait(timeout=10)
                        except subprocess.TimeoutExpired:
                            process.kill()
                            process.wait(timeout=10)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend", choices=("flatfile", "mariadb"), default="flatfile")
    parser.add_argument("--server", type=Path)
    args = parser.parse_args()
    (ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
    if args.backend == "flatfile":
        subprocess.run(["python3", "tests/async/test_flatfile_player_repository.py",
                        "--build-inspector", str(journey.INSPECTOR)], cwd=ROOT, check=True, timeout=180)
    elif not all(os.environ.get(name) for name in ("TEST_DB_HOST", "TEST_DB_USER", "TEST_DB_PASSWORD")):
        parser.error("MariaDB requires explicit disposable TEST_DB_HOST, TEST_DB_USER and TEST_DB_PASSWORD")
    with tempfile.TemporaryDirectory(prefix="information-build-", dir=ROOT / "bin/tests") as build:
        if args.server:
            binary = args.server.resolve()
        elif args.backend == "flatfile":
            binary = journey.build_flatfile_server(Path(build))
        else:
            binary = Path(build) / "dms_information_mysql"
            subprocess.run(["make", "-C", "src", "-j2", "PERSISTENCE_BACKEND=mariadb",
                            "DMS_BINARY=" + str(binary)], cwd=ROOT, check=True)
        run(binary, args.backend)
