# Coordinator review of R2 currency value/payment preparation - 2026-10-07

R2 is implemented, connected, qualified at the scoped component/build/caller
level, published and independently reviewed with no actionable source defect.
Primary adoption and combined-candidate qualification remain unknown/separately
owned. This extraction does not implement RAM authority or native coin recovery.

## Exact source and compatibility

Reservation `82ef254e098fa39ece6a69659452fee08f5a0665` precedes implementation
`24fa551ae16900b41e509f79fe4685762e0fb2e9`. The three-file bundle adds
`src/economy/currency_value_plan.h`, replaces only the canonical-value,
wallet-value-delta and bank-payment-delta helper bodies in
`currency_transaction.c` with owned-input calls, and adds
`tests/async/test_currency_value_plan.py`. Actual production callers use those
wrappers; this is no unused strategy interface.

Native inputs remain `std::array<int,4>`, with compile-time range constraints.
Their denomination-weighted sum fits int64_t. Removing the former upper-overflow
guard preserves all defined bounded sums; its subtraction was undefined for
negative running totals. This adds no negative-balance admission policy. Preserve
positive actor-independent reward decomposition, zero/INT64_MIN refusal,
insufficient-funds refusal, ascending bank denominations, wallet change and
success-only output assignment. Bank costs above bounded available value refuse
before ceil arithmetic. Native mutation continues to enforce actual balance
validity. Submission, identity/revisions, capability, native/accounting effects,
publication, ACK and recovery are unchanged; locker lore/receipts remain owned
by the existing service.

The coordinator independently inspected the source diff and reservation, passed
`git apply --check` against accounting `d91f59af06239a5736d10498b89091699c5e05c6`
without applying it, and passed whitespace checks. Confirm current preimages and
unpublished primary work at import time. Import R2 independently; R1/R0 are not
prerequisites for its three-file patch.

## Independent checks and inspected terminal qualification

In the separate clean review checkout at the exact implementation, the
coordinator passed the same sanitizer numerical regression against both the
retained parent helper preimage and extracted production wrapper/header:
`DURIS_CURRENCY_VALUE_PREIMAGE=<private-original-file> python3
tests/async/test_currency_value_plan.py`, then the same runner without that
variable. Original expectations, sanitizer flags and deadlines remain intact.
The actual owned implementation also has direct range/refusal/sentinel controls.

The worker's original `test_currency_transaction_contract.py` has eight passes,
one stale source-shape error in untouched `critical_command_repository.c`, and
one writer-census failure for twelve existing `coin_physical_recovery.c`
assignments. The coordinator independently executed this unchanged runner on
the accounting source and R2: both fail with exactly those error/failure
identities. They are retained baseline limitations, not an extraction regression
or a full-runner pass.

The coordinator inspected original maintained development build logs, terminal
exits and source pins. Each backend recompiles one provider,
`currency_transaction.c`, with original C++20 flags, then links all 740 server
objects. Flat defines `__NO_MYSQL__`. Both make commands exit0. These are
incremental builds from the worker's qualified R1 objects, not fresh 740-provider
compiles. A trailing-CR hash-read failure occurred after successful flat build;
the corrected direct read is retained. Read-only copies of both terminal binaries
from the stopped task container independently match:

| Artifact | SHA256 |
|---|---|
| SQL `dms_new` | `fad42d11f52857a6ee79f9af28f4c2ab73f34f84050476576a1bd3b56866f2f5` |
| Flat `dms_flat_new` | `17aa1f48f932fcfb460817b66d0fffb5164a6c4661df23661bc2cedba2749ccd` |
| Owned header | `a043b439dd46a25974268f5a36ca77d553fb561760afe0a3d8af48977afb295f` |
| Transaction source | `075a07dc357c11ea2c8f2fe87c02333c8213c37a3ea7d8424838f4a50dda90d7` |
| Numerical runner | `e050407ceb90048bf6ae547e9b41354673664e17e1b5ef9d094a6bc5cbf5544b` |

All three source hashes match exact published Git bytes. The private evidence
index matches every retained proof file. Container/image and isolated output
volume are those recorded in the [R1 review](R1_CRAFTING_REVIEW_2026-10-07.md).

The original full completion runner fails link before cases. A private launcher
adds only existing native-birth/Collector/recovery providers, selects the original
42 non-coin scenarios per backend and installs abort guards for two excluded coin
endpoints. Inspection confirms unchanged selected assertions, sanitizers and
deadlines. The terminal log contains exactly all 42 selected names on MySQL and
flatfile: **84 PASS**, no duplicates/missing cases. Exact selected/excluded names
and nine added providers are in the worker's
[terminal handoff](https://github.com/Community-Duris/Duris/blob/4f7fa384b09914094a56cd8040e6f9c1588f92a4/docs/persistence/economy_accounting/domain-separation/HANDOFF.md).
Twenty-five coin scenarios per backend are excluded; neither guarded endpoint
was reached. This qualifies production-linked wallet/bank/payment completion
behavior with fixture authority seams; it does not qualify DB transaction
execution, native physical coin recovery or the complete shared runner.

The private currency-adapter run passes both sanitizer configurations after
adding only the existing recovery-manifest provider; its original runner's
missing-provider failure remains. Locker-identify components pass, without a
claim of a real server payment journey. Initial private setup/type-name/link
failures remain retained. No shared runner/owner was repaired or weakened.

Disposition: available for optional primary review/import at its normal boundary.
The fixed extraction requirement is discharged at this stated scope. Remaining
native/shared fixture qualification belongs to primary integration and does not
become a new architecture gate. No accounting release/activation gate is closed.
