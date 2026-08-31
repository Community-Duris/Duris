"""Regression coverage for case-insensitive area source lookup on Linux."""

import errno
import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

GENERATORS = {
    "make_mob": ("mob", "mob"),
    "make_obj": ("obj", "obj"),
    "make_qst": ("qst", "qst"),
    "make_shp": ("shp", "shp"),
    "make_wld": ("wld", "wld"),
    "make_zon": ("zon", "zon"),
}


with tempfile.TemporaryDirectory(prefix="duris-area-case-") as temporary:
    workspace = Path(temporary)
    (workspace / "AREA").write_text("mixedcase\n")

    for directory, extension in GENERATORS.values():
        source_directory = workspace / directory
        source_directory.mkdir()
        (source_directory / f"MixedCase.{extension}").write_text(
            f"#1\nCASE_TOKEN_{extension}\n$~\n"
        )

    for tool, (directory, extension) in GENERATORS.items():
        binary = workspace / tool
        subprocess.run(
            [
                "gcc",
                str(ROOT / f"areas/src/{directory}/{tool}.c"),
                "-o",
                str(binary),
            ],
            cwd=ROOT,
            check=True,
        )
        completed = subprocess.run(
            [str(binary)],
            cwd=workspace,
            check=True,
            text=True,
            capture_output=True,
        )
        combined = (workspace / f"tworld.{extension}").read_text()
        assert f"CASE_TOKEN_{extension}" in combined, completed.stdout

    resolver_source = workspace / "resolver_harness.c"
    resolver_source.write_text(
        """
#include <errno.h>
#include <stdio.h>

#include "areas/src/area_file.h"

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;

    FILE *file = fopen_area_file(argv[1]);
    if (file == NULL)
    {
        fprintf(stderr, "%d\\n", errno);
        return 1;
    }

    fclose(file);
    return 0;
}
"""
    )
    resolver = workspace / "resolver_harness"
    subprocess.run(
        ["gcc", "-I", str(ROOT), str(resolver_source), "-o", str(resolver)],
        cwd=ROOT,
        check=True,
    )

    # A caller may pass a filename relative to its current directory.
    (workspace / "NoDirectory.txt").write_text("NO_DIRECTORY_TOKEN\n")
    subprocess.run(
        [str(resolver), "nodirectory.txt"],
        cwd=workspace,
        check=True,
        text=True,
        capture_output=True,
    )

    # Root-relative paths use / as their directory instead of an empty path.
    assert Path("/tmp").is_dir()
    assert not Path("/TMP").exists()
    completed = subprocess.run(
        [str(resolver), "/tmp"],
        cwd=workspace,
        check=False,
        text=True,
        capture_output=True,
    )
    assert completed.returncode != 0
    assert completed.stderr.strip() == str(errno.EISDIR)

    completed = subprocess.run(
        [str(resolver), "/TMP"],
        cwd=workspace,
        check=False,
        text=True,
        capture_output=True,
    )
    assert completed.returncode != 0
    assert completed.stderr.strip() == str(errno.EISDIR)

    # An exact match remains authoritative if both spellings exist.
    (workspace / "wld/mixedcase.wld").write_text("#2\nEXACT_TOKEN\n$~\n")
    subprocess.run(
        [str(workspace / "make_wld")],
        cwd=workspace,
        check=True,
        text=True,
        capture_output=True,
    )
    combined = (workspace / "tworld.wld").read_text()
    assert "EXACT_TOKEN" in combined
    assert "CASE_TOKEN_wld" not in combined

    # Without an exact match, two folded matches are ambiguous and neither is
    # selected based on readdir() order.
    (workspace / "AREA").write_text("ambiguous\n")
    (workspace / "wld/Ambiguous.wld").write_text("#3\nFIRST_TOKEN\n$~\n")
    (workspace / "wld/AMBIGUOUS.wld").write_text("#4\nSECOND_TOKEN\n$~\n")
    completed = subprocess.run(
        [str(workspace / "make_wld")],
        cwd=workspace,
        check=False,
        text=True,
        capture_output=True,
    )
    assert completed.returncode != 0
    combined = (workspace / "tworld.wld").read_text()
    assert "FIRST_TOKEN" not in combined
    assert "SECOND_TOKEN" not in combined
    assert "matches both" in completed.stderr

    # A folded directory match is not accepted as an area source file.
    (workspace / "AREA").write_text("directory\n")
    (workspace / "wld/Directory.wld").mkdir()
    completed = subprocess.run(
        [str(workspace / "make_wld")],
        cwd=workspace,
        check=False,
        text=True,
        capture_output=True,
    )
    assert completed.returncode != 0
    assert "Is a directory" in completed.stderr

    # Required area types fail the build when no source exists, while quest
    # and shop sources remain optional for areas that do not define them.
    (workspace / "AREA").write_text("missing\n")
    completed = subprocess.run(
        [str(workspace / "make_wld")],
        cwd=workspace,
        check=False,
        text=True,
        capture_output=True,
    )
    assert completed.returncode != 0
    assert "No such file or directory" in completed.stderr
    subprocess.run(
        [str(workspace / "make_qst")],
        cwd=workspace,
        check=True,
        text=True,
        capture_output=True,
    )

    # Optional files do not make their entire source directory optional.
    quest_directory = workspace / "qst"
    missing_quest_directory = workspace / "qst-missing"
    quest_directory.rename(missing_quest_directory)
    completed = subprocess.run(
        [str(workspace / "make_qst")],
        cwd=workspace,
        check=False,
        text=True,
        capture_output=True,
    )
    assert completed.returncode != 0
    assert "No such file or directory" in completed.stderr
    missing_quest_directory.rename(quest_directory)

    # A missing required source directory is likewise fatal.
    source_directory = workspace / "wld"
    missing_directory = workspace / "wld-missing"
    source_directory.rename(missing_directory)
    completed = subprocess.run(
        [str(workspace / "make_wld")],
        cwd=workspace,
        check=False,
        text=True,
        capture_output=True,
    )
    assert completed.returncode != 0
    assert "No such file or directory" in completed.stderr
    missing_directory.rename(source_directory)

    # Execute-only directory access permits the exact ENOENT probe but makes
    # the fallback directory scan fail with EACCES for non-root test users.
    if os.geteuid() != 0:
        source_directory.chmod(0o111)
        try:
            completed = subprocess.run(
                [str(workspace / "make_wld")],
                cwd=workspace,
                check=False,
                text=True,
                capture_output=True,
            )
            assert completed.returncode != 0
            assert "Permission denied" in completed.stderr
        finally:
            source_directory.chmod(0o755)


print("area filename case resolution OK")
