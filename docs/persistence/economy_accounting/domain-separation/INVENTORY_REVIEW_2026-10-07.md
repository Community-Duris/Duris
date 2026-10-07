# Coordinator assessment of operation inventory v1 - 2026-10-07

The coordinator accepts the bounded inventory and proposed R1 preparation seam
as the current independent implementation queue. Implementation and qualification
of R1/R2 remain outstanding. This is not a claim that all game domains are
converted, RAM-authoritative, or ready for accounting activation.

Reviewed inventory publication:
`92871c3dcb9bf90932eb102775ed9547022f4163`,
[OPERATION_INVENTORY.md](https://github.com/Community-Duris/Duris/blob/92871c3dcb9bf90932eb102775ed9547022f4163/docs/persistence/economy_accounting/domain-separation/OPERATION_INVENTORY.md).
Its source candidate is accounting
`a7b3181edb80bf188f616c39b8e7cb144e11cb18` and worker
`ce3b631003303ee4fbfb8a0bd527ad8508982250`. The coordinator's current accounting
revision `2dc200abfc565e6913471aba6bf676044a727464` adds review documentation;
the examined production source remains the same.

## Scope and independent source checks

The inventory covers all nine requested accounting-facing areas and distinguishes
existing owned-state preparation/evidence from remaining live capture and native
authority coupling. Inspection confirms the following relevant boundaries:

- Currency mutation preparation takes payload, balances and revisions;
  economic currency adapters build plans from owned native evidence. ATM
  deposit/withdraw use typed currency deltas through existing submission.
- Item transfer/craft accounting accepts typed payloads and snapshot spans.
  Live complete-root capture, UID/source proof and actual execution remain native
  owner work; an evidence adapter alone does not separate every producer rule.
- Shop trade plans and auction bid/listing/settlement/claim plans accept owned
  command/authority/result inputs. Their locked native result and publication
  obligations remain necessary and actively owned.
- Collector lifecycle policy accepts records/rules/time. Purchase/expiry
  preparation accepts immutable listing, item/custody and actor facts. R0 adds
  the collection singleton-image boundary and already has a completed review.
- Quest native selection/publication and bartender callbacks overlap the
  primary/quest-prep work. The published QP02/QP07 fixes are available sidework,
  not evidence of primary adoption or completed native/refund authority.
- Craft/Forge's shared quote still calculates counts from captured item facts
  and a global multiplier in `crafting_build_plan`. Preview and make callers
  already share this plan, supporting the proposed R1 seam.
- Active blackjack and unsupported paid compound outcomes have deliberate
  refusal contracts. Extraction must not reopen them. Locker identify persists
  its prepared currency command and can reuse R2 numerical preparation without
  changing its receipt/lore/recovery owner.

The required feasible set is R0 Collector image (delivered), R1 Craft/Forge quote
(outstanding), and R2 wallet-value/bank-payment deltas (outstanding). Further
item/shop/auction/quest/compound-service capture and authority coupling is recorded
as concrete owner-dependent future conversion work. It remains visible; this
finite sidework milestone does not claim those domains are fully separated.
Any expansion or removal of a required row needs an explicit reviewed inventory
revision. A required row that becomes blocked stays unfinished.

## R1/R2 review conditions

R1 may change only its plan type/header, local material quote/capture wrapper and
focused existing regression. Preserve the current wide arithmetic, ceil/finite
multiplier checks, material VNUM bounds, magical fact and unchanged output on
failure. Exercise the actual extracted implementation and wrapper. Preserve
preview/make agreement and the original sanitizer controls. Do not touch input
selection, native submission, progression, receipt/ACK or recipe persistence.

R2 is required but not yet a reviewed implementation reservation. Before edits,
publish exact helper/wrapper preimages and checks against current upstream.
The existing helpers distinguish positive reward decomposition from a wallet
spend, and bank spending consumes ascending denominations with wallet change.
Retain zero/INT64_MIN refusal, insufficient funds, actual native balance range
and overflow behavior, output guarantees and denomination order. Do not let a
broader numeric input type silently change accepted production facts or introduce
overflow beyond the former native range. Submission, identity, revision, fences,
accounting capability, receipt/publication and recovery semantics stay outside
this extraction. Use existing completion/payment tests where they cover real
callers; pure arithmetic proof alone does not qualify authority.

The primary may defer either compatible bundle at a normal integration boundary.
Unknown unpublished overlap remains unknown. These proposed reservations do not
require the primary to acknowledge them or wait. The continuing coordinator Goal
remains active pending implementation, exact revision qualification, review and
usable handoffs for the fixed delivery set and selected quest-prep follow-up.
