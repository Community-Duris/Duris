#!/usr/bin/env python3
"""Divine reward containers never duplicate or destroy ordinary contents."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
reward = (ROOT / "src/account_reward.c").read_text()
header = (ROOT / "src/account_reward.h").read_text()
fight = (ROOT / "src/fight.c").read_text()
migration = (ROOT / "migrations/account_bound_rewards.sql").read_text()
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()

# Manual dismissal refuses non-empty containers before extraction.
dismiss_start = reward.index("static void dismiss_player_grant")
dismiss_end = reward.index("static void player_divineclaim", dismiss_start)
dismiss = reward[dismiss_start:dismiss_end]
assert "instance->contains" in dismiss
assert "Empty it first" in dismiss
assert dismiss.index("instance->contains") < dismiss.index("extract_obj(instance)")

# Summoned containers arrive open even when the stored source template was closed.
summon_start = reward.index("static bool summon_one")
summon_end = reward.index("static bool parse_positive", summon_start)
summon = reward[summon_start:summon_end]
assert "obj->type==ITEM_CONTAINER" in summon
assert "REMOVE_BIT(obj->value[1],CONT_CLOSED)" in summon
assert summon.index("REMOVE_BIT(obj->value[1],CONT_CLOSED)") < summon.index("obj_to_char(obj,ch)")

# Account reward code owns one public pre-persistence corpse hook. fight.c invokes
# it after inventory becomes corpse contents and before writeCorpse persists it.
assert "void account_bound_reward_prepare_player_corpse(P_char ch, P_obj corpse);" in header
assert '#include "account_reward.h"' in fight
hook = "account_bound_reward_prepare_player_corpse(ch, corpse);"
assert hook in fight
assert fight.index("corpse->contains = ch->carrying;") < fight.index(hook) < fight.index("writeCorpse(corpse);")

# Forced disappearance promotes direct children to the same parent, traverses
# nested containers first, and extracts only after the reward container is empty.
assert "promote_reward_contents" in reward
promote_start = reward.index("static bool promote_reward_contents")
promote_end = reward.index("static std::string human_duration", promote_start)
promote = reward[promote_start:promote_end]
assert "ITEM2_CRUMBLELOOT" in promote
assert "VOBJ_COINS" in promote
assert "room_coin_merge" in promote
assert "dissolve_reward_containers" in reward
assert "obj_from_obj" in reward
assert "obj_to_obj" in reward
assert "obj_to_char" in reward
assert "obj_to_room" in reward
assert "fades from existence, leaving its contents behind" in reward
assert "ITEM2_NOLOOT" in reward
corpse_start = reward.index("static void dissolve_reward_containers")
corpse_end = reward.index("void account_bound_reward_prepare_player_corpse", corpse_start)
corpse_hook = reward[corpse_start:corpse_end]
assert "account_bound_reward_owner(ch,obj)" not in corpse_hook
assert "marker.account" in corpse_hook and "strcasecmp" in corpse_hook
prepare_start = corpse_end
prepare_end = reward.index("void account_bound_reward_on_login", prepare_start)
assert "reward_account(ch)" in reward[prepare_start:prepare_end]
assert corpse_hook.index("promote_reward_contents") < corpse_hook.index("recovery_ready=1") < corpse_hook.index("extract_obj")

# Death recovery is distinct from last-summoned history.
for text in (migration, bootstrap):
    assert "recovery_ready" in text
assert "recovery_ready<>0" in reward or "recovery_ready != 0" in reward
assert "recovery_ready=0" in reward
assert "recovery_ready=1" in reward
assert "last_summoned_at=NOW()" in reward

# Saved forced removals reparent children before deleting marker-matching bags;
# live forced removals use the same promotion helper before extraction.
assert "UPDATE player_items child JOIN player_items reward" in reward
assert "SET child.container_id=reward.container_id" in reward
clear_start = reward.index("static bool clear_saved_grant")
clear_end = reward.index("static void revoke_live_grant", clear_start)
clear_saved = reward[clear_start:clear_end]
assert clear_saved.index("UPDATE player_items child") < clear_saved.rindex("DELETE")
revoke_start = reward.index("static void revoke_live_grant")
revoke_end = reward.index("static void purge_expired_grants", revoke_start)
revoke = reward[revoke_start:revoke_end]
assert revoke.index("promote_reward_contents") < revoke.index("extract_obj")

print("account reward container lifecycle contract: ok")
