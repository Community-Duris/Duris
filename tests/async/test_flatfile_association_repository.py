#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-association-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_association_test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            "tests/async/flatfile_association_repository_harness.cpp",
            "src/flatfile_association_repository.c",
            "src/flatfile_authority_transaction.c",
            "src/flatfile_store.c",
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
    run_result = subprocess.run(
        [str(binary), str(temporary_path / "state")],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    print(run_result.stdout.strip())

preprocess = subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-D__NO_MYSQL__",
        "-Isrc/no_mysql",
        "-Isrc",
        "-Isrc/ships",
        "-I/usr/include/libxml2",
        "-E",
        "-P",
        "src/assocs.c",
    ],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if preprocess.returncode:
    raise SystemExit(preprocess.stdout)
for token in (
    "flatfile_association_list(root, &records, &error)",
    "flatfile_association_establish(root, {}, &error)",
    "flatfile_association_save(root, record, &error)",
    "flatfile_association_erase(root, id_number, &error)",
    "missing = load_guild(id) ? 0 : missing + 1",
):
    if token not in preprocess.stdout:
        raise SystemExit(f"client-free guild runtime route is missing {token}")
for query in (
    "SELECT id, name FROM associations WHERE id",
    "SELECT id, name, prestige, construction_points FROM associations",
):
    if query in preprocess.stdout:
        raise SystemExit(f"client-free guild runtime still contains SQL: {query}")

source = (ROOT / "src/assocs.c").read_text()
for token in (
    'sscanf(buf, "%u %ld %ld %13s %c"',
    'sscanf(buf, "%13s %u %u %c"',
    '((frag_fields == 3) != (new_guild->frags.top_frags == 0))',
):
    if token not in source:
        raise SystemExit(f"historical guild parser safety is missing {token}")
