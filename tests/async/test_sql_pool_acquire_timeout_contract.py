from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
h = (ROOT / "src" / "sql_pool.h").read_text()
pool = (ROOT / "src" / "sql_pool.c").read_text()
sql = (ROOT / "src" / "sql.c").read_text()

assert "SQL_POOL_ACQUIRE_TIMEOUT_MS" in h
assert "sql_pool_acquire_with_status" in h
assert "pthread_cond_timedwait" in pool
assert "ETIMEDOUT" in pool
assert "pool_was_active" in sql
assert "if (pool_was_active)" in sql
