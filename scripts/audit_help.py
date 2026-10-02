#!/usr/bin/env python3
"""Report help coverage and source quality. Never connect to or write a database."""

import argparse
from collections import Counter
import csv
from dataclasses import asdict
import difflib
import hashlib
import io
import json
from pathlib import Path
import re
import sys

from parse_help_index import HelpEntry, ROOT, read_help_index, read_parsed_help


ASCII_LOWER = str.maketrans("ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz")
C_STRING = r'"(?:\\.|[^"\\])*"'


def canonical(value):
    return value.translate(ASCII_LOWER)


def strip_comments(text):
    """Keep strings and line numbers while excluding disabled registrations."""
    pattern = C_STRING + r"|'(?:\\.|[^'\\])*'|//[^\n]*|/\*[\s\S]*?\*/"
    return re.sub(pattern, lambda m: re.sub(r"[^\n]", " ", m[0])
                  if m[0].startswith(("//", "/*")) else m[0], text)


def table(text, name):
    found = re.search(r"\b" + name + r"\s*\[[^;]*?=\s*\{(.*?)\n\};", text, re.S)
    if not found:
        raise ValueError(f"Cannot find source table {name}; update the auditor for this layout")
    return found.group(1), found.start(1)


def strings(text):
    return [(json.loads(m[0]), m.start()) for m in re.finditer(C_STRING, text)]


def tracked_entries(root):
    """Follow the production loader's file list and last-writer-wins order."""
    source = strip_comments((root / "src/flatfile/flatfile_help_catalog.c").read_text())
    individual, _ = table(source, "individual_sources")
    entries = []
    for path, title in re.findall(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*\}', individual):
        filename = root / path
        if filename.exists():
            text = filename.read_bytes().decode("utf-8")
            if text:
                entries.append(HelpEntry(title, text, path, 1))
    for path, reader in (("lib/information/help_index", read_help_index),
                         ("help/duris_help_parsed.hlp", read_parsed_help)):
        if (root / path).exists():
            entries.extend(reader(root / path, path))
    if not entries:
        raise ValueError("No help sources found")
    return entries


def effective_catalog(entries):
    catalog = {}
    collisions = []
    for entry in entries:
        key = canonical(entry.title)
        if key in catalog:
            old = catalog[key]
            collisions.append({"title": entry.title,
                               "previous": f"{old.source}:{old.line}",
                               "winner": f"{entry.source}:{entry.line}",
                               "same_text": old.text == entry.text})
        catalog[key] = entry
    return catalog, collisions


def source_terms(root):
    """Extract registered names, not every token or commented-out feature."""
    terms = {}

    def add(kind, term, path, text, offset, **metadata):
        term = re.sub(r"&\+.|&[A-Za-z]", "", term).strip()
        if term and term not in ("None", "Not Used", "Undefined", "\n"):
            key = (kind, canonical(term))
            terms.setdefault(key, {"kind": kind, "term": term,
                                   "definition": f"{path}:{text.count(chr(10), 0, offset) + 1}",
                                   **metadata})

    path = "src/cmd/interp.c"
    text = strip_comments((root / path).read_text())
    names, start = table(text, "command")
    names = strings(names)
    defines = dict(re.findall(r"^#define\s+(CMD_\w+)\s+(\d+)\b",
                              (root / "src/cmd/interp.h").read_text(), re.M))
    registered = set()
    assignments = re.finditer(r"\b(CMD_[A-Z_]+)\s*\((CMD_\w+)\s*,([^;]*?)\)\s*;", text, re.S)
    for match in assignments:
        macro, symbol, arguments = match.groups()
        if symbol not in defines:
            raise ValueError(f"Unresolved command number: {symbol}")
        index = int(defines[symbol]) - 1
        if not 0 <= index < len(names):
            raise ValueError(f"Command number outside table: {symbol}")
        name, _ = names[index]
        args = [arg.strip() for arg in arguments.split(",")]
        level = "0" if macro == "CMD_SOC" else args[0 if macro == "CMD_TRIG" else 2]
        kind = "social" if macro == "CMD_SOC" else (
            "trigger_command" if macro == "CMD_TRIG" else
            "staff_command" if macro == "CMD_GRT" or not level.isdigit() else "command")
        add(kind, name, path, text, match.start(), minimum_level=level,
            registration=macro, symbol=symbol,
            handler="do_action" if macro == "CMD_SOC" else
                    "do_not_here" if macro == "CMD_TRIG" else args[1])
        registered.add(index)
    for index, (name, offset) in enumerate(names):
        if index not in registered:
            add("unregistered_command", name, path, text, start + offset)

    path = "src/classes/skills.c"
    text = strip_comments((root / path).read_text())
    for match in re.finditer(r"\b((?:SPELL|SKILL)_CREATE\w*)\s*\(\s*(" + C_STRING + r")\s*,\s*(\w+)", text):
        macro, name, symbol = match.groups()
        kind = "song" if symbol.startswith("SONG_") else "spell" if macro.startswith("SPELL") else "skill"
        add(kind, json.loads(name),
            path, text, match.start(), symbol=symbol)

    path = "src/core/common.c"
    text = strip_comments((root / path).read_text())
    for name, kind in (("race_names_table", "race"), ("class_names_table", "class")):
        body, start = table(text, name)
        values = [(json.loads(m[1]), start + m.start())
                  for m in re.finditer(r"\{\s*(" + C_STRING + r")", body)]
        if kind == "race":
            defines_text = (root / "src/core/defines.h").read_text()
            limit_symbol = re.search(r"#define\s+RACE_PLAYER_MAX\s+(\w+)", defines_text)[1]
            limit = int(re.search(r"#define\s+" + limit_symbol + r"\s+(\d+)", defines_text)[1])
            values = values[1:limit + 1]
        for value, offset in values:
            add(kind, value, path, text, offset)
            if kind == "class" and value != "None":
                add("class_skillset", value + " Skills", path, text, offset)
    body, start = table(text, "specdata")
    for value, offset in strings(body):
        add("specialization", value, path, text, start + offset)

    path = "src/classes/innates.c"
    text = strip_comments((root / path).read_text())
    body, start = table(text, "innates_data")
    for match in re.finditer(r"\{\s*(" + C_STRING + r")", body):
        add("innate", json.loads(match[1]), path, text, start + match.start())

    path = "docs/lib/information/command_attributes.txt"
    text = (root / path).read_text()
    begin = 0
    for block in text.split("~"):
        heading = re.search(r"\S[^\n]*", block)
        if heading:
            add("command_attribute", heading[0].strip(), path, text, begin + heading.start())
        begin += len(block) + 1

    # Only explicit quoted instructions are evidence of a static help query.
    # Exclude templates such as 'help <keyword>' and speculative prose tokens.
    for filename in sorted((root / "src").rglob("*.c")):
        text = strip_comments(filename.read_text(encoding="utf-8"))
        if "help " not in canonical(text):
            continue
        path = filename.relative_to(root).as_posix()
        for literal in re.finditer(C_STRING, text):
            if "help " not in canonical(literal[0]):
                continue
            try:
                value = json.loads(literal[0])
            except ValueError:
                continue  # C-only escape syntax is outside this narrow scan.
            for hint in re.finditer(r'''['"]help\s+([A-Za-z][A-Za-z0-9 _-]*)['"]''', value, re.I):
                add("help_hint", hint[1], path, text, literal.start())
    return sorted(terms.values(), key=lambda row: (row["kind"], canonical(row["term"])))


def redirect_target(entry):
    text = entry.text.strip()
    return text[len("Redirect:"):].strip() if text.startswith("Redirect:") else None


def resolve(catalog, title, categories=None):
    seen = set()
    for _ in range(8):
        key = canonical(title)
        if key in seen:
            return "cycle", title
        seen.add(key)
        entry = catalog.get(key)
        if entry is None:
            return "missing_target", title
        target = redirect_target(entry)
        if target is None:
            return "ok", entry.title
        if categories is not None and (categories.get(key) != 1 or
                                       not entry.text.startswith("Redirect: ")):
            return "inactive_redirect", entry.title
        title = target
    return "depth_limit", title


def coverage_row(term, catalog, bodies=None, categories=None):
    key = canonical(term["term"])
    titles = [entry.title for name, entry in sorted(catalog.items()) if key in name]
    if key in catalog:
        outcome, target = resolve(catalog, term["term"], categories)
        status = "exact" if outcome == "ok" else "inactive_redirect" if outcome == "inactive_redirect" else "broken_redirect"
    else:
        status = "partial" if len(titles) == 1 else "ambiguous" if titles else "missing"
        target = None
    body_pattern = re.compile(r"(?<!\w)" + re.escape(key) + r"(?!\w)")
    body_hits = [catalog[name].title for name, text in bodies.items()
                 if body_pattern.search(text)] if bodies and status != "exact" else []
    suggestions = difflib.get_close_matches(key, sorted(catalog), n=3, cutoff=0.65) if not titles else []
    return {**term, "status": status, "matches": titles, "resolved_title": target,
            "body_mentions": body_hits[:5], "body_mention_count": len(body_hits),
            "suggestions": [catalog[name].title for name in suggestions]}


def references(text):
    # Colorized headings and links are still authored references.
    text = re.sub(r"&\+.|&[A-Za-z]", "", text)
    targets = set(re.findall(r"\[\[([^\]]+)\]\]", text))
    for inline in re.finditer(r"^\s*See\s+also:\s*([^\n]+)", text, re.M | re.I):
        targets.update(part.strip() for part in inline[1].split(","))
    for section in re.finditer(r"==\s*See also\s*==\s*\n(.*?)(?=\n\s*\n|\n==|\nThe following|$)", text, re.S | re.I):
        for line in section[1].splitlines():
            targets.update(part.strip().lstrip("* ") for part in line.split(","))
    return sorted({re.sub(r"&\+.|&[A-Za-z]", "", title.split("|", 1)[0]).strip()
                   for title in targets if title.strip()})


def audit(root, pages_file=None):
    invalid_pages = []
    duplicates = set()
    categories = {}
    if pages_file:
        content = pages_file.read_text(encoding="utf-8")
        pages = json.loads(content) if content.lstrip().startswith("[") else [
            json.loads(line) for line in content.splitlines() if line.strip()]
        entries = []
        for index, page in enumerate(pages, 1):
            if not isinstance(page, dict):
                raise ValueError("Each exported page must be a JSON object")
            title, text = page.get("title"), page.get("text")
            if not isinstance(title, str) or not title or not isinstance(text, str) or \
                    page.get("category_id") is None or len(title.encode("utf-8")) > 256:
                invalid_pages.append({"row": index, "title": title,
                                      "reason": "Invalid title, text, or category for the MySQL loader"})
            if isinstance(title, str) and title:
                entries.append(HelpEntry(title, text if isinstance(text, str) else "", pages_file.name, index))
                key = canonical(title)
                if key in categories:
                    duplicates.add(key)
                categories[key] = page.get("category_id")
        source_kind = "pages_export"
    else:
        entries = tracked_entries(root)
        source_kind = "tracked_flatfile"
    catalog, collisions = effective_catalog(entries)
    bodies = {name: canonical(entry.text) for name, entry in sorted(catalog.items())}
    coverage = [coverage_row(term, catalog, bodies, categories if pages_file else None)
                for term in source_terms(root)]
    inactive_redirects = {name for name, entry in catalog.items() if redirect_target(entry) is not None and
                          (categories.get(name) != 1 or not entry.text.startswith("Redirect: "))} if pages_file else set()
    for row in coverage:
        key = canonical(row["term"])
        if key in duplicates:
            row["status"] = "duplicate_title"
        elif key in inactive_redirects:
            row["status"] = "inactive_redirect"
    quality = {"captured_search_output": [], "embedded_edit_header": [],
               "redirect_problems": [], "unresolved_links": [],
               "invalid_database_pages": invalid_pages,
               "duplicate_database_titles": sorted(duplicates)}
    for _, entry in sorted(catalog.items()):
        location = {"title": entry.title, "source": f"{entry.source}:{entry.line}"}
        if "The following help topics" in entry.text:
            quality["captured_search_output"].append(location)
        if re.search(r"^.* - Last Edited:", entry.text, re.M):
            quality["embedded_edit_header"].append(location)
        target = redirect_target(entry)
        if target is not None:
            status, resolved = resolve(catalog, entry.title)
            # Tracked imports always use category 0, so MySQL cannot follow
            # these redirects without an explicit category correction.
            category = categories.get(canonical(entry.title)) if pages_file else 0
            mysql_status, _ = resolve(catalog, entry.title, categories if pages_file else {})
            if status != "ok" or mysql_status != "ok":
                quality["redirect_problems"].append({**location, "target": target,
                    "flatfile_resolution": status, "mysql_category": category,
                    "mysql_resolution": mysql_status, "resolved_title": resolved})
        for target in references(entry.text):
            row = coverage_row({"term": target}, catalog, categories=categories if pages_file else None)
            if canonical(target) in duplicates:
                row["status"] = "duplicate_title"
            elif canonical(target) in inactive_redirects:
                row["status"] = "inactive_redirect"
            if row["status"] != "exact":
                quality["unresolved_links"].append({**location, "target": target,
                                                   "status": row["status"], "matches": row["matches"]})
    summary = {kind: dict(Counter(row["status"] for row in coverage if row["kind"] == kind))
               for kind in sorted({row["kind"] for row in coverage})}
    catalog_rows = [{"title": entry.title, "source": f"{entry.source}:{entry.line}",
                     "bytes": len(entry.text.encode("utf-8")),
                     "sha256": hashlib.sha256(entry.text.encode("utf-8")).hexdigest()}
                    for _, entry in sorted(catalog.items())]
    if pages_file:
        # Duplicate DB rows have no stable runtime winner: ORDER BY title
        # does not order ties. Only flat-file precedence determines a winner.
        for collision in collisions:
            collision["later_row"] = collision.pop("winner")
    return {"catalog_source": source_kind, "source_entries": len(entries),
            "effective_titles": len(catalog), "summary": summary,
            "quality_counts": {key: len(value) for key, value in quality.items()},
            "coverage": coverage, "collisions": collisions, "quality": quality,
            "catalog": catalog_rows}


def csv_report(report):
    output = io.StringIO(newline="")
    fields = ["kind", "term", "status", "definition", "matches", "resolved_title",
              "body_mention_count", "body_mentions", "suggestions"]
    writer = csv.DictWriter(output, fields, extrasaction="ignore", lineterminator="\n")
    writer.writeheader()
    for row in report["coverage"]:
        writer.writerow({key: " | ".join(value) if isinstance(value, list) else value
                         for key, value in row.items()})
    return output.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--pages-json", type=Path, help="Explicit read-only pages JSON array or JSONL export")
    parser.add_argument("--format", choices=("json", "csv"), default="json")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--require-term", action="append", default=[],
                        help="Fail unless this keyword has an exact usable topic; repeatable")
    args = parser.parse_args()
    try:
        report = audit(args.root, args.pages_json)
        output = csv_report(report) if args.format == "csv" else json.dumps(report, indent=2, ensure_ascii=False) + "\n"
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(output, encoding="utf-8", newline="\n")
        else:
            sys.stdout.write(output)
        titles = {canonical(row["title"]) for row in report["catalog"]}
        broken = {canonical(row["title"]) for row in report["quality"]["redirect_problems"]
                  if row["flatfile_resolution"] != "ok" or (args.pages_json and row["mysql_resolution"] != "ok")}
        broken.update(report["quality"]["duplicate_database_titles"])
        broken.update(canonical(row["title"]) for row in report["quality"]["invalid_database_pages"]
                      if isinstance(row["title"], str))
        failed = [term for term in args.require_term if canonical(term) not in titles or canonical(term) in broken]
        if failed:
            print("Exact usable help required for: " + ", ".join(failed), file=sys.stderr)
            return 1
        return 0
    except (OSError, ValueError, KeyError, IndexError) as error:
        print(f"Help audit failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
