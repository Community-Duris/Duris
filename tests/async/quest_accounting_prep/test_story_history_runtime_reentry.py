#!/usr/bin/env python3
"""Real flat story runtime component; no server, database or native journey.

Run on Linux/WSL. Builds/evidence default to task-specific D: directories.
The disposable authority defaults to private tmpfs because ordinary DrvFS
mounts cannot satisfy the real provider's mandatory ownership/mode checks.
Every compile/run stream, including unsuccessful attempts, remains in evidence.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import signal
import subprocess
import sys
import tarfile
import tempfile
import time

REPO = Path(__file__).resolve().parents[3]
HARNESS = Path(__file__).with_name("story_history_runtime_reentry.cpp")
SOURCES = [
    "src/world/zone_story_quest_runtime.c",
    "src/world/zone_story_quest_production.c",
    "src/world/zone_story_quest_feature.c",
    "src/world/zone_story_quest_tracking.c",
    "src/world/zone_story_quest_catalog.c",
    "src/flatfile/flatfile_zone_story_quest_state.c",
    "src/flatfile/flatfile_store.c",
    "src/flatfile/flatfile_authority_transaction.c",
    "src/persistence/persistence_mode.c",
    "src/flatfile/flatfile_ip_activity_repository.c",
]
COMMON = [
    "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
    "-D__NO_MYSQL__", "-DTEST_MUD", "-D__NO_TESTS__", "-Isrc", "-Isrc/no_mysql",
    "-ffunction-sections", "-fdata-sections",
]
PROFILES = {
    "O1": ["-O1"],
    "Og-sanitized": [
        "-Og", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        "-fno-sanitize-recover=all",
        "-fno-pie", "-no-pie",
    ],
}


def digest(path):
    data = path.read_bytes()
    return {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def save(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate", required=True, help="published source commit/ref")
    parser.add_argument("--git", default="git", help="Git executable")
    parser.add_argument("--git-dir", type=Path,
                        help="mapped worktree Git metadata when its .git uses Windows paths")
    parser.add_argument("--evidence-parent", type=Path, default=Path("/mnt/d/Dev/Temp"))
    parser.add_argument("--bin-parent", type=Path,
                        default=Path("/mnt/d/Dev/Builds/NewDuris/quest-story-runtime"))
    parser.add_argument("--authority-parent", type=Path, default=Path("/dev/shm"))
    args = parser.parse_args()
    git = [args.git, *(["--git-dir=" + str(args.git_dir)] if args.git_dir else []),
           "--work-tree=" + str(REPO)]
    if sys.platform != "linux":
        parser.error("Linux/WSL is required for actual POSIX providers and syscall wrapping")
    args.evidence_parent.mkdir(parents=True, exist_ok=True)
    evidence = Path(tempfile.mkdtemp(prefix="quest-story-runtime-", dir=args.evidence_parent))
    binary_dir = args.bin_parent / evidence.name / "bin"
    binary_dir.mkdir(parents=True, exist_ok=False)
    print(f"evidence={evidence}\nbin={binary_dir}", flush=True)
    deadline = time.monotonic() + 600
    sequence = 0
    environment = os.environ.copy()
    environment.update({"TMPDIR": str(args.evidence_parent), "PYTHONDONTWRITEBYTECODE": "1",
                        "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1",
                        "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})

    def run(label, command, cwd=REPO, timeout=60):
        nonlocal sequence
        sequence += 1
        prefix = evidence / f"{sequence:03d}-{label}"
        command = [str(part) for part in command]
        limit = min(timeout, deadline - time.monotonic())
        if limit <= 0:
            raise RuntimeError("600-second whole component deadline exceeded")
        save(prefix.with_suffix(".command.json"), {
            "argv": command, "cwd": str(cwd), "timeout_seconds": limit,
            "environment_overrides": {key: environment[key] for key in
                                      ("TMPDIR", "ASAN_OPTIONS", "UBSAN_OPTIONS",
                                       "PYTHONDONTWRITEBYTECODE")},
        })
        started = time.monotonic()
        with prefix.with_suffix(".stdout").open("wb") as out, \
                prefix.with_suffix(".stderr").open("wb") as err:
            process = subprocess.Popen(command, cwd=cwd, env=environment,
                                       stdout=out, stderr=err, start_new_session=True)
            timed_out = False
            try:
                process.wait(timeout=limit)
            except subprocess.TimeoutExpired:
                timed_out = True
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
        result = {"returncode": process.returncode, "timeout": timed_out,
                  "elapsed_seconds": time.monotonic() - started}
        save(prefix.with_suffix(".result.json"), result)
        if process.returncode or timed_out:
            raise RuntimeError(f"{label}: {result}; streams retained at {prefix}")
        return prefix.with_suffix(".stdout").read_bytes()

    manifest = {"status": "running", "profiles": PROFILES, "sources": SOURCES}
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
                    raise RuntimeError(f"unexpected source archive member {member.name}")
            tar.extractall(source_root)
        # Preserve the exact submitted test inputs outside the mutable worktree.
        harness_copy = evidence / HARNESS.name
        shutil.copyfile(HARNESS, harness_copy)
        runner_copy = evidence / Path(__file__).name
        shutil.copyfile(__file__, runner_copy)
        manifest["test_inputs"] = {str(path): digest(path) for path in
                                   (harness_copy, runner_copy)}
        manifest["source_archive"] = digest(archive)
        compiler = Path(shutil.which("g++") or "missing").resolve(strict=True)
        run("compiler-version", [compiler, "--version"])
        run("compiler-target", [compiler, "-v"])
        run("system", ["uname", "-a"])
        run("mounts-D", ["findmnt", "--json", "-T", args.evidence_parent])
        run("mounts-authority", ["findmnt", "--json", "-T", args.authority_parent])
        probe = evidence / "metadata-probe"
        probe.mkdir(mode=0o700)
        probe.chmod(0o700)
        info = probe.stat()
        manifest["D_metadata_probe"] = {
            "path": str(probe), "mode": oct(info.st_mode & 0o777), "uid": info.st_uid,
            "euid": os.geteuid(),
            "private": info.st_uid == os.geteuid() and info.st_mode & 0o077 == 0,
        }
        probe.rmdir()
        interpreter = Path(sys.executable).resolve(strict=True)
        git_binary = Path(shutil.which(args.git) or args.git).resolve(strict=True)
        nm = Path(shutil.which("nm") or "missing").resolve(strict=True)
        tools = {str(path): digest(path) for path in (compiler, interpreter, git_binary, nm)}
        for name in ("cc1plus", "collect2", "as", "ld", "libasan.so", "libubsan.so",
                     "libstdc++.so", "libgcc_s.so", "libcrypto.so"):
            option = "-print-prog-name=" if name in ("cc1plus", "collect2", "as", "ld") \
                else "-print-file-name="
            output = run("tool-" + name.replace(".", "-"),
                         [compiler, option + name]).decode().strip()
            path = Path(output) if "/" in output else Path(shutil.which(output) or output)
            path = path.resolve(strict=True)
            tools[str(path)] = digest(path)
        manifest["tools"] = tools
        compiled = [str(harness_copy), *SOURCES]
        dependencies = {}
        profile_files = {}
        for profile, flags in PROFILES.items():
            print(f"building/running {profile}", flush=True)
            build = binary_dir / profile
            build.mkdir()
            objects = []
            for index, source in enumerate(compiled):
                obj = build / f"{index:02d}.o"
                dep = build / f"{index:02d}.d"
                run(f"{profile}-compile-{index:02d}",
                    [compiler, *COMMON, *flags, "-MD", "-MF", dep, "-c", source, "-o", obj],
                    cwd=source_root, timeout=90)
                # GCC's make dependency escaping is parsed only for file paths;
                # the actual provider/parser is never extracted or rewritten.
                text = dep.read_text().replace("\\\n", " ")
                for item in shlex.split(text.split(":", 1)[1]):
                    path = (source_root / item).resolve(strict=True)
                    dependencies[str(path)] = digest(path)
                objects.append(obj)
            binary = build / "story_history_runtime_reentry"
            run(profile + "-link", [compiler, *flags, *objects, "-lcrypto", "-pthread",
                                    "-Wl,--gc-sections", "-Wl,--wrap=renameat",
                                    "-Wl,--wrap=fsync", "-Wl,-Map," + str(build / "link.map"),
                                    "-o", binary],
                cwd=source_root, timeout=90)
            manifest.setdefault("binaries", {})[profile] = digest(binary)
            run(profile + "-symbols", [nm, "-C", binary])
            linked = run(profile + "-ldd", ["ldd", binary]).decode()
            libraries = {}
            for line in linked.splitlines():
                for word in line.split():
                    if word.startswith("/"):
                        path = Path(word).resolve(strict=True)
                        libraries[str(path)] = digest(path)
            manifest.setdefault("libraries", {})[profile] = libraries
            profile_evidence = evidence / profile
            profile_evidence.mkdir()
            with tempfile.TemporaryDirectory(prefix="duris-story-reentry-",
                                             dir=args.authority_parent) as temporary:
                root = Path(temporary)
                info = root.stat()
                if info.st_uid != os.geteuid() or info.st_mode & 0o077:
                    raise RuntimeError("authority filesystem cannot supply private metadata")
                run(profile + "-authority-mount", ["findmnt", "--json", "-T", root])
                output = run(profile + "-component", [binary, root, profile_evidence],
                             cwd=source_root, timeout=30)
                print(output.decode().strip(), flush=True)
                inventory = {str(path.relative_to(root)): {
                    "mode": oct(path.stat().st_mode), "uid": path.stat().st_uid,
                    **(digest(path) if path.is_file() else {}),
                } for path in root.rglob("*")}
                save(profile_evidence / "authority-inventory.json", inventory)
                if any(".critical-authority-transaction" in name for name in inventory):
                    raise RuntimeError("unexpected pending journal")
            if root.exists():
                raise RuntimeError("disposable authority cleanup failed")
            save(profile_evidence / "cleanup.json", {"root": str(root), "removed": True})
            profile_files[profile] = {path.name: digest(path) for path in
                                      profile_evidence.iterdir()
                                      if path.suffix == ".bin" or path.name.endswith(
                                          (".memory.txt", ".authority.txt"))}
        if profile_files["O1"] != profile_files["Og-sanitized"]:
            raise RuntimeError("exact memory/authority/file bytes differ across profiles")
        manifest["dependencies"] = dependencies
        manifest["dependency_count"] = len(dependencies)
        manifest["status"] = "PASS"
    except Exception as error:
        manifest["status"] = "FAIL"
        manifest["error"] = str(error)
        raise
    finally:
        save(evidence / "manifest.json", manifest)
        save(evidence / "ARTIFACTS.json", {str(path.relative_to(evidence)): digest(path)
                                         for path in evidence.rglob("*") if path.is_file()
                                         and path.name != "ARTIFACTS.json"})
    print("PASS both profiles; exact cross-profile bytes; isolated authorities removed", flush=True)


if __name__ == "__main__":
    main()
