#!/usr/bin/env python3
"""Qualify native account token allocation, account cache and migration 56."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile
import unittest

from _source_contract import function_body

ROOT = Path(__file__).resolve().parents[2]
SQL = ROOT / "src/sql/sql_telemetry_account_identity.c"
RUNTIME = ROOT / "src/telemetry/telemetry_runtime.c"


def compile_run(source: str, *, flatfile: bool) -> None:
    artifacts = ROOT / "bin/tests"
    artifacts.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="telemetry-account-cache-", dir=artifacts) as directory:
        path = Path(directory)
        (path / "cache.cc").write_text(source)
        command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src")]
        if flatfile:
            command += ["-D__NO_MYSQL__"]
        else:
            command += shlex.split(subprocess.check_output(["mysql_config", "--cflags"], text=True))
        subprocess.run(command + [str(path / "cache.cc"), "-o", str(path / "cache")], check=True)
        subprocess.run([str(path / "cache")], check=True)


class AccountIdentityTests(unittest.TestCase):
    def test_account_load_is_the_only_preparation_call(self):
        account = (ROOT / "src/account/account.c").read_text()
        read = function_body(account, r"\bint\s+read_account\s*\(")
        self.assertIsNotNone(read)
        self.assertIn("acct->telemetry_account_token = 0U", read)
        self.assertLess(read.index("telemetry_account_token = 0U"), read.index("sql_load_account"))
        self.assertGreater(read.index("telemetry_runtime_account_prepare(acct)"), read.index("free(loaded)"))
        self.assertIn("(void)telemetry_runtime_account_prepare(acct);", read)
        self.assertEqual(RUNTIME.read_text().count("telemetry_runtime_account_prepare("), 1)
        self.assertEqual(account.count("telemetry_runtime_account_prepare("), 1)

    def test_lifetime_schema_retains_retired_bindings(self):
        ddl = (ROOT / "migrations/immutable/0056_telemetry_account_identity.sql").read_text()
        self.assertNotIn("AUTO_INCREMENT", ddl)
        self.assertIn("ON DELETE SET NULL ON UPDATE CASCADE", ddl)
        self.assertIn("UNIQUE KEY uq_telemetry_token_lifetime (environment_id,season_id,lifetime_id)", ddl)
        self.assertNotIn("created_at", ddl)

    def test_cache_executes_actual_runtime_helper(self):
        helper = function_body(RUNTIME.read_text(), r"\bbool\s+telemetry_runtime_account_prepare\s*\(")
        self.assertIsNotNone(helper)
        helper = "bool telemetry_runtime_account_prepare(struct acct_entry *account)\n" + helper
        source = '''
#include "account/account.h"
#include "telemetry/telemetry_runtime.h"
#include <cassert>
struct { bool initialized, enabled, shutdown_pending; std::uint64_t session_scope_environment_id, session_scope_season_id; } R;
static unsigned calls;
static bool publish;
bool sql_prepare_telemetry_account_token(const char *, std::uint64_t env, std::uint64_t season, std::uint64_t *token) {
    ++calls; assert(env == 7 && season == 11); *token = publish ? 99 : 0; return publish;
}
''' + helper + '''
int main() {
    acct_entry account{}; account.telemetry_account_token = 81;
    assert(!telemetry_runtime_account_prepare(nullptr));
    assert(!telemetry_runtime_account_prepare(&account));
    assert(!calls && !account.telemetry_account_token && !account.telemetry_environment_id);
    R = {true,true,false,7,11};
#ifdef __NO_MYSQL__
    publish = true; assert(!telemetry_runtime_account_prepare(&account)); assert(!calls);
#else
    assert(!telemetry_runtime_account_prepare(&account)); assert(calls == 1 && !account.telemetry_account_token);
    publish = true; assert(telemetry_runtime_account_prepare(&account));
    assert(account.telemetry_account_token == 99 && account.telemetry_environment_id == 7 && account.telemetry_season_id == 11);
    R.shutdown_pending = true; assert(!telemetry_runtime_account_prepare(&account));
    assert(calls == 2 && !account.telemetry_account_token && !account.telemetry_environment_id && !account.telemetry_season_id);
#endif
}
'''
        compile_run(source, flatfile=True)
        compile_run(source, flatfile=False)

    def test_client_free_allocator_keeps_identity_unknown(self):
        source = '''
#include "sql/sql_telemetry_account_identity.h"
#include <cassert>
''' + SQL.read_text() + '''
int main() { std::uint64_t token = 91; assert(!sql_prepare_telemetry_account_token("fixture",1,1,&token)); assert(token == 0); assert(!sql_prepare_telemetry_account_token(nullptr,0,0,nullptr)); }
'''
        compile_run(source, flatfile=True)


def sql_fixture():
    import pymysql
    from test_telemetry_repository import prepare_sql_fixture, drop_sql_fixture
    environment, command, database = prepare_sql_fixture()
    admin = None
    users = []
    executable = ROOT / "bin/tests/telemetry-account-identity-sql"
    executable.parent.mkdir(parents=True, exist_ok=True)
    try:
        flags = shlex.split(subprocess.check_output(["mysql_config", "--cflags", "--libs"], text=True))
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "src"),
                        str(ROOT / "tests/async/telemetry_account_identity_sql.cc"), str(SQL),
                        "-o", str(executable), *flags, "-lcrypto"], check=True)
        admin = pymysql.connect(host=environment["TELEMETRY_REPOSITORY_HOST"],
                                port=int(environment["TELEMETRY_REPOSITORY_PORT"]),
                                user=environment["TELEMETRY_REPOSITORY_USER"],
                                password=environment["TELEMETRY_REPOSITORY_PASSWORD"],
                                database=database, charset="utf8mb4", autocommit=True)
        def query(statement, args=None):
            with admin.cursor() as cursor:
                cursor.execute(statement, args)
                return cursor.fetchall()
        owner, report = "telemetry_identity_owner", "telemetry_identity_report"
        password = "synthetic-local-account-identity"
        for user in (owner, report):
            query("CREATE USER %s@'%%' IDENTIFIED BY %s", (user, password)); users.append(user)
        query(f"GRANT SELECT (account_name,blocked) ON `{database}`.accounts TO %s@'%%'", (owner,))
        for table in ("telemetry_account_lifetime", "telemetry_account_token"):
            query(f"GRANT SELECT,INSERT ON `{database}`.{table} TO %s@'%%'", (owner,))
        query(f"GRANT SELECT ON `{database}`.telemetry_rollup_progression_day TO %s@'%%'", (report,))
        native_environment = {**environment, "TELEMETRY_IDENTITY_USER": owner,
                              "TELEMETRY_IDENTITY_PASSWORD": password}
        def prepare(name, env=7, season=11, mode="normal", collision=0):
            run = subprocess.run([str(executable), name, str(env), str(season), mode, str(collision)],
                                 env=native_environment, capture_output=True, text=True, timeout=15)
            assert run.returncode == 0, run.stderr
            value = json.loads(run.stdout)
            assert value["token"] != 0 if value["accepted"] else value["token"] == 0
            return value
        def add_account(name, blocked=0):
            query("INSERT INTO accounts(account_name,blocked) VALUES (%s,%s)", (name, blocked))
        def counts():
            return query("SELECT (SELECT COUNT(*) FROM telemetry_account_lifetime),(SELECT COUNT(*) FROM telemetry_account_token)")[0]
        def accepted(value):
            assert value["accepted"] and value["commits"] == 1 and value["rollbacks"] == 0, value
            return value["token"]
        add_account("IdentityFixture")
        if "mysql" in environment["TELEMETRY_REPOSITORY_DB_IMAGE"]:
            missing_lock_grant = prepare("IdentityFixture")
            assert not missing_lock_grant["accepted"] and missing_lock_grant["last_error"] == 1142
            assert counts() == (0, 0)
        # MySQL FOR UPDATE requires a write/lock privilege on the native account
        # table. One column grant suffices; private identity stores need no UPDATE.
        query(f"GRANT UPDATE (account_name) ON `{database}`.accounts TO %s@'%%'", (owner,))
        original = accepted(prepare("identityfixture"))
        lifetime = query("SELECT lifetime_id FROM telemetry_account_lifetime WHERE account_name='IdentityFixture'")[0][0]
        for _ in range(3):
            retry = prepare("IDENTITYFIXTURE")
            assert accepted(retry) == original and retry["entropy_calls"] == 0
        assert counts() == (1, 1)
        second_season = accepted(prepare("IdentityFixture", season=12))
        second_environment = accepted(prepare("IdentityFixture", env=8))
        assert len({original, second_season, second_environment}) == 3 and counts() == (1, 3)
        query("UPDATE accounts SET account_name='RenamedFixture' WHERE account_name='IdentityFixture'")
        assert accepted(prepare("renamedfixture")) == original
        assert not prepare("IdentityFixture")["accepted"]
        query("DELETE FROM accounts WHERE account_name='RenamedFixture'")
        assert query("SELECT account_name FROM telemetry_account_lifetime WHERE lifetime_id=%s", (lifetime,)) == ((None,),)
        assert counts() == (1, 3)
        add_account("RenamedFixture")
        replacement = accepted(prepare("RenamedFixture"))
        assert replacement != original and counts() == (2, 4)
        assert not prepare("missingfixture")["accepted"]
        assert not prepare("")["accepted"]
        assert not prepare("RenamedFixture", env=0)["accepted"]
        assert not prepare("RenamedFixture", season=0)["accepted"]
        assert not prepare("x" * 201)["accepted"]
        add_account("DeletionFixture", blocked=2)
        assert not prepare("DeletionFixture")["accepted"]
        before = counts()
        outer = prepare("RenamedFixture", mode="outer_transaction")
        assert not outer["accepted"] and outer["outer_retained"] and outer["queries"] == 0 and outer["commits"] == 0
        assert counts() == before
        for mode in ("entropy_failure", "zero_entropy", "collision", "token_write_failure", "commit_failure", "allocation_failure"):
            name = "FaultFixture" + mode
            add_account(name)
            refused = prepare(name, mode=mode, collision=lifetime)
            assert not refused["accepted"] and refused["rollbacks"] == 1 and counts() == before
            if mode in ("zero_entropy", "collision"): assert refused["entropy_calls"] == 4
            if mode == "entropy_failure": assert refused["entropy_calls"] == 1
        # Existing lifetime: all four token collisions preserve the old scope.
        token_collision = prepare("RenamedFixture", mode="collision", collision=original)
        assert token_collision["accepted"] and token_collision["token"] == replacement  # Existing scope does not allocate.
        collided = prepare("RenamedFixture", env=8, mode="collision", collision=second_environment)
        assert not collided["accepted"] and collided["entropy_calls"] == 4 and counts() == before
        add_account("AmbiguousFixture")
        lost = prepare("AmbiguousFixture", mode="lost_commit_reply")
        assert not lost["accepted"] and lost["commits"] == 1 and lost["rollbacks"] == 1
        assert counts() == (before[0] + 1, before[1] + 1)
        recovered = prepare("AmbiguousFixture")
        ambiguous_token = accepted(recovered)
        assert recovered["entropy_calls"] == 0 and accepted(prepare("AmbiguousFixture")) == ambiguous_token
        assert counts() == (before[0] + 1, before[1] + 1)
        add_account("ConcurrentFixture")
        concurrent = [subprocess.Popen([str(executable), "ConcurrentFixture", "7", "11", "normal", "0"],
                                       env=native_environment, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                      for _ in range(2)]
        simultaneous = []
        for process in concurrent:
            output, errors = process.communicate(timeout=15)
            assert process.returncode == 0, errors
            simultaneous.append(accepted(json.loads(output)))
        assert simultaneous[0] == simultaneous[1]
        assert counts() == (before[0] + 2, before[1] + 2)
        add_account("Quote'Fixture")
        quote_token = accepted(prepare("Quote'Fixture"))
        assert accepted(prepare("Quote'Fixture")) == quote_token
        assert not prepare("unknown' OR 1=1 -- ")["accepted"]
        for role, denied in ((owner, ["UPDATE telemetry_account_lifetime SET account_name=NULL", "DELETE FROM telemetry_account_token", "SELECT email FROM accounts", "INSERT INTO accounts(account_name) VALUES ('DeniedFixture')"]),
                             (report, ["SELECT * FROM accounts", "SELECT * FROM telemetry_account_lifetime", "SELECT * FROM telemetry_account_token"])):
            connection = pymysql.connect(host=environment["TELEMETRY_REPOSITORY_HOST"], port=int(environment["TELEMETRY_REPOSITORY_PORT"]), user=role, password=password, database=database, autocommit=True)
            try:
                with connection.cursor() as cursor:
                    for statement in denied:
                        try: cursor.execute(statement)
                        except pymysql.MySQLError as failure: assert failure.args[0] in (1142,1143), failure
                        else: raise AssertionError("restricted identity role exceeded its grants")
            finally: connection.close()
        for statement in ("INSERT INTO telemetry_account_lifetime VALUES (0,NULL)",
                          f"INSERT INTO telemetry_account_lifetime VALUES ({lifetime},NULL)",
                          "INSERT INTO telemetry_account_token VALUES (0,11,1,1)",
                          "INSERT INTO telemetry_account_token VALUES (7,11,0,1)",
                          f"INSERT INTO telemetry_account_token VALUES (7,11,{replacement + 1},999)",
                          f"DELETE FROM telemetry_account_lifetime WHERE lifetime_id={lifetime}"):
            try: query(statement)
            except pymysql.MySQLError as failure: assert failure.args[0] in (1062,1451,1452,3819,4025), failure
            else: raise AssertionError("account identity constraint accepted invalid data")
        verifier = ["bash", "migrations/immutable/0056_telemetry_account_identity.sh"]
        verified = subprocess.run(verifier, env=environment, capture_output=True, text=True, timeout=30)
        assert verified.returncode == 0, verified.stderr
        engine = environment["TELEMETRY_REPOSITORY_DB_IMAGE"]
        drop = "DROP CONSTRAINT" if "mariadb" in engine else "DROP CHECK"
        query(f"ALTER TABLE telemetry_account_lifetime {drop} chk_telemetry_lifetime_nonzero")
        query("ALTER TABLE telemetry_account_lifetime ADD CONSTRAINT chk_telemetry_lifetime_nonzero CHECK (lifetime_id > 0)")
        refused = subprocess.run(verifier, env=environment, capture_output=True, text=True, timeout=30)
        assert refused.returncode != 0 and "checks differ" in refused.stderr, refused.stderr
        query(f"ALTER TABLE telemetry_account_lifetime {drop} chk_telemetry_lifetime_nonzero")
        query("ALTER TABLE telemetry_account_lifetime ADD CONSTRAINT chk_telemetry_lifetime_nonzero CHECK (lifetime_id <> 0)")
        # Measure only a fully verified chain; fingerprint changes require both engines.
        measured = subprocess.run(["bash", "migrations/verify_runtime_compatibility.sh", "--schema-only"],
                                  env=environment, capture_output=True, text=True, timeout=45)
        output = measured.stdout + measured.stderr
        match = re.search(r"normalized metadata fingerprint mismatch: expected=[0-9a-f]{64} actual=([0-9a-f]{64})", output)
        runtime = json.loads((ROOT / "migrations/runtime_compatibility_manifest.json").read_text())
        key = "mariadb10_11" if "mariadb" in engine else "mysql8"
        if match:
            assert measured.returncode != 0 and output.count("FAILED:") == 1, output
            fingerprint = match[1]
        else:
            assert measured.returncode == 0, output
            fingerprint = runtime["normalized_metadata_fingerprints"][key]
        artifact = dict(status="passed", engine=engine, migration_head=runtime["migration_head"]["id"], normalized_metadata_fingerprint=fingerprint,
                        qualification="native allocator, retry, scopes, rename, deletion/recreation, bounded entropy, rollback, lost commit reply, roles, constraints, verifier drift")
        suffix = "mariadb" if key == "mariadb10_11" else "mysql"
        (ROOT / f"bin/telemetry-account-identity-{suffix}.json").write_text(json.dumps(artifact, indent=2) + "\n")
        print(json.dumps(artifact), flush=True)
    finally:
        if admin is not None:
            with admin.cursor() as cursor:
                for user in users: cursor.execute("DROP USER IF EXISTS %s@'%%'", (user,))
            admin.close()
        drop_sql_fixture(environment, command, database)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sql-fixture", action="store_true")
    args = parser.parse_args()
    if args.sql_fixture:
        sql_fixture()
    else:
        unittest.main(argv=[__file__])
