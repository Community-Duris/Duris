#!/usr/bin/env python3
"""Exercise the real shared worker/session adapter and #191 latency recorder."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

# Every expensive interactive path must be wired to the adapter. Runtime checks
# below use its real descriptor lifecycle, bounded worker, and latency recorder.
for path in ("account/account.c", "account/account_recovery_nanny.c",
             "net/ws_handlers.c", "item/storage_lockers.c", "sql/sql_player.c"):
    text = (SRC / path).read_text()
    assert "bcrypt_hash_password(" not in text, path
    assert "bcrypt_verify_password(" not in text, path
comm = (SRC / "net/comm.c").read_text()
assert comm.index("password_async_pulse(point)") < comm.index("get_casting_cmd_from_q(&point->input")
assert "password_async_cancel(d);" in comm
ws = (SRC / "net/ws_handlers.c").read_text()
assert "d->login_password_job || d->password_request" in ws

account = (SRC / "account/account.c").read_text()
def function(name):
    start = account.index("void " + name + "(")
    opening = account.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (account[end] == "{") - (account[end] == "}")
        end += 1
    return account[start:end]

with tempfile.TemporaryDirectory(prefix="duris-password-async-") as tmp:
    binary = Path(tmp) / "password_async"
    harness = Path(tmp) / "harness.cpp"
    harness.write_text((ROOT / "tests/async/password_async_harness.cpp").read_text() +
                       function("get_new_account_password") + "\n" +
                       function("verify_new_account_password"))
    subprocess.run([
        "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
        "-fsanitize=address,undefined", "-I", str(SRC), "-I/usr/include/mysql",
        str(harness),
        str(SRC / "account/password_hash.c"), str(SRC / "account/password_async.c"),
        str(SRC / "net/command_latency.c"), "-lcrypt", "-lcrypto", "-pthread",
        "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
print("password async runtime and interactive-path contracts passed")
