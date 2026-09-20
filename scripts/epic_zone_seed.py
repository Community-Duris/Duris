#!/usr/bin/env python3
"""Derive epic-zone data from tracked sources; emit SQL, never connect to a DB."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "migrations/seeds/epic_zone_payouts.json"


def parse_stones(text: str) -> dict[int, int]:
    result = {}
    for object_name, type_name in [("EPIC_SMALL_STONE", "EPIC_ZONE_TYPE_SMALL"),
                                   ("EPIC_LARGE_STONE", "EPIC_ZONE_TYPE_LARGE"),
                                   ("EPIC_MONOLITH", "EPIC_ZONE_TYPE_MONOLITH")]:
        values = []
        for name in [object_name, type_name]:
            matches = re.findall(r"^#define\s+" + name + r"\s+(\d+)\s*$", text, re.M)
            if len(matches) != 1 or int(matches[0]) <= 0:
                raise ValueError(f"missing/ambiguous numeric stone definition: {name}")
            values.append(int(matches[0]))
        if values[0] in result:
            raise ValueError("duplicate stone object definition")
        result[values[0]] = values[1]
    return result


def parse_payouts(text: str) -> dict[int, int]:
    """Fail closed if the legacy, explicit-value source changes grammar/policy."""
    payouts = {}
    group_seen = False
    for raw in text.splitlines():
        line = raw.split("--", 1)[0].strip()
        if not line:
            continue
        if line == "UPDATE zones SET suggested_group_size = 100 WHERE epic_type != '0';":
            if group_seen:
                raise ValueError("duplicate group-size policy")
            group_seen = True
            continue
        match = re.fullmatch(r"UPDATE zones SET epic_payout = (\d+) WHERE number = (\d+);", line)
        if not match:
            raise ValueError("unrecognized payout source statement")
        payout, number = map(int, match.groups())
        if number in payouts:
            raise ValueError(f"duplicate payout assignment: {number}")
        payouts[number] = payout
    if not group_seen or not payouts:
        raise ValueError("missing payout/group-size source policy")
    return payouts


def parse_zones(text: str, source: str, stones: dict[int, int]) -> dict[int, dict]:
    zones = {}
    current = None
    lines = text.splitlines()
    for index, raw in enumerate(lines):
        line = raw.strip()
        if line.startswith("#"):
            if not re.fullmatch(r"#\d+", line) or index + 1 >= len(lines):
                raise ValueError(f"invalid zone header in {source}")
            number = int(line[1:])
            if number in zones:
                raise ValueError(f"duplicate zone number {number} in {source}")
            name = lines[index + 1].strip()
            if not name.endswith("~"):
                raise ValueError(f"invalid zone name in {source}")
            current = {"number": number, "name": name[:-1], "source": source, "stones": []}
            zones[number] = current
        elif line and line.split()[0] in {"O", "G", "E", "P"}:
            fields = line.split()
            if len(fields) < 3 or not fields[2].isdigit():
                raise ValueError(f"invalid object load in {source}")
            stone = int(fields[2])
            if stone in stones:
                if current is None:
                    raise ValueError(f"stone before zone header in {source}")
                current["stones"].append(stone)
    return zones


def derive(root: Path = ROOT) -> dict:
    sources = {}

    def read(relative: str) -> str:
        data = (root / relative).read_bytes()
        sources[relative] = hashlib.sha256(data).hexdigest()
        # Area files are legacy single-byte text; numeric records are ASCII.
        return data.decode("latin-1")

    payouts = parse_payouts(read("migrations/epic-zone-payout.sql"))
    stones = parse_stones(read("src/world/epic.h"))
    areas = read("areas/AREA")
    zones = {}
    for line in areas.splitlines():
        if not line.strip() or line.startswith("*"):
            continue
        name = line.split()[0]
        if not re.fullmatch(r"[A-Za-z0-9_-]+", name):
            raise ValueError(f"invalid area-list entry: {name}")
        relative = f"areas/zon/{name}.zon"
        parsed = parse_zones(read(relative), relative, stones)
        if zones.keys() & parsed.keys():
            raise ValueError(f"duplicate active zone numbers in {relative}")
        zones.update(parsed)
    rows = []
    for number, zone in sorted(zones.items()):
        if not zone["stones"]:
            continue
        payout = payouts.get(number)
        status = "unresolved" if payout is None else "disabled" if payout == 0 else "known"
        rows.append({
            "number": number, "name": zone["name"], "source": zone["source"],
            "stone_loads": len(zone["stones"]),
            "epic_type": max(stones[stone] for stone in zone["stones"]),
            "status": status, "epic_payout": payout,
            "suggested_group_size": 100 if status == "known" else None,
        })
    if not rows:
        raise ValueError("no active stone-bearing zones")
    return {
        "format_version": 1,
        "policy": "seed missing rows and exact (payout=0, group=1) defaults only; preserve all overrides",
        "sources": dict(sorted(sources.items())),
        "unseeded_assignments": [
            {"number": number, "epic_payout": payout,
             "reason": "no stone load in active area sources"}
            for number, payout in sorted(payouts.items())
            if number not in {row["number"] for row in rows}
        ],
        "zones": rows,
    }


def serialized(manifest: dict) -> str:
    return json.dumps(manifest, indent=2, ensure_ascii=True) + "\n"


def literal(value) -> str:
    if value is None:
        return "NULL"
    if isinstance(value, int):
        return str(value)
    # Independent of SQL mode (NO_BACKSLASH_ESCAPES) and client charset.
    return "CONVERT(X'" + value.encode("utf-8").hex() + "' USING utf8mb4)"


def seed_select(rows: list[dict]) -> str:
    columns = ("number", "name", "epic_type", "epic_payout", "suggested_group_size", "status")
    return "\nUNION ALL\n".join(
        "SELECT " + ", ".join(literal(row[c]) + (f" AS {c}" if i == 0 else "")
                                 for c in columns)
        for i, row in enumerate(rows)
    )


def audit_sql(manifest: dict) -> str:
    audit_rows = manifest["zones"] + [
        {"number": row["number"], "name": row["reason"], "epic_type": 0,
         "epic_payout": row["epic_payout"], "suggested_group_size": None,
         "status": "unseeded"}
        for row in manifest["unseeded_assignments"]
    ]
    return """-- Read-only epic-zone seed audit; overrides are reported, never overwritten.
WITH seed AS (
""" + seed_select(audit_rows) + """
)
SELECT s.number, s.name, s.status AS source_status,
       s.epic_payout AS expected_payout, s.suggested_group_size AS expected_group_size,
       COUNT(z.id) AS actual_rows, MIN(z.epic_payout) AS actual_payout,
       MIN(z.suggested_group_size) AS actual_group_size,
       CASE
         WHEN COUNT(z.id) > 1 THEN 'duplicate-number'
         WHEN s.status = 'unseeded' THEN 'outside-active-source-no-seed'
         WHEN s.status = 'unresolved' THEN 'unresolved-no-seed'
         WHEN s.status = 'disabled' THEN 'disabled-no-seed'
         WHEN COUNT(z.id) = 0 THEN 'missing'
         WHEN MIN(z.epic_payout) = s.epic_payout
          AND MIN(z.suggested_group_size) = s.suggested_group_size THEN 'matches'
         WHEN MIN(z.epic_payout) = 0 AND MIN(z.suggested_group_size) = 1 THEN 'default-repairable'
         ELSE 'override-preserved'
       END AS seed_status
FROM seed s LEFT JOIN zones z ON z.number = s.number
GROUP BY s.number, s.name, s.status, s.epic_payout, s.suggested_group_size
ORDER BY s.number;
"""


def apply_sql(manifest: dict, database: str) -> str:
    if not re.fullmatch(r"[A-Za-z0-9_]+", database):
        raise ValueError("database must contain only letters, digits, and underscore")
    rows = [row for row in manifest["zones"] if row["status"] == "known"]
    # Temporary tables do not implicitly commit. Duplicate-key guard works on
    # both MySQL and MariaDB without routines or version-dependent CHECK rules.
    return f"""-- Explicit operator action only: verified backup, stopped server, no other writers required.
-- Never run with mysql --force. Only known source values and exact defaults are seeded.
CREATE TEMPORARY TABLE _duris_epic_seed (
  number INT PRIMARY KEY, name VARCHAR(100) CHARACTER SET utf8mb4 NOT NULL,
  epic_type INT NOT NULL, epic_payout INT NOT NULL, suggested_group_size INT NOT NULL,
  status VARCHAR(16) NOT NULL
) ENGINE=InnoDB;
INSERT INTO _duris_epic_seed
{seed_select(rows)};
CREATE TEMPORARY TABLE _duris_epic_seed_guard (ok INT PRIMARY KEY) ENGINE=InnoDB;
INSERT INTO _duris_epic_seed_guard VALUES (1);
SET TRANSACTION ISOLATION LEVEL SERIALIZABLE;
START TRANSACTION;
-- Defense-in-depth locking; not a substitute for stopping all other writers.
-- The number index is not unique and its access plan is optimizer-dependent.
SELECT number AS locked_zone_number FROM zones ORDER BY number FOR UPDATE;
SET @duris_epic_seed_ok = (
  COALESCE(CAST(DATABASE() AS BINARY) = CAST('{database}' AS BINARY), 0)
  AND (SELECT COUNT(*) FROM information_schema.tables
       WHERE table_schema = DATABASE() AND table_name = 'zones' AND engine = 'InnoDB') = 1
  AND NOT EXISTS (SELECT number FROM zones GROUP BY number HAVING COUNT(*) > 1)
);
-- A failed target/engine/uniqueness guard raises a duplicate-key error. The
-- DML also requires a passing guard, even with an unsafe --force client.
-- This does NOT make --force safe after a later SQL error: fail-fast is mandatory.
INSERT INTO _duris_epic_seed_guard SELECT 1 WHERE NOT @duris_epic_seed_ok;
UPDATE zones z JOIN _duris_epic_seed s ON s.number = z.number
SET z.epic_payout = s.epic_payout, z.suggested_group_size = s.suggested_group_size
WHERE @duris_epic_seed_ok AND z.epic_payout = 0 AND z.suggested_group_size = 1;
SELECT ROW_COUNT() AS repaired_default_rows;
INSERT INTO zones (number, name, epic_type, epic_payout, suggested_group_size)
SELECT s.number, s.name, s.epic_type, s.epic_payout, s.suggested_group_size
FROM _duris_epic_seed s LEFT JOIN zones z ON z.number = s.number
WHERE @duris_epic_seed_ok AND z.id IS NULL;
SELECT ROW_COUNT() AS inserted_missing_rows;
COMMIT;
DROP TEMPORARY TABLE _duris_epic_seed_guard;
DROP TEMPORARY TABLE _duris_epic_seed;
"""


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=["manifest", "check", "write", "sql-audit", "sql-apply"])
    parser.add_argument("--database", help="exact target database bound into generated application SQL")
    parser.add_argument("--server-stopped", action="store_true", help="acknowledge operator stop requirement")
    args = parser.parse_args()
    if args.command == "sql-apply" and (not args.database or not args.server_stopped):
        parser.error("sql-apply requires --database and --server-stopped; see docs/operations/EPIC_ZONE_SEED.md")
    if args.command != "sql-apply" and (args.database or args.server_stopped):
        parser.error("database/stopped arguments are only valid for sql-apply")
    try:
        manifest = derive()
        text = serialized(manifest)
        if args.command == "manifest":
            print(text, end="")
        elif args.command == "write":
            MANIFEST.parent.mkdir(parents=True, exist_ok=True)
            MANIFEST.write_text(text)
        else:
            if MANIFEST.read_text() != text:
                raise ValueError("seed manifest differs from source; review changes and run 'write'")
            if args.command == "sql-audit":
                print(audit_sql(manifest), end="")
            elif args.command == "sql-apply":
                print(apply_sql(manifest, args.database), end="")
            else:
                print("epic-zone seed matches tracked sources")
    except (ValueError, OSError) as error:
        print(f"epic-zone seed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
