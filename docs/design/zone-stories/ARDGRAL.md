# Ard'gral source story dossier

Ard'gral is an interplanar space where dreamers' thoughts gather in an
endless ethereal mist. Its authored room prose describes a soft glow without a
visible source, possible voices in the background, and the bards' name for
the place. The same description and title are repeated throughout all 64
rooms, so the map gives the plane a strong atmosphere but no separate story
for individual locations ([room #30401](../../../areas/wld/ardgral.wld#L1),
[room #30464](../../../areas/wld/ardgral.wld#L1954)).

The one local creature is the essence of nightmares. The zone reset places
it in room #30422 and equips it with the horrid mask of nightmares
([mobile](../../../areas/mob/ardgral.mob#L1),
[mask](../../../areas/obj/ardgral.obj#L1),
[reset](../../../areas/zon/ardgral.zon#L28)). Its description says that the
viewer cannot wake and hears only their own screaming. This is the area's
only named encounter; there is no local quest record, shop, or story sidecar
that gives the nightmare a recorded resolution.

## The plane's gateways

Six reset swirling gateways use a compiled callback that sends an entrant to
a randomly chosen eligible surface-map room. Twelve other reset rifts are
bound to fixed destinations in the Ethereal Plane, the four elemental planes,
the Killing Fields, Moonshae, Vella's Bordello, Bloodstone Keep, the Caves of
Mt. Skelenak, the Deep Ravine of Passage, and the Astral Plane. The
[audit supplement](../quest-story-followups/ARDGRAL_AUDIT_REVIEW.md#typed-travel-and-cross-area-dependencies)
maps every reset and target. The illusionist Ard'gral spell adds a one-way
portal to a random room in the plane, subject to its source checks.

An explicit incoming route stands in Alatorin's Mystic Ward: its reset
places a stable shadow rift in room #83342, and the rift's long description
says it opens to Ard'gral. Odeth the Hooded also tells visitors they may enter
and offers a separate potion recipe; the recipe is a thematic story service,
not the rift's travel mechanism ([Alatorin route](ALATORIN.md#L221),
[rift reset](../../../areas/zon/alatorin.zon#L1555),
[rift description](../../../areas/obj/alatorin.obj#L589),
[Odeth's dialogue](../../../areas/qst/alatorin.qst#L997)).

## Builder questions

| Question | Source evidence | Follow-up |
| --- | --- | --- |
| Should the plane's dream imagery lead to a player-facing story? | All 64 rooms repeat the same dream-mist description; the nightmare essence and mask are the only local encounter and item. | Decide whether Ard'gral is intended as a travel hub and atmosphere, or whether dreamers' stories should have local outcomes. |
| Are the fixed rift destinations and random surface gateways deliberate? | Twelve reset rifts name fixed far destinations, while six gateways use the compiled random-map callback. | Preserve each typed destination and confirm any desired return route before changing the network. |
| Is Alatorin the intended stable entrance? | Its active zone reset places the stable rift to Ard'gral at #83342. | Keep the Alatorin reset and target linked in any separate movement or ownership proposal. |
| Is dormant Yggdrasil data expected to reuse Ard'gral's identifiers? | Unregistered Yggdrasil rooms #30401–#30412 overlap Ard'gral, and its old portals use the same numeric targets. | Resolve the ownership and identifier collision before considering any Yggdrasil reactivation or attic move. |
| Should `ardgral` have any command-target alias beyond its normalized title “Ard'gral, plane of dreams”? | The normalized title is unique among 442 scanned `.zon` files and no lexical title candidate was found; dormant Yggdrasil's numeric overlaps are a separate source-ownership issue. | Keep `ardgral` and “Ard'gral, plane of dreams” as the source key and display identity. Add an alias only after the owner names its exact target and intended distinction. |

Keep Ard'gral in the active area list. Its local exits form a complete
wraparound plane, and its cross-area access depends on fixed rift callbacks,
random surface travel, compiled illusionist travel, and Alatorin's active
incoming rift. The [audit supplement](../quest-story-followups/ARDGRAL_AUDIT_REVIEW.md)
records the source checks, old-data collisions, and migration disposition.
