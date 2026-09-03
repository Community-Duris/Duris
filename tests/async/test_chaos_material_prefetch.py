#!/usr/bin/env python3
"""Verify the boot-time Chaos material description prefetch contract."""

from __future__ import annotations

import os
import re
import subprocess
import tempfile
from pathlib import Path

from _paths import ROOT, source


MATERIALS = source("chaos_materials.c")
HEADER = source("chaos_materials.h")
CRAFTING = source("crafting.c")
CONFIG = ROOT / "lib/kingdom.cfg"

materials = MATERIALS.read_text(encoding="utf-8")
header = HEADER.read_text(encoding="utf-8")
crafting = CRAFTING.read_text(encoding="utf-8")
kingdom_config = CONFIG.read_text(encoding="utf-8")

body_match = re.search(
    r"void chaos_materials_initialize\(\)\s*\{(.*?)\n\}\n\nP_obj chaos_material_pouch_find",
    materials,
    re.DOTALL,
)
assert body_match is not None, "Chaos material prefetch function is missing"
body = body_match.group(1)
assert "chaos_starter_materials_enabled()" in body
assert "for (int vnum = first; vnum <= last; ++vnum)" in body
assert "read_object(vnum, VIRTUAL)" in body
assert "extract_obj(material, FALSE)" in body
assert "prefetch_range(LOWEST_MAT_VNUM, HIGHEST_MAT_VNUM)" in body
assert "prefetch_range(ENCRUST_VNUM_BEGIN, ENCRUST_VNUM_END)" in body
assert "void chaos_materials_initialize(void);" in header

config_load = crafting.index("load_crafting_config();")
prefetch_call = crafting.index("chaos_materials_initialize();", config_load)
assert config_load < prefetch_call, "prefetch must run after crafting boot reaches its object-ready hook"
assert re.search(r"^kingdom\.enabled\s*=\s*1\s*$", kingdom_config, re.MULTILINE)

HARNESS = r"""
#include "core/prototypes.h"
#include "combat/chaos_materials.h"

#include <stdio.h>
#include <stdlib.h>

static struct obj_data fake_object = {};
static int read_count = 0;
static int extract_count = 0;
static int seen[219] = {};

P_obj read_object(int vnum, int type)
{
    if (type != VIRTUAL || read_count >= 219)
        return nullptr;
    seen[read_count++] = vnum;
    return &fake_object;
}

void extract_obj(P_obj object, int gone_for_good)
{
    if (object != &fake_object || gone_for_good != 0)
        exit(2);
    ++extract_count;
}

void logit(const char *, const char *, ...)
{
}

int main(void)
{
    const bool expect_disabled = getenv("CHAOS_EXPECT_DISABLED") != nullptr;
    chaos_materials_initialize();
    if (expect_disabled)
        return read_count == 0 && extract_count == 0 ? 0 : 1;

    if (read_count != 219 || extract_count != 219)
        return 1;
    for (int index = 0; index < 210; ++index)
        if (seen[index] != 400000 + index)
            return 1;
    for (int index = 0; index < 9; ++index)
        if (seen[210 + index] != 400291 + index)
            return 1;
    puts("Chaos material descriptions prefetched and temporary objects released");
    return 0;
}
"""

with tempfile.TemporaryDirectory(prefix="duris-chaos-material-prefetch-") as temp_dir:
    temp = Path(temp_dir)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-ffunction-sections",
            "-fdata-sections",
            f"-I{ROOT / 'src'}",
            str(harness),
            str(MATERIALS),
            str(ROOT / "src/combat/chaos_config.c"),
            "-Wl,--gc-sections",
            "-o",
            str(binary),
        ],
        check=True,
    )

    enabled = os.environ.copy()
    enabled.update(
        {
            "CHAOS_MUD": "TRUE",
            "CHAOS_STARTER_BONUSES": "TRUE",
            "CHAOS_STARTER_MATERIALS": "TRUE",
        }
    )
    subprocess.run([str(binary)], check=True, env=enabled)

    disabled = os.environ.copy()
    disabled.update(
        {
            "CHAOS_MUD": "FALSE",
            "CHAOS_STARTER_BONUSES": "TRUE",
            "CHAOS_STARTER_MATERIALS": "TRUE",
            "CHAOS_EXPECT_DISABLED": "1",
        }
    )
    subprocess.run([str(binary)], check=True, env=disabled)

print("Chaos material prefetch and kingdom-enabled boot contracts passed")
