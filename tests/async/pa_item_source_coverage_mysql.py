#!/usr/bin/env python3
"""Disposable SQL regressions for durable item-source capture and baselining."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
import uuid
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))


def require_disposable_target():
    if (os.environ.get("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA") != "1" or
        os.environ.get("DB_HOST") != "127.0.0.1" or os.environ.get("DB_SOCKET") or
        not re.fullmatch(r"economic_schema_test_[A-Za-z0-9_]+",
                         os.environ.get("DB_NAME", ""))):
        raise SystemExit("explicit disposable loopback schema required")
    for key in ("DB_USER", "DB_PASSWD"):
        if not os.environ.get(key):
            raise SystemExit("explicit disposable database credentials required")
    os.environ.setdefault("DB_PORT", "3306")


def sql_failure(stage, result):
    errors = " | ".join(re.findall(r"^ERROR \d+.*$", result.stderr, re.MULTILINE))
    secret = os.environ.get("DB_PASSWD", "")
    if secret:
        errors = errors.replace(secret, "[REDACTED]")
    return RuntimeError(stage + (": " + errors if errors else ""))


def run_mysql(args, environment, sql, timeout=120):
    result = subprocess.run(args, input=sql, text=True, capture_output=True,
                            env=environment, timeout=timeout)
    if result.returncode:
        raise sql_failure("disposable item-source SQL fixture failed", result)
    return result.stdout


def count(client, query):
    value = client.sql(query).strip()
    if not value.isdigit():
        raise RuntimeError("disposable item-source query returned a non-count")
    return int(value)


def install_baseline_script(temp_root):
    project = temp_root / "baseline-project"
    migrations = project / "migrations"
    migrations.mkdir(parents=True)
    copied = migrations / "baseline_item_ownership.sh"
    copied.write_bytes((ROOT / "migrations/baseline_item_ownership.sh").read_bytes()
                       .replace(b"\r\n", b"\n"))
    copied.chmod(0o700)
    keys = ("ENVIRONMENT", "DB_HOST", "DB_PORT", "DB_USER", "DB_PASSWD", "DB_NAME")
    values = {
        "ENVIRONMENT": "local",
        "DB_HOST": os.environ["DB_HOST"],
        "DB_PORT": os.environ["DB_PORT"],
        "DB_USER": os.environ["DB_USER"],
        "DB_PASSWD": os.environ["DB_PASSWD"],
        "DB_NAME": os.environ["DB_NAME"],
    }
    env_file = project / ".env"
    env_file.write_text("".join(f"{key}={shlex.quote(values[key])}\n" for key in keys))
    env_file.chmod(0o600)
    return copied


def compile_harness(directory):
    output = directory / "pa_item_source_coverage_harness"
    flags = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-O1", "-g", "-Isrc"]
    flags += shlex.split(subprocess.check_output(["mysql_config", "--cflags"],
                                                  text=True))
    flags += ["tests/async/pa_item_source_coverage_harness.cpp",
              "src/persistence/economic_sql_source_snapshot.c"]
    flags += shlex.split(subprocess.check_output(["mysql_config", "--libs"],
                                                  text=True))
    flags += ["-lcrypto", "-o", str(output)]
    subprocess.run(flags, cwd=ROOT, check=True, timeout=180)
    return output


def assert_baseline(client, expected):
    query = (
        "SELECT CONCAT_WS('|',item_uid,root_item_uid,COALESCE(parent_item_uid,0),"
        "owner_type,owner_id,owner_context_id,vnum,source_table,source_row_id) "
        "FROM item_ownership_baseline ORDER BY item_uid"
    )
    actual = client.sql(query).splitlines()
    if actual != expected:
        raise AssertionError("baseline UID/owner reconstruction did not match fixture")
    current_query = (
        "SELECT CONCAT_WS('|',item_uid,root_item_uid,COALESCE(parent_item_uid,0),"
        "owner_type,owner_id,owner_context_id,vnum) "
        "FROM item_current_owner ORDER BY item_uid"
    )
    current = client.sql(current_query).splitlines()
    expected_current = ["|".join(row.split("|")[:7]) for row in expected]
    if current != expected_current:
        raise AssertionError("current-owner roots/parents disagreed with baseline")


def ancestry_regression_fixture():
    sql = []
    refused_uids = []
    admitted = []
    # Each source gets a two-row cycle and a 34-row chain (33 parent links).
    sources = (
        ("player_pet_items", "pet_id", 101, 97000, 11, 9101, 11),
        ("shopkeeper_items", "shopkeeper_id", 201, 98000, 9, 42, 0),
        ("siege_items", "room_vnum", 700, 99000, 3, 700, 0),
    )
    for table, context_column, context_id, uid_base, owner_type, owner_id, owner_context in sources:
        item_rows = [
            (100, context_id, 800, uid_base),
            (101, context_id, 801, uid_base + 1),
        ]
        item_rows.extend(
            (item_id, context_id, 802 + item_id - 1000, uid_base + 100 + item_id - 1000)
            for item_id in range(1000, 1034)
        )
        values = ",".join(
            f"({item_id},{context_id},{vnum},NULL,{item_uid})"
            for item_id, context_id, vnum, item_uid in item_rows
        )
        sql.append(
            f"INSERT INTO {table}(id,{context_column},vnum,container_id,obj_uid) "
            f"VALUES {values};"
        )
        edges = [(100, 101), (101, 100)]
        edges.extend((item_id, item_id - 1) for item_id in range(1001, 1034))
        sql.extend(
            f"UPDATE {table} SET container_id={parent_id} WHERE id={item_id};"
            for item_id, parent_id in edges
        )
        refused_uids.extend((uid_base, uid_base + 1, uid_base + 133))
        # The 32-link boundary remains admissible, not just the shallow cases.
        for depth in range(33):
            uid = uid_base + 100 + depth
            parent = uid - 1 if depth else 0
            admitted.append(
                f"{uid}|{uid_base + 100}|{parent}|{owner_type}|{owner_id}|"
                f"{owner_context}|{802 + depth}|{table}|{1000 + depth}"
            )
    return "".join(sql), refused_uids, admitted


def main():
    require_disposable_target()
    from migrations.verify_economy_accounting_schema import Client

    admin = Client()
    if admin.args[-1] != os.environ["DB_NAME"]:
        raise SystemExit("disposable schema guard mismatch")
    admin.args = admin.args[:-1]
    name = "economic_schema_test_pa_" + uuid.uuid4().hex
    if not re.fullmatch(r"economic_schema_test_pa_[0-9a-f]{32}", name):
        raise AssertionError("generated disposable schema name failed validation")
    admin.sql(f"CREATE DATABASE `{name}` CHARACTER SET utf8mb4;")
    try:
        with mock.patch.dict(os.environ, {"DB_NAME": name}):
            client = Client()
        bootstrap = subprocess.run(
            client.args, input=(ROOT / "migrations/bootstrap_multithread_safe.sql").read_text(),
            text=True, capture_output=True, env=client.env, timeout=180)
        if bootstrap.returncode:
            raise RuntimeError("disposable source schema bootstrap failed")
        run_mysql(client.args, client.env,
                  (ROOT / "migrations/immutable/0028_pet_custody.sql").read_text())
        run_mysql(client.args, client.env,
                  (ROOT / "migrations/immutable/0038_item_equipment_slot.sql").read_text())
        run_mysql(client.args, client.env,
                  (ROOT / "migrations/immutable/0043_shopkeeper_item_condition.sql").read_text())

        fixture = """
INSERT INTO player_data(pid,name) VALUES(11,'pa_source_coverage_player');
INSERT INTO player_pets(id,owner_pid,pet_uid,mob_vnum,pet_order)
VALUES(101,11,9101,500,0),(102,11,9102,501,1),(103,11,NULL,502,2);
INSERT INTO player_items(pid,vnum,obj_uid)
VALUES(11,100,9100),(11,199,9600);
INSERT INTO player_pet_items(id,pet_id,vnum,container_id,obj_uid)
VALUES(1,101,200,NULL,9200),(2,101,201,1,9201),
      (3,102,301,1,9301),(4,103,302,NULL,9302),
      (5,101,210,NULL,9220),(6,101,211,5,9219);
INSERT INTO shopkeepers(id,shop_id,room_vnum) VALUES(201,41,700);
INSERT INTO shopkeeper_items(id,shopkeeper_id,vnum,obj_uid,item_condition)
VALUES(1,201,400,9400,37),(2,201,401,9600,82),(3,201,402,9602,NULL);
INSERT INTO siege_items(id,room_vnum,vnum,obj_uid)
VALUES(1,700,500,9500),(2,701,501,9602);
"""
        run_mysql(client.args, client.env, fixture)

        with tempfile.TemporaryDirectory(prefix="pa-item-source-coverage-") as temp:
            temporary = Path(temp)
            harness = compile_harness(temporary)
            harness_env = dict(client.env, ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA="1",
                               DB_HOST="127.0.0.1", DB_PORT=os.environ["DB_PORT"],
                               DB_NAME=name)
            subprocess.run([str(harness)], cwd=ROOT, env=harness_env,
                           check=True, timeout=120)

            os.environ["DB_NAME"] = name
            # The copied operational script reads only this owner-only disposable
            # environment file, never the checkout's .env.
            baseline = install_baseline_script(temporary)

            initial = [
                "9100|9100|0|1|11|0|100|player_items|1",
                "9200|9200|0|11|9101|11|200|player_pet_items|1",
                "9201|9200|9200|11|9101|11|201|player_pet_items|2",
                # Child UID 9219 is lower than its parent UID 9220; both
                # baseline and current-owner must still use the parent root.
                "9219|9220|9220|11|9101|11|211|player_pet_items|6",
                "9220|9220|0|11|9101|11|210|player_pet_items|5",
                "9400|9400|0|9|42|0|400|shopkeeper_items|1",
                "9500|9500|0|3|700|0|500|siege_items|1",
            ]
            first = subprocess.run(["bash", str(baseline)], cwd=baseline.parents[1],
                                   text=True, capture_output=True, env=client.env,
                                   timeout=120)
            if first.returncode:
                raise sql_failure("disposable item baseline fixture failed", first)
            assert_baseline(client, initial)
            second = subprocess.run(["bash", str(baseline)], cwd=baseline.parents[1],
                                    text=True, capture_output=True, env=client.env,
                                    timeout=120)
            if second.returncode:
                raise RuntimeError("re-runnable item baseline fixture failed")
            assert_baseline(client, initial)

            run_mysql(client.args, client.env,
                      "UPDATE item_ownership_quarantine SET repaired_at=CURRENT_TIMESTAMP "
                      "WHERE item_uid IN (9301,9302,9600,9602);")
            reopened = subprocess.run(["bash", str(baseline)], cwd=baseline.parents[1],
                                      text=True, capture_output=True, env=client.env,
                                      timeout=120)
            if reopened.returncode:
                raise RuntimeError("recurrent ambiguity refusal fixture failed to run")
            assert_baseline(client, initial)
            if count(client, "SELECT COUNT(*) FROM item_ownership_quarantine "
                              "WHERE repaired_at IS NULL AND item_uid IN (9301,9302,9600,9602)") < 4:
                raise AssertionError("recurrent source conflicts were not reopened")

            if count(client, "SELECT COUNT(*) FROM item_ownership_quarantine "
                              "WHERE repaired_at IS NULL AND item_uid IN (9301,9302)") != 2:
                raise AssertionError("ambiguous pet ownership/parent rows were not quarantined")
            if count(client, "SELECT COUNT(*) FROM item_current_owner "
                              "WHERE item_uid IN (9301,9302,9600)") != 0:
                raise AssertionError("ambiguous or duplicate UIDs were admitted")
            if count(client, "SELECT COUNT(*) FROM item_ownership_quarantine "
                              "WHERE repaired_at IS NULL AND item_uid=9600 AND conflict_code=1") != 2:
                raise AssertionError("duplicate retained UIDs were not quarantined at both sources")
            if count(client, "SELECT COUNT(*) FROM item_ownership_quarantine "
                              "WHERE repaired_at IS NULL AND item_uid=9602 AND conflict_code=1") != 2:
                raise AssertionError("duplicate new-source UIDs were not quarantined at both sources")
            if count(client, "SELECT COUNT(*) FROM item_current_owner "
                              "WHERE item_uid=9602") != 0:
                raise AssertionError("duplicate new-source UID was freshly admitted")

            run_mysql(client.args, client.env,
                      "INSERT INTO player_items(pid,vnum,obj_uid) VALUES(11,199,9601);"
                      "INSERT INTO shopkeeper_items(id,shopkeeper_id,vnum,obj_uid) "
                      "VALUES(4,201,402,9601);")
            duplicate = subprocess.run(["bash", str(baseline)], cwd=baseline.parents[1],
                                       text=True, capture_output=True, env=client.env,
                                       timeout=120)
            if duplicate.returncode:
                raise RuntimeError("duplicate-UID refusal fixture failed to run")
            assert_baseline(client, initial)
            if count(client, "SELECT COUNT(*) FROM item_ownership_quarantine "
                              "WHERE item_uid=9601 AND repaired_at IS NULL AND conflict_code=1") != 2:
                raise AssertionError("new duplicate UID was not refused at both sources")

            ancestry_sql, refused_uids, admitted = ancestry_regression_fixture()
            run_mysql(client.args, client.env, ancestry_sql)
            unresolved = subprocess.run(["bash", str(baseline)], cwd=baseline.parents[1],
                                        text=True, capture_output=True, env=client.env,
                                        timeout=120)
            if unresolved.returncode:
                raise RuntimeError("cyclic/deep ancestry baseline fixture failed to run")
            uid_list = ",".join(str(uid) for uid in refused_uids)
            if count(client, f"SELECT COUNT(*) FROM item_ownership_baseline "
                              f"WHERE item_uid IN ({uid_list})") != 0:
                raise AssertionError("cyclic/depth-exhausted item entered baseline")
            if count(client, f"SELECT COUNT(*) FROM item_current_owner "
                              f"WHERE item_uid IN ({uid_list})") != 0:
                raise AssertionError("cyclic/depth-exhausted item entered current-owner")
            if count(client, f"SELECT COUNT(*) FROM item_ownership_quarantine "
                              f"WHERE item_uid IN ({uid_list}) AND repaired_at IS NULL "
                              "AND conflict_code=2") != len(refused_uids):
                raise AssertionError("cycle/depth-exhausted ancestry was not quarantined")
            expected = sorted(initial + admitted, key=lambda row: int(row.split("|")[0]))
            assert_baseline(client, expected)

            run_mysql(client.args, client.env,
                      f"UPDATE item_ownership_quarantine SET repaired_at=CURRENT_TIMESTAMP "
                      f"WHERE item_uid IN ({uid_list}) AND repaired_at IS NULL "
                      "AND conflict_code=2;")
            reopened_ancestry = subprocess.run(
                ["bash", str(baseline)], cwd=baseline.parents[1], text=True,
                capture_output=True, env=client.env, timeout=120)
            if reopened_ancestry.returncode:
                raise RuntimeError("ancestry quarantine rerun fixture failed to run")
            if count(client, f"SELECT COUNT(*) FROM item_ownership_quarantine "
                              f"WHERE item_uid IN ({uid_list}) AND repaired_at IS NULL "
                              "AND conflict_code=2") != len(refused_uids):
                raise AssertionError("ancestry quarantine was not reopened on rerun")
            assert_baseline(client, expected)
            print(f"ancestry admission PASS: {len(refused_uids)} refused; "
                  f"{len(admitted)} valid chain items; repaired quarantine reopened")

            before = (count(client, "SELECT COUNT(*) FROM item_current_owner"),
                      count(client, "SELECT COUNT(*) FROM item_ownership_baseline"),
                      count(client, "SELECT COUNT(*) FROM item_ownership_quarantine"))
            run_mysql(client.args, client.env,
                      "DROP TABLE player_pet_item_affects;"
                      "DROP TABLE player_pet_item_extra_descr;"
                      "DROP TABLE player_pet_items;")
            subprocess.run([str(harness), "--missing-source"], cwd=ROOT, env=harness_env,
                           check=True, timeout=120)
            missing = subprocess.run(["bash", str(baseline)], cwd=baseline.parents[1],
                                     text=True, capture_output=True, env=client.env,
                                     timeout=120)
            if missing.returncode == 0:
                raise AssertionError("missing durable item source was accepted")
            after = (count(client, "SELECT COUNT(*) FROM item_current_owner"),
                     count(client, "SELECT COUNT(*) FROM item_ownership_baseline"),
                     count(client, "SELECT COUNT(*) FROM item_ownership_quarantine"))
            if after != before:
                raise AssertionError("missing-source refusal changed durable ownership state")

        print("durable SQL item-source coverage and baseline refusal fixtures PASS")
    finally:
        admin.sql(f"DROP DATABASE IF EXISTS `{name}`;")
        if admin.sql(
            "SELECT COUNT(*) FROM information_schema.schemata "
            f"WHERE schema_name='{name}';").strip() != "0":
            raise RuntimeError("disposable item-source schema cleanup was not verified")


if __name__ == "__main__":
    main()
