#!/usr/bin/env python3
"""Exercise the actual read-only item revision queries against minimal history."""

from pathlib import Path
import re
import sqlite3
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = (ROOT / "migrations/reconcile_item_ownership.sh").read_text()


def query(name: str) -> str:
    match = re.search(
        rf'^{name}=\$\("\$\{{MYSQL\[@\]\}}" -e "([^"]+)"\)$',
        SCRIPT, re.MULTILINE,
    )
    if match is None:
        raise AssertionError(f"missing {name} SQL")
    return match.group(1)


class ItemOwnershipReconcileQueriesTest(unittest.TestCase):
    def test_baseline_and_post_baseline_revision_chains(self):
        connection = sqlite3.connect(":memory:")
        connection.executescript("""
            CREATE TABLE item_current_owner (item_uid INTEGER, item_revision INTEGER);
            CREATE TABLE item_ownership_baseline
                (item_uid INTEGER, opening_item_revision INTEGER);
            CREATE TABLE item_ownership_ledger (item_uid INTEGER, item_revision INTEGER);
        """)
        # A baseline chain and a new UID with contiguous creation/transfer history.
        connection.executemany("INSERT INTO item_current_owner VALUES (?,?)", [
            (1, 6), (2, 2), (3, 2), (4, 7), (5, 4), (6, 3), (7, 2),
        ])
        connection.executemany("INSERT INTO item_ownership_baseline VALUES (?,?)", [
            (1, 4), (5, 2), (6, 2),
        ])
        connection.executemany("INSERT INTO item_ownership_ledger VALUES (?,?)", [
            (1, 5), (1, 6), (2, 1), (2, 2), (3, 1), (3, 3),
            (5, 3), (5, 5), (7, 1),
        ])

        self.assertEqual(connection.execute(query("missing_baseline")).fetchone()[0], 1)
        self.assertEqual(connection.execute(query("item_revision_mismatch")).fetchone()[0], 2)
        self.assertEqual(connection.execute(query("ledger_revision_gap")).fetchone()[0], 2)

        # The two healthy chains remain unreported even with no baseline for UID 2.
        connection.execute("DELETE FROM item_current_owner WHERE item_uid>2")
        connection.execute("DELETE FROM item_ownership_baseline WHERE item_uid>2")
        connection.execute("DELETE FROM item_ownership_ledger WHERE item_uid>2")
        for name in ("missing_baseline", "item_revision_mismatch",
                     "ledger_revision_gap"):
            self.assertEqual(connection.execute(query(name)).fetchone()[0], 0, name)


if __name__ == "__main__":
    unittest.main()
