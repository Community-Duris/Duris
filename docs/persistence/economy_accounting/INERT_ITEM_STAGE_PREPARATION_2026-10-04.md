# Inert literal item stage preparation — 2026-10-04

Status: **IMPLEMENTED, SOURCE-REVIEWED, UNQUALIFIED**. This is an opt-in,
discard-only constructor prerequisite, not live publication or cold recovery.
No compiler/native/AST/SQL/gameplay/recovery check ran. Normal read_object and
existing SQL/flatfile/player materializers remain unchanged.

Source tracing found normal instantiation issues a UID, enters global lists/counts,
mutates shared text/procedure caches, invokes procedures, consumes RNG, schedules
events, fills spellbooks and converts fields. Detached saved-items staging also
recalculates container weights by constructing another normal prototype object.
Extraction/free_obj reaches native custody and can invoke barb cleanup. Existing
corpse/artifact stage guards do not suppress all those effects.

The new player/inert_item_stage module accepts an already prepared prototype and
complete four-string SQL literal. It validates index/vnum and representable retained
UID/key/bitvector/affect/timer data, bounded descriptions and no-NUL text before
allocation. The prepared prototype is trusted input, not source/authenticity proof.
The exact supplied literal mask is preserved. It copies saved plain fields, weights,
text and exact description chains without merging mutable template descriptions.
No parser, normal instantiation, UID allocator, creation flag, global list/count,
shared string-cache change, runtime custody registration, conversion, weight
recalculation, procedure, RNG, event or persistence write occurs.

A move-only stage immediately owns every partial pool/string/node allocation.
Its destructor frees those resources directly and returns its originating pool
slot, with no extract_obj/free_obj/effect/procedure/event/custody callbacks and no
recursive graph cleanup. Null partial pointers are skipped. Failure preserves
an existing output; successful replacement and move/self-move ownership are explicit.
Only const borrowed inspection is public. There is no ownership release/enrollment
API until the complete native proof/publication owner exists.

## Explicit unsupported boundary

Before acquiring any object, refuse prototype special functions, procedure keywords/
flags, random-exit names, switches, spellbooks, artifacts, corpses, money, transient
flags and trap-bearing templates. Nonzero saved timers, dynamic affects and
spellbook description data also refuse. The snapshot has no historical trap fields;
copying today's trap template would invent state. This conservative subset does not
silently discard behavior and does not claim supported gameplay/census coverage.

Use the [nonfatal allocation primitives](INERT_ALLOCATION_PREPARATION_2026-10-04.md).
Absent/exhausted or mismatched object pools refuse; there is no growth fallback.
All stages must drain before world/debug-memory teardown. Exact full graph topology,
UID uniqueness, current SQL season/custody/epoch/payload proof, authenticated prototype
preparation, allocation preflight, fresh eligibility recheck, atomic runtime hydration
and nonthrowing final enrollment/room linking remain separate integration gates.

## Prepared source pins

- `src/player/inert_item_stage.c`: `4dd014289925427c4213d04980dd4833edf4d9233edc81d4f47b53e0db806b60`.
- `src/player/inert_item_stage.h`: `749632640d451fdd3650b3aaab0b2f8593bccc76b8e2a2709d007a736c6dffee`.
- `src/Makefile`: `2ff3ca5c082ce3e371876ce3299457fcba89cccc44dc157aeb8f360bab2d501c`.

BEFORE base64c495eb0: `tmp/inert-item-stage-before-v1.local/manifest.json`,
SHA-256 `764ca948a812e91c63954b0f356445aed58f3c0f4570feea74a1084df26c654a`.
The module/header do not exist on BEFORE; missing APIs or compile failure cannot
be semantic RED. Native deferred cases must cover every allocation failure, partial
chain cleanup/output preservation, moves, exhausted pools, exact literal round-trip,
unsupported cases before acquisition, zero UID/RNG/list/count/procedure/event effects
and unchanged ordinary/inactive behavior. Private owner preparation remains pending.
Production object-list registration is source-only; no build has executed.
R1–R8 and release=BLOCKED remain.


## Final private owner preparation (unexecuted)

`tmp/inert-item-stage-prepared-v1.local.json` SHA-256
`5ed45c6c55bcab2fc49878c16b6e0ba3225781e8f619ed9e22e99f3849b1b7c2`;
LF `b77eb7be265995172ec4010caaddab70fa106fc448605ba7f3845197f5ec847d`.
Immutable b36e9690f full source archive supplies constructor/mm/memory:529 declared
source inputs,531 with private owner files,22 case groups. Ten actual malloc sites
have all-ordinal failure oracles with native __free/tag/pool cleanup. Output/moves,
exact literals and pre-acquisition refusals are prepared; all selected LF pins match.
300 compile/30 case/120 aggregate bounds are proposed, unmeasured. BEFORE API absence
is unsupported exit78. UID/RNG/custody invocation absence is a source/link boundary,
not full-world proof; no normal constructor/enrollment/release success stubs exist.
LP64 wider integer narrowing cases are architecture-limited. No execution ran.


Pin reconciliation: source-review rawe53f1247... preceded two affect-guard line
wraps. Read-only reversal exactly reproduces that raw hash; no semantic change.
Final current module raw `4dd014289925427c4213d04980dd4833edf4d9233edc81d4f47b53e0db806b60`,
LF `373377acfb532671112741bb8061b9845b4f7980336d44fc2c7c0c92b331e06a`;
header raw `749632640d451fdd3650b3aaab0b2f8593bccc76b8e2a2709d007a736c6dffee`,
LF `9f996becdaa6e4cf1697c300b7790101769a8516bb02ae447dad291878f72841`.
Both current LF images equal b36e9690f Git blobs and were reread by the source
reviewer with no remaining blocker in discard-only scope. No native checks ran.
