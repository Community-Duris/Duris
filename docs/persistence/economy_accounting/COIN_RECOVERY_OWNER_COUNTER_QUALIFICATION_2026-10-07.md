# Ordinary coin recovery owner counters — 2026-10-07

The retained ordinary-coin publisher checked both endpoint owner revisions but
only hydrated the destination item. A cold cache after complete pickup could
therefore retain no room counter; successful drops and rejected drops could
retain stale system/room counters. The cache's exact-item comparison also omitted
owner_revision, allowing a physically matching entry to keep its old counter.

`src/economy/coin_physical_recovery.c` now projects both endpoint revisions from
the same already-locked native state on successful publication and rejected-drop
repair. Exact runtime identity includes owner_revision. Current-cut, conflict,
literal/census and uncertain-effect refusal checks remain in place before repair.
No new durable effect, ACK, UID, wallet credit, source claim or admission is added.
Inactive paths, generic loading, closed flat coin admission and the declined spell
change are preserved.

## Evidence and scope

The pinned offline QA image4994cc50 runs actual source-extracted projection and
runtime cache primitives with strict C++20 warnings-as-errors, ASan/UBSan, original
300-second compile and30-second case bounds. Original source fails three cases:
consumed_cold_cache, drop_existing_stale_counters and rejected_drop_stale_counters;
seven unchanged controls pass. Corrected private source passes all ten. Primary
authenticates565 source/header inputs, both actual ELF exports and all20 results.

The maintained `tests/async/test_coin_recovery_owner_revisions.py` independently
extracts current source at execution and passes the same ten cases. It includes
same-owner partial pickup and six no-effects refusals for conflicting counters,
conflicting literals, lost authority and uncertain native effects. Primary checks
the actual harness/ELF/results and all source/header pins. Native/world authority,
capture/body and inert-enrollment seams are explicit; this does not establish
real SQL/flat proof acquisition, physical enrollment, ACK, gameplay or cold boot.
The complete corrected SQL translation unit also passes strict syntax checking.
Central native regression registration and lexical writer locations are updated;
backend route statuses and release readiness remain unqualified.

Private evidence: `tmp/ordinary-coin-cold-owner-primary-20261007/evidence/`;
maintained evidence: `bin/tests/coin-recovery-owner-revisions-primary-20261007/`.

Maintained result SHA-256`8f917de2a6471a8b83f90e48df7e11f817a8981a8b041501de417380b43f28ca`; actual ELF`c6989bc987b8f9400c233bbf8fff985a8b87a5e93e916b82d52617b231ee0613`.

## Remaining work

Plan2 still needs the complete original native/gameplay/persistence/recovery
qualification. Post-ACK SQL room coins require typed cold restoration from native
current coin payload/custody and genuine accounting history; the existing generic
room loader intentionally excludes money and cannot recover this route. The saved
item overlap must also avoid restoring a stale acknowledged/consumed pile. This is
a separate integration issue; this counter regression is not proof that boot works.
