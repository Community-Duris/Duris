#!/usr/bin/env python3
"""Regression contracts for starter-grant retries and reconciliation safety."""

import re
import subprocess
import tempfile
import os
from pathlib import Path
from _paths import extract_function


ROOT = Path(__file__).resolve().parents[2]
MOVEMENT_PATH = ROOT / "src/item/item_movement_transaction.c"
LOAD_PATH = ROOT / "src/player/player_load_items.c"
MOVEMENT = MOVEMENT_PATH.read_text(encoding="utf-8", errors="replace")
LOAD = LOAD_PATH.read_text(encoding="utf-8", errors="replace")


def function_body(source: str, signature: str, *, last: bool = False) -> str:
    start = source.rindex(signature) if last else source.index(signature)
    brace = source.index("{", start)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


def check(label: str, condition: bool) -> None:
    if not condition:
        raise AssertionError(label)
    print(f"[PASS] {label}")


QUEUE_GRANT = function_body(MOVEMENT, "bool queue_creation_grant(")
RECONCILE = function_body(MOVEMENT, "bool reconcile_creation_grant_batch(")
BATCH_COMPLETE = function_body(MOVEMENT, "void creation_grant_batch_completion(")
SINGLE_COMPLETE = function_body(MOVEMENT, "void creation_grant_completion(")
LIVE_READY = function_body(MOVEMENT, "bool creation_grant_batch_live_ready(")
REQUEST_READY = function_body(MOVEMENT, "bool creation_grant_request_live_ready(")
MATERIALIZE = function_body(
    LOAD, "bool player_load_item_graph_materialize_creation("
)

# A transient conflict must leave the gameplay-created object in the queue.  Returning
# true tells callers not to extract it; pump_creation_grants() retries it later.
transient = QUEUE_GRANT.find("if (item_movement_reject_is_transient(reject))")
pop = QUEUE_GRANT.find("queue.requests.pop_back()")
check("queue_creation_grant retains transient rejections", transient >= 0 and transient < pop)
check("queue_creation_grant retains the queued request on transient conflict",
      "return true;" in QUEUE_GRANT[transient:pop])

# Single-grant completions use the same retained publication-repair boundary as
# batch completions; erasing the pending iterator before the callback would lose
# a failure and invalidate the iterator when a successful callback queues next work.
check("single grants use the publication helper", "publish_creation_grant(actor, request)" in SINGLE_COMPLETE)
check("single publication failures retain the queue",
      "note_creation_grant_publication_failure(actor, queue" in SINGLE_COMPLETE and
      "return;" in SINGLE_COMPLETE[SINGLE_COMPLETE.find("note_creation_grant_publication_failure"):])
check("single completion erases pending by stable key after callback",
      "const std::string pending_key = found->first;" in MOVEMENT and
      "pending.erase(pending_key);" in MOVEMENT and
      "pending.erase(found);" not in SINGLE_COMPLETE)
check("single missing graphs use detached reconciliation",
      "!entry.creation_batch && entry.completion == creation_grant_completion && committed &&" in MOVEMENT and
      re.search(r"reconcile_creation_grant_batch\(actor, entry, queue_found->second, result,\s*false\)",
                MOVEMENT, re.S) is not None)

check("single reconciliation is scoped to one request",
      re.search(r"!entry\.creation_batch.*?entry\.payload\.reason == item_transfer_reason::creation",
                MOVEMENT, re.S) is not None and
      "roots.size() != request_count" in RECONCILE and
      re.search(r"creation_grant_request_live_ready\(actor,\s*queue_found->second\.requests\.front\(\)\)",
                MOVEMENT, re.S) is not None)

# A partially published batch must be verified as actually carried, not merely
# present in NOWHERE, before its queue/pending completion is discarded.
check("batch publication has an actor-carried postcondition",
      "creation_grant_batch_published(actor, queue)" in BATCH_COMPLETE)
check("batch publication failure retains the queue",
      "note_creation_grant_publication_failure(actor, queue, actor_pid)" in BATCH_COMPLETE and
      re.search(r"creation_grant_batch_published\(actor, queue\).*?\{.*?note_creation_grant_publication_failure",
                BATCH_COMPLETE, re.S) is not None)
check("published predicate requires actor carriage",
      "OBJ_CARRIED_BY(object, actor)" in LIVE_READY or
      "OBJ_CARRIED_BY(object, actor)" in MOVEMENT)
check("single request live-ready resolves every intended destination",
      "OBJ_NOWHERE(object)" in REQUEST_READY and
      "request.to_room" in REQUEST_READY and
      "OBJ_IN_ROOM(object, request.room)" in REQUEST_READY and
      "request.recipient_pid" in REQUEST_READY and
      "OBJ_CARRIED_BY(object, recipient)" in REQUEST_READY and
      "OBJ_CARRIED_BY(container, recipient)" in REQUEST_READY and
      "OBJ_INSIDE_OBJ(object, container)" in REQUEST_READY)
check("single request live-ready never substitutes the submitting actor",
      "OBJ_CARRIED_BY(object, actor)" not in REQUEST_READY)

# Reconciliation must be able to replace a partially carried/malformed graph.  It
# stages the replacement while old UIDs are hidden, restores them on failure, and
# extracts old roots only after successful materialization.
check("reconciliation handles actor-carried or nested existing objects",
      "if (object && !OBJ_NOWHERE(object))" not in RECONCILE and
      "stage_displaced_creation_objects" in RECONCILE)
check("reconciliation stages displaced live objects",
      "displaced_creation_object" in MOVEMENT and
      "displaced" in RECONCILE and
      "object->obj_uid = 0" in MOVEMENT)
check("reconciliation restores old UIDs after materialization failure",
      "restore_displaced_creation_objects" in RECONCILE)
check("reconciliation removes old graphs only after successful staging",
      RECONCILE.find("player_load_item_graph_materialize_creation") >= 0 and
      RECONCILE.find("extract_displaced_creation_objects") >
      RECONCILE.find("player_load_item_graph_materialize_creation"))
check("reconciliation cleans staged roots on every failed validation",
      RECONCILE.count("extract_creation_roots(roots)") >= 2)
check("reconciliation handles nested children and duplicate UID matches",
      "object_list" in MOVEMENT and "stage_displaced_creation_objects(entry.payload" in RECONCILE)

# The public materializer must reject malformed VNUMs before real_object().
check("creation materializer rejects non-positive VNUMs",
      "entry->vnum <= 0" in MATERIALIZE)

print("[PASS] creation grant retry/reconciliation safety contracts")

# Compile the production admission predicates with real revision state. Control
# the async boundaries explicitly so both save-first and grant-first ordering
# are deterministic, including inbound grants and dirty-but-unsealed changes.
PIPELINE = (ROOT / "src/player/player_save_pipeline.c").read_text()
CONFLICTS = function_body(MOVEMENT, "bool creation_grant_conflicts(")
CAPTURE = function_body(PIPELINE, "static player_save_pipeline_result checkpoint_dirty_with_quest_xp(")
LITERAL_STATE = PIPELINE[PIPELINE.index("struct literal_inventory_checkpoint"):
                         PIPELINE.index("bool literal_inventory_blob(")]
SANITIZER_FLAGS = (["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
                   if os.environ.get("DURIS_TEST_SANITIZERS") == "1" else [])
check("grant admission fences sealed recipient saves before movement admission",
      CONFLICTS.index("player_save_pipeline_sealed_save_pending") < CONFLICTS.index("movement_conflicts"))
check("capture waits before queueing or sealing an unpublished grant",
      CAPTURE.index("item_creation_grant_player_publication_pending") < CAPTURE.index("player_revision_queue("))
harness = r'''
#include "player/player_save_pipeline.h"
#include "item/item_ownership_runtime.h"
#include <cassert>
#include <deque>
#include <mutex>
#include <unordered_map>
#include <array>
struct char_data { int pid; bool npc = false; };
#define GET_PID(ch) ((ch)->pid)
#define IS_NPC(ch) ((ch)->npc)
std::mutex pipeline_mutex;
player_save_pipeline_health health = {};
bool stop_requested = false, accepting = true;
int append_inflight_pid = 0;
bool quarantined = false, retained = false, worker_pending = false;
bool login_fenced = false, terminal_fenced = false;
bool player_save_journal_pid_quarantined(int) { return quarantined; }
void *find_target_save_login_fence_locked(int) { return login_fenced ? &health : nullptr; }
void *find_terminal_fence_locked(int) { return terminal_fenced ? &health : nullptr; }
bool any_snapshot_is_retained_locked(int) { return retained; }
bool player_save_worker_pid_pending(int) { return worker_pending; }
struct pending_creation_grant { uint64_t item_uid; uint32_t recipient_pid; bool to_room; };
struct creation_grant_queue { bool active; std::deque<pending_creation_grant> requests; };
std::unordered_map<uint32_t, creation_grant_queue> creation_grants;
const item_owner_identity system_owner_identity = {};
bool player_movement_conflict = false;
bool movement_conflicts(const item_owner_identity &, const item_owner_identity &) { return player_movement_conflict; }
bool item_ownership_runtime_lookup(uint64_t, item_ownership_runtime_entry *) { return false; }
item_owner_identity creation_grant_owner(const pending_creation_grant &request) {
    return {request.to_room ? item_owner_type::room : item_owner_type::player, request.recipient_pid, 0};
}
''' + LITERAL_STATE + extract_function("player_save_pipeline.c", "bool player_save_pipeline_sealed_save_pending(") + '\n' + CONFLICTS + '\n' + extract_function("item_movement_transaction.c", "bool item_creation_grant_player_publication_pending(") + r'''
int main() {
    char_data recipient{41}, other{42};
    pending_creation_grant grant{1001,41,false};
    health.initialized = true;
    assert(player_revision_hydrate(41, 7));
    auto &literal = literal_inventory_checkpoints[0];
    literal.token = {41, 71, 7001, 1};
    assert(find_literal_inventory_locked(41) == &literal);
    assert(player_save_pipeline_sealed_save_pending(41) && creation_grant_conflicts(grant));
    assert(!player_save_pipeline_sealed_save_pending(42));
    pending_creation_grant unrelated{1002,42,false};
    assert(!creation_grant_conflicts(unrelated));
    literal.held = true;
    assert(player_save_pipeline_sealed_save_pending(41) && creation_grant_conflicts(grant));
    assert(!creation_grant_conflicts(unrelated));
    player_revision_snapshot lease_state = {};
    assert(player_revision_snapshot_copy(41, &lease_state));
    assert(lease_state.current_revision == 7 && lease_state.acknowledged_revision == 7 &&
           !lease_state.dirty_components && !lease_state.queued_components);
    literal_inventory_checkpoints.fill({});
    assert(!player_save_pipeline_sealed_save_pending(41) && !creation_grant_conflicts(grant));
    player_revision_t revision;
    player_component_mask_t components;
    assert(player_revision_mark(41, PLAYER_COMPONENT_INVENTORY, &revision));
    assert(!creation_grant_conflicts(grant)); // Dirty marks do not seal old bytes.
    assert(player_revision_queue(41, &revision, &components));
    assert(creation_grant_conflicts(grant));
    creation_grants[99] = {false,{grant}};
    assert(!item_creation_grant_player_publication_pending(&recipient)); // Let old save drain.
    retained = true;
    assert(player_revision_begin_inflight(41, revision, components));
    worker_pending = true;
    assert(creation_grant_conflicts(grant));
    retained = false;
    assert(creation_grant_conflicts(grant)); // Worker result still awaits game-thread ACK.
    assert(player_revision_acknowledge(41, revision, components));
    assert(creation_grant_conflicts(grant)); // Worker slot must also drain.
    worker_pending = false;
    assert(!creation_grant_conflicts(grant));
    creation_grants[99].active = true; // Grant-first: fence capture until publication.
    assert(item_creation_grant_player_publication_pending(&recipient));
    assert(!item_creation_grant_player_publication_pending(&other));
    assert(player_revision_mark(41, PLAYER_COMPONENT_STATUS, nullptr));
    assert(!player_save_pipeline_sealed_save_pending(41)); // New dirty state does not deadlock grant.
    creation_grants[99].active = false;
    assert(!item_creation_grant_player_publication_pending(&recipient));
    assert(player_revision_queue(41, &revision, &components));
    assert(components == PLAYER_COMPONENT_STATUS);
    assert(player_revision_begin_inflight(41, revision, components));
    assert(player_revision_acknowledge(41, revision, components));
    for (bool *boundary : {&retained, &worker_pending, &login_fenced, &terminal_fenced, &quarantined, &stop_requested}) {
        *boundary = true;
        assert(creation_grant_conflicts(grant));
        *boundary = false;
    }
    append_inflight_pid = 41;
    assert(creation_grant_conflicts(grant));
    append_inflight_pid = 0;
    health.initialized = false;
    assert(creation_grant_conflicts(grant));
    health.initialized = true;
    accepting = false;
    assert(creation_grant_conflicts(grant));
    accepting = true;
    player_movement_conflict = true;
    assert(creation_grant_conflicts(grant)); // Genuine conflict stays refused.
    player_movement_conflict = false;
    grant.to_room = true;
    quarantined = true;
    assert(!creation_grant_conflicts(grant)); // Room grant has no player graph.
    creation_grants[99] = {true,{grant}};
    assert(!item_creation_grant_player_publication_pending(&recipient));
    assert(!item_creation_grant_player_publication_pending(nullptr));
}
'''
with tempfile.TemporaryDirectory(prefix="duris-grant-save-order-") as directory:
    program = Path(directory) / "ordering.cpp"
    binary = Path(directory) / "ordering"
    program.write_text(harness)
    subprocess.run(["g++", *SANITIZER_FLAGS, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-Isrc", str(program), "src/player/player_revision_state.c",
                    "-pthread", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
print("[PASS] deterministic save/grant ordering, inbound recipient, dirty progress, retained/worker/login/quarantine fences")
print("[PASS] actual held and unheld literal leases seal their recipient without fencing unrelated PID grants")

# Exercise the ordinary custody barrier using the actual retained-entry type,
# production predicates and real revision state. Service boundaries are driven
# explicitly; this focused test is not a SQL/native/publication certificate.
ORDINARY_SUBMIT = function_body(MOVEMENT, "static bool submit_movement(")
CUSTODY_PENDING = function_body(
    MOVEMENT, "bool item_movement_transaction_player_custody_pending(")
check("ordinary custody waits before minting an operation or submitting SQL",
      ORDINARY_SUBMIT.index("owner_sealed_save_pending") <
      ORDINARY_SUBMIT.index("critical_operation_id_generate"))
BATCH_SUBMIT = function_body(MOVEMENT, "bool item_movement_transaction_submit_batch(")
check("ordinary batch custody also waits before minting an operation",
      BATCH_SUBMIT.index("owner_sealed_save_pending") <
      BATCH_SUBMIT.index("critical_operation_id_generate"))
check("new full capture waits before queueing the immutable snapshot",
      CAPTURE.index("item_movement_transaction_player_custody_pending") <
      CAPTURE.index("player_revision_queue("))
check("capture-only observation never waits on its own Save state",
      "sealed_save_pending" not in CUSTODY_PENDING and
      "item_movement_transaction_player_busy" not in CUSTODY_PENDING)
ordinary_harness = r'''
#include "item/item_movement_state_internal.h"
#include "economy/economic_gameplay_authority.h"
#include "persistence/persistence_mode.h"
#include "core/utils.h"
#include <algorithm>
#include <cassert>
#include <mutex>
#include <unordered_map>
using item_movement_detail::pending_movement;
std::unordered_map<std::string, pending_movement> pending;
std::mutex pipeline_mutex;
player_save_pipeline_health health{};
bool stop_requested = false, accepting = true;
int append_inflight_pid = 0, retained_pid = 0, worker_pid = 0;
bool quarantined = false, requested_or_unconfigured = false;
bool authority_active = false, mysql_selected = true;
bool economic_outcome_accounting_requested() noexcept { return requested_or_unconfigured; }
bool economic_gameplay_authority::active() { return authority_active; }
bool persistence_mode_requires_mysql() { return mysql_selected; }
bool player_save_journal_pid_quarantined(int) { return quarantined; }
void *find_target_save_login_fence_locked(int) { return nullptr; }
void *find_terminal_fence_locked(int) { return nullptr; }
void *find_literal_inventory_locked(int) { return nullptr; }
bool any_snapshot_is_retained_locked(int pid) { return retained_pid == pid; }
bool player_save_worker_pid_pending(int pid) { return worker_pid == pid; }
void creation_grant_completion(P_char, bool, const item_transfer_result &,
                               unsigned int, const uint8_t *, size_t) {}
bool retained_publication(const critical_operation_id &, P_char, bool,
                          const item_transfer_result &, unsigned int,
                          const uint8_t *, size_t) { return false; }
''' + extract_function("player_save_pipeline.c", "bool player_save_pipeline_sealed_save_pending(") + '\n' + '\n'.join(
    function_body(MOVEMENT, signature) for signature in (
        "bool ordinary_sql_save_ordering(", "uint32_t owner_save_pid(",
        "bool owner_sealed_save_pending(", "bool ordinary_capture_owner(",
        "bool item_movement_transaction_player_custody_pending(")) + r'''
int main() {
    health.initialized = true;
    auto pc = std::make_unique<pc_only_data>();
    auto actor = std::make_unique<char_data>();
    pc->pid = 41;
    actor->only.pc = pc.get();
    const item_owner_identity player{item_owner_type::player, 41, 0};
    const item_owner_identity other{item_owner_type::player, 42, 0};
    const item_owner_identity pet{item_owner_type::pet, 99, 41};
    assert(player_revision_hydrate(41, 7));
    player_revision_t revision = 0;
    player_component_mask_t components = 0;
    assert(player_revision_mark(41, PLAYER_COMPONENT_INVENTORY, &revision));
    assert(!owner_sealed_save_pending(player)); // Dirty is not sealed.
    assert(player_revision_queue(41, &revision, &components));
    assert(owner_sealed_save_pending(player) && owner_sealed_save_pending(pet));
    assert(!owner_sealed_save_pending(other));
    retained_pid = 41;
    assert(player_revision_begin_inflight(41, revision, components));
    worker_pid = 41;
    retained_pid = 0;
    assert(owner_sealed_save_pending(player)); // SQL result alone is not ACK.
    assert(player_revision_acknowledge(41, revision, components));
    assert(owner_sealed_save_pending(player)); // Real worker/WAL boundary remains.
    worker_pid = 0;
    assert(!owner_sealed_save_pending(player));
    assert(player_revision_mark(41, PLAYER_COMPONENT_STATUS, &revision));
    auto &entry = pending.try_emplace("original-movement").first->second;
    entry.actor_pid = 41;
    entry.payload.from_owner = {item_owner_type::room, 77, 0};
    entry.payload.to_owner = player;
    entry.payload.reason = item_transfer_reason::player_get;
    assert(item_movement_transaction_player_custody_pending(actor.get()));
    assert(!owner_sealed_save_pending(player)); // No capture/self-wait cycle.
    entry.completion_ready = true;
    entry.registry_applied = true;
    assert(item_movement_transaction_player_custody_pending(actor.get()));
    entry.payload.reason = item_transfer_reason::trusted_steal;
    entry.publication_failed = true;
    assert(item_movement_transaction_player_custody_pending(actor.get()));
    entry.actor_pid = 42;
    entry.payload.from_owner = pet;
    entry.payload.to_owner = other;
    assert(item_movement_transaction_player_custody_pending(actor.get()));
    entry.payload.from_owner = {item_owner_type::room, 77, 0};
    entry.adopting = true;
    entry.requested_to_owner = player;
    assert(item_movement_transaction_player_custody_pending(actor.get()));
    entry.publication = retained_publication;
    assert(!item_movement_transaction_player_custody_pending(actor.get()));
    entry.publication = nullptr;
    entry.payload.reason = item_transfer_reason::craft;
    assert(!item_movement_transaction_player_custody_pending(actor.get()));
    entry.payload.reason = item_transfer_reason::player_get;
    entry.completion = creation_grant_completion;
    assert(!item_movement_transaction_player_custody_pending(actor.get()));
    entry.completion = nullptr;
    entry.creation_batch = true;
    assert(!item_movement_transaction_player_custody_pending(actor.get()));
    entry.creation_batch = false;
    entry.restored_sql_drop = true;
    assert(!item_movement_transaction_player_custody_pending(actor.get()));
    entry.restored_sql_drop = false;
    requested_or_unconfigured = true;
    assert(!ordinary_sql_save_ordering()); // Never infer OFF from !active.
    assert(!item_movement_transaction_player_custody_pending(actor.get()));
    requested_or_unconfigured = false;
    authority_active = true;
    assert(!ordinary_sql_save_ordering());
    authority_active = false;
    mysql_selected = false;
    assert(!ordinary_sql_save_ordering());
    mysql_selected = true;
    assert(item_movement_transaction_player_custody_pending(actor.get()));
    pending.erase("original-movement"); // Actual terminal publication boundary.
    assert(!item_movement_transaction_player_custody_pending(actor.get()));
    assert(player_revision_queue(41, &revision, &components));
    assert(components == PLAYER_COMPONENT_STATUS);
    assert(player_revision_begin_inflight(41, revision, components));
    assert(player_revision_acknowledge(41, revision, components));
    assert(!owner_sealed_save_pending(player));
    quarantined = true;
    assert(owner_sealed_save_pending(player));
}
'''
with tempfile.TemporaryDirectory(prefix="duris-ordinary-save-order-") as directory:
    program = Path(directory) / "ordering.cpp"
    binary = Path(directory) / "ordering"
    program.write_text(ordinary_harness)
    subprocess.run(["g++", *SANITIZER_FLAGS, "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                    "-Isrc", str(program), "src/player/player_revision_state.c",
                    "-pthread", "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
print("[PASS] ordinary save-first and custody-first ordering, true ACK, no capture cycle, retained AFTER owners")
