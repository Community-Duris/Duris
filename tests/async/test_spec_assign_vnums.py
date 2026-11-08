#!/usr/bin/env python3
"""Keep literal special-procedure VNUMs in the world selected by areas/AREA."""

from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[2]
AREAS = ROOT / "areas"
KINDS = {"mobile": "mob", "object": "obj", "room": "wld"}


def authored_vnums(extension):
    directory = AREAS / extension
    files = {path.name: path for path in directory.iterdir() if path.is_file()}
    folded = {}
    for name, path in files.items():
        folded.setdefault(name.lower(), []).append(path)
    vnums = set()
    for line in (AREAS / "AREA").read_text().splitlines():
        # Match make_mob/make_obj/make_wld: first-column '*' is a comment;
        # the first whitespace-delimited token is the area basename.
        if not line.strip() or line.startswith("*"):
            continue
        name = f"{line.split()[0]}.{extension}"
        path = files.get(name)
        if path is None:
            matches = folded.get(name.lower(), [])
            assert len(matches) == 1, f"{name}: missing or ambiguous area source"
            path = matches[0]
        vnums.update(int(value) for value in re.findall(
            r"^#(\d+)\s*$", path.read_text(encoding="latin-1"), re.MULTILINE
        ))
    assert vnums, f"no {extension} records found in the active area sources"
    return vnums


def literal_lookups():
    source = (ROOT / "src/specs/specs.assign.c").read_text()
    # Let the C preprocessor discard comments and disabled blocks, without
    # importing headers. Named constants and loop variables are not literal
    # assignments and remain outside this authored-content check.
    source = re.sub(r"^\s*#include[^\n]*", "", source, flags=re.MULTILINE)
    active = subprocess.run(
        ["g++", "-E", "-P", "-x", "c++", "-"], input=source,
        text=True, capture_output=True, check=True,
    ).stdout
    lookups = re.findall(r"\breal_(mobile|object|room)0\(\s*(\d+)\s*\)", active)
    assert lookups, "no literal special-procedure lookups found"
    return [(kind, int(value)) for kind, value in lookups]


def main():
    catalogues = {kind: authored_vnums(extension) for kind, extension in KINDS.items()}
    lookups = literal_lookups()
    missing = [(kind, vnum) for kind, vnum in lookups if vnum not in catalogues[kind]]
    if missing:
        for kind, vnum in missing:
            print(f"specs.assign.c: missing {kind} VNUM {vnum} would resolve to index zero")
        raise SystemExit(f"{len(missing)} special-procedure references lack active area records")
    print(f"{len(lookups)} literal special-procedure references match active area records")


if __name__ == "__main__":
    main()
