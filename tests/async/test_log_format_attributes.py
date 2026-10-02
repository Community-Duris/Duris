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
    (prototypes, "void debug(const char *format,...)", 1, 2),
    (prototypes, "void debug(const char *,...)", 1, 2),
    (prototypes, "void logexp(const char *,...)", 1, 2),
    (prototypes, "void ereglog(int level,const char *format,...)", 2, 3),
    (prototypes, "void loginlog(int,const char *,...)", 2, 3),
    (prototypes, "void statuslog(int,const char *,...)", 2, 3),
    (prototypes, "void banlog(int,const char *,...)", 2, 3),
    (prototypes, "void epiclog(int,const char *,...)", 2, 3),
    (prototypes, "void wizlog(int level,const char *,...)", 2, 3),
    (prototypes, "void logit(const char *,const char *,...)", 2, 3),
    (utility, "void logit(const char *,const char *,...)", 2, 3),
    (sql, "void sql_log(P_char ch,const char *kind,const char *format,...)", 3, 4),
]

for source, declaration, fmt_index, first_arg in EXPECTED:
    annotated = f"{declaration}__attribute__((format(printf,{fmt_index},{first_arg})))"
    assert contains(source, annotated), f"missing printf format attribute on: {declaration}"

# The declarations in prototypes.h and utility.h must agree, or one translation
# unit would lose the checking the other has.
assert contains(prototypes, "void logit(const char *,const char *,...)__attribute__")
assert contains(utility, "void logit(const char *,const char *,...)__attribute__")

print(f"printf format attributes present on {len(EXPECTED)} logging declarations")
