# Hell, Phlegethos

This dossier covers the active Hell, Phlegethos contributor. The [audit supplement](../quest-story-followups/HELL_FOUR_PHLEGETHOS_AUDIT_REVIEW.md) records the checked-in source census, directional reachability, typed dependencies, title history, and retention disposition.

## Environmental story

All 400 room prototypes, #52200-#52599, have the same title, The Plane of Phlegethos on the Nine Hells, and the same environmental paragraph. It describes a volcanic, tortured plane of gouting fires, ash hills, smoking-dung pits, oppressive heat, tremors, eruptions, and fissures. It says intruders keep moving and running because the ground is uncomfortable, but the source provides no named landmarks or distinct room scenes. The wording includes the apparent typo fouth and the unusual phrase gouting fires ([first room text](../../../areas/wld/hell_four_phlegethos.wld#L1)).

The mobile and object files each contain one placeholder prototype named nothing monster and nothing. The zone reset file contains no M, O, G, E, or P commands, so it does not place either prototype. No authored population or object-driven story appears in these local sources ([mobile](../../../areas/mob/hell_four_phlegethos.mob#L1), [object](../../../areas/obj/hell_four_phlegethos.obj#L1), [zone resets](../../../areas/zon/hell_four_phlegethos.zon#L1)).

## Quest and story boundary

The contributor is listed in both areas/AREA and areas/qst/AREA, but no local QST file, SHP file, story sidecar, or native Q/QA contract is present. There is no authored player objective, accepted outcome, reward, or named encounter in the checked-in zone sources. Treat its current story as environmental copy only; do not infer a quest from the QST roster entry.

## Builder questions and recommended resolutions

| Builder question | Recommended resolution |
| --- | --- |
| Are 400 rooms with one repeated title and paragraph the intended presentation of this plane? | Confirm whether the uniformity is deliberate. If the area is meant to communicate a meaningful journey, add clearly named landmarks and distinct environmental beats under normal area ownership; otherwise document that the repeated room copy is intentional. |
| Are the 86 directional exits without a matching reverse exit intentional one-way passages? | Establish the intended entry, route, and return before relying on the room range as reachable. Review the four rooms not reached directionally from #52200 and either pair unintended one-way links or label deliberate one-way connections in builder notes. |
| Is the Phlegethos portal object in Bronze Citadel intended as an active arrival path? | Confirm the portal's owner, loader, destination, and return route. The checked-in template has destination value 99999 and no reset or code loader was found; do not count it as a working route until its runtime path is established. |
| Are the empty reset block and placeholder mobile/object intentional? | Preserve their VNUMs while the area remains active. Have the owner either document their reserved role or provide the intended population and behavior through the normal content workflow. |
| Is the area intended to remain quest-free, and are the copy typos deliberate? | Keep it classified as environmental-only unless a builder defines an objective and accepted outcome. Correct fouth to fourth; confirm whether gouting is intended wording before changing it. |

## Connectivity and retention boundary

The static WLD has one weak component containing all 400 rooms, but directional reach from #52200 covers 396. Four rooms are not directionally reachable from that seed, and no external WLD exit leads into or out of the range. No actual player arrival room is established by these static records; see the [connectivity report](../quest-story-followups/WORLD_CONNECTIVITY.md#checkpoint-224-hell-phlegethos-directed-grid-and-ingress).

Hell, Phlegethos remains an active full-area contributor and is **DO NOT MOVE**. The named Bronze Citadel portal prototype is an unresolved source reference, not proof of a live entrance. No attic candidate or migration plan is established for this active area.
