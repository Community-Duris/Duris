"""Report appends that claim more room than their buffer has.

Finds `snprintf(BUF + strlen(BUF), CAP - strlen(BUF), ...)` where BUF is declared
smaller than CAP.  glibc's __snprintf_chk aborts the process outright when the
claimed size exceeds the object size it can determine - it does not wait for the
output to actually be long - so each of these is a latent

    *** buffer overflow detected ***: terminated

that fires as soon as the compiler can resolve the destination's size at that
call site.  _FORTIFY_SOURCE=3 resolves far more of them than level 2 did, so a
rebuild can turn a dormant one live.

This is how the prompt.c crash was found; see
tests/async/test_login_crash_regressions.py.  Run from the repository root.
"""
import re, sys
from pathlib import Path

SIZES = {"MAX_STRING_LENGTH": 65536, "MAX_INPUT_LENGTH": 1024}
def enclosing_function(text, pos):
    """Body of the function containing pos, found from the last definition-like
    line that starts at column 0 before it."""
    starts = [m.start() for m in
              re.finditer(r'^[A-Za-z_][A-Za-z0-9_ \t*:&<>,]*\([^;]*?\)\s*\n?\s*\{', text, re.M)
              if m.start() < pos]
    return text[starts[-1]:pos] if starts else text[:pos]


root = Path("src")
hits = []

for path in sorted(list(root.rglob("*.c")) + list(root.rglob("*.h"))):
    text = path.read_text(errors="replace")
    # local char array declarations: char NAME[SIZE]
    decls = {}
    for m in re.finditer(r'\bchar\s+(\w+)\s*\[\s*([A-Za-z0-9_]+)\s*\]', text):
        name, size = m.group(1), m.group(2)
        val = SIZES.get(size)
        if val is None and size.isdigit():
            val = int(size)
        if val is not None:
            decls.setdefault(name, set()).add(val)
    for m in re.finditer(
            r'snprintf\(\s*(\w+)\s*\+\s*strlen\(\s*\1\s*\)\s*,\s*([A-Za-z0-9_]+)\s*-\s*strlen\(\s*\1\s*\)',
            text, re.S):
        buf, cap = m.group(1), m.group(2)
        capval = SIZES.get(cap)
        if capval is None:
            continue
        sizes = decls.get(buf)
        if not sizes:
            continue
        # Only trust a size when the buffer is declared as an array inside the
        # enclosing function.  A same-named array elsewhere in the file says
        # nothing about a `char *buf` parameter, whose size only the caller
        # knows -- reporting those produced false positives.
        fn = enclosing_function(text, m.start())
        local = re.search(r'\bchar\s+[^;\n()]*?\b' + re.escape(buf) + r'\s*\[\s*([A-Za-z0-9_]+)\s*\]', fn)
        if not local:
            continue
        declared = SIZES.get(local.group(1))
        if declared is None and local.group(1).isdigit():
            declared = int(local.group(1))
        if declared is None or declared >= capval:
            continue
        line = text.count("\n", 0, m.start()) + 1
        hits.append((str(path), line, buf, declared, cap, capval))

if hits:
    print("%-26s %6s  %-14s %-9s %s" % ("file", "line", "buffer", "declared", "claimed"))
    for f, l, b, sz, cap, cv in hits:
        print("%-26s %6d  %-14s %-9d %s (%d)" % (f, l, b, sz, cap, cv))
    print("\n%d mismatched bound(s)" % len(hits))
else:
    print("no mismatched snprintf-append bounds found")
