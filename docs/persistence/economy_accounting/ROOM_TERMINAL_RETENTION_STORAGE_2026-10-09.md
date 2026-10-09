# ROOM original terminal BODY retention and bounded authority commit

## Result and scope

Private flat ROOM publication storage now has a terminal BODY writer paired with
its authenticated reader. It accepts the original terminal envelope, genuine root
UID and same borrowed root lock. Complete original terminal validation precedes
recovery; every retry recovers the actual authority journal before reading or
staging a new after-image. The full immutable birth success, original command and
receipt are authenticated before the exact terminal BODY can be retained.

An already retained identical BODY succeeds idempotently. A conflicting,
malformed or inaccessible retained record refuses. A missing origin is not a
successful absence. A new record uses one domains write to
`room_reset_terminal_<root_uid>.zrt`; its bytes are the original canonical recovery
BODY, with no synthetic envelope, revision, generation, delivery or ACK.

The writer uses a genuine prospective authority-commit companion. That companion
preserves the original lock, pending-journal refusal, complete canonical encoding,
duplicate and store checks, atomic publication, ordered apply, fault cuts and
unlink/sync semantics. Public callers still cannot write economic_evidence;
existing typed private storage retains its original entitlement. The actual
encoder's byte-push and range-insert growth, replacement buffers, simultaneous
payload/file/digest, path strings and native atomic working frames are admitted
before allocation and journal publication. Original ordinary methods remain
unchanged. Supported request sizing remains the existing pinned GCC13/C++11 ABI;
unsupported policy refuses.

The committed outcome means the JOURNAL is durable; it does not prove successful
after-image application or final journal removal. Terminal retention requires
both the successful complete commit result and committed outcome, then an exact
secure BODY readback and final lock proof. Any uncertain/error/refusal must keep
the coordinator journal, carrier and fences. Retrying never overwrites a pending
journal or reuses a prestaged after-image.

## Source evidence

Seven source files change: flat ROOM transaction C/header, authority transaction
C/header, accounting storage header and flatfile store C/header. Independent
review of both the integrated raw candidate and exact final formatted candidate
passes. All three previous C files remain complete byte prefixes; header
insertions are exactly reversible. All seven token and preprocessing comparisons
pass. The real terminal caller and bounded commit API agree; this source review
does not prove compilation or runtime behavior.

All 931 writer policies and 393 authenticated source pins retain their scope.
The clean candidate census remains 2,911 occurrences and 2,853 unique sites,
with zero new or unmapped sites. This source metadata is not full semantic
coverage or release evidence. The two unrelated test changes and independent
Plan5 metadata draft remain unchanged and unpublished.

Private source snapshots, inverses, formatting comparisons, exact hashes,
independent review and registry evidence are under
`tmp/room-terminal-retention-integrated-20261009/`.

## Remaining acceptance work and narrow interfaces

The actual warm/cold publication callback is not joined in this milestone.
The publication owner must carry its genuine selected root, same borrowed lock
and live budget context into the guarded coordinator terminal transfer. It must
not reacquire that held lock. Original envelope/revision/generation, physical
release, once-only action/checkpoint and ACK/fence ownership remain mandatory.

Warm flat preparation and refresh must use the genuine factory/scope lifetime
and full authenticated CURRENT room/custody/coin cut, then the original native
forest proof. Stored complete-room ordering differs from original factory order;
normalize by authenticated UID and parent, never infer a new root or fabricate
SQL creation_origin. Original cold present/completed/reconstruction/pending paths
still need real flat counterparts. This does not replace those requirements with
a warm-only deliverable.

Complete journal retirement storage remains open: the existing journal scan
wrapper catches allocation failure but does not prospectively admit its whole
mixed-journal scan, duplicate maps, canonical frames and uncertain rewrite
attempt. Existing non-ROOM records must retain their full validator contracts.
The coordinator's own live prefix must also remain charged across terminal
transfer. No census wrapper or guessed allowance proves those phases bounded.

No builds, native tests, gameplay, persistence or fault/restart journeys ran for
this successor; native tests remain in the requested major-plan batch. Complete
aggregate budgeting, actual publication/retirement/ACK, combined independent
Plan5 audit/backup/restore and R1–R8 acceptance/release gates remain open.
Accounting stays inactive, admission CLOSED, coverage incomplete, release
BLOCKED and the goal ACTIVE.
