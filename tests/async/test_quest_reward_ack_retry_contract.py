from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


class QuestRewardAckRetryContractTest(unittest.TestCase):
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
