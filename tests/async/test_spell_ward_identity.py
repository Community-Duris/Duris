#!/usr/bin/env python3
"""Link the actual shared classifier under both maintained PFILE headers."""
from pathlib import Path
import os
import subprocess
import tempfile
from _paths import ROOT

with tempfile.TemporaryDirectory(prefix="duris-ward-identity-") as directory:
    for backend in ("sql", "flatfile"):
        executable = Path(directory) / backend
        command = [os.environ.get("CXX", "g++"), "-std=c++20", "-Wall", "-Wextra",
                   "-Wpedantic", "-Werror", "-D_PFILE_", "-D__NO_TESTS__",
                   "-DDURIS_PERSISTENT_TRANSPORT", "-Isrc"]
        if backend == "flatfile":
            command += ["-D__NO_MYSQL__", "-Isrc/no_mysql"]
        command += ["tests/async/spell_ward_identity_test.cpp",
                    "src/combat/spell_ward_identity.c", "-o", str(executable)]
        subprocess.run(command, cwd=ROOT, check=True, timeout=180)
        subprocess.run([str(executable)], cwd=ROOT, check=True, timeout=10)
print("Actual ward identity: SQL and flat-file PFILE controls passed")
