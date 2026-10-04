#!/usr/bin/env python3
"""Compile and run the focused #265 gameplay-adapter journey."""

from pathlib import Path
import argparse
import shlex
import subprocess
import tempfile

from test_telemetry_combat_hooks import function

ROOT = Path(__file__).resolve().parents[2]


def compile_gameplay(executable: Path, *, sanitize: bool = False, native_sql: bool = False) -> None:
    # Execute the maintained helper bodies with the game-service seams in the
    # existing harness; the actual runtime/worker/writer remain linked below.
    source = (ROOT / "src/magic/affects.c").read_text()
    control_helpers = executable.parent / "telemetry-control-helpers.cc"
    control_helpers.write_text(
        '#include "core/prototypes.h"\n#include "core/utils.h"\n'
        '#include "magic/spells.h"\n#include "telemetry/telemetry_runtime.h"\n'
        + function((ROOT / "src/core/utility.c").read_text(), "int BOUNDED(int a, int b, int c)") + "\n"
        + function(source, "bool blind(P_char ch, P_char victim, int duration)") + "\n"
        + function(source, "void Stun(P_char stunnee, P_char stunner, int duration, bool Fear_Check)") + "\n",
        encoding="utf-8",
    )
    command = [
        "g++",
        "-std=c++20",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-pthread",
        "-I",
        str(ROOT / "src"),
        str(ROOT / "tests/async/telemetry_gameplay_adapters.cc"),
        str(control_helpers),
        str(ROOT / "src/account/character_identity.c"),
        *[
            str(ROOT / "src/telemetry" / name)
            for name in (
                "telemetry_activity.c",
                "telemetry_battle.c",
                "telemetry_battle_contribution.c",
                "telemetry_battle_contract.c",
                "telemetry_combat_summary.c",
                "telemetry_config.c",
                "telemetry_encounter.c",
                "telemetry_failure.c",
                "telemetry_health.c",
                "telemetry_outage.c",
                "telemetry_queue.c",
                "telemetry_progression.c",
                "telemetry_repository.c",
                "telemetry_runtime.c",
                "telemetry_session.c",
                "telemetry_transport.c",
            )
        ],
        "-lcrypto",
        "-o",
        str(executable),
    ]
    if native_sql:
        command.extend(["-DTELEMETRY_TEST_NATIVE_BATTLE_SQL", "-DTELEMETRY_TEST_STUB_REPOSITORY",
                        str(ROOT / "tests/async/telemetry_gameplay_sql.cc")])
        command.extend(shlex.split(subprocess.check_output(["mysql_config", "--cflags", "--libs"], text=True)))
    else:
        command.append("-D__NO_MYSQL__")
    if sanitize:
        command.extend(["-g", "-fno-omit-frame-pointer", "-fsanitize=address,undefined"])
    subprocess.run(command, cwd=ROOT, check=True, timeout=120)


def main(*, sanitize: bool = False) -> None:
    artifacts = ROOT / "bin/tests"
    artifacts.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-gameplay-", dir=artifacts) as directory:
        executable = Path(directory) / "telemetry-gameplay-adapters"
        compile_gameplay(executable, sanitize=sanitize)
        completed = subprocess.run(
            [str(executable)], cwd=ROOT, check=False, text=True, capture_output=True, timeout=30
        )
        print(completed.stdout, end="")
        print(completed.stderr, end="")
        completed.check_returncode()
        assert "telemetry gameplay adapter paths passed" in completed.stdout


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true", help="run AddressSanitizer and UndefinedBehaviorSanitizer")
    main(sanitize=parser.parse_args().sanitize)
