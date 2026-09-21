#!/usr/bin/env python3
"""Real flatfile bank execution across coordinator restart; no live publication claim."""
import os
from pathlib import Path
import subprocess
import tempfile
from test_flatfile_accounting_store import ROOT, SOURCES

with tempfile.TemporaryDirectory(prefix="duris-flat-admission-") as temporary:
    work = Path(temporary)
    # Reuse phase8's unchanged identity/metadata fixtures rather than copy them.
    fixture = (ROOT / "tests/async/flatfile_accounting_bank_test.cpp").read_text()
    marker = "int main(int argc, char **argv)"
    assert fixture.count(marker) == 1, "phase8 bank fixture must contain exactly one expected main boundary"
    (work / "phase8_bank_fixture.h").write_text(fixture.split(marker)[0])
    sources = ["tests/async/economic_flatfile_admission_native_test.cpp",
               "src/flatfile/flatfile_accounting_dispatch.c",
               "src/flatfile/flatfile_accounting_bank_transaction.c",
               "src/flatfile/flatfile_accounting_authority.c",
               "src/flatfile/flatfile_identity_repository.c",
               "src/flatfile/flatfile_player_domain_repository.c",
               "src/world/epic_command.c", "src/combat/combat_outcome_command.c",
               "src/economy/economic_command_admission.c",
               "src/persistence/critical_command_coordinator.c",
               "src/persistence/critical_command_journal.c", *SOURCES[1:]]
    binary = work / "native"
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-fno-pie", "-no-pie", "-D__NO_MYSQL__", "-DDURIS_FLATFILE_ACCOUNTING_TEST",
                    "-DDURIS_FLATFILE_AUTHORITY_FAULT_TEST", "-Isrc", "-Isrc/no_mysql", "-I" + str(work),
                    *sources, "-Wl,--wrap=_Znwm,--wrap=_Znam", "-lcrypto", "-lz", "-pthread", "-o", str(binary)],
                   cwd=ROOT, check=True)
    subprocess.run([str(binary), str(work / "state")], check=True, timeout=90,
                   env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                            UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
