#!/usr/bin/env python3
"""Exercise the static quest's durable offering and publication callbacks."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source


quest = source("world/quest.c").read_text(encoding="utf-8")
assert "item_transfer_reason::quest_turnin" in quest
complete_quest = extract_function("world/quest.c", "static void complete_quest_offering(")
assert "transfer_result.operation_id" in complete_quest
assert (
    "quest_reward_recover_pending(actor, transfer_result.operation_id, continuation)"
    in complete_quest
)
assert "continuation.version = 6" in complete_quest
assert "context.skill_eligibility_mask" in complete_quest
assert "frozen_xp" in complete_quest
assert "quest_reward_character_present(award.recipient_pid)" in complete_quest
flatfile_transfer = extract_function(
    "flatfile/flatfile_item_repository.c", "bool generic_transfer_supported("
)
assert "accounted_player_quest_turnin" in flatfile_transfer
live_ready = extract_function(
    "item/item_movement_transaction.c", "bool destruction_publication_live_ready("
)
publish_movement = extract_function("item/item_movement_transaction.c", "void publish(")
restore_replayed = extract_function(
    "item/item_movement_transaction.c",
    "bool item_movement_transaction_restore_replayed_command(",
)
restore_publication = extract_function(
    "item/item_movement_transaction.c",
    "bool item_movement_transaction_restore_replayed_publication(",
)
gameplay_restore = extract_function(
    "net/comm.c", "static bool critical_gameplay_restore_replayed_command("
)
lifecycle_restore = extract_function(
    "magic/spell_item_lifecycle.c",
    "bool spell_item_lifecycle_restore_replayed_command(",
)
soulbind_submit = extract_function("magic/spell_item_lifecycle.c", "void do_soulbind(")
transfer_validation = extract_function("item/item_transfer_command.c", "bool validate_payload(")
assert "std::all_of(seen.begin(), seen.end(), [](uint8_t count) { return count == 0; })" in live_ready
assert publish_movement.index("pending.erase(current);") < publish_movement.index(
    "completion_fn(actor, committed, result, error_code"
)
assert "publication_required" in restore_replayed
assert "item_transfer_continuation_kind::quest_offering" in restore_replayed
assert "item_transfer_forced_weapon_drop(payload.reason)" in restore_replayed
assert "payload.to_owner.type == item_owner_type::room" in restore_replayed
assert "recovered_durable_item_publication" in restore_replayed
assert "unresolved_replay_publication" in restore_replayed
assert ".registry_applied = true" in restore_publication
assert "ITEM_MOVEMENT_PUBLICATION_MAX_ATTEMPTS - 1" in restore_publication
assert "soulbind_transfer" in lifecycle_restore
assert "payload.continuation.kind != item_transfer_continuation_kind::soulbind_transfer" in lifecycle_restore
assert "item_movement_transaction_restore_replayed_publication" in lifecycle_restore
assert "item_transfer_continuation_kind::soulbind_transfer" in soulbind_submit
assert "valid_soulbind_continuation(payload)" in transfer_validation
assert "spell_item_lifecycle_restore_replayed_command(command)" in gameplay_restore
assert "item_movement_transaction_restore_replayed_command(command)" in gameplay_restore
context_start = quest.index("constexpr size_t QUEST_DURABLE_MAX_OFFERINGS")
context_end = quest.index("static P_obj quest_object_by_uid(", context_start)
recovery_helpers_start = quest.index("struct quest_reward_recovery_attempt")
recovery_helpers_end = quest.index("void give_reward(", recovery_helpers_start)
recovery_helpers = quest[recovery_helpers_start:recovery_helpers_end]
reward_header = source("item/quest_reward_continuation.h").read_text()
reward_source_start = reward_header.index("constexpr uint32_t QUEST_REWARD_CURRENCY_OPERATION_DOMAIN")
reward_source_end = reward_header.index("// Version 1 retained exact offering terms.", reward_source_start)
recovery_helpers = reward_header[reward_source_start:reward_source_end] + recovery_helpers
functions = "\n".join(
    extract_function("world/quest.c", signature)
    for signature in (
        "static P_obj quest_object_by_uid(",
        "static struct quest_complete_data *quest_completion_by_index(",
        "static P_char quest_mobile_for(",
        "static bool publish_quest_offering(",
        "static void complete_quest_offering(",
        "void quest_reward_recover_pending(",
        "void quest_reward_recover_xp_entitlement(",
        "static bool submit_durable_quest_offering(",
    )
)
program = r'''
#include <cassert>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <array>
#include <new>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <openssl/sha.h>

struct character;
struct player_skill { int learned = 0; };
struct pc_only { player_skill skills[64] = {}; };
struct only_data { pc_only *pc = nullptr; };
struct player_state { int spec = 0; };
struct skill_info { int rlevel[5] = {}; };
skill_info skills[64];
long new_exp_table[100] = {};
struct object {
    uint64_t obj_uid = 0;
    int vnum = 0;
    character *carrier = nullptr;
    object *next = nullptr;
    object *next_content = nullptr;
};
struct character {
    bool npc = false;
    int pid = 0, rnum = 0, vnum = 0, in_room = 0;
    bool trusted = false;
    int level = 10, racewar = 1;
    const char *name = "tester";
    only_data only;
    player_state player;
    struct group_list *group = nullptr;
    character *next = nullptr;
    object *carrying = nullptr;
    object *equipment[4] = {};
    struct descriptor_data *desc = nullptr;
};
struct descriptor_data {};
struct group_list { character *ch; group_list *next; };
using P_char = character *;
using P_obj = object *;
using player_component_mask_t = uint64_t;
P_obj unequip_char(P_char, int);
void extract_char(P_char);
struct goal_data { char goal_type; int number; goal_data *next = nullptr; };
struct quest_complete_data {
    goal_data *give = nullptr, *receive = nullptr;
    bool echoAll = false;
    bool disappear = false;
    const char *disappear_message = "gone";
    const char *message = "done";
    quest_complete_data *next = nullptr;
};
struct quest_data { int quester = 0; quest_complete_data *quest_complete = nullptr; };
struct room { int number = 0; };
struct critical_operation_id { std::array<uint8_t, 16> bytes = {}; };
struct item_transfer_result { critical_operation_id operation_id = {}; };
bool critical_operation_id_is_zero(const critical_operation_id &id) {
    for (uint8_t byte : id.bytes) if (byte) return false;
    return true;
}
bool critical_operation_id_derive(const critical_operation_id &parent, uint32_t domain,
                                  uint32_t discriminator, critical_operation_id *child) {
    if (!child) return false;
    *child = parent;
    child->bytes[14] ^= static_cast<uint8_t>(domain);
    child->bytes[15] ^= static_cast<uint8_t>(discriminator);
    return true;
}
struct currency_command_result {};
enum class currency_reason_type { wallet_reward };
enum class critical_source_site { recovery };
enum class critical_deadline_class { recovery };
using currency_completion_fn = void (*)(P_char, bool, const currency_command_result &,
                                        unsigned int, const uint8_t *, size_t);
struct quest_reward_goal { uint32_t type = 0, number = 0, flags = 0, frozen_amount = 0; };
struct quest_reward_continuation {
    uint32_t version = 0;
    uint32_t player_pid = 0, quester_id = 0, completion_index = 0;
    uint32_t mobile_vnum = 0, room_vnum = 0;
    uint64_t completed_at = 0;
    uint32_t root_count = 0;
    std::array<uint64_t, 14> roots = {};
    uint32_t reward_count = 0;
    std::array<quest_reward_goal, 64> rewards = {};
    uint32_t zone_number = 0;
    int32_t player_level = 0, player_racewar = 0;
    uint32_t party_size = 0;
    int32_t strongest_party_level = 0;
    uint32_t credited_count = 0;
    std::array<uint32_t, 64> credited_pids = {};
    struct xp_award { uint32_t recipient_pid = 0, reward_index = 0, amount = 0; };
    uint32_t xp_award_count = 0;
    std::array<xp_award, 64> xp_awards = {};
    uint32_t season_id = 0, catalog_revision = 0, daily_policy_revision = 0;
    uint32_t daily_count = 0;
    std::array<uint32_t, 64> daily_pids = {};
    std::string character_name, definition_id;
};
void quest_reward_recover_pending(P_char, const critical_operation_id &,
                                  const quest_reward_continuation &, uint64_t = 0,
                                  uint64_t = 0, bool = true);
void quest_reward_recover_xp_entitlement(P_char, const critical_operation_id &,
                                         const quest_reward_continuation &, uint32_t,
                                         uint32_t);
enum class item_transfer_continuation_kind { none, quest_offering };
struct item_transfer_continuation {
    item_transfer_continuation_kind kind = item_transfer_continuation_kind::none;
    std::vector<uint8_t> data;
};
enum class item_owner_type { player, destruction };
struct item_owner_identity { item_owner_type type; uint64_t id, context_id; };
enum class item_transfer_reason { destruction, quest_turnin };
enum class item_movement_reject { none };
enum class economic_source_kind { intentional_destruction, quest_completion };
bool accounting_active = false;
namespace economic_gameplay_authority { bool active() { return accounting_active; } }
constexpr size_t ITEM_MOVEMENT_CONTEXT_MAX_BYTES = 768;
constexpr size_t ITEM_TRANSFER_CONTINUATION_MAX_BYTES = 8 * 1024;
constexpr size_t QUEST_REWARD_MAX_CREDITED_PIDS = 64;
constexpr size_t QUEST_REWARD_MAX_CHARACTER_NAME_BYTES = 64;
constexpr size_t QUEST_REWARD_MAX_DEFINITION_ID_BYTES = 4096;
constexpr uint32_t QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION = 1U;
constexpr int VIRTUAL = 1;
constexpr int QUEST_GOAL_ITEM = 1, QUEST_GOAL_COINS = 3;
constexpr int QUEST_GOAL_SKILL = 4, QUEST_GOAL_EXP = 5;
constexpr int PLAYER_COMPONENT_STATUS = 1, PLAYER_COMPONENT_EQUIPMENT = 2;
constexpr int PLAYER_COMPONENT_INVENTORY = 4, LOG_DEBUG = 1;
constexpr int PLAYER_COMPONENT_SKILLS = 8, RENT_CRASH = 1, NOWHERE = -1, MAX_SKILLS = 64;
constexpr int PLAYER_COMPONENT_TROPHIES = 16, EXP_QUEST = 9;
constexpr int MAX_SPEC = 4;
constexpr int TRUE = 1, FALSE = 0;
constexpr int MAX_WEAR = 4;
constexpr int MAX_NAME_LENGTH = 12;
#define IS_NPC(ch) ((ch)->npc)
#define GET_RNUM(ch) ((ch)->rnum)
#define GET_VNUM(ch) ((ch)->vnum)
#define GET_PID(ch) ((ch)->pid)
#define GET_LEVEL(ch) ((ch)->level)
#define GET_RACEWAR(ch) ((ch)->racewar)
#define GET_NAME(ch) ((ch)->name)
#define SKILL_DATA_ALL(ch, skill) (skills[(skill)])
#define IS_PC(ch) (!(ch)->npc)
#define IS_TRUSTED(ch) ((ch)->trusted)
#define OBJ_VNUM(obj) ((obj)->vnum)
#define OBJ_CARRIED_BY(obj, ch) ((obj)->carrier == (ch))
#define TO_ROOM 1
#define TO_VICT 2

quest_data quest_index[1];
int number_of_quests = 1;
struct mob_index_entry { int virtual_number = 0; };
mob_index_entry mob_index[12];
P_char character_list = nullptr;
P_obj object_list = nullptr;
room rooms[50] = {};
room *world = rooms;
int submissions = 0, rewards = 0, messages = 0, dirty = 0, removed = 0, acked = 0;
using player_revision_t = uint64_t;
using player_component_mask_t = uint64_t;
struct player_revision_snapshot { player_revision_t current_revision = 0, acknowledged_revision = 0; };
enum class player_save_pipeline_result { queued, coalesced, unavailable };
enum persistence_mode { PERSISTENCE_MODE_MARIADB_PRIMARY = 0,
                        PERSISTENCE_MODE_MARIADB_PRIMARY_FLATFILE_FALLBACK,
                        PERSISTENCE_MODE_FLATFILE_PRIMARY };
enum persistence_mode test_mode = PERSISTENCE_MODE_MARIADB_PRIMARY;
enum persistence_mode persistence_mode_get() { return test_mode; }
player_revision_snapshot save_revision;
bool revision_available = true;
int skill_save_requests = 0;
bool player_revision_snapshot_copy(int, player_revision_snapshot *out) {
    if (!out || !revision_available) return false;
    *out = save_revision;
    return true;
}
player_save_pipeline_result player_save_pipeline_request(P_char, int, int, int) {
    ++skill_save_requests;
    ++save_revision.current_revision;
    return player_save_pipeline_result::queued;
}
struct player_quest_xp_receipt_snapshot {
    critical_operation_id offering_operation;
    uint32_t reward_index = 0, amount = 0;
};
int xp_gained = 0, xp_save_requests = 0;
bool xp_save_refused = false;
std::vector<player_quest_xp_receipt_snapshot> queued_xp_receipts;
int gain_exp(P_char, P_char, int amount, int) { xp_gained += amount; return amount; }
player_save_pipeline_result player_save_pipeline_request_quest_xp(
    P_char, uint64_t, const player_quest_xp_receipt_snapshot *receipts, size_t count, int) {
    ++xp_save_requests;
    if (xp_save_refused) return player_save_pipeline_result::unavailable;
    queued_xp_receipts.assign(receipts, receipts + count);
    ++save_revision.current_revision;
    return player_save_pipeline_result::queued;
}
uint64_t last_reward_source = 0;
uint64_t last_completion_time = 0;
int last_completion_room = 0;
std::vector<uint32_t> last_credited_pids;
std::string last_tracking_name;
int last_tracking_level = 0, last_strongest_party_level = 0;
struct queued_grant {
    object *grant_object = nullptr;
    void (*completion)(P_char, uint64_t, bool, unsigned int) = nullptr;
    uint64_t source_id = 0;
};
std::vector<queued_grant> queued_grants;
struct queued_currency {
    currency_completion_fn completion = nullptr;
    critical_operation_id operation;
    std::vector<uint8_t> context;
};
std::vector<queued_currency> queued_currencies;
item_transfer_continuation saved_continuation;
bool item_command_uses_durable_ownership(P_obj object) { return object != nullptr; }
const char *item_movement_reject_name(item_movement_reject) { return "none"; }
void logit(int, const char *, ...) {}
void send_to_char(const char *, P_char) { ++messages; }
void act(const char *, int, P_char, int, P_char, int) { ++messages; }
void mark_player_dirty_components(int, int) { ++dirty; }
void finish_quest_reward(quest_complete_data *, P_char, P_char, uint64_t offering_uid,
                         uint64_t completed_at = 0, int32_t room_vnum = 0) {
    assert(offering_uid);
    last_reward_source = offering_uid;
    last_completion_time = completed_at;
    last_completion_room = room_vnum;
    ++rewards;
}
int get_artifact_data_sql(int, void *) { return 0; }
P_obj read_object(int vnum, int) {
    auto *object = new ::object{};
    object->obj_uid = 1000 + queued_grants.size();
    object->vnum = vnum;
    object->next = object_list;
    object_list = object;
    return object;
}
enum class quest_reward_obligation_result { ok };
enum class quest_reward_ack_submit_result { queued };
quest_reward_ack_submit_result quest_reward_obligation_pipeline_submit(
    uint32_t, const critical_operation_id &) { ++acked; return quest_reward_ack_submit_result::queued; }
bool item_creation_grant_submit_to_player_before_entry_with_completion(
    P_char, P_obj object, P_char,
    void (*completion)(P_char, uint64_t, bool, unsigned int),
    economic_source_kind, uint64_t source_id) {
    queued_grants.push_back({object, completion, source_id});
    return true;
}
bool currency_transaction_submit_wallet_value_identified(
    P_char, const critical_operation_id &operation, int64_t, currency_reason_type, int64_t,
    critical_source_site, critical_deadline_class, currency_completion_fn completion,
    const void *context, size_t context_size) {
    auto *bytes = static_cast<const uint8_t *>(context);
    queued_currency queued;
    queued.completion = completion;
    queued.operation = operation;
    queued.context.assign(bytes, bytes + context_size);
    queued_currencies.push_back(queued);
    return true;
}
namespace zone_story_quest_runtime {
struct frozen_daily_context {
    uint32_t season_id = 0, catalog_revision = 0, policy_revision = 0;
    std::vector<uint32_t> eligible_pids;
};
uint32_t current_season_id() { return 1; }
uint32_t content_revision() { return 2; }
bool daily_eligible(P_char, std::string_view, int, int64_t) { return false; }
bool record_legacy_completion(P_char, quest_complete_data *, int32_t, int64_t,
                              std::string *, std::string_view) { return true; }
bool record_authoritative_completion(std::string_view, int32_t, uint32_t,
                                     const std::vector<uint32_t> &credited_pids, int32_t,
                                     int64_t, std::string_view character_name, int level, int,
                                     bool, uint32_t, int strongest_party_level,
                                     std::string *, std::string_view,
                                     const frozen_daily_context * = nullptr) {
    last_credited_pids = credited_pids;
    last_tracking_name = character_name;
    last_tracking_level = level;
    last_strongest_party_level = strongest_party_level;
    return true;
}
}
namespace zone_story_quest_production {
constexpr size_t ZONE_STORY_QUEST_MAX_DURABLE_OFFERINGS = 14;
const std::string *definition_id_for(const quest_complete_data *) {
    static const std::string id = "zone-story:qst:77:abcd";
    return &id;
}
int zone_for_giver_vnum(int vnum) { return vnum > 0 ? 1 : 0; }
}
void extract_obj(P_obj object, int = 0) {
    P_obj *link = &object_list;
    while (*link && *link != object) link = &(*link)->next;
    assert(*link == object);
    *link = object->next;
    link = &object->carrier->carrying;
    while (*link && *link != object) link = &(*link)->next_content;
    assert(*link == object);
    *link = object->next_content;
    object->carrier = nullptr;
    ++removed;
}
unsigned char saved_context[ITEM_MOVEMENT_CONTEXT_MAX_BYTES] = {};
size_t saved_size = 0;
bool item_movement_transaction_submit_batch(
    P_char, P_obj const *, size_t, P_obj, const item_owner_identity &,
    const item_owner_identity &, item_transfer_reason, int64_t,
    auto, const void *context, size_t size, P_obj, item_movement_reject *, auto,
    economic_source_kind, uint64_t, const item_transfer_continuation &continuation) {
    assert(size <= sizeof(saved_context));
    memcpy(saved_context, context, size);
    saved_size = size;
    saved_continuation = continuation;
    ++submissions;
    return true;
}
''' + extract_function("item/quest_reward_continuation.h", "inline bool quest_reward_continuation_decode(") + "\n" + quest[context_start:context_end] + "\n" + recovery_helpers + "\n" + functions + r'''
int main() {
    new_exp_table[11] = 1000;
    new_exp_table[21] = 2000;
    character actor{false, 7, 0, 0, 42};
    pc_only actor_pc{};
    actor.only.pc = &actor_pc;
    character mob{true, 0, 11, 77, 42};
    actor.next = &mob;
    character_list = &actor;
    goal_data feather{QUEST_GOAL_ITEM, 103};
    goal_data branch{QUEST_GOAL_ITEM, 102, &feather};
    goal_data acorn{QUEST_GOAL_ITEM, 101, &branch};
    goal_data coin_reward{QUEST_GOAL_COINS, 250};
    goal_data item_reward{QUEST_GOAL_ITEM, 777, &coin_reward};
    quest_complete_data completion{&acorn, &item_reward};
    quest_index[0] = {11, &completion};
    mob_index[11].virtual_number = 77;
    rooms[42].number = 4200;
    object a{1, 101, &actor};
    actor.carrying = object_list = &a;

    // Presenting a partial set leaves custody untouched.
    assert(submit_durable_quest_offering(&mob, &actor, 0, &a));
    assert(submissions == 0 && removed == 0 && actor.carrying == &a);

    object b{2, 102, &actor};
    object c{3, 103, &actor};
    a.next = a.next_content = &b;
    b.next = b.next_content = &c;
    auto read32 = [](size_t offset, const std::vector<uint8_t> &bytes) {
        uint32_t value = 0;
        for (unsigned byte = 0; byte < 4; ++byte)
            value |= static_cast<uint32_t>(bytes[offset + byte]) << (byte * 8);
        return value;
    };

    // Equal complete offerings select the first native linked-list binding,
    // regardless of the presented ingredient. Neverwinter's loader prepends
    // its five source-file variants; this fixture uses that loaded order.
    {
        object runes[5]{};
        goal_data gives[5]{};
        goal_data receives[5]{};
        quest_complete_data variants[5]{};
        const int outputs[] = {99009, 99071, 99074, 99075, 99073};
        for (unsigned i = 0; i < 5; ++i) {
            runes[i] = {100 + i, 99002 + static_cast<int>(i), &actor};
            gives[i] = {QUEST_GOAL_ITEM, 99002 + static_cast<int>(i)};
            if (i + 1 < 5) {
                runes[i].next = runes[i].next_content = &runes[i + 1];
                gives[i].next = &gives[i + 1];
            }
            receives[i] = {QUEST_GOAL_ITEM, outputs[i]};
            variants[i] = {gives, &receives[i]};
            if (i + 1 < 5) variants[i].next = &variants[i + 1];
        }
        const int previous_messages = messages;
        actor.carrying = object_list = runes;
        quest_index[0].quest_complete = variants;
        for (bool active : {false, true}) {
            accounting_active = active;
            for (auto &rune : runes) {
                const auto previous_submissions = submissions;
                assert(submit_durable_quest_offering(&mob, &actor, 0, &rune));
                assert(submissions == previous_submissions + 1 && removed == 0);
                quest_reward_continuation admitted;
                assert(quest_reward_continuation_decode(saved_continuation.data.data(),
                    saved_continuation.data.size(), &admitted));
                assert(admitted.completion_index == 0 && admitted.root_count == 5);
                assert(admitted.reward_count == 1 &&
                    admitted.rewards[0].type == QUEST_GOAL_ITEM &&
                    admitted.rewards[0].number == 99009);
                assert(actor.carrying == runes);
            }
        }
        actor.carrying = object_list = &a;
        quest_index[0].quest_complete = &completion;
        accounting_active = false;
        submissions = 0;
        messages = previous_messages;
    }

    // A solo XP turn-in freezes the reward amount before consuming offerings.
    goal_data exp_only{QUEST_GOAL_EXP, 100};
    quest_complete_data exp_only_completion{&acorn, &exp_only};
    quest_index[0].quest_complete = &exp_only_completion;
    accounting_active = true;
    assert(submit_durable_quest_offering(&mob, &actor, 0, &a));
    assert(submissions == 1 && removed == 0 && actor.carrying == &a);
    assert(saved_continuation.data[0] == 6);
    assert(read32(64, saved_continuation.data) == QUEST_GOAL_EXP);
    assert(read32(76, saved_continuation.data) == 100);
    goal_data skill_only{QUEST_GOAL_SKILL, 12};
    quest_complete_data skill_only_completion{&acorn, &skill_only};
    quest_index[0].quest_complete = &skill_only_completion;
    assert(submit_durable_quest_offering(&mob, &actor, 0, &a));
    accounting_active = false;
    assert(submissions == 2 && removed == 0 && actor.carrying == &a);
    assert(saved_continuation.data.size() ==
           36 + 3 * sizeof(uint64_t) + sizeof(uint32_t) + 16 +
               6 * sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
               std::strlen("tester") + sizeof(uint32_t) +
               std::strlen("zone-story:qst:77:abcd") + sizeof(uint32_t) + 16);
    assert(saved_continuation.data[0] == 6 && saved_continuation.data[1] == 0);
    assert(read32(64, saved_continuation.data) == QUEST_GOAL_SKILL);
    assert(read32(68, saved_continuation.data) == 12);
    assert(read32(72, saved_continuation.data) ==
           QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION);

    quest_index[0].quest_complete = &completion;
    accounting_active = true;
    character teammate{false, 8, 0, 0, 42};
    teammate.level = 20;
    group_list member{&teammate, nullptr};
    actor.group = &member;
    quest_durable_context credit_context{};
    assert(capture_quest_credit_context(&actor, &credit_context));
    assert(credit_context.credited_count == 1 && teammate.desc == nullptr);
    // A group entry without an in-world character cannot participate.
    actor.next = &teammate;
    teammate.next = &mob; // an in-world linkdead member remains eligible
    assert(capture_quest_credit_context(&actor, &credit_context));
    assert(credit_context.credited_count == 2 && credit_context.credited_pids[1] == 8);
    actor.next = &mob;
    quest_index[0].quest_complete = &exp_only_completion;
    accounting_active = true;
    assert(submit_durable_quest_offering(&mob, &actor, 0, &a));
    assert(submissions == 3 && removed == 0 && actor.carrying == &a);
    assert(saved_continuation.data[0] == 6);
    quest_index[0].quest_complete = &completion;
    actor.next = &teammate;
    teammate.next = &mob;
    assert(submit_durable_quest_offering(&mob, &actor, 0, &a));
    actor.group = nullptr; // completion uses the admission-time recipient snapshot
    accounting_active = false;
    assert(submissions == 4 && saved_size == sizeof(quest_durable_context));
    assert(saved_continuation.kind == item_transfer_continuation_kind::quest_offering);
    assert(saved_continuation.data.size() ==
           36 + 3 * sizeof(uint64_t) + sizeof(uint32_t) + 2 * 16 +
               6 * sizeof(uint32_t) + 2 * sizeof(uint32_t) + sizeof(uint32_t) +
               std::strlen("tester") + sizeof(uint32_t) +
               std::strlen("zone-story:qst:77:abcd") + sizeof(uint32_t) + 16);
    assert(read32(0, saved_continuation.data) == 6);
    assert(read32(4, saved_continuation.data) == 7);
    assert(read32(16, saved_continuation.data) == 77);
    assert(read32(20, saved_continuation.data) == 4200);
    assert(read32(32, saved_continuation.data) == 3);
    assert(read32(60, saved_continuation.data) == 2);
    assert(read32(64, saved_continuation.data) == QUEST_GOAL_ITEM);
    assert(read32(68, saved_continuation.data) == 777);
    assert(read32(80, saved_continuation.data) == QUEST_GOAL_COINS);
    assert(read32(84, saved_continuation.data) == 250);
    assert(read32(96, saved_continuation.data) == 1);
    assert(read32(100, saved_continuation.data) == actor.level);
    assert(read32(108, saved_continuation.data) == 2);
    assert(read32(112, saved_continuation.data) == teammate.level);
    assert(read32(116, saved_continuation.data) == 2);
    assert(read32(120, saved_continuation.data) == actor.pid);
    assert(read32(124, saved_continuation.data) == teammate.pid);
    assert(read32(24, saved_continuation.data) != 0 ||
           read32(28, saved_continuation.data) != 0);
    assert(removed == 0 && rewards == 0);

    // An interrupted publication retains every item and can retry.
    b.carrier = nullptr;
    assert(!publish_quest_offering({}, &actor, true, {}, 0, saved_context, saved_size));
    assert(removed == 0 && rewards == 0);
    b.carrier = &actor;
    assert(publish_quest_offering({}, &actor, true, {}, 0, saved_context, saved_size));
    assert(removed == 3 && actor.carrying == nullptr && dirty == 1);
    complete_quest_offering(&actor, true, {}, 0, saved_context, saved_size);
    assert(rewards == 1 && last_reward_source == a.obj_uid);
    assert(last_completion_time != 0 && last_completion_room == 4200);

    // Reconnected inventory omits a previously committed destruction.
    object d{4, 101, &actor};
    object e{5, 102, &actor};
    object f{6, 103, &actor};
    d.next = d.next_content = &e;
    e.next = e.next_content = &f;
    actor.carrying = object_list = &d;
    actor.group = &member;
    assert(submit_durable_quest_offering(&mob, &actor, 0, &d));
    actor.group = nullptr;
    actor.carrying = object_list = nullptr;
    assert(publish_quest_offering({}, &actor, true, {}, 0, saved_context, saved_size));
    complete_quest_offering(&actor, true, {}, 0, saved_context, saved_size);
    assert(rewards == 2 && removed == 3 && last_reward_source == d.obj_uid);

    // A live durable completion uses the retained operation ID and the same
    // idempotent reward pipeline as reconnect recovery.
    item_transfer_result live_result{};
    live_result.operation_id.bytes[0] = 9;
    complete_quest_offering(&actor, true, live_result, 0, saved_context, saved_size);
    assert(queued_grants.size() == 1 && queued_currencies.size() == 1 && acked == 0);
    assert((last_credited_pids == std::vector<uint32_t>{7, 8}));
    assert(last_tracking_name == "tester" && last_tracking_level == actor.level &&
           last_strongest_party_level == teammate.level);
    queued_grants[0].grant_object->carrier = &actor;
    queued_grants[0].completion(&actor, queued_grants[0].grant_object->obj_uid, true, 0);
    assert(acked == 0);
    queued_currencies[0].completion(
        &actor, true, {}, 0, queued_currencies[0].context.data(),
        queued_currencies[0].context.size());
    assert(acked == 1);

    // If the NPC disappears after item publication, the retained completion
    // still dispatches the durable rewards.
    actor.next = nullptr;
    item_transfer_result missing_npc_result{};
    missing_npc_result.operation_id.bytes[0] = 10;
    complete_quest_offering(&actor, true, missing_npc_result, 0, saved_context,
                            saved_size);
    assert(queued_grants.size() == 2 && queued_currencies.size() == 2 && acked == 1);
    queued_grants.back().grant_object->carrier = &actor;
    queued_grants.back().completion(&actor, queued_grants.back().grant_object->obj_uid,
                                    true, 0);
    queued_currencies.back().completion(
        &actor, true, {}, 0, queued_currencies.back().context.data(),
        queued_currencies.back().context.size());
    assert(acked == 2);

    // The player-ready recovery path acknowledges only after supported grant
    // callbacks and the progression save revision are durable. Mixed rewards
    // with an unsupported XP effect stay pending.
    critical_operation_id operation{};
    operation.bytes[0] = 7;
    quest_reward_continuation pending{};
    pending.version = 2;
    pending.player_pid = 7;
    pending.quester_id = 0;
    pending.completion_index = 0;
    pending.mobile_vnum = 77;
    pending.room_vnum = 4200;
    pending.completed_at = 1234;
    pending.root_count = 1;
    pending.roots[0] = 90;
    pending.reward_count = 1;
    pending.rewards[0] = {QUEST_GOAL_ITEM, 777};
    pending.zone_number = 1;
    pending.player_level = actor.level;
    pending.player_racewar = actor.racewar;
    pending.party_size = 1;
    pending.strongest_party_level = actor.level;
    pending.credited_count = 1;
    pending.credited_pids[0] = actor.pid;
    pending.character_name = actor.name;
    pending.definition_id = "zone-story:qst:77:abcd";
    goal_data recovery_item{QUEST_GOAL_ITEM, 777};
    quest_complete_data recovery_completion{nullptr, &recovery_item};
    quest_index[0].quest_complete = &recovery_completion;
    const size_t grants_before_recovery = queued_grants.size();
    quest_reward_recover_pending(&actor, operation, pending);
    assert(queued_grants.size() == grants_before_recovery + 1 && acked == 2);
    queued_grants.back().grant_object->carrier = &actor;
    queued_grants.back().completion(&actor, queued_grants.back().grant_object->obj_uid,
                                    true, 0);
    assert(acked == 3);

    // Eligible skill grants are idempotent state assignments, but recovery
    // acknowledges only after the player-skills snapshot is durable.
    operation.bytes[0] = 9;
    pending.version = 3;
    pending.reward_count = 1;
    pending.rewards[0] = {QUEST_GOAL_SKILL, 12,
                          QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION};
    goal_data recovery_skill{QUEST_GOAL_SKILL, 12};
    quest_complete_data skill_completion{nullptr, &recovery_skill};
    quest_index[0].quest_complete = &skill_completion;
    quest_reward_recover_pending(&actor, operation, pending);
    assert(actor_pc.skills[12].learned == 1 && skill_save_requests == 1 && acked == 3);
    save_revision.acknowledged_revision = save_revision.current_revision;
    quest_reward_recovery_pulse();
    assert(acked == 4);
    quest_reward_recover_pending(&actor, operation, pending);
    assert(actor_pc.skills[12].learned == 1 && skill_save_requests == 1 && acked == 5);

    operation.bytes[0] = 8;
    pending.version = 2;
    goal_data mixed_exp{QUEST_GOAL_EXP, 100};
    goal_data mixed_coin{QUEST_GOAL_COINS, 250, &mixed_exp};
    goal_data mixed_item{QUEST_GOAL_ITEM, 777, &mixed_coin};
    quest_complete_data mixed_completion{nullptr, &mixed_item};
    quest_index[0].quest_complete = &mixed_completion;
    pending.reward_count = 3;
    pending.rewards[0] = {QUEST_GOAL_ITEM, 777};
    pending.rewards[1] = {QUEST_GOAL_COINS, 250};
    pending.rewards[2] = {QUEST_GOAL_EXP, 100};
    const size_t grants_before_mixed = queued_grants.size();
    quest_reward_recover_pending(&actor, operation, pending);
    assert(queued_grants.size() == grants_before_mixed + 1 &&
           queued_currencies.size() == 3 && acked == 5);
    queued_grants.back().grant_object->carrier = &actor;
    queued_grants.back().completion(&actor, queued_grants.back().grant_object->obj_uid,
                                   true, 0);
    assert(acked == 5);
    queued_currencies[2].completion(
        &actor, true, {}, 0, queued_currencies[2].context.data(),
        queued_currencies[2].context.size());
    assert(acked == 5);
    const auto first_cash_operation = queued_currencies[2].operation;
    quest_reward_recover_pending(&actor, operation, pending);
    assert(queued_currencies.size() == 4 &&
           queued_currencies[2].operation.bytes == first_cash_operation.bytes &&
           queued_currencies[3].operation.bytes == first_cash_operation.bytes);

    // The live completion callback reconstructs the immediate recovery terms
    // from the admission context, including frozen skill eligibility.
    actor.group = nullptr;
    actor.next = &mob;
    object *live_feather = new object{32, 103, &actor};
    object *live_branch = new object{31, 102, &actor};
    object *live_acorn = new object{30, 101, &actor};
    live_acorn->next_content = live_acorn->next = live_branch;
    live_branch->next_content = live_branch->next = live_feather;
    actor.carrying = object_list = live_acorn;
    goal_data live_skill{QUEST_GOAL_SKILL, 13};
    quest_complete_data live_skill_completion{&acorn, &live_skill};
    quest_index[0].quest_complete = &live_skill_completion;
    assert(submit_durable_quest_offering(&mob, &actor, 0, actor.carrying));
    assert(saved_continuation.data[0] == 6);
    const auto *admitted_context =
        reinterpret_cast<const quest_durable_context *>(saved_context);
    assert(admitted_context->skill_eligibility_mask & UINT64_C(1));
    skills[13].rlevel[0] = actor.level + 1; // catalog threshold changed after admission
    item_transfer_result live_skill_result{};
    live_skill_result.operation_id.bytes[0] = 11;
    const auto acknowledgements_before_live_skill = acked;
    const auto skill_saves_before_live_skill = skill_save_requests;
    complete_quest_offering(&actor, true, live_skill_result, 0, saved_context, saved_size);
    assert(actor_pc.skills[13].learned == 1 &&
           skill_save_requests == skill_saves_before_live_skill + 1 &&
           acked == acknowledgements_before_live_skill);
    save_revision.acknowledged_revision = save_revision.current_revision;
    quest_reward_recovery_pulse();
    assert(acked == acknowledgements_before_live_skill + 1);

    // XP is replayed from a frozen value, then saved with its operation and
    // reward index before the durable offering obligation is acknowledged.
    operation.bytes[0] = 12;
    pending.version = 4;
    pending.reward_count = 1;
    pending.rewards[0] = {QUEST_GOAL_EXP, 100, 0, 75};
    pending.credited_count = pending.party_size = 1;
    pending.credited_pids[0] = actor.pid;
    const auto xp_before = xp_gained;
    const auto xp_saves_before = xp_save_requests;
    const auto xp_acks_before = acked;
    quest_reward_recover_pending(&actor, operation, pending, 0);
    assert(xp_gained == xp_before + 75 && xp_save_requests == xp_saves_before + 1 &&
           queued_xp_receipts.size() == 1 &&
           queued_xp_receipts[0].offering_operation.bytes == operation.bytes &&
           queued_xp_receipts[0].reward_index == 0 &&
           queued_xp_receipts[0].amount == 75 && acked == xp_acks_before);
    save_revision.acknowledged_revision = save_revision.current_revision;
    quest_reward_recovery_pulse();
    assert(acked == xp_acks_before);
    auto wrong_xp_receipt = queued_xp_receipts[0];
    wrong_xp_receipt.amount = 74;
    quest_reward_recovery_save_acknowledged(actor.pid, save_revision.current_revision,
                                            &wrong_xp_receipt, 1);
    assert(acked == xp_acks_before);
    quest_reward_recovery_save_acknowledged(actor.pid, save_revision.current_revision,
                                            queued_xp_receipts.data(),
                                            queued_xp_receipts.size());
    assert(acked == xp_acks_before + 1);
    quest_reward_recover_pending(&actor, operation, pending, UINT64_C(1));
    assert(xp_gained == xp_before + 75 && xp_save_requests == xp_saves_before + 1 &&
           acked == xp_acks_before + 2);

    // A linkdead PC remains in character_list and receives its frozen group
    // entitlement immediately with no active descriptor.
    character teammate_linkdead{false, 8, 0, 0, 42};
    assert(teammate_linkdead.desc == nullptr);
    actor.next = &teammate_linkdead;
    teammate_linkdead.next = &mob;
    assert(quest_reward_character_present(8) == &teammate_linkdead);
    pending.version = 5;
    pending.reward_count = 1;
    pending.rewards[0] = {QUEST_GOAL_EXP, 100};
    pending.credited_count = 2;
    pending.credited_pids[0] = actor.pid;
    pending.credited_pids[1] = teammate_linkdead.pid;
    pending.xp_award_count = 2;
    pending.xp_awards[0] = {static_cast<uint32_t>(actor.pid), 0, 75};
    pending.xp_awards[1] = {static_cast<uint32_t>(teammate_linkdead.pid), 0, 50};
    const auto teammate_xp_before = xp_gained;
    const auto teammate_saves_before = xp_save_requests;
    quest_reward_recover_xp_entitlement(&teammate_linkdead, operation, pending, 0, 50);
    assert(xp_gained == teammate_xp_before + 50 &&
           xp_save_requests == teammate_saves_before + 1 &&
           queued_xp_receipts.back().reward_index == 0 &&
           queued_xp_receipts.back().amount == 50);

    // A committed group turn-in publishes the linkdead player's frozen XP
    // award in the same callback without waiting for a connection.
    quest_index[0].quest_complete = &exp_only_completion;
    quest_durable_context group_context{};
    group_context.quester_id = 0;
    group_context.completion_index = 0;
    group_context.room = 42;
    group_context.count = 1;
    group_context.roots[0] = 777;
    group_context.completed_at = 1235;
    group_context.credited_count = 2;
    group_context.credited_pids[0] = actor.pid;
    group_context.credited_pids[1] = teammate_linkdead.pid;
    group_context.credited_levels[0] = actor.level;
    group_context.credited_levels[1] = teammate_linkdead.level;
    group_context.party_size = 2;
    group_context.player_level = actor.level;
    group_context.strongest_party_level = teammate_linkdead.level;
    memcpy(group_context.character_name, "tester", sizeof("tester"));
    character_list = &actor;
    actor.next = &teammate_linkdead;
    teammate_linkdead.next = &mob;
    assert(quest_reward_character_present(teammate_linkdead.pid) == &teammate_linkdead);
    item_transfer_result group_xp_result{};
    // Group XP admission and publication use the same frozen contract on both
    // maintained backends, including a participant with no descriptor.
    for (auto mode : {PERSISTENCE_MODE_MARIADB_PRIMARY, PERSISTENCE_MODE_FLATFILE_PRIMARY}) {
        test_mode = mode;
        accounting_active = true;
        member.ch = &teammate_linkdead;
        actor.group = &member;
        const auto group_submissions_before = submissions;
        assert(submit_durable_quest_offering(&mob, &actor, 0, actor.carrying));
        assert(submissions == group_submissions_before + 1);
        quest_reward_continuation admitted;
        assert(quest_reward_continuation_decode(saved_continuation.data.data(), saved_continuation.data.size(), &admitted));
        assert(admitted.credited_count == 2 && admitted.xp_award_count == 2 &&
               admitted.rewards[0].frozen_amount == 100 && admitted.xp_awards[0].amount == 100);
        actor.group = nullptr;
        group_xp_result.operation_id.bytes[0] = 14 + static_cast<unsigned>(mode);
        const auto group_xp_before = xp_gained;
        const auto group_xp_saves_before = xp_save_requests;
        complete_quest_offering(&actor, true, group_xp_result, 0,
                                reinterpret_cast<const uint8_t *>(&group_context),
                                sizeof(group_context));
        assert(xp_gained == group_xp_before + 200);
        assert(xp_save_requests == group_xp_saves_before + 2);
        assert(queued_xp_receipts.back().reward_index == 0 &&
               queued_xp_receipts.back().amount == 100);
    }
    test_mode = PERSISTENCE_MODE_MARIADB_PRIMARY;
    accounting_active = false;
    // Lost save admission cannot discard applied XP or reopen its live replay
    // fence. A later checkpoint collects its exact receipt and components.
    quest_reward_recoveries.clear();
    pending.version = 4;
    pending.credited_count = pending.party_size = 1;
    pending.credited_pids[0] = actor.pid;
    pending.reward_count = 2;
    actor.only.pc->skills[47].learned = 0;
    pending.rewards[0] = {QUEST_GOAL_SKILL, 47, QUEST_REWARD_FLAG_SKILL_ELIGIBLE_AT_ADMISSION};
    pending.rewards[1] = {QUEST_GOAL_EXP, 100, 0, 75};
    operation.bytes[0] = 30;
    xp_save_refused = true;
    const auto refused_xp_before = xp_gained;
    const auto refused_acks_before = acked;
    quest_reward_recover_pending(&actor, operation, pending, 0);
    assert(xp_gained == refused_xp_before + 75);
    quest_reward_recover_pending(&actor, operation, pending, 0);
    assert(xp_gained == refused_xp_before + 75);
    std::vector<player_quest_xp_receipt_snapshot> fallback;
    player_component_mask_t required_components = 0;
    assert(quest_reward_recovery_pending_save_receipts(actor.pid, &fallback, &required_components));
    assert(fallback.size() == 1 && fallback[0].offering_operation.bytes == operation.bytes &&
           fallback[0].amount == 75 && fallback[0].reward_index == 1 &&
           actor.only.pc->skills[47].learned == 1 && required_components ==
           (PLAYER_COMPONENT_STATUS | PLAYER_COMPONENT_TROPHIES | PLAYER_COMPONENT_SKILLS));
    revision_available = false;
    quest_reward_recovery_pulse();
    revision_available = true;
    assert(acked == refused_acks_before);
    assert(quest_reward_recovery_pending_save_receipts(actor.pid, &fallback, &required_components));
    assert(fallback.size() == 1);
    quest_reward_recovery_save_acknowledged(actor.pid, save_revision.current_revision + 1,
                                            fallback.data(), fallback.size());
    assert(acked == refused_acks_before + 1);
    assert(quest_reward_recovery_pending_save_receipts(actor.pid, &fallback, &required_components));
    assert(fallback.empty() && required_components == 0);
    // The same fallback protects a descriptor-less peer, with no owner ACK.
    pending.version = 5;
    pending.reward_count = 1;
    pending.rewards[0] = {QUEST_GOAL_EXP, 100, 0, 75};
    pending.credited_count = pending.party_size = 2;
    pending.credited_pids[1] = teammate_linkdead.pid;
    pending.xp_award_count = 2;
    pending.xp_awards[0] = {static_cast<uint32_t>(actor.pid), 0, 75};
    pending.xp_awards[1] = {static_cast<uint32_t>(teammate_linkdead.pid), 0, 50};
    operation.bytes[0] = 31;
    quest_reward_recover_xp_entitlement(&teammate_linkdead, operation, pending, 0, 50);
    quest_reward_recover_xp_entitlement(&teammate_linkdead, operation, pending, 0, 50);
    assert(xp_gained == refused_xp_before + 125);
    assert(quest_reward_recovery_pending_save_receipts(teammate_linkdead.pid, &fallback, &required_components));
    assert(fallback.size() == 1 && fallback[0].amount == 50);
    quest_reward_recovery_save_acknowledged(teammate_linkdead.pid, save_revision.current_revision + 1,
                                            fallback.data(), fallback.size());
    assert(quest_reward_recovery_pending_save_receipts(teammate_linkdead.pid, &fallback, &required_components));
    assert(fallback.empty() && acked == refused_acks_before + 1);
    // Receipt capacity must refuse before a 65th live XP application, then
    // become available again after the exact completion of the first batch.
    pending.version = 4;
    pending.credited_count = pending.party_size = 1;
    const auto capacity_xp_before = xp_gained;
    for (unsigned index = 0; index < 64; ++index) {
        operation.bytes[0] = 100 + index;
        quest_reward_recover_pending(&actor, operation, pending, 0);
    }
    assert(xp_gained == capacity_xp_before + 64 * 75);
    operation.bytes[0] = 200;
    quest_reward_recover_pending(&actor, operation, pending, 0);
    assert(xp_gained == capacity_xp_before + 64 * 75);
    assert(quest_reward_recovery_pending_save_receipts(actor.pid, &fallback, &required_components));
    assert(fallback.size() == 64);
    quest_reward_recovery_save_acknowledged(actor.pid, save_revision.current_revision + 1,
                                            fallback.data(), fallback.size());
    assert(quest_reward_recovery_pending_save_receipts(actor.pid, &fallback, &required_components));
    assert(fallback.empty());
    quest_reward_recover_pending(&actor, operation, pending, 0);
    assert(xp_gained == capacity_xp_before + 65 * 75);
    xp_save_refused = false;
    // A mixed recovered reward must publish all economic effects before sealing
    // its XP save. Otherwise native custody can advance past the saved graph.
    quest_reward_recoveries.clear();
    pending.version = 4;
    pending.reward_count = 3;
    pending.rewards[0] = {QUEST_GOAL_ITEM, 88, 0, 0};
    pending.rewards[1] = {QUEST_GOAL_COINS, 50, 0, 0};
    pending.rewards[2] = {QUEST_GOAL_EXP, 75, 0, 75};
    operation.bytes[0] = 200;
    const auto mixed_saves = xp_save_requests, mixed_acks = acked;
    quest_reward_recover_pending(&actor, operation, pending, 0);
    assert(xp_save_requests == mixed_saves && acked == mixed_acks);
    auto mixed_recovery_item = queued_grants.back();
    mixed_recovery_item.grant_object->carrier = &actor;
    mixed_recovery_item.completion(&actor, mixed_recovery_item.grant_object->obj_uid, true, 0);
    quest_reward_recovery_pulse();
    assert(xp_save_requests == mixed_saves && acked == mixed_acks);
    auto mixed_cash = queued_currencies.back();
    mixed_cash.completion(&actor, true, {}, 0, mixed_cash.context.data(), mixed_cash.context.size());
    assert(xp_save_requests == mixed_saves + 1 && acked == mixed_acks);
    assert(queued_xp_receipts.size() == 1 && queued_xp_receipts[0].reward_index == 2);
    quest_reward_recovery_save_acknowledged(actor.pid, save_revision.current_revision,
                                            queued_xp_receipts.data(), queued_xp_receipts.size());
    assert(acked == mixed_acks + 1);
    // Only native verified slots suppress economic delivery on cold recovery.
    pending.version = 2;
    pending.reward_count = 2;
    pending.rewards[0] = {QUEST_GOAL_ITEM, 88, 0, 0};
    pending.rewards[1] = {QUEST_GOAL_COINS, 50, 0, 0};
    operation.bytes[0] = 201;
    const auto paid_grants = queued_grants.size(), paid_cash = queued_currencies.size();
    const auto paid_acks = acked, paid_messages = messages;
    quest_reward_recover_pending(&actor, operation, pending, 0, 3);
    assert(queued_grants.size() == paid_grants && queued_currencies.size() == paid_cash && acked == paid_acks + 1);
    operation.bytes[0] = 202;
    quest_reward_recover_pending(&actor, operation, pending, 0, 1);
    assert(queued_grants.size() == paid_grants && queued_currencies.size() == paid_cash + 1 && acked == paid_acks + 1);
    auto remaining_cash = queued_currencies.back();
    remaining_cash.completion(&actor, true, {}, 0, remaining_cash.context.data(), remaining_cash.context.size());
    assert(acked == paid_acks + 2);
    operation.bytes[0] = 203;
    quest_reward_recover_pending(&actor, operation, pending, 0, 3, false);
    assert(queued_grants.size() == paid_grants && queued_currencies.size() == paid_cash + 1 && acked == paid_acks + 2);
    assert(messages > paid_messages);
    operation.bytes[0] = 204;
    quest_reward_recover_pending(&actor, operation, pending, 0, 4);
    assert(queued_grants.size() == paid_grants && queued_currencies.size() == paid_cash + 1 && acked == paid_acks + 2);
}
P_obj unequip_char(P_char mob, int slot) {
    P_obj object = mob->equipment[slot];
    mob->equipment[slot] = nullptr;
    return object;
}
void extract_char(P_char) {}
'''

with tempfile.TemporaryDirectory(prefix="duris-quest-offering-") as directory:
    cpp = Path(directory) / "quest.cpp"
    exe = Path(directory) / "quest.exe"
    cpp.write_text(program, encoding="utf-8")
    subprocess.run(["g++", "-std=c++20", "-O0", str(cpp), "-lcrypto", "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)

print("durable quest offering regression passed")
