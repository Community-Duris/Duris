# Crakkaros' Liar: comprehensive source story map

**Source-comprehensive, revision one — October 4, 2026. Gameplay qualification
remains open.** The [journal](../../../areas/story/crakkaro.story.json) covers all
eleven native exchanges with six story outcomes and five independent services.
Twenty-nine contacts retain all nine addressed dialogue families. Twenty-seven
optional checks comprise twenty-four current-material checks and three earlier
exchange histories. No native contract is omitted or grouped into a whole campaign.

**This checkpoint ships journal guidance and evidence, with no actual native
zone or quest repair.** The oversized fur payment, ambiguous badges, sculpture,
key aliases, exterior connection and unused content remain identified findings
or builder decisions. Future actual repairs require separate clear `fix` commits
where practical and prominent PR/news entries with zone, interaction, player
trigger, before/after behavior, validation, limits and a truthful news sentence.
Journal guidance and a proposed repair are different deliverables.

Active, ready accounting is mandatory for discovery, encounters, journals and
new credit. Frozen obligation persistence and recovery remain separate. Exact
supplied proof fits without personal source history. Accepted receipts record
their own exchange; they do not prove first acquisition, hunting, burial, rescue,
learning a keyword, gate passage, treasure recovery or dragon victory.

## Evidence and review boundary

Read every [native block](../../../areas/qst/crakkaro.qst): eleven QA and nine
addressed MA, with no ambient/default families. The
[reproducible source index](../../reference/zone-story-audits/crakkaro.md) retains
exact bindings, native aliases, dialogue and declared sources. QA/MA include
room-visible narrative; they do not create an extra completion for observers.

Read all 372 [rooms](../../../areas/wld/crakkaro.wld), physically 87001–87470
with gaps: all 44 exact prose groups, 23 numeric headers, 28 non-exit metadata
groups and 225 exit families. The registered lower boundary 85819 is not another
physical room. Read all 37 [mobile prototypes](../../../areas/mob/crakkaro.mob)
and all 68 [object prototypes](../../../areas/obj/crakkaro.obj). There is no local
shop file. No literal local procedure is bound in
[special assignment](../../../src/specs/specs.assign.c), and no local `_proclib_`
description reinstates the stale numeric procedure flags cleared by the loader.

Read all 501 [reset commands](../../../areas/zon/crakkaro.zon) across 199
families: 191 M, 94 D, 82 E, 64 G, 51 O, twelve F, six P and one R. Reserved
arguments are zero. All native input/reward prototypes and local reset targets
resolve, including the mount reset separately from the usual M/F source scan.
Global active-area producer scans cover all 31 input/reward kinds, additional
keys, controls and quest-like orphans. No foreign material is required.

Shared execution review includes durable quest submission/reward continuation,
mount/follower resets, container selection, automatic switches, command/object
dispatch, world/reset door flags, key consumption and ordinary NPC wandering.
These explain dependencies without emitting new accepted quest objectives.

## Eleven exchanges and their progression

Exact kinds below remain separate. The player's journal uses descriptive source
labels, with independently evaluated readiness, rather than exposing these IDs.

| Outcome | Exact input → native result | Relationship and classification |
| --- | --- | --- |
| [Centaur's brother](../../../areas/qst/crakkaro.qst#L18) | Tail 87007 → saddle 87000 | Story; centaur remains, burial is narrated |
| [Burnhard's shield](../../../areas/qst/crakkaro.qst#L59) | Finger 87016 + toe 87017 + eye 87018 + ear 87019 + nose 87020 → shield 87021 | First ogre-story stage, five different kinds |
| [Burnhard's bracer](../../../areas/qst/crakkaro.qst#L73) | Shield 87021 → bracer 87022 | Second stage; consumes shield, producer history optional |
| [Burnhard's ring](../../../areas/qst/crakkaro.qst#L84) | Bracer 87022 → ring 87023 | Third stage; supplied bracer fits |
| [Burnhard's earring](../../../areas/qst/crakkaro.qst#L95) | Ring 87023 → earring 87024 | Fourth stage; supplied ring does not complete earlier stages |
| [Dragon eyepatch](../../../areas/qst/crakkaro.qst#L107) | Scale 87028 + tooth 87029 → eyepatch 87030 | Independent service; two red-dragon instances |
| [Dragonkin whip](../../../areas/qst/crakkaro.qst#L119) | Ear 87031 + skin 87032 + foot 87033 → whip 87034 | Independent service; three source instances |
| [Illithid ring](../../../areas/qst/crakkaro.qst#L140) | Head 87035 + arm 87036 → ring 87037 | Independent service; distinct from ogre ring |
| [Devil horns](../../../areas/qst/crakkaro.qst#L152) | Horn 87039 + eye 87040 + hand 87041 → horns 87042 | Independent service; older devil and two younger devils |
| [Seventeen furs](../../../areas/qst/crakkaro.qst#L166) | 17 × fur 87080 → 100,000 base coins / 100 platinum | Guarded service; reward, no player fee; exceeds durable limit |
| [Woman's badges](../../../areas/qst/crakkaro.qst#L211) | Badge kinds 87044 + 87045 + 87046 + 87047 → ice key 87064 | Story; recipient leaves, no automatic reset renewal |

Burnhard's ogre stages have continuing narrative: successive attempts to win
the player's appreciation, each consuming his preceding creation. They retain
four story outcomes rather than turning the earring receipt into an AND campaign.
The four other part recipes are independent commissions, without prerequisites
on the ogre chain or an additional story resolution. These and ordinary fur work
are services, available in guidance without inflating achievement completion.
The classification changes authored projection only, not native contract identity,
reward behavior or recorded receipts.

Native definitions retain nine potential daily candidates: the centaur and eight
supported Burnhard part exchanges stay repeatable because their recipients remain.
Reset mode zero does **not** exclude those contracts by itself. The guarded fur
offering is `Unsupported durable offering`; the departing woman is `Story-only
quest`. Authored service classification leaves five story daily candidates in
this zone. Candidate shape still requires actual supply, surviving recipients,
accessible routes, successful accounting and gameplay qualification.

## Mounted centaur and the forest promise

The [elf's M reset and centaur's R reset](../../../areas/zon/crakkaro.zon#L443)
start in room 87007. R loads centaur 87000 as the previous elf 87001's mount,
sets sentinel/mount flags and adds the mount as a follower. The elf is not a
sentinel prototype; mounted/follower movement needs live qualification. This
is an active source, despite a source extractor that only lists M/F placements
omitting the centaur. R changes the runtime `mob` pointer to the centaur, so
following E/G commands target the mount: its equipped saddle and carried quiver
are separate from a later quest reward. The M/F-only inventory index attributes
those two sources to the elf; this limitation is explicitly corrected here.

`hello` asks about the forest promise; `drow/dark` explains the brother's tail.
Dark drow 87005 starts at 87223 and can wander within the zone, carrying tail
87007. The accepted tail exchange narrates laying the brother to rest. No burial
effect, personal kill or forest-nonviolence objective is implemented. Do not
infer a pacifist branch merely from the centaur's prose.

## Prison supply and Burnhard's chain

Burnhard starts in hut 87386. `hello/yes/no/parts` explain his collection and
former access to the prison; they are four addressed families, not four awards.
Five ogres in cell 87384 carry one different requested part each. Two red dragons
in 87382 split scale/tooth; three dragonkin in 87383 split ear/skin/foot; two
illithids in 87380 split head/arm. In 87378 the older devil has the horn and
two younger devils separately have eye and hand. Individual creature defeat
does not necessarily provide a complete bundle. Global caps, custody and actual
stock matter; declared G resets are sources, not first-recovery receipts.

Shield → bracer → ring → earring consumes three intermediate objects. Each later
entry includes one optional producer receipt and one current material check.
Supplied exact equipment fits without that personal history. Worn equipment
does not satisfy the loose-carried check, and a receipt does not restore spent
proof. Duplicate ogre parts, dragon scales, dragonkin ears or devil eyes cannot
replace different required kinds.

Twenty fur G placements share cap twenty: nine deer, four squirrels, three does
and four rabbits. Six doe M placements do not all have a fur G. Squirrels can
wander across boundaries; the other animal sources stay within the zone. An
initial declared supply can cover seventeen pieces, but live renewal, ownership
and depleted sources remain unqualified in this reset-mode-zero area.

The [durable submission](../../../src/world/quest.c#L1517) limits roots through
the [fourteen-item constant](../../../src/world/zone_story_quest_production.h#L15).
The [reward continuation](../../../src/item/quest_reward_continuation.h#L38)
also stores fourteen roots. With a complete durable fur set, submission refuses
the oversized bundle before transfer/reward publication; with an incomplete
set it may instead request the remaining items. It retains the player's items.
This is an established safety boundary, not evidence of a new crash. The journal
can show seventeen carried furs while explaining that acceptance remains guarded.
Do not reduce the native recipe or claim the inventory check made it playable.

## Four identical badges, one unrelated badge and the ice key

Four required prototypes share the exact `badge` keyword, name and description:

| Required kind | Declared sources | Current proof |
| --- | --- | --- |
| 87044 | Maial wears it in 87297; paladin 87025 has another in 87338 | Matching Maial kind, one item |
| 87045 | Malinok wears it in 87219; paladin 87025 has another in 87337 | Matching Malinok kind, one item |
| 87046 | Burnherf carries it in 87040; paladin 87026 has another in 87335 | Matching Burnherf kind, one item |
| 87047 | P reset into bookshelf prototype 87012 | Fourth kind, retrieved loose from a shelf |

The [P handler](../../../src/world/db.c#L3716) calls
[get_obj_num](../../../src/world/handler.c#L2370), selecting the first existing
matching container in the world object list. Thirty-seven bookshelf O placements
share prototype 87012. A P line following one O does not guarantee permanent
custody in that O's room on later resets. Guidance therefore points to actual
shelf contents rather than inventing a fixed room. Shelf-contained or worn
badges are not currently loose-carried proof. Worn variants are still the exact
same required kind once actually recovered into inventory.

Badge 87071 sits in prison-guard-corpse container 87072 in cell 87376. Its visible
name is also identical, but it is a fifth kind and cannot satisfy the woman's
contract. Her claim that all badges require killing trusted temple members is
not a prerequisite enforced by the native offering: the bookshelf source and
supplied materials contradict that interpretation. Preserve exact contract
matching and obtain builder intent before altering badge names or dialogue.

The woman starts in basement 87410. `lover/paladin/paladins/statue` introduce her
missing lover and treasure promise. Four exact badges give ice key 87064, followed
by D departure. Reset mode zero supplies no ordinary automatic reappearance.
No native lover-rescue, statue ritual, dragon-kill or treasure-recovery contract
follows. The key is a single-use key with value[1] 100; its actual aliases are
`ice white`, lacking `key` despite the displayed name. Use an existing alias;
adding a compatible key alias is a distinct pending native content repair.

## Access, sculpture and the icy finale

| Route | Declared prerequisite | Limits |
| --- | --- | --- |
| Main gate 87084 north / 87085 south | Steel key 87001 from a gate guard | Persistent key; unlock and open separately |
| Watch-tower 87040 down / 87466 up; 87216 down / 87220 up | Small key 87006 from a guard in 87466 | Persistent; latter route is secret; source is reached from another approach |
| Northwest study 87218 up / 87219 down | Tower key 87010 from tower guards | Single-use key |
| Northeast study 87296 up / 87297 down | Forward key 87010, reverse key zero | Asymmetric native key field; builder intent/reverse escape needs review |
| Main prison and ten cell gate pairs | Prison key 87015 from guards | Eleven declared keys, all single-use; source and prior use matter |
| Basement 87418 down / tunnel 87428 up | Ice key 87064, woman's reward | Secret, locked, pickproof; single-use key, departure/return qualification needed |

The [native unlock handler](../../../src/cmd/actmove.c#L3132) also clears lock
and secret state on an actual reciprocal back exit. A single-use key therefore
does not automatically require a second key for the ordinary immediate return.
The asymmetric tower key field alone does not prove a trapped player after a
successful forward unlock; alternative approach, relocking and reset scenarios
need separate qualification.

Sculpture 87062 is an automatically bound ITEM_SWITCH with values
`[346,87418,5,1]`: command 346 is `envy`, targeting the basement's down exit.
[item_switch](../../../src/specs/specs.object.c#L309) only clears EX_BLOCKED and
returns "Nothing happens" when that bit is absent. The
[basement reset](../../../areas/zon/crakkaro.zon#L376) uses state six, which adds
secret + locked/closed, without EX_BLOCKED. The loader masks initial world exit
state to the two door-shape bits; the raw seven is not an extra live block.
Thus the declared switch does not reveal this currently unblocked route. The
existing secret-key route remains a separate access path, so this does not prove
the entire cavern is inaccessible. Builder decisions could retain/remove the
decorative switch or implement an intended reveal/control; validate both sides,
reset and return behavior before any future separate fix/news entry.

The ice caves have nineteen ice-devil M placements, ten ice-golem M placements,
nine wall/glyph objects with trap metadata and Crakkaros in 87465. Crakkaros has
five declared treasure items, alongside four ordinary prison-guard placements.
The spiral staircase links the lair-side approach to 87466 beneath the watch
tower. Existing treasure is not a quest payout triggered by the badge handoff.
Neither meeting the dragon nor accepting a key certifies defeating guardians,
obtaining treasure, reopening the temple, resolving the lover or a safe return.

The only positive exterior exit is 87008 south → 259959, absent from the active
static `areas/AREA` room inventory; no active static room points back into this
zone. Inactive `surf.wld` and `surface2011.wld` contain an ocean room with that
number, and no reciprocal Crakkaro exit there. The world generator assembles
AREA-listed sources, while runtime reads generated `areas/world.wld`. The
[world renumbering handler](../../../src/world/db.c#L1533) removes exits whose
positive target does not resolve. An AREA-only assembly therefore cannot retain
this connection without another actual source of the target. Qualify the intended
deployed assembly, entry and return before selecting a replacement. Room 87177 has a southwest exit to -1, which
the loader discards; it is not another positive foreign boundary.

## Findings, planned capabilities and fair repair proposals

| Finding | Needed work | Current status |
| --- | --- | --- |
| Seventeen-fur payment exceeds fourteen-root durable limit | Bounded larger batch plus versioned continuation/recovery compatibility, atomic reward/currency publication, duplicate distinct UIDs, incomplete-set retention, rejection/replay/crash recovery | Guarded; recipe/payment unchanged, no repair ships |
| Four identical required badges plus identical unrelated badge | Builder-approved distinguishing aliases/descriptions or provenance-aware labels without changing identities; test alternatives and wrong fifth kind | Journal labels/readiness clarify sources now; native repair proposed |
| Mounted centaur omitted by M/F-only source extraction | Include R source, rider association and actual post-R E/G owner; qualify mount movement and encounter visibility | Source explained manually; extractor enhancement planned |
| Same-prototype P container selection | Capture committed item/source/container UID and actual custody; support location hints qualified against active instance | No permanent shelf room fabricated |
| Sculpture expects a block absent from reset | Builder selects intended control and key relationship; verify successful dispatch, both door sides, reset, unlocking and return | Pending decision; no automatic route activation |
| Ice-key name lacks key alias; reverse tower key differs | Compatible alias and intentional two-sided key review, with exact before/after proof | Separate native content repairs proposed |
| Exterior target only in inactive map sources | Qualify generated world ownership and actual entry/return; builder selects intended connection | Static finding; live blockage unproven |
| Tail burial, lover rescue, temple sealing and dragon finale narrated/implicit | Builder defines resolution and actual successful effect/report endpoints; scoped AND campaign with supplied-material branches | No invented credit |
| Unused heart 87027, alternate prison key 87075 and level potion 87082; extra badge 87071/token 87043 | Decide decorative/retired/future story intent; supply and endpoint only if desired | Quest-like orphans explained, not automatically called broken |

Extend causal first-source acquisition versus player handoff with durable object
UID, actor, source instance, actual container/corpse/mount context and committed
transfer/reward operation. Keep current possession, accepted producer history,
source defeat and actual acquisition separate. Extend accepted reveal/key-use,
unlock/open, arrival and reset-generation events with precise targets and
ownership/survival scopes. A switch callback, item name or declared reset is not
evidence that those actions succeeded. Completion arrays currently express OR
alternatives; a future complete-temple episode requires implemented AND semantics
and builder-selected terminal effects, with supplied-proof branches respected.
Add deeper schema fields only with implemented parser/runtime, persistence and
rejection/replay/recovery tests; no inert fields ship in this mapping.

## Verification and remaining qualification

Focused existing source and C++ projection tests cover all eleven exact bindings,
six story/five service classification, four different badges, wrong extra badge,
five different ogre parts, supplied intermediate gear without invented earlier
history, spent/worn proof, seventeen-fur current readiness, no credit from reads,
receipt replay and cold recovery. Native daily shapes, mounted source, actual
P selection, sculpture/reset mismatch, keys and orphan sources are asserted.
Catalog generation, daily report, accounting-gate/tracking regressions, maintained
server build, changed/staged formatting, whitespace and documentation links are
checked for the published checkpoint.

Live mounted encounters, source hunting/stock, unsafe/secret routes, retirement,
larger-bundle acceptance, trap effects, world assembly, cavern traversal and any
future finale remain unqualified. No database, account activation, operational
script, generated world edit or live server mutation is part of this checkpoint.
The global catalog becomes 71 journals / 1,658 achievement units / 1,474 potential
daily units / 2,218 projected rows. All 2,668 native definitions, content revision
two, source fingerprint, zone registry and prior seventy sidecars are preserved.
The original queue has 50/220 source-comprehensive areas and 170 pending; Rogue
Plains (`roguerai`) is next.
