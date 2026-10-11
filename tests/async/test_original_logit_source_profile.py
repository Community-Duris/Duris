#!/usr/bin/env python3
"""Compile whole production utility objects and test three passive Source getters.

Two unsectioned SQL-header/flat production objects are compiled without runtime
claims. Four sectioned whole objects test the supported profile and genuine
_GLIBCXX_ASSERTIONS refusal in each backend. No logger, server, SQL or Redis runs.
Requires the supported Linux GNU13.3/libstdc++13 production profile; a missing or
incompatible profile fails. Optional library paths must be caller-reviewed.
"""
import argparse
import json
from pathlib import Path
import shlex
import signal
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROFILES = (
    ("production", (), True, True),
    ("supported", ("-ffunction-sections", "-fdata-sections"), True, False),
    ("assertions", ("-D_GLIBCXX_ASSERTIONS", "-ffunction-sections", "-fdata-sections"), False, False),
)


def flags_from_make(command, compiler, *, source, execute, timeout):
    dry = execute([command[0], "-n", *command[1:]], cwd=source, timeout=timeout,
                  capture_output=True)
    commands = [shlex.split(line) for line in dry.splitlines()]
    commands = [words for words in commands if words and words[0] == compiler and "-c" in words]
    assert len(commands) == 1, dry
    words = commands[0]
    flags = []
    index = 1
    while index < len(words):
        if words[index] in ("-c", "-o"):
            index += 2
        elif words[index] in ("-MMD", "-MP"):
            index += 1
        else:
            flags.append(words[index])
            index += 1
    return flags


def check_macros(text, lane):
    for definition in ("#define __GNUC__ 13", "#define __GNUC_MINOR__ 3",
                       "#define _GLIBCXX_RELEASE 13", "#define MEMCHK 1",
                       "#define __USE_FORTIFY_LEVEL 3"):
        assert definition in text.splitlines(), definition
    assert not any(line == "#define NDEBUG" or line.startswith("#define NDEBUG ")
                   for line in text.splitlines()), "Fixture assertions must be enabled"
    for macro in ("_GLIBCXX_ASSERTIONS", "_GLIBCXX_CONCEPT_CHECKS", "__SANITIZE_ADDRESS__"):
        selected = macro == "_GLIBCXX_ASSERTIONS" and lane == "assertions"
        assert ("#define " + macro + " " in text) == selected, (lane, macro)


def main():
    source_parser = argparse.ArgumentParser(add_help=False)
    source_parser.add_argument("--source-root", type=Path, default=ROOT)
    preliminary, _ = source_parser.parse_known_args()
    source = preliminary.source_root.resolve()
    sys.path.insert(0, str(source / "tests/async"))
    # Reuse the maintained positive deadline parser and owned process-group
    # cleanup, including deferred SIGTERM across process creation and reaping.
    from test_item_transfer_complete_bounded import execute, positive_seconds, terminate_on_signal
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--proposal-root", type=Path, default=ROOT)
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--library-path", type=Path, action="append", default=[])
    parser.add_argument("--compile-timeout", type=positive_seconds, default=1200)
    parser.add_argument("--run-timeout", type=positive_seconds, default=180)
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, terminate_on_signal)
    paths = [path.resolve() for path in args.library_path]
    if any(not path.is_dir() for path in paths):
        parser.error("Each --library-path must name an existing reviewed directory")
    linker_flags = [flag for path in paths for flag in ("-L", str(path))]
    harness = args.proposal_root.resolve() / "tests/async/original_logit_source_profile_test.cpp"
    assert harness.is_file(), harness
    bin_root = source / "bin/tests"
    bin_root.mkdir(parents=True, exist_ok=True)
    probe_timeout = min(60, args.compile_timeout)
    with tempfile.TemporaryDirectory(prefix="original-logit-profile-", dir=bin_root) as temporary:
        work = Path(temporary)
        control = work / "profile-control.cpp"
        control.write_text('#include "core/utility.h"\n#include <thread>\n#include <sys/stat.h>\n#include <time.h>\n',
                           encoding="utf-8")
        for backend in ("mariadb", "flatfile"):
            for lane, extra, supported, baseline in PROFILES:
                label = backend + "-" + lane
                directory = work / label
                directory.mkdir()
                objects = directory / "objects"
                target = objects / "core/utility.o"
                command = [
                    "make", "-C", str(source / "src"), "-j1", "CC=" + args.compiler,
                    "BUILD_PROFILE=production", "PERSISTENCE_BACKEND=" + backend,
                    "OBJDIR=" + str(objects), "DMS_BINARY=" + str(directory / "unused-server"),
                    "EXTRA_CFLAGS=" + " ".join(extra), str(target),
                ]
                flags = flags_from_make(command, args.compiler, source=source, execute=execute,
                                        timeout=probe_timeout)
                assert "-Og" in flags and "-Werror" in flags and "-D__NO_TESTS__" in flags
                assert "-DTEST_MUD" not in flags and "-DDURIS_PERSISTENT_TRANSPORT" in flags
                assert ("-D__NO_MYSQL__" in flags) == (backend == "flatfile")
                macros = execute([args.compiler, *flags, "-dM", "-E", "-x", "c++", str(control)],
                                 cwd=source / "src", timeout=probe_timeout, capture_output=True)
                check_macros(macros, lane)
                utility_macros = execute([
                    args.compiler, *flags, "-dM", "-E", "-x", "c++", str(source / "src/core/utility.c"),
                ], cwd=source / "src", timeout=probe_timeout, capture_output=True)
                check_macros(utility_macros, lane)
                native_prefix = "#define _GLIBCXX_NATIVE_THREAD_ID "
                control_native = next((line for line in macros.splitlines() if line.startswith(native_prefix)), None)
                utility_native = next((line for line in utility_macros.splitlines() if line.startswith(native_prefix)), None)
                # Keep the genuine definition/path, including include-order
                # differences; never undefine it or infer an absent family.
                print(f"original-logit {label} thread macros: " + json.dumps(
                    dict(control=control_native, utility=utility_native), sort_keys=True))
                execute(command, cwd=source, timeout=args.compile_timeout)
                assert target.is_file(), target
                if baseline:
                    print(f"original-logit {label} whole production object: PASS")
                    continue
                executable = directory / "profile-getters"
                execute([
                    args.compiler, *flags, "-DEXPECT_ORIGINAL_LOGIT_SOURCE_SUPPORTED=" + str(int(supported)),
                    "-Wl,--gc-sections", str(harness), str(target), "-pthread", *linker_flags,
                    "-o", str(executable),
                ], cwd=source / "src", timeout=args.compile_timeout)
                output = execute([str(executable)], cwd=source, timeout=args.run_timeout,
                                 capture_output=True)
                reports = [json.loads(line.split(" ", 1)[1]) for line in output.splitlines()
                           if line.startswith("UTILITY2_PROFILE_GETTERS ")]
                assert reports == [dict(supported=supported, getters=3, null_refusals=3, strong_outputs=3)], reports
                print(output, end="")
                print(f"original-logit {label} getter profile: PASS")
    print("original-logit Source profile: PASS (6 whole objects, 4 getter runs)")


if __name__ == "__main__":
    main()
