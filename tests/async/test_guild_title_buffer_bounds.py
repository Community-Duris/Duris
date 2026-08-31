#!/usr/bin/env python3
"""Regression contract for Guild's fixed-size default-title buffers."""

from _paths import SRC
from pathlib import Path


root = Path(__file__).resolve().parents[2]
source = (SRC / "assocs.c").read_text()

constructors_start = source.index("Guild::Guild(char *_name")
constructors_end = source.index("\nvoid Guild::initialize()", constructors_start)
constructors = source[constructors_start:constructors_end]

safe_copy = 'snprintf(titles[i], sizeof(titles[i]), "%s", guild_default_titles[i]);'
assert constructors.count(safe_copy) == 2, (
    "both Guild constructors must use each rank title's actual buffer capacity"
)
assert "snprintf(titles[i], MAX_STRING_LENGTH" not in constructors, (
    "Guild constructors must not claim MAX_STRING_LENGTH for 81-byte rank titles"
)

print("guild title buffer bound contract passed")
