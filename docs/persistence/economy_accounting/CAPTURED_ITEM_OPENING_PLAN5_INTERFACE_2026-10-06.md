# Captured SQL item opening: Plan 5 interface

This handoff records the producer preimages for independent reader qualification.
It does not complete physical source coverage, activation or a release gate.
The captured-item producer remains in private qualification candidates; the
maintained 740-provider branch does not yet contain that complete producer.

## Source authority

The primary owner checked the actual lifecycle source in both frozen candidates:

| Candidate | Source archive SHA256 | Evidence scope |
| --- | --- | --- |
| Auction/native opening | `77ca3da7d89ac882a1999a80af75aaf2c4ae1d2a2cfb6e50b247669f3ca71e71` | Both original 747-provider builds and 18 original native cases pass. Full independent canonical audits pass on both genuine money-recovery fixtures. |
| Complete startup predecessor | `3c7e646bcddd4b9e05c5e7ce9aab4940d3fe84e9c6f23cf2191b457ae2dead4b` | SQL build fails on an incomplete private entry type instantiated by an inline constructor. The failed result is retained. |
| Constructor-only startup successor | `113824dc58cc7a06c69164a78914e35395470b173c8427cb91b5d356e34a5db7` | Default noexcept constructor moved out of line after the complete private entry. Full builds and genuine cold-world/journal qualification remain pending. |

In `src/persistence/economic_sql_accounting_lifecycle_transaction.c`, the
`read_opening_items`, `native_digest` and `make_batch` function bytes are
identical between the auction and startup predecessor. The constructor-only
successor leaves this entire lifecycle source unchanged. The primary authentication is
`tmp/auction-maintained-source-claims-primary-20261006/CAPTURED-ITEM-INTERFACE-PINS.json`.
The startup successor's complete member/mode manifest is
`tmp/native-complete-boot-owner-constructor-primary-20261006/source-pins.json`,
SHA256 `906c152568a727e70693cee41b80059c18e5ef3870a9db75ef1bf84bd9169962`.
The original native receipt is
`bin/tests/auction-captured-opening-native-primary-20261006/money0062-fixture-successor/NATIVE-HANDOFF.json`,
SHA256 `0e659ccb491f545ea8c69466bf8c3bd8d13161ced6344b7c8a3d5a4546b8e90b`.
These are private reproducible evidence paths, not committed artifact payloads.

## Exact preimages

All hashes below are SHA256. Tags are the four raw ASCII bytes shown.
`frame(bytes)` is an unsigned 64-bit little-endian byte count followed by the
raw bytes. `u64(value)` is an unsigned 64-bit little-endian value. Hashes are
32 raw bytes inside their frames, rather than hexadecimal strings.

| Binding | Complete preimage |
| --- | --- |
| Per-item source, EBS2 | `EBS2 || frame(native ESR row digest) || frame(observed equipment row digest) || frame(resolved owner-revision ESR row digest)` |
| Native boundary, ESN5 | `ESN5 || frame(original ESN1–4 hash) || frame(item_current_owner content_digest) || frame(item_owner_revision content_digest) || frame(observed equipment projection content_digest) || frame(snapshot.item_sources_digest)` |
| Complete coverage, EIC2 | `EIC2 || frame(original holding coverage digest) || u64(item_count) || repeated [u64(item UID) || frame(EBS2 digest)]` |

EIC2 repeats the producer's normalized native-item order. Each entry must
correspond to the same captured item and resolved source rows; readers must
not independently reorder only one side of the binding.
`snapshot.item_sources_digest` is the existing EIM1 physical-item source
projection digest. The observed equipment row/content digests above are not
the EIE2 registry digest. The resolved owner revision must equal the captured
item's observed owner revision.

ESN5 deliberately binds the native projection inputs. Full ESC2 capture
equality remains a validation prerequisite, but ESC2 also includes receipt
and mapping rows changed by installation and is not the ESN5 preimage.

## Compatibility and independent acceptance

Empty native-item openings retain the original coverage and ESN1–4 bytes.
The EAB1/EAB2 witness layouts, existing ESD1/ESR1 row grammar and lifecycle
request V1/V2 selection remain unchanged. This slice introduces no EBC2 tag,
legacy backfill or new opening authority.

Plan 5 owns independent reconstruction and qualification of these three
bindings from original captured evidence. Required checks include the exact
nonempty preimages, item/source-row correspondence, observed zero equipment
slots, original empty compatibility, and refusal when any bound native,
equipment, owner-revision or physical-source evidence is absent or changed.
Existing full-forest, UID reservation, witness count and retained baseline
receipt checks continue to apply. A missing original preimage remains unknown
or refused; the reader must not fabricate it or auto-correct history.

Producer qualification still owes the genuine nonempty installation and
retained replay, stale first-activation refusals, complete source ownership,
cold restoration and applicable gameplay/recovery gates. The 18 component
passes above do not by themselves prove those complete routes. The primary
owner retains shared producer, coordinator, activation and registry ownership;
Plan 5 retains independent reader, reconciliation and release qualification.
