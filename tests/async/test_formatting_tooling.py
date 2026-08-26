"""Contracts for the C/C++ formatting configuration and tooling.

`.clang-format` once carried two keys no released clang-format understands,
which made every invocation fail outright -- a config nothing can load is
worse than no config.  These checks pin that it stays loadable, that the style
it encodes does not silently drift, and that the guidance in AGENTS.md and
README.md still matches what the tooling actually does.
"""

import os
import re
import shutil
import subprocess
import tempfile
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
assert "--all" in body, "format.sh lost whole-tree formatting"

# --all must verify a real fixpoint.  A dirty filename remains in
# `git diff --name-only` before and after formatting, so comparing those lists
# reports success after one pass even when clang-format would change it again.
with tempfile.TemporaryDirectory() as temp_dir:
    fixture = Path(temp_dir)
    (fixture / "scripts").mkdir()
    (fixture / "src").mkdir()
    fake_bin = fixture / "bin"
    fake_bin.mkdir()
    shutil.copy2(SCRIPT, fixture / "scripts/format.sh")
    (fixture / ".clang-format").write_text("---\n")
    probe = fixture / "src/probe.c"
    probe.write_text("stage0\n")

    fake_git = fake_bin / "git"
    fake_git.write_text(
        "#!/usr/bin/env bash\n"
        "if [[ $1 == ls-files ]]; then\n"
        "  printf 'src/probe.c\\n'\n"
        "elif [[ $1 == diff ]]; then\n"
        "  printf 'src/probe.c\\n'\n"
        "else\n"
        "  exit 1\n"
        "fi\n"
    )
    fake_git.chmod(0o755)

    fake_formatter = fake_bin / "clang-format"
    fake_formatter.write_text(
        "#!/usr/bin/env bash\n"
        "if [[ \" $* \" == *\" --dump-config \"* ]]; then exit 0; fi\n"
        "target=${!#}\n"
        "if [[ \" $* \" == *\" --dry-run \"* ]]; then\n"
        "  grep -qx stage2 \"$target\"\n"
        "  exit\n"
        "fi\n"
        "if [[ \" $* \" == *\" -i \"* ]]; then\n"
        "  if grep -qx stage0 \"$target\"; then\n"
        "    printf 'stage1\\n' >\"$target\"\n"
        "  elif grep -qx stage1 \"$target\"; then\n"
        "    printf 'stage2\\n' >\"$target\"\n"
        "  fi\n"
        "  exit 0\n"
        "fi\n"
        "exit 2\n"
    )
    fake_formatter.chmod(0o755)

    fixture_env = os.environ.copy()
    fixture_env["PATH"] = f"{fake_bin}:{fixture_env['PATH']}"
    fixed = subprocess.run(
        ["bash", str(fixture / "scripts/format.sh"), "--all"],
        cwd=fixture, env=fixture_env, capture_output=True, text=True,
    )
    assert fixed.returncode == 0, fixed.stdout + fixed.stderr
    assert probe.read_text() == "stage2\n", (
        "--all stopped before clang-format reached a real fixpoint"
    )
    assert "stable after 2 pass(es)" in fixed.stdout, fixed.stdout

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
assert 'format.sh" --staged' in hook, "the hook must auto-format staged lines"
assert "--no-verify" in hook, "the hook must tell the user how to bypass it"
# A missing clang-format must warn and let the commit through, not block work.
missing_branch = hook.split("command -v clang-format", 1)[1].split("fi", 1)[0]
assert "exit 0" in missing_branch, (
    "the hook must not block commits when clang-format is unavailable"
)

# Auto-fixing must update the committed index without swallowing unrelated
# unstaged edits from a partially staged source file.
if clang_format and shutil.which("git-clang-format"):
    with tempfile.TemporaryDirectory() as temp_dir:
        fixture = Path(temp_dir)
        (fixture / "scripts/git-hooks").mkdir(parents=True)
        shutil.copy2(CONFIG, fixture / ".clang-format")
        shutil.copy2(SCRIPT, fixture / "scripts/format.sh")
        shutil.copy2(HOOK, fixture / "scripts/git-hooks/pre-commit")

        subprocess.run(["git", "init", "-q"], cwd=fixture, check=True)
        subprocess.run(
            ["git", "config", "user.name", "Formatting Test"],
            cwd=fixture, check=True,
        )
        subprocess.run(
            ["git", "config", "user.email", "format@example.invalid"],
            cwd=fixture, check=True,
        )
        subprocess.run(
            ["git", "config", "core.hooksPath", "scripts/git-hooks"],
            cwd=fixture, check=True,
        )

        probe = fixture / "probe.c"
        probe.write_text("int value()\n{\n\treturn 0;\n}\n")
        subprocess.run(
            ["git", "add", ".clang-format", "probe.c"], cwd=fixture, check=True
        )
        subprocess.run(
            ["git", "commit", "--no-verify", "-qm", "base"],
            cwd=fixture, check=True,
        )

        probe.write_text("int value(){int number=1;return number;}\n")
        subprocess.run(["git", "add", "probe.c"], cwd=fixture, check=True)
        with probe.open("a") as source:
            source.write("// preserve this unstaged edit\n")

        committed = subprocess.run(
            ["git", "commit", "-m", "auto-format"],
            cwd=fixture, capture_output=True, text=True,
        )
        assert committed.returncode == 0, committed.stdout + committed.stderr
        indexed = subprocess.run(
            ["git", "show", "HEAD:probe.c"],
            cwd=fixture, capture_output=True, text=True, check=True,
        ).stdout
        assert "int value()\n{" in indexed, indexed
        assert "int number = 1;" in indexed, indexed
        assert "preserve this unstaged edit" not in indexed, indexed
        assert "preserve this unstaged edit" in probe.read_text(), (
            "the hook discarded an unstaged edit"
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
# 3c. The whole tree is formatted, and stays that way.  This is the check that
#     fails the moment someone commits unformatted C/C++, and the reason the
#     config had to be fixed: an unloadable .clang-format cannot enforce this.
# --------------------------------------------------------------------------
if clang_format:
    tree = subprocess.run(
        ["bash", str(SCRIPT), "--all", "--check"],
        cwd=ROOT, capture_output=True, text=True,
    )
    assert tree.returncode == 0, (
        "tracked C/C++ files no longer match .clang-format:\n" + tree.stdout[-4000:]
    )

    # This fence is deliberate and must stay: the format string makes
    # clang-format oscillate instead of reaching a fixpoint.
    actnew = (ROOT / "src/actnew.c").read_text()
    assert "// clang-format off" in actnew, (
        "src/actnew.c lost the fence around do_vote()'s fprintf; without it "
        "clang-format never reaches a fixpoint"
    )
    # A constant's value must never end up on a backslash continuation line.
    import glob as _glob

    stranded = []
    for path in _glob.glob(str(ROOT / "src/**/*.h"), recursive=True) + _glob.glob(
        str(ROOT / "src/**/*.c"), recursive=True
    ):
        lines = open(path, errors="replace").read().split("\n")
        for i, line in enumerate(lines[:-1]):
            if re.match(r"^#define ([A-Z_0-9]+) \\$", line) and re.match(
                r"^\s*-?\d+\s*(/\*|//|$)", lines[i + 1]
            ):
                stranded.append(f"{path}:{i + 1}")
    assert not stranded, (
        "these #defines have their value on a continuation line; fence them: "
        + ", ".join(stranded[:5])
    )

# --------------------------------------------------------------------------
# 4. Documentation and dependency declarations.
# --------------------------------------------------------------------------
assert DOC.is_file(), "docs/formatting.md is missing"
doc = DOC.read_text()
assert "scripts/format.sh" in doc, "docs/formatting.md does not document the runner"
assert "install-hooks.sh" in doc, "docs/formatting.md does not document the hook"
assert "clang-format off" in doc, "docs/formatting.md does not explain the fences"
assert "tabs" in doc and "Allman" in doc

agents = (ROOT / "AGENTS.md").read_text()
assert "scripts/format.sh" in agents, "AGENTS.md does not point at the formatter"
assert "install-hooks.sh" in agents, "AGENTS.md does not mention the pre-commit hook"

readme = (ROOT / "README.md").read_text()
assert "2 spaces" not in readme, (
    "README.md still claims 2-space indentation, contradicting .clang-format"
)
assert "clang-format" in readme and "docs/formatting.md" in readme

equivs = (ROOT / "packaging/duris-build-deps.equivs").read_text()
depends = equivs.split("Depends:", 1)[1].split("Suggests:", 1)[0]
assert "clang-format" in depends, "clang-format is not a declared build dependency"

print("formatting tooling contracts OK")
