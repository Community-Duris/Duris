# Troll Caves: source-only boundaries and external-item supplement

Status: repository-wide audit supplement to the existing source-comprehensive
[Troll Caves dossier](../zone-stories/TROLL_CAVES.md). It extends the active
WLD census with unregistered source edges, reconciles current external item
references, and records the active-title/history boundary. It also corrects the
gem dealer's area attribution in the dossier. No native zone or gameplay change
is made.

## Active and source-only WLD boundaries

Troll Caves has 178 authored and resolving active WLD exits, one local weak
component of 82 rooms, no current fields, and two active reciprocal boundary
pairs: Surface room 622564 with Troll Caves room 96900, and Malch room 23741
with Troll Caves room 96976. The existing dossier and active
[cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv) retain those
four directed active records.

A scan of all 95 current unregistered WLD files parsed 2,080,773 authored
D0–D9 exit records and found three additional records targeting active Troll
Caves rooms:

| Unregistered source record | Direction and active target | Evidence |
| --- | --- | --- |
| `surf.wld` room 275641 | South (`D2`) to Troll Caves room 96903 | [exit target](../../../areas/wld/surf.wld#L1587268) |
| `surf.wld` room 276840 | Down (`D5`) to Troll Caves room 96900 | [exit target](../../../areas/wld/surf.wld#L1612439) |
| `underdark2011.wld` room 272717 | Down (`D5`) to Troll Caves room 96900 | [exit target](../../../areas/wld/underdark2011.wld#L298938) |

These records are authored in unregistered source and are not active-world
entrances. At Checkpoint 401 they added three individually reviewed rows to the
curated source-only list, bringing that checkpoint tally from 362 to 365. The
three routes are already included in the current cumulative total of 11,993
overall / 415 Surf. They do not alter the active 178-exit census or establish
ordinary access, safe travel, or a return route.

## External item and travel references

The Surface Keeps area has two distinct source roles for the Troll Caves gem
inputs:

- Shop `120003` lists emeralds 96910, rubies 96907, and obsidian 96906 as
  stock. The active Surface Keeps reset places the same three object
  prototypes on its gem dealer in room 123215, each with reset cap 999.
  [Shop stock](../../../areas/shp/surfacekeeps.shp#L3) ·
  [dealer and reset group](../../../areas/zon/surfacekeeps.zon#L548) ·
  [shopkeeper prototype](../../../areas/mob/surfacekeeps.mob#L48).
- The shop stock and reset declarations are separate evidence of a possible
  supply source. They do not prove that current stock is available, that a
  purchase is accepted, or that a player personally mined or acquired a gem.

The earlier dossier and the Troll Caves story sidecar label this dealer an
Alatorin source; room 123215 and shop 120003 are in active Surface Keeps. The
dossier now uses the correct area name, but the sidecar retains the old label
in its overview and three gem hints ([overview](../../../areas/story/troll_caves.story.json#L19),
[emerald hint](../../../areas/story/troll_caves.story.json#L115),
[ruby hint](../../../areas/story/troll_caves.story.json#L202),
[obsidian hint](../../../areas/story/troll_caves.story.json#L239)). The sidecar
is source data and was not changed in this documentation-only checkpoint. The
separate Alatorin loose-change reset discussed in the Dirk’nspire audit does
not supply Troll Caves materials.

The current fixed-teleport object scan also retains Graves sign 19510's PUNCH
destination at Troll Caves room 96937. The object declaration is present, but
the reviewed Graves reset sources do not place that sign. Its declaration is
not proof of a current item copy or an admitted arrival.
[Graves teleport destination](../../../areas/obj/graves.obj#L134) ·
[full Troll Caves route and default review](../zone-stories/TROLL_CAVES.md).

The existing dossier separately maps the code-defined Ixarkon veil target at
room 96909, the actual command mismatch, the imported bat/guano reset
references, and the full fixed-teleport inventory. No external QST source was
found with a direct numeric reference to the Troll Caves prototype ranges in
this source scan. Shop, reset, fixed-teleport, and custom-code declarations are
different route or supply mechanisms and remain subject to the dossier's live
qualification limits.

## Active title and history boundary

The normalized active title “Troll Caves” has one owner: area `troll_caves`,
zone 969. The reviewed tracked WLD/ZON path history and active manifest show no
named distinct predecessor or replacement. This conclusion is limited to that
title and bounded file-history evidence, not a universal semantic-equivalence
claim.

[Active title](../../../areas/zon/troll_caves.zon#L2) ·
[active-area entry](../../../areas/AREA#L347) ·
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)

CP401 followed the tracked WLD, MOB, OBJ, ZON, and QST paths; their reviewed
history stays on the same area stem through the existing moves and edits. The
story sidecar first appears in the October 4, 2026 documentation/source update.
No distinct predecessor or replacement is named in the reviewed path history.

Other current Troll-named titles have separate active owners and different
names: “The Troll Hills,” “Troll Slave Compound,” and “The Lair of the Swamp
Troll King.” Their titles and reviewed paths do not establish a predecessor or
replacement relationship with Troll Caves.

[Troll Hills title](../../../areas/zon/troll_hills.zon#L2) ·
[Troll Slave Compound title](../../../areas/zon/torture.zon#L2) ·
[Swamp Troll King title](../../../areas/zon/swamp_two.zon#L2)

## CP401 builder-question checkpoint

The [Troll Caves dossier](../zone-stories/TROLL_CAVES.md#explicit-builder-questions-and-recommended-resolutions)
now contains exactly twenty direct-source builder questions with conservative
recommendations. The review captures the wand/recipe mismatch, exact exchange
and blessing boundaries, switch and travel controls, falls, inactive WLD
approaches, and external reset dependencies. It proposes no gameplay or source
repair.

## Audit boundary

These are static source findings. Actual shop availability and purchase,
reset-time stock, fixed or custom arrival, falling hazards, story completion,
and reward settlement have not been qualified in live play. Editor/tooling,
persisted-player references, and indirect or dynamic consumers remain outside
this bounded closure.

## Checkpoint 856: Current boundaries and typed dependencies carried forward

The reviewed `areas/` and `src/` content remains identical to pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`; this checkpoint carries the Checkpoint 401 source findings and builder recommendations forward without claiming a second independent parse. Troll Caves remains an 82-room WLD with 178/178 resolving exits, one local weak component, and two active reciprocal boundaries: Surface #622564 / Troll Caves #96900 and Malch #23741 / Troll Caves #96976. The three previously cataloged unregistered approaches (#275641 and #276840 from Surf, #272717 from underdark2011) remain source-only and are already counted in the current cumulative total of 11,993 overall / 415 Surf. The older CP401 global total of 9,480 / 298 is historical.

The recorded typed closure remains unchanged: active Surface Keeps shop/reset sources supply gem objects #96910/#96907/#96906; the Graves sign #19510 still declares a fixed destination at #96937 without a reviewed reset placement; and the code-defined Ixarkon veil targets #96909. The existing local QST and sidecar remain mapped, including the documented stale Alatorin label in the sidecar overview and three gem hints. Preserve the imported bat/guano reset roles and local type-25 objects separately from WLD edges. These configured sources do not prove current stock, purchase, item placement, arrival, safety, or accepted quest completion. The twenty builder questions from CP401 remain source review, not gameplay qualification.

Keep active `troll_caves` **DO NOT MOVE**. Preserve both reciprocal boundaries and all three inactive source records with their current map owners; retain the two Surf approaches on existing Surf candidate row 97 and the underdark2011 approach on existing candidate row 114. Add no Troll Caves candidate, activate no route, and grant no migration clearance. No source-data correction is made to the sidecar. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L314), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-856-troll-caves-boundaries-and-source-only-approaches-remain-unchanged), [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), and [underdark2011 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L115).
