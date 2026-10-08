#!/usr/bin/env python3
"""Actual fee providers on modeled values; no SQL/world/owner/journal access.

Run in a composed candidate on D:. Compile and execute the original maintained
context fixture unchanged, then the separate fee component. Both use the original
strict sanitizer recipe and 20-second runtime deadline. No provider is extracted
or replaced; only the original fixture needs its delegating allocator observer.
"""

import ast
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tests/async"))
from native_build_artifacts import build_native

COMPONENT = "tests/async/quest_accounting_prep/native_quest_fee_owner_boundary_test.cpp"
PROVIDERS = [
    "src/economy/native_quest_cost.c",
    "src/economy/native_quest_coin_give.c",
    "src/item/lockpick_retirement_continuation.c",
]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def maintained_configuration():
    path = ROOT / "tests/async/test_native_quest_recovery_context.py"
    values = {}
    for statement in ast.parse(path.read_text()).body:
        if isinstance(statement, ast.Assign) and len(statement.targets) == 1:
            target = statement.targets[0]
            if isinstance(target, ast.Name) and target.id in {"SOURCES", "FLAGS", "LINK", "RUNTIME_SECONDS"}:
                values[target.id] = ast.literal_eval(statement.value)
    if set(values) != {"SOURCES", "FLAGS", "LINK", "RUNTIME_SECONDS"} or values["RUNTIME_SECONDS"] != 20:
        raise ValueError("maintained fixture controls changed; review required")
    return values


def main():
    config = maintained_configuration()
    parent = ROOT / "bin/tests"
    parent.mkdir(parents=True, exist_ok=True)
    if parent.is_symlink() or not parent.resolve().is_relative_to(ROOT.resolve()):
        raise ValueError("artifact parent must be owned below candidate/bin/tests")
    artifacts = Path(tempfile.mkdtemp(prefix="native-quest-fee-component-", dir=parent))
    sources = config["SOURCES"][1:] + PROVIDERS
    inputs = [*sources, COMPONENT, config["SOURCES"][0],
              "tests/async/test_native_quest_recovery_context.py",
              "tests/async/native_build_artifacts.py", "tests/async/server_build_artifacts.py",
              "tests/async/quest_accounting_prep/test_native_quest_fee_owner_boundary.py"]
    receipt = dict(
        classification="actual providers on modeled fee values; original context controls unchanged",
        candidate_revision=os.environ.get("DURIS_QUEST_FEE_CANDIDATE_REVISION"),
        owner_authenticated=False, SQL=False, journal=False, native_journey=False,
        source_sha256={name: sha(ROOT / name) for name in inputs},
        flags=config["FLAGS"], original_link=config["LINK"],
        component_link=[flag for flag in config["LINK"] if flag != "-Wl,--wrap=_Znwm"],
        provider_sources=sources, compile_command_timeout_seconds=600,
        runtime_timeout_seconds=config["RUNTIME_SECONDS"], compiler=os.environ.get("CXX", "g++"),
        runs=[], status="prepared", artifacts_retained=True,
        generated_fixture=False,
        scaffolding="literal modeled values only; original unchanged fixture allocator wrapper delegates to real allocator",
        uncalled_capabilities=["native capture/birth/admission", "SQL", "world publication/rebind",
                              "physical or reward ACK execution", "journal access/retirement"],
    )
    try:
        receipt["compiler_version"] = subprocess.check_output(
            [*shlex.split(receipt["compiler"]), "--version"], cwd=ROOT, text=True, timeout=20).splitlines()[0]
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        receipt["sanitizer_runtime"] = {name: env[name] for name in ("ASAN_OPTIONS", "UBSAN_OPTIONS")}
        for name, source, link in (
                ("original-context", config["SOURCES"][0], config["LINK"]),
                ("fee-component", COMPONENT, receipt["component_link"])):
            run = dict(name=name, source=source, link=link, status="compiling")
            receipt["runs"].append(run)
            started = time.monotonic()
            run["compile_sources"] = [source, *sources]
            binary = build_native(artifacts / name, run["compile_sources"], config["FLAGS"], link,
                                  compiler=receipt["compiler"], name="native-quest-" + name)
            run.update(binary=str(binary), binary_sha256=sha(binary),
                       build_elapsed_seconds=time.monotonic() - started, status="running",
                       runtime_command=[str(binary)])
            started = time.monotonic()
            completed = subprocess.run([str(binary)], cwd=ROOT, env=env, capture_output=True,
                                       timeout=config["RUNTIME_SECONDS"])
            (artifacts / (name + ".log")).write_bytes(completed.stdout + completed.stderr)
            run.update(exit_code=completed.returncode, runtime_elapsed_seconds=time.monotonic() - started)
            if completed.returncode or completed.stderr:
                raise AssertionError(name + " failed; see retained runtime log")
            if name == "fee-component":
                run["summary"] = json.loads(completed.stdout.splitlines()[-1])
                if (run["summary"]["owner_authenticated"] is not False or
                        run["summary"]["SQL"] is not False or
                        run["summary"]["journal"] is not False or
                        run["summary"]["native_journey"] is not False or
                        run["summary"]["controls"] < 1 or
                        run["summary"]["controls"] != sum(line.startswith(b"PASS ") for line in completed.stdout.splitlines())):
                    raise AssertionError("invalid component proof classification/count")
            run["status"] = "passed_component_only"
        receipt["status"] = "passed_components_only"
    except BaseException as error:
        receipt.update(status="failed", failure_type=type(error).__name__, failure=str(error))
        raise
    finally:
        receipt["source_unchanged"] = all(sha(ROOT / name) == pin for name, pin in receipt["source_sha256"].items())
        if not receipt["source_unchanged"]:
            receipt.update(status="failed", failure="source changed during qualification")
        (artifacts / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
        print("FEE_COMPONENT_RECEIPT " + str(artifacts / "receipt.json"), flush=True)
        if not receipt["source_unchanged"]:
            raise RuntimeError("source changed during qualification")


if __name__ == "__main__":
    main()
