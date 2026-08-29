#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


with tempfile.TemporaryDirectory(prefix="duris-flatfile-ip-activity-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_ip_activity_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_ip_activity_harness.cpp",
            "src/sql.c",
            "src/flatfile_ip_activity_repository.c",
            "src/flatfile_store.c",
            "-Wl,--gc-sections",
            "-lcrypto",
            "-lbsd",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(temporary_path / "state")], cwd=ROOT, check=True)

    nanny = (ROOT / "src/nanny.c").read_text()
    if "if (timer < 0)" not in nanny or "Login history is temporarily unavailable" not in nanny:
        raise AssertionError("one-hour rule does not fail closed when flat IP state is unavailable")

print("flat-file IP activity runtime regression passed")
