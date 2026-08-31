from _paths import SRC
from pathlib import Path

text = (SRC / "sql_player.c").read_text()
start = text.rfind("bool sql_commit(void)")
end = text.find("bool sql_rollback(void)", start)
commit = text[start:end]
failure_start = commit.index("if (!sql_trace_exec")
failure_end = commit.index("return false;", failure_start)
failure = commit[failure_start:failure_end]

assert "sql_commit: failed" in failure
assert "in_transaction = false" not in failure
assert "sql_clear_account_character_cache_sync" not in failure

print("commit-failure transaction-state checks passed")
