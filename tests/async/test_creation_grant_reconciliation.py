#!/usr/bin/env python3
"""Regression contracts for starter-grant retries and reconciliation safety."""

import re
from pathlib import Path


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
LIVE_READY = function_body(MOVEMENT, "bool creation_grant_batch_live_ready(")
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
