#!/usr/bin/env python3
"""Source contract for siege/kingdom retirement and gated prototype custody."""

from __future__ import annotations

import json
import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

RETIRED_OBJECT_VNUMS = (160, 161, 178, 179, 461, 462, 463, 464)
QUARTERMASTER_VNUMS = tuple(range(401000, 401091, 10))

# Release A must retain every dedicated prototype until the production and retained-backup
# custody gate is cleared. Flip this to False only in the gated Release B cleanup.
COMPATIBILITY_PROTOTYPES_RETAINED = True


def source_text() -> str:
    paths = sorted((*SRC.rglob("*.c"), *SRC.rglob("*.h")))
    return "\n".join(path.read_text(errors="replace") for path in paths)


class SiegeKingdomRemovalContractTest(unittest.TestCase):
    def test_runtime_implementation_and_destruction_state_are_absent(self) -> None:
        self.assertFalse((SRC / "combat/siege.c").exists())
        self.assertFalse((SRC / "combat/siege.h").exists())

        production = source_text()
        makefile = (SRC / "Makefile").read_text()
        forbidden = (
            "SIEGE_ENABLED",
            "P_siege",
            "P_town",
            "troop_info_rec",
            "kingdom_type_list",
            "flat_siege_",
            "flat_town_",
            "IS_DESTROYING",
            "set_destroying",
            "stop_destroying",
            "destroying_obj",
            "next_destroying",
            "destroying_list",
        )
        present = [token for token in forbidden if token in production]
        self.assertEqual(present, [], f"forbidden production tokens: {present}")
        self.assertNotRegex(makefile, r"(?m)^\s*combat/siege\.o(?:\s|$)")

        associations = (SRC / "guild/assocs.h").read_text()
        migrate_stubs = (ROOT / "migrations/tools/migrate_stubs.c").read_text()
        self.assertNotIn("is_kingdom", associations)
        self.assertNotIn("is_kingdom", migrate_stubs)

    def test_feature_world_wiring_and_prototype_custody_are_safe(self) -> None:
        for relative in (
            "areas/mob/siege.mob",
            "areas/obj/siege.obj",
            "areas/wld/siege.wld",
            "areas/zon/siege.zon",
        ):
            self.assertFalse((ROOT / relative).exists(), relative)

        area_index = (ROOT / "areas/AREA").read_text()
        self.assertNotRegex(area_index, r"(?m)^siege\s+\*4010\b")
        self.assertRegex(area_index, r"(?m)^\*RETIRED 4010\b")

        zone_sources = "\n".join(
            path.read_text(errors="replace") for path in sorted((ROOT / "areas/zon").glob("*.zon"))
        )
        quartermasters = "|".join(str(vnum) for vnum in QUARTERMASTER_VNUMS)
        self.assertNotRegex(zone_sources, rf"(?m)^M\s+\d+\s+(?:{quartermasters})\b")

        shop = (ROOT / "areas/shp/kzkrkeep.shp").read_text()
        self.assertNotRegex(shop, r"(?m)^\s*(?:178|179)\s*$")

        full_world = (ROOT / "areas/obj/heavens.obj").read_text()
        minimal_world = (ROOT / "areas_mini/mini.obj").read_text()
        full_vnums = {int(value) for value in re.findall(r"(?m)^#(\d+)\s*$", full_world)}
        minimal_vnums = {int(value) for value in re.findall(r"(?m)^#(\d+)\s*$", minimal_world)}
        gated_full = set(RETIRED_OBJECT_VNUMS) & full_vnums
        gated_minimal = {160, 161, 178, 179} & minimal_vnums
        if COMPATIBILITY_PROTOTYPES_RETAINED:
            self.assertEqual(gated_full, set(RETIRED_OBJECT_VNUMS))
            self.assertEqual(gated_minimal, {160, 161, 178, 179})
        else:
            self.assertEqual(gated_full, set())
            self.assertEqual(gated_minimal, set())

    def test_feature_flatfile_authorities_and_regressions_are_absent(self) -> None:
        for relative in (
            "defaults/towns",
            "tests/async/test_flatfile_towns.py",
            "tests/async/flatfile_town_harness.cpp",
            "tests/async/test_flatfile_siege.py",
            "tests/async/flatfile_siege_harness.cpp",
        ):
            self.assertFalse((ROOT / relative).exists(), relative)

        quality = (ROOT / ".github/workflows/quality.yml").read_text()
        self.assertNotRegex(quality, r"test_flatfile_(?:towns|siege)\.py")

    def test_help_and_command_surfaces_are_retired_in_place(self) -> None:
        self.assertFalse((ROOT / "lib/information/helpkingdoms").exists())

        importer = (ROOT / "scripts/import_help_to_prod.sh").read_text()
        catalog = (SRC / "flatfile/flatfile_help_catalog.c").read_text()
        help_index = (ROOT / "lib/information/help_index").read_text()
        attributes = (ROOT / "docs/lib/information/command_attributes.txt").read_text()
        for text in (importer, catalog):
            self.assertNotIn("helpkingdoms", text)
        self.assertNotIn("ADD (Immortal Command)", help_index)
        self.assertNotIn("Kingdom View", help_index)
        self.assertNotRegex(attributes, r"(?m)^(?:add|deploy)\s*$")

        interp = (SRC / "cmd/interp.c").read_text()
        interp_header = (SRC / "cmd/interp.h").read_text()
        self.assertIn('"_retired_827"', interp)
        self.assertIn('"_retired_828"', interp)
        self.assertIn("CMD_RETIRED_827", interp_header)
        self.assertIn("CMD_RETIRED_828", interp_header)
        self.assertNotRegex(interp_header, r"\bCMD_(?:ADD|DEPLOY)\b")

        structs = (SRC / "core/structs.h").read_text()
        self.assertIn("PLR2_RETIRED_KINGDOMVIEW", structs)
        self.assertNotRegex(structs, r"\bPLR2_KINGDOMVIEW\b")

    def test_runtime_sql_is_detached_but_schema_tombstones_remain(self) -> None:
        runtime_sql = "\n".join(
            (SRC / relative).read_text()
            for relative in ("sql/sql.c", "sql/sql_player.c", "sql/sql_player.h")
        )
        for token in (
            "sql_save_towns",
            "sql_load_towns",
            "sql_save_kingdom_land",
            "sql_save_siege_item",
            "sql_save_siege_list",
            "sql_delete_siege_items",
            "sql_load_siege_list",
        ):
            self.assertNotIn(token, runtime_sql, token)
        table_references = sorted(
            set(
                re.findall(
                    r"(?i)\b(?:FROM|INTO|UPDATE|DELETE\s+FROM)\s+"
                    r"(towns|kingdom_land|siege_items|siege_item_affects|siege_item_extra_descr)\b",
                    runtime_sql,
                )
            )
        )
        self.assertEqual(table_references, [])

        lifecycle = json.loads((ROOT / "migrations/data_lifecycle_manifest.json").read_text())
        stores = {
            entry["locator"]: entry
            for entry in lifecycle["entries"]
            if entry["locator"]
            in {
                "towns",
                "kingdom_land",
                "siege_items",
                "siege_item_affects",
                "siege_item_extra_descr",
            }
        }
        self.assertEqual(
            set(stores),
            {
                "towns",
                "kingdom_land",
                "siege_items",
                "siege_item_affects",
                "siege_item_extra_descr",
            },
        )
        for store, entry in stores.items():
            self.assertEqual(entry["season_action"], "retain", store)
            self.assertIn("compatibility", entry["technical_purpose"].lower(), store)


if __name__ == "__main__":
    unittest.main()
