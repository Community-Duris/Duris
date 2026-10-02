#!/usr/bin/env python3
"""Qualify reviewed behavioral faults in a private copy of production sources.

Each fault must compile and be rejected by its actual executable owner. Positive
controls run before and after the mutation; build failures and missing mutation
sites never count as detection. SQL controls use new migrated/empty schemas.
"""
from __future__ import annotations

import argparse
from contextlib import nullcontext
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tests/async"))
from disposable_sql_fixture import DisposableSQL, clean_environment
from run_regression_tests import terminate_test


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def atomic(path, report):
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(report, indent=2) + "\n")
    temporary.replace(path)


def run_owner(command, cwd, environment, log, redact, timeout=900):
    process = subprocess.Popen(command, cwd=cwd, env=environment, text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               start_new_session=True)
    try:
        output = process.communicate(timeout=timeout)[0]
    except BaseException:
        # Retain the original partial diagnostics before the private source
        # copy and SQL provider are cleaned up. Stop descendants as well.
        output = terminate_test(process)
        log.write_text(redact(output))
        raise
    output = redact(output)
    log.write_text(output)
    return process.returncode, output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--family", choices=("offline", "sql"), required=True)
    parser.add_argument("--image")
    parser.add_argument("--evidence-dir", type=Path, required=True)
    args = parser.parse_args()
    if args.family == "sql" and not args.image:
        parser.error("SQL qualification requires the selected disposable engine image")
    directory = args.evidence_dir.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    catalog_path = ROOT / "tests/behavioral_faults.json"
    catalog = json.loads(catalog_path.read_text())
    faults = [fault for fault in catalog["faults"] if fault["family"] == args.family]
    report = dict(status="running", catalog_sha256=digest(catalog_path), attempts=[],
                  pending=[fault["id"] for fault in faults])
    target = directory / "faults.json"
    atomic(target, report)
    environment = dict(clean_environment(), CXX=os.environ.get("CXX", "g++"),
                       DURIS_NATIVE_BUILD_JOBS="2")
    # No credentials, local configuration, runtime data or artifacts are copied.
    with tempfile.TemporaryDirectory(prefix="behavioral-faults-", dir=ROOT / "bin") as temporary:
        copied = Path(temporary)
        for name in ("src", "tests", "migrations"):
            shutil.copytree(ROOT / name, copied / name, ignore=shutil.ignore_patterns("__pycache__"))
        (copied / "bin/tests").mkdir(parents=True)
        original_sources = {fault["source"]: digest(ROOT / fault["source"]) for fault in faults}
        provider = DisposableSQL(args.image, directory / "service") if args.family == "sql" else nullcontext(None)
        try:
            with provider as sql:
                for fault in faults:
                    path = copied / fault["source"]
                    original = path.read_text()
                    if original.count(fault["find"]) != 1:
                        raise RuntimeError("mutation site changed: " + fault["id"])
                    outcomes = []
                    for phase in ("before", "fault", "after"):
                        path.write_text(original.replace(fault["find"], fault["replace"])
                                        if phase == "fault" else original)
                        fixture = sql.schema(fault["schema_prefix"], migrated=fault["migrated"]) if sql else nullcontext(environment)
                        with fixture as fixture_environment:
                            execution_environment = dict(fixture_environment, **fault.get("environment", {}))
                            started = time.monotonic()
                            exit_code, output = run_owner(
                                [sys.executable, str(copied / "tests/async" / fault["owner"])],
                                copied, execution_environment,
                                directory / (fault["id"] + "-" + phase + ".log"),
                                sql.redact if sql else lambda value: value)
                            passed = (exit_code != 0 and fault["compiled_marker"] in output
                                      and fault["failure_marker"] in output) if phase == "fault" else (
                                          exit_code == 0 and fault["positive_marker"] in output)
                            outcomes.append(dict(phase=phase, exit=exit_code,
                                                 elapsed=time.monotonic() - started, passed=passed))
                            atomic(directory / (fault["id"] + ".json"), dict(fault=fault, outcomes=outcomes))
                            print(f"fault {fault['id']} {phase}: {'qualified' if passed else 'FAILED'}", flush=True)
                            if not passed:
                                raise RuntimeError("fault/control did not qualify: " + fault["id"] + "/" + phase)
                    path.write_text(original)
                    report["attempts"].append(dict(id=fault["id"], requirement=fault["requirement"], outcomes=outcomes))
                    report["pending"].remove(fault["id"])
                    atomic(target, report)
            if any(digest(ROOT / name) != value for name, value in original_sources.items()):
                raise RuntimeError("qualification changed the original production source")
            report["status"] = "passed"
        except BaseException as error:
            report["status"] = "incomplete" if isinstance(error, (KeyboardInterrupt, SystemExit)) else "failed"
            report["error"] = str(error) or type(error).__name__
        finally:
            atomic(target, report)
    if report["status"] != "passed":
        raise SystemExit("behavioral fault qualification failed; " + str(target))
    print("PASS: behavioral faults and before/after executable controls: " + args.family, flush=True)


if __name__ == "__main__":
    main()
