# Ceothia: repository-wide boundary and dependency supplement

Status: bounded static supplement to the
[comprehensive Ceothia source map](../zone-stories/CEOTHIA.md). It reconciles
active and source-only WLD edges, selected direct typed references from foreign
quest/reset/story/code sources, and the repeated title across the three active
Ceothia phases. It does not qualify runtime availability or player outcomes.

## Active and source-only WLD boundaries

The active area is `ceothia`, zone 808, with physical rooms 80800–81094. The
cross-area exit ledger records eight inbound and one outbound declaration:

| Active boundary | Evidence |
| --- | --- |
| Potions room 30709, five directions (north, east, west, up, down) → Ceothia vault 81094 | [north](../../../areas/wld/potions.wld#L278), [east](../../../areas/wld/potions.wld#L282), [west](../../../areas/wld/potions.wld#L290), [up](../../../areas/wld/potions.wld#L294), [down](../../../areas/wld/potions.wld#L298) |
| Potions room 30713 down → Ceothia vault 81094 | [exit](../../../areas/wld/potions.wld#L394) |
| Potions room 30714 down → Ceothia vault 81094 | [exit](../../../areas/wld/potions.wld#L419) |
| Surface forest room 628287 north ↔ Ceothia road room 80800 south | [Surface exit](../../../areas/wld/surface.wld#L2693467), [Ceothia exit](../../../areas/wld/ceothia.wld#L19) |

The seven Potions exits are staged/wander records into the vault, not ordinary
player entrances. They remain distinct from the reciprocal Surface pair. The
comprehensive dossier also maps Future Ceothia portal 81413, reset in room
81677, to present audience room 80980
([portal prototype](../../../areas/obj/ceofutur.obj#L233),
[destination](../../../areas/obj/ceofutur.obj#L239),
[reset](../../../areas/zon/ceofutur.zon#L41)). A portal endpoint is not a WLD
edge or evidence of a player's personal traversal.

A stateful parse of all 95 current unregistered WLD sources read 2,080,773
authored D0–D9 exits with zero parse issues. One source-only `surf.wld` exit
targets a current Ceothia room:

| Unregistered source room | Direction and active target | Evidence |
| --- | --- | --- |
| 267058 | North (`D0`) to Ceothia road room 80800 | [exit target](../../../areas/wld/surf.wld#L1407141) |

This record is in an unregistered source file, not the active WLD input. It
does not create active-world ingress. It brings the curated source-only inbound
count from 384 to 385 and the reviewed recent `surf.wld` subset from 17 to 18.

## Direct external typed references

The local potion 80831 is an object prototype in the Ceothia object file. Five
active foreign `O` reset rows load that object VNUM in four active zone sources:

| Foreign reset source | Target room | Evidence |
| --- | ---: | --- |
| Bahamut | 25925 and 25930 | [first reset](../../../areas/zon/bahamut.zon#L163), [second reset](../../../areas/zon/bahamut.zon#L167) |
| Celestia | 45725 | [reset](../../../areas/zon/celestia.zon#L166) |
| Negative Plane | 26867 | [reset](../../../areas/zon/negplane.zon#L201) |
| Tiamat | 19634 | [reset](../../../areas/zon/tiamat.zon#L93) |

The prototype is a level-53 potion
([object 80831](../../../areas/obj/ceothia.obj#L508)). Two code paths in
`random.zone.c` also read object 80831 directly
([random-zone potion](../../../src/world/random.zone.c#L459),
[potion selection](../../../src/world/random.zone.c#L953)). These are source
references, not proof that a particular reset or procedural branch executes.
The similar 80831 rows in `potions.zon` are commented with `*` and are not
active resets; the commented monolith row targeting room 81094 there is also
excluded ([commented rows](../../../areas/zon/potions.zon#L10),
[commented room target](../../../areas/zon/potions.zon#L22)).

Two active Alatorin QA exchanges accept Ceothian weapons for their own coin
rewards: Brino accepts dagger 80804 for 60,000 copper and longsword 80808 for
40,000 copper. Their native `G I` inputs and sidecar contracts agree
([dagger input](../../../areas/qst/alatorin.qst#L5322),
[dagger contract](../../../areas/story/alatorin.story.json#L2670),
[longsword input](../../../areas/qst/alatorin.qst#L5352),
[longsword contract](../../../areas/story/alatorin.story.json#L2748)). These
consumers retain Alatorin ownership; they do not add Ceothian outcomes.

Five Quietus QST branches also reference item 80808 as a supplied credential
and return the same VNUM in their completion contract. The Quietus story data
describes that copied “badge” role, while Ceothia's prototype is a blackened
steel longsword. Preserve the existing type and Quietus owner; do not infer
that the item is one of Ceothia's common guild badges
([first branch](../../../areas/qst/quietus.qst#L50),
[second](../../../areas/qst/quietus.qst#L104),
[third](../../../areas/qst/quietus.qst#L137),
[fourth](../../../areas/qst/quietus.qst#L180),
[fifth](../../../areas/qst/quietus.qst#L228),
[local prototype](../../../areas/obj/ceothia.obj#L130),
[Ceothian guild badge prototype](../../../areas/obj/ceothia.obj#L213),
[Quietus sidecar](../../../areas/story/quietus.story.json#L19)).

Past Ceothia's contact list directly names mobile 80803, Present Ceothia's
Lord Lenbrea. Its description says this is the Present exchange owner and that
the local Past portal arrives independently; the contact reference does not
merge the two phase contracts
([Past contact](../../../areas/story/ceopast.story.json#L132),
[Present mobile](../../../areas/mob/ceothia.mob#L54)).

Other bounded code references already mapped in the comprehensive dossier
include ship-location entries for room 81021
([entry](../../../src/ships/ship_variables.c#L341),
[second entry](../../../src/ships/ship_variables.c#L453)) and the progression
beacon table's room 81094 row
([table entry](../../../src/specs/specs.character_progression.c#L39)). The
dossier distinguishes those rows from room 81021's active `crew_shop_proc`
binding ([assignment](../../../src/specs/specs.assign.c#L2405)) and notes that
the beacon table has no live assignment here. The complete tracked
SHP source scan found no other shop file with typed references to Ceothia's
local room, mobile, or object VNUM range; the one local shop is #80861
([source](../../../areas/shp/ceothia.shp#L1)).

## Three active phases with one normalized title

`areas/AREA` and `areas/qst/AREA` list these as separate active contributors.
Their color-stripped zone titles all normalize to “Ceothia.”

| Phase | Source key and room range | Manifest and title evidence |
| --- | --- | --- |
| Present | `ceothia`, zone 808, rooms 80800–81094 | [`areas/AREA`](../../../areas/AREA#L288), [`areas/qst/AREA`](../../../areas/qst/AREA#L257), [title](../../../areas/zon/ceothia.zon#L2) |
| Past | `ceopast`, zone 811, rooms 81100–81392 | [`areas/AREA`](../../../areas/AREA#L289), [`areas/qst/AREA`](../../../areas/qst/AREA#L258), [title](../../../areas/zon/ceopast.zon#L2) |
| Future | `ceofutur`, zone 814, rooms 81400–81677 | [`areas/AREA`](../../../areas/AREA#L290), [`areas/qst/AREA`](../../../areas/qst/AREA#L259), [title](../../../areas/zon/ceofutur.zon#L2) |

The active manifests and distinct room ranges support preserving Present,
Past, and Future identity. The shared title and direct Past-to-Present Lenbrea
contact are not replacement evidence. The duplicate-name register retains the
same conclusion and links this supplement.

## Audit boundary

These are static source and manifest findings. They do not prove reset firing,
portal execution, safe or ordinary access, player custody, quest settlement, or
durable outcomes. The source-only `surf.wld` edge is not an active entrance.
Editor/tooling, persisted-player, and indirect or dynamic consumers remain
outside this bounded closure. No native zone, quest, story-sidecar, code, or
runtime change is included.
