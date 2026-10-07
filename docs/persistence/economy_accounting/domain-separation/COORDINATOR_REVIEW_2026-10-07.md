# Independent review of the first domain-separation delivery

## Recommendation and current status

The bounded Collector collection-image extraction is suitable for primary review
and candidate integration, subject to the current candidate's compatibility and
maintained qualification. No actionable source defect was found in implementation
`3d2b85b0688684721f8db559cb3ea35b1830a1cc`. The coordinator independently executed
the published collection-preparation and policy runners successfully. This is
not unconditional merge approval, primary adoption, or full accounting release
qualification.

This review is **in progress**: the worker's maintained SQL build has completed,
its flatfile build is running, and its final terminal-results handoff is not yet
published. The coordinator Goal remains active until the final delivered evidence
and remaining handoff gaps have been assessed. Native database/server/player and
recovery journeys are unrun for this delivery; component results do not imply them.

Primary accounting continues independently. If import or validation would disrupt
the current accounting slice, defer this optional architecture bundle.

## Reviewed revisions and ownership

- Accounting base: `7dbc472123a729f2fedc345e5309a586ba8a02d8`.
- Research: `3e9d03db25ea459662b0cd6df8e764f05dd5a0e1`.
- Production/test bundle: `3d2b85b0688684721f8db559cb3ea35b1830a1cc`.
- Published worker handoff examined:
  `bc19aa68294f5e0e1e19a2deecb84566d425ce8b`.
- Worker: `origin/codex/accounting-domain-separation`, canonical
  [handoff](https://github.com/Community-Duris/Duris/blob/codex/accounting-domain-separation/docs/persistence/economy_accounting/domain-separation/HANDOFF.md).
- Independent review checkout:
  `C:\Users\alexa\.codex\worktrees\accounting-domain-review\NewDuris Max`,
  detached at the examined worker handoff. Builds/tests use its own `bin/`.

The implementation touches exactly two production paths and two test paths:
`src/economy/collector_collection_image.h`,
`src/economy/collector_collection_preparation.c`,
`tests/async/collector_collection_preparation_harness.cpp`, and
`tests/async/test_collector_collection_preparation.py`.

The production change replaces only `capture_exact_blob`'s local calculation and
encoding with a call to the owned-state preparer. It changes no transaction,
repository, migration, registry, accounting codec, quest, audit or recovery owner.
Published upstream source and shared capture/codec/command inputs still match the
base at this review checkpoint. Unpublished primary Collector work remains an
overlap risk; this observation cannot reserve those functions against it.

## Behavioral assessment

The original capture copies `object->weight` into `player_item_snapshot.weight`.
The extracted preparation receives that owned snapshot and explicit direct-child
weights. It retains the original negative/overflow checks, subtracts the same
children's weights, requires an int32-representable nonnegative own weight,
normalizes the singleton parent/equipment fields, and encodes with the same
production codec and Collector size bound. Other literal properties move through
unchanged; no identity, time, randomness, recipient or revision is recalculated.

Both `collector_collection_prepare` and `collector_collection_live_matches`
continue to use `capture_exact_blob`. Thus admission and live publication
revalidation use the same image rules. Complete source-root capture, eligibility,
custody/topology/revision checks and actual detachment remain in the existing
code. The prepared image does not establish authority or prove applied effects.

The extraction adds a temporary child-weight vector and therefore another
allocation opportunity. Its allocation failure returns the existing false/
invalid-topology refusal. This preserves failure safety but does not prove equal
allocation cost or performance. No performance improvement is claimed.

The collection harness replaces its stub encoder with the real production codec;
the live capture and custody/world seams remain fixtures. Its previous 32-byte
stub-format expectation becomes real decoding and property assertions. Original
custody, room/corpse, claim, exclusion and stale-publication controls remain. The
new section/link flags allow unrelated codec entrypoints to be discarded; original
warnings, optimization and the 30-second execution deadline are retained.

## Independent verification

Executed from the separate review checkout at the exact handoff revision:

```powershell
wsl --distribution Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-domain-review/NewDuris Max' --exec python3 tests/async/test_collector_collection_preparation.py
wsl --distribution Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-domain-review/NewDuris Max' --exec python3 tests/async/test_collector_policy.py
git diff 7dbc472123a729f2fedc345e5309a586ba8a02d8 3d2b85b0688684721f8db559cb3ea35b1830a1cc --check
```

All exited 0. These are executable components with production image encoding,
not native database or player journeys. The source diff and actual capture/caller
implementations were examined directly; this review does not rely solely on the
worker's narrative.

Git blob identities for the independently examined source:

| Input | Git blob |
| --- | --- |
| Base collection preparation | `90bca1b00e8991650a7cc3958866fcefcf732515` |
| Extracted collection preparation | `ed5494d3cf9d2204f669e32836f7bb43ed284e41` |
| Owned image header | `cb3de74a025126f93c9a039b9d6c3facd5774821` |
| Collection harness | `7b3503ecc7e146f6443a45cf003cdf0c392a575d` |
| Published runner | `72017a51f5661c269430b22636093abecb633aa8` |

Worker-only evidence is recorded separately in its handoff. The coordinator
observed `SQL_EXIT=0` and final make completion from the worker's isolated
`duris-domain-separation-ac24` build; its SQL executable SHA256 is
`6de48baed73542d44bf697ee93f4682b93f30e8c6d816ca940060cbe2c2659c1`.
The flatfile build has started but has no terminal result at this checkpoint.
The worker reports original/extracted ASan/UBSan passes and a detected deliberately
incorrect weight calculation. Those sanitizer runs have not been independently
repeated by the coordinator.

## Import prerequisites and remaining evidence

The production/test commit is the independently useful import unit; research and
handoff commits are advisory metadata, not additional runtime prerequisites.
Fetch the branch and compare the existing collection helper's preimage and shared
snapshot inputs against the current integrated candidate. Do not blanket-merge
the worker branch into accounting or overwrite primary private work. Primary may
adapt the small call site if compatible, or defer it.

Assess the final published terminal-build evidence before claiming maintained
qualification. Original adjacent purchase/transaction/accounting runners report
missing shared link inputs; privately adding an existing recovery-manifest source
is diagnostic evidence, not a passing unmodified runner. Those shared failures
remain with their current owner. No competing manifest/transaction repair is
required of this architecture stream.

After import, execute the collection-preparation and policy regressions on the
combined candidate, the maintained build and applicable Collector verification
at the existing qualification boundary. Record imported SHAs and actual results
in the primary's normal implementation history. Worker build hashes cannot
qualify a different combined candidate. This review changes neither accounting
release gates nor actual authority, and does not certify RAM conversion.
