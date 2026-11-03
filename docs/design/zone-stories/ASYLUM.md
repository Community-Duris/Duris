# The Temple of Lizizania (`asylum`) source story dossier

This dossier maps the active area's authored environmental story across its
temple, wings, underground cells, and surrounding paths. The [Checkpoint 191
audit supplement](../quest-story-followups/ASYLUM_AUDIT_REVIEW.md) records its
full WLD graph, door resets, active and source-only approaches, typed
dependencies, title scan, and bounded history.

## Story shape in the authored rooms

### A public temple behind a frozen approach

The outside paths cross snow-covered permafrost and lead past a wrought-iron
lichgate toward the temple's oaken doors. The courtyard holds a frozen pond,
fish, and benches; its prose promises quiet and relief from worries. Inside,
the dark fountain and polished stone pool contrast with the cold landscape.
An occasional worshiper comes to pay homage to the Serpent Goddess
([temple approach](../../../areas/wld/asylum.wld#L1786),
[lichgate](../../../areas/wld/asylum.wld#L1646),
[courtyard](../../../areas/wld/asylum.wld#L1746),
[temple interior](../../../areas/wld/asylum.wld#L1626),
[fountain](../../../areas/wld/asylum.wld#L40)).

The altar room centers on a hematite altar marked with the goddess's intertwined
woman-and-serpent emblem. The Head Priestess chants behind it and invites
visitors to leave offerings in exchange for the goddess's favor
([altar](../../../areas/wld/asylum.wld#L16),
[Head Priestess](../../../areas/mob/asylum.mob#L144)). The worshiper prototypes
describe people traveling through the cold to leave a small gift
([female worshiper](../../../areas/mob/asylum.mob#L204),
[male worshiper](../../../areas/mob/asylum.mob#L218)). This is environmental
and dialogue evidence; the area has no local Q/QA contract or recorded
completion state.

### Temple labor, learning, and rank

The East and West Wings hold a library, sleeping chambers, meditation room,
service corridors, and a later-cut shortcut. Tapestries, dust, brooms, food,
and spare clothing show routine work in a worn building. Initiates are new and
overwhelmed by the layout. A blue priestess searches for her ward, saying she
was due at the temple at the “Hour of Rememberance”; the source does not
identify a matching task or establish that the initiate is that ward
([East Wing](../../../areas/wld/asylum.wld#L67),
[library](../../../areas/wld/asylum.wld#L231),
[meditation room](../../../areas/wld/asylum.wld#L610),
[sleeping chambers](../../../areas/wld/asylum.wld#L646),
[initiate](../../../areas/mob/asylum.mob#L73),
[blue priestess](../../../areas/mob/asylum.mob#L91)).

### The cells beneath the temple

The lower tunnels are cold, dark, and littered with rat remains. Their cramped
cells describe rotted straw, blood, urine, vomit, and people the priestesses
call “undesirables.” One room questions who decides whether an individual can
function in the outside world. In the blood-drenched treatment room, a metal
table and restraints frame the Head Priestess's successor drilling into
patients' skulls and applying leeches. The unconscious patient protests and
asks for the drill to be removed; the room and mobile copy together portray
the supposed cure as coercive and harmful
([cell block](../../../areas/wld/asylum.wld#L705),
[treatment room](../../../areas/wld/asylum.wld#L1014),
[cell conditions](../../../areas/wld/asylum.wld#L784),
[the cell's question about who decides](../../../areas/wld/asylum.wld#L824),
[patient](../../../areas/mob/asylum.mob#L160),
[black priestess](../../../areas/mob/asylum.mob#L124)).

An especially dark connecting passage is explicitly described as a shortcut
added later to the temple's winding halls. That is an in-world construction
detail, not evidence that another area replaced this zone
([connecting passage](../../../areas/wld/asylum.wld#L1517)).

## Story-bearing reset items

The Head Priestess carries emerald key #9213. It matches the key fields on the
two secret exits between altar room #9201 and niche #9200. The ZON file resets
both directions locked and secret. The niche contains an animal-shaped chest
with a nested shield of humanoid skulls
([door and key fields](../../../areas/wld/asylum.wld#L11),
[key reset](../../../areas/zon/asylum.zon#L256),
[chest reset](../../../areas/zon/asylum.zon#L233),
[nested shield reset](../../../areas/zon/asylum.zon#L234),
[key prototype](../../../areas/obj/asylum.obj#L183)). The altar remains
reachable from the fountain by a separate ordinary exit; the key governs the
hidden niche passage.

The female servant's coffee cup #9225 is reset in the temple and wing; the
mobile's copy also describes her carrying steaming coffee
([servant resets](../../../areas/zon/asylum.zon#L310),
[coffee object](../../../areas/obj/asylum.obj#L327),
[servant](../../../areas/mob/asylum.mob#L190)). Another active zone consumes
that same object as merchant stock and carried stock; the audit supplement
records those exact references. Neither the key, coffee, sacrificial chalice,
nor visible gifts establish a native quest outcome.

The altar room also resets the onyx altar and blood-filled chalice, while the
niche chest holds the nested skull shield
([altar reset](../../../areas/zon/asylum.zon#L235),
[chalice reset](../../../areas/zon/asylum.zon#L236),
[chalice prototype](../../../areas/obj/asylum.obj#L471),
[nested shield prototype](../../../areas/obj/asylum.obj#L214)).

## Builder questions and recommended resolutions

| Question for the area owner | Source evidence | Recommendation |
| --- | --- | --- |
| Does the High Priestess's invitation represent a real offering service? | Her dialogue invites an offering, but no local QST contract, shop, or story sidecar records an accepted item or outcome; worshiper gifts alone do not prove completion. | Define an accepted item or alternative, beneficiary, and persistent result before presenting it as a working task. |
| Is the missing ward the hurried initiate, or are they separate people? | A blue priestess worries that her ward is missing, while an initiate appears elsewhere; sources give no matching identity, route, or resolved state. | Confirm identity and intended search path before describing a successful rescue or reunification. |
| Is patient care or escape intended as a player-facing story? | Cell and treatment-room prose describes severe abuse and patients but no local contract or recorded patient outcome. | If a rescue is intended, define patient agency, consent, treatment, survival, recipient, and safe destination. Entry, combat, or item custody alone is not rescue. |
| Is key #9213 meant to lead to the hidden niche chest, and does the skull shield have a story purpose? | The key resets for the hidden niche passage and a chest contains a nested skull shield; the altar itself has a separate ordinary approach. | Preserve the distinct passage and altar routes. Confirm whether the shield is loot or a defined story item before attaching an outcome to it. |
| Are the worship imagery, fountain, and frozen courtyard meant to have supernatural, healing, or refuge effects? | Rooms describe worship, a fountain, and frozen grounds, but sources record no matching effect or safe-arrival result. | Keep those effects as unconfirmed atmosphere unless an authored interaction defines an admitted action and lasting result. |
| Should `asylum` have any command-target alias beyond its normalized title “The Temple of Lizizania”? | The title is unique in the 442-title scan; no additional command-target alias is established in the bounded area sources. | Keep `asylum` and “The Temple of Lizizania” as the source key and display identity. Add an alias only after the owner names its exact target and intended distinction. |

