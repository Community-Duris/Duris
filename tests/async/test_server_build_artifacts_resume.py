#!/usr/bin/env python3
"""Real Make/compiler recovery tests; disposable files, no server or services."""

import multiprocessing
import os
from pathlib import Path
import shlex
import shutil
import select
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

import server_build_artifacts as artifacts


MAKEFILE = """CC = {compiler}
CFLAGS = -std=c++20 -D__NO_MYSQL__ -Wall -Wextra -Werror
CFLAGS += $(EXTRA_CFLAGS)
all: $(DMS_BINARY)
$(DMS_BINARY): $(OBJDIR)/a.o $(OBJDIR)/b.o
\t@mkdir -p $(dir $@)
\t$(CC) $(CFLAGS) $^ -o $@
$(OBJDIR)/%.o: %.cpp
\t@mkdir -p $(dir $@)
\t$(CC) $(CFLAGS) -MMD -MP -c $< -o $@
-include $(OBJDIR)/a.d $(OBJDIR)/b.d
"""

# This driver delegates successful compilation to the installed C++ compiler.
# Its controlled failure writes the very output paths the atomic wrapper gave it.
DRIVER = r'''
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
root = Path(__file__).parent
compiler, *arguments = sys.argv[1:]
source = next((arg for arg in arguments if arg.endswith('.cpp')), None)
if '-c' in arguments:
    with (root / 'counts').open('a') as output:
        output.write(source + '\n')
    mode = (root / 'control').read_text()
    if source == 'b.cpp' and mode in ('change', 'partial-change'):
        (root / 'src/shared.h').write_text('#define ADJUSTMENT 2\n')
    if source == 'b.cpp' and mode in ('partial', 'partial-change'):
        Path(arguments[arguments.index('-o') + 1]).write_bytes(b'partial object')
        Path(arguments[arguments.index('-MF') + 1]).write_text('partial dependency')
        sys.exit(9)
    if source == 'b.cpp' and mode == 'block':
        # Wait for a completed sibling object: timeout tests cannot accidentally
        # pass after stalling before there was any meaningful work to preserve.
        object_path = Path(arguments[arguments.index('-MT') + 1]).with_name('a.o')
        while not object_path.exists():
            time.sleep(0.01)
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
        program = ('import os,signal; from pathlib import Path; '
                   'signal.signal(signal.SIGTERM,signal.SIG_IGN); '
                   f'Path({str(root / "grandchild")!r}).write_text(str(os.getpid())); '
                   'signal.pause()')
        child = subprocess.Popen([sys.executable, '-c', program])
        while not (root / 'grandchild').exists():
            time.sleep(0.01)
        (root / 'ready').write_text(str(os.getpid()))
        signal.pause()
sys.exit(subprocess.call([compiler, *arguments]))
'''


def alive(pid):
    try:
        state = Path(f"/proc/{pid}/stat").read_text().split(") ", 1)[1].split()[0]
        return state not in {"Z", "X"}
    except FileNotFoundError:
        return False


def wait_for(predicate, timeout=10):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if predicate():
            return
        time.sleep(0.01)
    raise AssertionError("bounded fixture readiness/cleanup wait expired")


def stop_child(process):
    if process.is_alive():
        process.kill()
    process.join(timeout=5)


class ResumeTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="duris-build-resume-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / "src").mkdir()
        self.cache = self.root / "bin/cache"
        self.driver = self.root / "compiler.py"
        self.driver.write_text(DRIVER)
        compiler = shlex.join([sys.executable, "-B", str(self.driver),
                               shutil.which(shlex.split(os.environ.get("CXX", "g++"))[0])])
        (self.root / "src/Makefile").write_text(MAKEFILE.format(compiler=compiler))
        (self.root / "src/a.cpp").write_text("int value() { return 41; }\n")
        (self.root / "src/shared.h").write_text("#define ADJUSTMENT 1\n")
        (self.root / "src/b.cpp").write_text(
            '#include "shared.h"\n#include <cstdio>\nint value();\n'
            'int main(int argc, char **) { if (argc > 1) { '
            'std::puts("holding"); std::fflush(stdout); std::getchar(); } '
            'std::printf("%d\\n", value() + ADJUSTMENT); }\n')
        (self.root / "control").write_text("normal")
        for context in (
            patch.object(artifacts, "ROOT", self.root),
            # Installed-toolchain byte sensitivity is separately covered by
            # test_server_build_artifacts.py; avoid hashing the entire sysroot
            # repeatedly in this small process/atomicity integration fixture.
            patch.object(artifacts, "toolchain_key", side_effect=lambda env:
                         artifacts.file_hash(self.driver)),
            patch.dict(os.environ, {artifacts.CACHE_ENV: str(self.cache), "MAKEFLAGS": ""}),
        ):
            context.start()
            self.addCleanup(context.stop)

    def build(self):
        return artifacts.build_flatfile_server(self.root / "unused-runtime")

    def key(self):
        return artifacts.input_key(dict(os.environ))

    def count(self, name):
        return (self.root / "counts").read_text().splitlines().count(name)

    def no_live_compilers(self):
        for name in ("ready", "grandchild"):
            path = self.root / name
            self.assertTrue(path.exists(), "compiler did not reach its synchronized stall")
            wait_for(lambda: not alive(int(path.read_text())))

    def test_timeout_retains_completed_objects_and_resumes_same_key(self):
        (self.root / "control").write_text("block")
        key = self.key()
        # Only this disposable fixture uses a shortened injection deadline.
        # The production helper contract remains 600 seconds with no retries.
        with patch.object(artifacts, "BUILD_TIMEOUT", 4):
            key = self.key()
            with self.assertRaises(subprocess.TimeoutExpired):
                self.build()
            self.no_live_compilers()
            pending = self.cache / f"pending-{key}"
            completed = pending / "objects/server/a.o"
            timestamp = completed.stat().st_mtime_ns
            self.assertTrue((pending / "objects/server/a.d").read_text().startswith(str(completed) + ":"))
            self.assertFalse((pending / "objects/server/b.o").exists())
            self.assertFalse((pending / "objects/server/b.d").exists())
            self.assertIsNone(artifacts.verified_artifact(self.cache, key))
            self.assertFalse((self.cache / f"{key}.json").exists())
            (self.root / "control").write_text("normal")
            binary = self.build()
            self.assertEqual(completed.stat().st_mtime_ns, timestamp)
            self.assertEqual(self.count("a.cpp"), 1)
            self.assertEqual(self.count("b.cpp"), 2)
            self.assertTrue(binary.parent.parent.name.startswith("artifact-"))
            self.assertEqual(subprocess.check_output([str(binary)], text=True), "42\n")
            self.assertFalse(binary.stat().st_mode & 0o222)
            self.assertEqual(self.build(), binary)

    def test_failed_partial_output_is_not_published_and_retry_is_incremental(self):
        (self.root / "control").write_text("partial")
        key = self.key()
        with self.assertRaises(AssertionError):
            self.build()
        pending = self.cache / f"pending-{key}"
        self.assertTrue((pending / "objects/server/a.o").exists())
        self.assertFalse((pending / "objects/server/b.o").exists())
        self.assertFalse((pending / "objects/server/b.d").exists())
        self.assertFalse(list(pending.rglob(".duris-compile-*")))
        self.assertIsNone(artifacts.verified_artifact(self.cache, key))
        (self.root / "control").write_text("normal")
        self.assertEqual(subprocess.check_output([str(self.build())], text=True), "42\n")
        self.assertEqual(self.count("a.cpp"), 1)

    def test_changed_source_isolates_pending_work(self):
        (self.root / "control").write_text("partial")
        previous = self.key()
        with self.assertRaises(AssertionError):
            self.build()
        (self.root / "src/shared.h").write_text("#define ADJUSTMENT 2\n")
        current = self.key()
        self.assertNotEqual(current, previous)
        (self.root / "control").write_text("normal")
        self.assertEqual(subprocess.check_output([str(self.build())], text=True), "43\n")
        self.assertEqual(self.count("a.cpp"), 2)
        self.assertTrue((self.cache / f"pending-{previous}/objects/server/a.o").exists())
        self.assertTrue((self.cache / f"pending-{current}/objects/server/a.o").exists())

    def test_inputs_changed_during_failed_attempt_discard_its_completed_objects(self):
        (self.root / "control").write_text("partial-change")
        previous = self.key()
        with self.assertRaises(AssertionError):
            self.build()
        self.assertFalse((self.cache / f"pending-{previous}").exists())
        self.assertFalse((self.cache / f"{previous}.json").exists())
        (self.root / "src/shared.h").write_text("#define ADJUSTMENT 1\n")
        (self.root / "control").write_text("normal")
        self.assertEqual(self.key(), previous)
        self.assertEqual(subprocess.check_output([str(self.build())], text=True), "42\n")
        self.assertEqual(self.count("a.cpp"), 2)

    def test_inputs_changed_during_successful_attempt_cannot_publish(self):
        (self.root / "control").write_text("change")
        previous = self.key()
        with self.assertRaises(RuntimeError):
            self.build()
        self.assertFalse((self.cache / f"pending-{previous}").exists())
        self.assertFalse((self.cache / f"{previous}.json").exists())
        self.assertFalse(list(self.cache.glob("artifact-*")))
        (self.root / "control").write_text("normal")
        self.assertEqual(subprocess.check_output([str(self.build())], text=True), "43\n")

    def test_concurrent_real_builds_publish_once(self):
        context = multiprocessing.get_context("fork")
        results = context.Queue()
        def run():
            try:
                results.put(str(self.build()))
            except BaseException as error:
                results.put(type(error).__name__)
        processes = [context.Process(target=run) for _ in range(3)]
        for process in processes:
            self.addCleanup(stop_child, process)
            process.start()
        paths = [results.get(timeout=20) for _ in processes]
        for process in processes:
            process.join(timeout=10)
            self.assertEqual(process.exitcode, 0)
        self.assertEqual(len(set(paths)), 1)
        self.assertTrue(Path(paths[0]).is_file())
        self.assertEqual(self.count("a.cpp"), 1)
        self.assertEqual(self.count("b.cpp"), 1)
        self.assertEqual(len(list(self.cache.glob("artifact-*"))), 1)

    def test_inherited_make_overrides_and_strict_warnings_are_preserved(self):
        source = self.root / "src/b.cpp"
        source.write_text('#ifndef FLAG\n#error missing inherited build flag\n#endif\n'
                          'static_assert(FLAG == 123);\n' + source.read_text())
        with patch.dict(os.environ, {"MAKEFLAGS": "w -- EXTRA_CFLAGS=-DFLAG=123"}):
            binary = self.build()
            self.assertEqual(subprocess.check_output([str(binary)], text=True), "42\n")
            self.assertIn("-DFLAG=123", (binary.parent.parent / "build.log").read_text())
        source.write_text('int main() { int unused = 1; }\n')
        key = self.key()
        with self.assertRaises(AssertionError) as failure:
            self.build()
        self.assertIn("-Werror", str(failure.exception))
        self.assertFalse((self.cache / f"{key}.json").exists())
        self.assertFalse((self.cache / f"pending-{key}/objects/server/b.o").exists())

    def test_caller_term_and_hard_kill_leave_no_live_compiler_descendants(self):
        context = multiprocessing.get_context("fork")
        for signum in (signal.SIGTERM, signal.SIGKILL):
            with self.subTest(signal=signum):
                for name in ("ready", "grandchild"):
                    (self.root / name).unlink(missing_ok=True)
                (self.root / "control").write_text("block")
                child = context.Process(target=self.build)
                self.addCleanup(stop_child, child)
                child.start()
                wait_for(lambda: (self.root / "ready").exists())
                os.kill(child.pid, signum)
                child.join(timeout=10)
                self.assertFalse(child.is_alive())
                self.no_live_compilers()
                self.assertIsNone(artifacts.verified_artifact(self.cache, self.key()))
        (self.root / "control").write_text("normal")
        self.assertEqual(subprocess.check_output([str(self.build())], text=True), "42\n")

    def test_workspace_symlinks_and_wrong_key_are_refused(self):
        self.cache.mkdir(parents=True)
        key = self.key()
        pending = self.cache / f"pending-{key}"
        pending.mkdir()
        artifacts.workspace_state(pending, key, True)
        outside = self.root / "outside"
        outside.mkdir()
        sentinel = outside / "sentinel"
        sentinel.write_text("unchanged")
        (pending / "objects").symlink_to(outside, target_is_directory=True)
        with self.assertRaises(ValueError):
            self.build()
        self.assertEqual(sentinel.read_text(), "unchanged")
        (pending / "objects").unlink()
        artifacts.workspace_state(pending, "0" * 64, True)
        with self.assertRaises(ValueError):
            self.build()
        with patch.object(artifacts, "input_key", return_value="../escaped"):
            with self.assertRaises(ValueError):
                self.build()

    def test_cache_and_lock_symlinks_and_hardlinked_objects_are_refused(self):
        self.cache.mkdir(parents=True)
        key = self.key()
        outside = self.root / "outside"
        outside.mkdir()
        sentinel = outside / "sentinel"
        sentinel.write_text("unchanged")
        lock = self.cache / f"{key}.lock"
        lock.symlink_to(sentinel)
        with self.assertRaises(OSError):
            self.build()
        lock.unlink()
        pending = self.cache / f"pending-{key}"
        pending.mkdir()
        artifacts.workspace_state(pending, key, True)
        os.link(sentinel, pending / "object.o")
        with self.assertRaises(ValueError):
            self.build()
        self.assertEqual(sentinel.read_text(), "unchanged")
        linked_cache = self.root / "bin/linked-cache"
        linked_cache.symlink_to(self.cache, target_is_directory=True)
        with patch.dict(os.environ, {artifacts.CACHE_ENV: str(linked_cache)}):
            with self.assertRaises(ValueError):
                self.build()

    def test_invalid_published_artifact_rebuilds_without_mutating_its_inode(self):
        binary = self.build()
        original = binary.read_bytes()
        inode = binary.stat().st_ino
        running = subprocess.Popen([str(binary), "hold"], stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, text=True)
        self.addCleanup(lambda: running.communicate(timeout=5))
        self.addCleanup(running.terminate)
        self.assertTrue(select.select([running.stdout], [], [], 5)[0])
        self.assertEqual(running.stdout.readline(), "holding\n")
        log = binary.parent.parent / "build.log"
        log.unlink()
        outside = self.root / "external-log"
        outside.write_text("g++ -D__NO_MYSQL__")
        log.symlink_to(outside)
        self.assertIsNone(artifacts.verified_artifact(self.cache, self.key()))
        replacement = self.build()
        self.assertNotEqual(replacement, binary)
        self.assertEqual(binary.stat().st_ino, inode)
        self.assertEqual(binary.read_bytes(), original)
        self.assertIsNone(running.poll(), "replacement interrupted an executing published binary")
        self.assertEqual(self.count("a.cpp"), 1)
        self.assertEqual(self.count("b.cpp"), 1)


if __name__ == "__main__":
    unittest.main()
