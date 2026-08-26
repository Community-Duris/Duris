#!/usr/bin/env python3
"""Contract for the build's warning profile.

The project removed every flag that used to live in LEGACY_WARNING_EXCEPTIONS.
This pins that outcome so a reintroduced suppression fails a test rather than
quietly restoring 9,947 hidden diagnostics.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
makefile = (root / "src/Makefile").read_text()
inventory = (root / "scripts/warning-inventory.sh").read_text()

failures = []


def check(name, ok):
    print(("[PASS] " if ok else "[FAIL] ") + name)
    if not ok:
        failures.append(name)


CATEGORIES = [
    "write-strings",
    "unused-parameter",
    "unused-variable",
    "unused-but-set-variable",
    "missing-field-initializers",
    "unused-function",
]

# The six former exceptions are enabled, not merely un-suppressed.
for cat in CATEGORIES:
    check("-W%s is enabled" % cat, ("-W" + cat) in makefile)
    check("-Wno-%s is gone" % cat, ("-Wno-" + cat) not in makefile)

check("no -Wno- suppression anywhere in the Makefile",
      not re.search(r"-Wno-[a-z0-9-]+", makefile))
check("LEGACY_WARNING_EXCEPTIONS is gone", "LEGACY_WARNING_EXCEPTIONS" not in makefile)
check("-Werror is still on", "-Werror" in makefile)

# The build must not carry file-wide suppressions either.
src = root / "src"
pragma_offenders = []
for path in list(src.rglob("*.c")) + list(src.rglob("*.h")) + list(src.rglob("*.cpp")):
    text = path.read_text(errors="replace")
    for m in re.finditer(r'#\s*pragma\s+GCC\s+diagnostic\s+ignored\s+"(-W[a-z0-9-]+)"', text):
        if m.group(1)[3:] in CATEGORIES or m.group(1)[5:] in CATEGORIES:
            pragma_offenders.append("%s: %s" % (path.relative_to(root), m.group(1)))
check("no pragma suppresses a former exception category", not pragma_offenders)
for o in pragma_offenders:
    print("       " + o)

# The inventory helper must keep reporting on all six regardless of the Makefile.
for cat in CATEGORIES:
    check("inventory enables %s" % cat, cat in inventory)
check("inventory clears LEGACY_WARNING_EXCEPTIONS",
      "LEGACY_WARNING_EXCEPTIONS=" in inventory)
check("inventory drops -Werror so a dirty build still reports",
      "s/-Werror//" in inventory)
check("inventory asks for byte-accurate warning columns",
      "-fdiagnostics-column-unit=byte" in inventory)

# The sanitizer build must stay isolated from the runtime binary.
san = (root / "scripts/build-san.sh").read_text()
check("sanitizer build does not overwrite the runtime dms binary",
      "cp dms_new ../dms" not in san and "dms_san" in san)
check("sanitizer build keeps its objects separate", "OBJDIR=../obj-san" in san)
check("sanitizer build appends to the warning profile rather than replacing it",
      "EXTRA_CFLAGS=" in san and 'export CFLAGS=' not in san)
check("Makefile honours EXTRA_CFLAGS/EXTRA_LDFLAGS",
      "CFLAGS += $(EXTRA_CFLAGS)" in makefile and "LDFLAGS += $(EXTRA_LDFLAGS)" in makefile)

if failures:
    print("\nFailed regression checks:")
    for f in failures:
        print("- " + f)
    raise SystemExit(1)
print("compiler warning profile contract passed")
