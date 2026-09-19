#!/usr/bin/env python3
"""Exercise the artifact catalog validator and two-phase publish workflow."""

from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CLI = ROOT / "scripts" / "artifactctl.py"
CATALOG = ROOT / "lib" / "artifacts" / "catalog.json"
sys.path.insert(0, str(ROOT / "scripts"))
import artifactctl  # noqa: E402


def run(*args: str, cwd: Path = ROOT) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(CLI), *args], cwd=cwd, text=True,
        capture_output=True, check=False,
    )


def main() -> int:
    result = run("--catalog", str(CATALOG), "validate")
    assert result.returncode == 0, result.stderr
    source = json.loads(CATALOG.read_text(encoding="utf-8"))
    assert len(source["definitions"]) == 7
    assert source["revision"] == 2
    assert artifactctl.catalog_hash(source) == "6ffca26e302fedc9"
    for definition in source["definitions"]:
        assert definition["holderPolicy"] == {
            "player": "legacy",
            "wildNpc": "legacy",
            "controlledNpc": "legacy",
        }

    with tempfile.TemporaryDirectory(prefix="artifact-control-") as directory:
        root = Path(directory)
        active = root / "catalog.json"
        shutil.copy2(CATALOG, active)
        staged = run("--catalog", str(active), "set-mode", "31514", "player", "telegraphic")
        assert staged.returncode == 0, staged.stderr
        draft = Path(str(active) + ".draft")
        assert draft.exists()
        assert json.loads(active.read_text(encoding="utf-8"))["definitions"][4]["holderPolicy"]["player"] == "legacy"
        assert json.loads(draft.read_text(encoding="utf-8"))["definitions"][4]["holderPolicy"]["player"] == "telegraphic"
        staged_enable = run("--catalog", str(active), "enable", "31514", "0")
        assert staged_enable.returncode == 0, staged_enable.stderr
        draft_value = json.loads(draft.read_text(encoding="utf-8"))
        assert draft_value["definitions"][4]["holderPolicy"]["player"] == "telegraphic"
        assert draft_value["definitions"][4]["enabled"] is False

        published = run("--catalog", str(active), "publish")
        assert published.returncode == 0, published.stderr
        active_value = artifactctl.load(active)
        assert active_value["revision"] == 3
        assert active_value["definitions"][4]["holderPolicy"]["player"] == "telegraphic"
        assert active_value["hash"] == artifactctl.catalog_hash(active_value)

        invalid = json.loads(active.read_text(encoding="utf-8"))
        invalid["definitions"].append(dict(invalid["definitions"][0]))
        try:
            artifactctl.validate(invalid, check_hash=False)
        except artifactctl.CatalogError as error:
            assert "duplicate artifact id" in str(error)
        else:
            raise AssertionError("duplicate definitions must be rejected")

    print("artifact control catalog and publish workflow: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
