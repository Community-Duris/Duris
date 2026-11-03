# Ny'Neth's Stronghold: first stage

Active source `nyneth`, zone 229, covers 78 rooms from #22900 through #22996, 32 mobile prototypes, and 47 objects. The area is in both `areas/AREA` and `areas/qst/AREA`, but it has no local QST file, no local giver or contract, and no story sidecar. The authored story is carried by room prose, gates, portals, reset populations, and combat props.

Sources: [world rooms](../../../areas/wld/nyneth.wld#L1), [mobiles](../../../areas/mob/nyneth.mob#L1), [objects](../../../areas/obj/nyneth.obj#L1), [resets](../../../areas/zon/nyneth.zon#L1), [full-world membership](../../../areas/AREA#L116), [QST-area membership](../../../areas/qst/AREA#L109), [route, dependency, and history review](../quest-story-followups/NYNETH_AUDIT_REVIEW.md).

## Story in the source

On the Surface map, a room titled “Nyneth” (#581017) leads south to the rocky foothills (#581417), whose north exit enters #22900. The stronghold approach passes an archway and smooth earthen corridors, then reaches a chamber holding a chained green wyrm. Gas, an altar, and two named keys establish a guarded threshold before three shimmering portals. The prose signals danger and a progression through the stronghold; it does not define a tracked quest or a saved completion.

The first portal, object #22902, leads from portal room #22908 to the boulder maze at #22930. The main route moves through a hidden-panel rock maze, the Hall of Titans, the Cavern of the Living Dead, the River of Blood, lava banks and a bridge, and the Hall of Emissaries. The audience chamber at #22993 says a construct holds Ny'Neth's mind. The next room, #22994, is titled the First Vault and its prose presents a trophy case after that confrontation. The source does not record a release, restoration, or other narrative result when the construct is defeated.

The named keys make parts of the route legible: Ny'Neth's key #22900 and the wyrm key #22901 secure the entry passage; the straw key #22930 and lightning key #22931 protect doors around the Titans and dead; vault key #22965 gates access to the first trophy room. Their WLD exits and reset door states are listed in the [route review](../quest-story-followups/NYNETH_AUDIT_REVIEW.md#active-wld-graph-and-approaches). Other objects include a shard collection, lightningstone hammer, reed spear, lava rock, potions, vault equipment, and the second Ny'Neth key. These are authored challenge and treasure props, not QST inputs.

## Portals and stage boundaries

The entrance component has three reset portals: #22902 leads to the main complex at #22930, #22903 leads to Nyneth2 at #23000, and #22904 leads to Nyneth3 at #38700. They are type-25 objects using the `enter` command and unlimited charges. A dark portal, #22966, is placed in the first vault and returns to entrance room #22908. The same #22966 prototype is reset into the second vault in Nyneth2, so its ownership and destination are shared across stages.

These item routes conditionally bridge areas that the WLD graph leaves disconnected. The two sequel portals make this first stage part of a three-stage campaign, but their declarations do not establish actual arrival, intended admission, completion prerequisites, or a safe return. See the separate [Nyneth2 dossier](NYNETH2.md) and [Nyneth3 dossier](NYNETH3.md) for their native exchanges and stage-specific stories.

## Resets, rareloads, and special behavior

The zone file contains 311 reset rows: 22 door-state (`D`), 12 object (`O`), 93 mobile (`M`), 96 equipment (`E`), 60 give (`G`), and 28 follower (`F`) commands. The door rows configure eleven paired exits as closed, locked, or secret; several pairs name the keys above. The reset table places five named mobiles in #22995: Vlessus, Tssiss, Drkehn, Nian, and Agaroch. Each of those `M` rows has its percent field set to 100. Room #22995 says “25% chance of loading each mob,” so the source contains a real configuration-versus-description question. Room #22996 is described as a load room for rareloads that will not enter the grid, but no local reset row targets it.

Mobile #22956, the construct, is assigned the `construct` special procedure. Its code adds a combat-turn toss or knockdown effect for nearby eligible victims; this is combat behavior rather than a quest completion. Object #22962, the Hammer of Titans, is assigned `hammer_titans`, which can call a lightning-bolt effect on a hit. Neither source binding records a story outcome. The first vault also imports Heavens object #360, the rune-covered monolith, so its separate epic payout path must remain distinct from ordinary trophy-room loot.

## Current interactions and services

There is no local `areas/qst/nyneth.qst`, local SHP file, or `areas/story/nyneth.story.json`, despite the QST-area manifest entry. There are no local QST giver records or contracts. The available story is environmental and encounter-based. Any future tracked objective would need an explicit admitted action, proof, beneficiary or reward, replay policy, and saved result; room text, keys, a boss fight, or portal arrival alone do not supply that contract.

Other active zones use Nyneth prototypes directly. Alatorin, Earth Plane, Ravenloft2, Stormhold, Nyneth2, and Nyneth3 contain typed reset references to local objects or mobile #22904. Preserve those exact dependencies as detailed in the [typed-reference review](../quest-story-followups/NYNETH_AUDIT_REVIEW.md#exact-typed-dependencies-and-compiled-bindings). The local title is unique among the 442 scanned `.zon` titles. The active `nyneth2` and `nyneth3` areas are distinct sequel stages, not duplicates or replacements.

## Builder decisions

The source defines a guarded entrance, a portal-led campaign, a boss-like construct encounter, a first trophy vault, and rareload staging rooms. It leaves the portal admission, some key and gate intent, the rareload rate, the construct's narrative outcome, and the missing local quest record unresolved. The [audit review](../quest-story-followups/NYNETH_AUDIT_REVIEW.md#checkpoint-315-twenty-direct-builder-questions-and-recommendations) records twenty source-backed questions and conservative recommendations. Keep the active contributor **DO NOT MOVE** while those decisions and its cross-area prototype dependencies remain in place.
