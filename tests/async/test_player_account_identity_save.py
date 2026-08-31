#!/usr/bin/env python3
"""Source contract for transactional player/account identity persistence."""

from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "sql_player.c").read_text()
status = source.rsplit("bool sql_save_player_status", 1)[1].split(
    "// master save function", 1
)[0]
core_source = (SRC / "sql.c").read_text()
core = core_source.rsplit("int sql_save_player_core(P_char ch)", 1)[1].split(
    "/* Save a variable delta", 1
)[0]

identity = status.index("component=account_identity")
batch = status.index("// batched array saves")
commit = status.rindex("sql_commit()")

assert "ch->desc->account->acct_name" in status
assert "UPDATE player_data SET account_name='%s' WHERE pid=%d" in status
assert identity < batch < commit
assert "!sql_run_query(query)" in status[status.rfind("account_written", 0, identity) : identity]
assert "UPDATE player_data SET active=1,account_name='%s' WHERE pid=%d" in core
assert "ch->desc->account->acct_name" in core
assert core.index("account_name") < core.index("sql_update_account_character(ch)")

print("player account identity save contracts passed")
