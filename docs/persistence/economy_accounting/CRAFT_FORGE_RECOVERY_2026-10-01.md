# Craft and Forge interruption recovery - 2026-10-01

This qualification follows the normal master integration in PR #674, merged as
`ffb73e6518a2dc39f3d243d3384ea851b4c3b8a4`. It preserves the upstream save,
recipe bounds, migration, and lifecycle fixes, and subsequent enhancement
payment/prototype guards through `5017a0e9e`.

## Observed failures and repairs

1. **Disconnect before item publication could quarantine later player saves.**
   A terminal inventory capture could freeze the old graph while the native
   craft transaction established new custody. The snapshot authority guard
   correctly refused the stale graph, leaving progression unable to save.
   Pending crafts now hold inventory capture until their committed graph has
   been published. Progression can then request its save while the operation
   and entity fences remain retained. The authority guard stays in force.
2. **Legacy recipe replay could silently skip progression.** Schema-1 journal
   envelopes have no publication bit. Startup formerly restored neither the
   movement publication nor its progression obligation, then checkpointed a
   successfully replayed native item commit. The coordinator and movement
   restore now use one decoder for the existing durable recipe continuation.
   Invalid legacy item payloads reject replay. Schema-1 bytes remain unchanged;
   schema-2 publication retention continues to use its existing envelope bit.
3. **A failed publication checkpoint left completed progression attempts in
   memory.** Retrying that ACK could repeat the command callback and omit the
   progression owner's cleanup notification. Retries now notify once and
   perform cleanup after a successful durable checkpoint, including when the
   already-notified player is absent. This prevents acknowledged entries from
   accumulating toward the bounded progression queue limit.
4. **An obsolete player frame could quarantine a valid character after a
   later craft save.** Receipt verification compared the award's application
   revision only with the old frame revision. It now accepts completion bounded
   by the locked durable player revision when retiring an obsolete frame,
   without overwriting newer XP. Frozen terms and successful native root
   evidence must still match; future and mismatched receipts remain rejected.

## Verification

The SQL cases use disposable MySQL 8.0.46 and MariaDB 10.11.19 databases,
unprivileged server processes, and the frozen mortal leather recipe fixture.
Accounting activation remains disabled. Each physical Craft and Forge checks
exact material/tool UID retirement, one fresh output UID, one receipt, and the
frozen 4,000 XP award. Each journey then exercises ordinary retained-pouch
Craft/Forge, copyover, and two cold restarts, comparing output UIDs, counters,
and durable XP.

| Integrated case | MariaDB 10.11.19 | MySQL 8.0.46 |
| --- | --- | --- |
| Disconnect while native craft waits for its owner row | Passed | Passed |
| Process crash with a complete durable root, before native commit | Passed | Passed |
| Process crash after native commit, before progression save | Passed | Passed |
| Progression save committed, publication checkpoint refused, then process crash | Passed | Passed |

The before-commit fixture holds the owner row, validates the complete retained
item command frame and checksum, then fsyncs it before injecting the process
crash. This establishes the durable recovery-root boundary independently of
engine diagnostic tables. The reservation message alone does not establish
journal durability; the coordinator's own fsync-before-execution order has
separate native admission coverage.
The before-save fixture holds a shared player row lock: native craft FK checks
can complete, while the later player update waits. The lost-ACK fixture denies
checkpoint creation, proves the saved progression receipt is applied while the
critical journal frame stays unchanged, then crashes and replays it.

Both strict production build profiles pass. All 1,210 tracked `src/` files used
by the latest builds match committed tree `8b2693dc4` after newline
normalization. The MariaDB fault matrix uses the frozen `0408d0825` candidate;
MySQL uses the integrated `256785124` candidate. Both contain the same recovery
repairs. Subsequent native changes to temporary material-probe cleanup,
enhancement level/payment/modifier/prototype gates, and initialized flatfile
UID authority have separate
focused regression evidence and both production-build profiles pass after
integration. The SQL fault candidates remain pinned; this report does not
claim that later binaries reran those SQL journeys.
The flatfile physical/pouch control on `70ff83d74` passes copyover and
two cold restarts; it does not qualify flatfile interruption modes.

The actual movement/coordinator/journal regression covers the save-capture
gate, checkpoint failure with repeated retries, notification and cleanup with
an absent player, unchanged schema-1 codec roundtrip, malformed/unrelated
payload classification, and two cold replays including an already-applied
native commit. It passes with ordinary compilation and ASan/UBSan. The actual
SQL repository regression reproduces obsolete receipt rejection before the
repair and verifies that replay preserves newer XP and refuses mismatched or
future evidence. Its real player-journal replay retires the obsolete frame
without quarantining the character. Existing
coordinator admission/replay/uncertain-journal, progression, planner, restore,
save pipeline, prompt/input queue, spell publication, and lifecycle checks pass.

Reproduce the integrated SQL matrix with the existing runner, against an
explicitly disposable loopback database, as a non-root POSIX user:

```sh
export TEST_DB_HOST=127.0.0.1 TEST_DB_USER=root TEST_DB_DISPOSABLE=1
# Supply the disposable TEST_DB_PASSWORD and TEST_DB_PORT for the chosen engine.
for fault in disconnect crash-before-commit crash-before-save lost-ack; do
  python3 tests/async/run_alchemist_crafting_journey.py \
    bin/server/dms_new redis --recipe-fault="$fault"
done
```

Run one journey at a time per SQL server so its boot guard has a single owner.
Build the selected binary before starting a journey. The runner creates and
drops only its private `alchemy_test_*` schema and temporary world.

## Remaining qualification

This evidence is for the fixture's physical input faults and healthy retained
pouch path. It does not qualify every recipe, interruption during pouch
mutation, active accounting posting/reconciliation, or flatfile fault parity.
The writer census has 2,811 occurrences, 2,753 unique sites, 863 routes,
and zero unmapped sites. All 750 runtime/projection routes retain their existing
release qualification gaps. No production migration, repair, activation, or
deployment is performed by this work.

The next SQL qualification target is the coupled death/resurrection path.
The current inactive-accounting journey preserves original assets but does
not establish linked accounting roots/postings. Active-epoch qualification
must include original item UIDs and exact wallet value through interruption
and restart. Captured-target migration/restore and rollback rehearsal remain
separate release gates.
