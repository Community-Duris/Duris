"""Exercise real corpse batch admission, codec, registry and publication with sanitizers."""
from pathlib import Path
import subprocess
import tempfile
from _paths import ROOT, SRC, extract_function
from test_item_movement_prompt_runtime import PRELUDE

prelude = PRELUDE.replace('void obj_from_char(P_obj) {}', r'''
void obj_from_char(P_obj obj) {
    P_obj *link = &obj->loc.carrying->carrying;
    while (*link != obj) link = &(*link)->next_content;
    *link = obj->next_content;
    obj->next_content = nullptr;
    obj->loc_p = 0;
}
''').replace('void obj_to_obj(P_obj, P_obj) {}', r'''
void obj_to_obj(P_obj obj, P_obj container) {
    obj->loc_p = LOC_INSIDE; obj->loc.inside = container;
    obj->next_content = container->contains; container->contains = obj;
    container->weight += obj->weight;
}
''')
completion_fixture = r'''
#include "classes/necromancy.h"
static unsigned corpse_writes, retry_wakes;
struct corpse_transfer_context { uint64_t corpse_uid, item_uid, corpse_save_id; };
P_obj corpse_live_item(uint64_t uid) {
    for (P_obj obj = object_list; obj; obj = obj->next) if (obj->obj_uid == uid) return obj;
    return nullptr;
}
void note_corpse_transfer_dispute(P_char ch) { ch->only.pc->death_custody_disputed = true; }
bool corpse_lifecycle_transaction_note_item_transfer(uint32_t, uint32_t, uint64_t) { return true; }
bool submit_next_corpse_item(P_char, P_obj) { abort(); }
void writeCorpse(P_obj corpse) { assert(corpse->contains); ++corpse_writes; }
static void wake_death_extract_retry(P_char) { ++retry_wakes; }
''' + extract_function('fight.c', 'void corpse_item_completion(')

driver = r'''
#include "classes/necromancy.h"
static unsigned callbacks;
static bool expected_commit;
static void completed(P_char actor, bool committed, const item_transfer_result &result, unsigned,
                      const uint8_t *, size_t) {
    ++callbacks;
    corpse_transfer_context context{9999,0,7};
    corpse_item_completion(actor,committed,result,committed ? 0 : EMSGSIZE,
                           reinterpret_cast<const uint8_t *>(&context),sizeof(context));
    assert(corpse_writes == (committed ? 1u : 0u));
    assert(retry_wakes == corpse_writes);
    assert(actor->only.pc->death_custody_disputed == !committed);
    assert(committed == expected_commit);
    assert(!item_movement_transaction_player_busy(actor));
    if (committed) assert(!actor->carrying);
}
static void run(int count, int scenario) {
    item_movement_transaction_reset_for_tests(); item_ownership_runtime_reset();
    callbacks = corpse_writes = retry_wakes = 0; busy_coin_uid = 0; expected_commit = scenario != 1;
    char_data actor{}; pc_only_data pc{}; actor.only.pc = &pc; pc.pid = 42;
    character_list = &actor; actor.in_room = 0;
    indexes[0].virtual_number = 100;
    const item_owner_identity source{item_owner_type::player,42,0};
    const item_owner_identity dest{item_owner_type::corpse,item_corpse_owner_id(42,7),0};
    std::vector<obj_data> objects(count + 1);
    std::vector<P_obj> roots;
    obj_data corpse{};
    corpse.obj_uid = 9999; corpse.type = ITEM_CORPSE;
    corpse.value[CORPSE_PID] = 42; corpse.value[CORPSE_SAVEID] = 7;
    corpse.value[CORPSE_FLAGS] = PC_CORPSE;
    corpse.action_description = const_cast<char *>("Fixture"); corpse.weight = 50;
    corpse.next = objects.data(); object_list = &corpse;
    for (int i = 0; i <= count; ++i) {
        auto &obj = objects[i]; obj.obj_uid = 100+i; obj.R_num = 0; obj.weight = 1;
        obj.next = i < count ? &objects[i+1] : nullptr;
        obj.loc_p = i == count ? LOC_INSIDE : LOC_CARRIED;
        if (i == count) obj.loc.inside = &objects[0]; else obj.loc.carrying = &actor;
        obj.next_content = i + 1 < count ? &objects[i+1] : nullptr;
        if (i < count) roots.push_back(&obj);
        assert(item_ownership_runtime_hydrate({(uint64_t)(100+i),
            i == count ? 100u : (uint64_t)(100+i), i == count ? 100u : 0u,
            source, 1, 3, 100, item_custody_state::active}));
    }
    objects[0].contains = &objects[count]; objects[0].weight = 2;
    actor.carrying = objects.data();
    assert(item_ownership_runtime_hydrate_owner(dest,7));
    item_movement_reject reject{};
    if (scenario == 3) busy_coin_uid = objects[0].obj_uid;
    bool accepted = item_movement_transaction_submit_batch(&actor, roots.data(), roots.size(),
        nullptr, source, dest, item_transfer_reason::corpse_create,7,completed,nullptr,0,
        &corpse,&reject);
    if (scenario == 3) {
        assert(!accepted && reject == item_movement_reject::pending_conflict);
        assert(actor.carrying && !corpse.contains && callbacks == 0); return;
    }
    assert(accepted && item_movement_transaction_health_copy().submitted == 1);
    item_transfer_payload payload{};
    assert(item_transfer_command_decode_payload(submitted,&payload));
    assert(payload.multi_root && payload.item_count == count+1);
    assert(payload.corpse.weight == 50+count+1);
    if (scenario == 2) objects[count].loc.inside = &corpse;
    if (scenario == 4) objects[0].contains = nullptr;
    critical_completion completion{}; completion.operation_id = submitted.operation_id;
    completion.outcome = expected_commit ? critical_apply_outcome::applied :
                                          critical_apply_outcome::terminal_failure;
    item_transfer_result result{100,(uint16_t)(count+1),4,8,2,1};
    std::array<uint8_t,ITEM_TRANSFER_RESULT_BYTES> encoded{};
    assert(item_transfer_command_encode_result(result,&encoded));
    completion.result_size = encoded.size();
    std::copy(encoded.begin(),encoded.end(),completion.result_payload.begin());
    item_movement_transaction_handle_completions(&completion,1);
    if (scenario == 2 || scenario == 4) {
        assert(callbacks == 0 && actor.carrying && !corpse.contains);
        assert(item_movement_transaction_player_busy(&actor));
        objects[count].loc.inside = &objects[0];
        objects[0].contains = &objects[count];
        item_movement_transaction_player_ready(&actor);
    }
    assert(callbacks == 1);
    assert(!item_movement_transaction_player_busy(&actor));
    assert(expected_commit ? !actor.carrying && corpse.contains : actor.carrying && !corpse.contains);
    if (expected_commit) {
        assert(corpse.weight == 50+count+1);
        for (auto &obj : objects) {
            item_ownership_runtime_entry entry{};
            assert(item_ownership_runtime_lookup(obj.obj_uid,&entry));
            assert(item_owner_identity_equal(entry.owner,dest));
        }
    }
    item_movement_transaction_handle_completions(&completion,1);
    assert(callbacks == 1); // duplicate completion cannot move or finalize twice
}
int main() {
    for (int count : {1,15,100}) for (int scenario : {0,1,2,3,4}) run(count,scenario);
    puts("corpse batches: nested topology, one command, refusal, pending coins, retention and replay passed");
}
'''
(ROOT/'bin/tests').mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='corpse-batch-', dir=ROOT/'bin/tests') as tmp:
    source=Path(tmp)/'harness.cpp'; binary=Path(tmp)/'harness'
    source.write_text(prelude+'\n'+completion_fixture+'\n'+driver)
    subprocess.run(['g++','-std=c++20','-g','-O1','-ffunction-sections','-fdata-sections',
        '-fsanitize=address,undefined','-Isrc',str(source),
        *[str(SRC/name) for name in ['item_movement_transaction.c','item_ownership_runtime.c',
        'item_transfer_command.c','critical_command.c','player_snapshot_capture.c','player_snapshot_codec.c']],
        '-Wl,--gc-sections','-lcrypto','-o',str(binary)],cwd=ROOT,check=True,timeout=180)
    subprocess.run([str(binary)],check=True,timeout=30)

# Exercise the production wake-up helper independently of coordinator dispatch.
wake_prelude = r'''
#include "core/prototypes.h"
#include "core/utils.h"
#include <cassert>
static nevent_data event{};
static bool scheduled;
static unsigned wakeups;
static uint64_t now = 900;
P_nevent get_scheduled(P_char, event_func_type) { return scheduled ? &event : nullptr; }
nevent_handle nevent_handle_from_event(P_nevent p) { return {p, 1}; }
bool nevent_reschedule_after(nevent_handle h, unsigned long long delay) {
    assert(h.event == &event && delay == 0); ++wakeups; return true;
}
uint64_t persistence_observability_now_usec() { return now; }
static void event_death_extract_retry(P_char, P_char, P_obj, void *) { abort(); }
'''
wake_driver = r'''
int main() {
    char_data actor{}; pc_only_data pc{}; actor.only.pc = &pc;
    actor.specials.position = POS_STANDING | STAT_DEAD;
    scheduled = true;
    wake_death_extract_retry(&actor); assert(wakeups == 1);
    scheduled = false; pc.death_retry_due_usec = 2000;
    wake_death_extract_retry(&actor);
    assert(wakeups == 1 && pc.death_retry_due_usec == now);
    actor.specials.position = POS_STANDING | STAT_NORMAL;
    pc.death_retry_due_usec = 2000; scheduled = true;
    wake_death_extract_retry(&actor);
    assert(wakeups == 1 && pc.death_retry_due_usec == 2000);
    wake_death_extract_retry(nullptr);
}
'''
with tempfile.TemporaryDirectory(prefix='corpse-wakeup-', dir=ROOT/'bin/tests') as tmp:
    source = Path(tmp)/'wake.cpp'; binary = Path(tmp)/'wake'
    source.write_text(wake_prelude + extract_function('fight.c',
        'static void wake_death_extract_retry(P_char ch)\n{') + wake_driver)
    subprocess.run(['g++','-std=c++20','-Isrc',str(source),'-o',str(binary)],
                   cwd=ROOT,check=True,timeout=30)
    subprocess.run([str(binary)],check=True,timeout=10)
print('corpse completion wakes the retry event/fallback without inline extraction')
