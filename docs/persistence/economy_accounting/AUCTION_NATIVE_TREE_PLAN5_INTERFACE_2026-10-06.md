# Auction native tree reader handoff — 2026-10-06

Status: private source interface, compiled in both original753-provider
production profiles on final source30837d50; early admission is native-unit
qualified and runtime qualification remains open.
See [current qualification](SHARED_PROFILE_BUILD_QUALIFICATION_2026-10-06.md). This describes
the unpublished auction payload version2 candidate. It does not activate a
writer, change existing version1 facts, or complete a release gate. Plans1–4
own the producer and historical SQL proof; Plan5 owns independent audit and
reconciliation of the eventual integrated candidate.

The original native auction can select several roots, including complete trees
under later selected roots. Each root must retain its own complete literal.
After command/journal retirement, the original listing's stored EAI facts anchor
the combined selected forest. A checksum of a custody blob alone cannot prove
that it is the original listing's literal.

## Stored native facts: ANF2

Exactly124 little-endian bytes append to the original typed facts for native
payload version2. EAI admission `facts_version` remains1; the extension itself
has an explicit version2. Listing has a16-byte original prefix, total140 bytes.
Item claim has an82-byte prefix plus27 bytes per claimed root, then the extension.
Original payload version1 facts remain byte-identical.

| Offset | Field | Encoding |
| --- | --- | --- |
| 0 | ANF2 magic | uint32 `0x32464e41` |
| 4 | Extension version | uint16, exactly2 |
| 6 | Reserved | uint16, exactly0 |
| 8 | Original actor level | uint32 |
| 12 | Acknowledged save revision | uint64 |
| 20 | Complete BEFORE digest | 32 bytes |
| 52 | Complete AFTER digest | 32 bytes |
| 84 | Combined SELECTED digest | 32 bytes |
| 116 | Selected node count | uint32 |
| 120 | Selected root count | uint16 |
| 122 | Reserved | uint16, exactly0 |

Digests are SHA256 of the exact original `player_item_snapshot_list_encode`
bytes, with no additional hashing domain. Root count is1..9; selected node
count is root count..4096. PC level is1..255 and save revision is nonzero.
That is the structural wire/read-only facts bound. Admitted native LIST/CLAIM
accounting is limited to3,000 selected nodes, one event per node. The private
308 successor enforces this before SQL mutation; its original native boundary
and both production builds pass. See [qualification](AUCTION_NATIVE_ADMISSION_CAP_QUALIFICATION_2026-10-07.md).
A decodable4,096-node observation grants no admission authority.
Exact length, version, reserved fields, counts and canonical facts are required.
The extension contains neither the full forest nor the ordered BEFORE UID list.

The source-only observation APIs are
`auction_listing_accounting_observe_native_facts` and
`auction_item_claim_accounting_observe_native_facts`. Both accept the original
`economic_frozen_intent` and return `auction_accounting_native_facts` values.
They grant no SQL authorization, original command-header reconstruction,
publication authority or ACK permission.

## Per-root custody literal: ACT2

Each original root custody blob contains an ACT2 header: uint32 magic
`0x32544341`, uint32 node count and uint32 canonical-list byte length. This is
followed by ordered pairs of uint64 UID and uint64 original listing-after native
revision, then exact canonical full-root list bytes with root-relative parent
indices. There is no padding, reserved field or trailing data. Each UID matches
the same-position literal. UIDs are unique, nonzero and not the sentinel;
revisions are nonzero and not the maximum. Revisions come from each actual
locked native item revision plus1.

The complete list is bounded by the original4MiB snapshot limit. ACT2 framing
adds12 bytes plus16 bytes per node. This body is held in custody, not inlined
into the original384KiB accepted-command payload.

The pure `auction_repository_decode_native_tree_blob` returns values and paired
listing revisions. Decode success alone does not authenticate the source.

## Independent verification boundary

Authenticate the actual original successful listing inbox/root receipt,
canonical EAI/EAP hashes and writer/version before interpreting these formats.
Combine every original listing root in its original slot order, rebase parent
indices, encode the combined canonical forest and compare its digest and counts
against original ANF2. Verify every UID's native ledger, EAP item event/reference,
owner witness, parent and original listing-after revision. Do not infer missing
original command headers from stored hashes.

A pickup can select a legitimate subset of original roots. Authenticate the
whole original listing first, then return the selected subset in the exact
claim order. Current escrow custody must match for selected nodes; other roots
may already have been claimed and remain historical evidence. Apply the
original lineage/book and entitlement rules without inventing epoch equality.

Historical reader, full retained verifier, SQL leaves and native owner are now
composed privately. Plan5 should retain existing version1 support and mark this
interface as pending until the
producer source, reader and original runtime evidence meet on one qualified
candidate. Current coverage and release status remain incomplete/blocked.

## Source evidence

Private source packets reside under the owning review worktree's `tmp/`:
`auction-native-intent-facts-primary-20261006` (ANF2 layout/source),
`auction-repository-full-tree-primary-20261006` (ACT2 layout/source), and
`auction-native-leaves-primary-20261006` (listing transaction).
These generated packets stay local. Current integration status belongs in
[shared producer progress](SHARED_PRODUCER_INTEGRATION_PROGRESS_2026-10-06.md).

## Private recovery proof handoff

The composed NAR has a distinct `restored_after_proven` stage (enum3) for an
authentic passive player slot rebound to the actual loader, after current SQL/
world AFTER proof. Entering it preserves every original native callback state;
it does not claim that pre-restart callbacks returned. Phase2 held ACK retry is
separate from the genuinely released continuation/notice capability.

The private scalar `ownership` effect is serialized immediately before balances
and notice. It records actual once-only current owner/custody-cache hydration for
successful BID/FINALIZE/REMOVE, including actorless settlement. Original LIST/
CLAIM use their per-root runtime records. MONEY and rejected commands leave this
effect unused; actorless balance publication remains forbidden. Both changes are
unpublished source interfaces, pending full combined qualification.
