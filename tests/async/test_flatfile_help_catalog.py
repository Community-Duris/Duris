#!/usr/bin/env python3

from _paths import rel
import pathlib
import subprocess
import sys
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from audit_help import canonical, effective_catalog, tracked_entries


(ROOT / "bin").mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix="duris-flatfile-help-", dir=ROOT / "bin") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_help_catalog_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            "tests/async/flatfile_help_catalog_harness.cpp",
            rel("flatfile_help_catalog.c"),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(binary), str(ROOT)], check=True)

    def compare_catalog(root):
        dumped = subprocess.check_output([str(binary), str(root), "--dump"], text=True)
        actual = {}
        for line in dumped.splitlines():
            title, text = (bytes.fromhex(field).decode("utf-8") for field in line.split("\t"))
            actual[canonical(title)] = (title, text)
        expected, _ = effective_catalog(tracked_entries(root))
        assert actual == {key: (entry.title, entry.text) for key, entry in expected.items()}, \
            "audit catalog differs from the production C++ loader"

    compare_catalog(ROOT)

    fixture = temporary_path / "search-fixture"
    (fixture / "lib/information").mkdir(parents=True)
    content = '"help"\nDEFAULT_MENU\n  # \n'
    content += "".join(f'"aaa bulk{index:03d}"\nother topic\n#\n' for index in range(150))
    content += 'bulk (test entry)\nEXACT_AFTER_CAP\n#\n'
    (fixture / "lib/information/help_index").write_text(content, encoding="utf-8")
    # The auditor reads the same source list even for a small content fixture.
    (fixture / "src/flatfile").mkdir(parents=True)
    (fixture / "src/flatfile/flatfile_help_catalog.c").write_text(
        (ROOT / rel("flatfile_help_catalog.c")).read_text(), encoding="utf-8")
    compare_catalog(fixture)

    dynamic_fixture = temporary_path / "dynamic-fixture"
    (dynamic_fixture / "lib/information").mkdir(parents=True)
    (dynamic_fixture / "lib/duris.properties").write_bytes((ROOT / "lib/duris.properties").read_bytes())
    (dynamic_fixture / "lib/creation_availability.cfg").write_bytes(
        (ROOT / "lib/creation_availability.cfg").read_bytes())
    (dynamic_fixture / "lib/information/help_index").write_text('''"help"
DEFAULT_MENU
#
"Human"
Human - Last Edited: old
========================
NARRATIVE_BEFORE
==Class list==
STALE_CLASS
==Racial Statistics==
Strength    : STALE_STATS
==Innate abilities==
STALE_INNATES
==Strengths==
NARRATIVE_AFTER
==See also==
* Races
#
"Warrior Skills"
==See also==
* Warrior
==Skills==
STALE_SKILLS
==Spells==
STALE_SPELLS
#
"SKILL_WARRIOR"
Skills
----------------------
STALE_SKILLS
Warrior Specializations
----------------------
STALE_SPECS
See also: Warrior
#
"Bard Skills"
==Songs==
STALE_SONGS
==Instruments==
STALE_INSTRUMENTS
#
"Elementalist"
UNRELATED_PARTIAL_MATCH
#
"Assassin"
Current or historical narrative
#
"Races"
==Good Races==
STALE_ROSTER
#
"Multiclass"
NARRATIVE_BEFORE
Here are the options available to each class:
STALE_OPTIONS
==Multi-Class Names==
STALE_NAMES
#
''', encoding="utf-8")

    invalid_root = temporary_path / "invalid"
    (invalid_root / "lib/information").mkdir(parents=True)
    with (invalid_root / "lib/information/help_index").open("wb") as source:
        source.truncate(8 * 1024 * 1024 + 1)
    invalid = subprocess.run(
        [str(binary), str(invalid_root)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if invalid.returncode == 0:
        raise AssertionError("oversized help source was accepted")

    runtime_binary = temporary_path / "flatfile_help_runtime_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_help_runtime_harness.cpp",
            rel("wikihelp.c"),
            rel("flatfile_help_catalog.c"),
            rel("common.c"),
            rel("constant.c"),
            rel("creation_availability_config.c"),
            rel("specializations.c"),
            "-Wl,--gc-sections",
            "-o",
            str(runtime_binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(runtime_binary)], cwd=ROOT, check=True)
    subprocess.run([str(runtime_binary), "dynamic-catalog"], cwd=ROOT, check=True)
    subprocess.run([str(runtime_binary), "search-fixture"], cwd=fixture, check=True)
    subprocess.run([str(runtime_binary), "dynamic-fixture"], cwd=dynamic_fixture, check=True)

    # Compile the same gameplay renderer/harness through its SQL branch. Only
    # the cache boundary is seeded from files; rendering still performs no SQL.
    sql_binary = temporary_path / "sql_help_runtime_test"
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-ffunction-sections",
        "-fdata-sections", "-I/usr/include/mysql", "-Isrc",
        "tests/async/flatfile_help_runtime_harness.cpp", rel("wikihelp.c"),
        rel("flatfile_help_catalog.c"), rel("common.c"), rel("constant.c"),
        rel("creation_availability_config.c"), rel("specializations.c"),
        "-Wl,--gc-sections", "-o", str(sql_binary)
    ], cwd=ROOT, check=True)
    subprocess.run([str(sql_binary), "dynamic-catalog"], cwd=ROOT, check=True)
    subprocess.run([str(sql_binary), "dynamic-fixture"], cwd=dynamic_fixture, check=True)

    mud_info_binary = temporary_path / "flatfile_mud_info_runtime_test"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-D__NO_MYSQL__",
            "-ffunction-sections",
            "-fdata-sections",
            "-Isrc/no_mysql",
            "-Isrc",
            "tests/async/flatfile_mud_info_runtime_harness.cpp",
            rel("sql.c"),
            rel("flatfile_help_catalog.c"),
            "-Wl,--gc-sections",
            "-o",
            str(mud_info_binary),
        ],
        cwd=ROOT,
        check=True,
    )
    subprocess.run([str(mud_info_binary)], cwd=ROOT, check=True)

print("flat-file help catalog regression passed")
