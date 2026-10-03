#!/usr/bin/env python3
"""Multi-grant and per-character instance contracts."""
from _paths import SRC
from pathlib import Path
from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "account_reward.c").read_text()
bootstrap = (ROOT / "migrations/bootstrap_multithread_safe.sql").read_text().lower().replace("`", "")
migration = (ROOT / "migrations/account_bound_rewards.sql").read_text().lower().replace("`", "")

loader = source[source.index("static std::vector<RewardGrant> query_grants"):source.index("static bool clear_saved_grant")]
assert contains(loader, "std::vector<RewardGrant>")
assert contains(loader, "ORDER BY account_name,id")
assert not contains(loader, "LIMIT 1")

# One physical instance per grant and character; instances on other account
# characters are deliberately left in place.
instance = source[source.index("static P_obj existing_character_instance"):source.index("static uint32_t reward_retirement_u32")]
assert contains(instance, "const bool character_owned = reward_item_owner(obj) == ch")
assert contains(instance, "reward_item_in_character_corpse(ch,obj)")
assert contains(instance, "retire_empty_duplicates&&character_owned")
assert contains(instance, "if (!keep) keep=obj")
assert contains(instance, "extract_obj(obj)")
assert contains(instance, "retire_empty_duplicates")
assert contains(instance, "submit_accounted_reward_promotion(ch,grant,obj)")
assert not contains(source, "previous_owner")
assert not contains(source, "clear_saved_rewards(account, 0)")
assert contains(source, "account_bound_reward_summons")
assert contains(source, "ON DUPLICATE KEY UPDATE last_summoned_at=NOW()", literal=True)
assert contains(source, "grant_marker_matches")
corpse_start = source.index(
    "static bool reward_item_in_character_corpse(P_char ch, P_obj obj)\n{"
)
corpse_owner = source[corpse_start:source.index("\n}", corpse_start) + 2]
assert contains(corpse_owner, "GET_ITEM_TYPE(top)==ITEM_CORPSE")
assert contains(corpse_owner, "IS_SET(top->value[CORPSE_FLAGS],PC_CORPSE)")
assert contains(corpse_owner, "top->value[CORPSE_PID]==GET_PID(ch)")
assert contains(corpse_owner, "top->value[CORPSE_SAVEID]>0")
assert contains(source, "reward_marker_matches(obj, grant.account.c_str(), grant.id)")
assert contains(source, "grant.template_version == 0 && reward_marker_matches")
summon = source[source.index("static bool summon_one"):source.index("static bool parse_positive")]
assert contains(summon, "const bool accounting_active=economic_gameplay_authority::active()")
assert contains(summon, "existing_character_instance(ch,grant,!accounting_active,accounting_active)")
assert contains(summon, "if(!accounting_active&&beautify_reward_item(existing)")
duplicate_cleanup = source[source.index("static P_obj existing_character_instance"):
                          source.index("static bool summon_one")]
assert contains(duplicate_cleanup, "retire_saved_reward_instance(ch,obj)")
promotion = source[source.index("static bool submit_accounted_reward_promotion(P_char ch, const RewardGrant &grant,"):]
assert contains(promotion, "item_transfer_reason::player_put")
assert contains(promotion, "item_transfer_reason::player_get")
assert contains(promotion, "account_reward_duplicate_promotion_completion")
assert contains(promotion, "submit_accounted_reward_retirement(actor,grant,duplicate,false)")
dismiss = source[source.index("static void dismiss_player_grant"):
                 source.index("static void player_divineclaim(P_char")]
assert contains(dismiss, "const bool accounting_active=economic_gameplay_authority::active()")
assert contains(dismiss, "existing_character_instance(ch,selected,!accounting_active)")
assert contains(dismiss, "submit_accounted_reward_retirement(ch,selected,instance,true)")
assert contains(source, "item_transfer_continuation_kind::account_reward_retirement")
assert contains(source, "item_movement_transaction_submit(ch,instance,NULL,from_owner,destruction")
assert contains(source, "account_reward_retirement_publication")
assert contains(source, "economic_source_kind::intentional_destruction")
retirement = source[source.index("static bool decode_reward_retirement"):
                    source.index("struct reward_promotion_terms")]
assert contains(retirement, "version == 2 && size != 28")
assert contains(retirement, "expected_uid ? expected_uid : result.root_item_uid")
assert contains(retirement, "(!expected_uid && !result.root_item_uid)")
assert contains(source, "continuation.data.resize(28)")
assert contains(source, "instance->obj_uid")
assert not contains(dismiss, "cannot be dismissed while item accounting is active", literal=True)
assert index(dismiss, "if(accounting_active)") < index(
    dismiss, "retire_saved_reward_instance(ch,instance)"
)
grant_submit = "item_creation_grant_submit_to_player(ch,obj,ch,NULL,economic_source_kind::boon)"
assert contains(summon, grant_submit)
assert contains(summon, "Your divine account reward is in your corpse")
assert index(summon, "account_bound_reward_summons") < index(
    summon, grant_submit
)
assert not contains(summon, "OBJ_CARRIED(obj)")

# Stable IDs allow multiple exact rewards sharing a vnum and precise removal;
# the old account/vnum and account/all forms remain as compatibility paths.
assert contains(source, "divineclaim remove <claim-id>", literal=True)
assert contains(source, "divineclaim remove <account> <reward vnum|all>", literal=True)
assert contains(source, "divineclaim list [account]")
assert contains(source, "WHERE id=%llu", literal=True)
assert contains(source, "template_version=0 ORDER BY id LIMIT 1", literal=True)
assert "primary key (id)" in migration
assert "primary key (grant_id, pid)" in migration or "primary key(grant_id,pid)" in migration
assert "primary key (id)" in bootstrap
assert "primary key (grant_id, pid)" in bootstrap or "primary key (grant_id,pid)" in bootstrap

print("multi-claim account reward runtime contract passed")
