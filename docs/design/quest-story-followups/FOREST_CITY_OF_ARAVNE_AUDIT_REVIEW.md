# The Forest City of Aravne: repository-wide connectivity and dependency audit

**Checkpoint 90; priority 130 of the original 220-zone queue.** This
supplement reconciles the [source-comprehensive dossier](../zone-stories/FOREST_CITY_OF_ARAVNE.md)
with active WLD topology, all inactive WLD sources, typed references, compiled
numeric matches, normalized title, and bounded source history. Static exits
and code references do not establish ordinary availability, safe travel, or
played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. The Forest City of Aravne has
438 rooms and 923 authored exits; all 923 resolve. The internal graph is one
weak component of 438 rooms. Its only outbound boundary is reciprocal with
Surface, while Winterhaven contributes a second inbound-only boundary:

| Aravne edge | Active return or inbound | Cross-area register row |
| --- | --- | --- |
| #21500 south to Surface #620880 | Surface #620880 north to #21500 | [Aravne outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L359), [Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1627) |
| Winterhaven #55632 southeast (D9) to Aravne #21937 | No reciprocal exit from #21937 to #55632 | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L788) |

There are no negative or positive unresolved exits. One room has a fall-chance
field and no room has a current field. The unrelated global active-WLD duplicate
room #51800 belongs to Hell Two and Hell Three. Door/reset state, entry from
Winterhaven staging, dynamic portal use, route safety, and successful travel
remain separate from this authored graph.

## Inactive WLD sources

The all-95 inactive WLD scan parsed 2,080,773 exits without issues and found no
source-room VNUM overlap with Aravne. It found one source-only approach:
`surf.wld` room #248234 north to #21500
([source room](../../../areas/wld/surf.wld#L1012070)). This is an inactive
declaration, not active ingress. It raises the curated source-only reference
total to 1,093 and the reviewed Surf subset to 122.

## Typed references and external producers

Typed scans found one active foreign QST item command and one active foreign
shop product. The active Airship Grave QST has the smith accept local sand
object #21673 and 25,000 coin for lens #77566
([item command](../../../areas/qst/airshipgrave.qst#L167)). This is the
Dawndale service mapped in the source dossier; it is not a fourth local Aravne
receipt.

Nax shop #37762 lists local object #21585, a sack of strange herbs, as a
product ([shop entry](../../../areas/shp/nax.shp#L288)). Three active foreign
ZON records also reset local objects: Alatorin resets elixir #21647, Nax resets
object #21585, and Surface Keeps resets thin quiver #21567
([Alatorin](../../../areas/zon/alatorin.zon#L4078),
[Nax](../../../areas/zon/nax.zon#L378),
[Surface Keeps](../../../areas/zon/surfacekeeps.zon#L452)). No external mobile
or object prototype definition matched a local Aravne VNUM.

Among 866 external type-25 declarations (707 active and 159 source-only), none
targets an Aravne room. All 231 external story sidecars parsed without errors.
Airship Grave's sidecar has a typed contact reference to local sand-elemental
mobile #21673 and an exact `item_vnums` reference to fine-sand object #21673
([contact](../../../areas/story/airshipgrave.story.json#L190),
[item](../../../areas/story/airshipgrave.story.json#L827)). These are separate
mobile and object prototypes sharing a VNUM. No other external sidecar has a
typed local VNUM reference.

## Compiled numeric references

The source-context scan classified the local C/C++ matches as follows:

| Local target | Static reference | Classification |
| --- | --- | --- |
| Mobile #21673 and object #21673 | [active mobile binding](../../../src/specs/specs.assign.c#L287), [`wh_corpse_to_object`](../../../src/specs/specs.winterhaven.c#L71) | Mobile #21673 is bound to `wh_corpse_to_object`, which creates object #21673 through its death callback. The same integer names distinct mobile and object prototypes; UID creation does not itself establish admitted ownership or player recovery. |
| Mobile #21549 | [active binding](../../../src/specs/specs.assign.c#L574), [`llyren` handler](../../../src/specs/specs.clfhaven.c#L27) | The Marooned xixchil's `llyren` handler is an active service callback; its accounting guard and nested-owner source risk remain as mapped in the dossier. |
| Room #21611 | [inn assignment](../../../src/specs/specs.assign.c#L2297) | The local room is assigned the inn handler. This is a service binding, not a city-restoration result. |
| Mobile #21535 | [epic teacher table](../../../src/classes/epic_skills.c#L217), [full initialization](../../../src/world/epic.c#L1368) | Babedo is dynamically assigned `epic_teacher` for epic luck during full epic initialization. The trainer's guarded PRACTICE path is separate from the legacy scroll exchange. Tenkuss #21618's nearby table entry is commented out. |
| Local objects #21617, #21633, #21641, #21645, and #21662 | [#21633](../../../src/account/chaos_eq_data.h#L108), [#21641](../../../src/account/chaos_eq_data.h#L115), [#21645](../../../src/account/chaos_eq_data.h#L132), [#21617](../../../src/account/chaos_eq_data.h#L157), [#21662](../../../src/account/chaos_eq_data.h#L201) | These objects occur in class loadout tables. The static profile references do not guarantee kit eligibility, issuance, or receipt. |
| Mobile #21672 | [high-drop mobile list](../../../src/item/randomeq.c#L127), [consumer block](../../../src/item/randomeq.c#L650) | The King of Aravne appears in the `highdrop_mobs` list, but the matching bonus loop is commented out. The list entry does not establish an active high-drop bonus. |

Other raw integer matches are constants, comments, strings, or other source
contexts. The scan distinguishes room, mobile, and object namespaces and does
not treat a numeric match alone as an active producer or route.

## Title, history, and disposition

The color-stripped title in
[clfhaven.zon](../../../areas/zon/clfhaven.zon#L2) normalizes to “The Forest
City of Aravne” and matches exactly one of the 351 active contributors. The
followed WLD and ZON histories include older source edits and the 2009 area-tree
path move; the curated [replacement register](DUPLICATE_AND_REPLACED_ZONES.md)
identifies no distinct predecessor or replacement. The current un-commented
[areas/AREA row](../../../areas/AREA#L110) remains **DO NOT MOVE**.

The existing dossier maps the three local accepted returns, exact proof
identities, services, doors, reset supply, and selected foreign sources. This
repository-wide scan adds the active and inactive WLD boundaries, typed Nax
and Dawndale consumers, three external object resets, and source-context
classifications for the dynamic bindings and configuration tables. Dynamic
reachability, editor/tooling closure, persisted identity, stock admission,
door/reset state, safe travel, and played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.
