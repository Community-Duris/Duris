#!/usr/bin/env python3
"""Player divineclaim UX: list, one-at-a-time summon, dismiss, and login choice."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/account_reward.c").read_text()
config_source = (ROOT / "src/account_reward_config.c").read_text()
config_header = (ROOT / "src/account_reward_config.h").read_text()
config_file = (ROOT / "lib/account_rewards.cfg").read_text()

# Player grammar is separate from the trusted management parser.
for text in (
    "divineclaim list",
    "divineclaim summon <number>",
    "divineclaim dismiss <number>",
):
    assert text in source
assert "player_divineclaim" in source
assert "if (!IS_TRUSTED(ch))" in source

# Player list is ordered, actionable, and reports one status per grant.
assert "Ready" in source
assert "Recovering:" in source
assert "Carried" in source
assert "Equipped" in source
assert "cooldown_countdown(remaining)" in source
assert "(seconds + 59) / 60" in source
assert "account_reward_config_show_claim_ids" in source
assert "player.show.claim.ids" in config_source
assert "player.show.claim.ids=true" in config_file
assert "account_reward_config_show_claim_ids" in config_header

# Manual selection resolves one list entry and applies the configured active
# reward capacity. Zero means unlimited; positive values cap distinct grants.
assert "resolve_player_grant" in source
assert "summon_one(ch,selected,true)" in source or "summon_one(ch, selected, true)" in source
assert "active_player_grant_count" in source
assert "account_reward_config_max_active_rewards" in source
assert "active_limit==0" in source or "active_limit == 0" in source
assert "active_count >= active_limit" in source or "active_count>=active_limit" in source
assert "player.max.active.rewards" in config_source
assert "player.max.active.rewards=0" in config_file
assert "account_reward_config_max_active_rewards" in config_header
assert "ACCOUNT_REWARD_ACTIVE_MAX" in config_source

# Login never confiscates other active rewards. It attempts only its selected
# grant and respects the same capacity when that selected grant is missing.
assert "return_other_login_rewards" not in source
assert "maximum of %d active divine reward" in source

# Dismiss removes this character's physical instance, saves, and preserves the
# summon ledger/cooldown rather than resetting or deleting it.
dismiss_start = source.index("static void dismiss_player_grant")
dismiss_end = source.index("static void player_divineclaim", dismiss_start)
dismiss_slice = source[dismiss_start:dismiss_end]
assert "extract_obj" in dismiss_slice
assert "do_save_silent" in dismiss_slice
assert "DELETE FROM account_bound_reward_summons" not in dismiss_slice
assert "UPDATE account_bound_reward_summons" not in dismiss_slice

# Login chooses only the newest successfully summoned active grant for this PID,
# falling back to the first active list entry when no history exists.
assert "login_reward" in source
assert "s.last_summoned_at DESC" in source
assert "LIMIT 1" in source
assert "grants.front()" in source
assert "summon_one(ch,selected,true)" in source or "summon_one(ch, selected, true)" in source

# White brackets and a per-letter heavenly white/cyan/blue/gold gradient.
assert "[&+C" in source or "[&+B" in source
assert "&+W]" in source
for color in ("&+W", "&+C", "&+B", "&+Y"):
    assert color in source
assert "Divinely Bound" not in source  # letters are individually colorized
assert "LEGACY_BINDING_TAG" in source
assert "description.erase" in source
assert "beautify_reward_item(existing)" in source

print("account reward player list/single-summon/dismiss/login UX contract: ok")
