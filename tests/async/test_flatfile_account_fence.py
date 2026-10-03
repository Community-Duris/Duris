#!/usr/bin/env python3
"""Native account-fence admission under the existing authority transaction lock."""
import os
from pathlib import Path
import subprocess
import tempfile

from test_flatfile_accounting_store import ROOT, SOURCES

sources = ["tests/async/flatfile_account_fence_harness.cpp",
           "src/flatfile/flatfile_account_adapter.c",
           "src/flatfile/flatfile_account_repository.c",
           "src/flatfile/flatfile_identity_repository.c",
           "src/flatfile/flatfile_ip_activity_repository.c",
           "src/persistence/persistence_mode.c",
           "src/flatfile/flatfile_accounting_authority.c", *SOURCES[1:]]

with tempfile.TemporaryDirectory(prefix="flat-account-fence-") as temporary:
    binary = Path(temporary) / "fixture"
    subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                    "-fno-pie", "-no-pie", "-ffunction-sections", "-fdata-sections",
                    "-Wl,--gc-sections", "-D__NO_MYSQL__",
                    "-DDURIS_FLATFILE_ACCOUNTING_TEST", "-Isrc", "-Isrc/no_mysql",
                    *sources, "-lcrypto", "-pthread", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(Path(temporary) / "state")], cwd=ROOT, check=True,
                   env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                            UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
