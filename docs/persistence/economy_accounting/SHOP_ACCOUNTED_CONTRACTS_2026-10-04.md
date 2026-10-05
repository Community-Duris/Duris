# Accounted shop contracts: source milestone, October 4

Status: source integrated and reviewed; native qualification and active shop delivery remain pending.

## Failure and change

The current native shop adapter moves only scalar item rows and static metadata.
Deleting a player row cascades its complete runtime payload; produced purchases
and keeper loading/saving also omit that payload. A transaction-only copy would
still be erased by ordinary keeper saves. This complete native path is being
implemented separately before active shop admission is enabled.

This prerequisite freezes the original player level and native save revision in
an explicitly accounted-only v6 payload. Default v5 builders and historical v1-v5
execution remain unchanged. The original source item blob remains a preimage;
a pure bounded helper derives only the existing purchase root's STOREITEM flag
and generated-key adjustment from the frozen original level/PID. Every UID and
parent/root relation must match exactly, and malformed or incomplete snapshots
refuse without replacing output. Native owners must independently prove both
locked item and status preimages; this codec supplies no admission permission.
The central legacy predicate rejects v6 before mutation while pure transient
construction remains available for schema-2 intent freezing.

The new v6 capability binds only native wallet/bank lifetimes and the already
agreed shared shop sink (ID21) or issuance (ID22), both context0. Existing v5
24-byte per-keeper treasury capabilities retain historical interpretation.
Shared v6 facts contain the two 8-byte lifetimes. Exact native denominations,
price, keeper/shop witnesses, item decisions and revisions remain required.
Zero-price purchases omit the unused virtual counterparty; existing wallet
normalization and native price rules are preserved. No per-NPC economic holding,
new policy version, UID, queue or item valuation is introduced.

## Review and remaining acceptance

Independent source review found duplicate/omitted/reparented source acceptance
in the first helper and a legacy-v6 execution bypass; both are corrected.
The zero-price unreferenced-account defect and version/fact binding are corrected.
Reviewed input pins are retained privately and in the central candidate registry.
Changed-line clang18 fixed points, source pin verification, JSON and diff hygiene
pass. Native compilation/tests/gameplay/persistence/recovery remain deferred to
this major plan's original qualification batch, as requested.

The native runtime-payload store, additive migration, complete keeper loader/save,
SQL/flat domain owners, status/source checkpoints, reserved publication/cold
recovery, genuine completion and guarded ACK are still required. Active shop
refusal remains. This source milestone is not an executable writer proof or a
completed shop journey; coverage_complete remains false and release blocked.

Plan5 backup slice94e81480b is imported and normally pushed in4c2abb329. Its three
files match exact peer blobs and Python AST passes. Peer evidence proves managed
present-receipt preservation, two native service boots and refusal cuts against
its frozen inputs. It does not qualify primary v3 origin/runtime changes or this
new shop source. Matching independent v3 discovery and combined qualification
remain open. The original inactive behavior and declined spell path are preserved.
