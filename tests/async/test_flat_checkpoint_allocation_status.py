#!/usr/bin/env python3
"""Test the genuine fixed checkpoint allocation status in both header backends.

Requires the granted GNU13.3/libstdc++13/OpenSSL3.0.13 native queue. Whole
providers use original production Make flags. ASan/UBSan has a genuine refusal
lane. Synthetic checkpoint values confer no native origin or publication proof.
BIN_ROOT selects task artifacts; commands, macros and failures are retained.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import tempfile
import time

from test_item_transfer_complete_bounded import execute, positive_seconds, terminate_on_signal
from test_original_logit_source_profile import flags_from_make

ROOT = Path(__file__).resolve().parents[2]
PROVIDERS = ("flatfile/flatfile_shopkeeper_repository.c", "player/player_snapshot_codec.c")
FIXTURE = "tests/async/flat_checkpoint_allocation_status_test.cpp"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--compile-timeout", type=positive_seconds, default=180)
    parser.add_argument("--run-timeout", type=positive_seconds, default=20)
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, terminate_on_signal)
    inputs = [*(ROOT / "src" / name for name in PROVIDERS), ROOT / FIXTURE,
              ROOT / "src/flatfile/flatfile_shopkeeper_repository.h",
              Path(__file__).resolve()]
    pinned = {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
              for path in inputs}
    parent = Path(os.environ.get("BIN_ROOT", str(ROOT / "bin"))).resolve() / "tests"
    parent.mkdir(parents=True, exist_ok=True)
    scratch = Path(tempfile.mkdtemp(prefix="flat-checkpoint-allocation-status-", dir=parent))
    try:
        for backend in ("mariadb", "flatfile"):
            for lane in ("production", "sanitizer"):
                directory = scratch / (backend + "-" + lane)
                directory.mkdir()
                extra = ["-ffunction-sections", "-fdata-sections"]
                if lane == "sanitizer":
                    extra += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                              "-fno-pie", "-no-pie"]
                target = directory / "objects/flatfile/flatfile_shopkeeper_repository.o"
                make = ["make", "-C", str(ROOT / "src"), "-j1", "CC=" + args.compiler,
                        "BUILD_PROFILE=production", "PERSISTENCE_BACKEND=" + backend,
                        "OBJDIR=" + str(directory / "objects"),
                        "DMS_BINARY=" + str(directory / "unused-server"),
                        "EXTRA_CFLAGS=" + " ".join(extra), str(target)]
                flags = flags_from_make(make, args.compiler, source=ROOT, execute=execute,
                                        timeout=min(60, args.compile_timeout))
                assert "-Og" in flags and "-Werror" in flags and "-D__NO_TESTS__" in flags
                assert ("-D__NO_MYSQL__" in flags) == (backend == "flatfile")
                macros = execute([args.compiler, *flags, "-dM", "-E", "-x", "c++",
                                  str(ROOT / "src" / PROVIDERS[0])], cwd=ROOT / "src",
                                 timeout=min(60, args.compile_timeout), capture_output=True)
                definitions = set(macros.splitlines())
                for required in ("#define __GNUC__ 13", "#define __GNUC_MINOR__ 3",
                                 "#define _GLIBCXX_RELEASE 13", "#define OPENSSL_VERSION_MAJOR 3",
                                 "#define OPENSSL_VERSION_MINOR 0", "#define OPENSSL_VERSION_PATCH 13"):
                    assert required in definitions, required
                assert not any(line == "#define NDEBUG" or line.startswith("#define NDEBUG ")
                               for line in definitions), "Assertions must remain enabled"
                assert ("#define __SANITIZE_ADDRESS__ 1" in definitions) == (lane == "sanitizer")
                (directory / "macros.txt").write_text(macros, encoding="utf-8")
                supported = lane == "production"
                sources = [*(ROOT / "src" / name for name in PROVIDERS), ROOT / FIXTURE]
                objects = [directory / (str(index) + ".o") for index in range(len(sources))]
                binary = directory / "checkpoint-status"
                commands = [[args.compiler, *flags,
                             "-DEXPECT_FLAT_CHECKPOINT_SOURCE_SUPPORTED=" + str(int(supported)),
                             "-c", str(source), "-o", str(obj)]
                            for source, obj in zip(sources, objects)]
                commands.append([args.compiler, *flags, *(str(obj) for obj in objects),
                                 "-pthread", "-lcrypto", "-Wl,--gc-sections",
                                 "-Wl,--wrap=_Znwm", "-o", str(binary)])
                (directory / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
                deadline = time.monotonic() + args.compile_timeout
                for command in commands:
                    remaining = deadline - time.monotonic()
                    if remaining <= 0:
                        raise TimeoutError("Checkpoint aggregate compile/link deadline exceeded")
                    execute(command, cwd=ROOT / "src", timeout=remaining)
                output = execute([str(binary)], cwd=ROOT, timeout=args.run_timeout,
                                 env=dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                                          UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1"),
                                 capture_output=True)
                expected = ("FLAT_CHECKPOINT_ALLOCATION_STATUS supported=1 allocation_failure=1 strong_output=1"
                            if supported else
                            "FLAT_CHECKPOINT_ALLOCATION_STATUS supported=0 refusal_only=1")
                assert output.strip() == expected, output
                (directory / "output.txt").write_text(output, encoding="utf-8")
                print(backend + "-" + lane + ": " + output, end="", flush=True)
        assert pinned == {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                          for path in inputs}, "Checkpoint inputs changed during qualification"
        (scratch / "receipt.json").write_text(json.dumps(pinned, indent=2) + "\n")
    except BaseException as failure:
        captured = getattr(failure, "output", None)
        if captured:
            print(captured, flush=True)
        print("Retained checkpoint failure evidence: " + str(scratch), flush=True)
        raise
    print("Retained checkpoint evidence: " + str(scratch))


if __name__ == "__main__":
    main()
