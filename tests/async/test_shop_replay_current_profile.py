#!/usr/bin/env python3
"""Native passive SHOP CURRENT metadata; storage/publication acceptance is separate."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[2]
SOURCES = (
    "tests/async/shop_replay_current_profile_harness.cpp",
    "src/economy/shop_trade_transaction.c",
    "src/world/db.c",
    "src/player/player_snapshot_codec.c",
    "src/economy/shop_trade_accounting.c",
    "src/player/inert_item_stage.c",
    "src/mob/studioproclib.c",
    # Retained pending-map destruction reaches these actual allocator owners.
    "src/core/mm.c",
    "src/core/memory.c",
    "src/core/utility.c",
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--expect", choices=("supported", "refusal"), required=True)
    parser.add_argument("--backend", choices=("sql", "flatfile"), required=True)
    parser.add_argument("--extra-cxxflags", default="")
    parser.add_argument("--no-redis", action="store_true",
                        help="Exercise the required client-free Redis profile refusal")
    parser.add_argument("--old-glibc-predicate", action="store_true",
                        help="Test actual old-glibc refusal predicate with installed headers")
    args = parser.parse_args()
    if args.no_redis and args.expect != "refusal":
        parser.error("--no-redis requires --expect refusal")
    if args.old_glibc_predicate and args.expect != "refusal":
        parser.error("--old-glibc-predicate requires --expect refusal")
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    binary = output / f"shop-current-profile-{args.backend}-{args.expect}"
    flags = shlex.split(args.extra_cxxflags)
    if args.old_glibc_predicate:
        gate = output / "old-glibc-predicate.h"
        gate.write_text('#include <bits/stdc++.h>\n#include <features.h>\n'
                        '#undef __GLIBC__\n#undef __GLIBC_MINOR__\n'
                        '#define __GLIBC__ 2\n#define __GLIBC_MINOR__ 26\n')
        flags += ["-include", str(gate)]
    if args.no_redis:
        flags += ["-D__NO_REDIS__"]
    if args.backend == "flatfile":
        flags += ["-D__NO_MYSQL__", "-Isrc/no_mysql"]
    else:
        flags += shlex.split(subprocess.check_output(
            ["mysql_config", "--cflags"], text=True, timeout=30))
    command = [os.environ.get("CXX", "g++"), "-std=c++20", "-O1", "-g",
               "-Wall", "-Wextra", "-Wpedantic", "-Werror",
               "-ffunction-sections", "-fdata-sections", "-Isrc", "-I/usr/include/libxml2",
               *flags, *SOURCES, "-Wl,--gc-sections", "-pthread", "-o", str(binary)]
    receipt = {"expect": args.expect, "backend": args.backend, "no_redis": args.no_redis,
               "old_glibc_predicate_only": args.old_glibc_predicate,
               "command": command,
               "source_sha256": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
                                 for path in SOURCES}}
    compiled = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, timeout=900)
    (output / "compile.log").write_text(compiled.stdout, encoding="utf-8")
    receipt["compile_exit"] = compiled.returncode
    if not compiled.returncode:
        run = subprocess.run([str(binary), args.expect], cwd=ROOT, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20)
        (output / "run.log").write_text(run.stdout, encoding="utf-8")
        receipt.update(run_exit=run.returncode,
                       binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest())
        print(run.stdout, end="", flush=True)
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    if compiled.returncode:
        raise SystemExit(compiled.stdout)
    if receipt["run_exit"]:
        raise SystemExit(receipt["run_exit"])
    print("PASS passive SHOP CURRENT metadata; retained storage/native effects remain separate")


if __name__ == "__main__":
    main()
