# Winterhaven bounded source audit

Canonical key: `area:wh`. Physical coverage row 345. This checkpoint
supplements the [source-comprehensive Winterhaven dossier](../zone-stories/WINTERHAVEN.md)
and its [reproducible source index](../../reference/zone-story-audits/wh.md).
The dossier remains the detailed account of quest contracts, room prose,
mobile/object behavior, rewards, custom handlers, and story revision 2.

## Checkpoint 432: source boundary and prior dossier

The existing dossier covers all 599 rooms, 314 mobiles, 483 objects, thirteen
shops, 1,284 reset commands, 221 native Q/QA contracts, and the 219-entry story
sidecar. Those 221 contracts comprise 135 requests, 84 services, and two
exclusions. It is source-comprehensive; the dossier explicitly leaves installed
availability, player discovery, live payment, reset recurrence, and travel
qualification open. This supplement adds bounded cross-zone and identity scans
and does not replace those qualifications.

## Active WLD graph, directed reach, and approaches

The active `wh.wld` has 599 rooms and 1,558 resolving exits: 1,498 local and 60
outbound to 51 other area stems. Five active exits enter Winterhaven from
Heavens, Nlakes, and Surface. Four of those edges have a reverse Winterhaven
edge; Heavens #41 east to Winterhaven #55160 is one-way. See [Heavens row
15](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L15), [Nlakes row
870](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L870), and [Surface rows
1395-1397](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1395).
Winterhaven's 60 outbound exits are indexed in the same register, including
the earlier recorded one-way routes. There are seven weak components; the
largest contains 585 rooms, with component sizes 5, 4, 2, 1, 1, and 1 for the
remaining components.

Treating every open WLD edge as traversable, the union of the five active
arrival rooms reaches 553 of 599 local rooms. From 587 rooms there is a directed
local path to at least one outbound edge; twelve have no such path. These are
map-graph results, not claims of ordinary player access or safe return. Doors,
keys, the hidden and teleport routes, special procedures, and changing state
remain governed by the source details in the [Winterhaven dossier](../zone-stories/WINTERHAVEN.md#access-availability-and-custom-dependencies).
The inactive-WLD sweep found no source-only approach or overlapping room
identity for Winterhaven; no new candidate record is warranted.

## Typed dependencies and exact source closure

An exact-role scan of Winterhaven's local ZON object commands finds 29 distinct
object VNUMs without a local `wh.obj` definition. Twenty have a sole definition
in `heavens.obj` (#252, #338, #361, #363, #368, #369, #385, #386, #393, #421,
#500, and #820-#828). Six more resolve uniquely to Breale #2656, Quietus #1715,
Verz1 #3097, Bandit Camp #14800, BS #74210, and Fisherman's Wharf #88903. Object
#53 has definitions in Dalvik, Heavens, Hostel, and Mavulsk, and #100 in Heavens
and Mavulsk, so their imported identity is ambiguous. Object #6067 has no
definition in the current `areas/obj` tree. Preserve those reset records and
owner ambiguities; do not infer which duplicate the loader selects or that the
unresolved reset produces an object.

The bounded external typed scan also finds 114 active ZON reset references to
Winterhaven mobiles or objects across 74 active area files: 29 M-mobile, 64
G-object, 19 O-object, and two P-object records. Active QST files contain 463
exact item-command references across 17 files. Six active SHP product records
occur across six files. Nine active story sidecars contain 331 structured
Winterhaven VNUM references (301 item, 18 giver, and 12 mobile keys). These are
source references, not proof that each reset, shop, quest, or sidecar path is
currently usable.

Unregistered `brass-old-1` and `brass-old-3` sources also refer to Winterhaven
items; exact lines and candidate-specific preservation notes are recorded on
[attic candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17) and
[row 19](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L19).
Those records remain inactive source evidence. Winterhaven's existing outgoing
dependencies also remain: its QST line 2626 references Dungeon object #93011,
and the dossier documents its separate Astral Tiamat cloak/story relationship.
Earlier audits identify active reset imports from Nyneth3, Orrak, Keep, New
Cave, Astral Main, and Swamp Two; preserve their exact object-type ownership.

A raw C/C++ numeric scan produces 188 matching lines across 14 files. The
existing dossier separately identifies 92 literal special-assignment
candidates and manual macro-resolution limits. Numeric matches are leads, not
semantic assignments. An active type-25 portal scan found no target into
Winterhaven among 865 records. Code indirection, editor/tooling consumers,
persisted player items, loader choice for duplicate definitions, and runtime
availability remain open; keep the typed-dependency status PENDING.

## Title, followed history, and owner boundary

After color-code removal, the active ZON title “The City of Winterhaven” is
unique among current active titles. Followed WLD, MOB, OBJ, ZON, QST, and SHP
history reaches the August 2009 areas-tree move and remains on the `wh` stem.
The story sidecar has two same-stem revisions dated October 2, 2026. The
bounded current-source and followed-history review found no distinct
alternate-stem predecessor or whole-area replacement. This does not equate
Winterhaven's neighboring routes, foreign consumers, or same-numbered
cross-type prototypes with common ownership. Command-target alias disambiguation
remains separate in coverage column 37.

## Safe disposition

Keep active `wh` **DO NOT MOVE**. It is a current full-area contributor with
many native stories, custom behavior, inbound reset consumers, foreign QST and
SHP references, and external WLD approaches. Preserve the map's one-way edges
and unresolved directed reach as authored evidence; do not add guessed exits.
Do not create a Winterhaven attic candidate. Keep the existing Brass candidates
and their Winterhaven references under their current dispositions until each
candidate's complete source, history, typed-reference, and tooling closure is
reviewed. No source-area or runtime file was changed for this checkpoint.

## Checkpoint 776: active Brass consumers

Active Brass resets Winterhaven object #55150 with G at [brass.zon line 820](../../../areas/zon/brass.zon#L820), and the Tila Banto bakery record lists the same object as a product at [brass.shp line 583](../../../areas/shp/brass.shp#L583). The local prototype is a cup of ice cream. Preserve Winterhaven as the object owner and retain the Brass references as active source declarations; neither row proves current reset admission or operating shop stock. See the [Brass typed-dependency audit](BRASS_AUDIT_REVIEW.md).

## Checkpoint 788: Domain skull and sheath references

Active Domain Q47 consumes Winterhaven skull #55037 three times and returns Winterhaven
sheath #55038 ([QST](../../../areas/qst/dlsc.qst#L52),
[skull](../../../areas/obj/wh.obj#L396), [sheath](../../../areas/obj/wh.obj#L404)). The
current Domain story sidecar has a typed item_vnums reference to #55037 and repeats the
exact Q47 completion key in two contract/step records. Preserve Winterhaven as the owner
of both objects; the declarations do not establish live supply. See the [Domain typed
audit](DLSC_AUDIT_REVIEW.md#checkpoint-788-post-sidecar-typed-closure-refresh).

## Checkpoint 798: Charcoal Palace reward reference

Firep's local QST awards Winterhaven object #55270, broken rays of morning
Sunrise, and its story-sidecar receipt repeats that output ([QST](../../../areas/qst/firep.qst#L51),
[sidecar](../../../areas/story/firep.story.json#L105),
[object](../../../areas/obj/wh.obj#L3360)). Preserve Winterhaven as the current
object owner; the reward reference does not establish supply or successful
settlement. See the
[Charcoal Palace typed audit](FIREP_AUDIT_REVIEW.md#checkpoint-798-current-typed-census-refresh).

## Checkpoint 800: Wharf fishing-pole product

Winterhaven's active shop source lists Fisherman's Wharf fishing pole #88903
as a product at [wh.shp line 452](../../../areas/shp/wh.shp#L452).
Fisherman's Wharf remains the pole's prototype owner; this listing does not
establish live stock. See the
[Wharf typed audit](FISHERMANS_WHARF_AUDIT_REVIEW.md#checkpoint-800-current-typed-census-refresh).
