"""Regression coverage for case-insensitive area source lookup on Linux."""

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
                "-std=c11",
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
        check=True,
        text=True,
        capture_output=True,
    )
    combined = (workspace / "tworld.wld").read_text()
    assert "FIRST_TOKEN" not in combined
    assert "SECOND_TOKEN" not in combined
    assert "matches both" in completed.stderr


print("area filename case resolution OK")
