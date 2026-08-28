#!/usr/bin/env python3
"""Regression coverage for the retired corpse cleanup script."""

from __future__ import annotations

import os
import stat
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "delete_corpses.sh"


def write_probe(path: Path, marker: Path) -> None:
    path.write_text(f"#!/usr/bin/env bash\ntouch '{marker}'\n", encoding="utf-8")
    path.chmod(path.stat().st_mode | stat.S_IXUSR)


def main() -> None:
    source = SCRIPT.read_text(encoding="utf-8")
    forbidden = (
        "redis-cli",
        "mysql",
        "mud:world_state",
        "mud:floor_drops",
        "DELETE FROM",
    )
    for token in forbidden:
        assert token not in source, f"retired script still contains {token!r}"

    with tempfile.TemporaryDirectory(prefix="delete-corpses-retired-") as temp_dir:
        temp = Path(temp_dir)
        bin_dir = temp / "bin"
        bin_dir.mkdir()
        database_marker = temp / "database-invoked"
        cache_marker = temp / "cache-invoked"
        write_probe(bin_dir / "mysql", database_marker)
        write_probe(bin_dir / "redis-cli", cache_marker)

        env = os.environ.copy()
        env["PATH"] = f"{bin_dir}{os.pathsep}{env['PATH']}"
        result = subprocess.run(
            [str(SCRIPT)],
            cwd=ROOT,
            env=env,
            check=False,
            capture_output=True,
            text=True,
        )

        assert result.returncode == 2, result
        assert "retired and performs no cleanup" in result.stderr
        assert not database_marker.exists(), "retired script invoked the database client"
        assert not cache_marker.exists(), "retired script invoked the cache client"

    print("delete_corpses retirement tests passed")


if __name__ == "__main__":
    main()
