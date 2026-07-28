#!/usr/bin/env python3
"""Source contracts for exact account-reward grants, expiry, cooldown, and pwipe policy."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/account_reward.c").read_text()
snapshot_source = (ROOT / "src/account_reward_snapshot.c").read_text()
header = (ROOT / "src/account_reward.h").read_text()
nanny = (ROOT / "src/nanny.c").read_text()
sql_source = (ROOT / "src/sql.c").read_text()
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()
migration = (ROOT / "migrations/account_bound_rewards.sql").read_text()
verifier = (ROOT / "migrations/verify_account_bound_rewards.sh").read_text()
makefile = (ROOT / "src/Makefile").read_text()
comm = (ROOT / "src/comm.c").read_text()

config_source_path = ROOT / "src/account_reward_config.c"
config_header_path = ROOT / "src/account_reward_config.h"
config_file_path = ROOT / "lib/account_rewards.cfg"
assert config_source_path.exists()
assert config_header_path.exists()
assert config_file_path.exists()
config_source = config_source_path.read_text()
config_header = config_header_path.read_text()
config_file = config_file_path.read_text()

# Player-facing reward output must happen after the initial room look.
look_pos = nanny.index("do_look(ch, 0, -4);")
hook_pos = nanny.index("account_bound_reward_on_login(ch);")
assert look_pos < hook_pos
assert 'send_to_char("\\r\\n", ch)' in source or 'send_to_char("\\r\\n",ch)' in source

# Dedicated, boot-time, documented settings.
assert "account_reward_config.o" in makefile
assert "boot_account_reward_config();" in comm
assert "summon.cooldown.seconds" in config_source
assert "pwipe.preserve" in config_source
assert "summon.cooldown.seconds=3600" in config_file
assert "pwipe.preserve=true" in config_file
assert "account_reward_config_cooldown_seconds" in config_header
assert "account_reward_config_preserve_on_pwipe" in config_header

# Stable claim IDs, exact snapshots, lifetime fields, and per-character ledger.
for text in (bootstrap, migration):
    normalized = text.lower().replace("`", "")
    assert "id bigint unsigned not null auto_increment" in normalized
    assert "template_version smallint unsigned not null default '0'" in normalized or "template_version smallint unsigned not null default 0" in normalized
    assert "template_json longtext" in normalized
    assert "display_name varchar(512)" in normalized
    assert "expires_at datetime" in normalized
    assert "remaining_pwipes int unsigned" in normalized
    assert "create table" in normalized and "account_bound_reward_summons" in normalized
    assert "primary key (grant_id, pid)" in normalized or "primary key (grant_id,pid)" in normalized
    assert "account_bound_reward_pwipe_state" in normalized
    assert "last_processed_at datetime" in normalized

assert "account_bound_reward_summons" in verifier
assert "template_json" in verifier
assert "remaining_pwipes" in verifier

# Versioned exact-object snapshot and claim-ID ownership markers.
assert "ACCOUNT_REWARD_TEMPLATE_VERSION" in header
assert "account_reward_snapshot_serialize" in source
assert "account_reward_snapshot_apply" in source
assert '"template_version"' in snapshot_source
assert '"craftsmanship"' in snapshot_source
assert '"extra_descriptions"' in snapshot_source
assert '"linked_affects"' in snapshot_source
assert 'read_int(root, "type", ITEM_LOWEST, ITEM_LAST' in snapshot_source
assert 'APPLY_REWARD_INT("material", MAT_UNDEFINED, MAT_HIGHEST' in snapshot_source
assert 'read_int(entry,"location",APPLY_NONE,APPLY_LAST' in snapshot_source
assert "grant_id" in source

# Trusted command UX: exact item shorthand, explicit lifetimes, global listing,
# stable-ID removal, and backward-compatible vnum form.
assert "permanent|days <count>|wipes <count>" in source
assert "divineclaim list [account]" in source
assert "divineclaim remove <claim-id>" in source
assert "The source item remains in your inventory" in source
assert "Each character on that account may summon one copy" in source
assert "canonical_account(first,&first_account) && (!*second || parse_positive(second,&legacy_vnum))" in source
assert "human_duration(grant.expires_seconds, true)" in source

# Cooldown is persisted per claim/PID, not kept in process memory.
assert "last_summoned_at" in source
assert "account_bound_reward_summons" in source
assert "GET_PID(ch)" in source
assert "cooldown" in source.lower()

# Pwipe policy is applied only after existing postflight passes and before
# success is announced.
pwipe_hook = "account_bound_rewards_on_successful_pwipe()"
assert pwipe_hook in sql_source
postflight_pos = sql_source.index("if (!postflight_ok)")
pwipe_pos = sql_source.index(pwipe_hook, postflight_pos)
completed_pos = sql_source.index('send_to_all("WIPE COMPLETED!")', pwipe_pos)
assert postflight_pos < pwipe_pos < completed_pos
assert "return FALSE" not in sql_source[pwipe_pos:completed_pos]
manifest = sql_source[sql_source.index("bool sql_verify_pwipe_manifest"):sql_source.index("bool sql_verify_persistence_schema")]
assert '"account_bound_rewards"' in manifest
assert '"account_bound_reward_summons"' in manifest
assert '"account_bound_reward_pwipe_state"' in manifest
assert '"remaining_pwipes"' in manifest
assert '"last_processed_at"' in manifest
assert pwipe_hook.replace("()", "") in header
assert "remaining_pwipes = remaining_pwipes - 1" in source
assert "DELETE FROM account_bound_reward_summons" in source
lock_pos = source.index("FOR UPDATE", source.index("bool account_bound_rewards_on_successful_pwipe"))
delete_pos = source.index("DELETE FROM account_bound_reward_summons", lock_pos)
stamp_pos = source.index("SET last_processed_at=NOW()", delete_pos)
assert lock_pos < delete_pos < stamp_pos
assert "INTERVAL 28 DAY" in source
assert "account_bound_reward_pwipe_state" in verifier

print("account reward exact-item/config/cooldown/expiry/pwipe source contract: ok")
