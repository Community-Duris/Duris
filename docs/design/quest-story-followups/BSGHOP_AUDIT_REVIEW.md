# BSGHOP Audit Review

Reviewed October 11, 2026, Checkpoint 457. This is a static source, followed-history, WLD graph,
and bounded typed-reference review of the unregistered BSGHOP component. It does not establish live
loading, player access, portal operation, source ownership, or permission to move files.

## Source scope and identity

The current component contains [WLD](../../../areas/wld/bsghop.wld), [MOB](../../../areas/mob/bsghop.mob),
[OBJ](../../../areas/obj/bsghop.obj), and [ZON](../../../areas/zon/bsghop.zon) files. The ZON header
is #1, titled “The UC Outpost For Blood Seekers,” with last VNUM #29443 and reset mode 2. The source
contains four rooms #29440–#29443, four mobiles #29440–#29443, and one object #29440. It has no
same-stem QST or SHP file and no story sidecar.

No BSGHOP row appears in the current [full area manifest](../../../areas/AREA) or [QST
manifest](../../../areas/qst/AREA). The active area manifest lists Harrow at *294–296
([line 142](../../../areas/AREA#L142)); the QST manifest lists Harrow at *294–296
([line 132](../../../areas/qst/AREA#L132)). Each BSGHOP room record carries zone field 1 although
the room VNUMs are in zone 294. The source therefore has both an unlisted status and a zone-number
discrepancy; neither fact settles the owner or intended loader path.

## Environmental story

The four rooms form a compact horror scene. Room #29440 describes bloody walls, children painting
with blood, and laughter over cries of bloodlust; its extra description reads “REDRUM!”
([room source](../../../areas/wld/bsghop.wld#L1)). Room #29441 presents death on one side and life
on the other, then offers east and west exits to the same device room
([room source](../../../areas/wld/bsghop.wld#L22)). Room #29442 places the Device of Dark Compact
beside a bulletin board; the board's content includes legacy character references, jokes, game
coordination notes, gear notes, and a personal-contact claim. This review summarizes the board
without reproducing player names or contact details
([room and board](../../../areas/wld/bsghop.wld#L42)). Room #29443 shows a poisonous, starless
expanse associated with a necromancer and returns down to #29442
([room source](../../../areas/wld/bsghop.wld#L82)).

The mobiles describe a spectre of a young girl, a dancing boy skeleton, a frail child wraith whose
text says Breale slaughtered the child, and an Angel of Death said to have been summoned by the
children's laughter ([mobile source](../../../areas/mob/bsghop.mob#L1),
[child wraith](../../../areas/mob/bsghop.mob#L28), and
[Angel of Death](../../../areas/mob/bsghop.mob#L42)). The source frames the outpost residents as
continuing the Angel's work. The portal device's description says it creates a portal to the Living
and includes a deliberately comic particle-collider explanation
([object source](../../../areas/obj/bsghop.obj#L1)).

This is environmental story text, not a native quest contract: there is no BSGHOP QST or SHP file.
The source does not explain who the UC or the outpost residents are, who Breale is, how a visitor is
meant to respond to the horror scene, or whether the portal is functional. The violence toward
children and the bulletin-board material need an owner decision before any rewrite, redaction, or
preservation decision.

## Conflicting numeric ownership and copied room text

All four BSGHOP room and mobile VNUMs overlap current active Harrow VNUMs, and BSGHOP object #29440
also overlaps Harrow object #29440. The records have different identities:

| VNUM | BSGHOP source identity | Current Harrow identity |
| --- | --- | --- |
| #29440 | Bloody room, spectre of a young girl, portal device | Artist's workshop, artist, near-finished painting |
| #29441 | Doorstep room, boy skeleton | Household sleeping area, stag |
| #29442 | Device room, child wraith | Sewing home, blind fish |
| #29443 | Poisonous expanse, Angel of Death | Family sleeping area, warty toad, yo-yo |

Harrow's active source confirms these distinct room, mobile, and object records
([WLD #29440–#29443](../../../areas/wld/harrow.wld#L1010),
[MOB #29440–#29443](../../../areas/mob/harrow.mob#L513),
[OBJ #29440–#29443](../../../areas/obj/harrow.obj#L521)). If BSGHOP were ever reintroduced, equal
VNUMs would not distinguish these alternate identities. The full area list currently gives those
numbers to Harrow; this audit makes no loader or runtime claim about the unlisted BSGHOP files.

The unregistered darkop source contains nearly the same environmental map text. Its WLD descriptions
and exits for #29441–#29443 match BSGHOP; #29440 shares the description and north exit, while BSGHOP
also has an up exit with target −1 ([darkop rooms](../../../areas/wld/darkop.wld#L202)).
darkop.obj #29440 uses the same portal name and prose but has different numeric fields
([darkop portal](../../../areas/obj/darkop.obj#L65)); darkop's ZON instead resets mobiles #29434–#29437
in room #29440 ([darkop resets](../../../areas/zon/darkop.zon#L32)). These are source-content and
numeric overlaps, not evidence that either component replaced the other. Neither BSGHOP nor darkop
is in the current full area list.

## Followed history and replacement review

Git path history shows the WLD under the same stem from the 2009 area-tree move commit
642da5400. The MOB, OBJ, and ZON histories also include same-stem changes in commits bb0d59425
and 71d731c5b. This tracked history contains no identified alternate-stem rename or explicit
replacement decision. The repeated darkop room text and portal prose establish a content overlap
only; they do not establish which version is canonical or whether one was intended to supersede the
other. See the [darkop audit references](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-457-bsghop-room-reuse-and-identity).

## Static WLD graph and zone-number discrepancy

The four-room graph has nine authored directions: eight local exits and one negative target. Every
room is in one weak component; directed traversal from #29440 reaches all four rooms.

| BSGHOP source exit | Target | Static classification |
| --- | --- | --- |
| #29440 D0 north | #29441 | Local |
| #29440 D4 up | −1 | Negative target; not a destination |
| #29441 D1 east | #29442 | Local |
| #29441 D2 south | #29440 | Local |
| #29441 D3 west | #29442 | Local; duplicates the east destination |
| #29442 D1 east | #29441 | Local |
| #29442 D3 west | #29441 | Local; duplicates the west/east destination |
| #29442 D4 up | #29443 | Local |
| #29443 D5 down | #29442 | Local |

The authored records stay within the four numeric VNUMs, but the WLD zone field is 1 on each room
while #29440–#29443 divide by 100 to zone 294. The ZON also declares zone #1 and ends at #29443
([header](../../../areas/zon/bsghop.zon#L1)). The loader's
[recalc_zone_numbers function](../../../src/world/db.c#L1691) derives a loaded zone-table number
from its lowest loaded room VNUM. That code fact does not show that BSGHOP is loaded or resolve the
source's ownership.

No BSGHOP-specific active inbound WLD exit is established. Active Harrow exits can contain the same
numeric targets, but under the current manifest those targets resolve to Harrow's different active
rooms. Harrow's broader source-only numeric-match table records eight BSGHOP target matches across
four overlapping source rooms; those matches do not make the inactive BSGHOP map an active
destination ([Harrow review](HARROW_AUDIT_REVIEW.md#unregistered-wld-target-matches)).

## Reset, quest, and code references

BSGHOP's ZON has one O reset for object #29440 in room #29442 and four M resets for mobiles
#29440–#29443 in room #29440 ([BSGHOP reset commands](../../../areas/zon/bsghop.zon#L9)).
Those object/mobile identifiers collide with active Harrow prototypes, and the reset room collides
with active Harrow room #29440. Keep these source-only reset instructions separate from Harrow's
active reset file; do not infer that the BSGHOP instructions run.

The active Tundra story sidecar contains an item-give reference to object #29440
([contract](../../../areas/story/tundra.story.json#L407)). In the current active catalog this is
Harrow's near-finished painting, which the Harrow story documents as part of Bom's exchange
([Harrow story](../zone-stories/HARROW_THE_GNOME_VILLAGE.md)). It does not identify BSGHOP's
portal device. Harrow's own QST, ZON, and SHP data also use overlapping #29440–#29443 values as
Harrow items, rooms, and mobiles.

The bounded C/C++ search found #29443 in equipment-item slots in
[chaos_eq_data.h](../../../src/account/chaos_eq_data.h#L204), also at
[line 426](../../../src/account/chaos_eq_data.h#L426) and
[line 450](../../../src/account/chaos_eq_data.h#L450). Under current active prototype ownership,
object #29443 is Harrow's yo-yo, while BSGHOP mobile #29443 is the Angel of Death. Preserve this
type distinction in any code-owner review. No other exact #29440–#29443 C/C++ references were
found by the bounded source scan.

## Safe disposition

Keep BSGHOP **RETAIN IN PLACE**. Before any separate path proposal, resolve the owner of VNUMs
#29440–#29443 across active Harrow, BSGHOP, and darkop; identify the canonical room and portal
versions; settle the zone-number mismatch and reset intent; review the violent story and board
material with the content owner; and identify editor/tooling and persisted-VNUM owners. The active
Tundra item contract and the C/C++ item slots currently resolve to Harrow's active object records,
not to BSGHOP. The full set of owner questions is in the
[BSGHOP builder questions](BSGHOP_BUILDER_QUESTIONS.md). No source, manifest, gameplay, or
configuration change is authorized by this audit.
