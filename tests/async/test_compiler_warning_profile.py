#!/usr/bin/env python3
"""Contract for the build's warning profile.

The project removed every flag that used to live in LEGACY_WARNING_EXCEPTIONS.
This pins that outcome so a reintroduced suppression fails a test rather than
quietly restoring 9,947 hidden diagnostics.
"""
from _paths import SRC
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
makefile = (SRC / "Makefile").read_text()
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
src = SRC
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
check("sanitizer build keeps its objects separate",
      "OBJDIR=../bin/objects/server-san" in san)
check("sanitizer build appends to the warning profile rather than replacing it",
      "EXTRA_CFLAGS=" in san and 'export CFLAGS=' not in san)
check("Makefile honours EXTRA_CFLAGS/EXTRA_LDFLAGS",
      "CFLAGS += $(EXTRA_CFLAGS)" in makefile and "LDFLAGS += $(EXTRA_LDFLAGS)" in makefile)

# Repository API headers must use the client library's public MYSQL declaration.
# MySQL 8 and MariaDB intentionally expose different internal struct tags, so a
# local forward declaration can compile on one engine and conflict on the other.
repository_headers = [
    "artifact_guild_repository.h",
    "auction_repository.h",
    "boon_reward_repository.h",
    "combat_outcome_repository.h",
    "critical_command_repository.h",
    "item_transfer_repository.h",
    "item_uid_allocator.h",
    "player_load_repository.h",
    "player_snapshot_repository.h",
    "session_audit_repository.h",
    "zone_touch_repository.h",
]
for name in repository_headers:
    header = (src / name).read_text()
    check(f"{name} uses the public MYSQL header", "#include <mysql/mysql.h>" in header)
    check(f"{name} does not guess the MYSQL struct tag",
          "typedef struct st_mysql MYSQL" not in header)

# MYSQL 8 uses bool for statement null indicators while MariaDB retains
# my_bool. Derive the type from MYSQL_BIND so both client libraries compile.
item_transfer_repository = (src / "item_transfer_repository.c").read_text()
check("statement null indicator follows the installed client library",
      "std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>" in item_transfer_repository)
check("statement null indicator does not name MariaDB-only my_bool",
      not re.search(r"\bmy_bool\b", item_transfer_repository))

# MYSQL_OPT_SSL_ENFORCE and MYSQL_OPT_SSL_VERIFY_SERVER_CERT were removed in
# MySQL 8; MYSQL_OPT_SSL_MODE replaces them and MariaDB Connector/C does not
# ship it. Both spellings are enum values rather than macros, so neither can be
# probed with #ifdef -- the arms have to be selected on the client library. The
# MySQL arm must ask for SSL_MODE_VERIFY_IDENTITY: the weaker modes would drop
# CA or hostname verification that the MariaDB arm performs.
sql_c = (src / "sql.c").read_text()
check("remote TLS options are selected per client library",
      "#if defined(MARIADB_BASE_VERSION) || defined(MARIADB_PACKAGE_VERSION)" in sql_c)
check("MariaDB arm keeps enforcement and server certificate verification",
      "MYSQL_OPT_SSL_ENFORCE" in sql_c and "MYSQL_OPT_SSL_VERIFY_SERVER_CERT" in sql_c)
check("MySQL arm uses the replacement option",
      "MYSQL_OPT_SSL_MODE" in sql_c)
check("MySQL arm does not weaken verification",
      "SSL_MODE_VERIFY_IDENTITY" in sql_c and
      not re.search(r"SSL_MODE_(DISABLED|PREFERRED|REQUIRED)\b", sql_c))
mysql_arm = re.search(
    r"#if defined\(MARIADB_BASE_VERSION\).*?\n#else\n(.*?)\n#endif", sql_c, re.S)
check("the client-library guard has a MySQL arm", mysql_arm is not None)
if mysql_arm:
    check("MySQL arm does not name options MySQL 8 removed",
          "MYSQL_OPT_SSL_ENFORCE" not in mysql_arm.group(1) and
          "MYSQL_OPT_SSL_VERIFY_SERVER_CERT" not in mysql_arm.group(1))

if failures:
    print("\nFailed regression checks:")
    for f in failures:
        print("- " + f)
    raise SystemExit(1)
print("compiler warning profile contract passed")
