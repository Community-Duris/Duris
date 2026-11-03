# Drakenwood Hostel inactive-source audit

Canonical key: component:hostel. Physical coverage row 402 / CSV line 403;
candidate dependency row 52 / CSV line 52. Reviewed October 13, 2026,
Checkpoint 901 (baseline inventory Checkpoint 489), against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This is a static source, graph,
numeric-identity, story-reference, and bounded-history review. It does not
establish runtime loading, gameplay, editor ownership, persisted world state,
or permission to activate, merge, rename, or remove these files.

## Source inventory and current identity

The unlisted component has [WLD](../../../areas/wld/hostel.wld#L1),
[MOB](../../../areas/mob/hostel.mob#L1),
[OBJ](../../../areas/obj/hostel.obj#L1),
[ZON](../../../areas/zon/hostel.zon#L1), and
[QST](../../../areas/qst/hostel.qst#L1) files. It has no same-stem SHP file or
story sidecar. The ZON header is #1, titled “The Drakenwood Hostel,” with
top #102 and reset mode 2. The WLD has 102 rooms, #1–#102; the MOB has 66
prototypes, #1–#66; the OBJ has 76 prototypes, #1–#76; and the QST has four
giver records keyed #4, #15, #45, and #46.

Neither [areas/AREA](../../../areas/AREA) nor
[areas/qst/AREA](../../../areas/qst/AREA) lists hostel. The candidate AREA
annotation is blank. The source itself uses “Drakenwood” in the ZON title,
“Drakkenwood” in room prose, while active Savannah text says “Hostel of
Drakken Wood” and its sidecar says “Drakenwood Hostel.” These spellings and
references establish a likely narrative association, not a confirmed canonical
area alias.

The low numeric ranges overlap current loaded sources:

| Source type | Candidate hostel | Matching active AREA records | Comparison |
| --- | --- | --- | --- |
| WLD | 102 rooms, #1–#102 | 45 IDs in limbo.wld and heavens.wld | All 45 same-number room records differ; the other 57 candidate IDs have no current active room record. |
| MOB | 66 prototypes, #1–#66 | 65 IDs in limbo.mob and heavens.mob | All 65 same-number mobile blocks differ; candidate #18 has no active same-number MOB record. |
| OBJ | 76 prototypes, #1–#76 | All 76 IDs in limbo.obj and heavens.obj | All 76 same-number object blocks differ. Candidate #41 is a twisted metal key while active Heavens #41 is a guild badge; candidate #72 is dragon-scale leg armor while active Heavens #72 is a magical fountain. |
| QST | Four giver records: #4, #15, #45, #46 | Only #46 appears in active qst/AREA files | Active heavens.qst #46 is an empty S record; candidate #46 contains a quest. |

Existing [Torrhan](../zone-stories/CITY_OF_TORRHAN.md#L225) and
[Hall of Knighthood](../zone-stories/HALL_OF_KNIGHTHOOD.md#L28) reviews already
cite the unlisted Hostel #41 and #72 objects as competing same-number records
whose current active consumers use Heavens definitions. Preserve those current
consumers and identities while Hostel remains unlisted.

## Candidate graph and active numeric matches

The candidate WLD has 242 exit records across directions D0–D9. All 242
targets are positive, within the candidate’s #1–#102 room set, and there are
no candidate exits to another area or negative destinations. Its local graph
has five weak components of sizes 78, 17, 4, 2, and 1. Traversal from candidate
#1 reaches 78 rooms; 24 rooms lie outside that directed traversal. The source
has no WLD fall-chance or current fields.

Only 91 of the 242 target-number references have a matching room in the
current active AREA files. The other 151 point to candidate-local room numbers
that have no current active room record. Thus the self-contained source map
does not resolve as a self-contained map through today’s active room table.

Ten active cross-area exits also target numbers in the candidate range. Nine
directions from active Desert #49790 target #1, and active Thetis #38075 south
targets #38 ([Desert register entries](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L630),
[Thetis entry](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L542)). Those numbers resolve
to active Limbo #1 and active Heavens #38, respectively; they do not enter the
unlisted Hostel rooms. The active inbound edges therefore represent numeric
matches, not current Hostel ingress. No outbound candidate edge or reciprocal
cross-area pair was found.

## Resets, quests, and the Savannah reference

The candidate ZON contains 230 reset commands: 122 M mobile loads, 26 F
follower loads, 16 O room objects, five P container loads, 25 E equipment
loads, and 36 G inventory loads. It also has 36 D door-state commands. Across
M and F commands, all 66 candidate mobile IDs are referenced; 13 occur only
as F followers. Sixty-seven distinct candidate object IDs occur in object
resets. Nine OBJ records (#8, #22, #45, #47, #55, #66, #74, #75, and #76) are not placed
by ZON; some remain QST-related. Every reset mobile, object, and room
reference resolves to a same-stem candidate record.

The four candidate QST records describe different story threads:

- #4 asks for six demon-summoning ingredients and narrates the creation of a
  demon blade.
- #15 concerns a Grey Elf ranger, a missing councilor, proof of his fate, and
  a buckle.
- #45 asks the player to return a malicious weapon to an agent of Athos.
- #46 is a dying robed man who names Athos and participates in the weapon
  thread.

All four giver mobiles are referenced by candidate resets. No candidate QST
text describes the six epic instruments requested by active Savannah.

Active [Savannah QST](../../../areas/qst/savannah.qst#L12) says Lynstar’s sister
at the Hostel wants all six epic instruments, and the active
[Savannah sidecar](../../../areas/story/savannah.story.json#L19) records that
full-set lead as unfinished with no supported journal ending. The existing
[Savannah story review](../zone-stories/SAVANNAH_OF_BROKEN_TRUSTS.md#L197)
checked the retained Hostel QST, relevant prototype and assignment references,
and the authoritative area list; it found no six-instrument consumer or
reviewed Hostel sister. Treat the Savannah line as a current unresolved
narrative dependency, not proof that the retained Hostel files implement it
or that an older version never did.

## History and disposition

Followed file history stays on the Hostel stem and begins with the 2012
“Adding Octavaz’s files to tracking” import. Later tracked edits include a
2015 ZON quest-item flag pass, a 2016 OBJ text cleanup, and a 2026 OBJ player
gear stat change. No Hostel-to-other-stem rename or explicit whole-area
replacement was found in the bounded history. Later edits to one source file
do not prove that the area is in the active loader or identify its editor and
deployment owners.

Keep candidate row 52, hostel, **RETAIN IN PLACE**. Preserve the five source
files and the Savannah textual lead. Resolve the area name, all same-number
WLD/MOB/OBJ identities, QST giver #46, the disconnected graph, current
numeric inbound owners, Savannah’s six-instrument continuation, old source
owners, and editor/deployment/persisted-ID state before any activation, merge,
rename, archive, or path decision. See the
[24 builder questions](HOSTEL_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-489-drakenwood-hostel-inactive-source-graph),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-489-hostel-source-identities-and-history),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-489-hostel-retained-pending-story-and-low-vnum-ownership),
[candidate dependency row 52](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L52), and
[coverage row 402](ZONE_COVERAGE_REGISTER.csv#L403).

## Checkpoint 901: typed cross-source closure

The role-aware source scan does not change the CP489 map counts, active
inbound count, or **RETAIN IN PLACE** disposition. Numeric matches in other
source files were resolved by field role and source identity before treating
them as possible Hostel dependencies.

### World and key fields

The all-source WLD scan found one non-source-local outside record whose target
number is in the Hostel range: unregistered `surf.wld` room #201670 D5 points to #36
([source record](../../../areas/wld/surf.wld#L35061)). The current active room
#36 belongs to Heavens, not Hostel. This edge was already included in the
Checkpoint 448 source-only recount (11,987 total / 415 from `surf.wld`), so it
does not add active Hostel ingress or change that aggregate. Other raw target
number matches in unregistered maps are local to those maps; no external WLD
record targets a room that is unique to the retained Hostel source.

Hostel WLD contains 25 positive key fields. Every key VNUM names an OBJ in
`hostel.obj`, and each also has a current active Limbo or Heavens OBJ owner.
Outside Hostel, 25 WLD key fields reuse the same numeric object range: 16 in
unregistered Mavulsk resolve to Mavulsk objects, two in unregistered
Sylvandawn include #1 resolving to active Limbo, and seven in active Mistywood,
Moss, Mush1, and Split Shield resolve to current Limbo/Heavens object owners.
These are lock-key operands, not room destinations. Hostel's 76 OBJ records
are item types 5, 9, 10, 11, 13, 15, 34, and 37; none is a type-25 teleport or
type-29 switch.

### Typed resets, quests, and shops

Role-aware ZON parsing found candidate-range room, mobile, and object values,
but no outside typed reset depends on a Hostel-only room (#46–#102) or its
mobile #18, which has no current active mobile owner. Matching fields either
belong to their own source-local room/mobile/object identity or resolve to an
active Limbo/Heavens owner. Mavulsk's mobile reset and QST giver #18, for
example, resolve to Mavulsk's own [mobile](../../../areas/mob/mavulsk.mob#L282)
and [QST](../../../areas/qst/mavulsk.qst#L1) records, not Hostel.

Across other QST files, 60 typed `G I`/`R I` item commands reuse Hostel OBJ
numbers: 36 from active QST sources (35 active-owner roles and one
source-local role) and 24 from unregistered QST sources, all Mavulsk-local.
None is an unresolved import into Hostel. The only outside QST giver header
match is active Heavens #46, whose record is empty `S`; no active QST giver
matches Hostel #4, #15, or #45. Savannah's six-instrument sister lead remains
without a Hostel QST consumer.

The candidate has no SHP file. The cross-source shop scan found 77 external
product fields with Hostel-range numbers (37 in active SHP files and 40 in
unregistered files), all with current active OBJ owners. No outside shop keeper
matches a Hostel mobile. The active #41 stock entries in
[`bs.shp`](../../../areas/shp/bs.shp#L690) and
[`torrhan.shp`](../../../areas/shp/torrhan.shp#L196) therefore retain the Heavens guild-badge identity; no active
shop stock uses #72. The single external shop-room number match is Heavens'
own room #23, not a Hostel location.

### Portals, code, and owner decisions

No external type-25 teleport destination (`value[0]`, the destination read by
[`spell_travel.c`](../../../src/magic/spell_travel.c#L991)) or type-29 switch
room target (`value[1]`, resolved through `real_room` in
[`specs.object.c`](../../../src/specs/specs.object.c#L339)) points into the
Hostel WLD range; the item type constants are
[`ITEM_TELEPORT` 25](../../../src/core/defines.h#L140) and
[`ITEM_SWITCH` 29](../../../src/core/defines.h#L144). Direct compiled
`real_room`, `real_mobile`, and `real_object` calls produce 100 numeric
matches (15 room, 26 mobile, and 59 object roles); each resolves to an active
same-number definition. None points to a Hostel-only room or its otherwise
unowned mobile #18. The code roles also reinforce the distinction between
Hostel and active Heavens objects #18, #41, #46, and #72, and between
Hostel's dying robed man #46 and active Heavens mobile #46. Active Heavens
#46 is Xena and has a direct `epic_teacher` assignment; the active Heavens QST
#46 remains empty. The compiled assignments bind Heavens object #46 to
`lightning_armor` ([`specs.assign.c`](../../../src/specs/specs.assign.c#L1478)),
#41 to `guild_badge` ([same file](../../../src/specs/specs.assign.c#L1568)),
#72 to `spell_pool` ([same file](../../../src/specs/specs.assign.c#L1620)),
object #18 to `githpc_special_weap` ([same file](../../../src/specs/specs.assign.c#L1751)),
and mobile #46 to `epic_teacher` ([same file](../../../src/specs/specs.assign.c#L2565));
another #18 code check is typed as an object comparison
([`specs.underworld.c`](../../../src/specs/specs.underworld.c#L2048)). Preserve
those active identities separately from the candidate QST/mobile records.

These bounded source scans do not establish editor, deployment, dynamic,
persisted-state, or runtime ownership. Keep the source bundle **RETAIN IN
PLACE** and resolve H-04, H-07, H-09 through H-12, H-19, H-20, and H-24 with
the affected owners before any activation, merge, renumbering, archive, or
path proposal. See the [updated builder questions](HOSTEL_BUILDER_QUESTIONS.md),
[typed connectivity note](WORLD_CONNECTIVITY.md#checkpoint-901-hostel-role-scans-leave-the-graph-unchanged),
[identity supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-901-hostel-parallel-low-vnum-identities),
[retention note](ATTIC_MIGRATION_PLAN.md#checkpoint-901-hostel-retains-separate-low-vnum-identities),
[candidate dependency row 52](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L52), and
[coverage row 402](ZONE_COVERAGE_REGISTER.csv#L403).
