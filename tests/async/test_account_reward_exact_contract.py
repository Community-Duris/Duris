#!/usr/bin/env python3
"""Source contracts for exact account-reward grants, expiry, cooldown, and pwipe policy."""
from pathlib import Path
from contract_text import contains, find, index

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
look_pos = index(nanny, "do_look(ch, 0, -4);")
hook_pos = index(nanny, "account_bound_reward_on_login(ch);")
assert look_pos < hook_pos
assert contains(source, 'send_to_char("\\r\\n", ch)') or contains(source, 'send_to_char("\\r\\n",ch)')

# Dedicated, boot-time, documented settings.
assert "account_reward_config.o" in makefile
assert contains(comm, "boot_account_reward_config();")
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
assert contains(header, "ACCOUNT_REWARD_TEMPLATE_VERSION")
assert contains(source, "account_reward_snapshot_serialize")
assert contains(source, "account_reward_snapshot_apply")
assert contains(snapshot_source, '"template_version"')
assert contains(snapshot_source, '"craftsmanship"')
assert contains(snapshot_source, '"extra_descriptions"')
assert contains(snapshot_source, '"linked_affects"')
assert contains(snapshot_source, 'read_int(root, "type", ITEM_LOWEST, ITEM_LAST')
assert contains(snapshot_source, 'APPLY_REWARD_INT("material", MAT_UNDEFINED, MAT_HIGHEST')
assert contains(snapshot_source, 'read_int(entry,"location",APPLY_NONE,APPLY_LAST')
assert contains(source, "grant_id")

# Trusted command UX: exact item shorthand, explicit lifetimes, global listing,
# stable-ID removal, and backward-compatible vnum form.
assert contains(source, "permanent|days <count>|wipes <count>")
assert contains(source, "divineclaim list [account]")
assert contains(source, "divineclaim remove <claim-id>")
assert contains(source, "The source item remains in your inventory")
assert contains(source, "Each character on that account may summon one copy")
assert contains(source, "canonical_account(first,&first_account) && (!*second || parse_positive(second,&legacy_vnum))")
assert contains(source, "human_duration(grant.expires_seconds, true)")

# Cooldown is persisted per claim/PID, not kept in process memory.
assert contains(source, "last_summoned_at")
assert contains(source, "account_bound_reward_summons")
assert contains(source, "GET_PID(ch)")
assert contains(source.lower(), "cooldown")

# Pwipe policy is applied only after existing postflight passes and before
# success is announced.
pwipe_hook = "account_bound_rewards_on_successful_pwipe()"
assert pwipe_hook in sql_source
postflight_pos = index(sql_source, "if (!postflight_ok)")
pwipe_pos = sql_source.index(pwipe_hook, postflight_pos)
completed_pos = index(sql_source, 'send_to_all("WIPE COMPLETED!")', pwipe_pos)
assert postflight_pos < pwipe_pos < completed_pos
assert not contains(sql_source[pwipe_pos:completed_pos], "return FALSE")
manifest = sql_source[index(sql_source, "bool sql_verify_pwipe_manifest"):index(sql_source, "bool sql_verify_persistence_schema")]
assert contains(manifest, '"account_bound_rewards"')
assert contains(manifest, '"account_bound_reward_summons"')
assert contains(manifest, '"account_bound_reward_pwipe_state"')
assert contains(manifest, '"remaining_pwipes"')
assert contains(manifest, '"last_processed_at"')
assert pwipe_hook.replace("()", "") in header
assert contains(source, "remaining_pwipes = remaining_pwipes - 1")
assert contains(source, "DELETE FROM account_bound_reward_summons")
lock_pos = index(source, "FOR UPDATE", source.index("bool account_bound_rewards_on_successful_pwipe"))
delete_pos = index(source, "DELETE FROM account_bound_reward_summons", lock_pos)
stamp_pos = index(source, "SET last_processed_at=NOW()", delete_pos)
assert lock_pos < delete_pos < stamp_pos
assert contains(source, "INTERVAL 28 DAY")
assert "account_bound_reward_pwipe_state" in verifier

print("account reward exact-item/config/cooldown/expiry/pwipe source contract: ok")
