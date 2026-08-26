"""Regression coverage for the repository-level clean-all target."""

import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MAKEFILES = (
    "Makefile",
    "src/Makefile",
    "areas/de/src/Makefile",
    "areas/src/Makefile",
    "areas/src/areas/Makefile",
    "areas/src/mob/Makefile",
    "areas/src/obj/Makefile",
    "areas/src/qst/Makefile",
    "areas/src/shp/Makefile",
    "areas/src/wld/Makefile",
    "areas/src/zon/Makefile",
    "src-migrate/Makefile",
)


def write(path: Path, contents: str = "artifact\n") -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(contents)


with tempfile.TemporaryDirectory(prefix="duris-clean-all-") as directory:
    checkout = Path(directory)
    for relative in MAKEFILES:
        destination = checkout / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / relative, destination)

    artifacts = (
        "bin/server/dms_new",
        "bin/server/dms",
        "bin/server/dms_san",
        "bin/server/history/dms.old",
        "bin/objects/server/object.o",
        "bin/objects/server-san/object.o",
        "bin/packages/duris-build-deps.deb",
        "bin/tests/native-test",
        "areas/world.mob",
        "areas/world.obj",
        "areas/world.qst",
        "areas/world.shp",
        "areas/world.wld",
        "areas/world.zon",
        "areas/tworld.mob",
        "areas/tworld.obj",
        "areas/tworld.qst",
        "areas/tworld.shp",
        "areas/tworld.wld",
        "areas/tworld.zon",
        "areas/mini.mob",
        "areas/mini.obj",
        "areas/mini.wld",
        "areas/mini.zon",
        "areas/.world.stamp",
        "lib/misc/lookup.mob",
        "lib/misc/lookup.obj",
        "lib/misc/lookup.wld",
        "lib/misc/lookup.zon",
        "lib/misc/lookup_with_limits.zon",
        "build/warning-inventory/report.txt",
        ".pytest_cache/v/cache/nodeids",
        "tests/__pycache__/runner.pyc",
        "nested/.mypy_cache/state",
        "nested/.ruff_cache/state",
        "nested/htmlcov/index.html",
        "coverage.xml",
        ".coverage",
        ".coverage.worker",
        "profile.gcda",
        "profile.gcno",
        "profile.profraw",
        "gmon.out",
        "areas/de/src/cscope.out",
    )
    for relative in artifacts:
        write(checkout / relative)

    preserved = {
        "bin/.gitignore": "*\n!.gitignore\n",
        ".env": "DB_NAME=duris_dev\n",
        "logs/log/status": "running\n",
        "Players/a/player": "player data\n",
        "Accounts/a/account": "account data\n",
        "areas/world.justice": "justice input\n",
        "areas/world.tab": "table input\n",
        "areas/world.weather": "weather input\n",
        "areas/source.mob": "area source\n",
        ".git/cscope.out": "Git metadata\n",
    }
    for relative, contents in preserved.items():
        write(checkout / relative, contents)

    subprocess.run(
        ["make", "clean-all"],
        cwd=checkout,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )

    for relative in artifacts:
        assert not (checkout / relative).exists(), f"artifact survived: {relative}"

    bin_entries = sorted(
        path.relative_to(checkout / "bin").as_posix()
        for path in (checkout / "bin").rglob("*")
    )
    assert bin_entries == [".gitignore"], f"unexpected bin contents: {bin_entries}"

    for relative, contents in preserved.items():
        path = checkout / relative
        assert path.read_text() == contents, f"clean-all changed protected data: {relative}"

print("clean-all contracts OK")
