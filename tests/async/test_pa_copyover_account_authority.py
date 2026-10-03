#!/usr/bin/env python3
"""Real SQL copyover regression for authoritative account restoration.

Run this only in the centralized disposable SQL/game test batch. The caller must
supply a trusted parent-frozen build descriptor and its independently expected
source SHA. The shared loader validates descriptor consistency and binary bytes;
this test never selects a checkout or old binary automatically.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import signal
import subprocess
import tempfile
import time

import pa_copyover_fixture as fixture
import test_pa_copyover_sql as frozen_artifacts

journey = fixture.journey


def parent_artifact(descriptor_arg: Path, provenance_sha: str) -> tuple[Path, str]:
    binary, digest, descriptor = frozen_artifacts.load_frozen_binary(
        descriptor_path=descriptor_arg,
        expected_source_sha=provenance_sha,
    )
    fixture.record("parent-binary-provenance", binary=str(binary),
                   binary_sha256=digest, provenance_sha=descriptor["head"],
                   descriptor=str(descriptor_arg.resolve()),
                   descriptor_sha256=fixture.sha256_file(descriptor_arg))
    return binary, digest


def run(binary: Path, digest: str, provenance_sha: str) -> None:
    fixture.require(journey.ACCOUNT.casefold() != journey.CHARACTER.casefold(),
                    "regression requires distinct account and character identities")
    fixture.record("account-regression-source", fixture_sha256=fixture.sha256_file(
        Path(fixture.__file__)), regression_sha256=fixture.sha256_file(Path(__file__)),
        provenance_sha=provenance_sha)
    db = fixture.DisposableMariaDB()
    with tempfile.TemporaryDirectory(prefix="duris-pa-copyover-account-") as temporary:
        runtime = Path(temporary) / "runtime"
        process = client = output = None
        phase = "db-start"
        try:
            db.start()
            db.prepare_schema()
            runtime.mkdir()
            port, env = fixture.make_runtime(runtime, binary, db)
            output_path = runtime / "server.out"
            output = output_path.open("w")
            phase = "initial-boot"
            process = subprocess.Popen(
                [str(runtime / "bin/server/dms"), "--minimal", "-s", "-d",
                 str(runtime), str(port)],
                cwd=runtime, env=env, stdout=output, stderr=subprocess.STDOUT)
            deadline = time.monotonic() + 120
            while "Entering game loop." not in output_path.read_text(errors="replace"):
                fixture.require(process.poll() is None and time.monotonic() < deadline,
                                "account regression game failed to boot")
                time.sleep(0.1)
            fixture.require(fixture.sha256_file(Path(f"/proc/{process.pid}/exe")) == digest,
                            "account regression is not using the parent-supplied binary")
            before_owners = db.owner_sessions()

            def save():
                assert client is not None
                client.send("save")
                client.expect(f"Save complete for {journey.CHARACTER}.", timeout=45)

            def account_menu():
                assert client is not None
                start = len(client.transcript)
                client.send("quit")
                client.expect("ACCOUNT MENU", timeout=35)
                text = client.transcript[start:].decode(errors="replace")
                fixture.require(f"{journey.ACCOUNT}'s ACCOUNT MENU" in text,
                                "copyover returned a different account's menu: " + text[-3000:])

            phase = "baseline-account-return"
            client = journey.MudClient(port)
            journey.create_character(client)
            save()
            pid = int(db.sql("SELECT pid FROM player_data WHERE name='" + journey.CHARACTER +
                             "' AND account_name='" + journey.ACCOUNT + "'"))
            baseline = fixture.character_state(db, pid)
            fixture.require(db.sql("SELECT COUNT(*) FROM accounts WHERE account_name='" +
                                   journey.CHARACTER + "'") == "0",
                            "fixture unexpectedly has an account named after the character")
            account_menu()
            client.send("0")
            client.close()
            client = journey.reconnect_character(port)
            fixture.record(phase, status="PASS", account=journey.ACCOUNT,
                           character=journey.CHARACTER, pid=pid)

            phase = "account-copyover"
            socket_identity = (client.socket.fileno(), client.socket.getsockname(),
                               client.socket.getpeername())
            process.send_signal(signal.SIGUSR1)
            client.expect("Copyover complete!", timeout=120)
            after_owners = db.owner_sessions(previous=before_owners)
            fixture.require(not db.old_sessions(before_owners), "old SQL authority remained")
            fixture.require(socket_identity == (client.socket.fileno(), client.socket.getsockname(),
                                                client.socket.getpeername()),
                            "copyover replaced the gameplay socket")
            fixture.require(fixture.sha256_file(Path(f"/proc/{process.pid}/exe")) == digest,
                            "post-copyover executable differs from parent-supplied binary")
            save()
            fixture.require(fixture.character_state(db, pid) == baseline,
                            "copyover changed durable character state")
            fixture.require(db.sql(f"SELECT account_name FROM player_data WHERE pid={pid}") ==
                            journey.ACCOUNT, "copyover changed durable player/account binding")
            fixture.record(phase, status="PASS", authority_before=before_owners,
                           authority_after=after_owners, same_game_socket=True,
                           binary_sha256=digest, provenance_sha=provenance_sha,
                           durable_state_sha256=fixture.state_digest(baseline))

            phase = "post-copyover-account-return"
            try:
                account_menu()
            except AssertionError:
                # Diagnose independently, then re-raise the original regression.
                client.close()
                client = None
                try:
                    client = journey.reconnect_character(port)
                    save()
                    fixture.require(fixture.character_state(db, pid) == baseline,
                                    "fresh login did not preserve the durable character")
                    account_menu()
                    fixture.record("diagnostic-fresh-login", status="PASS",
                                   account=journey.ACCOUNT, original_regression_still_red=True)
                    client.send("0")
                except Exception as diagnostic_error:
                    fixture.record("diagnostic-fresh-login", status="FAIL",
                                   error=db.redact(str(diagnostic_error)))
                raise
            fixture.record(phase, status="PASS", account=journey.ACCOUNT)
        except Exception as error:
            fixture.record("account-regression-failure", failed_phase=phase,
                           error=db.redact(str(error)))
            if (runtime / "server.out").exists():
                print("--- account regression server output ---\n" + db.redact(
                    (runtime / "server.out").read_text(errors="replace")[-12000:]), flush=True)
                print("--- account regression runtime logs ---\n" +
                      db.redact(journey.runtime_logs(runtime)), flush=True)
            raise
        finally:
            try:
                if client is not None:
                    client.close()
                if process is not None and process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=30)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=15)
                        raise AssertionError("account regression game required forced shutdown")
            finally:
                if output is not None:
                    output.close()
                try:
                    if db.started and db.port:
                        db.no_game_sessions()
                finally:
                    db.close()
    fixture.require(not Path(temporary).exists(), "account regression runtime survived cleanup")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--descriptor", type=Path, required=True,
                        help="trusted parent-frozen JSON build descriptor")
    parser.add_argument("--source-sha", required=True,
                        help="independently expected source/integration Git SHA")
    args = parser.parse_args(argv)
    fixture.require(Path.cwd().resolve() == fixture.ROOT, "run from repository root")
    binary, digest = parent_artifact(args.descriptor, args.source_sha)
    run(binary, digest, args.source_sha)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
