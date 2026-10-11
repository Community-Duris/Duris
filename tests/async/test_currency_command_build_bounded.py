#!/usr/bin/env python3
"""Whole production currency objects and genuine bounded-builder regressions.

Two ordinary currency production objects plus four backend/profile component
lanes with genuine whole currency/critical providers. Uses no server or services.
Requires the actual GNU13.3/OpenSSL3.0.13 production profile. Assertions must
refuse strongly. This does not select or qualify the original ATM producer.
"""
import argparse
import json
from pathlib import Path
import signal
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROFILES = (
    ("production", (), True, True),
    ("supported", ("-ffunction-sections", "-fdata-sections"), True, False),
    ("assertions", ("-D_GLIBCXX_ASSERTIONS", "-ffunction-sections", "-fdata-sections"), False, False),
)


def currency_macros(text, lane):
    lines = text.splitlines()
    for required in (
        "#define __GNUC__ 13", "#define __GNUC_MINOR__ 3", "#define _GLIBCXX_RELEASE 13",
        "#define _GLIBCXX_USE_CXX11_ABI 1", "#define __cplusplus 202002L",
        "#define __USE_FORTIFY_LEVEL 3", "#define OPENSSL_VERSION_MAJOR 3",
        "#define OPENSSL_VERSION_MINOR 0", "#define OPENSSL_VERSION_PATCH 13",
    ):
        assert required in lines, required

    def selected(name):
        return any(line == "#define " + name or line.startswith("#define " + name + " ")
                   for line in lines)

    for macro in ("NDEBUG", "_GLIBCXX_DEBUG", "_GLIBCXX_PARALLEL", "_GLIBCXX_CONCEPT_CHECKS",
                  "__SANITIZE_ADDRESS__", "OPENSSL_NO_DEPRECATED_3_0"):
        assert not selected(macro), (lane, macro)
    assert selected("_GLIBCXX_ASSERTIONS") == (lane == "assertions")
    # The actual pure currency header closure excludes core/config.h. Do not
    # borrow Utility's owning-TU MEMCHK requirement or inject config.h in a probe.
    assert not selected("MEMCHK"), (lane, "currency owning closure")


def main():
    preliminary_parser = argparse.ArgumentParser(add_help=False)
    preliminary_parser.add_argument("--source-root", type=Path, default=ROOT)
    preliminary, _ = preliminary_parser.parse_known_args()
    source = preliminary.source_root.resolve()
    sys.path.insert(0, str(source / "tests/async"))
    from test_item_transfer_complete_bounded import execute, positive_seconds, terminate_on_signal
    from test_original_logit_source_profile import flags_from_make

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
    harness = args.proposal_root.resolve() / "tests/async/currency_command_build_bounded_test.cpp"
    assert harness.is_file(), harness
    bin_root = source / "bin/tests"
    bin_root.mkdir(parents=True, exist_ok=True)
    probe_timeout = min(60, args.compile_timeout)
    with tempfile.TemporaryDirectory(prefix="currency-build-component-", dir=bin_root) as temporary:
        work = Path(temporary)
        for backend in ("mariadb", "flatfile"):
            for lane, extra, supported, baseline in PROFILES:
                label = backend + "-" + lane
                directory = work / label
                directory.mkdir()
                objects = directory / "objects"
                families = ("economy/currency_command",) if baseline else (
                    "economy/currency_command", "persistence/critical_command")
                targets = []
                currency_flags = None
                for family in families:
                    target = objects / (family + ".o")
                    command = [
                        "make", "-C", str(source / "src"), "-j1", "CC=" + args.compiler,
                        "BUILD_PROFILE=production", "PERSISTENCE_BACKEND=" + backend,
                        "OBJDIR=" + str(objects), "DMS_BINARY=" + str(directory / "unused-server"),
                        "EXTRA_CFLAGS=" + " ".join(extra), str(target),
                    ]
                    flags = flags_from_make(command, args.compiler, source=source,
                                            execute=execute, timeout=probe_timeout)
                    assert "-Og" in flags and "-Werror" in flags and "-D__NO_TESTS__" in flags
                    assert "-DTEST_MUD" not in flags and "-DDURIS_PERSISTENT_TRANSPORT" in flags
                    assert ("-D__NO_MYSQL__" in flags) == (backend == "flatfile")
                    macros = execute([
                        args.compiler, *flags, "-dM", "-E", "-x", "c++",
                        str(source / "src" / (family + ".c")),
                    ], cwd=source / "src", timeout=probe_timeout, capture_output=True)
                    if family == "economy/currency_command":
                        currency_macros(macros, lane)
                        currency_flags = flags
                    print(f"currency-build {label} {family} actual macros: " + json.dumps(
                        [line for line in macros.splitlines() if line.startswith((
                            "#define MEMCHK ", "#define _GLIBCXX_ASSERTIONS ",
                            "#define OPENSSL_VERSION_MAJOR ", "#define OPENSSL_VERSION_MINOR ",
                            "#define OPENSSL_VERSION_PATCH "))], sort_keys=True))
                    execute(command, cwd=source, timeout=args.compile_timeout)
                    assert target.is_file(), target
                    targets.append(target)
                assert currency_flags is not None
                if baseline:
                    print(f"currency-build {label} whole production currency object: PASS")
                    continue
                fixture_macros = execute([
                    args.compiler, *currency_flags, "-DEXPECT_CURRENCY_BUILD_SUPPORTED=" + str(int(supported)),
                    "-dM", "-E", "-x", "c++", str(harness),
                ], cwd=source / "src", timeout=probe_timeout, capture_output=True)
                currency_macros(fixture_macros, lane)
                executable = directory / "currency-build-component"
                execute([
                    args.compiler, *currency_flags,
                    "-DEXPECT_CURRENCY_BUILD_SUPPORTED=" + str(int(supported)),
                    "-Wl,--gc-sections", str(harness), *map(str, targets), "-pthread",
                    *linker_flags, "-l:libcrypto.so.3", "-o", str(executable),
                ], cwd=source / "src", timeout=args.compile_timeout)
                output = execute([str(executable)], cwd=source, timeout=args.run_timeout,
                                 capture_output=True)
                reports = [json.loads(line.split(" ", 1)[1]) for line in output.splitlines()
                           if line.startswith("CURRENCY_BUILD_COMPONENT ")]
                assert len(reports) == 1, reports
                report = reports[0]
                assert report["supported"] is supported and report["wire_bytes"] == 136
                if supported:
                    assert 0 < report["denied_ordinals"] <= 256
                    assert report["cases"] >= 35 + report["denied_ordinals"]
                else:
                    assert report["cases"] == 4 and report["denied_ordinals"] == 0
                print(output, end="")
                print(f"currency-build {label} genuine provider component: PASS")
    print("Currency bounded builder: PASS (10 whole objects, 4 component runs)")


if __name__ == "__main__":
    main()
