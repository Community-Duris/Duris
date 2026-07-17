#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/account_reward.c").read_text()
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
migration = (ROOT / "migrations/account_bound_rewards.sql").read_text()

loader = source[source.index("assigned_reward_vnums"):source.index("static bool account_exists")]
assert "std::vector<int>" in source
assert "ORDER BY reward_vnum" in loader
assert "LIMIT 1" not in loader
assert "summon_reward_vnum" in source
assert "clear_saved_rewards(account, 0)" in source
assert "clear_saved_rewards(second, vnum)" in source
assert "char marker[sizeof(ACCOUNT_REWARD_MARKER) + ACCOUNT_REWARD_ACCOUNT_MAX + 2]" in source
assert "ACCOUNT_REWARD_ACCOUNT_MAX + 2) * 2 + 1" in source
assert "DELETE FROM account_bound_rewards WHERE account_name = '%s' AND reward_vnum = %d" in source
assert "DELETE FROM account_bound_rewards WHERE account_name = '%s'" in source
assert "Syntax: divineclaim remove <account> <reward vnum|all>" in source
assert "Syntax: divineclaim list <account>" in source
assert "ON DUPLICATE KEY UPDATE granted_by = VALUES(granted_by)" in source
assert "PRIMARY KEY (`account_name`,`reward_vnum`)" in bootstrap
assert "PRIMARY KEY (account_name, reward_vnum)" in migration
assert "columns_signature = 'account_name,reward_vnum'" in migration

print("multi-claim account reward runtime contract passed")
