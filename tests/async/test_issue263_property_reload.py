#!/usr/bin/env python3
"""Compile and run the real administrator-property command journey, narrowly."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def main():
    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="issue263-properties-", dir=output) as directory:
        work = Path(directory)
        (work / "lib").mkdir()
        for mode, flags in [("sql", []), ("flatfile", ["-D__NO_MYSQL__", "-I" + str(ROOT / "src/no_mysql")])]:
            binary = work / mode
            subprocess.run([os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                            "-ffunction-sections", "-fdata-sections", "-I" + str(ROOT / "src"),
                            "-I/usr/include/libxml2", "-I/usr/include/mysql", *flags,
                            str(ROOT / "tests/async/issue263_property_reload.cpp"),
                            str(ROOT / "src/core/safe_format.c"),
                            str(ROOT / "src/telemetry/telemetry_config.c"), "-lcrypto",
                            "-Wl,--gc-sections", "-o", str(binary)], check=True, cwd=ROOT)
            subprocess.run([str(binary)], check=True, cwd=work)
            print("real property-command journey passed:", mode, flush=True)

if __name__ == "__main__":
    main()
