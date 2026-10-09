# ROOM placement native journal transport correction

## Failure and correction

The genuine ROOM command builder emits payload version 2 when it carries the
original placement recipe. The coordinator already recognizes versions 1 and 2,
but the native journal's shared transport predicate accepted only version 1.
Source tracing shows that a valid placement command consequently fails native
frame construction before durability admission; an otherwise valid saved version
2 frame also fails native decode during journal scanning. This is a transport
mismatch, not a new accounting route or source permission.

`critical_command_journal.c` now uses the existing named ROOM version constants
and accepts precisely those two formats. The shared predicate covers native
append, replacement/synchronization and decode. Unknown versions still refuse.
Other command families, accounting schema, publication requirement, generic
envelope checks and journal framing remain unchanged.

Transport does not authenticate execution. The original registered typed
initial/replay/successor/publication/terminal validators remain installed in
`net/comm.c`. ROOM command decode still verifies payload, intent binding and the
complete rebuilt canonical command. Repository execution still requires the
original INITIAL recovery proof and genuine execution owner. Original inactive
behavior, active-flat reset refusal and all accounting admission gates remain.
The declined inactive spell path is untouched.

## Evidence and deferred native check

Independent review of the exact final source passes. Removing the one include
and reversing the one ROOM predicate restores the complete original file byte
for byte. Changed-line formatting preserves tokens. The genuine builder and
coordinator constants independently confirm versions 1 and 2; accepting a range
or skipping typed validation was not introduced.

All 931 writer policies and 393 authenticated pins retain scope. The clean-source
census remains 2,911 occurrences and 2,853 unique sites with zero new/unmapped
sites; this does not establish accounting completion. Exact snapshots, inverse,
review and registry evidence are in
`tmp/room-placement-journal-transport-integrated-20261009/`.

No native tests/builds or append/restart journeys ran for this successor. They
remain in the user-requested major-plan batch. Extend the existing native journal
fixture pattern under
`tmp/native-quest-transport-qualification-primary-20261006/fee_journal_cases.cpp`
with a genuinely codec-built placement carrier: append, shutdown/reopen, native
replay, checkpoint, continuation and reopen. Preserve ROOM1 behavior and prove
unknown-version append/decode rejection. The ordinary-command fault test does
not already establish this native case. These provider checks also cannot replace
the complete genuine gameplay/persistence/recovery acceptance journeys.

Actual flat warm/cold publication and guarded terminal transfer/retirement/ACK,
complete aggregate and journal budgeting, combined Plan5 and R1–R8 gates remain
open. Accounting is inactive, admission CLOSED, coverage incomplete, release
BLOCKED and the goal ACTIVE.
