#!/usr/bin/env python3
"""Source contract for the early database compatibility boot gate."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CYCLE = (ROOT / "scripts" / "cycle_mud.sh").read_text()
SQL_PLAYER = (ROOT / "src" / "sql_player.c").read_text()

migration = 'python3 scripts/migration_runner.py run'
verification = './migrations/verify_runtime_compatibility.sh'
launch = '"$RUNTIME_BINARY" "${SERVER_ARGS[@]}" "${MUD_PORT}"'

assert 'export DB_NAME="$EFFECTIVE_DB_NAME"' in CYCLE
assert '[[ "$ENVIRONMENT" == "local" ]]' in CYCLE
assert migration in CYCLE
assert verification in CYCLE
assert CYCLE.index(migration) < CYCLE.index(launch)
assert CYCLE.index(verification) < CYCLE.index(launch)
assert "Database schema is incompatible with this server; refusing to boot" in CYCLE

restore = SQL_PLAYER[SQL_PLAYER.index("void sql_restore_saved_items(void)") :]
assert "root restore query failed; saved ground items were not loaded" in restore
assert restore.index("if (!result)") < restore.index("root restore query failed")

print("database boot gate contract passed")
