# Original actorless shop refusal and v6 literal callback — source checkpoint

These private source changes are reviewed, unqualified and not integrated into
production routes. Current published checkpoint is30b24413e. Published native
tree remains e0185879; no activation or declined inactive spell change is made.

## Established missing path and implemented correction

The private domain handled completions only when find_player_by_pid returned a
body. A disconnected player therefore kept an exact never-admitted execution
hold. Calling the existing shop_trade_completion with NULL would immediately
return, skipping original produced-item/sequence cleanup.

The existing bounded pending entry now independently retires the exact original
hold through the private pipeline/coordinator cancellation owner when no actor
exists. Cancellation still requires its retained never-admitted command and
canonical receipt; it never invokes native publication, claims economic rollback,
or emits guarded success ACK. The original preparation, command, payload,
notification callback and produced continuation remain retained for player_ready.
No new queue, timer, ownership catalog or durable format is introduced.

A retired latch prevents a second hold cancellation. Any changed post-retirement
receipt, including queued/started/completed timing, blocks; an identical redelivery
keeps the original completion unchanged. Existing committed applied/already-applied
equivalence stays intact. A returning body follows original failure notification.
Missing actor or uncertain cancellation never discards the original continuation.

## Literal callback integration dependency

Source review also identified that generic v5 item matching cannot compare an
ordinary mask0 live object with a v6 full-literal selected snapshot. A private
shop.c candidate now dispatches its four selected-source/failure-cleanup checks
to the existing accounted literal matcher for v6 checkpoint fields. v5 uses its
original generic matcher. Never-admitted produced cleanup compares the original
literal payload without requiring the returning actor's current level.

The successful after-image comparison captures full literal fields for v6 and
requires the actual level to equal the original frozen level before applying
native STOREITEM/zero-key transforms. Descendants and all other fields stay exact.
The existing v5 capture/transforms remain unchanged. The signed actual level is
explicitly normalized for comparison with its bounded unsigned frozen value.
This callback adaptation alone does not prove custody, native current balances,
physical uniqueness, original-session cleanup or publication ACK.

## Frozen source evidence

- Domain C:`61c77868667b8440a3dfc02958f7271674d813961ed65e393ccd5559f23c2938`.
- Domain H unchanged:`f9216bd4b31bc8c0fb8e338b35e3dd5e55ce93f694de0f9e8cab4d84c2b216ea`.
- Domain original preimage:`16fcdcd0c66b6e361f52ea7de28b9d4a90bf60ddbaaf0e53f09f7bd294678fb4`.
- Callback C:`e0ee8533b3ec66949f96a11faf9b4ba68239e2fd81e26f962e3c86f050b780d3`.
- Callback original C:`26633d0ee7d935f80abbab036a5ef35166ebc618cde8f20cbaff588552602db6`.

Sources/preimages and receipts are retained in
`tmp/plan4-shop-domain-preparation-proposal-20261004/` and
`tmp/plan4-shop-accounted-callback-proposal-20261004/`.
Independent final source reviews, changed-line clang18 fixed points and whitespace
checks pass. First formatting path-depth failure and reviewed timing/signedness
corrections are not native test results. No build/tests/SQL/services/migrations,
gameplay or recovery execution is claimed. Major-plan testing remains deferred.

## Remaining delivery work

Complete actual native shop publication under the original reconnect-off lease,
full-world before/after uniqueness and forest witness (including nested produced
items with mixed saved/literal policies), current custody/owner/shop cache and
wallet/bank/cash projection, native signed row IDs, confirmed same-session cleanup,
guarded ACK, replay/cold startup, producer/writer/lifecycle integration and the
original backend/gameplay qualification remain required. Plan4 and R1–R8 are
unfinished; no release gate is waived or replaced with this source checkpoint.
