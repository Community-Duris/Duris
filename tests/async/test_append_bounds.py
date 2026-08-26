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
"""
from pathlib import Path
import re
import subprocess
import sys

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
