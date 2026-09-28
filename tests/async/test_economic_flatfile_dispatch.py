#!/usr/bin/env python3
"""Native bank-only dispatcher contract in both compilation modes."""
import os
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="duris-flat-dispatch-") as temporary:
    for mode in ("sql", "client-free"):
        binary = Path(temporary) / mode
        flags = ["-D__NO_MYSQL__", "-Isrc/no_mysql"] if mode == "client-free" else []
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                        "-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                        "-fno-pie", "-no-pie", "-Isrc", *flags,
                        "tests/async/economic_flatfile_dispatch_test.cpp",
                        "src/flatfile/flatfile_accounting_dispatch.c", "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True, timeout=30,
                       env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                                UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"))
        print(mode + " dispatcher passed", flush=True)
