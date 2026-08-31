#!/usr/bin/env python3
"""Scalar fallback records must carry the replay discriminator."""
from _paths import SRC
from pathlib import Path
import re
from contract_text import contains

root = Path(__file__).resolve().parents[2]
utility = (SRC / "utility.c").read_text()
compact = re.sub(r"\s+", " ", utility)

assert contains(utility, "static const char *persistence_fallback_record_line")
assert contains(utility, "PERSISTENCE_SCALAR_EVENT_PREFIX")
assert contains(utility, "strcmp(domain, \"scalar_event\")")
assert contains(compact, "persistence_fallback_record_line(line, \"scalar_event\"")
assert contains(utility, "fputs(record_line, log_f)")
assert contains(utility, "const char *scalar_sql = event_line + strlen(PERSISTENCE_SCALAR_EVENT_PREFIX);")
assert contains(utility, "sql_persistence_write_scalar_event_line(scalar_sql)")

print("scalar fallback replay framing checks passed")
