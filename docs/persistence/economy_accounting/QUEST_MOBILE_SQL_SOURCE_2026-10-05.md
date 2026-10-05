# NPC native image SQL participant: source integration — 2026-10-05

The reviewed `quest_mobile_native_sql.c`461ecd26 and header5977e354 are
integrated in src/persistence and registered in the maintained Makefile.
The missing native NPC image/stock participant now supplies prepared binary
reads, exact retained-before DML and exact canonical after readback inside the
caller's original reconnect-disabled transaction. It never starts, commits,
rolls back or replaces a transaction, issues IDs, touches the world or ACKs.

Native columns and the complete canonical image must agree. NULL, truncated,
oversized, duplicate and noncanonical reads refuse. Existing birth identity,
source, provenance and location remain immutable; retired values cannot revive.
A missing-row birth requires the original parent to equal the image birth
operation. These are value checks, not authentication of an admitted birth.
The original parent still proves source/epoch/inbox/exclusion, typed revision
transitions and ascending mobile-ID locks before item custody. Errors after
DML require that parent's rollback or uncertain-session retirement. Outputs
remain unchanged on refusal, which does not imply native writes rolled back.

This definition has no in-tree producer caller and is recorded as dormant in
the writer registry/matrix. SQL/MariaDB execution is unqualified; flat mode
returns ENOTSUP until the same-bundle participant is implemented. Additive
schema0059 remains private pending the coherent0057/0058/0059 migration and
major-plan qualification batch; no manifests or metadata fingerprints are
relabeled, existing databases and inactive gameplay are untouched.

Independent persistence source review accepted the exact component hashes.
Formatter fixed point, whitespace, source pins and source census pass. No
compiler, tests, database, runtime or production activation was executed.
Historical e018 native build proof does not qualify this extended candidate.
Native birth/rebind/lifecycle, unified custody/flat bundle, sequential quest and
independent reward recipient, publication/recovery and guarded ACK remain open.
