#!/usr/bin/env python3
"""Real whole production bank-admission objects and immutable bank components.

No fake providers, services, fresh ATM owner, or original producer promotion.
Two ordinary admission objects and four backend/profile lanes using all eight
genuine currency/critical/intent/adapter/plan/type/source-event providers.
"""
import argparse
import json
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROFILES = (
    ("production", (), True, True),
    ("supported", ("-ffunction-sections", "-fdata-sections"), True, False),
    ("assertions", ("-D_GLIBCXX_ASSERTIONS", "-ffunction-sections", "-fdata-sections"), False, False),
)
FAMILIES = (
    "economy/economic_command_admission", "economy/currency_command",
    "persistence/critical_command", "economy/economic_currency_adapter",
    "economy/economic_accounting_intent", "economy/economic_accounting_types",
    "economy/economic_source_event", "economy/economic_accounting_plan",
)


def macros(text, lane, openssl=False):
    lines = text.splitlines()
    for required in (
        "#define __GNUC__ 13", "#define __GNUC_MINOR__ 3", "#define _GLIBCXX_RELEASE 13",
        "#define _GLIBCXX_USE_CXX11_ABI 1", "#define __cplusplus 202002L",
        "#define __USE_FORTIFY_LEVEL 3",
    ):
        assert required in lines, required

    def selected(name):
        return any(line == "#define " + name or line.startswith("#define " + name + " ")
                   for line in lines)

    for name in ("NDEBUG", "_GLIBCXX_DEBUG", "_GLIBCXX_PARALLEL", "_GLIBCXX_CONCEPT_CHECKS",
                 "__SANITIZE_ADDRESS__", "__SANITIZE_THREAD__", "OPENSSL_NO_DEPRECATED_3_0"):
        assert not selected(name), (lane, name)
    assert selected("_GLIBCXX_ASSERTIONS") == (lane == "assertions")
    if openssl:
        for required in ("#define OPENSSL_VERSION_MAJOR 3", "#define OPENSSL_VERSION_MINOR 0",
                         "#define OPENSSL_VERSION_PATCH 13"):
            assert required in lines, required
    # MEMCHK is recorded from each actual owning closure below, never injected
    # or assumed to match the pure currency fixture's different header closure.


def main():
    preliminary = argparse.ArgumentParser(add_help=False)
    preliminary.add_argument("--source-root", type=Path, default=ROOT)
    parsed, _ = preliminary.parse_known_args()
    source = parsed.source_root.resolve()
    sys.path.insert(0, str(source / "tests/async"))
    from test_item_transfer_complete_bounded import execute, positive_seconds, terminate_on_signal
    from test_original_logit_source_profile import flags_from_make
    from test_currency_command_build_bounded import currency_macros

    def captured(command, *, cwd, timeout):
        try:
            return execute(command, cwd=cwd, timeout=timeout, capture_output=True)
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as failure:
            for value, stream in ((failure.output, sys.stdout),
                                  (getattr(failure, "stderr", None), sys.stderr)):
                if value:
                    if isinstance(value, bytes):
                        value = value.decode("utf-8", errors="replace")
                    print(value, end="" if value.endswith("\n") else "\n", file=stream, flush=True)
            raise

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--proposal-root", type=Path, default=ROOT)
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--library-path", type=Path, action="append", default=[])
    parser.add_argument("--compile-timeout", type=positive_seconds, default=1200)
    parser.add_argument("--run-timeout", type=positive_seconds, default=180)
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, terminate_on_signal)
    libraries = [path.resolve() for path in args.library_path]
    if any(not path.is_dir() for path in libraries):
        parser.error("Each --library-path must be a reviewed actual linker alias directory")
    link_flags = [flag for path in libraries for flag in ("-L", str(path))]
    fixture = args.proposal_root.resolve() / "tests/async/currency_bank_admission_bounded_test.cpp"
    assert fixture.is_file(), fixture
    bin_root = source / "bin/tests"
    bin_root.mkdir(parents=True, exist_ok=True)
    probe_timeout = min(60, args.compile_timeout)
    with tempfile.TemporaryDirectory(prefix="bank-admission-component-", dir=bin_root) as temporary:
        work = Path(temporary)
        for backend in ("mariadb", "flatfile"):
            for lane, extra, supported, baseline in PROFILES:
                label = backend + "-" + lane
                directory = work / label
                directory.mkdir()
                objects = directory / "objects"
                targets = []
                fixture_flags = None
                families = FAMILIES[:1] if baseline else FAMILIES
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
                    result = captured([
                        args.compiler, *flags, "-dM", "-E", "-x", "c++",
                        str(source / "src" / (family + ".c")),
                    ], cwd=source / "src", timeout=probe_timeout)
                    macros(result, lane, openssl=family in (
                        "economy/economic_command_admission", "economy/currency_command",
                        "economy/economic_accounting_intent"))
                    if family == "economy/currency_command":
                        currency_macros(result, lane)
                        fixture_flags = flags
                    print(f"bank-admission {label} {family} actual macros: " + json.dumps([
                        line for line in result.splitlines() if line.startswith((
                            "#define MEMCHK ", "#define _GLIBCXX_ASSERTIONS ",
                            "#define _GLIBCXX_CONCEPT_CHECKS ", "#define OPENSSL_VERSION_MAJOR ",
                            "#define OPENSSL_VERSION_MINOR ", "#define OPENSSL_VERSION_PATCH "))]))
                    execute(command, cwd=source, timeout=args.compile_timeout)
                    assert target.is_file(), target
                    targets.append(target)
                if baseline:
                    print(f"bank-admission {label} whole production admission object: PASS")
                    continue
                assert fixture_flags is not None
                fixture_macros = captured([
                    args.compiler, *fixture_flags, "-DEXPECT_BANK_ADMISSION_SUPPORTED=" + str(int(supported)),
                    "-dM", "-E", "-x", "c++", str(fixture),
                ], cwd=source / "src", timeout=probe_timeout)
                currency_macros(fixture_macros, lane)
                macros(fixture_macros, lane, openssl=True)
                executable = directory / "bank-admission-component"
                execute([
                    args.compiler, *fixture_flags,
                    "-DEXPECT_BANK_ADMISSION_SUPPORTED=" + str(int(supported)),
                    "-Wl,--gc-sections", str(fixture), *map(str, targets), "-pthread",
                    *link_flags, "-l:libcrypto.so.3", "-o", str(executable),
                ], cwd=source / "src", timeout=args.compile_timeout)
                output = captured([str(executable)], cwd=source, timeout=args.run_timeout)
                reports = [json.loads(line.split(" ", 1)[1]) for line in output.splitlines()
                           if line.startswith("BANK_ADMISSION_COMPONENT ")]
                assert len(reports) == 1, reports
                report = reports[0]
                assert report["supported"] is supported and report["wire_bytes"] == 136
                if supported:
                    assert 0 < report["denied_ordinals"] <= 2048
                    assert report["cases"] >= 80 + report["denied_ordinals"]
                else:
                    assert report["cases"] == 2 and report["denied_ordinals"] == 0
                print(output, end="")
                print(f"bank-admission {label} genuine provider component: PASS")
    print("Currency bank admission: PASS (34 whole objects, 4 component runs)")


if __name__ == "__main__":
    main()
