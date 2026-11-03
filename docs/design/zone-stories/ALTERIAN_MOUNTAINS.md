# Alterian Mountains

Canonical key: `component:alterian_mountains`. Physical coverage row 358.
This is a source-only mountain and wilderness map, not an active listed zone.
The [technical audit](../quest-story-followups/ALTERIAN_MOUNTAINS_AUDIT_REVIEW.md)
reconciles its room and object identities with active Mining and documents
its two-way source dependencies with Alterian Wilderness. The
[builder questions](ALTERIAN_MOUNTAINS_BUILDER_QUESTIONS.md) preserve the
owner decisions needed before any source changes.

## The authored mountain journey

The map starts on a winding trail into the Alterian range and climbs Mt.
Gandar through steep passes, narrow ledges, scrambles, and cracks in the
mountain face. The descriptions make the hike difficult and sometimes risky,
but do not specify a route's current access or safety behavior. The trail
opens onto wide views of forests and plains and then into a high lake whose
warm, steaming water suggests heat from a deep fissure.

Several cave passages give the landscape a hidden interior. Room #4956 is a
small natural cave littered with rocks and twigs; #4957 continues into a dark
lair with humanoid bones. At #4980, steam fills a hot cave around a deep hole.
Its down exit reaches #4750, “Vertical Cavern,” in the unregistered Alterian
Wilderness source. Both files describe source paths, not a confirmed live
connection.

The southern rooms describe Keeper's Dale, a smooth-sided gorge with a hidden
staircase carved into the rock. Rooms #4997–#4999 descend to a broad lower
area and two huge metal doors. The east exit from #4999 points to `-1`; the
prose hints at a sealed destination without defining one. The map keeps
environmental questions open: why the lake is warm, where the cave steam
comes from, what lives in the bone-filled lair, and whether the doors were
intended to open somewhere.

The encounters range from a treant, dire wolves, and a Tharnadian scout to a
hill giant, primitive humans, troglodytes, farm residents, an adventurer, and
mountain wildlife. They establish a broad ecology around the trail rather
than a local quest contract. There is no same-stem QST, SHP, map JSON, or
story JSON to define an accepted task or service.

## Separate source identities

The Mountain ZON title is “Alterian Region - Mountains,” zone number 49.
Active Mining also uses zone number 49 and defines a different 69-room map.
The Mountain source contributes 80 rooms from #4920–#4999; its 49 shared
room VNUMs all have different records from active Mining. Mountain object
#4900 is `nothing`, while active Mining object #4900 is a wooden crate. The
numeric matches are not evidence that the files are copies.

The Mountain map's rooms #4920–#4999 overlap Mining, while the separate
Alterian Wilderness map supplies the source-side destinations #4919 and
#4750. Wilderness #4919 also has a third, different active Mining room
identity. Keep all three source identities distinct until the owner settles
the VNUM and loader rules. Active areas/AREA lists Mining, not the Mountain
or Wilderness files.

Mountain and Wilderness reset sources share prototype ownership. Mountain
resets use Wilderness mobiles #4812/#4813 and object #4806; Wilderness resets
use 30 Mountain mobile references across 13 VNUMs. Compiled code assigns a
`wanderer` callback to Mountain mobile #4830. Those dependencies need to stay
visible even while the sources remain unlisted.

Keep `alterian_mountains` **RETAIN IN PLACE** with candidate row 7. Do not
merge its rooms with Mining or combine its resets with Wilderness based on
numeric overlap. See the [audit's identity and dependency findings](../quest-story-followups/ALTERIAN_MOUNTAINS_AUDIT_REVIEW.md#numeric-identity-and-bounded-history),
[candidate row 7](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L7),
and [coverage row 358](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L358).
