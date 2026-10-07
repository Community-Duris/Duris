# Native auction early admission qualification — 2026-10-07

The private native LIST and CLAIM accounting intents now refuse more than
3,000 selected nodes before domain SQL mutation. Each selected node produces
one item event, so the established event cap must hold at admission. Structural
native commands, full player snapshots and read-only ANF2 facts retain their
4,096-node capacity. Legacy payload version1 behavior remains.

The prior corrected source4e4efd71 allowed oversized native frozen commands;
plan normalization could refuse only after domain DML within the SQL transaction.
The repair adds a capacity check in each shared `append_native_facts` builder.
Intent freezing, accounting decode and repository frozen validation use it.
No global cap expansion, native wire restriction or fabricated fence is added.

## Original failure and positive boundary

The genuine original source freezes and validates six commands: LIST and CLAIM
at3,000,3,001 and4,096 selected nodes in a complete4,096-node player body. The
same new regression fails the original at LIST3,001's early capacity assertion
(exit-6). Fixed source30837d50 accepts3,000 and refuses3,001/4,096 through intent,
decode and the real frozen validator. The test also decodes the unchanged native
command and read-only facts at4,096. Both unchanged original legacy units pass.
Original30-second runtime deadlines and ASAN/UBSAN/warning flags stay exact.
No warning suppression or stub is introduced. Two link-prerequisite failures
are preserved; their closure uses the real inert stage, studio, allocator and
memory/utility providers. Eighteen unchanged objects from the first attempt are
reused only with same-source/image/argv proof.

The fixed boundary runs in8.300 seconds; original legacy units in0.219/0.168.
No sanitizer errors. All1,336 selected inputs per source remain byte/mode exact.
Primary authenticates67 artifacts and all six original frozen commands.
Protected packet:
`tmp/auction-native-admission-boundary-unit-complete-providers-primary-20261006/`.
Qualification SHA256:
`7bc35bc1baa32fb6b980ef4d0afe7f7b555c218311b0e9b3f8d5d240de01b489`.
The packet retains actual binaries, commands, raw inputs, logs and prior failures.
Owned containers are absent; no SQL/game services ran in this unit.

## Matching production candidate

Source archive308 SHA256:
`30837d50844b35abbb20cac709db06bf6ff0c7fa9c72fc03c11a8f70f883f661`;
manifest `0c4e20870753c69e9dfab75e705d2aec25b0b4eb8c363304a05ed127cb7addb1`.
Both original strict753-provider production builds and nine contracts pass.
Each rebuilds the two accounting providers, reuses751 dependency-authenticated
objects and performs a fresh complete link. MariaDB Make6.74 seconds; flatfile
7.40 seconds. Primary authenticates23 artifacts,6,367 source members and both
complete1,508-member caches. Handoff:
`713b9404910043a0cca8f8c1df7c7b71da8cb6c4d80a3c5018ea069179d29e11`.
Owned build containers/volumes are absent after export/seal.

This is private-source unit/build evidence. Original full-tree auction SQL and
same-ELF warm/cold/fault suites have separate terminal evidence on this matching candidate, recorded below.
The unit does not prove SQL mutation, physical gameplay, recovery, activation,
complete plans/R1–R8 or release. Producer integration remains unpublished.
Existing inactive behavior and the declined spell path remain unchanged.

## Subsequent runtime evidence

The matching actual same-ELF normal warm/full-cold/owned-shutdown journeys pass
on MySQL and MariaDB. Genuine MySQL origin-INSERT fault preserves the expected
phase-2 journal, but pending recovery hits the original 60-second timeout;
MariaDB fault cases are unrun under fail-fast. Full-tree LIST 151 applies,
then the original early-finalize assertion fails with unobserved receipt errno.
Primary authenticated those terminal exports and exact owned cleanup. These
results leave recovery and complete native non-item execution unfinished.
