#!/usr/bin/env python3
"""Divine reward containers never duplicate or destroy ordinary contents."""
from pathlib import Path
from contract_text import contains, find, index

ROOT = Path(__file__).resolve().parents[2]
reward = (ROOT / "src/account_reward.c").read_text()
header = (ROOT / "src/account_reward.h").read_text()
fight = (ROOT / "src/fight.c").read_text()
migration = (ROOT / "migrations/account_bound_rewards.sql").read_text()
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text()

# Manual dismissal refuses non-empty containers before extraction.
dismiss_start = index(reward, "static void dismiss_player_grant")
dismiss_end = index(reward, "static void player_divineclaim", dismiss_start)
dismiss = reward[dismiss_start:dismiss_end]
assert contains(dismiss, "instance->contains")
assert contains(dismiss, "Empty it first")
assert index(dismiss, "instance->contains") < index(dismiss, "extract_obj(instance)")

# Summoned containers arrive open even when the stored source template was closed.
summon_start = index(reward, "static bool summon_one")
summon_end = index(reward, "static bool parse_positive", summon_start)
summon = reward[summon_start:summon_end]
assert contains(summon, "obj->type==ITEM_CONTAINER")
assert contains(summon, "REMOVE_BIT(obj->value[1],CONT_CLOSED)")
assert index(summon, "REMOVE_BIT(obj->value[1],CONT_CLOSED)") < index(
    summon, "item_creation_grant_submit_to_player(ch,obj,ch)"
)

# Account reward code owns one public pre-persistence corpse hook. ACK-staged death
# invokes it before the first empty corpse snapshot and ownership submission; player
# inventory remains visible until each transfer commits.
assert contains(header, "void account_bound_reward_prepare_player_corpse(P_char ch, P_obj corpse);")
assert contains(fight, '#include "account_reward.h"')
hook = "account_bound_reward_prepare_player_corpse(ch, corpse);"
assert hook in fight
make_corpse = fight[index(fight, "P_obj make_corpse"):]
assert index(make_corpse, hook) < index(make_corpse, "writeCorpse(corpse);")
assert "item_transfer_reason::corpse_create" in fight
assert index(make_corpse, "if (IS_NPC(ch))") < index(make_corpse, "corpse->contains = ch->carrying;")

# Forced disappearance promotes direct children to the same parent, traverses
# nested containers first, and extracts only after the reward container is empty.
assert contains(reward, "promote_reward_contents")
promote_start = index(reward, "static bool promote_reward_contents")
promote_end = index(reward, "static std::string human_duration", promote_start)
promote = reward[promote_start:promote_end]
assert contains(promote, "ITEM2_CRUMBLELOOT")
assert contains(promote, "VOBJ_COINS")
assert contains(promote, "room_coin_merge")
assert contains(reward, "dissolve_reward_containers")
assert contains(reward, "obj_from_obj")
assert contains(reward, "obj_to_obj")
assert contains(reward, "obj_to_char")
assert contains(reward, "obj_to_room")
assert contains(reward, "fades from existence, leaving its contents behind")
assert contains(reward, "ITEM2_NOLOOT")
corpse_start = index(reward, "static void dissolve_reward_containers")
corpse_end = index(reward, "void account_bound_reward_prepare_player_corpse", corpse_start)
corpse_hook = reward[corpse_start:corpse_end]
assert not contains(corpse_hook, "account_bound_reward_owner(ch,obj)")
assert contains(corpse_hook, "marker.account") and contains(corpse_hook, "strcasecmp")
prepare_start = corpse_end
prepare_end = index(reward, "void account_bound_reward_on_login", prepare_start)
assert contains(reward[prepare_start:prepare_end], "reward_account(ch)")
assert index(corpse_hook, "promote_reward_contents") < index(corpse_hook, "recovery_ready=1") < index(corpse_hook, "extract_obj")

# Death recovery is distinct from last-summoned history.
for text in (migration, bootstrap):
    assert "recovery_ready" in text
assert contains(reward, "recovery_ready<>0") or contains(reward, "recovery_ready != 0")
assert contains(reward, "recovery_ready=0")
assert contains(reward, "recovery_ready=1")
assert contains(reward, "last_summoned_at=NOW()")

# Saved forced removals reparent children before deleting marker-matching bags;
# live forced removals use the same promotion helper before extraction.
assert contains(reward, "UPDATE player_items child JOIN player_items reward")
assert contains(reward, "SET child.container_id=reward.container_id")
clear_start = index(reward, "static bool clear_saved_grant")
clear_end = index(reward, "static void revoke_live_grant", clear_start)
clear_saved = reward[clear_start:clear_end]
assert index(clear_saved, "UPDATE player_items child") < clear_saved.rindex("DELETE")
revoke_start = index(reward, "static void revoke_live_grant")
revoke_end = index(reward, "static void purge_expired_grants", revoke_start)
revoke = reward[revoke_start:revoke_end]
assert index(revoke, "promote_reward_contents") < index(revoke, "extract_obj")

print("account reward container lifecycle contract: ok")
