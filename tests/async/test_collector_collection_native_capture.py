#!/usr/bin/env python3
"""Actual-provider Collector collection capture acceptance; no durable/publication claim.

Run on Linux/WSL with C++20, OpenSSL, ASan and UBSan. --source-root selects an
exact candidate without changing a checkout. --build-dir or BIN_ROOT selects
fresh retained artifacts; TMPDIR must select task-owned compiler scratch.
All providers are complete translation units; linker section collection excludes
uncalled detach and unrelated server functions. The sibling C++ harness supplies
only synthetic native inputs and thread/fail-stop diagnostics.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
SOURCES = ('src/economy/collector_collection_preparation.c',
 'src/economy/collector_policy.c',
 'src/economy/collector_codec.c',
 'src/economy/collector_command.c',
 'src/player/player_snapshot_capture.c',
 'src/player/player_snapshot_codec.c',
 'src/item/item_ownership_runtime.c',
 'src/item/item_transfer_command.c',
 'src/economy/currency_command.c',
 'src/persistence/critical_command.c',
 'src/world/handler.c',
 'src/core/utility.c')

CASES = ('room_nested',
 'room_root',
 'corpse_nested',
 'corpse_root',
 'new_owner',
 'ordinary_mask',
 'newer_item',
 'root_limit',
 'root_limit_plus_one',
 'blob_budget',
 'string_budget',
 'depth_budget',
 'negative_child',
 'negative_selected',
 'weight_sum',
 'zero_weight',
 'max_weight',
 'native_duplicate_nonselected',
 'command_duplicate',
 'command_parent',
 'command_cycle',
 'command_owner_max',
 'pre_take',
 'pre_money',
 'pre_corpse',
 'pre_artifact',
 'pre_transient',
 'pre_norent',
 'pre_nosell',
 'pre_bound',
 'pre_unique',
 'pre_powerunique',
 'pre_rnum',
 'pre_duplicate_selected',
 'pre_claimed_player',
 'pre_claimed_shop',
 'pre_claimed_pet',
 'pre_destroyed',
 'pre_quarantined',
 'pre_revision_max',
 'pre_revision_old',
 'pre_missing_registry',
 'sibling_revision',
 'ancestor_revision',
 'descendant_revision',
 'row_owner_revision',
 'row_owner',
 'row_root',
 'row_parent',
 'row_state',
 'row_vnum',
 'row_revision_zero',
 'owner_clock_only',
 'extra_child',
 'missing_child',
 'reciprocal',
 'ancestor_reciprocal',
 'sibling_cycle',
 'global_cycle',
 'duplicate_selected',
 'room_vnum',
 'room_reciprocal',
 'corpse_pid',
 'corpse_saveid',
 'corpse_flag',
 'corpse_reciprocal',
 'literal_cost',
 'literal_condition',
 'literal_fixed_affect',
 'literal_dynamic_affect',
 'literal_extension',
 'literal_string',
 'literal_weight',
 'literal_generated_key',
 'literal_value',
 'literal_timer',
 'literal_flags',
 'literal_mask',
 'affect_cycle',
 'description_cycle',
 'live_eligibility_omitted',
 'not_due')


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_logged(command, log_path, timeout, **kwargs):
    """Retain diagnostics, including partial output when a child times out."""
    with log_path.open("w") as log:
        try:
            return subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                  timeout=timeout, **kwargs)
        except subprocess.TimeoutExpired:
            log.write("\nTIMEOUT after " + str(timeout) + " seconds\n")
            return subprocess.CompletedProcess(command, 124)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--optimization", choices=("O1", "Og"), default="O1")
    args = parser.parse_args()
    root = args.source_root.resolve()
    output = (args.build_dir or Path(os.environ.get("BIN_ROOT", ROOT / "bin")) /
              "tests" / "collector-collection-native-capture" /
              (args.optimization + "-" + str(time.time_ns()))).resolve()
    output.mkdir(parents=True, exist_ok=False)
    harness = output / "driver.cpp"
    harness_source = Path(__file__).with_name("collector_collection_native_capture_harness.cpp")
    harness.write_bytes(harness_source.read_bytes())
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    result = run_logged(compiler + ["--version"], output / "compiler-version.txt", 20)
    assert result.returncode == 0, str(output / "compiler-version.txt")
    compiler_path = Path(shutil.which(compiler[0])).resolve()
    (output / "compiler-pin.json").write_text(json.dumps(
        {"path": str(compiler_path), "sha256": sha256(compiler_path)}, indent=2))
    tool_pins = {}
    for name in ("cc1plus", "collect2", "as", "ld"):
        log = output / (name + "-path.txt")
        result = run_logged(compiler + ["-print-prog-name=" + name], log, 20)
        assert result.returncode == 0, str(log)
        value = log.read_text().strip()
        path = Path(value if "/" in value else shutil.which(value)).resolve()
        tool_pins[name] = {"path": str(path), "sha256": sha256(path)}
    (output / "compiler-tool-pins.json").write_text(json.dumps(tool_pins, indent=2))
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-" + args.optimization, "-g", "-fsanitize=address,undefined",
             "-fno-omit-frame-pointer", "-fno-pie", "-ffunction-sections",
             "-fdata-sections", "-D__NO_MYSQL__", "-Isrc", "-Isrc/no_mysql", "-I."]
    commands = []
    direct = [root / source for source in SOURCES] + [harness, harness_source, Path(__file__).resolve()]
    before = {str(path): sha256(path) for path in direct}
    (output / "direct-input-pins.json").write_text(json.dumps(before, indent=2))
    units = [harness] + [root / source for source in SOURCES]

    def compile_unit(index_source):
        index, source = index_source
        obj = output / (str(index) + ".o")
        command = compiler + flags + ["-MD", "-MF", str(output / (str(index) + ".d")),
                                      "-c", str(source), "-o", str(obj)]
        start = time.monotonic()
        result = run_logged(command, output / (str(index) + "-compile.log"), 900, cwd=root)
        return {"command": command, "returncode": result.returncode,
                "seconds": time.monotonic() - start, "timeout_seconds": 900}

    # These are independent compilation jobs, not substitute providers.
    with ThreadPoolExecutor(max_workers=3) as pool:
        commands.extend(pool.map(compile_unit, enumerate(units)))
    (output / "compile-commands.json").write_text(json.dumps(commands, indent=2))
    assert all(entry["returncode"] == 0 for entry in commands), str(output)
    dependencies = set(direct)
    for depfile in output.glob("*.d"):
        # GCC make dependencies escape spaces and wrap lines; shlex handles the
        # escaped paths after the target colon (Linux paths contain no drive colon).
        value = depfile.read_text().replace(chr(92) + chr(10), " ").split(":", 1)[1]
        dependencies.update((root / token).resolve() for token in shlex.split(value))
    pins = {str(path): sha256(path) for path in sorted(dependencies)}
    assert all(pins[path] == digest for path, digest in before.items())
    (output / "input-pins.json").write_text(json.dumps(pins, indent=2))
    executable = output / "collector-collection-native-capture"
    link = compiler + flags + ["-no-pie"] + [str(output / (str(i) + ".o"))
                                            for i in range(len(units))] + [
        "-Wl,--gc-sections", "-Wl,-Map=" + str(output / "link.map"),
        "-lcrypto", "-pthread", "-o", str(executable)]
    result = run_logged(link, output / "link.log", 120, cwd=root)
    (output / "link-command.json").write_text(json.dumps(
        {"command": link, "returncode": result.returncode, "timeout_seconds": 120}, indent=2))
    assert result.returncode == 0, str(output)
    result = run_logged(["nm", "-C", "--defined-only", str(executable)],
                        output / "defined-symbols.txt", 20)
    assert result.returncode == 0, str(output / "defined-symbols.txt")
    symbols = (output / "defined-symbols.txt").read_text()
    for symbol in ("collector_collection_prepare(", "collector_collection_live_matches(",
                   "player_item_snapshot_tree_capture(", "player_item_snapshot_list_encode(",
                   "player_item_snapshot_list_decode(", "item_ownership_runtime_lookup(",
                   "item_ownership_runtime_hydrate(", "item_owner_identity_valid(",
                   "item_owner_key(", "collector_command_build(",
                   "collector_command_encode_payload(", "collector_command_decode_payload(",
                   "critical_command_encode(", "critical_command_decode(", "isname("):
        assert symbol in symbols, symbol
    assert "collector_collection_detach_live(" not in symbols
    result = run_logged(["ldd", str(executable)], output / "shared-libraries.txt", 20)
    assert result.returncode == 0, str(output / "shared-libraries.txt")
    libraries = (output / "shared-libraries.txt").read_text()
    library_pins = {}
    for line in libraries.splitlines():
        fields = line.split()
        path = next((Path(field) for field in fields if field.startswith("/")), None)
        if path:
            library_pins[str(path.resolve())] = sha256(path)
    (output / "shared-library-pins.json").write_text(json.dumps(library_pins, indent=2))
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    (output / "runtime-profile.json").write_text(json.dumps(
        {"optimization": args.optimization, "TMPDIR": environment.get("TMPDIR"),
         "ASAN_OPTIONS": environment["ASAN_OPTIONS"],
         "UBSAN_OPTIONS": environment["UBSAN_OPTIONS"],
         "native_durable_publication_qualification": False}, indent=2))
    runs = []
    for case in CASES:
        log_path = output / (case + ".log")
        result = run_logged([str(executable), case], log_path, 90, cwd=root, env=environment)
        runs.append({"case": case, "command": [str(executable), case],
                     "returncode": result.returncode, "timeout_seconds": 90})
        (output / "runs.json").write_text(json.dumps(runs, indent=2))
        assert result.returncode == 0 and log_path.read_text().startswith("PASS " + case + " "), (
            case, str(log_path), str(output))
    assert all(sha256(Path(path)) == digest for path, digest in pins.items())
    artifacts = {path.name: sha256(path) for path in sorted(output.iterdir()) if path.is_file()}
    (output / "artifact-pins.json").write_text(json.dumps(artifacts, indent=2))
    print(json.dumps({"passed": len(runs), "optimization": args.optimization,
                      "output": str(output), "elf_sha256": sha256(executable),
                      "native_durable_publication_qualification": False}, indent=2))


if __name__ == "__main__":
    main()
