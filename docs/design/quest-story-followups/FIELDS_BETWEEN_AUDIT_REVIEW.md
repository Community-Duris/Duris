# The Fields Between: source-boundary and typed-reference supplement

Status: repository-wide static supplement to the existing
[Fields Between dossier](../zone-stories/FIELDS_BETWEEN.md). It reconciles
active boundaries, current unregistered WLD inputs, exact external prototype
references, and the active-title/history boundary. No area data, story sidecar,
or gameplay code was changed or qualified.

## Active and source-only WLD boundaries

The active area entry is `fields_between *710-711`; the canonical zone header
is `#710`. The existing dossier records all 153 rooms, 428 authored exits,
and six active cross-zone exit records. The active Surface edge is reciprocal;
the remaining four declarations are one-way:

| Active boundary | Evidence |
| --- | --- |
| Fields Between 71001 north ↔ Surface 629085 south | [Fields exit](../../../areas/wld/fields_between.wld#L12), [Surface exit](../../../areas/wld/surface.wld#L2710281) |
| Juiblex 87671 down → Fields Between 71106 | [Juiblex exit](../../../areas/wld/juiblex.wld#L3610) |
| Scorched Valley 71331 down → Fields Between 71140 | [Scorched Valley exit](../../../areas/wld/scorchvalley.wld#L2882) |
| Winterhaven 55633 northeast → Fields Between 71128 | [Winterhaven exit](../../../areas/wld/wh.wld#L13076) |
| Fields Between 71152 down → Scorched Valley 71265 | [Fields exit](../../../areas/wld/fields_between.wld#L3413) |

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored D0–D9 exits with zero parse issues. One source-only exit in
`surf.wld` targets an active Fields Between room:

| Unregistered source room | Direction and active target | Evidence |
| --- | --- | --- |
| 267652 | South (`D2`) to Fields Between 71001 | [exit target](../../../areas/wld/surf.wld#L1419619) |

This record is in unregistered WLD source, not the active area input. It is
not an active-world entrance and does not change the 428-exit active census or
establish ordinary access, safe arrival, or a return path. It increases the
curated source-only record count from 382 to 383.

## Direct external typed references

The existing dossier covers the foreign troll-brewer and Scorched Valley
follow-on recipes. The bounded exact-prototype scan confirms those links and
adds the code-defined equipment references:

| Current source | Fields Between prototype and source role | Evidence |
| --- | --- | --- |
| Active Juiblex quest and story sidecar | Consumes the local troll brewer's fez 71024; its optional producer mapping uses brewer mobile 71037 to turn manuscript 71005 and overripe banana clump 71016 into that fez | [QST input](../../../areas/qst/juiblex.qst#L189), [consumer mapping](../../../areas/story/juiblex.story.json#L678), [producer mapping](../../../areas/story/juiblex.story.json#L689), [brewer](../../../areas/mob/fields_between.mob#L596), [manuscript](../../../areas/obj/fields_between.obj#L54), [banana clump](../../../areas/obj/fields_between.obj#L205), [fez](../../../areas/obj/fields_between.obj#L325) |
| Active Scorched Valley quest and story sidecar | Consumes Grog's local head 71009 as one input in the three-head advisor exchange | [QST input](../../../areas/qst/scorchvalley.qst#L127), [story mapping](../../../areas/story/scorchvalley.story.json#L371), [head](../../../areas/obj/fields_between.obj#L117) |
| Compiled chaos-equipment tables | Reference local writhing shield 71025 in the standard and enhanceable bard arrays, and Sorrow veil 71028 in the enhanceable monk array | [standard bard](../../../src/account/chaos_eq_data.h#L202), [enhanceable bard](../../../src/account/chaos_eq_data.h#L447), [enhanceable monk](../../../src/account/chaos_eq_data.h#L376), [shield](../../../areas/obj/fields_between.obj#L341), [veil](../../../areas/obj/fields_between.obj#L391) |

The exact active-source scan found no direct foreign reset or shop references
to local Fields Between prototypes and no other direct QST, story-sidecar, or
C/H references beyond the rows above. The code-table entries and quest source
do not prove item generation, availability, custody, equipment, access, or
accepted completion. Numeric values in unrelated MOB/OBJ records were excluded
where they are not typed prototype references.

## Active title and tracked history boundary

A fresh normalized scan of all 442 current `.zon` title lines finds one exact
“The Fields Between” owner: `fields_between`. The other current titles sharing
the generic word “Fields” belong to `battlefi` (“The Battlefield”), `jademini`
(“The Rice Fields”), `killing_fields` (“Killing Fields”), and `wander` (“The
Wandering Fields”). These are separate current contributors with distinct
titles and indexed ranges, not alternate title owners for `fields_between`.
Both current indexes list `fields_between *710-711 (kolut)`.

Followed WLD and ZON history remains on the `fields_between` stem from the 2009
area-tree move through later edits. The 2026 rift pickup corrections are
object and quest maintenance; they do not rename or replace the zone. The story
sidecar was added on 2026-10-04. No alternate-stem predecessor, retirement,
or whole-zone replacement was found in the reviewed history. This title
comparison resolves the previous broader semantic-review note at the title
level; the dossier's explicit area-owner questions remain open.

[Fields Between title](../../../areas/zon/fields_between.zon#L2) ·
[active-area entry](../../../areas/AREA#L265) ·
[QST entry](../../../areas/qst/AREA#L237) ·
[Battlefield](../../../areas/zon/battlefi.zon#L2) ·
[Rice Fields](../../../areas/zon/jademini.zon#L2) ·
[Killing Fields](../../../areas/zon/killing_fields.zon#L2) ·
[Wandering Fields](../../../areas/zon/wander.zon#L2) ·
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)

## Audit boundary

These are static source findings. The `surf.wld` route is in unregistered
source, not active ingress. Active boundary records and foreign typed
references do not qualify player travel, item sourcing, exchange settlement,
or campaign completion. Existing rift mobility restrictions remain as recorded
in the comprehensive dossier; this supplement proposes no mechanical change.
Gameplay, editor/tooling, persisted-player, and indirect or dynamic consumers
remain outside this bounded closure.

## Checkpoint 797: current typed census refresh

The active reset file has 264 typed prototype operands: 230 M, 3 F, 6 O,
14 G, and 11 E. Of these, 262 resolve to local prototypes; the two imports
are objects #359 from Heavens and #55435 from White Harbor, both reset into
room #71128. No unresolved operands or foreign mobile imports were found.
[Fields Between resets](../../../areas/zon/fields_between.zon#L97),
[Heavens object reset](../../../areas/zon/fields_between.zon#L100),
[White Harbor object reset](../../../areas/zon/fields_between.zon#L101),
[Heavens object](../../../areas/obj/heavens.obj#L3725), and
[White Harbor object](../../../areas/obj/wh.obj#L5159).

The eight local QST givers contain 21 item fields: 14 G inputs and 7 R
outputs, all resolving locally. All typed references in the local story
sidecar also resolve locally. Fields Between has no local shop source.
[QST item fields](../../../areas/qst/fields_between.qst#L16),
[last QST item field](../../../areas/qst/fields_between.qst#L231),
[story-sidecar contacts](../../../areas/story/fields_between.story.json#L23),
[story-sidecar mappings](../../../areas/story/fields_between.story.json#L632).

The active outside quest and sidecar consumers are Juiblex and Scorched
Valley. Juiblex consumes fez #71024 and maps optional brewer #71037 to the
producer path manuscript #71005 plus banana clump #71016 yielding the fez;
Scorched Valley consumes Grog's head #71009, also repeated in its sidecar.
No active foreign reset, shop product, QST giver header, or other typed
sidecar reference targets Fields Between; no same-type mobile/object duplicate
or source-only typed consumer was found. See the consumer table above for
source anchors.

Local type-25 objects #71001, #71002, #71030, and #71031 target local rooms
#71104, #71014, #71153, and #71001, respectively. There is no local type-29
object. Across the current 872 type-25 and 380 type-29 declarations, no
external destination targets a Fields Between room. Chaos-equipment tables
also reference local shield #71025 and Sorrow veil #71028. These typed item
and code references add no WLD edges and do not change the active 428-exit
census or the existing source-only Surf approach.
[First type-25 item](../../../areas/obj/fields_between.obj#L1),
[last type-25 item](../../../areas/obj/fields_between.obj#L434),
[CHAOS equipment references](../../../src/account/chaos_eq_data.h#L202).

This refresh is static source evidence only. Runtime availability, editor
state, persisted-player identity, indirect/dynamic closure, and gameplay
qualification remain outside scope. No area source, WLD total, or migration
disposition changed.
