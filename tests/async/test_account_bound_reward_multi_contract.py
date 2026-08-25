#!/usr/bin/env python3
"""Multi-grant and per-character instance contracts."""
from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/account_reward.c").read_text()
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text().lower().replace("`", "")
migration = (ROOT / "migrations/account_bound_rewards.sql").read_text().lower().replace("`", "")

loader = source[source.index("static std::vector<RewardGrant> query_grants"):source.index("static bool clear_saved_grant")]
assert contains(loader, "std::vector<RewardGrant>")
assert contains(loader, "ORDER BY account_name,id")
assert not contains(loader, "LIMIT 1")

# One physical instance per grant and character; instances on other account
# characters are deliberately left in place.
instance = source[source.index("static P_obj existing_character_instance"):source.index("static bool summon_one")]
assert contains(instance, "reward_item_owner(obj)!=ch")
assert contains(instance, "if (!keep) keep=obj")
assert contains(instance, "extract_obj(obj)")
assert not contains(source, "previous_owner")
assert not contains(source, "clear_saved_rewards(account, 0)")
assert contains(source, "account_bound_reward_summons")
assert contains(source, "ON DUPLICATE KEY UPDATE last_summoned_at=NOW()")
assert contains(source, "grant_marker_matches")
assert contains(source, "reward_marker_matches(obj, grant.account.c_str(), grant.id)")
assert contains(source, "grant.template_version == 0 && reward_marker_matches")

# Stable IDs allow multiple exact rewards sharing a vnum and precise removal;
# the old account/vnum and account/all forms remain as compatibility paths.
assert contains(source, "divineclaim remove <claim-id>")
assert contains(source, "divineclaim remove <account> <reward vnum|all>")
assert contains(source, "divineclaim list [account]")
assert contains(source, "WHERE id=%llu")
assert contains(source, "template_version=0 ORDER BY id LIMIT 1")
assert "primary key (id)" in migration
assert "primary key (grant_id, pid)" in migration or "primary key(grant_id,pid)" in migration
assert "primary key (id)" in bootstrap
assert "primary key (grant_id, pid)" in bootstrap or "primary key (grant_id,pid)" in bootstrap

print("multi-claim account reward runtime contract passed")
