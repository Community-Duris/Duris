#!/usr/bin/env python3
"""Run passive binding and complete Birth profiles through whole production providers.

Requires the qualified Linux GNU13.3 toolchain. SQL-header and flatfile builds
each exercise supported and assertions-refusal profiles, disjoint INITIAL, strong outputs and
errno preservation. No database, Redis, world operation or authority is used.
Declared-source PASS does not establish emitted/native stack, nonempty native construction,
or gameplay/lifecycle acceptance.
"""
import argparse
from contextlib import contextmanager
import json
from pathlib import Path
import signal
import shutil
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


@contextmanager
def owned_artifacts(parent):
    temporary = Path(tempfile.mkdtemp(prefix="native-binding-profile-", dir=parent))
    try:
        yield temporary
    except BaseException as failure:
        output = getattr(failure, "output", None)
        stderr = getattr(failure, "stderr", None)
        for captured in (output, stderr if stderr != output else None):
            if captured:
                if isinstance(captured, bytes):
                    captured = captured.decode(errors="replace")
                print(captured, file=sys.stderr, end="" if captured.endswith("\n") else "\n", flush=True)
        print(f"Retained native profile failure artifacts: {temporary}", file=sys.stderr, flush=True)
        raise
    else:
        shutil.rmtree(temporary)


def main():
    source_parser = argparse.ArgumentParser(add_help=False)
    source_parser.add_argument("--source-root", type=Path, default=ROOT)
    preliminary, _ = source_parser.parse_known_args()
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
    harness = args.proposal_root.resolve() / "tests/async/native_birth_binding_source_profile_test.cpp"
    assert harness.is_file(), harness
    parent = source / "bin/tests"
    parent.mkdir(parents=True, exist_ok=True)
    with owned_artifacts(parent) as temporary:
        for backend in ("mariadb", "flatfile"):
            for lane, supported in (("supported", True), ("assertions", False),
                                    ("old-glibc-predicate", False)):
                directory = Path(temporary) / (backend + "-" + lane)
                directory.mkdir()
                objects = directory / "objects"
                provider = objects / "world/db.o"
                extra = ["-ffunction-sections", "-fdata-sections"]
                if lane == "assertions":
                    extra.append("-D_GLIBCXX_ASSERTIONS")
                if lane == "old-glibc-predicate":
                    # Exercise the actual refusal predicate using the installed
                    # headers, not compatibility with an older glibc runtime.
                    gate = directory / "old-glibc-predicate.h"
                    gate.write_text('#include <bits/stdc++.h>\n#include <features.h>\n'
                                    '#undef __GLIBC__\n#undef __GLIBC_MINOR__\n'
                                    '#define __GLIBC__ 2\n#define __GLIBC_MINOR__ 26\n')
                    extra += ["-include", str(gate)]
                make = ["make", "-C", str(source / "src"), "-j1", "CC=" + args.compiler,
                        "BUILD_PROFILE=production", "PERSISTENCE_BACKEND=" + backend,
                        "OBJDIR=" + str(objects), "DMS_BINARY=" + str(directory / "unused-server"),
                        "EXTRA_CFLAGS=" + " ".join(extra), str(provider)]
                flags = flags_from_make(make, args.compiler, source=source, execute=execute,
                                        timeout=min(60, args.compile_timeout))
                assert "-Og" in flags and "-Werror" in flags and "-D__NO_TESTS__" in flags
                assert "-DTEST_MUD" not in flags and "-DDURIS_PERSISTENT_TRANSPORT" in flags
                assert ("-D__NO_MYSQL__" in flags) == (backend == "flatfile")
                macros = execute([args.compiler, *flags, "-dM", "-E", "-x", "c++",
                                  str(source / "src/world/db.c")], cwd=source / "src",
                                 timeout=min(60, args.compile_timeout), capture_output=True)
                check_macros(macros, lane)
                if lane == "old-glibc-predicate":
                    assert "#define __GLIBC__ 2" in macros.splitlines()
                    assert "#define __GLIBC_MINOR__ 26" in macros.splitlines()
                # Compile the provider as its actual production translation
                # unit. Including db.c in the fixture changes GCC's linkage
                # diagnostics for existing internal constructor/parser types.
                db_object = directory / "db-provider.o"
                execute([args.compiler, *flags, "-c", str(source / "src/world/db.c"),
                         "-o", str(db_object)], cwd=source / "src", timeout=args.compile_timeout)
                whole = directory / "binding-fixture.o"
                execute([args.compiler, *flags,
                         "-DEXPECT_NATIVE_SOURCE_SUPPORTED=" + str(int(supported)),
                         "-c", str(harness), "-o", str(whole)], cwd=source / "src",
                        timeout=args.compile_timeout)
                binary = directory / "binding-getters"
                execute([args.compiler, *flags, str(whole), str(db_object), "-pthread", "-Wl,--gc-sections",
                         *linker_flags, "-o", str(binary)], cwd=source / "src",
                        timeout=args.compile_timeout)
                output = execute([str(binary)], cwd=source, timeout=args.run_timeout,
                                 capture_output=True)
                reports = [json.loads(line.split(" ", 1)[1]) for line in output.splitlines()
                           if line.startswith("NATIVE_BINDING_SOURCE_PROFILE ")]
                assert reports == [dict(supported=supported, source=274 if supported else 0,
                                        query=73 if supported else (1 << 64) - 1,
                                        strong_outputs=2, null_refusals=1)], reports
                retention_reports = [json.loads(line.split(" ", 1)[1]) for line in output.splitlines()
                                     if line.startswith("NATIVE_DB_RETENTION_PROFILE ")]
                assert retention_reports == [dict(supported=supported, getters=3, queries=3,
                                                  null_refusals=3, strong_outputs=6,
                                                  initial=32 if supported else 0)], retention_reports
                print(f"native-binding {backend}-{lane}: " + output, end="")
                # Real complete DB and shared ROOM providers satisfy Birth's
                # passive child queries; no synthesized child or query stub.
                room_object = directory / "room-provider.o"
                execute([args.compiler, *flags, "-c",
                         str(source / "src/world/zone_reset_room_publication.c"),
                         "-o", str(room_object)], cwd=source / "src", timeout=args.compile_timeout)
                birth_object = directory / "birth-provider.o"
                execute([args.compiler, *flags, "-c",
                         str(source / "src/world/quest_mobile_native_birth.c"),
                         "-o", str(birth_object)], cwd=source / "src", timeout=args.compile_timeout)
                # Birth's genuine resident registry destroys its retained
                # binding members even when the passive query created no birth.
                # Link the original chain reset/destructor from its real owner.
                chain_object = directory / "procedure-chain-provider.o"
                chain_target = objects / "mob/studioproclib.o"
                chain_make = [*make[:-1], str(chain_target)]
                chain_flags = flags_from_make(chain_make, args.compiler, source=source,
                                              execute=execute,
                                              timeout=min(60, args.compile_timeout))
                assert "-Og" in chain_flags and "-Werror" in chain_flags
                execute([args.compiler, *chain_flags, "-c",
                         str(source / "src/mob/studioproclib.c"), "-o", str(chain_object)],
                        cwd=source / "src", timeout=args.compile_timeout)
                birth_fixture = directory / "birth-fixture.o"
                execute([args.compiler, *flags, "-DTEST_NATIVE_BIRTH_CURRENT_PROFILE",
                         "-DEXPECT_NATIVE_SOURCE_SUPPORTED=" + str(int(supported)),
                         "-c", str(harness), "-o", str(birth_fixture)], cwd=source / "src",
                        timeout=args.compile_timeout)
                birth_binary = directory / "birth-getters"
                execute([args.compiler, *flags, str(birth_fixture), str(birth_object), str(db_object), str(room_object), str(chain_object),
                         "-pthread", "-Wl,--gc-sections", *linker_flags, "-o", str(birth_binary)],
                        cwd=source / "src", timeout=args.compile_timeout)
                birth_output = execute([str(birth_binary)], cwd=source, timeout=args.run_timeout,
                                       capture_output=True)
                birth_reports = [json.loads(line.split(" ", 1)[1]) for line in birth_output.splitlines()
                                 if line.startswith("NATIVE_BIRTH_CURRENT_PROFILE ")]
                assert birth_reports == [dict(supported=supported, getters=2, queries=2,
                                              null_refusals=2, strong_outputs=4,
                                              initial=32 if supported else 0)], birth_reports
                print(f"native-birth {backend}-{lane}: " + birth_output, end="")
    print("Native binding/Birth Source profiles: PASS (12 whole production provider runs; "
          "old-glibc lanes test refusal predicates only)")


if __name__ == "__main__":
    main()
