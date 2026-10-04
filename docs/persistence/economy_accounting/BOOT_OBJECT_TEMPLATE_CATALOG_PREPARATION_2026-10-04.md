# SQL boot object-template catalog preparation — 2026-10-04

Status: source implemented, **UNQUALIFIED**. No compiler, tests, AST, native
engine, services, SQL or gameplay journey ran. Qualification remains deferred
until the major plan is ready.

## Source-established prerequisite

`src/world/db.c` currently populates `starter_object_templates` only for
alchemist/starter VNUMs. `find_object_template` therefore cannot supply ordinary
recovery for the complete native object index. `read_object` reparses at runtime
and invokes normal construction/procedures/events/conversion; it cannot serve
as an inert recovery lookup. Looping the old parser over every prototype would
also expose new fatal scan/allocation paths while booting unused prototypes.

BEFORE is preserved in
`tmp/object-template-recovery-before-v1.local/manifest.json`, base
`8130b347cc24fce6489cd2748eeb76b358c90800`, SHA-256
`64e259b208f8bc84a077716881662d07cd5c2f579d3dc32c718fe30e1883111b`.
Its 11 selected source/API inputs are not a compiler closure. Missing complete
catalog support is source-established; no measured native RED is claimed.

## Separate complete catalog

Only `src/world/db.c` and `src/world/object_template.h` change. SQL-mode boot
prepares a separate catalog after object index/special assignments and spell
pointers, before alchemist/starter caching and corpse/saved-world restoration.
No activation authority is inferred from SQL mode. The ordinary-drop caller
integration remains primary-owned.

Every native indexed prototype is staged, including prototypes with procedures
or activity-bearing types; no starter/alchemist subset or type filter narrows
catalog coverage. The private lookup vector is sorted by VNUM while each entry
retains its original native `R_num`, VNUM, source position and object special
function pointer. The native index order, counters, shared strings, procedures
and file formats remain unchanged. Duplicate VNUMs or invalid provenance refuse
the entire candidate rather than publishing a filtered catalog.

All parsing uses one `parse_object_template_with_reader` field/control-flow
implementation. Existing `parse_object_template` wraps its default reader and
continues the original `read_template_string`/shared-string and required/optional
scanf behavior. Numeric scan formats retain equivalent C whitespace semantics;
no changes are made to normal instantiation, RNG, events, conversion or UID
allocation.

The recovery reader avoids the fatal allocation and required-scan helpers. Its
bounded string mode preserves the valid native tilde, newline/truncation and
color-reset decisions; numeric/token mode bounds strings and checks native
numeric range. Malformed input, stream errors, capacity/allocation failure or
invalid B5 input refuse recoverably. OOM and all later staging failures leave
the catalog unavailable. Staging owns no live objects and invokes no gameplay
or diagnostic callbacks. The caller emits a bounded boot status diagnostic.
Valid-input equivalence still requires actual parser/native qualification;
source inspection alone is not its runtime proof.

The source stream position is restored before sealing. A local vector is
published only after complete parsing, uniqueness/provenance checks and
successful restoration. Boot, material-only boot and world teardown invalidate
the prior sealed generation first; failed new preparation cannot expose an old
generation or partial candidate. The existing flatfile/inactive starter cache
and cold `read_object` loading are preserved. This specifically SQL prerequisite
does not claim a complete flatfile catalog/recovery route.

## Lookup contract and integration

`recovery_object_templates_ready() noexcept` is allocation-free and checks the
sealed SQL catalog's current table, count and file identity. It is not a census
of every later mutable index field or a SQL/custody/ACK capability.

`find_recovery_object_template(vnum) noexcept` performs a bounded binary lookup,
then validates the target's exact current native `R_num`/VNUM/file-position/
object-special-function identity. It never parses, populates a cache, repairs
an index or falls back to starter templates. Miss/stale provenance returns
`nullptr`. Const pointers remain stable until world teardown/reboot. Boot and
the ordinary recovery owner retain the existing serialized main-thread world
ownership; no new mutex or concurrency contract is introduced.

The catalog is an immutable boot snapshot, not ongoing filesystem edit
authentication or live builder refresh. Source files must remain the trusted
native boot input. Fresh eligibility still belongs to the recovery owner;
preparing all prototype types does not qualify their activity bookkeeping,
procedural reconstruction, ordinary publication, held saves or critical ACK.
Primary must replace both ordinary recovery lookup sites with this dedicated
lookup and distinguish unavailable catalog from a genuinely missing VNUM.

Existing `test_newbie_object_template.py` extracts the former parser/cache code.
Its maintained closure needs explicit adaptation to the shared reader/core and
preserved default wrapper at the eventual major-plan gate. No maintained tests
or central registration were edited by this owner.

## Deferred evidence

`tmp/object-template-recovery-prepared-v1/cases.md` is a declarative plan only,
not an executable fixture or runner. It requires actual shared production
parser/catalog code and trusted native area/index inputs, with explicit
controlled allocator/construction endpoints where appropriate. Complete
boot/publication/restart qualification remains primary-owned. Preparation
formatting, whitespace inspection and hashes are not native test passes.

The major-plan native gate must measure full-index SQL boot time/resident
catalog cost and parser parity under the original deadlines. No deadline,
test budget, source activation gate or gameplay assertion was changed here.

## Frozen source review

- `src/world/db.c` SHA-256:
  `8a2f05bf8fb95c773b6d612608fae23571342215d604ac913d50aebd0ecf50e4`.
- `src/world/object_template.h` SHA-256:
  `96950e840d57ea7474e3ad5e8f80ac13edbc6ae079a07030aefbfccd47d23125`.

`cpp_modernization_architect` completed full source review at these pins and
reported no source blocker for this preparatory scope. Review checked legacy
valid-input parse decisions, nonfatal recovery paths, complete private staging,
cursor restoration/provenance, immutable allocation-free lookup and boot/
teardown ordering. Continuous filesystem/admin-change attestation remains
outside this catalog's authority. No compiler or execution was performed.

`tmp/object-template-recovery-prepared-v1/manifest.json` records 13 selected
inputs: 11 source/API files, this report and the 24-case declarative plan.
Raw copies are preserved in `source-after-v1/`. This is neither a complete
compiler closure nor an executable native fixture/runner. Owned source/header
formatting and `git diff --check` completed; all runtime acceptance is pending.

## Integrated recovery consumer

Primary changed both all-absent reconstruction lookups to the complete recovery
catalog, without starter fallback. Readiness is checked before private staging
and again before final enrollment; unavailable catalog retains the operation as
unavailable. The complete catalog removes the former starter/alchemist-only
prototype limit; activity/procedure eligibility and production recovery remain
separate unfinished requirements. No prototype is parsed during publication.

Consumer raw SHA-256:
f69c78d2cc3a2875fad6f3000302a001f0c00262d5965153ba2143e1b5f4759f.
Independent source review accepted this integration, with no execution proof.
Its one-input BEFORE manifest is
tmp/recovery-catalog-consumer-before-v1.local/manifest.json, SHA-256
d207569a219875562cec0c44e97b3d9f0c90390a405d7a7ca564b81703d2ea7b.
The earlier 13-input catalog AFTER manifest/report remains preserved at its
original pins; it does not include this later consumer/report supplement.
