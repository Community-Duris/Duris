# Interspace: a wrapped grid between the planes

The [Interspace audit supplement](../quest-story-followups/INTERSPACE_AUDIT_REVIEW.md)
reconciles its authored graph, one Astral boundary, reset imports, the external
portal destination, and tracked history. This source-comprehensive review covers
the current 100-room `interspa` contributor. It has no local QST, SHP, or story
sidecar, so its narrative is carried by repeated room prose, one mobile
prototype, and an unusual reset set.

## Environmental story

Every room is titled “Interspace” and describes “the intersecting singularity
of all planes yet part of none.” The one exception is punctuation: room #99900
ends the sentence with an ellipsis, while the other 99 rooms use a period. The
text gives Interspace a clear cosmological identity but no local faction,
quest, named plane, or destination-specific scene.

All rooms use the ethereal sector. The repeated room flags also include
underwater, jail, healing, guild, no-psionic, and atrium bits. Room #99999 has
the same flag value except it omits the underwater bit. These are authored
source values; their practical effects and intent are not established by the
room prose. See the [flag and loader review](../quest-story-followups/INTERSPACE_AUDIT_REVIEW.md#room-flags-and-loader-boundary).

## Wrapped routes and the Astral edge

The 100 rooms form four repeating 5-by-5 layers. Every room has one exit in
each of the six cardinal or vertical directions, with reciprocal local links.
The authored graph is strongly connected: from #99900, all 100 rooms can be
reached and all 100 have an authored local return. This describes the WLD
records and does not establish live access, door state, or safety.

Room #99999 also has a southeast-direction (`D9`) exit to Astral Main room
#19801. Astral Main has no authored reverse WLD exit to Interspace. The raw
Interspace exit state is `4`; the loader masks that field to its low two bits,
so this value does not set a door at WLD load. The separate reset file contains
no door-state command for this boundary. See the [static graph review](../quest-story-followups/INTERSPACE_AUDIT_REVIEW.md#static-connectivity-and-boundary).

## Reset prototypes and absent local quest content

Interspace defines one mobile prototype, a “vaporous, swirling mist” described
as enveloping a visitor in ruthless terror. The two local mobile reset records
instead reference mobile VNUM #2 at rooms #99900 and #99937. That prototype is
defined in Limbo as a black Durian dragon. Neither reset record names the local
mist prototype #99900, and no foreign reset-role reference to that local mobile
was found in the bounded scan. The local mist's keyword is the generic
`something`, and its short-description field is `(null)`.

The local object file is empty. There is no local QST file, shop file, or story
sidecar. The checked-in text and prototypes therefore define no local quest
receipt, service, portal item, or resolved story outcome.

One external source points into Interspace through an item destination value.
Ravenloft 2 object #59310 is a type-25 magical glove reward in Jander's Strahd
finale and stores room #99999 as its destination. Its other value is also
99999, while the travel handler requires that value to match the activating
command. The source establishes the typed destination and reward ownership,
but not a usable entrance or a return route. Details are in the [typed
dependency review](../quest-story-followups/INTERSPACE_AUDIT_REVIEW.md#typed-dependencies-and-tracked-history).

## Related plane contributors

The repository also contains the separate active `ethereal_main` contributor,
“The Ethereal Plane,” with its own 150-room range and authored routes to the
Dream and Dreggan Woods. Astral Main is a second separate plane contributor;
Interspace's only authored WLD boundary leads there. The similar plane
language establishes a thematic relationship but does not identify any of
these contributors as a predecessor or replacement. See the [Ethereal Plane dossier](THE_ETHEREAL_PLANE.md) and the [Astral Main dossier](ASTRAL_MAIN.md).

## Builder questions and recommended resolutions

| Question for the builder | Recommended resolution from current source |
| --- | --- |
| Is Interspace separate from the active Ethereal Plane and Astral Main, or is it meant to be one of those contributors under another name? | Preserve the distinct registered contributors and their source identities until an owner confirms the cosmology. The unique Interspace title and its single outbound Astral edge do not establish a rename or replacement of Ethereal Main. See the [Ethereal Main roster entry](../../../areas/AREA#L70), [Ethereal Plane dossier](THE_ETHEREAL_PLANE.md), and [Astral Main room](../../../areas/wld/astral_main.wld#L4004). |
| Is Interspace intended as a neutral crossing, a dangerous liminal environment, or only a destination marker? | Keep the existing “part of none” identity and terror prose separate from any mechanics. Add a route or encounter only with a named owner, arrival and return rules, and a defined outcome. See the [room descriptions](../../../areas/wld/interspa.wld#L1) and [mist prototype](../../../areas/mob/interspa.mob#L1). |
| Should all 100 rooms continue to repeat the same title and description? | Preserve the shared identity unless the builder wants authored subregions. If distinctions are added, tie each to a specific layer or route and keep the room VNUMs stable. See the [WLD source](../../../areas/wld/interspa.wld). |
| Is the four-layer wrapped 5-by-5 topology intentional? | Keep the reciprocal six-direction grid while its intent is unresolved. If the wrap is meant to be open or bounded, specify the exact layer and edge changes together with a verified return route. See the [local graph audit](../quest-story-followups/INTERSPACE_AUDIT_REVIEW.md#static-connectivity-and-boundary). |
| Should room #99999's southeast exit lead to Astral Main #19801, and is that one-way boundary intentional? | Preserve the authored edge and its one-way classification until the owner confirms the intended crossing and return. Do not infer player access or safety from the WLD edge. See the [exit record](../../../areas/wld/interspa.wld#L3000) and [cross-area register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1189). |
| Does the raw exit-state value `4` have intended meaning on the Astral edge? | Confirm it against the current loader semantics. The loader masks it to zero and the zone has no door reset for this exit; retain the question instead of treating it as a locked door. See [`setup_dir`](../../../src/world/db.c#L1545) and the [local reset file](../../../areas/zon/interspa.zon#L1). |
| Should the 99 underwater flag sets and the unflagged #99999 be retained? | Ask the builder to confirm whether the repeated underwater bit is deliberate, especially at the only external WLD edge. Do not infer submersion or its gameplay effects from the flag without runtime qualification. See the [room-flag comparison](../quest-story-followups/INTERSPACE_AUDIT_REVIEW.md#room-flags-and-loader-boundary) and [flag definitions](../../../src/core/defines.h#L396). |
| Are the jail, healing, guild, no-psionic, and atrium bits intentional across this plane? | Preserve the source values while the room owner checks them. Resolve each bit by its authored purpose before changing the repeated room records; source flags alone do not describe a player-facing restriction or service. See [`defines.h`](../../../src/core/defines.h#L396). |
| Should Interspace reset the local mist at #99900 and #99937, or is the imported Limbo mobile #2 intentional? | Confirm the intended mobile identity and placement before changing either reset. Keep the mist prototype and Limbo's black Durian dragon as separate owners; do not infer that the mist is present from its definition. See the [Interspace resets](../../../areas/zon/interspa.zon#L10), [local mist](../../../areas/mob/interspa.mob#L1), and [Limbo mobile #2](../../../areas/mob/limbo.mob#L12). |
| Is the unreferenced local mist prototype intended to remain available for a future encounter? | Preserve it as a defined but currently unreferenced prototype until its owner decides whether it needs a reset or should be retired. If it is intended to appear, replace the generic keyword and `(null)` short description with its intended identity. Any encounter needs an explicit role and outcome. See the [mobile record](../../../areas/mob/interspa.mob#L1). |
| Is Ravenloft 2's Jander reward intended to provide travel to room #99999? | Confirm the required activation command and whether the one-way destination is intentional. Keep the reward, `value[0]` destination, and `value[1]` activation check as separate facts until the builder verifies a usable route and return. See [Ravenloft 2 object #59310](../../../areas/obj/ravenloft2.obj#L5152), [quest reward](../../../areas/qst/ravenloft2.qst#L2431), and [`check_item_teleport`](../../../src/magic/spell_travel.c#L949). |
| Should Interspace remain without a local quest, shop, objects, or story sidecar? | Keep the current source-only environmental role unless an owner defines a local giver, accepted action, service, and outcome. Do not award progress for entering the grid or receiving a foreign item. See the [empty object file](../../../areas/obj/interspa.obj) and [quest roster](../../../areas/qst/AREA#L322). |

These questions record source-backed decisions for builders. They do not change
the local reset, add access, or qualify a played route.
