from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


class QuestRewardAckRetryContractTest(unittest.TestCase):
    def test_daily_v6_ack_does_not_require_fee_action_identity(self):
        repository = (ROOT / "src/persistence/quest_reward_obligation_repository.c").read_text()
        pipeline = (ROOT / "src/persistence/quest_reward_obligation_pipeline.c").read_text()
        self.assertIn("quest_reward_is_fee_only(candidate) &&", repository)
        self.assertNotIn("candidate.version == 6 &&", repository)
        self.assertIn("quest_reward_is_fee_only(terms)", pipeline)
        self.assertNotIn("terms.version == 6", pipeline)

    def test_waits_for_group_xp_receipts_then_retries_ack(self):
        repository = (ROOT / "src/persistence/quest_reward_obligation_repository.c").read_text()
        pipeline = (ROOT / "src/persistence/quest_reward_obligation_pipeline.c").read_text()

        self.assertIn(
            "return pending_effects ? quest_reward_obligation_result::pending_effects",
            repository,
        )
        retry = pipeline.split(
            "if (completion.result == quest_reward_obligation_result::pending_effects)", 1
        )[1].split("std::lock_guard<std::mutex> lock(pipeline_mutex);", 1)[0]
        self.assertIn("std::chrono::seconds(5)", retry)
        self.assertIn("requests.push_back(request)", retry)
        self.assertNotIn("completions.push_back", retry)
        self.assertLess(retry.index("catch (...)"), retry.index("remove_active_request"))


if __name__ == "__main__":
    unittest.main()
