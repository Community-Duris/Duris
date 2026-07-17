#!/usr/bin/env python3
"""Multi-grant and per-character instance contracts."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/account_reward.c").read_text()
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text().lower().replace("`", "")
migration = (ROOT / "migrations/account_bound_rewards.sql").read_text().lower().replace("`", "")

loader = source[source.index("static std::vector<RewardGrant> query_grants"):source.index("static bool clear_saved_grant")]
assert "std::vector<RewardGrant>" in loader
assert "ORDER BY account_name,id" in loader
assert "LIMIT 1" not in loader

# One physical instance per grant and character; instances on other account
# characters are deliberately left in place.
instance = source[source.index("static P_obj existing_character_instance"):source.index("static bool summon_one")]
assert "reward_item_owner(obj)!=ch" in instance
assert "if (!keep) keep=obj" in instance
assert "extract_obj(obj)" in instance
assert "previous_owner" not in source
assert "clear_saved_rewards(account, 0)" not in source
assert "account_bound_reward_summons" in source
assert "ON DUPLICATE KEY UPDATE last_summoned_at=NOW()" in source
assert "grant_marker_matches" in source
assert "reward_marker_matches(obj, grant.account.c_str(), grant.id)" in source
assert "grant.template_version == 0 && reward_marker_matches" in source

# Stable IDs allow multiple exact rewards sharing a vnum and precise removal;
# the old account/vnum and account/all forms remain as compatibility paths.
assert "divineclaim remove <claim-id>" in source
assert "divineclaim remove <account> <reward vnum|all>" in source
assert "divineclaim list [account]" in source
assert "WHERE id=%llu" in source
assert "template_version=0 ORDER BY id LIMIT 1" in source
assert "primary key (id)" in migration
assert "primary key (grant_id, pid)" in migration or "primary key(grant_id,pid)" in migration
assert "primary key (id)" in bootstrap
assert "primary key (grant_id, pid)" in bootstrap or "primary key (grant_id,pid)" in bootstrap

print("multi-claim account reward runtime contract passed")
