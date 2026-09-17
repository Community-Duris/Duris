#!/usr/bin/env python3
"""Regression contracts for supported terminal modes and MSP semantics."""

from _paths import source
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
UTILS = source("utils.h").read_text(encoding="utf-8", errors="replace")
CONSTANT = source("constant.c").read_text(encoding="utf-8", errors="replace")
NANNY = source("nanny.c").read_text(encoding="utf-8", errors="replace")
COMM = source("comm.c").read_text(encoding="utf-8", errors="replace")
ACTOTH = source("actoth.c").read_text(encoding="utf-8", errors="replace")
GUIDE = (ROOT / "docs" / "guides" / "TERMINAL_MODES.md").read_text(
    encoding="utf-8", errors="replace"
)


def check(name, condition):
    print(("OK: " if condition else "FAIL: ") + name)
    return bool(condition)


ok = True
ok &= check(
    "TERM_MSP is labeled as presentation markup without audio triggers",
    "legacy MSP-style presentation markup" in UTILS
    and "no sound/music triggers" in UTILS,
)
ok &= check(
    "terminal help lists every supported selection code",
    "01) Generic" in CONSTANT
    and "02) ANSI" in CONSTANT
    and "03) MSP presentation markup" in CONSTANT
    and "09) Quick ANSI" in CONSTANT
    and "no sound/music triggers" in CONSTANT,
)
ok &= check(
    "all login prompts explain the MSP presentation choice",
    NANNY.count("'3' for MSP markup") >= 3
    and "'1' for Generic" in NANNY
    and "'9' for Quick" in NANNY,
)
ok &= check(
    "the initial connection prompt lists all terminal choices",
    "'1' for Generic" in COMM
    and "'3' for MSP markup" in COMM
    and "'9' for Quick" in COMM
    and "'?' for help" in COMM,
)
ok &= check(
    "toggle feedback identifies MSP as markup",
    'return "MSP markup";' in ACTOTH,
)
ok &= check(
    "toggle usage documents the supported generic mode",
    "USAGE: TOGGLE terminal [ansi|gen|msp]" in ACTOTH,
)
ok &= check(
    "the guide documents presentation tags and the absence of audio triggers",
    "does not emit `!!SOUND(...)`" in GUIDE
    and "`!!MUSIC(...)`" in GUIDE
    and "`<prompt>...</prompt>`" in GUIDE
    and "`<map>...</map>`" in GUIDE
    and "`<group>...</group>`" in GUIDE,
)

if not ok:
    raise SystemExit(1)

print("\nAll terminal mode contracts passed.")
