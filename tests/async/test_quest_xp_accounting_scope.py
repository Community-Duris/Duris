#!/usr/bin/env python3
"""Keep progression XP updates outside economic double-entry accounting."""

import unittest

from _paths import extract_function, source
from contract_text import contains


class QuestXpAccountingScopeTests(unittest.TestCase):
    def test_durable_quest_xp_uses_progression_save_without_economic_accounts(self):
        recovery = extract_function(
            "world/quest.c", "void quest_reward_recover_xp_entitlement("
        )
        self.assertIn(
            "gain_exp(player, nullptr, static_cast<int>(amount), EXP_QUEST)", recovery
        )
        self.assertIn("player_save_pipeline_request_quest_xp(", recovery)
        self.assertNotIn("economic_", recovery)

    def test_regular_gain_exp_has_no_economic_accounting_route(self):
        limits = source("world/limits.c").read_text(encoding="utf-8")
        start = limits.index("int gain_exp(")
        end = limits.index("\nint gain_condition(", start)
        gain_exp = limits[start:end]
        self.assertNotIn("economic_", gain_exp)

    def test_linkdead_in_world_group_members_receive_the_same_completion_payout(self):
        quest = source("world/quest.c").read_text(encoding="utf-8")
        present = extract_function("world/quest.c", "static P_char quest_reward_character_present(")
        capture = extract_function("world/quest.c", "static bool capture_quest_credit_context(")

        self.assertIn("for (P_char player = character_list; player; player = player->next)", present)
        self.assertIn("GET_PID(player) == static_cast<int>(pid)", present)
        self.assertNotIn("desc", present)
        self.assertIn("member->ch->in_room != actor->in_room", capture)
        self.assertTrue(contains(capture, "quest_reward_character_present(static_cast<uint32_t>(GET_PID(member->ch)))"))
        self.assertNotIn("desc", capture)
        self.assertIn("quest_reward_character_present(award.recipient_pid)", quest)
        payout = quest[quest.index("for (size_t index = 0; index < continuation.xp_award_count; ++index)"):]
        payout = payout[:payout.index("if (!mob || !completion->disappear)")]
        self.assertIn("if (P_char recipient = quest_reward_character_present(award.recipient_pid))", payout)
        self.assertIn("quest_reward_recover_xp_entitlement(", payout)
        self.assertNotIn("desc", payout)


if __name__ == "__main__":
    unittest.main()
