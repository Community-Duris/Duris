#!/usr/bin/env python3
"""Reproduce scenery frames in terminal ANSI on a dark background, then alternate colors."""
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tests/async"))
from scenery_fixture import CONFIG, scenery_harness, alternate_config

colors = dict(zip("kbgcrmywKBGCRMYW", (30, 34, 32, 36, 31, 35, 33, 37,
                                        90, 94, 92, 96, 91, 95, 93, 97)))


def terminal(markup):
    def substitute(match):
        code = match.group(0)
        return "\x1b[0m" if code.lower() == "&n" else f"\x1b[{colors[code[2]]}m"
    return re.sub(r"&[nN]|&\+[kbgcrmywKBGCRMYW]", substitute, markup)


with scenery_harness() as binary:
    alternate = binary.parent / "alternate.json"
    alternate.write_text(json.dumps(alternate_config()))
    for title, config in (("STANDARD PALETTE", CONFIG), ("ALTERNATE PALETTE", alternate)):
        transcript = subprocess.check_output([str(binary), str(config), "--preview"], text=True, timeout=120)
        print(f"\n{title} — use a dark client background\n")
        print(terminal(transcript), end="\x1b[0m\n")
