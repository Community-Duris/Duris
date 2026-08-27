#!/usr/bin/env python3
"""Runtime and source contracts for private-chest password hardening."""

import ctypes
import hashlib
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
password_hash = (SRC / "password_hash.c").read_text()
sql_player = (SRC / "sql_player.c").read_text()
storage = (SRC / "storage_lockers.c").read_text()
account = (SRC / "account.c").read_text()
ws = (SRC / "ws_handlers.c").read_text()
header = (SRC / "password_hash.h").read_text()


def section(text: str, start_marker: str, end_marker: str) -> str:
    start = text.find(start_marker)
    assert start >= 0, start_marker
    end = text.find(end_marker, start)
    assert end >= 0, end_marker
    return text[start:end]


with tempfile.TemporaryDirectory(prefix="duris-chest-hash-") as temp_dir:
    library_path = Path(temp_dir) / "libpassword_hash.so"
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fPIC",
            "-shared",
            str(SRC / "password_hash.c"),
            "-o",
            str(library_path),
            "-lcrypt",
            "-lcrypto",
        ],
        check=True,
        cwd=ROOT,
    )
    library = ctypes.CDLL(str(library_path))
    libc = ctypes.CDLL(None)
    library.bcrypt_hash_password.argtypes = [ctypes.c_char_p]
    library.bcrypt_hash_password.restype = ctypes.c_void_p
    library.bcrypt_verify_password.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    library.bcrypt_verify_password.restype = ctypes.c_int
    library.is_bcrypt_hash.argtypes = [ctypes.c_char_p]
    library.is_bcrypt_hash.restype = ctypes.c_int
    library.password_verify_legacy_sha256.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    library.password_verify_legacy_sha256.restype = ctypes.c_int
    libc.free.argtypes = [ctypes.c_void_p]

    secret = b"correct horse battery staple"
    pointers = [library.bcrypt_hash_password(secret) for _ in range(2)]
    assert all(pointers)
    hashes = [ctypes.string_at(pointer) for pointer in pointers]
    for pointer in pointers:
        libc.free(pointer)

    assert hashes[0] != hashes[1]
    assert all(len(value) == 60 and value.startswith(b"$2b$12$") for value in hashes)
    assert library.bcrypt_verify_password(secret, hashes[0]) == 1
    assert library.bcrypt_verify_password(b"incorrect", hashes[0]) == 0
    assert library.is_bcrypt_hash(hashes[0]) == 1
    assert library.is_bcrypt_hash(b"$2b$12$short") == 0

    legacy = hashlib.sha256(secret).hexdigest().encode()
    assert library.password_verify_legacy_sha256(secret, legacy) == 1
    assert library.password_verify_legacy_sha256(secret, legacy.upper()) == 1
    assert library.password_verify_legacy_sha256(b"incorrect", legacy) == 0
    assert library.password_verify_legacy_sha256(secret, b"not-a-sha256-value") == 0
print("[PASS] bcrypt salts, cost, verification, and legacy SHA-256 runtime behavior")

assert 'crypt_gensalt_rn("$2b$", 12' in password_hash
assert "crypt_r(password" in password_hash
assert "CRYPTO_memcmp" in password_hash
assert "OPENSSL_cleanse" in password_hash
assert "#define BCRYPT_PASSWORD_MAX_BYTES 72" in header
assert '#include "password_hash.h"' in account
assert '#include "password_hash.h"' in ws
assert "char *bcrypt_hash_password" not in account
print("[PASS] shared hashing is reentrant, bounded, and consumed by account callers")

create = section(sql_player, "int sql_create_private_chest", "bool sql_delete_private_chest")
setter = section(sql_player, "bool sql_set_chest_password", "static bool sql_verify_chest_password_internal")
verify = section(sql_player, "static bool sql_verify_chest_password_internal", "int sql_count_private_chests")
assert "SHA2(" not in create + setter + verify
assert "sql_escape_string(password)" not in create + setter + verify
assert "bcrypt_hash_password(password)" in create
assert "bcrypt_hash_password(password)" in setter
assert create.index("bcrypt_hash_password(password)") < create.index("sql_escape_string(hash)")
assert setter.index("bcrypt_hash_password(password)") < setter.index("sql_escape_string(hash)")
assert create.count("BCRYPT_PASSWORD_MAX_BYTES") == 1
assert setter.count("BCRYPT_PASSWORD_MAX_BYTES") == 1
assert verify.count("BCRYPT_PASSWORD_MAX_BYTES") == 1
assert "sql_set_chest_password(chest_id, NULL)" in storage
assert "sql_set_chest_password(chest_id, arg3)" in storage
assert "Chest passwords must be at most 72 bytes." in storage
print("[PASS] create and reset hash before SQL and reject bcrypt-truncated inputs")

assert 'SELECT password_hash FROM private_chests WHERE id=%d' in verify
assert "row[0] == NULL" in verify
assert "bcrypt_verify_password(password, row[0])" in verify
assert "password_verify_legacy_sha256(password, row[0])" in verify
assert "WHERE id=%d AND password_hash='%s'" in verify
assert "mysql_affected_rows(DB) == 1" in verify
assert "sql_verify_chest_password_internal(chest_id, password, false)" in verify
assert "return true;" in verify
assert "SHA2(" not in storage
print("[PASS] opens verify in process and conditionally upgrade valid legacy hashes")

for relative in (
    "migrations/bootstrap_multithread_safe.sql",
    "migrations/pfile_to_db_combined_migration.sql",
    "migrations/run_migration.sh",
):
    schema = (ROOT / relative).read_text()
    assert "password_hash" in schema
    assert "password_hash VARCHAR(64)" in schema or "`password_hash` varchar(64)" in schema
print("[PASS] every authoritative schema already fits the 60-character bcrypt encoding")

print("private chest password hardening contracts passed")
