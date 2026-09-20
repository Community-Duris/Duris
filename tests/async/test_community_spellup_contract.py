#!/usr/bin/env python3
"""Focused source contracts for configurable community spell-up."""

from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (ROOT / "src/cmd/community_spellup.c").read_text()
ACTWIZ = (ROOT / "src/cmd/actwiz.c").read_text()
EVENTS = (ROOT / "src/world/new_events.c").read_text()
MAKEFILE = (ROOT / "src/Makefile").read_text()
HELP = (ROOT / "lib/information/help_index").read_text()
DESIGN = (ROOT / "docs/design/COMMUNITY_SPELLUP.md").read_text()


class CommunitySpellupContractTest(unittest.TestCase):
    def test_legacy_entry_point_and_default_order_remain_visible(self) -> None:
        self.assertIn("community_spellup_command(ch, arg);", ACTWIZ)
        self.assertIn("void newb_spellup(P_char ch, P_char victim)", ACTWIZ)
        default_order = (
            "spell_bless",
            "spell_spirit_armor",
            "spell_barkskin",
            "spell_enhance_armor",
            "spell_stone_skin",
            "spell_fly",
            "spell_haste",
            "spell_strength",
            "spell_agility",
            "spell_dexterity",
            "spell_accel_healing",
            "spell_rest",
        )
        positions = [ACTWIZ.index(name, ACTWIZ.index("void newb_spellup")) for name in default_order]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("cmd/community_spellup.o", MAKEFILE)

    def test_allowlist_and_defaults_are_explicit(self) -> None:
        names = (
            "bless",
            "spirit armor",
            "barkskin",
            "enhance armor",
            "stone skin",
            "fly",
            "haste",
            "strength",
            "agility",
            "dexterity",
            "accelerated healing",
            "rest",
            "regeneration",
        )
        positions = [SOURCE.index(f'"{name}"') for name in names]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("COMMUNITY_DEFAULT_SELECTION", SOURCE)
        self.assertIn("COMMUNITY_SPELL_LEVEL = 61", SOURCE)
        self.assertIn("COMMUNITY_MAX_TARGETS = 2048", SOURCE)
        self.assertIn("COMMUNITY_TARGETS_PER_SLICE = 32", SOURCE)

    def test_repeat_state_is_pointer_free_and_boot_scoped(self) -> None:
        self.assertIn("find_character_by_runtime_id(runtime_id)", SOURCE)
        self.assertIn("find_player_by_pid(job.creator_pid)", SOURCE)
        self.assertIn("add_event(community_spellup_event, delay, nullptr, nullptr, nullptr", SOURCE)
        self.assertIn("job.event_armed", SOURCE)
        self.assertIn("if (!job.active || !job.event_armed)", SOURCE)
        self.assertIn("job.pass_selection = job.selection", SOURCE)
        self.assertIn("job.pass_revision = job.revision", SOURCE)
        self.assertIn("community_spellup_reset_for_boot();", EVENTS)
        self.assertNotIn("P_char creator", SOURCE[SOURCE.index("struct repeat_job"):SOURCE.index("static std::array")])

    def test_interface_and_safety_contract_are_documented(self) -> None:
        for token in (
            "COMMUNITY_MIN_INTERVAL_SECONDS = 10",
            "COMMUNITY_MAX_INTERVAL_SECONDS = 60 * 60",
            "regeneration",
            "preview",
            "update",
            "stop",
            "creator authorization was revoked",
            "target cap truncated",
        ):
            self.assertIn(token, SOURCE, token)
        for token in (
            "Status: implemented in PR 521",
            "10 seconds through 1 hour",
            "offline-safe self-caster",
            "Cold reboot and copyover",
            "newbsa repeat <10s..1h> [g|e]",
        ):
            self.assertIn(token, DESIGN + HELP, token)


if __name__ == "__main__":
    unittest.main()
