# Auction source-claim qualification — 2026-10-06

Successful auction operations now record exactly one authenticated source claim
inside the caller's existing transaction. The five typed leaf owners previously
did not consistently establish this proof; retained reads and retired escrow/item
qualification could accept historical operations without it. The shared helper
checks original canonical intent, operation metadata and source identity. Duplicate
claims return `EEXIST`; missing or altered retained claims refuse. Rejected
operations require zero claims. Retired custody checks require the same historical
proof. Retained verification remains read-only and never repairs missing rows.

The nine source files use the existing schema and borrowed transaction, enforce
reconnect-disabled/same-session checks, and preserve current inactive behavior and
admission gates. The separately committed fault fixture (`2ab76dd51`) exercises the
actual append-only consumption INSERT and whole-transaction rollback.

## Validation

- All 18 original native cases pass: nine on each fresh canonical0062 MySQL and
  MariaDB engine, with the authenticated additive source-claim controls. Both
  complete migration/runtime verifiers and canonical money-recovery audits pass.
- Five leaf controls prove success-one/rejection-zero, duplicate refusal and full
  rollback, deleted retained claim refusal and orphan foreign-key refusal. All
  seven strict native profiles and the original flat unit pass. The primary
  independently authenticated all 175 sealed native artifacts.
- Both original production builds pass, each with 740 fresh compiles and a fresh
  link: MariaDB 666.99 seconds; flatfile 541.82 seconds. Nine source contracts pass.
  The primary independently authenticated 25 artifacts, all 6,339 frozen source
  members, 1,482 cache members per backend and the actual compile/link arguments.
- Required changed-line clang-format18 checks pass for all nine source files.
  Unrelated SHOP fixture and Plan5 report changes retain their original hashes.

The exact qualified source archive is
`743d6aafa75d66d9ff64bf493f06dc638c11c3754949a0e8667a8417677132fe`;
its manifest is `6022a35007ae32cc9862f349ea2ef69be525daaeec446d59bc5cd1434879140c`.
The companion JSON records every source preimage/new hash and both primary
authentication receipts. Evidence is retained locally under
`bin/tests/auction-maintained-native-primary-20261006/combined-once-1` and
`bin/tests/auction-maintained-production-primary-20261006`; raw artifacts remain
uncommitted. Git's existing CRLF-to-LF fixture filter changes only representation.

## Remaining qualification

This closes the source-claim component. Full auction native/save publication,
passive replay and guarded ACK, complete per-UID literals, cold world restoration
and activation remain open. The separate private750-provider startup candidate
passes its builds and warm MySQL publication, but its genuine cold recovery refuses;
that failure does not qualify recovery. Writer coverage remains incomplete and
release remains BLOCKED. No whole Plan or R1–R8 completion is claimed.
