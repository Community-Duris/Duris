"""Source contracts for command_attributes.txt coverage of registered commands.

Guarantees, per docs/content/HELP_SYSTEM.md:
  1. every command name registered in src/interp.c has an attribute entry
  2. every entry parses cleanly under load_cmd_attributes() semantics
     (name line, zero or more canonical GET_C_<STAT>(ch|vi lines, '~' terminator)
  3. the loader in src/wikihelp.c can hold every entry (capacity + bounds check)
  4. Luck attributes render from ATT_LUK flags, not ATT_STR (regression guard)
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INTERP_C = (ROOT / "src/interp.c").read_text()
WIKIHELP_C = (ROOT / "src/wikihelp.c").read_text()
WIKIHELP_H = (ROOT / "src/wikihelp.h").read_text()
ATTRS_PATH = ROOT / "docs/lib/information/command_attributes.txt"

STATS = ["STR", "DEX", "AGI", "CON", "POW", "INT", "WIS", "CHA", "KAR", "LUK"]


def registered_commands():
    block = re.search(r"const char \*command\[MAX_CMD\] = \{(.*?)\n\};", INTERP_C, re.S)
    assert block is not None, "command[] array not found in src/interp.c"
    names = re.findall(r'"([^"]+)"', block.group(1))
    # final "\n" is the sentinel the interpreter loops on, not a command
    return [n for n in names if n != "\n"]


def parse_entries(text):
    """Yield (name, attr_lines) mirroring load_cmd_attributes(); raises on
    a missing '~' terminator (the loader would swallow the next name)."""
    lines = text.split("\n")
    i = 0
    while i < len(lines):
        if lines[i] == "":
            i += 1
            continue
        name = lines[i]
        attrs = []
        i += 1
        while i < len(lines) and lines[i] != "~":
            attrs.append(lines[i])
            i += 1
        assert i < len(lines), f"entry {name!r} is not terminated with '~'"
        i += 1
        yield name, attrs


def test_every_registered_command_has_an_entry():
    commands = registered_commands()
    entries = dict(parse_entries(ATTRS_PATH.read_text()))
    missing = [c for c in commands if c not in entries]
    assert not missing, f"commands without an attribute entry: {missing}"


def test_entry_structure_is_loader_clean():
    for name, attrs in parse_entries(ATTRS_PATH.read_text()):
        for line in attrs:
            ok = any(
                line == f"GET_C_{stat}(ch" or line == f"GET_C_{stat}(vi"
                for stat in STATS
            )
            assert ok, f"{name!r}: non-canonical attribute line {line!r}"


def test_no_duplicate_entry_names():
    names = [name for name, _ in parse_entries(ATTRS_PATH.read_text())]
    dupes = sorted({n for n in names if names.count(n) > 1})
    assert not dupes, f"duplicate entries shadow earlier ones: {dupes}"


def test_loader_capacity_covers_all_entries():
    count = sum(1 for _ in parse_entries(ATTRS_PATH.read_text()))
    cap = re.search(r"#define CMD_ATTRIB_MAX (\d+)", WIKIHELP_H)
    assert cap is not None, "CMD_ATTRIB_MAX missing from src/wikihelp.h"
    assert count <= int(cap.group(1)), (
        f"{count} entries exceed CMD_ATTRIB_MAX={cap.group(1)}"
    )
    squeezed = re.sub(r"\s+", "", WIKIHELP_C)
    assert "if(count>=CMD_ATTRIB_MAX)" in squeezed, (
        "load_cmd_attributes() lacks a bounds check against CMD_ATTRIB_MAX"
    )


def test_luck_renders_from_att_luk_not_att_str():
    sq = re.sub(r"\s+", "", WIKIHELP_C)
    assert "if(ch_attributes[ATT_LUK])APPEND_ATTR(\"Char'sLuck.\\n\");" in sq
    assert "if(vi_attributes[ATT_LUK])APPEND_ATTR(\"Victim'sLuck.\\n\");" in sq


if __name__ == "__main__":
    # The rest of tests/async is a set of executable source-contract scripts, and
    # run_command_attributes_coverage.sh invokes this file with plain python3.
    # Without this driver the module would only define its test functions and
    # exit 0, reporting success while asserting nothing.
    import sys
    import traceback

    failures = 0
    for _name, _fn in sorted(globals().items()):
        if _name.startswith("test_") and callable(_fn):
            try:
                _fn()
            except Exception:
                failures += 1
                print("FAIL: %s" % _name)
                traceback.print_exc()
            else:
                print("OK: %s" % _name)

    if failures:
        print("\n%d command attribute contract check(s) failed." % failures)
        sys.exit(1)
    print("command attribute coverage contracts: OK")
