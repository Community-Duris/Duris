#!/usr/bin/env python3

import os
import pathlib
import stat
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


with tempfile.TemporaryDirectory(prefix="duris-flatfile-build-") as build_tmp:
    build_root = pathlib.Path(build_tmp)
    binary = build_root / "server" / "dms_new"
    build = subprocess.run(
        [
            "make",
            "-C",
            "src",
            "PERSISTENCE_BACKEND=flatfile",
            f"BIN_ROOT={build_root}",
            f"OBJDIR={build_root / 'objects' / 'server'}",
            f"SERVER_BIN_DIR={binary.parent}",
            f"DMS_BINARY={binary}",
            "-j2",
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=180,
    )
    require(build.returncode == 0, "client-free server build failed:\n" + build.stdout[-8000:])
    require("-D__NO_MYSQL__" in build.stdout, "flat build did not select __NO_MYSQL__")
    require("-I/usr/include/mysql" not in build.stdout, "flat build used system MySQL headers")
    require("-lmysqlclient" not in build.stdout, "flat build linked the MySQL client")

    with tempfile.TemporaryDirectory(prefix="duris-flatfile-state-") as state_tmp:
        with tempfile.TemporaryDirectory(prefix="duris-flatfile-run-") as run_tmp:
            state_root = pathlib.Path(state_tmp)
            os.chmod(state_root, 0o700)
            boot = subprocess.run(
                [str(binary)],
                cwd=run_tmp,
                env={
                    "PATH": os.environ.get("PATH", "/usr/bin:/bin"),
                    "PERSISTENCE_MODE": "flatfile-primary",
                    "FLATFILE_STATE_DIR": str(state_root),
                },
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                timeout=10,
            )

            require(boot.returncode != 0, "incomplete flat-file backend unexpectedly booted")
            require(
                "persistence mode flatfile-primary is not ready" in boot.stdout,
                "boot did not identify the selected incomplete backend:\n" + boot.stdout,
            )
            require(
                "unimplemented durable domains: character rename/delete completion"
                in boot.stdout,
                "boot did not emit the durable-domain inventory:\n" + boot.stdout,
            )

            expected_dirs = {
                "metadata",
                "identities",
                "identities/accounts",
                "identities/names",
                "players",
                "operations",
                "operations/wal",
                "domains",
                "manifests",
            }
            actual_dirs = {
                str(path.relative_to(state_root))
                for path in state_root.rglob("*")
                if path.is_dir()
            }
            require(actual_dirs == expected_dirs, f"unexpected authority topology: {actual_dirs}")
            for path in [state_root, *(state_root / name for name in expected_dirs)]:
                mode = stat.S_IMODE(path.stat().st_mode)
                require(mode == 0o700, f"insecure mode {mode:o} on {path}")

print("client-free build and flat-file boot preflight passed")
