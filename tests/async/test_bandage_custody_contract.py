#!/usr/bin/env python3
"""A consumed bandage must retire durable custody before leaving inventory."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


class BandageCustodyContract(unittest.TestCase):
    def test_bandage_is_extracted_only_after_committed_destruction(self):
        source = (ROOT / "src/economy/tradeskill.c").read_text(encoding="utf-8")
        submission = source[source.index("void do_bandage("):source.index("// Drannak Stuff")]
        publication = source[source.index("static bool publish_bandage_retirement("):
                             source.index("static void complete_bandage_retirement(")]

        self.assertIn("item_movement_transaction_submit(", submission)
        self.assertIn("item_transfer_reason::destruction", submission)
        self.assertIn("publish_bandage_retirement", submission)
        self.assertIn("economic_source_kind::intentional_destruction", submission)
        self.assertIn("if (!committed)", publication)
        self.assertIn("bandage->obj_uid != context.bandage_uid", publication)
        self.assertIn("extract_obj(bandage, TRUE)", publication)
        self.assertLess(publication.index("if (!committed)"),
                        publication.index("extract_obj(bandage, TRUE)"))
        self.assertLess(submission.index("item_movement_transaction_submit("),
                        submission.index("extract_obj(bandage, TRUE)"))


if __name__ == "__main__":
    unittest.main()
