# Ant Forest (`ants`) source story dossier

This dossier maps the environmental story and visible NPC/item hooks in the
active zone source. Its two data-driven switch objects open blocked passages;
they do not create a native quest record or a saved story outcome. The
[Checkpoint 183 audit supplement](../quest-story-followups/ANTS_AUDIT_REVIEW.md)
records the source graph, switch wiring, cross-area dependencies, title/history
review, and archive disposition.

## Story shape in the authored rooms

### A quiet snowy forest with hidden ways in

Ant Forest begins in a dark, snow-covered conifer forest, with small wildlife
and a quiet wind melody. A reciprocal Surface road runs west from room #10900
([forest entrance](../../../areas/wld/ants.wld#L1),
[active Surface link](../quest-story-followups/ANTS_AUDIT_REVIEW.md#active-wld-graph-and-inactive-approaches)).
Two concealed paths branch deeper: one through dense foliage and one behind a
snow bank ([branch and path](../../../areas/wld/ants.wld#L58)). Reset-loaded
hares, snowy paths, and the outer arctic plain contrast with the danger found
beneath the ice ([hare](../../../areas/mob/ants.mob#L17),
[plain](../../../areas/wld/ants.wld#L597)).

### The ant corpse opens a blocked descent

The room beyond the foliage contains a broken ant leg and a mutilated ant
corpse. Its extra text says the sight makes the viewer want to “cry” over it
([room](../../../areas/wld/ants.wld#L83),
[corpse object](../../../areas/obj/ants.obj#L14)). That wording matches the
corpse's native switch data: object #10901 is an `ITEM_SWITCH` whose command is
`CMD_CRY` and whose target is room #10904's down exit. The generic
[`item_switch` handler](../../../src/specs/specs.object.c#L302) clears the
target exit's `EX_BLOCKED` bit when the command matches; the generic object
loader binds that handler to switch objects
([switch type](../../../src/core/defines.h#L144),
[automatic binding](../../../src/world/db.c#L5281),
[`CMD_CRY`](../../../src/cmd/interp.h#L88)). This is an authored environmental
puzzle, not a QST hand-in. The path remains secret, and the source reset still
defines the exit's door state, so the code path alone does not establish what a
player sees or can traverse after a live reset.

### The handprint and the snow-hidden tunnel

Deeper in the frosty tunnels, an enclave contains a field of unexplained
etchings and a name, “liilen,” beside a large handprint. Its extra room text
explicitly invites a touch ([enclave](../../../areas/wld/ants.wld#L310)). The
handprint object #10902 is another `ITEM_SWITCH`: its values bind `CMD_TOUCH`
to the north exit of room #10915, whose description says packed snow conceals
a second tunnel ([object](../../../areas/obj/ants.obj#L31),
[snow-hidden exit](../../../areas/wld/ants.wld#L291),
[room reset](../../../areas/zon/ants.zon#L40),
[`CMD_TOUCH`](../../../src/cmd/interp.h#L355)). The object resets in the
enclave, while the targeted exit is in a different room. The generic handler
can clear a blocked exit remotely; it does not remove the remaining secret
flag or record a lasting quest outcome. No live interaction was qualified in
this review.

The two switches make the corpse and handprint part of actual passage design.
The etchings and the name remain unexplained: the source supplies no local
QST contract or story sidecar that says who left them or what the text means.

### An ice colony, its caretakers, and the lower rift

The lower tunnels contain a disciplined frost-ant colony: a fierce white
warrior guards the vale, workers carry ice, a supervisor watches the workers,
and a red handler tends frozen larvae in a chain of spherical burrows
([warrior](../../../areas/mob/ants.mob#L1),
[workers and supervisor](../../../areas/mob/ants.mob#L32),
[handler and larvae](../../../areas/mob/ants.mob#L65),
[burrows](../../../areas/wld/ants.wld#L485),
[handler and larva resets](../../../areas/zon/ants.zon#L77)). The room sources
describe the burrows' gelatinous material as odorless, in contrast with the
lower rift, where ant remains are half-digested in purple slime
([foul-smelling rift](../../../areas/wld/ants.wld#L905),
[green slime](../../../areas/mob/ants.mob#L111)). The source frames colony
labor, caregiving, predation, and decay, but has no rescue, investigation,
colony-resolution, or survival state.

The zone also resets a crown of ethereal frost onto an ant colony keeper in a
room titled `Void` ([crown](../../../areas/obj/ants.obj#L43),
[keeper and crown resets](../../../areas/zon/ants.zon#L127)). The static
entrance graph does not reach that room, and the adjacent second `Void` room
has a self-loop. The [audit supplement](../quest-story-followups/ANTS_AUDIT_REVIEW.md#active-wld-graph-and-inactive-approaches)
records this placement without assuming it is intended player stock.

## Builder questions and recommended resolutions

| Question for the area owner | Source evidence | Recommendation |
| --- | --- | --- |
| Are the corpse and handprint meant to be the two discovery steps for the blocked passages? | The corpse is wired to `cry` at room #10904 and opens its down exit; the handprint is wired to `touch` at room #10916 and opens the north exit in #10915. Both targets are still secret exits in WLD source. | Preserve the existing command/object/room mapping. Confirm reset door states, remaining search requirements, messages, and return paths in an authorized live qualification before adding more gates or promising access. |
| What do the etchings and the name “liilen” mean? | The enclave contains a cipher-like pattern, the name, and the handprint, but no local contract or explanation. | Decide whether these are a self-contained clue or a lead to another area. If they should resolve, name the intended reader, clue, result, and persistence before adding a quest. |
| Is the ant colony meant to be investigated or simply encountered? | A butchered ant, defensive warrior, organized workers, supervisor, larval caretakers, and an unexplained lower rift sit on the same authored route. | Decide whether the corpse/rift implies a player-facing ecological mystery. If so, specify evidence and a useful colony outcome; otherwise keep the prose as atmosphere and encounter context. |
| Is the keeper and frost crown meant to appear in the `Void` room? | The zone resets mobile #10906 and equips it with crown #10903 at #10939, which the raw entrance graph cannot reach; room #10940 is a separate self-loop tail. | Confirm whether the rooms are deliberate staging/transport destinations or misplaced encounter stock. Resolve the map/reset owner before describing the keeper or crown as normally discoverable. |
| What should an arrival guide say about the active route and the inactive Surf map? | The active Surface boundary is reciprocal at #10900/#526267. A second approach to #10900 exists only in inactive `surf.wld`. | Document the active reciprocal pair as the supported authored road and keep the Surf record classified as source-only until its source family is separately activated. |
| Is `Ant Forest` the only intended display and command-target identity for source key `ants`? | The normalized ZON title is unique among 442 current titles; the reviewed local area sources establish `ants` and “Ant Forest” but no additional alias mapping. | Preserve `ants` and “Ant Forest” as the source key and display title. Add another alias only after its exact player-facing target and scope are confirmed by the owner. |

Ant Forest has no local QST file, no shop rows (its `.shp` file is empty), and
no story sidecar. The two `ITEM_SWITCH` objects provide native passage
interactions, but no quest or colony outcome is recorded. Preserve that
distinction in any later area work and keep Ant Forest in the active map.
