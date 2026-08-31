from _paths import SRC
from pathlib import Path

text = (SRC / "sql_pool.c").read_text()

assert "pool_closing" in text
assert "pool_closing = 1" in text
assert "pool_closing = 0" in text

acquire_start = text.index("MYSQL *sql_pool_acquire")
release_start = text.index("void sql_pool_release")
acquire_body = text[acquire_start:release_start]
assert "!pool || pool_closing" in acquire_body
assert "if (!pool || pool_closing)" in acquire_body

replace_start = text.index("MYSQL *sql_pool_replace_connection")
replace_body = text[replace_start:]
assert "pool_closing" in replace_body

shutdown_start = text.index("void sql_pool_shutdown")
close_start = text.index("mysql_close", shutdown_start)
wait_start = text.find("pthread_cond_wait(&pool_cond, &pool_mutex);", shutdown_start, close_start)
assert wait_start != -1, "shutdown must wait for borrowers before mysql_close"

print("sql pool closing-state checks passed")
