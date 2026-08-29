#!/usr/bin/env python3
"""Contract for bounded appends to fixed-size buffers.

The codebase carried 115 instances of

    snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), ...)

against buffers as small as 50 bytes. glibc's __snprintf_chk aborts the process
as soon as the claimed size exceeds the destination size it can determine - it
does not wait for the output to actually be long - so each was a dormant

    *** buffer overflow detected ***: terminated

that a rebuild could wake. One of them (prompt.c) did wake, and killed the
server on every login. Where the call was checked_snprintf() rather than
snprintf() it was worse: no abort, just a memcpy of up to the claimed size into
the smaller buffer.

They are now APPENDF(), which deduces the capacity from the array type so it
cannot be stated wrongly.

A second shape of the same defect survived that cleanup, because the scanner
only matched a subtracted size:

    snprintf(buf + strlen(buf) - 2, MAX_STRING_LENGTH, ".\n");

The destination is advanced into the buffer while the size argument still claims
the whole thing. This is wrong by construction - the room left at the offset is
strictly less than the full capacity - and it is what took the server down
through the `deathsdoor` command (specs.gellz.c). The scanner now catches both
shapes; 20 further instances turned up when it did.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
header = (root / "src/safe_format.h").read_text()
impl = (root / "src/safe_format.c").read_text()
prompt = (root / "src/prompt.c").read_text()

failures = []


def check(name, ok):
    print(("[PASS] " if ok else "[FAIL] ") + name)
    if not ok:
        failures.append(name)


# The helper and its compile-time capacity guard.
check("checked_appendf is declared with printf format checking",
      "int checked_appendf(char *buffer, size_t capacity, const char *format, ...)" in header
      and "__attribute__((format(printf, 3, 4)))" in header)
check("capacity is deduced from the array type",
      "template <size_t N>" in header
      and "constexpr size_t duris_buffer_capacity(const char (&)[N])" in header)
check("APPENDF supplies the deduced capacity",
      "#define APPENDF(buffer, ...) checked_appendf((buffer), duris_buffer_capacity(buffer), __VA_ARGS__)"
      in header)
check("checked_appendf appends within capacity",
      "strnlen(buffer, capacity)" in impl
      and "vsnprintf(buffer + used, capacity - used, format, args)" in impl)
check("checked_appendf leaves a full buffer alone",
      "if (used + 1 >= capacity)" in impl)

# prompt.c uses the shared helper rather than a private copy of it.
check("prompt.c has no private duplicate of the helper", "prompt_appendf" not in prompt)
check("prompt.c appends through the shared helper",
      "APPENDF(promptbuf," in prompt and "checked_appendf(pPrompt, pPromptCap," in prompt)

# Repo-wide: no append may claim more room than its buffer has.
scan = subprocess.run([sys.executable, str(root / "scripts/scan-append-bounds.py")],
                      cwd=root, capture_output=True, text=True)
offenders = [l for l in scan.stdout.splitlines() if l.startswith("src/")]
check("no append claims more capacity than its buffer holds", not offenders)
for o in offenders:
    print("       " + o)

# The scanner must keep catching the offset-destination shape, not just the
# subtracted-size one. Feed it the deathsdoor defect and require a report.
with tempfile.TemporaryDirectory() as tmp:
    fixture = Path(tmp) / "src"
    fixture.mkdir()
    (fixture / "probe.c").write_text(
        "void probe(void)\n"
        "{\n"
        "\tchar buf[MAX_STRING_LENGTH];\n"
        "\tsnprintf(buf, MAX_STRING_LENGTH, \"x\");\n"
        "\tsnprintf(buf + strlen(buf) - 2, MAX_STRING_LENGTH, \".\\n\");\n"
        "}\n")
    probe = subprocess.run([sys.executable, str(root / "scripts/scan-append-bounds.py")],
                           cwd=tmp, capture_output=True, text=True)
    check("scanner reports an offset destination that claims the whole buffer",
          "probe.c" in probe.stdout)

# The specific site that aborted the server: deathsdoor must size its closing
# write by the room actually left, and must not back over its own header.
gellz = (root / "src/specs.gellz.c").read_text()
door = gellz[gellz.index("void do_deaths_door("):]
door = door[:door.index("\n\tif (!*arg)")]
check("deathsdoor sizes its closing write by the remaining room",
      "snprintf(buf + strlen(buf) - 2, MAX_STRING_LENGTH," not in door
      and "MAX_STRING_LENGTH - length" in door)
check("deathsdoor only trims a separator it actually wrote",
      "header_length" in door and "if (length > header_length)" in door)

# Two functions take a caller-owned char* whose size only the caller knows.
# They must keep saying so, or a future reader will "fix" them wrongly.
for path, fn in (("src/actwiz.c", "concat_which_flagsde"),
                 ("src/actinf.c", "get_equipment_list")):
    text = (root / path).read_text()
    idx = text.index(fn + "(")
    preamble = text[max(0, idx - 200):idx]
    check("%s documents that its buffer is caller-owned" % fn,
          "caller-owned" in preamble and "MAX_STRING_LENGTH" in preamble)

if failures:
    print("\nFailed regression checks:")
    for f in failures:
        print("- " + f)
    raise SystemExit(1)
print("append bounds contract passed")
