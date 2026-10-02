#!/usr/bin/env python3
"""Observe existing entry points without importing or collecting other tests."""

from __future__ import annotations

import ast
from collections import Counter
import json
from pathlib import Path
import runpy
import sys
import time
import unittest


def declarations(path: Path) -> tuple[list[str], str | None]:
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    functions = [node.name for node in tree.body if isinstance(node, ast.FunctionDef)
                 and node.name.startswith("test_")]
    entry = "main" if any(isinstance(node, ast.FunctionDef) and node.name == "main"
                         for node in tree.body) else None
    return functions, entry


def execute(path: Path, mode: str, output: Path, minimum: int) -> int:
    """Write only case identities/outcomes; retain the original script semantics."""
    functions, entry = declarations(path)
    records: list[dict] = []
    collected: list[str] = []
    active: dict[tuple[int, int], dict] = {}
    called: set[str] = set()
    started = time.monotonic()
    complete = False

    def save() -> None:
        output.write_text(json.dumps({"cases": records, "collected": collected,
                                     "called": sorted(called), "complete": complete}), encoding="utf-8")

    def case_id(test) -> str:
        return test.id().removeprefix("__main__.")

    original_run = unittest.TextTestRunner.run
    original_start = unittest.TestResult.startTest
    original_stop = unittest.TestResult.stopTest

    def members(suite):
        if isinstance(suite, unittest.TestSuite):
            for child in suite:
                yield from members(child)
        else:
            yield case_id(suite)

    def run(result_runner, suite):
        collected.extend(members(suite))
        save()
        return original_run(result_runner, suite)

    def start(result, test):
        row = {"id": case_id(test), "status": "running", "elapsed": 0.0}
        records.append(row)
        active[id(result), id(test)] = {"row": row, "started": time.monotonic()}
        save()
        return original_start(result, test)

    def stop(result, test):
        item = active.pop((id(result), id(test)))
        row = item["row"]
        row["elapsed"] = time.monotonic() - item["started"]
        if row["status"] == "running":
            row["status"] = "passed"
        save()
        return original_stop(result, test)

    unittest.TextTestRunner.run = run
    unittest.TestResult.startTest = start
    unittest.TestResult.stopTest = stop
    for method, status in (("addFailure", "failed"), ("addError", "error"),
                           ("addSkip", "skipped"), ("addExpectedFailure", "expected_failure"),
                           ("addUnexpectedSuccess", "unexpected_success")):
        original = getattr(unittest.TestResult, method)

        def outcome(result, test, *args, _original=original, _status=status):
            item = active.get((id(result), id(test)))
            if item:
                item["row"]["status"] = _status
                if _status == "skipped":
                    item["row"]["reason"] = str(args[0])
            return _original(result, test, *args)

        setattr(unittest.TestResult, method, outcome)
    original_subtest = unittest.TestResult.addSubTest

    def subtest(result, test, sub, error):
        if error and (id(result), id(test)) in active:
            active[id(result), id(test)]["row"]["status"] = "failed"
        return original_subtest(result, test, sub, error)

    unittest.TestResult.addSubTest = subtest

    # Observe calls only in entries with function tests or main(). Native
    # assertions remain inside their executable; they are not invented cases.
    def observe(frame, event, arg):
        if event == "call" and frame.f_code.co_filename == str(path):
            name = frame.f_code.co_name
            if name in functions or name == entry:
                called.add(name)

    if mode != "unittest" and (functions or entry):
        sys.setprofile(observe)
    sys.argv = [str(path)]
    sys.path.insert(0, str(path.parent))
    code = 0
    try:
        runpy.run_path(str(path), run_name="__main__")
    except SystemExit as error:
        code = error.code if isinstance(error.code, int) else (0 if error.code is None else 1)
        if error.code is not None and not isinstance(error.code, int):
            print(error.code, file=sys.stderr)
    except BaseException:
        import traceback
        traceback.print_exc()
        code = 1
    finally:
        sys.setprofile(None)

    if mode == "unittest":
        if code == 0 and (not records or len(records) < minimum or
                          Counter(row["id"] for row in records) != Counter(collected) or
                          any(row["status"] == "running" for row in records)):
            print("case execution contract failed: collected "
                  f"{len(collected)}, executed {len(records)}, minimum {minimum}", file=sys.stderr)
            code = 1
        if code == 0 and any(row["status"] in {"failed", "error", "unexpected_success"}
                             for row in records):
            print("case execution contract failed: a failed case returned a successful entry",
                  file=sys.stderr)
            code = 1
    else:
        missing = set(functions) - called
        if entry and entry not in called:
            missing.add(entry)
        if code == 0 and missing:
            print("case execution contract failed: uncalled " + ", ".join(sorted(missing)),
                  file=sys.stderr)
            code = 1
        records.append({"id": "script", "status": "passed" if code == 0 else "failed",
                        "elapsed": time.monotonic() - started, "evidence": "entry"})
        for name in sorted(set(functions) & called):
            records.append({"id": name, "status": "passed" if code == 0 else "unresolved",
                            "evidence": "function invocation"})
    complete = True
    save()
    return code


if __name__ == "__main__":
    raise SystemExit(execute(Path(sys.argv[1]).resolve(), sys.argv[2],
                            Path(sys.argv[3]), int(sys.argv[4])))
