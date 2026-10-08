#!/usr/bin/env python3
"""Real complete MySQL qry_at/repository TUs with an explicit SQL trace double.

No database service/client execution. Linux/WSL, task-specific D: evidence/builds.
Only the SQL trace symbol is weakened in a copied object; all executable section
bytes and relocations are authenticated. Real utility.c supplies reached logging.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import signal
import struct
import subprocess
import sys
import tarfile
import tempfile
import time

REPO = Path(__file__).resolve().parents[3]
HARNESS = Path(__file__).with_name("story_history_sql_capacity.cpp")
TRACE = "_Z17sql_trace_exec_at22persistence_query_sitePKcS1_mbb"
QRY = "_Z6qry_at22persistence_query_sitePKcz"
SOURCES = ["src/sql/zone_story_quest_state_repository.c", "src/sql/sql.c", "src/core/utility.c"]
COMMON = ["-std=c++20", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
          "-DTEST_MUD", "-D__NO_TESTS__", "-Isrc", "-ffunction-sections", "-fdata-sections"]
PROFILES = {"O1": ["-O1"], "Og-sanitized": [
    "-Og", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
    "-fno-sanitize-recover=all", "-fno-pie", "-no-pie",
]}


def identity(path):
    data = path.read_bytes()
    return {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def save(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2) + "\n")


def elf_sections(path):
    """Observe ELF64 LE section metadata; never rewrite code/relocations."""
    data = path.read_bytes()
    if data[:6] != b"\x7fELF\x02\x01":
        raise RuntimeError("component proof requires ELF64 little endian objects")
    offset = struct.unpack_from("<Q", data, 40)[0]
    entry_size, count, strings_index = struct.unpack_from("<HHH", data, 58)
    if entry_size != 64 or not count or strings_index >= count:
        raise RuntimeError("unsupported ELF section table")
    headers = [struct.unpack_from("<IIQQQQIIQQ", data, offset + i * 64) for i in range(count)]
    strings_header = headers[strings_index]
    strings = data[strings_header[4]:strings_header[4] + strings_header[5]]
    sections = {}
    for index, header in enumerate(headers):
        end = strings.find(b"\0", header[0])
        if end < 0:
            raise RuntimeError("invalid ELF section name")
        name = strings[header[0]:end].decode()
        payload = b"" if header[1] == 8 else data[header[4]:header[4] + header[5]]
        # COMDAT groups legitimately repeat .group; preserve every section index.
        sections[index] = {"name": name, "metadata": (*header[1:4], *header[5:]),
                           "data": payload, "header": header}
    return data[:40] + data[48:64], sections


def prove_trace_binding(original, controlled):
    header_before, before = elf_sections(original)
    header_after, after = elf_sections(controlled)
    if header_before != header_after or before.keys() != after.keys():
        raise RuntimeError("objcopy changed ELF identity/section inventory")
    changed = []
    for index, section in before.items():
        other = after[index]
        name = section["name"]
        if section["metadata"] != other["metadata"] or name != other["name"]:
            raise RuntimeError("objcopy changed semantic section metadata: " + name)
        if section["data"] != other["data"]:
            changed.append(name)
    if set(changed) != {".symtab", ".shstrtab"}:
        raise RuntimeError("unexpected changed object sections: " + str(changed))
    def unique_named(sections, name):
        matches = [section for section in sections.values() if section["name"] == name]
        if len(matches) != 1:
            raise RuntimeError("required ELF section is not unique: " + name)
        return matches[0]

    symbol_section = unique_named(before, ".symtab")
    symbols = symbol_section["data"]
    names = unique_named(before, ".strtab")["data"]
    if symbol_section["header"][9] != 24:
        raise RuntimeError("unexpected symbol entry size")
    matches = []
    for offset in range(0, len(symbols), 24):
        name_offset = struct.unpack_from("<I", symbols, offset)[0]
        if names[name_offset:names.find(b"\0", name_offset)].decode() == TRACE:
            matches.append(offset)
    if len(matches) != 1:
        raise RuntimeError("original trace function symbol is not unique")
    offset = matches[0] + 4  # Elf64_Sym.st_info, STB_GLOBAL/STT_FUNC -> STB_WEAK/STT_FUNC.
    expected = bytearray(symbols)
    if expected[offset] != 0x12:
        raise RuntimeError("original trace symbol is not a global function")
    expected[offset] = 0x22
    if bytes(expected) != unique_named(after, ".symtab")["data"]:
        raise RuntimeError("symbol change exceeds the intended trace binding")
    # objcopy re-encodes section names, while names/index/metadata remain equal.
    # Every other section, including all text, relocations and debug data, is exact.
    qry_section = ".text." + QRY
    qry_code = unique_named(before, qry_section)["data"]
    return {"original": identity(original), "controlled": identity(controlled),
            "only_semantic_change": TRACE + " GLOBAL/FUNC -> WEAK/FUNC",
            "representation_change": "section-name string table re-encoded by objcopy",
            "sections_authenticated": len(before), "qry_section": qry_section,
            "qry_code_sha256": hashlib.sha256(qry_code).hexdigest(),
            "qry_code_bytes": len(qry_code)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", required=True)
    parser.add_argument("--git-dir", type=Path,
                        help="mapped metadata for a Windows-managed worktree")
    parser.add_argument("--evidence-parent", type=Path, default=Path("/mnt/d/Dev/Temp"))
    parser.add_argument("--bin-parent", type=Path,
                        default=Path("/mnt/d/Dev/Builds/NewDuris/quest-story-sql-capacity"))
    args = parser.parse_args()
    if sys.platform != "linux":
        parser.error("Linux/WSL is required for this actual ELF/provider component")
    args.evidence_parent.mkdir(parents=True, exist_ok=True)
    evidence = Path(tempfile.mkdtemp(prefix="quest-story-sql-capacity-", dir=args.evidence_parent))
    bins = args.bin_parent / evidence.name / "bin"
    bins.mkdir(parents=True, exist_ok=False)
    print(f"evidence={evidence}\nbin={bins}", flush=True)
    environment = {**os.environ, "TMPDIR": str(args.evidence_parent),
                   "PYTHONDONTWRITEBYTECODE": "1", "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                   "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"}
    git = ["git", *(["--git-dir=" + str(args.git_dir)] if args.git_dir else []),
           "--work-tree=" + str(REPO)]
    deadline = time.monotonic() + 600
    counter = 0
    dependencies = {}
    manifest = {"status": "running", "sources": SOURCES, "profiles": PROFILES,
                "common_flags": COMMON, "SQL_double": ["DB handle token", "sql_escape_string",
                                                        "sql_trace_exec_at"]}

    def run(label, command, cwd=REPO, timeout=60):
        nonlocal counter
        counter += 1
        prefix = evidence / f"{counter:03d}-{label}"
        command = list(map(str, command))
        limit = min(timeout, deadline - time.monotonic())
        if limit <= 0:
            raise RuntimeError("600-second whole-component deadline exceeded")
        save(prefix.with_suffix(".command.json"), {"argv": command, "cwd": str(cwd),
                                                   "timeout_seconds": limit,
                                                   "environment_overrides": {key: environment[key]
                                                       for key in ("TMPDIR", "ASAN_OPTIONS",
                                                                   "UBSAN_OPTIONS")}})
        started = time.monotonic()
        code = None
        timed_out = False
        launch_error = None
        try:
            with prefix.with_suffix(".stdout").open("wb") as out, \
                    prefix.with_suffix(".stderr").open("wb") as err:
                process = subprocess.Popen(command, cwd=cwd, env=environment, stdout=out,
                                           stderr=err, start_new_session=True)
                try:
                    code = process.wait(timeout=limit)
                except subprocess.TimeoutExpired:
                    timed_out = True
                    os.killpg(process.pid, signal.SIGKILL)
                    code = process.wait()
        except OSError as error:
            launch_error = repr(error)
        result = {"returncode": code, "timeout": timed_out, "launch_error": launch_error,
                  "elapsed_seconds": time.monotonic() - started}
        save(prefix.with_suffix(".result.json"), result)
        if code != 0 or timed_out or launch_error:
            raise RuntimeError(f"{label}: {result}; streams retained at {prefix}")
        return prefix.with_suffix(".stdout").read_bytes()

    try:
        pin = run("candidate", [*git, "rev-parse", "--verify",
                                args.candidate + "^{commit}"]).decode().strip()
        manifest["candidate"] = pin
        manifest["prep_head"] = run("prep-head", [*git, "rev-parse", "HEAD"]).decode().strip()
        run("source-trees", [*git, "rev-parse", pin + ":src", pin + ":migrations"])
        archive = evidence / "candidate-src.tar"
        archive.write_bytes(run("source-archive", [*git, "archive", pin, "src"]))
        source_root = evidence / "candidate"
        source_root.mkdir()
        with tarfile.open(archive) as tar:
            for member in tar.getmembers():
                if member.name.startswith("/") or ".." in Path(member.name).parts or \
                        not (member.isdir() or member.isfile()):
                    raise RuntimeError("unexpected archive member " + member.name)
            tar.extractall(source_root)
        manifest["source_archive"] = identity(archive)
        harness = evidence / HARNESS.name
        shutil.copyfile(HARNESS, harness)
        runner = evidence / Path(__file__).name
        shutil.copyfile(__file__, runner)
        manifest["test_inputs"] = {str(path): identity(path) for path in (harness, runner)}
        mysql_flags = shlex.split(run("mysql-cflags", ["mysql_config", "--cflags"]).decode())
        manifest["mysql_cflags"] = mysql_flags
        run("mysql-version", ["mysql_config", "--version"])
        run("compiler-version", ["g++", "--version"])
        run("compiler-config", ["g++", "-v"])
        run("compiler-specs", ["g++", "-dumpspecs"])
        run("system", ["uname", "-a"])
        run("mounts", ["findmnt", "--json", "-T", evidence])
        tools = {str(path): identity(path) for path in
                 [Path(shutil.which(name) or name).resolve(strict=True) for name in
                  ("g++", "git", "objcopy", "objdump", "nm", "mysql_config", "ldd")] +
                 [Path(sys.executable).resolve(strict=True)]}
        for name in ("cc1plus", "collect2", "as", "ld", "libasan.so", "libubsan.so",
                     "libstdc++.so", "libgcc_s.so"):
            option = "-print-prog-name=" if name in ("cc1plus", "collect2", "as", "ld") \
                else "-print-file-name="
            value = run("tool-" + name.replace(".", "-"), ["g++", option + name]).decode().strip()
            path = (Path(value) if "/" in value else Path(shutil.which(value) or value)).resolve(strict=True)
            tools[str(path)] = identity(path)
        manifest["tools"] = tools
        profile_bytes = {}
        for profile, flags in PROFILES.items():
            print("building/running " + profile, flush=True)
            build = bins / profile
            build.mkdir()
            objects = []
            for index, source in enumerate([str(harness), *SOURCES]):
                obj = build / f"{index:02d}.o"
                dep = build / f"{index:02d}.d"
                run(f"{profile}-compile-{index:02d}", ["g++", *COMMON, *mysql_flags, *flags,
                    "-MD", "-MF", dep, "-c", source, "-o", obj], cwd=source_root, timeout=180)
                for item in shlex.split(dep.read_text().replace("\\\n", " ").split(":", 1)[1]):
                    path = (source_root / item).resolve(strict=True)
                    dependencies[str(path)] = identity(path)
                objects.append(obj)
            original = objects[2]
            run(profile + "-original-symbols", ["nm", original])
            run(profile + "-original-relocations", ["objdump", "-r", original])
            controlled = build / "02-trace-boundary.o"
            run(profile + "-trace-binding", ["objcopy", "--weaken-symbol=" + TRACE,
                                             original, controlled])
            manifest.setdefault("binding_proof", {})[profile] = prove_trace_binding(original, controlled)
            run(profile + "-controlled-symbols", ["nm", controlled])
            run(profile + "-controlled-relocations", ["objdump", "-r", controlled])
            objects[2] = controlled
            binary = build / "story_history_sql_capacity"
            run(profile + "-link", ["g++", *flags, *objects, "-Wl,--gc-sections", "-pthread",
                                     "-Wl,-Map," + str(build / "link.map"), "-o", binary],
                cwd=source_root, timeout=90)
            symbols = run(profile + "-ELF-symbols", ["nm", "-C", binary]).decode()
            for required in ("qry_at(", "sql_trace_exec_at(", "logit(", "sql_zone_story_quest_state_save("):
                if not any(" T " in line and required in line for line in symbols.splitlines()):
                    raise RuntimeError("final strong provider missing: " + required)
            undefined = run(profile + "-ELF-undefined", ["nm", "-u", binary]).decode()
            if "mysql_" in undefined:
                raise RuntimeError("unexpected live MySQL client dependency")
            run(profile + "-qry-disassembly", ["objdump", "-dr", "--disassemble=" + QRY, binary])
            run(profile + "-trace-disassembly", ["objdump", "-dr", "--disassemble=" + TRACE, binary])
            linked = run(profile + "-libraries", ["ldd", binary]).decode()
            libraries = {}
            for word in linked.split():
                if word.startswith("/"):
                    path = Path(word).resolve(strict=True)
                    libraries[str(path)] = identity(path)
            manifest.setdefault("libraries", {})[profile] = libraries
            manifest.setdefault("binaries", {})[profile] = identity(binary)
            output = evidence / profile
            output.mkdir()
            work = output / "work"
            work.mkdir()
            stdout = run(profile + "-component", [binary, output], cwd=work, timeout=30)
            print(stdout.decode().strip(), flush=True)
            profile_bytes[profile] = {path.name: identity(path) for path in output.iterdir()
                                      if path.is_file() and not path.name.endswith(".log.txt")}
        if profile_bytes["O1"] != profile_bytes["Og-sanitized"]:
            raise RuntimeError("exact query/observation bytes differ across profiles")
        manifest["status"] = "PASS"
    except Exception as error:
        manifest["status"] = "FAIL"
        manifest["error"] = str(error)
        raise
    finally:
        manifest["dependencies"] = dependencies
        manifest["dependency_count"] = len(dependencies)
        save(evidence / "manifest.json", manifest)
        save(evidence / "ARTIFACTS.json", {str(path.relative_to(evidence)): identity(path)
            for path in evidence.rglob("*") if path.is_file() and path.name != "ARTIFACTS.json"})
    print("PASS both profiles; complete-query boundary and exact SQL-double observations", flush=True)


if __name__ == "__main__":
    main()
