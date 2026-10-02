#!/usr/bin/env python3
"""Exercise report classification, source provenance, and keyword gates."""

import csv
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from audit_help import (audit, canonical, coverage_row, csv_report,
                        effective_catalog, references, resolve, source_terms)
from parse_help_index import HelpEntry, parse_help_index, read_help_index, read_parsed_help


class HelpAuditTests(unittest.TestCase):
    def setUp(self):
        (ROOT / "bin").mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=ROOT / "bin")
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)

    def test_index_parser_keeps_inline_hash_and_unquoted_titles(self):
        path = self.path / "index"
        path.write_bytes(b'last update: test\r\n # \r\n"One"\r\n===\r\nbody # stays\r\n===\r\n\t#\t\r\nTwo (category)\r\nsecond body\r\n')
        entries = list(read_help_index(path, "index"))
        self.assertEqual([(entry.title, entry.text, entry.line) for entry in entries],
                         [("One", "body # stays", 3), ("Two", "second body", 8)])
        self.assertEqual(parse_help_index(path), [("One", "body # stays"), ("Two", "second body")])

    def test_parsed_help_uses_full_heading_and_whitespace_delimiters(self):
        path = self.path / "parsed"
        path.write_text('Short\nComplete title - Last Edited: old\nbody\n #0 \n\nOther\nSecond title\nlong enough text\n', encoding="utf-8")
        entries = list(read_parsed_help(path, "parsed"))
        self.assertEqual([entry.title for entry in entries], ["Complete title", "Second title"])
        self.assertEqual([entry.line for entry in entries], [2, 7])
        self.assertTrue(entries[0].text.startswith("Complete title - Last Edited:"))

    def test_collisions_record_the_actual_winner(self):
        entries = [HelpEntry("HELP", "old", "first", 1), HelpEntry("help", "new", "second", 12)]
        catalog, collisions = effective_catalog(entries)
        self.assertEqual(catalog["help"].text, "new")
        self.assertEqual(collisions, [{"title": "help", "previous": "first:1",
                                      "winner": "second:12", "same_text": False}])

    def test_coverage_does_not_treat_body_mentions_as_help_entries(self):
        entries = [HelpEntry("Fire", "Describes missing topic", "index", 1),
                   HelpEntry("Fire shield", "shield", "index", 8)]
        catalog, _ = effective_catalog(entries)
        bodies = {key: canonical(entry.text) for key, entry in catalog.items()}
        for query, status in [("FIRE", "exact"), ("shield", "partial"),
                              ("fir", "ambiguous"), ("missing topic", "missing")]:
            self.assertEqual(coverage_row({"term": query}, catalog, bodies)["status"], status)
        missing = coverage_row({"term": "missing topic"}, catalog, bodies)
        self.assertEqual(missing["body_mentions"], ["Fire"])
        self.assertEqual(missing["body_mention_count"], 1)

    def test_redirects_and_see_also_references(self):
        entries = [HelpEntry("A", "Redirect: B", "index", 1),
                   HelpEntry("B", "Redirect: A", "index", 4),
                   HelpEntry("Gone", "Redirect: Absent", "index", 7)]
        catalog, _ = effective_catalog(entries)
        self.assertEqual(resolve(catalog, "A")[0], "cycle")
        self.assertEqual(resolve(catalog, "Gone"), ("missing_target", "Absent"))
        self.assertEqual(coverage_row({"term": "Gone"}, catalog)["status"], "broken_redirect")
        self.assertEqual(references('[[Fire|label]]\n==See also==\n* Water, Ice\n\nThe following help topics\nFoo'),
                         ["Fire", "Ice", "Water"])
        self.assertEqual(references('See also: Fire, Water\nbody'), ["Fire", "Water"])

    def test_source_inventory_tracks_current_registrations(self):
        terms = source_terms(ROOT)
        by_kind = {kind: [term for term in terms if term["kind"] == kind]
                   for kind in {term["kind"] for term in terms}}
        self.assertEqual(len(by_kind["race"]), 37)
        self.assertEqual(len(by_kind["class"]), 30)
        self.assertEqual(len(by_kind["class_skillset"]), 30)
        self.assertTrue(any(row["term"] == "help" and row["handler"] == "do_help"
                            for row in by_kind["command"]))
        # A commented-out registration in skills.c must not create a gap.
        self.assertFalse(any(row["term"] == "instant kill" for row in by_kind["skill"]))
        self.assertTrue(any(row["term"] == "named equipment" and
                            row["definition"].startswith("src/item/randomeq.c:")
                            for row in by_kind["help_hint"]))
        # Variable keyword templates are instructions, not static query terms.
        self.assertFalse(any("<" in row["term"] or ">" in row["term"]
                             for row in by_kind["help_hint"]))

    def test_explicit_export_and_keyword_gate_are_read_only(self):
        path = self.path / "pages.jsonl"
        pages = [{"title": "help", "text": "welcome", "category_id": 0},
                 {"title": "Fire", "text": "The following help topics also matched your search:\nOld", "category_id": 0}]
        path.write_text("\n".join(json.dumps(page) for page in pages), encoding="utf-8")
        before = path.read_bytes()
        report = audit(ROOT, path)
        self.assertEqual(report["catalog_source"], "pages_export")
        self.assertEqual(report["quality_counts"]["captured_search_output"], 1)
        self.assertEqual(path.read_bytes(), before)
        output = self.path / "report.csv"
        command = [sys.executable, str(ROOT / "scripts/audit_help.py"), "--pages-json", str(path),
                   "--format", "csv", "--output", str(output)]
        success = subprocess.run(command + ["--require-term", "help"], capture_output=True, text=True)
        self.assertEqual(success.returncode, 0, success.stderr)
        failure = subprocess.run(command + ["--require-term", "missing topic"], capture_output=True, text=True)
        self.assertEqual(failure.returncode, 1)
        self.assertIn("Exact usable help required", failure.stderr)
        rows = list(csv.DictReader(io.StringIO(csv_report(report))))
        self.assertTrue(any(row["kind"] == "race" and row["status"] == "missing" for row in rows))

    def test_export_reports_duplicate_titles_and_inactive_redirects(self):
        path = self.path / "pages.json"
        path.write_text(json.dumps([
            {"title": "help", "text": "welcome", "category_id": 0},
            {"title": "Fire", "text": "one", "category_id": 0},
            {"title": "FIRE", "text": "two", "category_id": 0},
            {"title": "Gone", "text": "Redirect: help", "category_id": 0},
            {"title": None, "text": "invalid", "category_id": 0},
        ]), encoding="utf-8")
        report = audit(ROOT, path)
        self.assertEqual(report["quality"]["duplicate_database_titles"], ["fire"])
        self.assertEqual(report["quality_counts"]["invalid_database_pages"], 1)
        self.assertEqual(report["collisions"][0]["later_row"], "pages.json:3")
        self.assertNotIn("winner", report["collisions"][0])
        result = subprocess.run([sys.executable, str(ROOT / "scripts/audit_help.py"),
                                 "--pages-json", str(path), "--output", str(self.path / "out.json"),
                                 "--require-term", "Gone"], capture_output=True, text=True)
        self.assertEqual(result.returncode, 1, result.stderr)


if __name__ == "__main__":
    unittest.main()
