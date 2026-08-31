#!/usr/bin/env python3
"""Regression contract for assigning the Frag Lord title."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE = (SRC / "fraglist.c").read_text()

start = SOURCE.index("static void check_frag_position(")
end = SOURCE.index("// shows the frag list", start)
body = SOURCE[start:end]

leader_query = body[body.index('res = db_query("SELECT char_name') : body.index("if (res)")]

assert '"WHERE deleted_at IS NULL AND total_frags > 0 "' in leader_query
assert '"ORDER BY total_frags DESC, id ASC LIMIT 1"' in leader_query

print("frag leader title contract passed")
