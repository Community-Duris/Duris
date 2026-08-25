"""Contracts for the C/C++ formatting configuration and tooling.

`.clang-format` once carried two keys no released clang-format understands,
which made every invocation fail outright -- a config nothing can load is
worse than no config.  These checks pin that it stays loadable, that the style
it encodes does not silently drift, and that the guidance in AGENTS.md and
README.md still matches what the tooling actually does.
"""

import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CONFIG = ROOT / ".clang-format"
SCRIPT = ROOT / "scripts/format.sh"
DOC = ROOT / "docs/formatting.md"

# --------------------------------------------------------------------------
# 1. The config must be loadable by the installed clang-format.  This is the
#    check that would have caught the original breakage.
# --------------------------------------------------------------------------
assert CONFIG.is_file(), ".clang-format is missing"

clang_format = shutil.which("clang-format")
if clang_format:
    probe = subprocess.run(
        [clang_format, "--dump-config", f"-style=file:{CONFIG}"],
        capture_output=True, text=True,
    )
    assert probe.returncode == 0, (
        f"clang-format cannot load .clang-format:\n{probe.stderr}"
    )

    # And it must actually format a sample the way the style claims: hard
    # tabs, Allman braces.
    sample = "int f(int a,int b){if(a>b){return a;}\nreturn b;}\n"
    out = subprocess.run(
        [clang_format, f"-style=file:{CONFIG}", "--assume-filename=probe.c"],
        input=sample, capture_output=True, text=True, check=True,
    ).stdout
    assert "int f(int a, int b)\n{\n" in out, f"Allman braces not applied:\n{out}"
    assert "\treturn a;" in out, f"hard-tab indentation not applied:\n{out}"
    assert "    return" not in out, f"space indentation leaked in:\n{out}"

# --------------------------------------------------------------------------
# 2. The style itself.  Changing any of these is a deliberate, tree-wide
#    decision, not an incidental edit.
# --------------------------------------------------------------------------
config = CONFIG.read_text()
for key, value in (
    ("UseTab", "Always"),
    ("IndentWidth", "8"),
    ("TabWidth", "8"),
    ("BreakBeforeBraces", "Allman"),
    ("ColumnLimit", "100"),
    ("PointerAlignment", "Right"),
    ("SortIncludes", "false"),
):
    assert f"{key}: {value}" in config, f"{key} is no longer {value}"

# src/*.c is compiled as C++20 by src/Makefile; the formatter must parse it the
# same way the compiler does.
assert "Standard: c++20" in config, ".clang-format no longer targets c++20"
makefile = (ROOT / "src/Makefile").read_text()
assert "-std=c++20" in makefile, "src/Makefile no longer builds C++20 -- resync Standard"

# The keys that made the file unloadable must never come back.
for dead in ("BreakAfterOpenBracketBracedList", "BreakBeforeCloseBracketBracedList"):
    assert dead not in config, f"{dead} is not a real clang-format option"

# Duris' iteration macros must stay registered, or they get formatted as calls.
for macro in ("LOOP_THRU_PEOPLE", "LOOP_EVENTS_CH", "LOOP_EVENTS_OBJ"):
    assert macro in config, f"{macro} dropped from ForEachMacros"

# --------------------------------------------------------------------------
# 3. The runner.
# --------------------------------------------------------------------------
assert SCRIPT.is_file(), "scripts/format.sh is missing"
assert os.access(SCRIPT, os.X_OK), "scripts/format.sh is not executable"
subprocess.run(["bash", "-n", str(SCRIPT)], check=True)

body = SCRIPT.read_text()
assert "git clang-format" in body, "format.sh must format changed lines, not whole files"
assert "--force" in body, (
    "without --force git-clang-format skips files that have unstaged edits"
)

helped = subprocess.run(
    ["bash", str(SCRIPT), "--help"], cwd=ROOT, capture_output=True, text=True
)
assert helped.returncode == 0 and "--check" in helped.stdout
assert "set -euo" not in helped.stdout, "--help is leaking script source"

bad = subprocess.run(
    ["bash", str(SCRIPT), "--nonsense"], cwd=ROOT, capture_output=True, text=True
)
assert bad.returncode == 2, f"unknown option exited {bad.returncode}, expected 2"

# --check must pass on a clean tree: the repository's own committed C/C++ is
# never expected to be reformatted by a no-op diff.
if clang_format and shutil.which("git-clang-format"):
    checked = subprocess.run(
        ["bash", str(SCRIPT), "--check", "--rev", "HEAD"],
        cwd=ROOT, capture_output=True, text=True,
    )
    assert checked.returncode == 0, (
        "scripts/format.sh --check fails against HEAD:\n"
        f"{checked.stdout}\n{checked.stderr}"
    )

# --------------------------------------------------------------------------
# 3b. The pre-commit hook and its installer.
# --------------------------------------------------------------------------
HOOK = ROOT / "scripts/git-hooks/pre-commit"
INSTALLER = ROOT / "scripts/install-hooks.sh"

for path in (HOOK, INSTALLER):
    assert path.is_file(), f"{path.relative_to(ROOT)} is missing"
    assert os.access(path, os.X_OK), f"{path.relative_to(ROOT)} is not executable"
    subprocess.run(["bash", "-n", str(path)], check=True)

hook = HOOK.read_text()
assert "--staged --check" in hook, "the hook must check staged lines only"
assert "--no-verify" in hook, "the hook must tell the user how to bypass it"
# A missing clang-format must warn and let the commit through, not block work.
missing_branch = hook.split("command -v clang-format", 1)[1].split("fi", 1)[0]
assert "exit 0" in missing_branch, (
    "the hook must not block commits when clang-format is unavailable"
)

installer = INSTALLER.read_text()
assert "core.hooksPath" in installer and "scripts/git-hooks" in installer
assert "--uninstall" in installer, "the installer must be reversible"

bad_install = subprocess.run(
    ["bash", str(INSTALLER), "--bogus"], cwd=ROOT, capture_output=True, text=True
)
assert bad_install.returncode == 2, (
    f"install-hooks.sh --bogus exited {bad_install.returncode}, expected 2"
)

# --------------------------------------------------------------------------
# 4. Documentation and dependency declarations.
# --------------------------------------------------------------------------
assert DOC.is_file(), "docs/formatting.md is missing"
doc = DOC.read_text()
assert "scripts/format.sh" in doc, "docs/formatting.md does not document the runner"
assert "Do not mass-format" in doc, "docs/formatting.md lost the mass-format warning"
assert "install-hooks.sh" in doc, "docs/formatting.md does not document the hook"
assert "tabs" in doc and "Allman" in doc

agents = (ROOT / "AGENTS.md").read_text()
assert "Do not mass-format files" in agents, "AGENTS.md lost the no-mass-format rule"
assert "scripts/format.sh" in agents, "AGENTS.md does not point at the formatter"

readme = (ROOT / "README.md").read_text()
assert "2 spaces" not in readme, (
    "README.md still claims 2-space indentation, contradicting .clang-format"
)
assert "clang-format" in readme and "docs/formatting.md" in readme

equivs = (ROOT / "packaging/duris-build-deps.equivs").read_text()
depends = equivs.split("Depends:", 1)[1].split("Suggests:", 1)[0]
assert "clang-format" in depends, "clang-format is not a declared build dependency"

print("formatting tooling contracts OK")
