#!/usr/bin/env python3
"""Core source contract for account-bound divineclaim rewards."""
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
makefile = (SRC / "Makefile").read_text()

assert MIGRATION.exists()
schema = MIGRATION.read_text()
normalized = schema.lower().replace("`", "")
assert "create table if not exists account_bound_rewards" in normalized
assert "id bigint unsigned not null auto_increment" in normalized
assert "account_name varchar(50)" in normalized
assert "reward_vnum int not null default 36419" in normalized
assert "template_json longtext" in normalized
assert "references accounts(account_name) on delete cascade" in normalized
assert "convert to character set utf8mb4 collate utf8mb4_unicode_ci" in normalized
assert "account_bound_reward_summons" in normalized
assert VERIFIER.exists()
assert MYSQL_SCHEMA_TEST.exists()
verifier_text = VERIFIER.read_text()
assert "parent_columns" in verifier_text
assert "ledger_columns" in verifier_text
assert "schema_signature" in MYSQL_SCHEMA_TEST.read_text()
assert "account_bound_rewards" in BOOTSTRAP.read_text()

assert "#define DEFAULT_ACCOUNT_REWARD_VNUM 36419" in header
assert "#define ACCOUNT_REWARD_TEMPLATE_VERSION 1" in header
assert "void do_divineclaim(P_char ch, char *argument, int cmd);" in prototypes
assert "void account_bound_reward_on_login(P_char ch);" in prototypes
assert "bool account_bound_reward_owner(P_char ch, P_obj obj);" in prototypes
assert "CMD_DIVINECLAIM" in interp_h
assert '"divineclaim"' in interp_c
assert "CMD_N(CMD_DIVINECLAIM" in interp_c
runner = (ROOT / "migrations/run_migration.sh").read_text()
assert "account_reward.o" in makefile
assert "account_reward_snapshot.o" in makefile
assert "account_reward_config.o" in makefile
assert "account_bound_rewards.sql" in runner
assert "verify_account_bound_rewards.sh" in runner
assert "CMD_N(CMD_DIVINECLAIM, STAT_DEAD + POS_PRONE, do_divineclaim, 0, TRUE);" in interp_c

assert "mysql_real_escape_string" in reward
assert "canonical_account" in reward
assert "insert_exact_grant" in reward
assert "assign_legacy_grant" in reward
assert "account_reward_snapshot_serialize" in reward
assert "account_reward_snapshot_apply" in reward
assert "read_object" in reward
assert "object_list" in reward
assert "extract_obj" in reward
assert "clear_saved_grant" in reward
assert "DELETE pi FROM player_items" in reward
assert "item_creation_grant_submit_to_player" in reward
assert "ITEM2_ACCOUNT_BOUND" in reward
assert "ITEM2_SOULBIND" in reward
assert "soulbound" in reward
assert "A divine account reward begins to materialize" in reward

assert "account_bound_reward_owner" in actobj
assert "You may not take that account-bound reward" in actobj
look = nanny.index("do_look(ch, 0, -4);")
hook = nanny.index("account_bound_reward_on_login(ch);")
assert look < hook

print("account-bound reward source contract passed")
