#!/usr/bin/env python3
"""Every variadic logging entry point stays printf-format-checked.

Without __attribute__((format(printf, ...))) the compiler cannot check a
single logit()/debug()/wizlog()/sql_log() call site, so mismatched
specifiers and player-controlled format strings compile silently under
-Wformat=2 -Werror. These annotations are what makes that class of defect
a build failure, so they are contractual.
"""
from _paths import SRC
from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
prototypes = (SRC / "prototypes.h").read_text()
utility = (SRC / "utility.h").read_text()
sql = (SRC / "sql.h").read_text()

# (declaration, format-index, first-vararg-index)
EXPECTED = [
    (prototypes, "voiddebug(constchar*format,...)", 1, 2),
    (prototypes, "voiddebug(constchar*,...)", 1, 2),
    (prototypes, "voidlogexp(constchar*,...)", 1, 2),
    (prototypes, "voidereglog(intlevel,constchar*format,...)", 2, 3),
    (prototypes, "voidloginlog(int,constchar*,...)", 2, 3),
    (prototypes, "voidstatuslog(int,constchar*,...)", 2, 3),
    (prototypes, "voidbanlog(int,constchar*,...)", 2, 3),
    (prototypes, "voidepiclog(int,constchar*,...)", 2, 3),
    (prototypes, "voidwizlog(intlevel,constchar*,...)", 2, 3),
    (prototypes, "voidlogit(constchar*,constchar*,...)", 2, 3),
    (utility, "voidlogit(constchar*,constchar*,...)", 2, 3),
    (sql, "voidsql_log(P_charch,constchar*kind,constchar*format,...)", 3, 4),
]

for source, declaration, fmt_index, first_arg in EXPECTED:
    annotated = f"{declaration}__attribute__((format(printf,{fmt_index},{first_arg})))"
    assert contains(source, annotated), f"missing printf format attribute on: {declaration}"

# The declarations in prototypes.h and utility.h must agree, or one translation
# unit would lose the checking the other has.
assert contains(prototypes, "voidlogit(constchar*,constchar*,...)__attribute__")
assert contains(utility, "voidlogit(constchar*,constchar*,...)__attribute__")

print(f"printf format attributes present on {len(EXPECTED)} logging declarations")
