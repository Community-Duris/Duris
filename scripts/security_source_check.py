#!/usr/bin/env python3
"""Fail on repository security-baseline regressions with no external scanner."""

import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def tracked_files() -> list[Path]:
    output = subprocess.run(
        ["git", "ls-files", "-z"], cwd=ROOT, check=True, capture_output=True
    ).stdout
    return [ROOT / value.decode() for value in output.split(b"\0") if value]


failures: list[str] = []
security = (ROOT / "SECURITY.md").read_text()
if "security/advisories/new" not in security or "three business days" not in security:
    failures.append("SECURITY.md lacks the supported private reporting contract")
if "Use this section to tell people" in security:
    failures.append("SECURITY.md still contains template text")

dependabot = (ROOT / ".github/dependabot.yml").read_text()
if "package-ecosystem: github-actions" not in dependabot or 'package-ecosystem: ""' in dependabot:
    failures.append("Dependabot does not contain the applicable GitHub Actions ecosystem")

for workflow in sorted((ROOT / ".github/workflows").glob("*.yml")):
    for number, line in enumerate(workflow.read_text().splitlines(), 1):
        match = re.search(r"\buses:\s*([^\s]+)", line)
        if not match or match.group(1).startswith("./"):
            continue
        reference = match.group(1).rsplit("@", 1)[-1]
        if not re.fullmatch(r"[0-9a-f]{40}", reference):
            failures.append(f"{workflow.relative_to(ROOT)}:{number} has mutable action reference")

private_key_markers = (b"BEGIN " + b"PRIVATE KEY", b"BEGIN RSA " + b"PRIVATE KEY")
for path in tracked_files():
    if not path.is_file():
        continue
    data = path.read_bytes()
    relative = path.relative_to(ROOT).as_posix()
    if any(marker in data for marker in private_key_markers):
        failures.append(f"{relative} contains a tracked private key")

sql_header = (ROOT / "src/sql.h").read_text()
for token in ("DB_HOST_DEFAULT", "DB_USER_DEFAULT", "DB_PASSWD_DEFAULT", "DB_NAME_DEFAULT"):
    if token in sql_header:
        failures.append(f"src/sql.h restores compiled database default {token}")

production_import = (ROOT / "scripts/import_help_to_prod.sh").read_text()
for token in ("${DB_USER:-", "${DB_PASSWD:-"):
    if token in production_import:
        failures.append(f"production import restores database fallback {token}")
if re.search(r"mysql[^\n]*-p(?:\$|\{)", production_import):
    failures.append("production import exposes a database password in command arguments")
if "/tmp/import_sql.tmp" in production_import:
    failures.append("production import uses a predictable remote SQL temporary file")
if re.search(r"^\s*set\s+-a\s*$", production_import, re.MULTILINE):
    failures.append("production import exports every value sourced from .env")

epic_payout_migration = (ROOT / "migrations/epic-zone-payout.sql").read_text()
if re.search(r"mysql\s+-uduris\s+-pduris", epic_payout_migration):
    failures.append("epic payout migration documents public database credentials")

chest_sources = (ROOT / "src/sql_player.c").read_text() + (ROOT / "src/storage_lockers.c").read_text()
if re.search(r"password_hash\s*=\s*SHA2|password_hash[^\n]+SHA2", chest_sources):
    failures.append("private chest SQL restores unsalted SHA2 password handling")

if failures:
    for failure in failures:
        print(f"[FAIL] {failure}")
    raise SystemExit(f"security source check failed with {len(failures)} finding(s)")

print("security source/configuration checks passed")
