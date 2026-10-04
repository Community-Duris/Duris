#!/usr/bin/env python3
"""Guard the manually reviewed native selected-status writer inventory.

The lexical inventory is a change detector, not a proof of C++ behavior. The
native gameplay/affect journeys execute the mutation classes; the accompanying
review records explain construction, alias and completed-operation boundaries.
"""
from collections import Counter
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]
INVENTORY = ROOT / "docs/telemetry/CONTROL_SOURCE_INVENTORY.json"
SELECTED = {"AFF_BLIND", "AFF_SLEEP", "AFF_BOUND", "AFF2_STUNNED", "AFF2_MAJOR_PARALYSIS",
            "AFF2_MINOR_PARALYSIS", "AFF2_SLOW", "AFF2_SILENCED"}
WRITES = re.compile(
    r"(?:SET_BIT|REMOVE_BIT)\s*\([^;]*?specials\.affected_by2?\b[^;]*?\);|"
    r"\b\w+->specials\.affected_by2?\s*[|&]?=[^;]+;|"
    r"(?:memset|bzero)\([^;]*sizeof\((?:struct )?char_data\)[^;]*\);|"
    r"(?:SPOFFSET|OFFSET)\((?:specials\.)?affected_by2?\)", re.S)
FUNCTION = re.compile(r"(?m)^[A-Za-z_][^;{}]*\([^;{}]*\)[^;{}]*\{")


def candidates():
    found = Counter()
    for path in sorted((ROOT / "src").rglob("*")):
        if path.suffix not in (".c", ".h"):
            continue
        source = path.read_text(encoding="utf-8")
        source = re.sub(r"/\*.*?\*/|//[^\n]*", lambda m: "\n" * m[0].count("\n"), source, flags=re.S)
        functions = list(FUNCTION.finditer(source))
        for match in WRITES.finditer(source):
            statement = " ".join(match[0].split())
            if statement.startswith(("SET_BIT", "REMOVE_BIT")):
                rhs = statement[statement.index(",") + 1:]
                names = re.findall(r"\b[A-Za-z_]\w*\b", rhs)
                if names and all(name.startswith(("AFF_", "AFF2_")) for name in names) and not SELECTED.intersection(names):
                    continue  # Reviewed literal bits outside the selected mask.
            before = [item for item in functions if item.start() <= match.start()]
            signature = before[-1][0] if before else ""
            name = re.search(r"([A-Za-z_]\w*)\s*\(", signature)
            found[path.relative_to(ROOT).as_posix(), name[1] if name else "", statement] += 1
    return found


class NativeControlInventoryTests(unittest.TestCase):
    def test_all_selected_writer_candidates_have_an_explicit_review(self):
        inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
        self.assertEqual(inventory["selected_flags"], sorted(SELECTED))
        expected = Counter({(row["path"], row["function"], row["statement"]): row["occurrences"]
                            for row in inventory["writers"]})
        self.assertEqual(candidates(), expected, "Review new or changed writers before claiming duration coverage")
        self.assertTrue(all(row["review"] in inventory["reviews"] for row in inventory["writers"]))

    def test_every_observed_function_has_its_completed_state_boundary(self):
        inventory = json.loads(INVENTORY.read_text(encoding="utf-8"))
        for row in inventory["boundaries"]:
            with self.subTest(path=row["path"], function=row["function"]):
                source = (ROOT / row["path"]).read_text(encoding="utf-8")
                source = re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)
                match = next(item for item in FUNCTION.finditer(source) if
                             re.search(r"([A-Za-z_]\w*)\s*\(", item[0])[1] == row["function"])
                depth, end = 1, match.end()
                while depth:
                    depth += (source[end] == "{") - (source[end] == "}")
                    end += 1
                body = source[match.start():end]
                self.assertIn(row["observation"], body)
        from test_telemetry_combat_hooks import function
        affects = (ROOT / "src/magic/affects.c").read_text(encoding="utf-8")
        callers = []
        for path in (ROOT / "src").rglob("*.c"):
            body = re.sub(r"/\*.*?\*/|//[^\n]*", "", path.read_text(encoding="utf-8"), flags=re.S)
            callers.extend((path.relative_to(ROOT).as_posix(), item) for item in
                           re.findall(r"\bapply_affs\([^;{]+[;{]", body))
        self.assertEqual(len(callers), 2)  # The definition and all_affects' single call.
        self.assertTrue(all(path == "src/magic/affects.c" for path, _ in callers))
        self.assertIn("apply_affs(ch, mode);", function(affects, "void all_affects("))


if __name__ == "__main__":
    unittest.main(verbosity=2)
