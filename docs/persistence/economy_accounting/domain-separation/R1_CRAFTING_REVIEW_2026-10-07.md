# Coordinator review of R1 Craft/Forge material preparation - 2026-10-07

R1's source and focused component assessment found no actionable defect. Both
maintained backend builds have successful terminal evidence. The existing isolated
runtime journey is still in progress at this review checkpoint; its result and
the worker's final qualification handoff remain outstanding. R2 and the continuing
coordination Goal are not complete.

## Exact bundle and scope

Reviewed implementation `48cdf9cb0893873651216f7940aae2691d060e58`, parent
`92871c3dcb9bf90932eb102775ed9547022f4163`, on
`origin/codex/accounting-domain-separation`. It changes four files:

- New `src/economy/crafting_plan.h`: the unchanged plan fields and pure
  `crafting_prepare_plan`/quantity scaling from explicit item value, material VNUM,
  magical fact and multiplier.
- `src/economy/crafting.h`: include that plan definition.
- `src/economy/crafting.c`: `crafting_build_plan` retains live fact capture and
  invokes the pure planner. Existing preview, recipe and make callers still use
  this wrapper.
- `tests/async/test_crafting_material_bounds.py`: retain the original production
  wrapper, gate, numeric/refusal controls, sanitizer flags and deadline; add direct
  owned-input controls that exercise the actual new header implementation.

The formula, ceil/finite checks, wide addition, VNUM bounds, magical result and
unchanged output on failure are preserved. The existing `has_affect` implementation
only reads item flags; moving that capture before quote refusal adds no mutation.
Input selection, native craft submission, output/UID admission, pouch conservation,
progression, config persistence, receipts, ACK and recovery functions are unchanged.
No authority interface or dual-backend execution strategy is introduced.

The four-file patch passes `git apply --check` against current accounting source
at `b876f9442040653cf53c15d81d5188166e1cd829`; no patch was applied. It can be
reviewed independently of R0's Collector changes. Confirm current preimages and
unpublished primary work at import time. Primary adoption is unknown, and this
optional sidework does not delay its accounting plan.

## Independent component proof and inspected build evidence

The coordinator's separate clean review checkout was detached at the exact R1
implementation revision. The following command passed independently through WSL
Ubuntu-22.04:

```bash
python3 tests/async/test_crafting_material_bounds.py
```

It compiles the actual header and extracted production capture wrapper/gate with
the existing address/undefined/float-cast-overflow sanitizers and executes the
original numeric controls plus new owned-input tests. Whitespace verification
also passed. The coordinator did not rerun the full builds or runtime journey.

Worker evidence in its private `bin/tests/domain-separation-r1-20261007/` includes
source-pins.json, material-bounds/recipe-transaction/progression logs, both build
logs and build-terminal.txt. All four recorded source hashes were independently
compared with the exact published Git bytes. Both logs recompile `crafting.c` with
the original strict C++20 development flags, and the flat build defines
`__NO_MYSQL__`. This is maintained incremental build evidence: 10 SQL and 9 flat
compile invocations, followed by a complete 740-object server link for each.
Do not describe it as 740 freshly compiled providers per backend.

Container `duris-domain-separation-ac24-r1` is terminal with exit0. It mounts the
worker source read-only and its private `duris-domain-separation-ac24-build` volume
for artifacts. Image:
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
The live runtime container uses the same mounts/image. Read-only SHA256 inspection
there matches the recorded source and both terminal binaries:

| Input/artifact | SHA256 |
|---|---|
| `crafting_plan.h` | `623cd76374693dc87a940db5409fae4884313d1fa5affc401f68a8f6437cb268` |
| `crafting.h` | `ea148ac7f1276c3a81449339494df0bbd94ad2a91d66f205b3ead59066c98c00` |
| `crafting.c` | `cadb63d7c3d060ed1e4628df21624f54d3fdb6bd70480c2e2dfa5cc766a96051` |
| Material-bounds runner | `6bc12e410bc7898399276548fc60033ff2f1ad8f773ab1d5d5c3793233a3d2bd` |
| SQL `dms_new` | `b85d8bbbd7970cf7fc831959531841658319a826c10df194bcec5fd80f0b72ab` |
| Flat `dms_flat_new` | `79e385143b87bf6d013f2e9c4a9d8393ee8359bbf71894076ed4370083872843` |

## Remaining actions

Let the confirmed live runtime verification finish; an observation timeout is
not a restart instruction. Record its original commands, tested source/binary,
backend, assertions, terminal result and retained setup failures before extending
this review to runtime scope. Genuine DB/player/replay qualification cannot be
inferred from the focused planner test or link success.

The coordinator asked the worker to refresh its canonical handoff, whose inventory
checkpoint still describes R1 as unimplemented. Distinguish current implementation,
component/build assessment and pending runtime evidence without rewriting history.
Publish the usable import pins and actual qualification scope promptly.

Then publish the exact R2 reservation before implementing wallet-value and
bank-payment arithmetic. Keep shared submission/publication/recovery ownership
and native range/overflow behavior intact, as required by the
[inventory assessment](INVENTORY_REVIEW_2026-10-07.md). This R1 review checkpoint
does not complete the fixed implementation set or the selected quest-prep follow-up.
