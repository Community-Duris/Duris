# Versioned baseline equipment repair handoff — 2026-10-05

## Established original-contract defect

[Plan5's exact frozen handoff](PLAN5_BASELINE_EQUIPMENT_FORMAT_HANDOFF_2026-10-05.md)
is imported unchanged from `95273da0514ac4a141bfd40ec4d3e428b69b3575`.
Its production-API probe demonstrates that EAB1 drops equipment_slot, so decoding
an equipped accepted baseline regenerates a different EAP1. Carried/equipped
otherwise-identical input can also share the old prepared fingerprints and
command bytes. The independent reader correctly refuses the equipped original
root mismatch; retain that refusal. These are frozen component and fixture
retention results, not current combined server/gameplay/recovery qualification.

This contradicts the original complete-baseline/replay contract. Earlier Plan1
acceptance remains historical evidence for its recorded scope; this baseline
defect and its repair qualification are now open. No extra independent gate or
full NPC state ledger is introduced.

## Primary-owned frozen native candidate

Five implementation inputs are source-accepted: economic_baseline_adapter.c/.h,
economic_baseline_codec.c, economic_sql_baseline_transaction.c, and
economic_sql_activation_receipt.c. Their accepted source receipt is
`0b8eca38cac7a19dce99d152d5fd8637558c3b77f7963180b5232c207c26c35b`;
patch `e343d487c339ac2e212b2820fbd01bed597abedb44a73fa454c6d771df621eb2`.

The eight-file native qualification successor adds three source-reviewed fixture
updates. Receipt `973a42dbc0887fa57478b8e9400517739fcda93dbcd46cdc1d59f26c26a5efd2`;
patch `6a8694ea8b98d3eb0191075f16cbdb97416e884dfa3cc232e4cf920a53cd943c`.
Its five core inputs are byte-identical to the accepted parent. Exact original
Git preimages, full forward/inverse and changed-line clang18 fixed points pass.
Historical reference bytes/digests and the 872144-byte legacy maximum remain
explicit v1 cases. New cases cover player slots0/1/43/65535, native-mobile0/1/43
and refusal44, slot roundtrip, exact command/plan binding, both changed equipment
fingerprints, malformed magic/version/padding, truncation, unchanged outputs,
the 920144-byte new maximum and decoded-versus-SQL-version agreement. Player
65535 follows existing accounting grammar; it is not physical MAX_WEAR evidence.
No fixture, native compiler, server build, SQL/service or recovery execution ran.

These eight inputs remain PRIVATE and uninstalled. The current public source
still emits EAB1. The following format is a reviewed interface for coordinated
implementation, not proof that EAB2 is executable on the public branch.

## Exact EAB2 interface

All integers are little-endian. Header remains192, each holding remains112,
counts remain at184/188, and existing count bounds remain3071 holdings/6000 items.
EAB1 requires magic EAB1, version1, item88 and maximum872144. EAB2 requires magic
EAB2, version2, item96 and maximum920144. Magic and numeric version must agree.
Counts and exact total length are checked before row allocation/decoding.

Each v2 item is UID8 + the complete existing EAP1 position56 + source_digest32:

| Item-relative offset | Width | Field |
| --- | --- | --- |
| 0 | 8 | original UID |
| 8 | 1 | owner type |
| 9 | 1 | custody state |
| 10 | 6 | zero reserved bytes |
| 16 | 8 | owner lifetime ID |
| 24 | 8 | owner context |
| 32 | 8 | root UID |
| 40 | 8 | parent UID |
| 48 | 8 | native revision |
| 56 | 2 | equipment_slot |
| 58 | 6 | zero reserved bytes |
| 64 | 32 | exact source digest |

V2 retains existing owner/state/forest/equipment rules from the native accounting
contract. Both reserved regions are validated. Newly prepared input defaults to
v2; explicit v1 preparation refuses a nonzero slot instead of silently losing
it. Historical v1 decoding retains the old zero-slot interpretation and exact
old canonical order, domain byte sequence, digest tags and command hashes. It
never adopts equipment from current inventory or relabels retained history.

V2 domain input is uint16(2), then canonical112-byte holding rows, then complete
96-byte item rows. It uses the existing NUL-terminated
DURIS-ECONOMIC-BASELINE-DOMAIN-V2 digest tag. V2 intent keeps the original metadata
sequence with uint16 version2 and appends the computed v2 domain digest; tag is
DURIS-ECONOMIC-BASELINE-INTENT-V2. Equipment-only changes bind both prepared
fingerprints. EBC1 stays the existing48-byte reference grammar and hashes the
entire exact original versioned witness; its payload_version remains1.
EAI1/EAP1/root/source identities and operation derivation are unchanged.

SQL witness_version must match the decoded canonical witness version. Activation
readback accepts only1/2 and checks that agreement. Its existing wallet/bank-only
scope and empty-item restriction stay; this does not enable equipped activation.

## Narrow ownership handoffs and remaining integration

Primary owns the additive guarded schema successor to immutable0032, compiled
runtime metadata/history closure, native codec/transactions/activation and
central recipe/matrix registration. Immutable0032 and its historical metadata
must remain unchanged. The successor must accept exact v1/v2 CHECK shapes and
refuse divergent or unenforced constraints; no data backfill/reseal. Current
public0056 and the private unmeasured0057–0060 chain are not compatible-v2 proof.
Both-engine metadata/fingerprints, fresh/populated upgrades and backup size
closure remain original qualification work; no placeholder hashes may pass.

Plan5 owns the independent dual-version Python and standalone restore readers,
audit/origin digest reconstruction and backup/restore qualification. In
particular scripts/qualify_flatfile_economic_baseline.h still assumes88-byte
items/EAB1, snapshots append zero equipment, reservation stride88; its complete
forest proof, max read cap and snapshot reconstruction need explicit v2 handling.
The lifecycle empty-book reader must accept exact1/2 without changing its
wallet/bank-only scope. The independent Python EAB1 refusal and every historical
expected byte/digest must remain. Do not infer missing slots or reseal old rows.
The existing standalone mobile grammar probe must explicitly retain v1 for its
280-byte historical fixture, then qualify v2 separately on the combined candidate.

Broader Plan5 saved-plan import remains held for the previously published exact
numeric type/range issue in audit_original_plans. The new peer canonical
source-claim slice at fd0a6f2e4 is not that repair and is not imported here.
This version handoff enables independent work while primary completes schema
and native integration. Cold SHOP cleanup/rearm continues independently.
No release, activation or coverage gate is promoted; full R1–R8 stay unfinished.
