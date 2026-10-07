# R4 superior tribute-count handoff — 2026-10-07

Connected owned material quote implemented at
`0a5084db1f30dd85f2181df6504c36a7540099d6`, after reservation
`693d57042ea92512d220d6c200176cb1f1c5126b`. Coordinator approved the exact
three-file boundary locally while GitHub writes were unavailable, independently
authenticated the original function pins and then passed the same extended
original/extracted material controls. No extraction defect was found. The coordinator boundary review is now
published at accounting `3f6f1fe0cb62906e07dd8a36b4436ebb53e7582f` in
[R4_SUPERIOR_TRIBUTE_REVIEW_2026-10-07.md](https://github.com/Community-Duris/Duris/blob/3f6f1fe0cb62906e07dd8a36b4436ebb53e7582f/docs/persistence/economy_accounting/domain-separation/R4_SUPERIOR_TRIBUTE_REVIEW_2026-10-07.md). Maintained
build/evidence final disposition remains pending. GitHub recovered: ordinary push
and remote read now verify the source commit on `codex/accounting-domain-separation`.
The continuing Goal remains ACTIVE beyond this checkpoint.

## Implementation and preserved behavior

`src/economy/enhancement_material_quote.h` owns low/high tribute counts and
prepares them from a native int target item value and explicit configured double
multiplier. Original wide `(item_value + 4) % 5` and `/ 5` rules, low-first scaling,
`count * multiplier + 0.999999`, truncation and upper-bound refusal are retained.
Zero item value is accepted; tiny positive multipliers may yield zero tribute.
Negative values, invalid multipliers and overflow refuse. The joint quote assigns
only on success. Native int bounds make the wide addition safe. No Craft ceil
policy is substituted.

`src/item/enhance.c` captures those facts at the original calculation point after
the existing target/prototype probes and material-VNUM determination. Replaced
local scaling/calculation is removed. Unchanged `superior_plan_add_material`
receives successful counts. Native target selection, catalog/remaining-step
iteration, aggregation/slots/capacity, partial overall plan on failure, source
objects, RNG, caps, effects, payment, persistence and active refusal retain their
existing authority and ordering. Only this source, the new header and the existing
material regression change.

## Terminal proof

`python3 tests/async/test_superior_enhancement_material_bounds.py` passes with
the real new header and native builder/aggregation. The same extended expectations
also pass on retained original complete bodies. The private original controller
redirects only `source('enhance.c')`; all original assertions, ASan/UBSan/
float-cast-overflow, no-PIE flags and 30-second execution deadline remain.
Owned and native cases cover zero/INT_MAX/negative values, fractional rounding,
invalid/extreme multipliers, low-first/high-later overflow, unchanged output on
failure, repeated preparation, zero-value native targets and tiny-factor free
tribute. Existing source/material effects are checked.

Adjacent material multiplier configuration, all-stat configuration, superior
configuration, all three payment controls and four actual active paid-outcome
refusal cases pass. Repository `scripts/format.sh --check --file src/item/enhance.c --file
src/economy/enhancement_material_quote.h` and whitespace checks pass. An earlier
empty-worktree changed-line check also terminates successfully; the explicit
file check verifies the committed R4 code.
No shared fixture, manifest or native authority was repaired to obtain these results.

Both maintained builds complete exit0, controller exit0:

- `make -C src -j2`
- `make -C src -j2 PERSISTENCE_BACKEND=flatfile DMS_BINARY=/workspace/bin/server/dms_flat_new`

Each recompiles one provider (`item/enhance.c`) and links all **740 objects** on
the preserved owned branch with R0/R1/R2/R3/R4. SQL ELF SHA256
`e5aa3008a0403137e9449a6f5375807ffd4e277efc28d146d12143e679d088f4`;
flat ELF SHA256
`c572ca3785bad63d64afa057e05cbd6071fa209879e47bcd4adc52521600e199`.
These are not the independently running 753-provider R3 candidate build.
No SQL/player service or native payment/persistence journey ran for R4.

Task-owned container `duris-domain-continuation-ac24-owned` has the actual
worktree read-only at `/workspace`, no network and the retained task build volume
at `/workspace/bin`. Image pin
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
Prior R3 binaries were retained before rebuilding. Ignored local evidence is
`bin/tests/domain-r4-20261007/`: original/final regressions, adjacent controls,
original source/controller, full build logs, terminal results, source pins,
RESULT.json and proof hash index.

## Candidate and next ownership boundary

Exact R4-only patch independently passes cached application against bare producer
`275df7f626e12cb396a22da34317a4e7f355e9a1`; patch SHA256
`f2947947133f2f624a8cf176403ffbbb047dce3eeba49e89106768ad91ac8701`.
This does not require R3 adoption. No primary source is imported or modified here.
R4-only current-candidate snapshot tree `34e24d64c6e42e7de2b5dfba5568e8f3cb5ac934`
contains6425 exported files; archive SHA256
`721ea5ec939a6972fd88a4ead268c7dec9c43245ccce36eedad51e1687746b53`.
Its actual material regression passes with unchanged sanitizer controls, and full
`enhance.c` passes SQL/flat strict C++20 syntax/type checks against current headers.
Task-owned `duris-domain-r4-ac24-candidate` uses that read-only Git export, no
network, a separate same-named output volume and the same pinned image. Copied
logs/RESULT.json are in the evidence path's `candidate/` directory. Current R4
candidate full-build/native/journey qualification is unrun; application,
component and module type compatibility remain distinct from integrated gameplay.
The immutable R3 candidate build continues without adding R4 to its source.

Source SHA256: header
`a5c63c4ca823b66c819a4bf67b52781eca9816b184329e17b33552fc830b93b6`;
enhancement producer
`f6ac9cc50f061ac149253fc858fd7e10ac8345f85dccfa0c097b28ca7c3554d2`;
material regression
`1fe33d11733323a9cb8069ef982ca5b9ed5abbaaa11567a226f519d916385f1e`.

Rerank the nine-domain queue from actual remaining operations and owner facts.
Forge preview and smith both select the existing five-entry price table with the
same ore-count validation, but smith selects/detaches ore before that validation.
Any proposal must keep this timing, rollback, active refusal and real producer
semantics. A thin table wrapper alone needs a concrete architectural benefit;
no new reservation is implied here. Template/stat/outcome capture and shared
native admission, receipt, publication and recovery remain their existing owners.
This handoff supplies neither primary adoption nor the continuing finish line.
