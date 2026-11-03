# The Hall of Knighthood repository-wide audit supplement

This supplement reconciles active and source-only connectivity, typed dependencies, and bounded title/history for the [Hall of Knighthood dossier](../zone-stories/HALL_OF_KNIGHTHOOD.md). Native `areas/` and gameplay `src/` remain unchanged. The active contributor is `knight`, zone 709.

## Active graph and source-only approach

The 41-room [WLD](../../../areas/wld/knight.wld) has 80/80 resolving exit records: 79 local and one outbound. Its local weak components are 40 and 1; #70999 is the isolated bookend. From #70900, raw directed reach covers 36/41 rooms; 39/41 have authored paths back to #70900. #70936, #70937, #70938, #70939, and #70999 are outside forward reach. #70937 and #70999 have no exits; #70936 has only its south exit to #70914. These figures treat every authored WLD edge as available and do not apply the 38 local door resets.

The active boundary pair is #70900 south to Surface #639011 and the return #639011 north to #70900 ([Knight record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L848), [Surface record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1671)). A source scan of the 95 unregistered WLD files found one exact target into the active Knight room set and no source-room VNUM overlap: `surf.wld` #259810 north to #70900 ([source room](../../../areas/wld/surf.wld#L1255034), [exit target](../../../areas/wld/surf.wld#L1255041)). This is a source-only record, not active ingress. Preserve it with Surf's existing RETAIN candidate row 97; do not add a Knight candidate.

At #70924 west, the WLD associates key VNUM #70917 with a pickable door. The reset sets state 1; `db.c` applies that state as closed and clears the locked bit ([WLD](../../../areas/wld/knight.wld#L689), [reset](../../../areas/zon/knight.zon#L152), [reset handling](../../../src/world/db.c#L7878)). The checked reset data therefore does not establish #70917 as a gate key, despite Arthel carrying it and the key's convergence-room text ([holder](../../../areas/zon/knight.zon#L332), [object](../../../areas/obj/knight.obj#L137)). No live reset or gameplay was performed.

Curated source-only totals now rise by one to 3,472 overall and 230 for the reviewed Surf subset. `ud_map2` remains 18 and Underworld2 remains eight. No dynamic portal, compiled room route, reset door, mobile movement, or safe return is inferred from those WLD records.

## Bounded typed dependency closure

| Reference | Source-backed classification |
| --- | --- |
| Local reset imports | `knight.zon` imports Heavens objects #72 and #360 at #70925 and White Haven objects #55269, #55325, and #55379 on local mobiles ([Saint Verdonnaly reset](../../../areas/zon/knight.zon#L303), [Arthel reset](../../../areas/zon/knight.zon#L333), [Kithron reset](../../../areas/zon/knight.zon#L372)). The active Heavens #72 is a magical fountain and #360 a rune-covered monolith ([Heavens prototypes](../../../areas/obj/heavens.obj#L1000), [#360](../../../areas/obj/heavens.obj#L3741)); White Haven defines the three other objects ([#55269](../../../areas/obj/wh.obj#L3346), [#55325](../../../areas/obj/wh.obj#L4012), [#55379](../../../areas/obj/wh.obj#L4576)). Unlisted `hostel.obj` and `mavulsk.obj` also declare #72 with different prototypes, so the intended object-index resolution for that imported VNUM remains a builder/loader question. |
| Outside quest inputs | Airp's active QST and structured sidecar refer to Four Winds boots #70976; its completion key says the item disappears. Bastine's active QST and sidecar refer to wand of Life #70970; its completion key says `disappear=0`. These retain Airp and Bastine as owners ([Airp QST/story](../../../areas/qst/airp.qst#L114), [Bastine QST/story](../../../areas/qst/bastine.qst#L156)). |
| Compiled mobile/equipment consumers | `randomeq.c` lists mobile #70941 as Kithron in `highdrop_mobs`; that numeric match refers to the local Kithron mobile, not local object #70941. Compiled Chaos kits use objects #70954, #70968, and #70979 ([equipment tables](../../../src/account/chaos_eq_data.h#L80)). Pathfinder object #70963 has a live compiled assignment and its `say path` handler in `specs.knight.c`; the handler requires a living user and worn object, applies an 800-second timer, and calls `spell_pass_without_trace` ([assignment](../../../src/specs/specs.assign.c#L1402), [handler](../../../src/specs/specs.knight.c#L18)). The #70924 Lobos-coat assignment is commented out ([source line](../../../src/specs/specs.assign.c#L1735)). |
| Other checked catalogs | No outside ZON reset-role records, SHP product rows, duplicate external mobile/object prototype definitions in #70900–#70999, or external type-25 destination into a Knight room was found. Outside typed story-sidecar item references are limited to #70970 and #70976. An exact numeric scan's lone external object-file match to #70999 is a cost/value field in `seakngdm.obj` #31519, not a prototype, room, or typed dependency. |

These checks are bounded to checked-in current source catalogs and direct compiled references. They do not inventory persisted player/account data, every indirect editor or builder tool, or live object-index and gameplay behavior.

## Title, same-stem history, and map identity

After color stripping and case/whitespace normalization, “The Hall of Knighthood” has one owner among current active `.zon` titles ([current title](../../../areas/zon/knight.zon#L2)). Followed WLD, MOB, OBJ, and ZON history stays on the `knight` stem, including the tracked 2009 move into `areas/`; later edits include the 2011/2013 WLD maintenance, 2015 mobile/reset quest-item flags, and 2016 object text cleanup. No alternate-stem predecessor, title rename, or whole-area replacement was found in those histories.

Commit `71d731c5b` (2013-11-13) changes the entrance boundary. Before it, #70900 had exits to old-map rooms #237364 and #237063; afterward the south exit targets active Surface #639011 and the other exit is removed. Both old VNUMs remain in unregistered `surf.wld` and `surface2011.wld`; #237063 is a hillside in `surf.wld` and an ocean room in `surface2011.wld`. That is bounded endpoint history across distinct source-map bodies, not evidence that either map replaced the Knight area or identifies the provenance of its former boundary. The newer `surf.wld` approach from #259810 is a separate source-only record.

The White Haven reset names “an Ambassador from The Hall of Knighthood,” and Bastine has knightly text; those thematic or explicit NPC descriptions do not make either contributor a duplicate owner or replacement. Keep `knight` **DO NOT MOVE**, keep Surf and `surface2011` under their existing source identities, and retain the one source-only Surf approach with Surf candidate row 97.

## Builder review boundary

The dossier presents 25 direct questions and recommended resolutions covering entrance commitment, spell and alignment rules, prose hazards, missing or contradictory portal/door text, key/reset progression, fountain and mushroom prose, tunnel asymmetry, Volknur target text, Lobos/trap-room access, the disconnected bookend, imported objects, external quest inputs, Soulrender and Pathfinder behavior, and live qualification. The questions do not authorize native route changes or create a local campaign. Any future quest, map repair, or owner change needs its own selected scope and source evidence.

## Checkpoint 513: Moorney quest story link

Unregistered areas/qst/myranth1.qst names Moorney and describes his escape
from a magical prison. Active Knight mobile Zyla #70919 says apprentice
Moorney was imprisoned for Zyla’s demon summoning
([Zyla source](../../../areas/mob/knight.mob#L278)). This is a direct
narrative correspondence, not a typed prototype or QST loader link: the
candidate giver #70132 is not defined in current mobile sources, the file is
absent from qst/AREA, and the runtime loader opens the aggregate world.qst
without globbing candidate QST files. Preserve the relationship as an
owner-confirmation question, not proof that the quest is active. See the
[myranth1 audit](MYRANTH1_AUDIT_REVIEW.md) and
[builder questions](MYRANTH1_BUILDER_QUESTIONS.md).