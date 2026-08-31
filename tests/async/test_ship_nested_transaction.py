from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "sql_player.c").read_text()
start = source.index("bool sql_save_ship(P_ship ship)")
end = source.index("static bool sql_load_ship_armor", start)
section = source[start:end]

assert "bool own_transaction = false" in section
assert "if (!sql_in_transaction())" in section
assert "own_transaction = true" in section
assert "if (own_transaction && !sql_commit())" in section
assert "if (own_transaction)\n\t\t\tsql_rollback();" in section
assert "if (sql_in_transaction())\n\t\t\tsql_rollback();" in section

print("ship nested transaction checks passed")
