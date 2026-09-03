#!/usr/bin/env python3
"""Source contract for siege/kingdom retirement and gated prototype custody."""

from __future__ import annotations

import json
import re
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _source_contract import function_body, strip_comments  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

RETIRED_OBJECT_VNUMS = (160, 161, 178, 179, 461, 462, 463, 464)
QUARTERMASTER_VNUMS = tuple(range(401000, 401091, 10))

# Release A must retain every dedicated prototype until the production and retained-backup
# custody gate is cleared. Flip this to False only in the gated Release B cleanup.
COMPATIBILITY_PROTOTYPES_RETAINED = False


def source_text() -> str:
    """Every .c and .h under src/, concatenated."""
    paths = sorted((*SRC.rglob("*.c"), *SRC.rglob("*.h")))
    return "\n".join(path.read_text(errors="replace") for path in paths)


class SiegeKingdomRemovalContractTest(unittest.TestCase):
    """Pins the siege/kingdom retirement: the retired runtime, world wiring and
    SQL surfaces stay gone, the gated prototypes stay in custody, and the two
    surfaces the new kingdom module legitimately revives are the only
    exceptions."""

    def test_runtime_implementation_and_destruction_state_are_absent(self) -> None:
        """siege.c/.h, their tokens and the destruction state are gone from
        src/, while the revived Guild::is_kingdom delegates to the module."""
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

        # The NEW kingdom module (src/kingdom/) revives Guild::is_kingdom with
        # a real definition delegating to its seam. What must stay dead is the
        # old declared-but-undefined form, so BOTH halves are required: the
        # declaration in the header (any spelling of an argument-less bool
        # method) and a DEFINITION in assocs.c whose body delegates to
        # kingdom_guild_has_realm() rather than re-deriving the answer.
        associations = strip_comments((SRC / "guild/assocs.h").read_text())
        migrate_stubs = (ROOT / "migrations/tools/migrate_stubs.c").read_text()
        self.assertRegex(
            associations, r"\bbool\s+is_kingdom\s*\(\s*(?:void)?\s*\)\s*(?:const\s*)?;"
        )
        definition = function_body(
            (SRC / "guild/assocs.c").read_text(),
            r"\bbool\s+Guild::is_kingdom\s*\(\s*(?:void)?\s*\)\s*(?:const\s*)?",
        )
        self.assertIsNotNone(definition, "Guild::is_kingdom has no definition in assocs.c")
        self.assertRegex(definition, r"\bkingdom_guild_has_realm\s*\(")
        self.assertNotIn("is_kingdom", migrate_stubs)

    def test_feature_world_wiring_and_prototype_custody_are_safe(self) -> None:
        """The siege area, its zone loads, shop entries and gated prototypes
        are absent from the world files."""
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
        """The towns/siege flat authorities and their tests are gone and no
        longer wired into CI."""
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

    def test_retirement_contract_is_in_maintained_documentation(self) -> None:
        """Compatibility reservations remain visible after the research ledger is retired."""
        codebase = (ROOT / "docs/reference/CODEBASE.md").read_text()
        database = (ROOT / "docs/reference/DATABASE.md").read_text()
        for token in (
            "CMD_RETIRED_827",
            "CMD_RETIRED_828",
            "PLR2_RETIRED_KINGDOMVIEW",
            "zone 4010",
            "401000",
            "160, 161, 178, 179, and 461 through 464",
            "SIEGE_ENABLED",
            "src/kingdom/",
        ):
            self.assertIn(token, codebase, token)
        for table in (
            "towns",
            "kingdom_land",
            "siege_items",
            "siege_item_affects",
            "siege_item_extra_descr",
            "kingdom_realms",
        ):
            self.assertIn(f"`{table}`", database, table)

    def test_help_and_command_surfaces_are_retired_in_place(self) -> None:
        """The old siege help, `add`/`deploy` commands and Kingdom View toggle
        stay retired; the new helpkingdoms file is registered."""
        # helpkingdoms returned with the NEW kingdom module: the file now
        # documents the ring-claim system and must be registered in the help
        # catalog (flat build) and is ALLOWED back in the production importer
        # (MariaDB build; test_kingdom_contract.py pins its presence). What
        # commit 552230d47 retired from the help surfaces, and what must stay
        # retired, is the old siege material: the `add` and `deploy` immortal
        # commands and the "Kingdom View" toggle. Every other in-place
        # retirement below stands unchanged.
        self.assertTrue((ROOT / "lib/information/helpkingdoms").exists())

        importer = (ROOT / "scripts/import_help_to_prod.sh").read_text()
        catalog = (SRC / "flatfile/flatfile_help_catalog.c").read_text()
        help_index = (ROOT / "lib/information/help_index").read_text()
        attributes = (ROOT / "docs/lib/information/command_attributes.txt").read_text()
        self.assertIn("helpkingdoms", catalog)

        # THE IMPORTER. It is a bash map from help FILE to page title, so help
        # TITLES ("Kingdom View", "ADD (Immortal Command)") could never appear
        # in it and forbidding them pinned nothing -- neither did forbidding
        # ["add"]/["deploy"] entries, which never existed either. The single
        # importer line 552230d47 actually touched was ["helpkingdoms"], and
        # the kingdom module deliberately restored it. So pin what is true and
        # load-bearing: the entry is back, and every file the importer names
        # still exists, which is what keeps a retired help file from being
        # listed and silently skipped at import time.
        table = re.search(r"declare\s+-A\s+HELP_FILES=\((.*?)\n\)", importer, re.S)
        self.assertIsNotNone(table, "importer HELP_FILES table not found")
        entries = dict(re.findall(r'\["([^"]+)"\]="([^"]*)"', table.group(1)))
        self.assertEqual(entries.get("helpkingdoms"), "kingdoms")
        # Resolve each name the way the import loop does -- hints.txt from
        # docs/, everything else from lib/information -- rather than accepting
        # EITHER directory. A file present only in the wrong one is skipped at
        # import time, which is the failure this pin exists to catch.
        def imported_path(name: str) -> Path:
            root = "docs/lib/information" if name == "hints.txt" else "lib/information"
            return ROOT / root / name

        missing = [name for name in entries if not imported_path(name).exists()]
        self.assertEqual(missing, [], f"importer names help files that do not exist: {missing}")

        # THE HELP INDEX and the command attributes DID carry the retired
        # surfaces, and 552230d47 removed them: these can fail.
        self.assertNotIn("ADD (Immortal Command)", help_index)
        self.assertNotIn("Kingdom View", help_index)
        self.assertNotRegex(attributes, r"(?m)^(?:add|deploy)\s*$")

        interp = (SRC / "cmd/interp.c").read_text()
        interp_header = (SRC / "cmd/interp.h").read_text()
        self.assertIn('"_retired_827"', interp)
        self.assertIn('"_retired_828"', interp)
        retired_guard = interp.split("static bool is_retired_command_spelling", 1)[1].split(
            "\n}", 1
        )[0]
        self.assertIn('"add"', retired_guard)
        self.assertIn('"deploy"', retired_guard)
        self.assertIn("is_retired_command_spelling(argument + begin, look_at)", interp)
        self.assertIn("CMD_RETIRED_827", interp_header)
        self.assertIn("CMD_RETIRED_828", interp_header)
        self.assertNotRegex(interp_header, r"\bCMD_(?:ADD|DEPLOY)\b")

        structs = (SRC / "core/structs.h").read_text()
        self.assertIn("PLR2_RETIRED_KINGDOMVIEW", structs)
        self.assertNotRegex(structs, r"\bPLR2_KINGDOMVIEW\b")

    def test_runtime_sql_is_detached_but_schema_tombstones_remain(self) -> None:
        """No runtime SQL touches the retired tables; the lifecycle manifest
        still retains them as compatibility tombstones."""
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
