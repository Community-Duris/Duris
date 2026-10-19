#!/usr/bin/env python3
"""Concurrent inspector requests share one build; later requests rebuild fresh."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

from _paths import ROOT


def wait_for(path):
    deadline = time.monotonic() + 30
    while not path.exists():
        assert time.monotonic() < deadline, f"timed out waiting for {path}"
        time.sleep(0.02)


build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="inspector-coalescing-", dir=build) as temporary:
    root = Path(temporary)
    fake_bin = root / "fake-bin"
    fake_bin.mkdir()
    compiler = fake_bin / "g++"
    compiler.write_text(r'''#!/usr/bin/env python3
import os
from pathlib import Path
import sys
import time
root = Path(os.environ["INSPECTOR_BUILD_TEST_ROOT"])
with (root / "calls").open("a") as calls:
    calls.write("compile\n")
(root / "compiling").touch()
deadline = time.monotonic() + 30
while not (root / "release").exists():
    assert time.monotonic() < deadline
    time.sleep(0.02)
output = Path(sys.argv[sys.argv.index("-o") + 1])
output.write_text("#!/bin/sh\nexit 0\n")
output.chmod(0o755)
''')
    compiler.chmod(0o755)
    wrapper = root / "request.py"
    wrapper.write_text(r'''
import fcntl
from pathlib import Path
import runpy
import sys
script, destination, marker = sys.argv[1:]
original = fcntl.flock
def observed(file, operation):
    Path(marker).touch()
    return original(file, operation)
fcntl.flock = observed
sys.path.insert(0, str(Path(script).parent))
sys.argv = [script, "--build-inspector", destination]
runpy.run_path(script, run_name="__main__")
''')
    environment = dict(os.environ, PATH=str(fake_bin) + os.pathsep + os.environ["PATH"],
                       INSPECTOR_BUILD_TEST_ROOT=str(root))
    script = ROOT / "tests/async/test_flatfile_player_repository.py"
    destination = root / "inspector"
    processes = []

    def request(name, output=destination):
        process = subprocess.Popen([sys.executable, str(wrapper), str(script), str(output),
                                    str(root / name)], cwd=ROOT, env=environment,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        processes.append(process)
        return process

    def finish(process):
        output, _ = process.communicate(timeout=30)
        assert process.returncode == 0, output

    try:
        first = request("first")
        wait_for(root / "compiling")
        second = request("second")
        third = request("third")
        wait_for(root / "second")
        wait_for(root / "third")
        (root / "release").touch()
        for process in (first, second, third):
            finish(process)
        assert (root / "calls").read_text().splitlines() == ["compile"]
        assert destination.is_file() and os.access(destination, os.X_OK)

        finish(request("later"))
        assert (root / "calls").read_text().splitlines() == ["compile", "compile"]
        invalid = request("invalid", ROOT / "outside-inspector")
        output, _ = invalid.communicate(timeout=30)
        assert invalid.returncode != 0 and "inspector must be placed below bin/" in output
        assert (root / "calls").read_text().splitlines() == ["compile", "compile"]
    finally:
        (root / "release").touch()
        for process in processes:
            if process.poll() is None:
                process.kill()
            process.wait(timeout=5)

print("inspector build coalescing, fresh sequential rebuild and destination guard passed")
