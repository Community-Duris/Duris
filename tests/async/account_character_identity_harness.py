#!/usr/bin/env python3
"""Prove repeated account_characters projection consumes no identity value.

account_characters.id is a signed INT AUTO_INCREMENT. MySQL consumes an identity
value on every INSERT ... ON DUPLICATE KEY UPDATE attempt, including the ones
that only update, so projecting an existing character on each save advanced the
counter without adding a row. The production writers now resolve the mapping id
first and UPDATE in place; only a genuinely new mapping inserts.
"""

from _paths import SRC
from pathlib import Path
import os
import subprocess

from contract_text import index

ROOT = Path(__file__).resolve().parents[2]
PROBE = "account_character_identity_probe"


def mysql(script: str) -> list[str]:
    """Run a script against the configured database and return its output rows.

    The credentials come from the guarded wrapper's environment and the password
    is passed through MYSQL_PWD rather than the command line. Empty lines are
    dropped so a caller can index the scalar results positionally.
    """
    command = [
        "mysql",
        "-h", os.environ["DB_HOST"],
        "-P", os.environ.get("DB_PORT", "3306"),
        "-u", os.environ["DB_USER"],
        "-N", "-B", os.environ["DB_NAME"],
    ]
    result = subprocess.run(
        command, input=script, capture_output=True, text=True,
        env={**os.environ, "MYSQL_PWD": os.environ["DB_PASSWD"]}, check=True,
    )
    return [line for line in result.stdout.splitlines() if line]


def next_identity(pattern: str) -> int:
    """Return the id a fresh row receives after five repeated projections."""
    rows = mysql(f"""
DROP TABLE IF EXISTS {PROBE};
CREATE TABLE {PROBE} (
  id INT NOT NULL AUTO_INCREMENT,
  account_name VARCHAR(255) NOT NULL,
  pid BIGINT NOT NULL,
  char_name VARCHAR(255) NOT NULL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  deleted_at DATETIME DEFAULT NULL,
  PRIMARY KEY (id),
  UNIQUE KEY idx_char_name_unique (char_name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
INSERT INTO {PROBE} (account_name,pid,char_name) VALUES ('acct',1,'Hero');
{pattern}
INSERT INTO {PROBE} (account_name,pid,char_name) VALUES ('acct',2,'Second');
SELECT LAST_INSERT_ID();
SELECT COUNT(*) FROM {PROBE};
DROP TABLE {PROBE};
""")
    identity, rows_present = int(rows[0]), int(rows[1])
    assert rows_present == 2, f"probe should hold two rows, held {rows_present}"
    return identity


legacy = "\n".join(
    f"INSERT INTO {PROBE} (account_name,pid,char_name) VALUES ('acct',1,'Hero') "
    "ON DUPLICATE KEY UPDATE account_name=VALUES(account_name),pid=VALUES(pid),"
    "deleted_at=NULL;"
    for _ in range(5)
)
current = "\n".join(
    f"UPDATE {PROBE} SET account_name='acct',pid=1,deleted_at=NULL "
    f"WHERE id=(SELECT id FROM (SELECT id FROM {PROBE} WHERE char_name='Hero' LIMIT 1) m);"
    for _ in range(5)
)

legacy_identity = next_identity(legacy)
current_identity = next_identity(current)

assert current_identity == 2, (
    f"lookup-then-update consumed identity values: next id was {current_identity}"
)
assert legacy_identity >= current_identity, (
    "the legacy upsert should never consume fewer identity values than the "
    f"current writer: legacy={legacy_identity} current={current_identity}"
)

# The source contract keeps both production writers on the resolve-first shape,
# so a revert to an unconditional upsert fails here rather than in production.
sql_text = (SRC / "sql.c").read_text(encoding="utf-8", errors="replace")
player_text = (SRC / "sql_player.c").read_text(encoding="utf-8", errors="replace")
index(sql_text, "static long sql_find_account_character_id(const char *escaped_char_name)")
index(sql_text, "const long mapping_id = sql_find_account_character_id(char_name_sql);")
index(sql_text, "UPDATE account_characters ")
index(player_text, "static long sql_find_account_character_mapping(const char *escaped_char_name)")
index(player_text, "const long mapping_id = sql_find_account_character_mapping(esc_char);")
index(player_text, "update account_characters set login_count=")

print(
    "account character identity churn: repeated projection consumed no identity "
    f"value (legacy next id {legacy_identity}, current next id {current_identity})"
)
