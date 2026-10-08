# Native quest fee value component — 2026-10-08

This implements the two-file component reserved by
[the fee-only boundary](NATIVE_QUEST_FEE_ONLY_OWNER_BOUNDARY_2026-10-08.md),
commit `ee981c97ac48f6c6a3a5dfb82cbbd8b13177e31e`. It exercises actual maintained
providers on explicitly modeled values. No genuine native lifetime, source,
retained carrier, hold, SQL operation, world census, publication, ACK or retirement
authority is established. The continuing native Goal remains **BLOCKED**.

## Pins and owned bundle

- Public candidate: `cda8aa6f65c72121e92d7c07efae7e91164d1050`.
- Source tree: `833d3085815b396861ad18a77635412212381e4b`.
- Migrations source tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`;
  no applied schema or database observation.
- Preserved prep parent: `ee981c97ac48f6c6a3a5dfb82cbbd8b13177e31e`.
- Component code/test commit: `b478374e0c9b17a999ff6377c919d6a5e56fc2a7`.
  This handoff follows as a separate documentation commit on the same prep branch;
  its publication SHA accompanies delivery. Import the code commit for both tests.

Owned files are only:

| File | Purpose |
| --- | --- |
| `tests/async/quest_accounting_prep/native_quest_fee_owner_boundary_test.cpp` | Actual fee/continuation/result/context/pair/cash-transition providers on modeled values and corruption controls |
| `tests/async/quest_accounting_prep/test_native_quest_fee_owner_boundary.py` | Maintained guarded build recipe, unchanged original context executable, separate fee executable, bounded sanitizer runs and retained receipts |
| This handoff | Exact qualification, pins, evidence and remaining native boundaries |

Production, existing tests, original QP03 controls, shared recipes, manifests,
registries, schemas, canonical `HANDOFF.md` and the finish plan remain unchanged.
The older prep checkout's production source is not the tested source. This
component does not qualify the primary's separately owned private candidate.

## Real provider closure and scaffolding

The runner reads `SOURCES`, `FLAGS`, `LINK` and the 20-second runtime deadline from
the maintained `tests/async/test_native_quest_recovery_context.py` without changing
that recipe. It builds and runs its original C++ fixture unchanged, then builds
the separate owned C++ fixture using the same provider nodes and strict flags.
The general payload decoder additionally requires the already known real providers
`src/economy/native_quest_cost.c`, `src/economy/native_quest_coin_give.c` and
`src/item/lockpick_retirement_continuation.c`. These are direct compiled bodies,
not copied/extracted codecs or selector stubs. Full per-command argv and results
are retained externally. Both final links passed with the 17 real providers and
one fixture translation unit each; the fee link reused all 17 authenticated provider
objects. There is no unresolved provider or added accepting seam.

The separate fee executable uses actual `quest_mobile_native.c` transition/image
providers, item-transfer and critical-command codecs, QRF6 encoder/decoder and
trigger/reward-source helpers, cost projection/codec, context/pair/fee-ACK validators,
native reference, snapshot, recovery forest and accounting-intent providers.
The maintained allocation observer exists only in the original executable and
delegates to `__real__Znwm`; the fee executable omits that wrapper link flag.

The owned fixture's scaffolding constructs literal numeric IDs, snapshots,
cash, receipts and stage values, then calls actual providers. No accepting
ownership/export/SQL double, new façade or authority capability exists. No provider
was text-extracted or replaced. Native capture/birth/admission, SQL, world
publication/rebind, physical/reward ACK execution and journal retirement remain
uncalled. A codec-valid physically-proven stage is still a modeled value.

## Executable acceptance scope

The fixture uses an intentionally synthetic mixed-reward contract: modeled
mobile70023/PID10/native instance9000, C25, two item70018 rewards, C17, group XP
and a frozen skill flag. This is **not Gorblag's production recipe**, whose
verified source contract is C5 and one70018 reward with no XP. No Gorblag setup,
funding, ingredient producer or native reachability is claimed.

| Controls | Actual component observation |
| --- | --- |
| Original pair and forests | Accounted v12 acceptance + zero-root v14/NQF2 fee payload + NQR5 attachment + v6/QRF6 continuation; full frozen child command bytes, typed48 parent receipt, unrelated inventories and canonically encoded empty native/player forests |
| Intentional sequence inequality | Fee sequences parent+1 and parent+4 both satisfy the maintained trigger/pair rules; original birth/lifetime/slot/PID remain modeled caller facts |
| Cash/stock/custody | Actual projection of one gold minus25 copper produces7 silver/5 copper; cash17->18, mobile advances once, stock/native custody8/player custody4 and literal forests stay unchanged |
| Payload/context refusals | Wrong versions; consumed roots/blob/UID/multi-root, illicit child parent marker/hooks/messages, frozen D, PID/mobile/VNUM/generation/slot/sequence mismatch; strong provider output preservation |
| Parent/continuation refusals | Wrong original parent identity, typed48 root/count/clocks/corpse/collector flags, action/source/slot, missing/changed XP-credit terms and frozen child-byte/phase/revision mismatch |
| Projection semantics | Charged/change, valid nonpositive and insufficient attempts, unchanged maximum revision when no charge, invalid denominations/order/overflow/revisions and corrupt transport; no blanket refusal of valid outcomes |
| Fee result and ACK values | Canonical NFR1 and all five clocks, size/reserved bytes/padding, full receipt/outcome/disposition/phase/revision mismatch; applied/already-applied compatibility; terminal failure uses pre-charge clocks and no reward continuation |
| Recovery boundary | Captured receipt-absent value and started-but-unreturned publishing value can encode, while terminal pair validation refuses them; this supplies no permission for cold replay |
| Rewards | Actual literal roundtrip of duplicate item ordinals, cash, group XP and frozen skill metadata; distinct actual logical reward-source IDs; a different valid literal stays distinguishable but unauthenticated |

All coherent positive pairs are explicitly unauthenticated. No SQL stored literal,
reward-obligation readback, economic/XP ACK masks, refund, current custody census,
owner release or actual paired retirement is invented or asserted.
The empty fee-native case models later original-branch values with a higher stock
clock; it does not execute or prove the intervening history.

## Commands, results and retained evidence

Isolated composition:
`D:/Dev/Builds/Duris/native-quest-fee-component-20261008/candidate`.
Build outputs, immutable object cache and per-attempt receipts are beneath its
`bin/`, as required by the maintained build helper; `BIN_ROOT` names that same
directory. Temporary/compiler files use `D:/Dev/Temp`. Task-local source/command
evidence is `D:/Dev/Temp/native-quest-fee-component-20261008`.

From the composition in Ubuntu22.04 WSL:

```bash
export TMPDIR=/mnt/d/Dev/Temp
export BIN_ROOT=/mnt/d/Dev/Builds/Duris/native-quest-fee-component-20261008/candidate/bin
export DURIS_REGRESSION_BUILD_CACHE=/mnt/d/Dev/Builds/Duris/native-quest-fee-component-20261008/candidate/bin/native-fee-cache
export DURIS_QUEST_FEE_CANDIDATE_REVISION=cda8aa6f65c72121e92d7c07efae7e91164d1050
timeout 900 python3 tests/async/quest_accounting_prep/test_native_quest_fee_owner_boundary.py
```

Qualification used the task-local `run.py` controller to execute that same runner
and observe/delegate original compiler subprocess calls unchanged. Maintained
compile-command bounds are 600 seconds; each executable retains 20 seconds, original
C++20/O1/debug/strict warnings/Werror/ASan/UBSan/frame-pointer/no-PIE flags, crypto/zlib,
GC sections and sanitizer halt/leak settings. No assertion, calibration, timeout,
provider behavior or shared recipe was weakened.

Final result: **PASS — component/model scope only**. GCC11.4.0 in Ubuntu22.04 WSL;
all final compile/link calls exited0, both executable runs exited0, stderr was empty
and source bodies remained unchanged. The final complete attempt has exactly two
component runs and no native journey.

| Final executable | Result | Build / runtime elapsed | ELF SHA256 |
| --- | --- | --- | --- |
| Original maintained context | All original controls passed unchanged | 219.974s / 0.406s | `dbd89c987757cf988ebea43612f15223e9316e84d67464464d1247e83714f1d0` |
| Owned fee component | 103 named controls passed; explicit owner/SQL/journal/native flags false | 101.483s / 0.220s | `f5fb4bde63eb1a0a88f64f4299334ba9d03463a4b15fe48f9c74d2814792a1db` |

Final receipt: composition `bin/tests/native-quest-fee-component-n22ro2oh/receipt.json`,
SHA256 `11e82aa452a9cf263c6b608c5e48865ebf1370130685160f913fbb374d3b51ed`.
It records every direct source SHA, compiler version, original flags/link, both
literal binary paths/hashes, commands, elapsed times, sanitizer settings and scopes.
Task-local `artifact-index.json` seals 155 retained files, including all observed
compile/link argv/output, original failures, exact owned historical fixture bodies,
receipts/logs and immutable binaries; SHA256
`8749ce91da50b4bef9acd3515d6999790e7824742d0f083934a55c483af431b2`.
`qualification.log` retains the final build/cache statistics and terminal exit0;
no timeout occurred. Exact owned Git bodies were also checked against the final
receipt's source hashes after the code commit.

| Final source/evidence body | SHA256 |
| --- | --- |
| Owned C++ fixture | `e2a21905460b76380050b152ea6dcacd235c93d9fcf9305725953677c91e399a` |
| Owned Python runner | `38fc13dcfee4f718a3d525d3b39ced6fba48e04b494689e92d56eee92a9dacf7` |
| `source-manifest.json` | `2b154e09f2c7e19b0bc6c3395b5ad1440075d3554e8794ccba9b836d5ca1764c` |
| `source-proof.json` | `2f761cde38b2a2e7baf9dec52b9bb6965bf550e77d10796c6223a9f7c89a8d94` |

Source authentication checks all 2,818 public Git blobs plus both owned test bodies
against the actual composition, including post-run verification. `source-manifest.json`
records each body/blob/SHA256; `source-proof.json` records the result. No generated
provider/fixture source exists: the C++ fixture is the exact owned source body.
Original failed attempts remain separate from the final receipt: initial guarded
cache refusal occurred before compilation; the first linked fee fixture then failed
because its modeled parent omitted required self-correlation. The correction adds
the actual required parent marker while the fee child marker remains zero. This is
an owned fixture construction correction, not a production defect or codec bypass.
The first linked fixture/runner bodies match their recorded original SHA256 values.
The second linked fixture exposed another owned expectation error: `NQF2` is
the fee payload tag, while the recovery attachment tag is `NQR5`. The final
fixture asserts each actual tag separately. Both initial failures and their
exact fixture bodies remain retained; no production/provider behavior changed.

Formatting uses `scripts/format.sh --check --file` for the new C++ file;
`git diff --cached --check` verifies the exact staged bundle; both checks passed.
No full server build,
database/service, journal read, native/gameplay/Plan5 batch, migration or activation ran.

## Native integration boundary

The component is independently importable in the reviewed source family without
shared production changes. Authentic native fee acceptance still needs original
retained parent/child carriers, real funded native cash/origin and triggering item
acceptance, genuine held inventories/save stage, original SQL historical proofs,
current native/world/custody census, same-session cleanup, actual physical/reward
ACK and actual paired retirement observations. Existing internal owners already
implement these boundaries; safe evidence exports/setup remain unproven.
Disappearing fee branches stay refused. Final native qualification remains at the
primary's integrated-candidate batch boundary. This finite component delivery does
not resume or complete the blocked native Goal and creates no primary adoption wait.
