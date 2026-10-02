#!/usr/bin/env python3

from _paths import rel
import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-item-uid-test-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_item_uid_test"
    store_object = temporary_path / "store.o"
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
             "-fno-pie", "-no-pie", "-Isrc"]
    # Keep the real native store; intercept only the initialization-marker write
    # to exercise a failure after the allocator high-water mark is durable.
    subprocess.run(["g++", *flags, "-Dflatfile_atomic_write=flatfile_test_atomic_write",
                    "-c", rel("flatfile_store.c"), "-o", str(store_object)],
                   cwd=ROOT, check=True)
    compile_result = subprocess.run(
        [
            "g++",
            *flags,
            "tests/async/flatfile_item_uid_allocator_harness.cpp",
            rel("flatfile_item_uid_allocator.c"),
            str(store_object),
            "-lcrypto",
            "-pthread",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if compile_result.returncode:
        raise SystemExit(compile_result.stdout)

    state_root = temporary_path / "state"
    run_result = subprocess.run(
        [str(binary), str(state_root)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    print(run_result.stdout.strip())
