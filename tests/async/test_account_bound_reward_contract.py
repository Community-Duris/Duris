#!/usr/bin/env python3
"""Source contract for account-bound divineclaim rewards."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
MIGRATION = ROOT / "migrations/account_bound_rewards.sql"
VERIFIER = ROOT / "migrations/verify_account_bound_rewards.sh"
MYSQL_SCHEMA_TEST = ROOT / "tests/async/run_account_bound_reward_schema_mysql.sh"
BOOTSTRAP = ROOT / "migrations/bootstrap_multithread_safe.sql"

reward = (SRC / "account_reward.c").read_text()
header = (SRC / "account_reward.h").read_text()
prototypes = (SRC / "prototypes.h").read_text()
interp_h = (SRC / "interp.h").read_text()
interp_c = (SRC / "interp.c").read_text()
actobj = (SRC / "actobj.c").read_text()
nanny = (SRC / "nanny.c").read_text()
defines = (SRC / "defines.h").read_text()
makefile = (SRC / "Makefile").read_text()

assert MIGRATION.exists(), "account reward migration is missing"
schema = MIGRATION.read_text()
assert "CREATE TABLE IF NOT EXISTS account_bound_rewards" in schema
assert "account_name VARCHAR(50) NOT NULL" in schema
assert "reward_vnum INT NOT NULL DEFAULT 36419" in schema
assert "CONSTRAINT account_bound_rewards_ibfk_1 FOREIGN KEY" in schema
assert "REFERENCES accounts(account_name) ON DELETE CASCADE" in schema
assert "ALTER TABLE account_bound_rewards MODIFY COLUMN reward_vnum" in schema
assert "CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci" in schema
assert "DROP INDEX idx_account_bound_rewards_vnum" in schema
assert VERIFIER.exists(), "account reward exact verifier is missing"
assert MYSQL_SCHEMA_TEST.exists(), "account reward MySQL convergence test is missing"
assert "columns_exact" in VERIFIER.read_text()
assert "schema_signature" in MYSQL_SCHEMA_TEST.read_text()
assert "account_bound_rewards" in BOOTSTRAP.read_text()

assert "#define DEFAULT_ACCOUNT_REWARD_VNUM 36419" in header
assert "void do_divineclaim(P_char ch, char *argument, int cmd);" in prototypes
assert "void account_bound_reward_on_login(P_char ch);" in prototypes
assert "bool account_bound_reward_owner(P_char ch, P_obj obj);" in prototypes
assert "CMD_DIVINECLAIM" in interp_h
assert '"divineclaim"' in interp_c
assert "do_divineclaim" in interp_c
assert "CMD_N(CMD_DIVINECLAIM" in interp_c
runner = (ROOT / "migrations/run_migration.sh").read_text()
assert "account_reward.o" in makefile
assert "account_bound_rewards.sql" in runner
assert "verify_account_bound_rewards.sh" in runner
assert "TOTAL=112" in runner
assert "CMD_N(CMD_DIVINECLAIM, STAT_DEAD + POS_PRONE, do_divineclaim, 0, TRUE);" in interp_c

assert "mysql_real_escape_string" in reward
assert "account_exists" in reward
assert "INSERT INTO account_bound_rewards" in reward
assert "ON DUPLICATE KEY UPDATE" in reward
assert "read_object" in reward
assert "object_list" in reward
assert "extract_obj" in reward
assert "clear_saved_rewards" in reward
assert "DELETE pi FROM player_items" in reward
assert "previous_owner" in reward
assert "obj_to_char" in reward
assert "ITEM2_ACCOUNT_BOUND" in reward
assert "ITEM2_SOULBIND" in reward
assert "soulbound" in reward
assert "The gods bestow" in reward

assert "account_bound_reward_owner" in actobj
assert "You may not take that account-bound reward" in actobj
assert "account_bound_reward_on_login(ch)" in nanny

print("account-bound reward source contract passed")
