# Full-player shop native publication source checkpoint

2026-10-04. Published parent `abd6e32cd0ba41e0fdbcd0b2cd0ee5bbba847cd7`.
**Private source foundation, independently source-reviewed; UNQUALIFIED.**
No candidate source or schema in this report is represented as integrated into
the public branch. No compiler, tests, SQL, services or gameplay ran for these
inputs. Original major-plan qualification and R1–R8 remain required.

## Established gaps and corrected source

Shop submission replaced the original whole-player checkpoint bytes with its
command. The selected trade payload cannot reconstruct every unrelated equipped
or carried item. The same original player slot now retains both that actual
checkpoint and the exact command; combined bytes count against its existing
32 MiB aggregate limit. Allocation and capacity checks precede hold installation.
Definitive refusal and guarded consumption clear the entire exact slot. A private
friend-only value reader binds the original token, operation, encoded command,
captured/acknowledged revision and level, with strong output preservation.

The complete player native image reader verifies every expected saved EQ/INV
UID, owner, root, parent, slot, VNUM, actual revision, physical row/parent ID,
canonical sidecar and native represented fields. Extra roots, foreign children
and duplicate physical copies refuse. Empty inventory is valid. Unphysical
inline coin bodies remain under the existing player repository separation;
claimed UIDs and forest references cannot hide behind that separation, and all
physical player rows remain counted.

Initial reader source incorrectly required every item's string mask to be 15.
Actual player capture uses full strings only for the selected carried subtree;
equipment and unrelated inventory retain their original save policy, and a
root-zero buyer has no selected carried tree. The final reader accepts each
actual valid captured mask while preserving exact stored sidecar and physical
equality. It does not promote generic fields to literals. The first correction
matched the keeper function; independent review caught it. Final source restores
the keeper's original mask-15 requirement and changes only the player reader.
Original reader/correction inputs remain preserved privately.

The native projection's original keeper custody cut omitted physical UIDs with
foreign or nonzero-context custody. Its later keeper helper could then request
custody after taking physical locks. A bounded nonlocking keeper ID/UID routing
census now contributes to the one globally sorted custody cut. Missing, foreign
or nonzero-context routed custody refuses before physical reads. Locked exact
physical membership is rechecked before the keeper helper can discover new UIDs.
Complete player custody joins the same cut before either full physical image.

The private implementation's `current_native_image` obtains the original player
body, derives the full expected current forest and consumes the native reader.
Successful sells remove only the exact original selected tree and reindex every
unrelated retained node; an unselected orphan child refuses. Successful buys add
the frozen v6 after-tree at the original destination. Rejection keeps the original
forest. The existing after-image codec supplies the original level-dependent
transform. No prototype defaults, fresh UID or current-level decision is invented.
After the complete current native locks, the borrowed historical verifier proves
the original inbox/root/plan/item ledger/outbox and compares the whole sealed
result array, size, error, failure stage and durable revision. Same original
session/transaction is rechecked before returning values. Advanced native money
and owner counters remain current values; historical vectors are not replayed.

## Exact private source inputs

Paths below are relative to their named private `tmp/.../candidate/` proposal.
These raw SHA256 values identify source review inputs, not a compiled closure.

| Proposal | File | SHA256 |
| --- | --- | --- |
| `plan4-shop-player-checkpoint-proposal-20261004` | `src/player/player_save_pipeline.c` | `c5ade31ccb69499bf7de0af69e7a7b9f32e0cc5611e282b2c7737ef9305e52d3` |
| same | `src/player/player_save_pipeline.h` | `92b2e0269db23592a76063c90e82ad745897f0e43d5b91ba41d5819672739d84` |
| `plan4-shop-current-player-image-proposal-20261004` | `src/persistence/shop_item_runtime_payload.c` | `699a94dd435e41c8d7c3572082275dc042e2944ef1dab22c78cf43b6c0d24893` |
| same | `src/persistence/shop_item_runtime_payload.h` | `1b266309481649aa134b871188067c91891a7378a47ec298f020991914ffaacc` |
| `plan4-shop-native-publication-proposal-20261004` | `src/persistence/economic_sql_shop_trade_transaction.c` | `829e563a49121bb6994de0bd5610f6390384134479c01b879b57d824fd201eb4` |
| same | `src/persistence/economic_sql_shop_trade_transaction.h` | `3af6637628537e9568c244e5e6068617fa5237252a307a763d1d13a2068c4bed` |
| same | `src/persistence/critical_command_repository.c` | `4467db73aad1f6806dcec2d197dca39dd66f3d43a0f54dc0d3f6885148bc56d8` |
| same | `src/persistence/critical_command_repository.h` | `2f5938b071d513bef52446003539b0a5e3eff19ace29d0db58dd3b36fada5f50` |
| `plan4-shop-domain-preparation-proposal-20261004` | `src/economy/shop_trade_transaction.c` | `16fcdcd0c66b6e361f52ea7de28b9d4a90bf60ddbaaf0e53f09f7bd294678fb4` |
| same | `src/economy/shop_trade_transaction.h` | `f9216bd4b31bc8c0fb8e338b35e3dd5e55ce93f694de0f9e8cab4d84c2b216ea` |

Independent database source review accepted original-body retention, full-forest
derivation, authentic retained-receipt comparison, complete custody ordering and
the final correctly scoped string-policy repair at these pins. Changed-line
clang18 reaches fixed point; raw input/candidate/delta readback and whitespace
checks pass. These are source checks only. The combined keeper/player/producer
foundation still depends on the private interfaces and pending additive schema
recorded in [the native admission handoff](SHOP_NATIVE_ADMISSION_HANDOFF_2026-10-04.md).

## Remaining integration and parallel handoff

The actual native publication consumer must retain this cut through full live
graph comparison, original physical callback/reentry stages, current balances
and custody publication, confirmed original lease cleanup and guarded ACK. Cold
replay registration/current-image observation, actorless refusal continuation,
keeper/UID writer fences, lifecycle and the gameplay driver remain unfinished.
Restored slots do not invent a lost live body or token; their recovery must use
the original command and verified native materialization. No new durable body
store is specified here. Flat parity and original major-plan gameplay,
persistence/recovery and coherent schema qualification remain required.

Plan5 may continue unaffected published-schema work. Its current branch remains
`6a2428cff3031e1a8d384644667910349cbddbc3`; no new peer slice was available at
this refresh. This checkpoint adds no release gate or production authorization.
Plan3's independent source stream is tracing the required addressed NPC lifetime,
ordered stock and restart handoff for sequential offerings; no native participant
implementation or quest qualification is claimed. Accounting remains inactive
under current safety gates and release remains **BLOCKED**.
