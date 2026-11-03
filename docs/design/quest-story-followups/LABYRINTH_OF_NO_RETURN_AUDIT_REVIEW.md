# Labyrinth of No Return: repository-wide connectivity and dependency audit

**Checkpoint 85; priority 125 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive dossier](../zone-stories/LABYRINTH_OF_NO_RETURN.md)
with the active WLD graph, inactive WLD sources, exact typed references,
compiled numeric matches, normalized title, and bounded source history. Authored
edges and code destinations do not establish ordinary availability, safe
travel, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. Labyrinth of No Return has 296
rooms and 602 exits. All 602 resolve to active rooms; its internal-exit graph
has one weak component of 296 rooms. Its only cross-area edge is reciprocal:

| Labyrinth edge | Active return | Cross-area register rows |
| --- | --- | --- |
| #5000 up to Mountain Tracks #20971 | Mountain Tracks #20971 down to #5000 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L87), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L349) |

There are no negative or positive unresolved Labyrinth exits. The one global
active-WLD duplicate room VNUM, #51800 in Hell Two and Hell Three, is unrelated.

## Inactive WLD sources

The all-95 inactive WLD scan parsed 2,080,773 exits without issues. It found no
inactive source-room VNUM overlap with Labyrinth and no inactive exit into an
active Labyrinth room. There are no source-only approaches to add. The curated
source-only total remains 1090, and the reviewed Surf subset remains 120.

## Typed references and compiled numeric matches

External typed scans found these existing cross-zone references:

| Source | Exact local targets | Classification |
| --- | --- | --- |
| Active Tower QST and story sidecar | Objects #5011, #5016, and #5035 | The active Tower quest's three item commands and structured item_vnums belong to Smedgewack's exact calcite/flask/tattoo return. The Tower dossier already maps that recipient and ownership; these are not additional Labyrinth receipts. See [QST commands](../../../areas/qst/tower.qst#L28), [structured records](../../../areas/story/tower.story.json#L172), and the Labyrinth source placements for [calcite](../../../areas/zon/labyrinth.zon#L213), [flask](../../../areas/zon/labyrinth.zon#L256), and [tattoo](../../../areas/zon/labyrinth.zon#L71). |
| Source-only labyrint.qst | #5031 and #5001, #5002, #5003, #5005, #5006, #5007, #5008, #5009, #5020 | This unregistered spelling contains ten typed item commands. They duplicate the active Sir Uthor Knight branch's exact ten item-command IDs, rather than adding a second active consumer. labyrint.qst is absent from both loader lists; the active QST uses labyrinth. See [source-only commands](../../../areas/qst/labyrint.qst#L30), [active Knight branch](../../../areas/qst/labyrinth.qst#L93), [world list](../../../areas/AREA#L26), and [QST list](../../../areas/qst/AREA#L24). |
| Active caertannad.zon and troll_caves.zon | Object #5019; object #5041 | Four Caertannad O resets load local #5019 in rooms #78581–#78584; one Troll Caves O reset loads #5041 in room #96981. These are the selected foreign reset references already classified by the dossier. See [Caertannad resets](../../../areas/zon/caertannad.zon#L409) and [Troll Caves reset](../../../areas/zon/troll_caves.zon#L131). |

The exact typed scan found no external SHP product and no external mobile or
object prototype definition. It parsed 231 other story sidecars without errors;
the only exact structured item_vnums hits are the three Tower records above.
The type-25 scan covered 872 declarations in other object files (713 active and
159 source-only) and found no destination into Labyrinth.

A context-reviewed C/C++ scan found these additional references:

| Local target | Static reference | What the source establishes |
| --- | --- | --- |
| Objects #5048, #5071, and #5069 | [chaos_eq_data.h #159](../../../src/account/chaos_eq_data.h#L159), [#408](../../../src/account/chaos_eq_data.h#L408), [#260](../../../src/account/chaos_eq_data.h#L260), [#504](../../../src/account/chaos_eq_data.h#L504), [#562](../../../src/account/chaos_eq_data.h#L562) | Static Necromancer, Illusionist, and Dragoon equipment profiles name these local objects. The profiles do not establish kit selection, delivery, or issuance. |
| Room #5065 | [specs.mobile.c](../../../src/specs/specs.mobile.c#L925) and [prototype declaration](../../../src/world/specs.prototypes.h#L289) | brass_dragon checks this room and blocks WEST. The scan found its declaration and function definition, but no active binding; this raw code reference is not an active local handler. |

The remaining raw numeric matches are constants, decimal values, comments, or
namespace collisions. Room, mobile, and object VNUMs were typed by source
context and are not treated as interchangeable.

## Title, history, and disposition

The color-stripped title in [labyrinth.zon](../../../areas/zon/labyrinth.zon#L2)
normalizes to “Labyrinth of No Return” and is unique among 351 active
contributors. The WLD history records the 2014 entrance/exit edits, including
removal of a prior up exit whose target did not exist; the ZON history includes
the 2015 quest-item-flag pass and earlier source edits. The followed WLD edits
include 5bae9f0c3 (2014-09-03) and 80a7e3c19 (2014-08-03). No distinct
predecessor or replacement appears in the curated
[replacement register](DUPLICATE_AND_REPLACED_ZONES.md). The active,
un-commented [areas/AREA row](../../../areas/AREA#L26) remains **DO NOT MOVE**.

The existing dossier covers the local quest, maze routes, reset behavior,
shared controls, and selected foreign ownership. This repository-wide scan adds
the active Mountain Tracks pair and classifies inactive-WLD, typed, and
compiled references. Tooling/editor closure, dynamic route availability,
door/reset state, safe travel, and played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Builder questions and recommended resolutions

The source-comprehensive dossier already records three independent requests,
their exact accepted items, room controls, foreign ownership, and a separate
epic-node maintenance candidate. These direct questions state the remaining
builder choices without converting authored text, source declarations, or raw
code matches into proven gameplay.

| Question | Source evidence | Recommended resolution |
| --- | --- | --- |
| Does No Return describe the labyrinth's mood, or is it meant to promise that a player cannot leave? | The active WLD graph is one weak component and has a reciprocal boundary with Mountain Tracks; the dossier separately records reset-sensitive doors, controls, and an unreliable map route ([graph](#active-wld-graph), [map](../zone-stories/LABYRINTH_OF_NO_RETURN.md#the-lost-adventurers-map)). | Keep the authored title. Describe only source-defined paths and door states; do not infer captivity, escape, or safe return from the title or weak connectivity alone. |
| Is the adventurer's three-steps-north map route intentionally unreliable, or should its directions be corrected? | The map says to begin at the entrance and go north three times, but the active graph reaches room #5019 after two north moves and has no third north exit ([dossier](../zone-stories/LABYRINTH_OF_NO_RETURN.md#the-lost-adventurers-map)). | Decide whether the wrong direction is intentional lore or an outdated clue before editing. If correcting it, validate the whole approved route in a separate map change; do not silently invent a route in the audit. |
| Does the map-for-robe acceptance prove that the adventurer returned home or escaped? | QA24 accepts map #5014 for robe #5073 and removes the adventurer, but the source has no persisted arrival or player-escape outcome ([dossier](../zone-stories/LABYRINTH_OF_NO_RETURN.md#the-lost-adventurers-map)). | Keep the item receipt and recipient departure separate from any homecoming claim. A lasting return needs an owner-defined and committed subject outcome. |
| Should the adventurer, Golden Flame, and Vadatorn requests form one required campaign? | QA24, Q85, and Q117 are independent local returns; the farewell mentions the other characters without a native prerequisite between them ([map](../zone-stories/LABYRINTH_OF_NO_RETURN.md#the-lost-adventurers-map), [proofs](../zone-stories/LABYRINTH_OF_NO_RETURN.md#nine-proofs-for-the-golden-flame), [parts](../zone-stories/LABYRINTH_OF_NO_RETURN.md#vadatorns-three-minotaur-parts)). | Keep the three receipts independent unless a builder defines an optional campaign's stages and ALL/ANY policy. Do not infer a map-first gate or shared completion. |
| Should the Knight require exactly one each of nine different proof kinds, including all four moss colors? | Q85 lists nine exact IDs; repeated red moss or duplicate shells do not cover missing kinds ([proof table](../zone-stories/LABYRINTH_OF_NO_RETURN.md#nine-proofs-for-the-golden-flame)). | Preserve the nine exact kinds and one-of-each bundle. Do not collapse them into nine generic items or count extra copies as additional completion. |
| Should proof acceptance require a player's first recovery, a personal kill, or the source actor named in the prose? | Exact loose supplied items fit; source declarations include equipment, mobile gifts, and nested web contents with mixed custody states ([proof sources](../zone-stories/LABYRINTH_OF_NO_RETURN.md#nine-proofs-for-the-golden-flame)). | Keep readiness of matching loose goods separate from source history and personal credit. Require a specific provenance contract only if the builder chooses that behavior. |
| Should moss given by a follower in the F reset chain count as party or player proof? | The F command changes the equipment parent to that follower; the dossier records the exact parent/source and notes that it does not declare player-group credit ([follower sources](../zone-stories/LABYRINTH_OF_NO_RETURN.md#nine-proofs-for-the-golden-flame)). | Preserve the reset parent and spawned source identity. Do not treat follower ownership or shared caps as evidence that a player earned or issued the moss. |
| What should the five room controls prove, and should an already opened shared passage count as each player's solution? | Three levers use PULL, stone #5046 uses PUSH, and non-takeable moss #5047 uses GET through switch handlers; they alter specific barriers ([controls](../zone-stories/LABYRINTH_OF_NO_RETURN.md#controls-doors-and-connected-requests)). | Keep each exact source, command, target, and barrier transition distinct. Record personal credit only from a selected actor's admitted activation, not from hearing the story or finding a door already open. |
| Is diamondine key #5057 meant to be personally required for all of its doors, including the asymmetric entrance? | The key opens local pairs at #5134/#5135 and #5295/#5000; the #5000 west entrance is secret/locked with key zero while the reverse names #5057 ([door analysis](../zone-stories/LABYRINTH_OF_NO_RETURN.md#controls-doors-and-connected-requests)). | Preserve the asymmetry and source key values until the owner defines intended access. Qualify each direction and reverse unlock separately; do not automatically clear the secret entrance or require a personal key recovery. |
| Does Golden Flame sigil #5031 actually guide or protect its holder during travel? | The object has protective equipment flags, but no local route controller or navigational effect is assigned ([sigil analysis](../zone-stories/LABYRINTH_OF_NO_RETURN.md#nine-proofs-for-the-golden-flame)). | Keep equipment data separate from an active travel or safety promise. Confirm the wearer, effect, and route before describing guidance or protection as delivered. |
| Does Vadatorn's accepted trio persistently cure his ward or free his people? | Q117 consumes the exact tail, heart, and eye, rewards the quiver and crystal ball, then removes Vadatorn; cure and liberation are narrated ([parts](../zone-stories/LABYRINTH_OF_NO_RETURN.md#vadatorns-three-minotaur-parts)). | Keep the exchange and disappearance as the declared receipt. Record a cured wizard, changed ward, or freed people only after an owner defines a persistent outcome. |
| Is Tower's Smedgewack return part of Labyrinth completion, or a separate Tower-owned receipt? | Tower QST accepts Labyrinth calcite #5011, flask #5016, and tattoo #5035 for its own reward; the current typed audit maps it to Tower ([typed scan](#typed-references-and-compiled-numeric-matches)). | Keep the exact foreign recipient and receipt under Tower ownership. Treat the three local items as Tower inputs, not a fourth Labyrinth achievement. |
| Is imported epic node #359 a Labyrinth quest return, and which committed outcome should its payout represent? | Heavens supplies the epic node and bound handler; eligible TOUCH, frozen participants, and committed zone-touch settlement differ from the three local QST returns ([node analysis](../zone-stories/LABYRINTH_OF_NO_RETURN.md#imported-node-and-maintenance-candidate)). | Keep node claims under the epic handler's owner and separate from Labyrinth Q credit. Use a committed claim and its selected beneficiaries for any persisted milestone; touch text or hum is not a settled award. |
| Should accepted map, proof, and key sources be considered available solely because the ZON declares reset stock? | The dossier's accounting review says active accounting refuses fresh O/P/E/G issuance, while reset caps and lifespan describe source declarations ([accounting](../zone-stories/LABYRINTH_OF_NO_RETURN.md#accounting-and-qualification)). | Do not promise live stock from reset declarations alone. Define admitted generation, current owner, renewal, refusal, and recovery before treating a proof as available. |
| Should epic-stone absorption preserve later root items when one extracted object is followed by another? | The existing maintenance analysis identifies a candidate: extraction clears the retired object's next_content link before the loop advances; no played crash is claimed ([candidate analysis](../zone-stories/LABYRINTH_OF_NO_RETURN.md#imported-node-and-maintenance-candidate)). | Keep this as a separate maintenance proposal. If approved, preserve the successor before extraction and qualify multiple ground/carried roots and pool reuse with a reproducible before/after check before any source repair. |
| Are memory #55184 and treasure #67233 intended to be inert local props, or should either own a handler or story outcome? | The local records have trap or treasure metadata but no local assigned quest or handler ([imported content review](../zone-stories/LABYRINTH_OF_NO_RETURN.md#imported-node-and-maintenance-candidate)). | Leave them outside quest accounting until an owner defines an interaction, admitted actor, result, and recovery. Metadata alone should not create a quest or completion. |
| Is the unregistered labyrint.qst file retained as source history, or does its duplicated branch have an intended active owner? | It is absent from both loader lists and repeats the active #5028 Knight branch's ten typed item commands and 50,000-experience amount; the active story sidecar records the same branch ([inactive commands](../../../areas/qst/labyrint.qst#L30), [active branch](../../../areas/qst/labyrinth.qst#L93), [active sidecar](../../../areas/story/labyrinth.story.json#L155), [Checkpoint 907](#checkpoint-907-labyrint-qst-branch-retains-active-labyrinth-item-roles)). | Keep the old file source-only unless its owner defines a distinct use. Preserve its duplicate authored text without counting it as a second active consumer or quest. |
| Should the brass_dragon room check at #5065 be bound to an active Labyrinth mobile? | A function and prototype declaration check #5065, but the scan found no active binding ([code review](#typed-references-and-compiled-numeric-matches)). | Treat the match as raw code, not current behavior. Add a binding only with a route owner's decision about actor, direction, and expected passage result. |

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad has four object reset rows for Labyrinth of No Return object #5019. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).

## Checkpoint 907: labyrint QST branch retains active Labyrinth item roles

The all-source role refresh confirms that `labyrint.qst` remains a single
unlisted QST source beside the active `labyrinth.qst`. Its giver header #5028
resolves to active Labyrinth MOB Sir Uthor. Its ten item commands
(#5031; #5001, #5002, #5003, #5005–#5009; and #5020) resolve to active
Labyrinth OBJ prototypes and duplicate the active Knight branch's exact
command IDs; `R E 50000` is an experience amount. The active
`labyrinth.story.json` records the same giver, item set, and completion key.
These authored copies do not constitute two active quest branches: both area
lists and quest runtime use only the active spelling.

The same-number scan also keeps namespaces distinct. Active Labyrinth owns
WLD rooms and MOB/OBJ prototypes with some overlapping numbers. External
Tower QST/story references to #5011, #5016, and #5035, plus the Caer Tannad
#5019 and Troll Caves #5041 resets, remain separate documented consumers of
active Labyrinth objects; they are not the ten-item `labyrint` command set.
Other numeric matches in unrelated object payloads, mobile damage expressions,
and code comments/counters did not establish typed VNUM roles. No external
SHP product, external sidecar item role for the candidate's command set, or
compiled C/C++ lookup into its giver/item VNUMs was found. The candidate has
no WLD, so it adds no map, route, or connectivity count. CP85's active
Labyrinth graph, Tower cross-area returns, and history conclusion remain
unchanged. Keep `labyrint.qst` **RETAIN IN PLACE** outside both area lists
until its author and quest owner resolve provenance and persisted identity.
