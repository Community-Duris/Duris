#!/usr/bin/env python3
"""Historical ordinary-pickup ownership characterization; not native qualification.

Uses unchanged shared owners and extracted pickup callbacks. Accounting is
inactive/schema 1; SQL apply, snapshot capture and native placement are doubles.
The terminal-outcome control is not a canonical SQL rejection receipt. Journals
default to POSIX tmpfs: these checks do not prove disk or crash durability.
Existing retention controls are read as a literal, never imported or executed.
"""

import argparse
import ast
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import tempfile
import time

from _paths import ROOT, extract_function

CALLBACK_HASHES = {
    "P_obj find_live_item_uid(": "214894bd143f18e2d302fa8544d531daab35aeef2807f158482471e89831e027",
    "void item_get_completion(": "dbf71bcb2118320aeee5dc7270cda6466467dc683a34a41e8fc5acdf59baa4ea",
}
CASES = ("success", "rejected", "missing_actor", "replacement_body",
         "stale_topology", "handler_rejected", "throw_before", "throw_after")


def replace_once(text, original, replacement):
    assert text.count(original) == 1, f"changed fixture binding: {original}"
    return text.replace(original, replacement, 1)


def build_program():
    fixture = ROOT / "tests/async/test_publication_retention_runtime.py"
    assignments = [node.value for node in ast.parse(fixture.read_text()).body
                   if isinstance(node, ast.Assign) and any(
                       isinstance(target, ast.Name) and target.id == "HARNESS"
                       for target in node.targets)]
    assert len(assignments) == 1, "expected one literal retention HARNESS"
    harness = ast.literal_eval(assignments[0])
    assert isinstance(harness, str) and harness.count("bool publication_callback(") == 1
    prelude = harness.split("bool publication_callback(", 1)[0]
    prelude += '\n#include "classes/necromancy.h"\n'
    for unused in ("static item_ownership_runtime_entry runtime_entry = {};",
                   "static item_ownership_runtime_entry pouch_runtime = {};",
                   "static int publication_attempts = 0;", "static int completion_calls = 0;"):
        prelude = replace_once(prelude, unused, "")
    # Remove the original harness's registry doubles: link the actual provider.
    assert prelude.count("bool item_ownership_runtime_lookup(") == 1
    assert prelude.count("bool currency_transaction_coin_item_busy(") == 1
    start = prelude.index("bool item_ownership_runtime_lookup(")
    end = prelude.index("bool currency_transaction_coin_item_busy(", start)
    prelude = prelude[:start] + prelude[end:]
    prelude += "\nbool nevent_is_game_thread() { return std::this_thread::get_id() == fixture_game_thread; }\n"
    prelude = replace_once(prelude,
        "void persistence_alert(int, const char *, const char *, const char *, const char *, const char *, const char *, ...) {}",
        "static int alerts = 0;\nvoid persistence_alert(int, const char *, const char *, const char *, const char *, const char *, const char *, ...) { ++alerts; }")

    actobj = (ROOT / "src/cmd/actobj.c").read_text()
    phase_declaration = "static get_outcome get_with_phase(P_char ch, P_obj object, P_obj container"
    for marker in ("struct get_movement_context", "struct synchronous_get_item",
                   "enum class get_phase", phase_declaration):
        assert actobj.count(marker) == 1, f"changed callback context marker: {marker}"
    context = actobj[actobj.index("struct get_movement_context"):actobj.index("struct synchronous_get_item")]
    enums = actobj[actobj.index("enum class get_phase"):actobj.index(phase_declaration)]
    functions = {signature: extract_function("actobj.c", signature) for signature in CALLBACK_HASHES}
    hashes = {signature: hashlib.sha256(body.encode()).hexdigest() for signature, body in functions.items()}
    assert hashes == CALLBACK_HASHES, "pickup callback changed; review historical controls before updating pins"
    return prelude + context + enums + SHIM + "\n".join(functions.values()) + DRIVER, hashes

PROVIDERS = (
    "src/account/character_identity.c", "src/item/item_movement_transaction.c",
    "src/economy/native_quest_cost.c", "src/economy/native_quest_coin_give.c",
    "src/economy/shop_trade_recovery_manifest.c", "src/item/lockpick_retirement_continuation.c",
    "src/item/item_ownership_runtime.c", "src/item/item_transfer_command.c",
    "src/world/quest_mobile_native_reference.c", "src/item/craft_pouch_mutation.c",
    "src/combat/chaos_pouch_ledger.c", "src/combat/chaos_pouch_publication.c",
    "src/player/player_snapshot_codec.c", "src/economy/item_transfer_accounting.c",
    "src/economy/economic_accounting_types.c", "src/economy/economic_accounting_plan.c",
    "src/economy/economic_source_event.c", "src/economy/economic_accounting_intent.c",
    "src/persistence/critical_command.c", "src/persistence/critical_command_journal.c",
    "src/persistence/critical_command_coordinator.c",
)

# Disclosed native placement double and abort-if-reached unrelated owners.
SHIM = r'''
#include <stdexcept>
#include "player/player_save_pipeline.h"
#include "item/ordinary_drop_recovery.h"
#include "cmd/lockpick_retirement.h"
// Link-only unrelated branch dependencies: none may be reached by this case.
bool item_actions_object_busy(uint64_t) { std::abort(); }
bool player_save_pipeline_literal_inventory_hold(const player_literal_inventory_token &, const critical_operation_id &) { std::abort(); }
bool player_save_pipeline_literal_inventory_release(const player_literal_inventory_token &, const critical_operation_id &) { std::abort(); }
bool player_save_restored_publication_owner::publish(const critical_completion &) noexcept { std::abort(); }
bool player_save_held_retirement_publication_owner::publish_held_retirement(
 const critical_command &, const critical_completion &, const held_retirement_publication_snapshot &,
 bool (*)(const critical_command &, const critical_completion &, void *) noexcept, void *) noexcept { std::abort(); }
bool lockpick_retirement_publication(const critical_operation_id &, P_char, bool,
 const item_transfer_result &, unsigned int, const uint8_t *, size_t) noexcept { std::abort(); }
player_snapshot_capture_result player_item_snapshot_tree_capture_literal(P_obj,
 std::vector<player_item_snapshot> *, size_t *) { std::abort(); }
inert_item_stage_result ordinary_drop_recovery_eligibility(const player_item_snapshot *, size_t) noexcept { std::abort(); }
ordinary_drop_observation ordinary_drop_recovery_publish_live(const critical_command &,
 const critical_completion &, uint64_t, ordinary_drop_live_publication_state &) noexcept { std::abort(); }
static int handler_calls = 0;
static bool throw_before = false, throw_after = false, reject_handler = false;
static get_outcome get_with_phase(P_char actor, P_obj object, P_obj, int, get_phase phase) {
    assert(phase == get_phase::publication);
    ++handler_calls;
    if (throw_before) throw std::runtime_error("component handler boundary before effect");
    if (reject_handler) return get_outcome::rejected;
    object->loc_p = LOC_CARRIED;
    object->loc.carrying = actor;
    actor->carrying = object;
    if (throw_after) throw std::runtime_error("component handler boundary after effect");
    return get_outcome::placed;
}
bool corpse_lifecycle_transaction_note_item_transfer(uint32_t, uint32_t, uint64_t) { std::abort(); }
void CheckEqWorthUsing(P_char, P_obj) { std::abort(); }
'''

# These assertions preserve the reviewed historical ownership semantics.
DRIVER = r'''
int main(int argc, char **argv) {
    assert(argc == 3);
    const std::string scenario = argv[2];
    fixture_check_runtime_identity_retirement();
    index_data indexes[1]{}; indexes[0].virtual_number = 42; obj_index = indexes;
    pc_only_data pc{}; pc.pid = 1001;
    char_data actor{}; actor.only.pc = &pc; actor.runtime_id = 7001; actor.in_room = 0;
    fixture_character_registration identity(&actor); character_list = &actor;
    obj_data object{}; object.obj_uid = 5001; object.R_num = 0;
    object.loc_p = LOC_ROOM; object.loc.room = 0; object_list = &object;
    const item_owner_identity room{item_owner_type::room, 120, 0};
    const item_owner_identity player{item_owner_type::player, 1001, 0};
    assert(item_ownership_runtime_hydrate({5001,5001,0,room,1,1,42,item_custody_state::active}));
    assert(item_ownership_runtime_hydrate_owner(player,1));
    assert(critical_command_coordinator_init(argv[1], apply_transfer, nullptr, 1,
        nullptr, nullptr, item_transfer_accounting_command_supported));
    const get_movement_context context{5001,0,0,1};
    item_movement_reject reject{};
    if (scenario == "rejected") forced_outcome = critical_apply_outcome::terminal_failure;
    assert(item_movement_transaction_submit(&actor,&object,nullptr,room,player,
        item_transfer_reason::player_get,5001,item_get_completion,&context,sizeof(context),nullptr,&reject));
    assert(item_movement_transaction_health_copy().pending == 1);
    critical_operation_id original{};
    assert(critical_command_coordinator_is_fenced({critical_entity_type::item,5001},&original));
    const auto journal_before = critical_command_journal_health_copy().records;
    // Await actual worker dispatch and journal checkpoint; no completion is fabricated.
    critical_completion incoming[8]{}; size_t count = 0;
    for (int spin=0; spin<3000 && !count; ++spin) {
        count = critical_command_coordinator_pulse(incoming,8);
        if (!count) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(count == 1 && incoming[0].operation_id.bytes == original.bytes);
    assert(!critical_command_coordinator_is_fenced({critical_entity_type::item,5001},nullptr));
    assert(!critical_command_coordinator_is_fenced({critical_entity_type::player,1001},nullptr));
    assert(critical_command_journal_health_copy().records == 0);
    assert(critical_command_coordinator_health_copy().publication_pending == 0);
    if (scenario == "missing_actor" || scenario == "replacement_body") character_list = nullptr;
    if (scenario == "stale_topology") object.loc.room = 1;
    if (scenario == "throw_before") throw_before = true;
    if (scenario == "throw_after") throw_after = true;
    if (scenario == "handler_rejected") reject_handler = true;
    bool escaped = false;
    try { item_movement_transaction_handle_completions(incoming,count); }
    catch (const std::runtime_error &) { escaped = true; }
    item_ownership_runtime_entry projected{};
    assert(item_ownership_runtime_lookup(5001,&projected));
    if (scenario == "missing_actor" || scenario == "replacement_body") {
        assert(item_movement_transaction_health_copy().pending == 1 && handler_calls == 0);
        assert(item_owner_identity_equal(projected.owner,room));
        // Empty pulses do not retry this nonretained callback. Readiness does.
        item_movement_transaction_handle_completions(nullptr,0);
        assert(handler_calls == 0 && item_movement_transaction_health_copy().pending == 1);
        char_data replacement = actor; replacement.runtime_id = 7002;
        if (scenario == "replacement_body") {
            fixture_register_character(&replacement); character_list = &replacement;
            item_movement_transaction_player_ready(&replacement);
            assert(OBJ_CARRIED_BY(&object,&replacement));
            fixture_retire_character(&replacement);
        } else {
            character_list = &actor; item_movement_transaction_player_ready(&actor);
            assert(OBJ_CARRIED_BY(&object,&actor));
        }
        character_list = &actor;
        assert(item_ownership_runtime_lookup(5001,&projected));
    }
    assert(item_movement_transaction_health_copy().pending == 0);
    assert(critical_command_journal_health_copy().records == 0);
    if (scenario == "rejected") {
        assert(item_owner_identity_equal(projected.owner,room) && handler_calls == 0);
    } else {
        assert(item_owner_identity_equal(projected.owner,player) && projected.item_revision == 2);
    }
    if (scenario == "stale_topology") assert(handler_calls == 0 && alerts == 1 && OBJ_ROOM(&object));
    if (scenario == "handler_rejected") assert(handler_calls == 1 && alerts == 1 && OBJ_ROOM(&object));
    if (scenario == "throw_before") assert(escaped && handler_calls == 1 && OBJ_ROOM(&object));
    if (scenario == "throw_after") assert(escaped && handler_calls == 1 && OBJ_CARRIED_BY(&object,&actor));
    if (scenario == "success") assert(!escaped && handler_calls == 1 && OBJ_CARRIED_BY(&object,&actor));
    const int before_retry = handler_calls;
    item_movement_transaction_handle_completions(nullptr,0);
    item_movement_transaction_player_ready(&actor);
    assert(handler_calls == before_retry);
    char op[33]{}; assert(critical_operation_id_to_hex(original,op,sizeof(op)));
    printf("OBS scenario=%s schema=1 active=0 mysql=0 id=%s journal_before=%llu journal_after=0 coordinator_fenced=0 pending=0 handler_calls=%d alerts=%d escaped=%d projected_owner=%u physical_carried=%d\n",
        scenario.c_str(),op,(unsigned long long)journal_before,handler_calls,alerts,escaped,
        (unsigned)projected.owner.type,OBJ_CARRIED(&object));
    critical_command_coordinator_shutdown();
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-root", type=Path,
                        default=Path(os.environ.get("BIN_ROOT", ROOT / "bin")))
    parser.add_argument("--evidence-root", type=Path, default=Path(tempfile.gettempdir()))
    parser.add_argument("--journal-root", type=Path,
                        default=Path("/dev/shm") if Path("/dev/shm").is_dir()
                        else Path(tempfile.gettempdir()))
    args = parser.parse_args()
    # No compatibility shim or missing-provider substitution on older source bases.
    missing = [path for path in PROVIDERS if not (ROOT / path).is_file()]
    if missing:
        raise SystemExit("requires published-primary runtime providers; missing: " + ", ".join(missing))
    program, callbacks = build_program()
    for directory in (args.build_root, args.evidence_root, args.journal_root):
        directory.mkdir(parents=True, exist_ok=True)
    build = Path(tempfile.mkdtemp(prefix="ordinary-pickup-", dir=args.build_root.resolve()))
    evidence = Path(tempfile.mkdtemp(prefix="ordinary-pickup-", dir=args.evidence_root.resolve()))
    native = Path(tempfile.mkdtemp(prefix="ordinary-pickup-", dir=args.journal_root.resolve()))
    print(f"evidence={evidence} build={build} journals={native}", flush=True)
    status = native.stat()
    if not hasattr(os, "getuid") or status.st_uid != os.getuid() or stat.S_IMODE(status.st_mode) & 0o077:
        raise SystemExit("critical journals require an owner-only POSIX path; use --journal-root /dev/shm on WSL")
    generated, binary = build / "pickup_component.cpp", build / "pickup_component"
    generated.write_text(program, encoding="utf-8")
    inputs = set(PROVIDERS) | {
        "src/cmd/actobj.c", "src/classes/necromancy.h",
        "tests/async/test_publication_retention_runtime.py", "tests/async/_paths.py",
        "tests/async/contract_text.py", "tests/async/character_identity_test_fixture.h",
    }
    command = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
        "-D__NO_MYSQL__", "-pthread", "-ffunction-sections", "-fdata-sections",
        "-I"+str(ROOT / "src"), "-I"+str(ROOT / "src/no_mysql"),
        "-I"+str(ROOT / "tests/async"), str(generated), "-g", "-Og",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        *(str(ROOT / path) for path in PROVIDERS), "-Wl,--gc-sections", "-lz", "-lcrypto",
        "-o", str(binary)]
    environment = dict(os.environ, TMPDIR=str(evidence), TEMP=str(evidence), TMP=str(evidence),
        ASAN_OPTIONS="detect_leaks=1:halt_on_error=1", UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    report = {"component_only": True, "schema": 1, "accounting_active": False, "mysql": False,
        "native_placement": "double", "apply": "double", "snapshot_capture": "double",
        "journal_root": str(native), "disk_or_crash_durability_qualified": False,
        "test_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "inputs": {path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest() for path in sorted(inputs)},
        "callbacks": callbacks, "generated_sha256": hashlib.sha256(generated.read_bytes()).hexdigest(),
        "command": command, "compile_timeout_seconds": 900, "case_timeout_seconds": 30, "cases": []}

    def save_report():
        (evidence / "results.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")

    save_report()
    started = time.monotonic()
    with (evidence / "compile.log").open("w") as output:
        try:
            compiled = subprocess.run(command, cwd=ROOT, env=environment,
                stdout=output, stderr=subprocess.STDOUT, timeout=900)
        except subprocess.TimeoutExpired:
            report["compile_timeout"] = True
            save_report()
            raise
    report["compile_seconds"] = time.monotonic()-started
    report["compile_exit_code"] = compiled.returncode
    save_report()
    compiled.check_returncode()
    report["binary_sha256"] = hashlib.sha256(binary.read_bytes()).hexdigest()
    print("PASS: strict component compile; running eight historical controls", flush=True)
    for case in CASES:
        journal = native / ("journal-"+case)
        started = time.monotonic()
        try:
            done = subprocess.run([str(binary), str(journal), case], cwd=ROOT, env=environment,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=30)
        except subprocess.TimeoutExpired as error:
            (evidence / (case+".log")).write_bytes(error.stdout or b"")
            report["cases"].append({"name": case, "timeout": True})
            save_report()
            raise
        (evidence / (case+".log")).write_text(done.stdout, encoding="utf-8")
        if journal.exists():
            shutil.copytree(journal, evidence / ("journal-"+case))
        report["cases"].append({"name": case, "exit_code": done.returncode,
            "seconds": time.monotonic()-started, "output": done.stdout})
        save_report()
        done.check_returncode()
        print(done.stdout.strip(), flush=True)
    print("PASS: eight ordinary-pickup component controls; inactive/noSQL/native double; no disk recovery qualification")


if __name__ == "__main__":
    main()
