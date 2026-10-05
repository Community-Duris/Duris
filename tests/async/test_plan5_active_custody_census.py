#!/usr/bin/env python3
"""Native read-only custody census qualification; no durable/world authority claim.

Build both native configurations from current source with sanitizers. An explicit
DURIS_PLAN5_CUSTODY_CENSUS_ARTIFACTS directory retains exact commands and outputs;
the default creates a fresh directory under bin/tests. No database is opened,
no operator production code imports mutation helpers, and no epoch is activated.
"""

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time
import unittest


ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    "tests/async/plan5_active_custody_census_fixture.cpp",
    "src/item/item_ownership_runtime.c",
    "src/item/item_transfer_command.c", "src/world/quest_mobile_native_reference.c", "src/economy/economic_source_event.c",
    "src/item/craft_pouch_mutation.c",
    "src/combat/chaos_pouch_ledger.c",
    "src/player/player_snapshot_codec.c",
    "src/persistence/critical_command.c",
]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class ActiveCustodyCensusTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which("g++"):
            raise RuntimeError("native custody qualification requires g++ and libcrypto")
        requested = os.environ.get("DURIS_PLAN5_CUSTODY_CENSUS_ARTIFACTS")
        if requested:
            cls.artifacts = Path(requested).resolve()
            if cls.artifacts.exists() or not cls.artifacts.is_relative_to((ROOT / "bin").resolve()):
                raise RuntimeError("select a fresh workspace/bin artifact directory")
            cls.artifacts.mkdir(parents=True)
        else:
            parent = ROOT / "bin/tests"
            parent.mkdir(parents=True, exist_ok=True)
            cls.artifacts = Path(tempfile.mkdtemp(prefix="plan5-active-custody-census-", dir=parent))
        cls.inputs = {
            str(path.relative_to(ROOT)): digest(path)
            for directory in (ROOT / "src", ROOT / "tests/async")
            for path in directory.rglob("*")
            if path.is_file() and path.suffix in {".c", ".cpp", ".h", ".hpp", ".inc"}
        }
        cls.inputs[str(Path(__file__).relative_to(ROOT))] = digest(Path(__file__))

    @classmethod
    def tearDownClass(cls):
        for name, sha in cls.inputs.items():
            assert digest(ROOT / name) == sha, name
        (cls.artifacts / "source-inputs.json").write_text(
            json.dumps(cls.inputs, indent=2, sort_keys=True) + "\n")

    def run_mode(self, backend):
        binary = self.artifacts / ("probe-" + backend)
        command = [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-fno-pie", "-no-pie", "-D__NO_TESTS__", "-Isrc",
        ]
        if backend == "flatfile":
            command.append("-D__NO_MYSQL__")
        command += SOURCES + ["-Wl,--wrap=_Znwm", "-lcrypto", "-o", str(binary)]
        started = time.monotonic()
        compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        row = {"backend": backend, "command": command, "compile_exit": compiled.returncode,
               "compile_seconds": time.monotonic() - started}
        (self.artifacts / (backend + "-compile.log")).write_text(compiled.stdout + compiled.stderr)
        (self.artifacts / (backend + "-results.json")).write_text(json.dumps(row, indent=2) + "\n")
        self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
        environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                           UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        started = time.monotonic()
        result = subprocess.run([str(binary)], cwd=ROOT, env=environment,
                                capture_output=True, text=True)
        row.update(run_exit=result.returncode, run_seconds=time.monotonic() - started,
                   binary_sha256=digest(binary),
                   sanitizer_environment={key: environment[key] for key in ("ASAN_OPTIONS", "UBSAN_OPTIONS")})
        (self.artifacts / (backend + "-stdout.log")).write_text(result.stdout)
        (self.artifacts / (backend + "-stderr.log")).write_text(result.stderr)
        (self.artifacts / (backend + "-results.json")).write_text(json.dumps(row, indent=2) + "\n")
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stderr, "")
        row["probe"] = json.loads(result.stdout)
        self.assertEqual(row["probe"]["cases"], 29)
        self.assertTrue(row["probe"]["read_only"])
        self.assertFalse(row["probe"]["world_authority_qualified"])
        self.assertFalse(row["probe"]["release_host_qualified"])
        (self.artifacts / (backend + "-results.json")).write_text(json.dumps(row, indent=2) + "\n")
        print("PLAN5_ACTIVE_CUSTODY_CENSUS " + json.dumps(row, sort_keys=True), flush=True)

    def test_native_sql_configuration(self):
        self.run_mode("sql")

    def test_native_flatfile_configuration(self):
        self.run_mode("flatfile")


if __name__ == "__main__":
    unittest.main(verbosity=2)
