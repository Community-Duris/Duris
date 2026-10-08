"""Real native builds exercise reuse, invalidation and stable publication."""

from contextlib import redirect_stdout
import io
import multiprocessing
import os
from pathlib import Path
import subprocess
import tempfile
import time
import unittest
from unittest.mock import patch

import native_build_artifacts as artifacts


class NativeArtifacts(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="native-artifact-test-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        (self.root / "src").mkdir()
        (self.root / "tests/async").mkdir(parents=True)
        self.header = self.root / "src/value.h"
        self.header.write_text("#define VALUE 42\n")
        (self.root / "src/value.cpp").write_text('#include "value.h"\nint value() { return VALUE; }\n')
        for name in ("first", "second"):
            (self.root / f"tests/async/{name}.cpp").write_text(
                "#include <cstdio>\nint value(); int main() { printf(\"%d\\n\", value()); }\n")
        self.flags = ["-std=c++20", "-Wall", "-Werror", "-Isrc"]
        self.links = []
        self.environment = patch.dict(os.environ, {
            artifacts.server.CACHE_ENV: str(self.root / "bin/cache"), "DURIS_NATIVE_BUILD_JOBS": "2"})
        self.environment.start()
        self.addCleanup(self.environment.stop)
        for module in (artifacts, artifacts.server):
            replacement = patch.object(module, "ROOT", self.root)
            replacement.start()
            self.addCleanup(replacement.stop)
        # Installed-toolchain byte sensitivity is exercised by the existing
        # server-artifact fixture. Keep these real compiler/process tests small.
        self.tools = patch.object(artifacts.server, "toolchain_key", return_value="fixture-toolchain")
        self.tools.start()
        self.addCleanup(self.tools.stop)

    def build(self, name="first", flags=None, compiler="g++"):
        output = io.StringIO()
        with redirect_stdout(output):
            path = artifacts.build_native(self.root / f"bin/private/{name}",
                ["src/value.cpp", f"tests/async/{name}.cpp"], flags or self.flags, self.links,
                compiler=compiler, name=name)
        return path, output.getvalue()

    def value(self, binary):
        return subprocess.check_output([str(binary)], text=True).strip()

    def test_real_objects_are_shared_but_runtime_inputs_are_private(self):
        first, _ = self.build()
        second, output = self.build("second")
        self.assertEqual(self.value(first), "42")
        self.assertEqual(self.value(second), "42")
        self.assertIn("objects_reused=1", output)
        reused, output = self.build()
        self.assertEqual(first, reused)
        self.assertIn("NATIVE_BUILD reused", output)
        self.assertFalse(first.stat().st_mode & 0o222)
        self.assertFalse((self.root / "bin/private").exists())
        self.assertEqual(len(list((self.root / "bin/cache/native/objects").glob("*.o"))), 3)

    def test_header_content_flags_toolchain_and_corruption_invalidate(self):
        first, _ = self.build()
        timestamp = self.header.stat()
        self.header.write_text("#define VALUE 73\n")
        os.utime(self.header, ns=(timestamp.st_atime_ns, timestamp.st_mtime_ns))
        second, _ = self.build()
        self.assertNotEqual(first, second)
        self.assertEqual(self.value(second), "73")
        third, _ = self.build(flags=self.flags + ["-DVARIANT=1"])
        self.assertNotEqual(second, third)
        with patch.object(artifacts.server, "toolchain_key", return_value="changed-toolchain"):
            fourth, _ = self.build()
        self.assertNotEqual(second, fourth)
        second.chmod(0o755)
        second.write_bytes(b"damaged")
        second.chmod(0o555)
        repaired, _ = self.build()
        self.assertNotEqual(second, repaired)
        self.assertEqual(self.value(repaired), "73")
        self.assertEqual(second.read_bytes(), b"damaged")

    def test_failed_input_stability_does_not_poison_reverted_build(self):
        original_command = artifacts.command
        changed = False

        def change_input(arguments, environment):
            nonlocal changed
            if not changed:
                changed = True
                self.header.write_text("#define VALUE 99\n")
            return original_command(arguments, environment)

        with patch.object(artifacts, "command", side_effect=change_input):
            with self.assertRaisesRegex(RuntimeError, "inputs changed"):
                self.build()
        self.header.write_text("#define VALUE 42\n")
        binary, _ = self.build()
        self.assertEqual(self.value(binary), "42", "unqualified objects poisoned the original key")

    def test_failed_compiler_and_cache_off_preserve_standalone_behavior(self):
        with patch.object(artifacts, "command", side_effect=AssertionError("compile refused")):
            with self.assertRaisesRegex(AssertionError, "compile refused"):
                self.build()
        self.assertFalse(list((self.root / "bin/cache/native/binaries").glob("*.json")))
        with patch.dict(os.environ, {artifacts.server.CACHE_ENV: "off"}):
            binary, output = self.build()
        self.assertEqual(binary, self.root / "bin/private/first")
        self.assertEqual(self.value(binary), "42")
        self.assertIn("NATIVE_BUILD built", output)

    @unittest.skipUnless(os.name == "posix", "requires an isolated forked builder")
    def test_killed_build_cannot_publish_objects_for_changed_inputs(self):
        context = multiprocessing.get_context("fork")
        marker = self.root / "second-command"
        original_command = artifacts.command

        def interrupted_build():
            count = 0

            def pause(arguments, environment):
                nonlocal count
                count += 1
                if count == 1:
                    self.header.write_text("#define VALUE 99\n")
                if count == 2:
                    marker.write_text("first object compiled with changed header")
                    while True:
                        time.sleep(0.01)
                return original_command(arguments, environment)

            with (patch.object(artifacts, "command", side_effect=pause),
                  patch.dict(os.environ, {"DURIS_NATIVE_BUILD_JOBS": "1"})):
                self.build()

        child = context.Process(target=interrupted_build)
        child.start()
        try:
            deadline = time.monotonic() + 10
            while not marker.exists() and time.monotonic() < deadline and child.is_alive():
                time.sleep(0.01)
            self.assertTrue(marker.exists(), "builder did not reach the publication boundary")
        finally:
            if child.is_alive():
                child.kill()
            child.join(timeout=5)
        self.header.write_text("#define VALUE 42\n")
        binary, _ = self.build()
        self.assertEqual(self.value(binary), "42", "killed builder left a poisoned object manifest")

    @unittest.skipUnless(os.name == "posix", "requires process-shared POSIX file locks")
    def test_concurrent_builds_publish_one_verified_binary(self):
        context = multiprocessing.get_context("fork")
        results = context.Queue()

        def build():
            try:
                results.put((str(self.build()[0]), None))
            except BaseException as error:
                results.put((None, str(error)))

        children = [context.Process(target=build) for _ in range(3)]
        for child in children:
            child.start()
        replies = [results.get(timeout=20) for child in children]
        for child in children:
            child.join(timeout=20)
            self.assertEqual(child.exitcode, 0)
        self.assertTrue(all(error is None for _, error in replies), replies)
        self.assertEqual(len({path for path, _ in replies}), 1)
        self.assertEqual(len(list((self.root / "bin/cache/native/binaries").glob("*.json"))), 1)
        self.assertEqual(self.value(Path(replies[0][0])), "42")


if __name__ == "__main__":
    unittest.main()
