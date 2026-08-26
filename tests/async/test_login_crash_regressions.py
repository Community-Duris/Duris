#!/usr/bin/env python3
"""Regressions for the three defects that made every login abort the server.

All three predate the warning cleanup but only became reachable once a clean
rebuild let _FORTIFY_SOURCE=3 resolve object sizes at the call sites. Symptom
in logs/duris-console.log was:

    *** buffer overflow detected ***: terminated
    Mud stopped, reason: unknown [134]

1. comm.c new_descriptor() and websocket.c stripped the "::ffff:" prefix from an
   IPv4-mapped peer address with strcpy(host, host + 7). Source and destination
   overlap, which is undefined; ASan reported strcpy-param-overlap. Every IPv4
   client arrives mapped, so this ran on every single connection.

2. prompt.c make_prompt() appended each prompt element with
   snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), ...) while its
   buffers are MAX_INPUT_LENGTH. glibc's __snprintf_chk aborts when the claimed
   size exceeds the real object size, so the first prompt sent to a player - the
   moment they enter the world - killed the process. This was one instance of a
   115-site class; see tests/async/test_append_bounds.py.

3. make_bar() had the same mismatch against a 512-byte static buffer, and
   divided by `max` without guarding zero.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
comm = (root / "src/comm.c").read_text()
websocket = (root / "src/websocket.c").read_text()
prompt = (root / "src/prompt.c").read_text()

failures = []


def check(name, ok):
    print(("[PASS] " if ok else "[FAIL] ") + name)
    if not ok:
        failures.append(name)


# 1. No self-overlapping copy on either accept path.
for name, text in (("comm.c", comm), ("websocket.c", websocket)):
    live = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    check("%s strips the mapped-IPv4 prefix with memmove" % name,
          "memmove(" in live and re.search(r'host,\s*mapped,\s*strlen\(mapped\) \+ 1', live) is not None)
    check("%s has no self-overlapping strcpy" % name,
          re.search(r'strcpy\(\s*(\w+(?:->\w+)?)\s*,\s*\1\s*\+', live) is None)

# 2/3. prompt.c appends through one bounded helper, with no open-coded mismatch.
# The helper this fix introduced now lives in safe_format.h and is shared with
# the other 115 sites; test_append_bounds.py covers the helper itself.
check("prompt.c appends through the shared bounded helper",
      '#include "safe_format.h"' in prompt
      and "APPENDF(promptbuf," in prompt
      and "checked_appendf(pPrompt, pPromptCap," in prompt)
check("prompt.c open-codes no append of its own",
      re.search(r'snprintf\(\s*(?:promptbuf2?|pPrompt)\s*\+', prompt) is None)
check("prompt.c no longer claims MAX_STRING_LENGTH on a smaller buffer",
      "MAX_STRING_LENGTH - strlen(" not in prompt)
check("pPrompt carries the capacity of the buffer it aliases",
      "size_t pPromptCap;" in prompt and
      "pPromptCap = sizeof(promptbuf2) - strlen(promptbuf2);" in prompt and
      "pPromptCap = sizeof(promptbuf) - strlen(promptbuf);" in prompt)

bar = prompt[prompt.index("char *make_bar("):]
bar = bar[:bar.index("\n}\n")]
check("make_bar appends with bounded strlcat", "strlcat(buf," in bar and "snprintf" not in bar)
check("make_bar guards division by zero", "(max > 0)" in bar)

# The buffers make_prompt writes into must stay consistent with the helper's use.
check("make_prompt still sizes its buffers from MAX_INPUT_LENGTH",
      "char promptbuf[MAX_INPUT_LENGTH];" in prompt and
      "char promptbuf2[MAX_INPUT_LENGTH], *pPrompt;" in prompt)

# The three fixed files must stay clean under the repo-wide scanner.
import subprocess, sys
out = subprocess.run([sys.executable, str(root / "scripts/scan-append-bounds.py")],
                     cwd=root, capture_output=True, text=True).stdout
offenders = [l for l in out.splitlines()
             if l.startswith(("src/prompt.c", "src/comm.c", "src/websocket.c"))]
check("no mismatched append bounds remain in prompt.c, comm.c or websocket.c", not offenders)
for o in offenders:
    print("       " + o)

if failures:
    print("\nFailed regression checks:")
    for f in failures:
        print("- " + f)
    raise SystemExit(1)
print("login crash regression contract passed")
