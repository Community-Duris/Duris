# Native birth journal carrier source handoff — 2026-10-06

The shared journal/coordinator carrier now supports original birth-v2 mutable
recovery through the existing native envelope. Independent source review accepts
this three-file private slice. It is not installed in maintained native source
and has no new build, native gameplay, persistence or recovery qualification.
Actual domain codec and constructor/startup wiring are still being integrated.
No Plan or release gate is marked complete.

## Established gap and implementation

The original envelope accepted item-transfer/native12 only. The existing raw
birth ACK removed its command after physical publication, leaving no durable
carrier for remaining original constructor progress. Its generation and refusal
cleanup methods also excluded native envelope owners. Birth needs its own exact
context without borrowing a player-save token or quest capability.

The journal admits birth21/schema2/payload2 through its existing framing,
checksum, quota, append, exact replacement, mixed replay and retirement paths.
The coordinator retains the original admission queue, uncertain-admission
handling, execution fences and byte limits. Raw birth-v1 remains readable;
typed birth-v2 requires its context. Every existing private quest submission,
copy, checkpoint, continuation and player publication entry retains an explicit
native12 route check.

The sole private birth friend gains envelope-bound submission, context copy,
same-phase progress CAS, generation observation, refusal cleanup, physical ACK
and terminal retirement. Five dedicated pure domain validators must be installed
for birth admission, passive replay, progress successors, publication and
retirement. These check the original body; actual current SQL/world proof remains
with the birth owner.

Physical ACK compares the complete supplied current genuine coordinator receipt,
original command, generation and expected attachment before exact journal CAS
from execution phase to continuation phase. The recovery codec separately binds
the retained original receipt's economic result to the current receipt. No
delivery timestamps are synthesized. The operation remains available for the
original tail; birth consumes no player-save hold and creates no command-only
completion-cache entry. Terminal retirement is a separate exact-context action.

Definite refusal cleanup binds the delivered original `never_admitted` receipt,
command, context and generation. It pins the operation across the original native
callback and removes it only after successful same-generation proof. Uncertain
admission cannot authorize cleanup. A never-admitted operation has no durable
frame to checkpoint and creates no fabricated ACK.

All copies, canonical command comparisons and successor preparation precede
pinning and journal I/O. Post-I/O and post-cleanup proof uses the original pointer,
generation, revision, phase and attachment without fallible encoding. An uncertain
transition retains its exact expected/successor pair; context copy refuses until
that transition is settled. A new revision cannot replace an uncertain retry.

## Exact source and evidence

Private parent:
`tmp/plan3-native-mobile-connected-live-major-source-20261006`, manifest
`4057b693b2af446dfe56e20265eb49bbf515051b6b41a755b51e56dcb4809df4`.

Reviewed packet: `tmp/plan3-native-mobile-birth-carrier-20261006`, manifest
`177159feb3d7e5aefd7ebc0146e768f3a36da7d15fa72e2391140555e1072cff`;
frozen receipt
`6983875e951ffa4441bb9f8db7b5172e681bfb69dffc9be980fb9777c184499f`.

| Changed private file | SHA-256 |
| --- | --- |
| `src/persistence/critical_command_coordinator.c` | `3a4b8f4937d98f4c7a4897e471ce13bd489d65051aca70155be3cd40d778098f` |
| `src/persistence/critical_command_coordinator.h` | `83e258eda8c4f85894d7d03f5278352a02a4ba792ebdc398980674495936afd9` |
| `src/persistence/critical_command_journal.c` | `f0c337de0a57055dc686da91979960de8bda035277bd9defbf8a7984435ace1e` |

Full private composition:
`tmp/plan3-native-mobile-birth-carrier-major-source-20261006`, manifest
`8ee284e7a89b75bfe56d029a29dfaadfd2723e24a8efa205b302a331bfcdba2f`.
All 5,771 original inputs match their pins; the original copy/build drivers and
flags are unchanged. Exact byte-edit inverses and changed-line clang-format18
fixed points pass. Private diff whitespace check reports no errors. The
`cpp_modernization_architect` verifies the actual three-file manifest and accepts
the bounded carrier source. These are source checks, not execution evidence.

## Remaining integration and qualification

The independent factory owner is completing original recipe capture/literal cold
restoration, published-object adoption and actual typed birth startup/publication
wiring. The independent codec owner is implementing the pure bounded original
UID/effect/mobile/binding progress body. Their immutable-parent slices must be
composed with this shared carrier into one candidate before native qualification.

Cold started/unreturned actions retain uncertainty. Recorded runtime projection
or physical-proof flags cannot bypass fresh native/world and confirmed SQL
cleanup proof. Original reset mechanics outside the connected subset, flat birth
parity, lifecycle/copyover, registry evidence and complete gameplay/persistence/
restart qualification remain required. The original limits and inactive behavior
are preserved; production activation and data changes remain unauthorized.

`coverage_complete=False` and `release=BLOCKED` remain unchanged. Existing peer
reader results and older native candidate results are not transferred to this
new private source.
