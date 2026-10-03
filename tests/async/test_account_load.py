#!/usr/bin/env python3
"""Executable account worker/session and transaction tests; no database connection."""
from pathlib import Path
import subprocess
import tempfile
from _paths import extract_function

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
account = (SRC / "account/account.c").read_text()
comm = (SRC / "net/comm.c").read_text()
ws = (SRC / "net/ws_handlers.c").read_text()
select = extract_function("account.c", "void select_accountname(")
login = extract_function("ws_handlers.c", "void ws_cmd_login(")
# Only the explicitly retained flat-file branches may use synchronous reads.
for handler in (select, login):
    mysql = handler[handler.index("#ifndef __NO_MYSQL__"):handler.index("#else", handler.index("#ifndef __NO_MYSQL__"))]
    assert "read_account(" not in mysql and "account_exists(" not in mysql
    assert "account_async_start(" in mysql
assert "account_async_cancel(d);" in extract_function("comm.c", "void close_socket(")
assert "account_load_worker_shutdown();" in comm
assert "account_async_pulse(descriptor)" in comm
assert "d->password_request || d->account_request" in ws

functions = "\n".join(extract_function("account.c", signature) for signature in (
    "void clear_account(", "char *check_and_clear(", "P_acct free_account(",
    "P_acct allocate_account(", "void add_account_to_list(", "void remove_account_from_list(",
    "void send_account_password_prompt(", "void select_accountname("))
build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="account-load-", dir=build) as temp:
    temp = Path(temp)
    runtime = temp / "runtime.cpp"
    runtime.write_text((ROOT / "tests/async/account_load_harness.cpp").read_text() +
                       "\n" + functions + "\n" + login)
    common = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
              "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
              "-I", str(SRC), "-I/usr/include/mysql"]
    binary = temp / "runtime"
    subprocess.run(common + [str(runtime), str(SRC / "account/account_load.c"),
                             str(SRC / "account/account_async.c"), str(SRC / "core/memory.c"),
                             "-lmysqlclient", "-lcjson", "-lcrypto", "-lbsd", "-pthread",
                             "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
    binary = temp / "repository"
    subprocess.run(common + [str(ROOT / "tests/async/account_load_repository_harness.cpp"),
                             str(SRC / "account/account_load.c"), "-lmysqlclient",
                             "-pthread", "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=30)
print("account load worker, session lifecycle, and transaction behavior passed")
