#!/usr/bin/env python3
"""Scalar fallback records must carry the replay discriminator."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
utility = (root / "src/utility.c").read_text()
compact = re.sub(r"\s+", " ", utility)

assert "static const char *persistence_fallback_record_line" in utility
assert "PERSISTENCE_SCALAR_EVENT_PREFIX" in utility
assert "strcmp(domain, \"scalar_event\")" in utility
assert "persistence_fallback_record_line(line, \"scalar_event\"" in compact
assert "fputs(record_line, log_f)" in utility
assert "const char *scalar_sql = event_line + strlen(PERSISTENCE_SCALAR_EVENT_PREFIX);" in utility
assert "sql_persistence_write_scalar_event_line(scalar_sql)" in utility

print("scalar fallback replay framing checks passed")
