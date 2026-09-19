#!/usr/bin/env python3
"""Exercise the generated seed on a fresh, network-isolated disposable SQL server."""
from __future__ import annotations

import argparse
import importlib.util
import os
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("epic_zone_seed", ROOT / "scripts/epic_zone_seed.py")
assert spec is not None and spec.loader is not None
seed = importlib.util.module_from_spec(spec)
spec.loader.exec_module(seed)


def run(image: str) -> None:
    name = f"duris-epic-seed-qa-{os.getpid()}"
    client = "mariadb" if image.startswith("mariadb:") else "mysql"
    subprocess.run(["docker", "run", "--detach", "--name", name, "--network", "none",
                    "--env", "MYSQL_ALLOW_EMPTY_PASSWORD=yes", image], check=True,
                   capture_output=True, text=True)
    try:
        def query(sql: str, *, database: str | None = "epic_seed_qa", ok: bool = True):
            command = ["docker", "exec", "-i", name, client, "--user=root",
                       "--protocol=tcp", "--host=127.0.0.1", "--batch",
                       "--skip-column-names", "--raw"]
            if database:
                command.append(database)
            result = subprocess.run(command, input=sql, text=True, capture_output=True)
            if ok and result.returncode:
                raise AssertionError(result.stderr)
            return result

        deadline = time.monotonic() + 120
        while query("SELECT 1;", database=None, ok=False).returncode:
            if time.monotonic() >= deadline:
                raise AssertionError("database did not become authenticated-ready")
            time.sleep(1)
        query("CREATE DATABASE epic_seed_qa CHARACTER SET utf8mb4;", database=None)
        baseline = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
        match = re.search(r"CREATE TABLE `zones` \(.*?\n\) ENGINE=.*?;", baseline, re.S)
        assert match, "authoritative zones schema missing"
        ddl = match.group()
        manifest = seed.derive()
        sql = seed.apply_sql(manifest, "epic_seed_qa")
        known = [z for z in manifest["zones"] if z["status"] == "known"]

        def snapshot():
            return query("SELECT * FROM zones ORDER BY id;").stdout

        def populated_defaults():
            query("DROP TABLE IF EXISTS zones;" + ddl)
            # Real source zone numbers and exact schema; no player/account data.
            values = ",".join(f"({z['number']},{seed.literal(z['name'])})"
                              for z in manifest["zones"])
            query("INSERT INTO zones(number,name) VALUES " + values + ";"
                  "INSERT INTO zones(number,name,epic_payout,suggested_group_size,last_touch) "
                  "VALUES (999999,'unrelated sentinel',71,3,'2026-01-01 00:00:00'),"
                  "(4200,'excluded assignment sentinel',17,4,'2026-01-01 00:00:00');")

        def audit():
            return [line.split("\t") for line in query(seed.audit_sql(manifest)).stdout.splitlines()]

        protected_columns = [name for name in re.findall(r"^  `([^`]+)`", ddl, re.M)
                             if name not in {"epic_payout", "suggested_group_size"}]
        protected_query = "SELECT " + ",".join(f"`{name}`" for name in protected_columns) + " FROM zones ORDER BY id;"
        populated_defaults()
        query("UPDATE zones SET alignment=7,reset_perc=23,last_touch='2026-01-02 00:00:00' WHERE number=14;")
        all_other_fields = query(protected_query).stdout
        protected_before = query("SELECT * FROM zones WHERE number IN (1389,590,999999) "
                                 "OR name LIKE '%Grumbar%' ORDER BY number;").stdout
        query(sql)
        assert query(protected_query).stdout == all_other_fields
        assert sum(r[-1] == "matches" for r in audit()) == 107
        excluded = [r for r in audit() if r[0] == "4200"]
        assert len(excluded) == 1 and excluded[0][-1] == "outside-active-source-no-seed"
        assert excluded[0][3] == "850" and excluded[0][6:8] == ["17", "4"]
        assert protected_before == query("SELECT * FROM zones WHERE number IN (1389,590,999999) "
                                         "OR name LIKE '%Grumbar%' ORDER BY number;").stdout
        for z in known:
            assert query(f"SELECT epic_payout,suggested_group_size FROM zones "
                         f"WHERE number={z['number']};").stdout.strip() == f"{z['epic_payout']}\t100"
        first = snapshot()
        query(sql)
        assert snapshot() == first
        print(f"{image}: populated defaults, 107 exact payouts, exceptions/sentinel and replay PASS")

        populated_defaults()
        query("UPDATE zones SET epic_payout=999,suggested_group_size=7 WHERE number=14;"
              "UPDATE zones SET suggested_group_size=100 WHERE number=24;"
              "UPDATE zones SET epic_payout=600 WHERE number=68;"
              "UPDATE zones SET epic_payout=42,suggested_group_size=9 WHERE number=1389;")
        before = query("SELECT * FROM zones WHERE number IN (14,24,68,1389) ORDER BY number;").stdout
        query(sql)
        assert before == query("SELECT * FROM zones WHERE number IN (14,24,68,1389) ORDER BY number;").stdout
        assert sum(r[-1] == "override-preserved" for r in audit()) == 3
        print(f"{image}: positive tuning, intentional zero, partial config and disabled override preserved PASS")

        query("DROP TABLE zones;" + ddl)
        query(sql)
        assert query("SELECT COUNT(*) FROM zones;").stdout.strip() == "107"
        assert all(r[-1] in {"matches", "disabled-no-seed", "unresolved-no-seed",
                            "outside-active-source-no-seed"} for r in audit())
        first = snapshot()
        query(sql)
        assert snapshot() == first
        print(f"{image}: pre-first-boot empty table seeded, missing exceptions untouched, replay PASS")

        populated_defaults()
        before = snapshot()
        result = query(seed.apply_sql(manifest, "wrong_database"), ok=False)
        assert result.returncode and "Duplicate entry" in result.stderr
        assert snapshot() == before
        query("INSERT INTO zones(number,name) VALUES (14,'duplicate sentinel');")
        before = snapshot()
        result = query(sql, ok=False)
        assert result.returncode and "Duplicate entry" in result.stderr
        assert snapshot() == before
        print(f"{image}: wrong target and duplicate-number refusal, no partial changes PASS")

        populated_defaults()
        query("ALTER TABLE zones ENGINE=MyISAM;")
        before = snapshot()
        assert query(sql, ok=False).returncode
        assert snapshot() == before
        print(f"{image}: nontransactional engine refusal PASS")

        populated_defaults()
        query("DELETE FROM zones WHERE number=14;"
              "CREATE TRIGGER reject_seed_insert BEFORE INSERT ON zones FOR EACH ROW "
              "SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='seed QA insertion fault';")
        before = snapshot()
        result = query(sql, ok=False)
        assert result.returncode and "seed QA insertion fault" in result.stderr
        assert snapshot() == before
        print(f"{image}: late insertion failure rolls back earlier payout updates PASS")
    finally:
        subprocess.run(["docker", "rm", "--force", "--volumes", name], check=True,
                       capture_output=True, text=True)
        exists = subprocess.run(["docker", "inspect", name], capture_output=True)
        assert exists.returncode != 0, "disposable container still exists"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", choices=["mysql:8.0", "mariadb:11.4"], default="mysql:8.0")
    run(parser.parse_args().image)
