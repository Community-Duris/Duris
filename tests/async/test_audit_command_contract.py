#!/usr/bin/env python3
"""Contract and behavior tests for the immortal 'audit' provenance command."""

from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


class AuditCommandContract(unittest.TestCase):
    def test_audit_command_registration(self):
        interp_h = (ROOT / "src/cmd/interp.h").read_text(encoding="utf-8")
        interp_c = (ROOT / "src/cmd/interp.c").read_text(encoding="utf-8")
        prototypes_h = (ROOT / "src/core/prototypes.h").read_text(encoding="utf-8")
        makefile = (ROOT / "src/Makefile").read_text(encoding="utf-8")

        self.assertIn("#define CMD_AUDIT 865", interp_h)
        self.assertIn('"audit",', interp_c)
        self.assertIn("CMD_GRT(CMD_AUDIT, STAT_DEAD + POS_PRONE, do_audit, FORGER);", interp_c)
        self.assertIn("void do_audit(P_char ch, char *arg, int cmd);", prototypes_h)
        self.assertIn("cmd/audit.o", makefile)

    def test_audit_command_implementation(self):
        audit_c = (ROOT / "src/cmd/audit.c").read_text(encoding="utf-8")

        # Security checks
        self.assertIn("if (IS_NPC(ch) || !IS_TRUSTED(ch))", audit_c)

        # Dual-backend queries
        self.assertIn("economic_accounting_item_reference_find_history", audit_c)
        self.assertIn("flatfile_item_accounting_reference_find_history", audit_c)

        # Audit item argument parsing
        self.assertIn("strtoull(", audit_c)
        self.assertIn("get_obj_vis(", audit_c)

        # Lineage details formatting
        self.assertIn("rev=", audit_c)
        self.assertIn("line=", audit_c)
        self.assertIn("event=", audit_c)
        self.assertIn("child=", audit_c)


if __name__ == "__main__":
    unittest.main()
