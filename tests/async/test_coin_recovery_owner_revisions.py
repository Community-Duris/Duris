#!/usr/bin/env python3
"""Regress locked owner-counter projection in the actual ordinary coin recovery code.

Extract current project/helpers/type layouts and actual runtime-cache primitives.
Native SQL/flat authority acquisition, receipt validation, physical census/literal
capture, body projection and allocation/enrollment are explicit test seams. Cases
supply already-proven native state to the projection boundary; they do not qualify
backend proof, ACK, cold boot loading, gameplay or the complete Plan 2 routes.
"""
from pathlib import Path
import argparse
import hashlib
import json
import os
import re
import shlex
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
SOURCES = ("src/economy/coin_physical_recovery.c",
           "src/item/item_ownership_runtime.c", "src/item/item_transfer_command.c")
CASES = ("consumed_cold_cache", "drop_existing_stale_counters",
         "rejected_drop_stale_counters", "partial_pickup_same_owner",
         "consumed_conflicting_owner", "consumed_uncertain_effect",
         "consumed_lost_authority", "drop_conflicting_owner",
         "drop_conflicting_literal", "rejected_drop_conflicting_owner")
COMPILE_SECONDS = 300
CASE_SECONDS = 30


def definition(source, pattern, *, semicolon=False):
    """Copy the actual declaration/body while ignoring quoted/comment braces."""
    quoted = re.compile(r"""//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'""", re.S)
    masked = quoted.sub(lambda match: re.sub(r"[^\n]", " ", match.group()), source)
    matches = list(re.finditer(pattern, masked, re.M))
    if len(matches) != 1:
        raise RuntimeError(f"current source definition changed: {pattern}")
    match = matches[0]
    opening = masked.index("{", match.start())
    depth, end = 1, opening + 1
    while depth:
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    if semicolon:
        while source[end].isspace():
            end += 1
        if source[end] != ";":
            raise RuntimeError("current struct terminator changed")
        end += 1
    return source[match.start():end]


def function(source, name):
    return definition(source, rf"^(?:static )?(?:bool|size_t) {re.escape(name)}\(")


def structure(source, name):
    return definition(source, rf"^struct {re.escape(name)}\b", semicolon=True)


def build_harness():
    recovery, runtime, identity = [(ROOT / name).read_text(encoding="utf-8")
                                   for name in SOURCES]
    # Only the existing private runtime maps/hash declarations, never saved bodies.
    cache_start = runtime.index("namespace\n{")
    cache_end = runtime.index("\n}\n", cache_start) + 3
    declarations = [structure(recovery, name)
                    for name in ("shape", "native_state", "publication", "projection_cut", "physical")]
    constants = []
    for pattern in (r"^constexpr size_t census_limit = [^;]+;",
                    r"^using digest = [^;]+;"):
        matches = re.findall(pattern, recovery, re.M)
        if len(matches) != 1:
            raise RuntimeError("current recovery constant/type alias changed")
        constants.append(matches[0])
    primitives = [function(identity, name)
                  for name in ("item_owner_identity_valid", "item_owner_identity_equal")]
    primitives += [function(runtime, name) for name in
                   ("snapshot_root", "item_ownership_runtime_snapshot_root",
                    "item_ownership_runtime_hydrate_many_atomic", "item_ownership_runtime_hydrate_owner",
                    "item_ownership_runtime_lookup", "item_ownership_runtime_peek_owner_revision",
                    "item_ownership_runtime_size")]
    projection = [function(recovery, "runtime_matches"),
                  function(recovery, "exact_runtime_identity")]
    if re.search(r"^bool project_owner_revisions\(", recovery, re.M):
        projection.append(function(recovery, "project_owner_revisions"))
    projection.append(function(recovery, "project"))
    return (HEAD + runtime[cache_start:cache_end] + "\n" + "\n".join(primitives) + "\n" +
            "\n".join(constants + declarations) + SEAMS + "\n" + "\n".join(projection) + SCENARIOS)


def bounded(argv, seconds):
    started = time.monotonic()
    process = subprocess.Popen(argv, cwd=ROOT, text=True, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, start_new_session=True)
    try:
        output, _ = process.communicate(timeout=seconds)
    except BaseException:
        os.killpg(process.pid, signal.SIGTERM)
        try:
            process.communicate(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.communicate()
        raise
    return process.returncode, output, round(time.monotonic() - started, 3)


def pins():
    selected = [ROOT / name for name in SOURCES]
    selected += sorted((ROOT / "src").rglob("*.h"))
    selected.append(Path(__file__).resolve())
    return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in selected if path.is_file()}


def run(directory):
    initial = pins()
    harness = build_harness()
    cpp, binary = directory / "coin_recovery_owner_revisions.cpp", directory / "coin_recovery_owner_revisions"
    cpp.write_text(harness, encoding="utf-8")
    argv = [*shlex.split(os.environ.get("CXX", "g++")), "-std=c++20", "-Wall", "-Wextra",
            "-Wpedantic", "-Werror", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-fno-pie", "-no-pie", "-Isrc", str(cpp), "-o", str(binary)]
    rc, output, elapsed = bounded(argv, COMPILE_SECONDS)
    report = {"scope": __doc__, "source_pins": initial, "compiler_argv": argv,
              "compile_exit": rc, "compile_seconds": elapsed, "compile_output": output,
              "compile_budget_seconds": COMPILE_SECONDS, "case_budget_seconds": CASE_SECONDS,
              "harness_sha256": hashlib.sha256(harness.encode()).hexdigest(), "cases": []}
    receipt = directory / "coin_recovery_owner_revisions.json"
    receipt.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    if rc:
        raise RuntimeError(f"strict owner-revision harness compile failed ({rc}):\n{output}")
    report["binary_sha256"] = hashlib.sha256(binary.read_bytes()).hexdigest()
    failures = []
    for case in CASES:
        if pins() != initial:
            raise RuntimeError("current source/header/test inputs changed during regression")
        rc, output, elapsed = bounded([str(binary), case], CASE_SECONDS)
        report["cases"].append({"case": case, "exit": rc, "seconds": elapsed, "output": output})
        receipt.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
        print(f"case={case} exit={rc}\n{output}", flush=True)
        if rc:
            failures.append(case)
    if pins() != initial:
        raise RuntimeError("current source/header/test inputs changed during regression")
    report["source_pins_unchanged"] = True
    receipt.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    if failures:
        raise AssertionError(f"coin recovery owner-revision failures: {failures}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, help="retain the actual harness, ELF and result receipt")
    args = parser.parse_args()
    if args.artifacts:
        directory = args.artifacts.resolve()
        directory.mkdir(parents=True, exist_ok=False)
        run(directory)
    else:
        with tempfile.TemporaryDirectory(prefix="coin-recovery-owner-revisions-") as temporary:
            run(Path(temporary))

HEAD = r"""#include "core/structs.h"
#include "economy/coin_transfer_command.h"
#include "persistence/critical_command_completion.h"
#include "player/player_snapshot.h"
#include "item/item_ownership_runtime.h"
#include <mysql/mysql.h>
#include <openssl/sha.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>
"""

SEAMS = r"""
static bool authority_current=true;
bool current_cut(const projection_cut &) {return authority_current;}
static P_obj observed=nullptr;
static bool literal_matches=true;
static int native_effects=0, enrollment_attempts=0, projections=0;
bool census(const shape &,physical &out){out.object=observed;return true;}
bool capture_equal(P_obj,const player_item_snapshot &){return literal_matches;}
bool opening_matches(player_item_snapshot,const shape &){return false;}
bool body_matches(P_char,const shape &,const native_state &,bool){return true;}
void project_bodies(const physical &,const shape &,const native_state &){++projections;}
struct inert_item_stage {};
enum class inert_item_stage_result {ok, invalid};
inert_item_stage_result prepare_inert_money_stage(const player_item_snapshot &,uint64_t,
    const std::array<int32_t,4>&,inert_item_stage &){++enrollment_attempts;return inert_item_stage_result::invalid;}
class coin_physical_recovery_owner {public: static bool enroll(const shape &,const native_state &,
    inert_item_stage &){++enrollment_attempts;return false;}};
int real_room(int){return 0;}
P_index obj_index=nullptr;
int top_of_objt=0;
bool coin_physical_publication_room_safe(int,P_obj){return true;}
player_snapshot_capture_result player_item_snapshot_tree_capture_literal(P_obj,
    std::vector<player_item_snapshot> *,size_t *){return player_snapshot_capture_result::invalid_identity;}
void extract_obj(P_obj,int){++native_effects;}
void add_coins(P_obj,int,int,int,int){++native_effects;}
"""

SCENARIOS = r"""
static int failures=0;
void check(bool ok,const char *label){if(!ok){++failures;std::cerr<<label<<" FAIL\n";}}
void counter(item_owner_identity owner,uint64_t expected,const char *label){
    uint64_t actual=UINT64_MAX;
    check(item_ownership_runtime_peek_owner_revision(owner,&actual)&&actual==expected,label);
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    const std::string scenario=argv[1];
    shape value; value.pile=std::make_unique<item_transfer_payload>();
    const item_owner_identity room={item_owner_type::room,101,0};
    const item_owner_identity system={item_owner_type::system,0,0};
    const item_owner_identity destroyed={item_owner_type::destruction,0,0};
    value.room=101;value.pile->selected_item_uid=9001;
    value.pile->items[0].vnum=3;value.pile->items[0].expected_item_revision=7;
    value.pile->expected_to_revision=2;
    native_state native;native.item_exists=true;native.committed=true;
    native.item={9001,9001,0,destroyed,8,5,3,item_custody_state::destroyed};
    native.from_revision=12;native.to_revision=5;
    value.pile->from_owner=room;value.pile->to_owner=destroyed;value.consumed=true;
    critical_completion receipt={};receipt.outcome=critical_apply_outcome::applied;
    publication stage;projection_cut cut={nullptr,0};obj_data object={};
    if(scenario=="drop_existing_stale_counters" || scenario=="drop_conflicting_owner" ||
       scenario=="drop_conflicting_literal"){
        value.drop=true;value.consumed=false;value.pile->from_owner=system;value.pile->to_owner=room;
        native.item={9001,9001,0,room,1,12,3,item_custody_state::active};
        native.from_revision=7;native.to_revision=12;native.literal=player_item_snapshot{};
        auto old=native.item;old.owner_revision=3;
        check(item_ownership_runtime_hydrate_many_atomic(&old,1),"setup");observed=&object;
        if(scenario=="drop_conflicting_owner")check(item_ownership_runtime_hydrate_owner(system,8),"setup conflict");
        if(scenario=="drop_conflicting_literal")literal_matches=false;
    }else if(scenario=="rejected_drop_stale_counters" || scenario=="rejected_drop_conflicting_owner"){
        value.drop=true;value.pile->from_owner=system;value.pile->to_owner=room;
        native.item_exists=false;native.committed=false;native.from_revision=0;native.to_revision=14;
        receipt.outcome=critical_apply_outcome::terminal_failure;
        check(item_ownership_runtime_hydrate_owner(room,
            scenario=="rejected_drop_conflicting_owner"?15:7),"setup rejected counter");
    }else if(scenario=="partial_pickup_same_owner"){
        value.consumed=false;value.pile->to_owner=room;native.to_revision=native.from_revision;
        native.item={9001,9001,0,room,8,12,3,item_custody_state::active};
        native.literal=player_item_snapshot{};observed=&object;
    }else if(scenario=="consumed_conflicting_owner"){
        check(item_ownership_runtime_hydrate_owner(room,13),"setup source conflict");
    }else if(scenario=="consumed_uncertain_effect"){
        stage.native_started=true;
    }else if(scenario=="consumed_lost_authority"){
        authority_current=false;
    }else if(scenario!="consumed_cold_cache")return 2;
    const auto before_entries=entries;
    const auto before_owners=owner_revisions;
    const bool refusal=scenario.find("conflicting")!=std::string::npos ||
        scenario=="consumed_uncertain_effect" || scenario=="consumed_lost_authority";
    const bool published=project(cut,value,native,receipt,stage);
    check(published!=refusal,"correct publication result");
    check(native_effects==0 && enrollment_attempts==0,"no renderer, extraction or enrollment");
    if(refusal){
        check(projections==0,"refusal has no body projection");
        check(entries.size()==before_entries.size() && owner_revisions.size()==before_owners.size(),"refusal leaves cache sizes unchanged");
        for(const auto &[owner,revision]:before_owners){auto found=owner_revisions.find(owner);
            check(found!=owner_revisions.end() && found->second==revision,"refusal leaves cache counters unchanged");}
        for(const auto &[uid,entry]:before_entries){
            auto found=entries.find(uid);check(found!=entries.end()&&
                found->second.item_revision==entry.item_revision&&
                found->second.owner_revision==entry.owner_revision,"refusal preserves item revision");
        }
    }else{
        counter(value.pile->from_owner,native.from_revision,"locked source counter restored");
        counter(value.pile->to_owner,native.to_revision,"locked destination counter restored");
        check(projections==1,"single body-projection pass");
        if(native.item_exists){item_ownership_runtime_entry current={};
            check(item_ownership_runtime_lookup(9001,&current)&&
                current.owner_revision==native.item.owner_revision,"exact locked item owner revision restored");}
    }
    std::cout<<scenario<<" failures="<<failures<<"\n";
    return failures?1:0;
}
"""

if __name__ == "__main__":
    main()
