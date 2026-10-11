#!/usr/bin/env python3
"""Build the whole coordinator and check public Zone Source getter profiles.

Requires Linux GNU13.3/libstdc++13, glibc >=2.27 and the real production Make
flags. Each SQL-header/flat backend runs supported and _GLIBCXX_ASSERTIONS
refusal profiles. Four ordinary whole coordinator Make objects and actual TU
macro probes supply compile-only evidence, with no private getter runtime claim.
Four whole included Zone providers check its two public pure getters; section
GC excludes uncalled operations. No extraction, authority hooks, stubs, services
or game operations are used. Coordinator real birthcaller integration remains
separate and unqualified by this runner.
Missing dependencies or an incompatible profile fail; neither becomes a skip.
"""
import argparse
import json
from pathlib import Path
import signal
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FAMILIES = (
    ("coordinator", "persistence/critical_command_coordinator"),
    ("zone", "world/zone_reset_item_owner"),
)


def main():
    preliminary_parser = argparse.ArgumentParser(add_help=False)
    preliminary_parser.add_argument("--source-root", type=Path, default=ROOT)
    preliminary, _ = preliminary_parser.parse_known_args()
    source = preliminary.source_root.resolve()
    sys.path.insert(0, str(source / "tests/async"))
    from test_item_transfer_complete_bounded import execute, positive_seconds, terminate_on_signal
    from test_original_logit_source_profile import flags_from_make, check_macros
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
    harness = args.proposal_root.resolve() / "tests/async/native_birth_zone_source_profile_test.cpp"
    assert harness.is_file(), harness
    bin_root = source / "bin/tests"
    bin_root.mkdir(parents=True, exist_ok=True)
    probe_timeout = min(60, args.compile_timeout)
    with tempfile.TemporaryDirectory(prefix="native-source-profile-", dir=bin_root) as temporary:
        work = Path(temporary)
        for backend in ("mariadb", "flatfile"):
            for lane, supported in (("supported", True), ("assertions", False)):
                for family, provider in FAMILIES:
                    extra = [] if family == "coordinator" else ["-ffunction-sections", "-fdata-sections"]
                    if not supported:
                        extra.append("-D_GLIBCXX_ASSERTIONS")
                    label = backend + "-" + lane + "-" + family
                    directory = work / label
                    directory.mkdir()
                    objects = directory / "objects"

                    def make_command(target):
                        return [
                            "make", "-C", str(source / "src"), "-j1", "CC=" + args.compiler,
                            "BUILD_PROFILE=production", "PERSISTENCE_BACKEND=" + backend,
                            "OBJDIR=" + str(objects), "DMS_BINARY=" + str(directory / "unused-server"),
                            "EXTRA_CFLAGS=" + " ".join(extra), str(target),
                        ]

                    # Derive exact flags from this provider's actual Make rule.
                    provider_target = objects / (provider + ".o")
                    flags = flags_from_make(make_command(provider_target), args.compiler,
                                            source=source, execute=execute, timeout=probe_timeout)
                    assert "-Og" in flags and "-Werror" in flags and "-D__NO_TESTS__" in flags
                    assert "-DTEST_MUD" not in flags and "-DDURIS_PERSISTENT_TRANSPORT" in flags
                    assert ("-D__NO_MYSQL__" in flags) == (backend == "flatfile")
                    macros = execute([
                        args.compiler, *flags, "-dM", "-E", "-x", "c++",
                        str(source / "src" / (provider + ".c")),
                    ], cwd=source / "src", timeout=probe_timeout, capture_output=True)
                    check_macros(macros, lane)
                    definitions = set(macros.splitlines())
                    assert "#define _GLIBCXX_NATIVE_THREAD_ID pthread_self()" in definitions
                    glibc = {
                        words[1]: int(words[2]) for line in definitions
                        if len(words := line.split()) == 3 and words[1] in ("__GLIBC__", "__GLIBC_MINOR__")
                    }
                    assert (glibc["__GLIBC__"], glibc["__GLIBC_MINOR__"]) >= (2, 27), glibc
                    print(f"native-source {label} profile: " + json.dumps(glibc, sort_keys=True))
                    if family == "coordinator":
                        execute(make_command(provider_target), cwd=source, timeout=args.compile_timeout)
                        assert provider_target.is_file(), provider_target
                        print(f"native-source {label} whole production object: PASS (compile only)")
                        continue
                    fixture = directory / "whole-provider.o"
                    execute([
                        args.compiler, *flags,
                        "-DEXPECT_NATIVE_SOURCE_SUPPORTED=" + str(int(supported)),
                        "-c", str(harness), "-o", str(fixture),
                    ], cwd=source / "src", timeout=args.compile_timeout)
                    executable = directory / "profile-getters"
                    execute([
                        args.compiler, *flags, str(fixture), "-pthread", "-Wl,--gc-sections",
                        *linker_flags, "-o", str(executable),
                    ], cwd=source / "src", timeout=args.compile_timeout)
                    output = execute([str(executable)], cwd=source, timeout=args.run_timeout,
                                     capture_output=True)
                    reports = [json.loads(line.split(" ", 1)[1]) for line in output.splitlines()
                               if line.startswith("NATIVE_SOURCE_PROFILE ")]
                    assert reports == [dict(family=family, supported=supported, getters=2,
                                            null_refusals=2, strong_outputs=2)], reports
                    print(output, end="")
                    print(f"native-source {label}: PASS")
    print("Native birth/Zone Source profiles: PASS (4 coordinator objects, 4 Zone getter runs)")


if __name__ == "__main__":
    main()
