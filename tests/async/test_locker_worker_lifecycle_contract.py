from _paths import SRC
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
src = (SRC / "locker_async.c").read_text()

assert "g_worker_created" in src
assert "locker_async_worker_available" in src
assert "!g_inited || !locker_async_worker_available()" in src
assert "join_created = g_worker_created" in src
assert "pthread_join(g_worker_tid, NULL)" in src
assert "g_worker_created = 0" in src
assert "Creation succeeded, so join ownership" in src
