#!/usr/bin/env python3
"""Entry-boundary regressions for active item movement admission."""

from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile
import unittest

from _paths import extract_function

ROOT = Path(__file__).resolve().parents[2]


class ItemAdmissionBoundaryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.header = (ROOT / "src/item/item_movement_transaction.h").read_text(
            encoding="utf-8"
        )
        cls.refusal = extract_function(
            "item/item_movement_transaction.c",
            "bool refuse_active_item_submission(",
        )
        cls.single = extract_function(
            "item/item_movement_transaction.c",
            "bool item_movement_transaction_submit(",
        )
        cls.batch = extract_function(
            "item/item_movement_transaction.c",
            "bool item_movement_transaction_submit_batch(",
        )

    def test_compiled_single_boundary_does_not_treat_rejected_result_as_predicate(self):
        # Compile the actual helper and callsite: source-presence checks alone
        # missed a false rejection result used as a true/false refusal predicate.
        helper_call = self.single.index("refuse_active_item_submission(")
        start = self.single.rfind("\tif (", 0, helper_call)
        stop = self.single.index("\tconst item_owner_identity effective_from", helper_call)
        self.assertGreaterEqual(start, 0)
        guard = self.single[start:stop]
        reject_helper = extract_function(
            "item/item_movement_transaction.c", "bool reject_with("
        )
        reject_enum = re.search(
            r"enum class item_movement_reject\s*\{[^}]+\};", self.header
        )
        assert reject_enum is not None, "actual item rejection enum not found"
        program = "#include <cassert>\n" + reject_enum.group(0) + r"""
enum class item_transfer_reason { creation, other };
namespace economic_gameplay_authority {
bool enabled = false;
bool active() { return enabled; }
}
int continuations = 0;
""" + reject_helper + "\n" + self.refusal + r"""
bool actual_single_guard(bool retained_owner_identity, bool owner_matches_request,
                         item_transfer_reason reason, item_movement_reject *reject)
{
""" + guard + r"""
    ++continuations;
    return true;
}
int main()
{
    const auto unset = static_cast<item_movement_reject>(-1);
    for (int bits = 0; bits < 16; ++bits) {
        const bool active = bits & 1;
        const bool retained = bits & 2;
        const bool matches = bits & 4;
        const bool creation = bits & 8;
        economic_gameplay_authority::enabled = active;
        continuations = 0;
        auto rejection = unset;
        const bool accepted = actual_single_guard(
            retained, matches, creation ? item_transfer_reason::creation
                                       : item_transfer_reason::other, &rejection);
        if (!active) {
            assert(accepted && continuations == 1 && rejection == unset);
            continue;
        }
        assert(!accepted && continuations == 0);
        const auto expected = retained && !matches
            ? item_movement_reject::owner_mismatch
            : !retained && !creation ? item_movement_reject::missing_owner_identity
                                    : item_movement_reject::active_accounting_unsupported;
        assert(rejection == expected);
        // Batch callers return the helper result directly; refusal must remain
        // false, rather than changing the helper to repair only the single path.
        rejection = unset;
        assert(!refuse_active_item_submission(true, retained, matches, creation, &rejection));
        assert(rejection == expected);
    }
}
"""
        with tempfile.TemporaryDirectory(prefix="duris-item-admission-") as temp:
            source = Path(temp) / "admission.cpp"
            binary = Path(temp) / "admission"
            source.write_text(program, encoding="utf-8")
            subprocess.run(
                shlex.split(os.environ.get("CXX", "g++")) + [
                    "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    str(source), "-o", str(binary),
                ], check=True, timeout=60,
            )
            subprocess.run([str(binary)], check=True, timeout=10, cwd=temp)

    def test_central_refusal_preserves_inactive_and_separates_unsupported_from_missing(self):
        self.assertIn("if (!accounting_active)\n\t\treturn false;", self.refusal)
        self.assertIn("item_movement_reject::missing_owner_identity", self.refusal)
        self.assertIn("item_movement_reject::active_accounting_unsupported", self.refusal)
        self.assertIn("item_movement_reject::owner_mismatch", self.refusal)
        self.assertIn("if (!retained_owner_identity && !new_creation)", self.refusal)
        self.assertIn("active_accounting_unsupported", self.header)
        self.assertIn("missing_owner_identity", self.header)

    def test_single_submission_refuses_before_creation_fallback_or_command_submission(self):
        admission = self.single.index("refuse_active_item_submission(")
        self.assertLess(admission, self.single.index("const item_owner_identity effective_from"))
        self.assertLess(admission, self.single.index("capture_absent(root"))
        self.assertLess(admission, self.single.index("item_transfer_command_build"))
        self.assertLess(admission, self.single.index("critical_command_coordinator_submit"))
        self.assertIn("reason == item_transfer_reason::creation", self.single[:admission + 400])
        self.assertIn(".reason = adopted ? reason : item_transfer_reason::creation", self.single)

    def test_absent_noncreation_and_ambiguous_batch_owners_are_refused_at_entry(self):
        self.assertIn("missing_owner_identity", self.refusal)
        batch_admission = self.batch.index("if (economic_gameplay_authority::active())")
        self.assertLess(batch_admission, self.batch.index("item_transfer_command_build"))
        self.assertLess(batch_admission, self.batch.index("critical_command_coordinator_submit"))
        self.assertLess(batch_admission, self.batch.index("item_ownership_runtime_owner_revision"))
        self.assertIn("!item_ownership_runtime_lookup(root->obj_uid, &runtime)", self.batch)
        self.assertIn("!item_owner_identity_valid(runtime.owner)", self.batch)
        self.assertIn("item_owner_identity_equal(runtime.owner, from_owner)", self.batch)

    def test_new_creation_is_unsupported_not_missing_identity_and_destruction_is_not_creation(self):
        self.assertIn("reason == item_transfer_reason::creation", self.single)
        self.assertRegex(self.single, r"reason == item_transfer_reason::creation,\s*reject\)")
        self.assertIn("if (creation)", self.batch)
        self.assertIn("reason == item_transfer_reason::creation;", self.batch)
        self.assertIn("return refuse_active_item_submission(true, false, false, true, reject);", self.batch)
        # For a missing retained row, only explicit creation reaches the
        # unsupported-source-admission refusal; destruction is not issuance.


if __name__ == "__main__":
    unittest.main()
