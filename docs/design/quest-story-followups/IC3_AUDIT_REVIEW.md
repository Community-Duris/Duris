# Dreadnaught Cyclops (IC3) inactive-source audit

Canonical key: `component:IC3`. Physical coverage row 403 / CSV line 404;
candidate dependency row 53 / CSV line 53. Reviewed October 13, 2026,
Checkpoint 902 (baseline source audit Checkpoint 490), against source snapshot
`72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a static source,
connectivity, numeric-identity, typed-reference, story, and bounded-history
review. It does not establish runtime loading, gameplay, saved-world ownership,
editor ownership, or permission to reactivate, rename, move, or archive the
files.

## Source inventory and current identity

The unlisted component has uppercase
[WLD](../../../areas/wld/IC3.wld#L1),
[MOB](../../../areas/mob/IC3.mob#L1),
[OBJ](../../../areas/obj/IC3.obj#L1), and
[ZON](../../../areas/zon/IC3.zon#L1) files. No same-stem QST, SHP, or story
sidecar is present. The ZON header is #4200, titled “The Wreck of the
Dreadnaught Cyclops,” with top #420053 and reset mode 0. The WLD contains 54
rooms (#420000–#420053), the MOB 21 prototypes (#420000–#420020), and the OBJ
50 prototypes (#420000–#420049). The candidate has no raw AREA annotation and
does not appear in [areas/AREA](../../../areas/AREA) or
[areas/qst/AREA](../../../areas/qst/AREA).

The source room, mobile, and object VNUM ranges have no exact same-type record
overlap with current active AREA files. The inactive high-numbered identities
therefore do not collide with current active room, mobile, or object records
in the scanned lists. This finding does not resolve saved characters, editor
copies, external data, or any previously persisted references.

On April 6, 2026, commit `ef7e3f9b17f492a946425657f1bce8e91e241506`
removed the `IC3 *4200-4300` entry from `areas/AREA` and changed that range to
`*OPEN`; it did not delete the four source files. On April 7, commit
`180978e3a5662eb113f6ece64c62aba766206ee0` removed the active approach in
`jademini.wld`, with the commit message “Adding epic stones to grumbar and crl2,
and removing entrance to ic3.” Those manifest and route changes establish
de-listing and an entrance removal, not authorization to delete or relocate
the retained source.

## Candidate graph and active numeric matches

The IC3 WLD has 112 exit records. One hundred eleven target candidate-local
rooms in #420000–#420053; the other is room #420000's D0 north exit to active
Rice Fields room #77262. The candidate's local graph is one weak component,
and a directed traversal from #420000 reaches all 54 rooms. There are no
negative targets, range-mismatch rooms, fall-chance fields, or current fields.
Because IC3 is not loaded, only the outbound #77262 target resolves through the
current active room table; the 111 local edges remain source-only references.

No current active cross-area WLD exit enters #420000–#420053. The earlier
Rice Fields approach was the reciprocal active south exit from #77262 to
#420000; the April 7 edit now sets that D2 destination to `-1`. The current
active #77262 record has no return to IC3. Preserve the source-only IC3 exit
and active Rice Fields room as separate facts: the former is not current
ingress, and the candidate is not a loaded map. No reverse or reciprocal
active pair remains.

## Resets, doors, portals, and typed dependencies

The candidate ZON has 210 reset commands: 42 M mobile loads, 40 F follower
loads, 20 O room-object loads, 21 P container loads, 79 E equipment loads,
and eight G inventory loads. It also has ten D door-state commands. All 21
candidate mobile VNUMs occur in M or F resets. Object reset arguments include
all 50 candidate OBJ prototypes and one external active prototype, Heavens
object #360. Every room destination named by the candidate reset commands is
inside IC3's room range. The #420018 object is a “Never Winter” steel key but
is not referenced by a ZON reset or by any of the five WLD door-key fields.

The external reset is ZON line 67, `O 0 360 1 420050`. Active
[Heavens OBJ #360](../../../areas/obj/heavens.obj#L3741) is a rune-covered
monolith. This is a typed source dependency from inactive IC3 ZON to a current
active Heavens OBJ definition; it is not an active IC3 reset while the zone is
unlisted.

Five WLD door-key fields use candidate-local objects: #420009 uses #420019;
#420018 and #420019 use #420001; #420032 uses #420046; and #420036 uses
#420030. The associated room exits and the ten reset door states should be
reviewed together. The source does not use OBJ #420018 for any of these locks.

Two type-25 portal objects, #420048 “rope ladder teleport” and #420049
“shimmering portal,” each store #420022 as their destination VNUM. The ZON
places the rope-ladder object at #420000 and #420022, and the shimmering portal
at #420033. Git history from January 15, 2026 describes a one-way main
entrance, a ladder teleport to #420022, and a portal near #420033 to the
entrance; the current authored object values point both portals to #420022.
Record that discrepancy for the area owner rather than inferring runtime portal
behavior from prose or reset placement.

A bounded typed-reference scan found no current active WLD inbound edge, active
ZON or SHP consumer, QST reference, or explicit `src/` VNUM binding into the
IC3 ranges. The raw literal `420000` in Caertannad quest/story data is a
420,000-copper reward, not a VNUM reference. A numeric tuple in
`areas/mob/nexus.mob` also contains `420000`, but no IC3-typed dependency was
established from that unrelated record; it remains an unclassified raw-number
match rather than evidence of active IC3 use.

## Narrative, history, and disposition

The map is story-bearing despite having no QST or sidecar: its room prose
describes a wrecked vessel, a Poseidon curse, the gunnery deck, captain's
quarters, and a treasury shipment. The ZON title calls it the Dreadnaught
Cyclops. No reviewed QST implementation or active route was found to continue
those story threads.

The WLD, MOB, OBJ, and ZON entered the repository in November 2025 and received
content or balance changes through January 2026. The OBJ file was edited again
on September 14, 2026, after the active-list removal. Later source maintenance
does not restore loader membership, but it is a reason to confirm current
authoring and deployment ownership before any path or archive decision. The
bounded history review found no whole-area replacement record that supersedes
this IC3 source.

**Disposition: RETAIN IN PLACE.** Keep all four files unlisted and at their
current paths while owners resolve the source-only Rice Fields exit, removed
active entrance, Heavens #360 reset dependency, door and portal intent, source
editing and deployment copies, and any saved identity state from the period
when IC3 was listed. Do not infer from its de-listing that its high VNUMs are
free, that its retained outbound exit is usable, or that it is safe to archive.
See the [24 builder questions](IC3_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-490-ic3-inactive-source-graph),
[source identity and history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-490-ic3-source-identity-and-history),
[candidate dependency row 53](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L53), and
[coverage row 403](ZONE_COVERAGE_REGISTER.csv#L404).

## Checkpoint 902: typed cross-source role refresh

The Checkpoint 490 graph, story inventory, and **RETAIN IN PLACE** decision
remain unchanged. The all-source typed sweep confirms that IC3's high VNUMs
do not acquire current outside references through reset, quest, shop,
teleport/switch, or direct compiled lookup fields.

No outside WLD destination enters rooms #420000–#420053; active ingress remains
zero. The one candidate outbound remains IC3 #420000 D0 north to active Rice
Fields #77262, with no active return after the April 7, 2026 `-1` edit. That
source-only exit is already included in Checkpoint 174's curated source-only
total (3,425 overall, 192 Surf, 15 `ud_map2`), so this refresh adds no edge or
recount item. An exact same-type scan across all other WLD, MOB, and OBJ files
found no definition in the IC3 room, mobile, or object ranges.

Across external ZON sources, no typed room, mobile, or object reset operand
matches IC3. The candidate's own ZON still has its one external item import,
Heavens OBJ #360 at [IC3 ZON line 67](../../../areas/zon/IC3.zon#L67); this is
an outgoing source dependency, not an external reference into IC3. No external
type-25 teleport `value[0]` or type-29 switch `value[1]` targets an IC3 room.
Both candidate type-25 objects, #420048 and #420049, store local destination
#420022 ([#420048 value](../../../areas/obj/IC3.obj#L522),
[#420049 value](../../../areas/obj/IC3.obj#L530)); their reset
placement and discrepancy with January history remain open per I-15/I-16.

No outside QST giver key or typed `G I`/`R I` item command matches the IC3
MOB/OBJ ranges. The raw-number sweep found only two outside uses of 420000:
Caertannad QST `R C 420000` is a copper reward
([QST line 859](../../../areas/qst/caertannad.qst#L859)), and its sidecar's
`receive=C:420000` fields describe that same currency reward
([story line 1336](../../../areas/story/caertannad.story.json#L1336)); neither
is an IC3 reference. A separate raw `420000` in `nexus.mob`
([line 1949](../../../areas/mob/nexus.mob#L1949)) remains unclassified because
the row is not a typed IC3 lookup. Keep I-21 open for the MOB-format owner to
classify that operand.

The all-source shop scan found no IC3-range SHP number, and the direct
`real_room`, `real_mobile`, and `real_object` call scan found zero IC3 VNUM
matches in `src/`. These negative source results do not resolve editor or
deployment copies, dynamic lookup paths, prior persisted state, reset-mode 0,
the unused #420018 key, the active Heavens #360 import, or portal/story owner
intent. Keep all four files **RETAIN IN PLACE** and preserve the CP490 owner
questions before any activation, merge, archive, or path proposal. See the
[updated 24 questions](IC3_BUILDER_QUESTIONS.md),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-902-ic3-has-no-new-external-ingress),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-902-ic3-remains-a-distinct-high-vnum-source),
[retention note](ATTIC_MIGRATION_PLAN.md#checkpoint-902-ic3-retains-its-source-only-route-and-typed-boundary),
[candidate dependency row 53](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L53), and
[coverage row 403](ZONE_COVERAGE_REGISTER.csv#L404).
