#!/usr/bin/env python3
"""Actual codecs on modeled QP03 exports; no SQL/world/owner or journal access.

Run in an isolated composed candidate on D:. Maintained build cache/destination
guards, compiler/sanitizer flags and 20-second native runtime deadline apply.
The fixture reuses the maintained context test body and all its original
controls, renaming only its main to a void control function before appending
the modeled export main. A newly incompatible maintained main fails compilation.
Only the empty FIFO refusal control uses Linux tmpfs: DrvFS cannot create FIFOs.
"""

import ast
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from case_data import ROOT
from native_build_artifacts import build_native
from native_quest_pair_checks import EXTERNAL


FIXTURE = r'''
#include <fstream>

// Modeled fixture bytes, not a production continuation encoder or owner.
std::vector<uint8_t> modeled_terms(uint32_t second_reward)
{
    historical_nqr1_writer out;
    out.bytes.clear();
    for (uint32_t part : {5U, 10U, 0U, 0U, 16006U, 16077U}) out.number(part);
    out.number<uint64_t>(1700000000);
    out.number<uint32_t>(3);
    for (uint64_t uid : {100ULL, 200ULL, 300ULL}) out.number(uid);
    out.number<uint32_t>(2);
    for (uint32_t kind : {16075U, second_reward})
        for (uint32_t part : {1U, kind, 0U, 0U}) out.number(part);
    for (uint32_t part : {160U, 1U, 1U, 1U, 1U, 1U, 10U}) out.number(part);
    out.text("Modeled private name");
    out.text("fixture:QP03");
    out.number<uint32_t>(0);
    quest_reward_continuation decoded;
    assert(quest_reward_continuation_decode(out.bytes.data(), out.bytes.size(), &decoded));
    return out.bytes;
}

original modeled(bool consumption, uint32_t second_reward = 16015, int final_kind = 16013)
{
    auto value = make(consumption);
    auto &payload = value.payload;
    payload.reason_id = 16006;
    auto &ref = payload.native_mobile.reference;
    ref.mobile_vnum = 16006;
    ref.birthplace_vnum = 16077;
    ref.reset_zone_vnum = 160;
    payload.item_count = consumption ? 3 : 1;
    payload.selected_item_uid = payload.target_root_item_uid = consumption ? 0 : 300;
    std::vector<player_item_snapshot> selected;
    const std::array<int, 3> kinds{16080, 16014, final_kind};
    for (size_t i = 0; i < payload.item_count; ++i)
    {
        const uint64_t uid = consumption ? (i + 1) * 100 : 300;
        const int kind = consumption ? kinds[i] : 16013;
        payload.items[i] = {uid, uid, 0, i + 2, kind, item_custody_state::active};
        selected.push_back(item(uid, PLAYER_SNAPSHOT_NO_PARENT, 0, kind));
    }
    const auto bytes = forest(selected);
    payload.item_blob_size = bytes.size();
    std::copy(bytes.begin(), bytes.end(), payload.item_blob.begin());
    value.value.native_before = {item(500, PLAYER_SNAPSHOT_NO_PARENT, 0, 16016),
                                 item(100, PLAYER_SNAPSHOT_NO_PARENT, 0, 16080),
                                 item(200, PLAYER_SNAPSHOT_NO_PARENT, 0, 16014)};
    value.value.player_before = {item(900, PLAYER_SNAPSHOT_NO_PARENT, 0, 16016)};
    if (consumption)
    {
        value.value.native_before.push_back(item(300, PLAYER_SNAPSHOT_NO_PARENT, 0, final_kind));
        payload.continuation.kind = item_transfer_continuation_kind::quest_offering;
        payload.continuation.data = modeled_terms(second_reward);
    }
    else value.value.player_before.push_back(selected[0]);
    const std::array<uint64_t, 3> roots{100, 200, 300};
    assert(consumption ? item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, {}, roots) :
        item_transfer_native_mobile_recovery_freeze(&payload, 10, 23, forest(value.value.player_before)));
    payload.native_recovery.publication_terms.disappear = consumption;
    assert(item_transfer_command_build_native_mobile_recovery(&value.command, id(consumption ? 16 : 6),
        payload, critical_source_site::command, critical_deadline_class::interactive));
    economic_source_event source{economic_source_kind::quest_completion, id(9), id(10), 1, 0};
    assert(item_native_mobile_accounting_intent(value.command, id(7), id(8), 10,
        consumption ? &source : nullptr, &value.command.accounting_intent) == economic_accounting_error::ok);
    value.command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
    value.command.publication_required = true;
    value.command.accepted_at_usec = 1700000000000000ULL;
    assert(critical_command_envelope_valid(value.command));
    value.value = context{};
    // Restore the exact original forests after resetting unrelated fixture progress.
    value.value.native_before = {item(500, PLAYER_SNAPSHOT_NO_PARENT, 0, 16016),
        item(100, PLAYER_SNAPSHOT_NO_PARENT, 0, 16080), item(200, PLAYER_SNAPSHOT_NO_PARENT, 0, 16014)};
    value.value.player_before = {item(900, PLAYER_SNAPSHOT_NO_PARENT, 0, 16016)};
    if (consumption)
    {
        value.value.native_before.push_back(item(300, PLAYER_SNAPSHOT_NO_PARENT, 0, final_kind));
        value.value.parent_acceptance = id(6);
        value.value.consumed_root_steps.resize(3);
    }
    else value.value.player_before.push_back(selected[0]);
    return value;
}

void write_export(const std::string &directory, const char *name, const std::vector<uint8_t> &bytes)
{
    std::ofstream out(directory + "/" + name, std::ios::binary);
    out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    out.close();
    assert(out);
}

void modeled_physical_receipt(original *value)
{
    physical_receipt(value); // Maintained structural fixture, never an owner ACK.
    item_transfer_result result{};
    auto &receipt = value->value.receipt;
    assert(item_transfer_command_decode_result(receipt.result_payload.data(), receipt.result_size, &result));
    result.max_item_revision = 0;
    for (size_t i = 0; i < value->payload.item_count; ++i)
        result.max_item_revision = std::max(result.max_item_revision, value->payload.items[i].expected_item_revision + 1);
    std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> bytes{};
    assert(item_transfer_command_encode_result(result, &bytes));
    std::copy(bytes.begin(), bytes.end(), receipt.result_payload.begin());
    receipt.durable_revision = std::max({result.from_owner_revision, result.to_owner_revision, result.max_item_revision});
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    maintained_controls();
    // Heap allocate large payloads as the maintained reader does.
    auto parent = std::make_unique<original>(modeled(false));
    auto child = std::make_unique<original>(modeled(true));
    modeled_physical_receipt(parent.get());
    parent->value.give_hooks.fill(2);
    parent->value.branch_program_frozen = true;
    native_quest_recovery_branch branch;
    branch.definition_id = "fixture:QP03";
    branch.give = {{1, 16080}, {1, 16014}, {1, 16013}};
    branch.receive = {{1, 16075}, {1, 16015}};
    branch.disappear = true;
    parent->value.branches = {branch};
    parent->value.parent_acceptance = id(6);
    parent->value.child_handoff_stage = 2;
    parent->value.next_child_operation = child->command.operation_id;
    parent->value.next_child_command = command_frame(child->command);
    write_export(argv[1], "parent.command", command_frame(parent->command));
    write_export(argv[1], "parent.attachment", encode(parent->command, parent->value));
    write_export(argv[1], "child.command", command_frame(child->command));
    write_export(argv[1], "held.attachment", encode(child->command, child->value));
    modeled_physical_receipt(child.get());
    child->value.consumed_root_steps.assign(3, 2);
    write_export(argv[1], "child.attachment", encode(child->command, child->value));
    // Maintained codecs accept these internally valid alternatives; the reader
    // must refuse the wrong recipe reward/kind even with exact child linkage.
    for (const auto &[label, reward, ingredient] : {
        std::tuple{"wrong-reward", 16048U, 16013}, std::tuple{"wrong-ingredient", 16015U, 16014}})
    {
        auto alternative = std::make_unique<original>(modeled(true, reward, ingredient));
        modeled_physical_receipt(alternative.get());
        alternative->value.consumed_root_steps.assign(3, 2);
        auto original_parent = parent->value;
        original_parent.next_child_command = command_frame(alternative->command);
        write_export(argv[1], (std::string(label) + ".parent").c_str(), encode(parent->command, original_parent));
        write_export(argv[1], (std::string(label) + ".command").c_str(), command_frame(alternative->command));
        write_export(argv[1], (std::string(label) + ".attachment").c_str(), encode(alternative->command, alternative->value));
    }
    std::puts("Maintained controls plus modeled QP03 exports passed; no owner execution.");
}
'''


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def maintained_configuration():
    path = ROOT / "tests/async/test_native_quest_recovery_context.py"
    values = {}
    for statement in ast.parse(path.read_text()).body:
        if isinstance(statement, ast.Assign) and len(statement.targets) == 1:
            target = statement.targets[0]
            if isinstance(target, ast.Name) and target.id in {"SOURCES", "FLAGS", "LINK", "RUNTIME_SECONDS"}:
                values[target.id] = ast.literal_eval(statement.value)
    if set(values) != {"SOURCES", "FLAGS", "LINK", "RUNTIME_SECONDS"} or values["RUNTIME_SECONDS"] != 20:
        raise ValueError("maintained fixture controls changed; review required")
    return values


def main():
    config = maintained_configuration()
    parent = ROOT / "bin/tests"
    parent.mkdir(parents=True, exist_ok=True)
    if parent.is_symlink() or not parent.resolve().is_relative_to(ROOT.resolve()):
        raise ValueError("artifact parent must be owned below candidate/bin/tests")
    artifacts = Path(tempfile.mkdtemp(prefix="qp03-pair-reader-", dir=parent))
    receipt = dict(classification="actual codecs and maintained pure component controls on modeled exports",
                   owner_execution=False, journal_access=False, SQL_execution=False, artifacts_retained=True,
                   runtime_timeout_seconds=config["RUNTIME_SECONDS"], status="prepared", checks=[])
    try:
        maintained = ROOT / config["SOURCES"][0]
        prefix, separator, main_body = maintained.read_text().partition("\nint main()\n")
        if not separator or maintained.read_text().count("\nint main()\n") != 1:
            raise ValueError("maintained main seam changed; review required")
        generated = artifacts / "modeled_exports.cpp"
        generated.write_text(prefix + "\nvoid maintained_controls()\n" + main_body + "\n#include <memory>\n#include <tuple>\n" + FIXTURE)
        sources = config["SOURCES"][1:]
        # These are canonical providers referenced by the maintained command decoder.
        sources = sources + ["src/economy/native_quest_cost.c", "src/economy/native_quest_coin_give.c",
                             "src/item/lockpick_retirement_continuation.c"]
        reader_source = "tests/async/quest_accounting_prep/read_native_quest_pair.cpp"
        pins = {name: sha(ROOT / name) for name in [*sources, config["SOURCES"][0], reader_source,
                "tests/async/test_native_quest_recovery_context.py", "tests/async/native_build_artifacts.py",
                "tests/async/server_build_artifacts.py", "tests/async/quest_accounting_prep/case_data.py",
                "tests/async/quest_accounting_prep/test_read_native_quest_pair.py",
                "tests/async/quest_accounting_prep/native_quest_pair_checks.py",
                "tests/async/quest_accounting_prep/quest_cut_checks.py",
                "docs/persistence/economy_accounting/registry.json"]}
        receipt.update(source_sha256=pins, generated_fixture_sha256=sha(generated),
                       flags=config["FLAGS"], link=config["LINK"], compiler=os.environ.get("CXX", "g++"),
                       generator_sources=[str(generated), *sources], reader_sources=[reader_source, *sources],
                       compile_command_timeout_seconds=600)
        receipt["compiler_version"] = subprocess.check_output(
            [*shlex.split(receipt["compiler"]), "--version"], cwd=ROOT, timeout=20, text=True).splitlines()[0]
        generator = build_native(artifacts / "generator", [str(generated), *sources], config["FLAGS"],
                                 config["LINK"], compiler=receipt["compiler"], name="qp03-modeled-pair-export")
        # The allocator wrapper belongs only to the original fixture controls.
        reader_link = [flag for flag in config["LINK"] if flag != "-Wl,--wrap=_Znwm"]
        reader = build_native(artifacts / "reader", [reader_source, *sources], config["FLAGS"], reader_link,
                              compiler=receipt["compiler"], name="qp03-original-pair-reader")
        receipt.update(generator_sha256=sha(generator), reader_sha256=sha(reader), reader_link=reader_link,
                       generator_path=str(generator), reader_path=str(reader))
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                   UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
        receipt["sanitizer_runtime"] = {name: env[name] for name in ("ASAN_OPTIONS", "UBSAN_OPTIONS")}
        completed = subprocess.run([str(generator), str(artifacts)], cwd=ROOT, env=env,
                                   capture_output=True, timeout=config["RUNTIME_SECONDS"])
        (artifacts / "generator.log").write_bytes(completed.stdout + completed.stderr)
        if completed.returncode:
            raise RuntimeError("modeled export/component controls failed; see generator.log")
        exports = {name: sha(artifacts / name) for name in
                   ("parent.command", "parent.attachment", "child.command", "child.attachment", "held.attachment",
                    "wrong-reward.parent", "wrong-reward.command", "wrong-reward.attachment",
                    "wrong-ingredient.parent", "wrong-ingredient.command", "wrong-ingredient.attachment")}
        for name in exports:
            (artifacts / name).chmod(0o444)
        base = [str(artifacts / "parent.command"), str(artifacts / "parent.attachment"), "11", "2",
                str(artifacts / "child.command"), str(artifacts / "child.attachment"), "7", "2"]

        def run(label, args, accepted=False, phase2=True):
            completed = subprocess.run([str(reader), *args], cwd=ROOT, env=env,
                                       capture_output=True, timeout=config["RUNTIME_SECONDS"])
            if accepted:
                if completed.returncode or completed.stderr or len(completed.stdout) >= 64 * 1024:
                    raise AssertionError(label + ": expected bounded clean summary: " + completed.stderr.decode(errors="replace"))
                value = json.loads(completed.stdout)
                if value["external_proof_required"] != EXTERNAL:
                    raise AssertionError(label + ": missing external proof requirements")
                if value["pair_checked"] is not phase2 or value["pair_valid"] is not phase2:
                    raise AssertionError(label + ": wrong structural validator phase")
                if any(private in completed.stdout for private in (b"Modeled private name", b"synthetic context item", b"fixture:QP03")):
                    raise AssertionError("raw private fixture strings escaped")
                (artifacts / (label + ".json")).write_bytes(completed.stdout)
            elif (completed.returncode != 2 or completed.stdout or completed.stderr !=
                  b"Original QP03 pair values refused; no owner authority inferred.\n"):
                raise AssertionError(label + ": expected strong refusal: " + completed.stderr.decode(errors="replace"))
            receipt["checks"].append(dict(name=label, accepted=accepted, exit_code=completed.returncode))

        run("phase2", base, True)
        held = base.copy()
        held[5], held[7] = str(artifacts / "held.attachment"), "1"
        run("phase1-held-no-pair-validator", held, True, False)
        carrier = base.copy()
        carrier[7] = "1"
        run("phase1-proven-carrier-no-authority", carrier, True, False)
        wrong_phase = held.copy()
        wrong_phase[7] = "2"
        run("phase2-cannot-borrow-held-attachment", wrong_phase)
        for label in ("wrong-reward", "wrong-ingredient"):
            args = base.copy()
            args[1], args[4], args[5] = (str(artifacts / (label + suffix)) for suffix in (".parent", ".command", ".attachment"))
            run(label + "-valid-codec-exact-link", args)
        for position, values in ((2, ("0", "01", "-1", "18446744073709551616")), (7, ("0", "3", "02"))):
            for value in values:
                args = base.copy()
                args[position] = value
                run(f"numeric-{position}-{value}", args)
        for name, position in (("parent.command", 0), ("parent.attachment", 1), ("child.command", 4), ("child.attachment", 5)):
            original = (artifacts / name).read_bytes()
            for suffix, data in (("trailing", original + b"x"), ("truncated", original[:-1]), ("empty", b"")):
                path = artifacts / (name + "." + suffix)
                path.write_bytes(data)
                path.chmod(0o444)
                args = base.copy()
                args[position] = str(path)
                run(name + "-" + suffix, args)
        link = artifacts / "symlink"
        link.symlink_to(artifacts / "parent.command")
        with tempfile.TemporaryDirectory(prefix="qp03-pair-fifo-", dir="/dev/shm") as special:
            fifo = Path(special) / "fifo"
            os.mkfifo(fifo, 0o444)
            args = base.copy()
            args[0] = str(fifo)
            run("fifo", args)
        receipt["fifo_control_storage"] = "empty Linux tmpfs FIFO; DrvFS does not support mkfifo"
        oversized = artifacts / "oversized"
        with oversized.open("wb") as handle:
            handle.truncate(32 * 1024 * 1024 + 1)
        oversized.chmod(0o444)
        writable = artifacts / "writable"
        writable.write_bytes((artifacts / "parent.command").read_bytes())
        writable.chmod(0o644)
        for label, path, position in (("symlink", link, 0), ("directory", artifacts, 0),
                                      ("command-bound", oversized, 0), ("attachment-bound", oversized, 1),
                                      ("writable-export", writable, 0)):
            args = base.copy()
            args[position] = str(path)
            run(label, args)
        args = base.copy()
        args[0] = base[4]
        run("swapped-original-command", args)
        run("missing-arguments", base[:-1])
        if exports != {name: sha(artifacts / name) for name in exports}:
            raise AssertionError("original exported files changed")
        receipt.update(status="passed_component_only", exports_sha256=exports)
    except Exception as error:
        receipt.update(status="failed", failure_type=type(error).__name__, failure=str(error))
        raise
    finally:
        if "source_sha256" in receipt:
            receipt["source_unchanged"] = all(sha(ROOT / name) == pin for name, pin in receipt["source_sha256"].items())
        (artifacts / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
        print("QP03 pair reader component artifacts:", artifacts)
    if not receipt["source_unchanged"]:
        raise RuntimeError("linked source changed during qualification")


if __name__ == "__main__":
    main()
