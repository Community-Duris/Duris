#!/usr/bin/env python3
"""Observe the actual flat provider during the unchanged Kord crash/drop journey.

GDB reads provider arguments/catalog only. It never assigns inferior variables,
calls inferior functions or changes a result. The original journey stays RED.
Private observations are diagnostic evidence, not a new recovery authority.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import time
from unittest.mock import patch

from case_data import ROOT, digest
import run_quest_reward_ack_crash as driver
from run_quest_execution import build_quest_inspector


def execute(args):
    if not re.fullmatch(r"[0-9a-f]{40}", args.source_commit):
        raise ValueError("actual binary source commit required")
    binary = args.server.resolve(strict=True)
    if digest(binary) != args.server_sha256:
        raise ValueError("actual retained binary hash differs")
    args.evidence_dir.mkdir(parents=True, mode=0o700)
    provider = ROOT / "src/flatfile/flatfile_item_repository.c"
    source = provider.read_text()
    predicate = "if (from_owner->revision != payload.expected_from_revision"
    line = source[:source.index(predicate)].count("\n") + 1
    admission = ROOT / "src/item/item_movement_transaction.c"
    admission_source = admission.read_text()
    admission_line = admission_source[:admission_source.index(
        "if (live_drop_token)\n\t{\n\t\tif (!item_ownership_runtime_peek_owner_revision"
    )].count("\n") + 1
    initial_probe = args.evidence_dir / "initial-room-observe.gdb"
    initial_probe.write_text('''python
import gdb,json
class Room(gdb.Breakpoint):
 def stop(self):
  p=gdb.parse_and_eval("payload")
  f=p["from_owner"];t=p["to_owner"]
  if not any(int(o["type"]) == 3 and int(o["id"]) == 22800 for o in (f,t)):
   return False
  result=dict(reason=int(p["reason"]),
   from_owner=[int(f[k]) for k in ("type","id","context_id")],
   to_owner=[int(t[k]) for k in ("type","id","context_id")],
   expected_from=int(p["expected_from_revision"]),expected_to=int(p["expected_to_revision"]),
   observed_from=int(gdb.parse_and_eval("from_owner").dereference()["revision"]),
   observed_to=int(gdb.parse_and_eval("to_owner").dereference()["revision"]))
  result["items"]=[]
  for i in range(int(p["item_count"])):
   e=p["items"]["_M_elems"][i]
   result["items"].append({k:int(e[k]) for k in ("item_uid","vnum")})
  gdb.write("QUEST_ROOM_PREREQUISITE "+json.dumps(result)+"\\n")
  return False
Room("flatfile/flatfile_item_repository.c:''' + str(line) + '''").silent=True
end
''', encoding="utf-8")
    probe = args.evidence_dir / "observe.gdb"
    probe.write_text('''set pagination off
set confirm off
python
import gdb,json
class Admission(gdb.Breakpoint):
 def stop(self):
  r=gdb.parse_and_eval("runtime")
  if int(r["vnum"]) != 29237: return False
  result=dict(live_drop_token=bool(gdb.parse_and_eval("live_drop_token")),
   accounting_active=bool(gdb.parse_and_eval("accounting_active")))
  gdb.write("QUEST_DROP_ADMISSION "+json.dumps(result)+"\\n")
  return False
Admission("item/item_movement_transaction.c:''' + str(admission_line) + '''").silent=True
class Drop(gdb.Breakpoint):
 def stop(self):
  p=gdb.parse_and_eval("payload")
  entry=p["items"]["_M_elems"][0]
  if int(p["item_count"]) != 1 or int(entry["vnum"]) != 29237:
   return False
  if int(p["reason"]) != int(gdb.parse_and_eval("item_transfer_reason::player_drop")):
   return False
  f=gdb.parse_and_eval("from_owner").dereference()
  t=gdb.parse_and_eval("to_owner").dereference()
  result=dict(expected_from=int(p["expected_from_revision"]),observed_from=int(f["revision"]),
              expected_to=int(p["expected_to_revision"]),observed_to=int(t["revision"]))
  for key in ("from_owner","to_owner"):
   result[key]=[int(p[key][k]) for k in ("type","id","context_id")]
  result["expected_item"]={k:int(entry[k]) for k in
   ("item_uid","root_item_uid","parent_item_uid","expected_item_revision","vnum","expected_state")}
  vector=gdb.parse_and_eval("catalog->items")
  start=vector["_M_impl"]["_M_start"]
  finish=vector["_M_impl"]["_M_finish"]
  result["observed_items"]=[]
  while start != finish:
   item=start.dereference()
   if int(item["item_uid"]) == int(entry["item_uid"]):
    result["observed_items"].append({k:int(item[k]) for k in
      ("item_uid","root_item_uid","parent_item_uid","item_revision","vnum","state")})
   start=start+1
  result["stack"]=[]
  frame=gdb.selected_frame()
  for index in range(6):
   if frame is None: break
   result["stack"].append(frame.name());frame=frame.older()
  gdb.write("QUEST_DROP_OBSERVATION "+json.dumps(result)+"\\n")
  return True
Drop("flatfile/flatfile_item_repository.c:''' + str(line) + '''")
gdb.write("QUEST_DROP_READY\\n")
end
continue
detach
quit
''', encoding="utf-8")
    observation_log = args.evidence_dir / "provider-observation.log"
    processes = []
    handles = []
    cuts = {}

    original_fault_boot = driver.fault_boot
    original_popen = subprocess.Popen

    def fault_boot_with_room_observer(*boot_args, **boot_kwargs):
        # Keep the maintained fault breakpoint/run/kill and every original flag.
        # Add a read-only, non-stopping observer to its existing GDB process.
        def launch(command, *positional, **keywords):
            command = list(command)
            if command[0] != "gdb" or ["-ex", "run"] != command[
                    command.index("run") - 1:command.index("run") + 1]:
                raise RuntimeError("maintained fault boot seam changed")
            index = command.index("run") - 1
            command[index:index] = ["-ex", "source " + str(initial_probe)]
            return original_popen(command, *positional, **keywords)
        with patch.object(subprocess, "Popen", launch):
            return original_fault_boot(*boot_args, **boot_kwargs)

    def observe(label, *, run_root, **unused):
        if label in ("before-offering", "at-crash", "recovered"):
            state = driver.journey.inspect_authority(unused["state_root"])
            cuts[label] = {k: state[k] for k in
                           ("player_owner_revision", "room_owner_revision", "room_items")}
        if label != "recovered":
            return
        candidates = []
        for directory in Path("/proc").iterdir():
            if not directory.name.isdecimal():
                continue
            try:
                if ((directory / "exe").resolve() == binary and
                        (directory / "cwd").resolve() == run_root.resolve()):
                    candidates.append(int(directory.name))
            except (FileNotFoundError, PermissionError):
                pass
        if len(candidates) != 1:
            raise RuntimeError("one actual isolated recovered server required")
        output = observation_log.open("w")
        handles.append(output)
        process = subprocess.Popen(["gdb", "--batch", "--quiet", "-p", str(candidates[0]),
                                    "-x", str(probe)], stdout=output, stderr=subprocess.STDOUT)
        processes.append(process)
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            if "QUEST_DROP_READY" in observation_log.read_text(errors="replace"):
                return
            if process.poll() is not None:
                break
            time.sleep(0.05)
        raise RuntimeError("read-only provider observer failed to attach")

    summary = dict(binary_source_commit=args.source_commit, binary_sha256=digest(binary),
        provider_sha256=digest(provider), observer_sha256=digest(Path(__file__)),
        admission_sha256=digest(admission),
        schema_manifest_sha256=digest(ROOT / "migrations/runtime_compatibility_manifest.json"),
        backend="flatfile", authority="actual-provider read-only diagnostic; legacy calibration")
    started = time.monotonic()
    try:
        driver.journey.INSPECTOR = build_quest_inspector()
        try:
            with patch.object(driver, "fault_boot", fault_boot_with_room_observer):
                driver.run(binary, True, fault_phase="xp-ack", quest_case="QP06", move_reward=True,
                           observer=observe, evidence_dir=args.evidence_dir)
        except AssertionError as error:
            summary["original_journey"] = "RED"
            summary["original_error"] = str(error)
            if "stale_authority_revision" not in str(error) or "You drop" not in str(error):
                raise AssertionError("original drop failure did not reproduce") from error
        else:
            raise AssertionError("original RED drop unexpectedly passed; reclassify the diagnostic")
        for process in processes:
            process.wait(timeout=10)
            if process.returncode != 0:
                raise AssertionError("GDB observation failed")
        observations = [json.loads(line.removeprefix("QUEST_DROP_OBSERVATION "))
                        for line in observation_log.read_text().splitlines()
                        if line.startswith("QUEST_DROP_OBSERVATION ")]
        if len(observations) != 1:
            raise AssertionError("one genuine provider observation required")
        summary["provider"] = observations[0]
        actual = observations[0]
        if not (actual["expected_from"] == actual["observed_from"] and
                actual["expected_to"] != actual["observed_to"] and
                actual["to_owner"] == [3, 22800, 0]):
            raise AssertionError("destination-only revision refusal not established")
        summary["admission"] = [json.loads(line.removeprefix("QUEST_DROP_ADMISSION "))
                                for line in observation_log.read_text().splitlines()
                                if line.startswith("QUEST_DROP_ADMISSION ")]
        if summary["admission"] != [dict(live_drop_token=False, accounting_active=False)]:
            raise AssertionError("one actual legacy admission required")
        summary["room_prerequisites"] = [json.loads(line.removeprefix("QUEST_ROOM_PREREQUISITE "))
            for line in (args.evidence_dir / "run/ack-crash.out").read_text().splitlines()
            if line.startswith("QUEST_ROOM_PREREQUISITE ")]
        if not summary["room_prerequisites"] or not all(
                row["expected_from"] == row["observed_from"] and
                row["expected_to"] == row["observed_to"]
                for row in summary["room_prerequisites"]):
            raise AssertionError("original durable room prerequisites not observed")
        summary["cuts"] = cuts
        summary["result"] = "PASS: observed original RED journey; no result/counter/journal mutation"
    except Exception as error:
        summary.update(result="FAIL: diagnostic incomplete", diagnostic_error=str(error))
        raise
    finally:
        for process in processes:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=10)
        for output in handles:
            output.close()
        summary["elapsed_seconds"] = round(time.monotonic() - started, 3)
        summary["cuts"] = cuts
        (args.evidence_dir / "diagnostic.json").write_text(json.dumps(summary, indent=2)+"\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in summary.items() if k != "original_error"}, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", type=Path, required=True)
    parser.add_argument("--server-sha256", required=True)
    parser.add_argument("--source-commit", required=True)
    parser.add_argument("--evidence-dir", type=Path, required=True)
    execute(parser.parse_args())
