#!/usr/bin/env python3
"""Execute property commands, including refreshing loaded PCs and NPCs after reload."""
from pathlib import Path
import os
import subprocess
import tempfile
from _paths import HARNESS_STUBS

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
                            "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                            "-fno-pie", "-no-pie",
                            "-I/usr/include/libxml2", "-I/usr/include/mysql", *flags,
                            str(ROOT / "tests/async/issue263_property_reload.cpp"),
                            str(ROOT / "src/core/safe_format.c"),
                            str(ROOT / "src/telemetry/telemetry_config.c"), "-lcrypto",
                            "-Wl,--gc-sections,--wrap=fclose,--wrap=rename,--wrap=fgets",
                            str(HARNESS_STUBS), "-o", str(binary)], check=True, cwd=ROOT)
            results = [subprocess.run([str(binary)], cwd=work).returncode]
            for case in ("keys", "reload-missing", "reload-read", "reload-close-source",
                         "reload-malformed", "reload-limit", "save-missing", "save-open",
                         "save-read", "save-write", "save-flush", "save-close-source",
                         "save-close-target", "save-rename", "save-malformed"):
                with tempfile.TemporaryDirectory(prefix=case + "-", dir=work) as scenario:
                    (Path(scenario) / "lib").mkdir()
                    results.append(subprocess.run([str(binary), case], cwd=scenario).returncode)
            assert all(result == 0 for result in results), results
            print("real property-command journey passed:", mode, flush=True)

if __name__ == "__main__":
    main()
