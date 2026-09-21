#!/usr/bin/env python3
"""Focused #265 runtime lifecycle journey with a bounded fake repository."""

from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def verify_shutdown_contract() -> None:
    header = (ROOT / "src/telemetry/telemetry_runtime.h").read_text()
    assert "telemetry_runtime_final_reap(void)" in header
    comm = (ROOT / "src/net/comm.c").read_text()
    main = comm[comm.index("int main(") :]
    assert main.index("telemetry_runtime_shutdown(") < main.index(
        "telemetry_runtime_final_reap()"
    ) < main.index("shutdown_mysql();")


def main(*, exhaustion: bool = False) -> None:
    verify_shutdown_contract()
    with tempfile.TemporaryDirectory(prefix="telemetry-runtime-") as directory:
        variants: list[tuple[str, bool, Path | None]] = [("flatfile", True, None)]
        for include_directory in (Path("/usr/include/mariadb"), Path("/usr/include/mysql")):
            if (include_directory / "mysql.h").is_file():
                variants.append(("sql", False, include_directory))
                break
        else:
            print("telemetry runtime SQL variant skipped: mysql headers unavailable")

        for label, no_mysql, mysql_include in variants:
            executable = str(Path(directory) / f"telemetry-runtime-integration-{label}")
            command = [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pthread",
            ]
            if no_mysql:
                command.append("-D__NO_MYSQL__")
            command += [
                "-I",
                str(ROOT / "src"),
            ]
            if mysql_include is not None:
                command += ["-I", str(mysql_include)]
            if mysql_include is not None:
                command.append("-DTELEMETRY_TEST_STUB_REPOSITORY")
            command += [
                str(ROOT / "tests/async" / ("telemetry_runtime_exhaustion.cc" if exhaustion
                                          else "telemetry_runtime_integration.cc")),
                *[
                    str(ROOT / "src/telemetry" / name)
                    for name in (
                        "telemetry_activity.c",
                        "telemetry_combat_summary.c",
                        "telemetry_config.c",
                        "telemetry_encounter.c",
                        "telemetry_failure.c",
                        "telemetry_health.c",
                        "telemetry_queue.c",
                        "telemetry_progression.c",
                        "telemetry_runtime.c",
                        "telemetry_session.c",
                        "telemetry_transport.c",
                    )
                    if not (exhaustion and name == "telemetry_runtime.c")
                ],
            ]
            if mysql_include is None:
                command.append(str(ROOT / "src/telemetry/telemetry_repository.c"))
            command += [
                "-lcrypto",
                "-o",
                executable,
            ]
            subprocess.run(command, cwd=ROOT, check=True, timeout=120)
            completed = subprocess.run(
                [executable], cwd=ROOT, check=False, text=True, capture_output=True, timeout=30
            )
            print(completed.stdout, end="")
            print(completed.stderr, end="")
            completed.check_returncode()
            marker = ("telemetry sequence-exhausted lifecycle consistency passed" if exhaustion
                      else "telemetry runtime lifecycle and copyover integration passed")
            assert marker in completed.stdout


if __name__ == "__main__":
    main()
