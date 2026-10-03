#!/usr/bin/env python3
"""Read tracked help using the flat-file loader's entry rules, without writes."""

from dataclasses import dataclass
from pathlib import Path

# Repository root, so the script works regardless of the caller's directory.
ROOT = Path(__file__).resolve().parents[1]
WHITESPACE = " \t\r\n"


@dataclass(frozen=True)
class HelpEntry:
    title: str
    text: str
    source: str
    line: int


def blocks(content, delimiter):
    """Yield nonempty blocks and their one-based first nonblank line."""
    lines = []
    begin = 1
    for number, line in enumerate(content.split("\n"), 1):
        line = line.removesuffix("\r")
        if line.strip(WHITESPACE) == delimiter:
            if lines:
                yield begin, lines
            lines = []
            begin = number + 1
        elif lines or line.strip(WHITESPACE):
            if not lines:
                begin = number
            lines.append(line)
    if lines:
        yield begin, lines


def separator(line):
    value = line.strip(WHITESPACE)
    return bool(value) and set(value) == {"="}


def read_help_index(filename, source=None):
    """Include unquoted titles; only a standalone # ends an entry."""
    path = Path(filename)
    for number, lines in blocks(path.read_text(encoding="utf-8"), "#"):
        heading = lines[0].strip(WHITESPACE)
        if heading.lower().startswith("last update:"):
            continue
        if heading.startswith('"'):
            end = heading.find('"', 1)
            title = heading[1:end].strip(WHITESPACE) if end != -1 else ""
        else:
            title = heading.split("(", 1)[0].strip(WHITESPACE)
        body = lines[1:]
        while body and not body[-1].strip(WHITESPACE):
            body.pop()
        while body and not body[0].strip(WHITESPACE):
            body.pop(0)
        if body and separator(body[0]):
            body.pop(0)
        if body and separator(body[-1]):
            body.pop()
        text = "\n".join(body).strip(WHITESPACE)
        if title and text:
            yield HelpEntry(title, text, str(source or path), number)


def read_parsed_help(filename, source=None):
    """Match parse_parsed_help: the block's second line supplies the title."""
    path = Path(filename)
    for number, lines in blocks(path.read_text(encoding="utf-8"), "#0"):
        if len(lines) < 2:
            continue
        heading = lines[1].strip(WHITESPACE)
        if not heading or heading.startswith(("==", "*")):
            continue
        title = heading.split(" - Last Edited:", 1)[0].strip(WHITESPACE)
        text = "\n".join(lines[1:]).strip(WHITESPACE)
        if title and len(text.encode("utf-8")) > 10:
            yield HelpEntry(title, text, str(source or path), number + 1)


def parse_help_index(filename):
    """Keep the original (title, text) interface for existing callers."""
    return [(entry.title, entry.text) for entry in read_help_index(filename)]


if __name__ == "__main__":
    entries = parse_help_index(ROOT / "lib/information/help_index")
    print(f"Parsed {len(entries)} help entries")
    for title, text in entries[:5]:
        print(f"  {title!r} ({len(text.encode('utf-8'))} bytes)")
