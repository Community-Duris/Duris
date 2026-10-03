#!/usr/bin/env python3
"""RED source-contract gates for the missing private flatfile lifecycle owner.

These are deliberately not synthetic activation tests. A lifecycle owner must
first exist before executable native capture, bank-lifetime reconciliation, and
receipt/proof-fenced activation can be exercised against production code.
"""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
FLATFILE = ROOT / "src/flatfile"


class FlatfileAccountingLifecycleOwnerRED(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.owner_files = sorted(FLATFILE.glob("flatfile_accounting_lifecycle*"))
        cls.owner_source = "\n".join(
            path.read_text(encoding="utf-8")
            for path in cls.owner_files
            if path.suffix in {".c", ".h"}
        )

    def require_owner(self, criterion):
        self.assertTrue(
            self.owner_source,
            "RED: no production flatfile accounting lifecycle owner exists; "
            f"cannot establish {criterion} through the private authority boundary",
        )

    def test_native_capture_covers_wallets_and_shared_banks(self):
        """A complete capture must enumerate both source classes and bind coverage."""
        self.require_owner("complete wallet/shared-bank native capture")
        self.assertIn("economic_account_kind::wallet", self.owner_source)
        self.assertIn("economic_account_kind::bank", self.owner_source)
        self.assertIn("coverage_digest", self.owner_source)

    def test_bank_capture_requires_exactly_one_retained_lifetime(self):
        """Missing or duplicate shared-bank lifetimes must stop baseline staging."""
        self.require_owner("one-to-one shared-bank lifetime reconciliation")
        self.assertIn("flatfile_economic_mapping_read", self.owner_source)
        self.assertIn("flatfile_economic_native_lookup", self.owner_source)
        self.assertIn("flatfile_accounting_baseline_storage::stage", self.owner_source)

    def test_epoch_activation_waits_for_receipt_and_never_activated_proof(self):
        """Selection must follow baseline receipts and external virgin-state proof."""
        self.require_owner("receipt-bearing, never-activated-gated epoch selection")
        self.assertIn("flatfile_accounting_baseline_storage::initialize", self.owner_source)
        self.assertIn("flatfile_accounting_baseline_storage::stage", self.owner_source)
        self.assertIn("flatfile_accounting_authority_storage::select_epoch", self.owner_source)
        self.assertTrue(
            "never_activated" in self.owner_source or "virgin_state" in self.owner_source,
            "RED: no explicit external never-activated/virgin-state proof gate",
        )
        self.assertTrue(
            "receipt" in self.owner_source.lower(),
            "RED: epoch activation is not bound to a retained lifecycle receipt",
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
