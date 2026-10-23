# Neverwind Valley: repository-wide connectivity and dependency audit

**Checkpoint 80; priority 120 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive Neverwind Valley dossier](../zone-stories/NEVERWIND_VALLEY.md)
with the active WLD graph, inactive WLD inputs, typed references, compiled
numeric matches, normalized title, and bounded source history. Authored edges
and references do not establish ordinary reachability, safe traversal, source
availability, or played qualification.

## Active WLD graph

The active manifest contains 351 WLD contributors. Neverwind Valley has 199
rooms and 555 exits; all 555 targets resolve. The local graph is one weak
component of 199 rooms, with 552 internal exits and three outward boundaries.
All three outward boundaries have reciprocal active returns. There is also a
fourth active inbound edge from Connector Zones without a local return.

| Boundary | Reciprocal edge | Cross-area register rows |
| --- | --- | --- |
| #20400 south to Surface #528459 ([exit](../../../areas/wld/pyramid.wld#L20)) | Surface #528459 north to #20400 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L336), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1382) |
| #20466 north to Surface #525659 ([exit](../../../areas/wld/pyramid.wld#L1642)) | Surface #525659 south to #20466 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L337), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1371) |
| #20466 east to Surface #526060 ([exit](../../../areas/wld/pyramid.wld#L1646)) | Surface #526060 west to #20466 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L338), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1372) |
| Connector Zones #53725 south to #20466 ([exit](../../../areas/wld/connectorzones.wld#L2242)) | Incoming-only; no reverse local edge | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L670) |

There are no unresolved active exits. The all-95 inactive WLD scan parsed
2,080,773 exits without issues and found no source-room overlap. It found one
non-overlapping source-only approach from `surf.wld`:

| Source-only exit | Source record |
| --- | --- |
| `surf.wld` #235767 north to #20400 | [exit](../../../areas/wld/surf.wld#L750375) |

This declaration is not active ingress. It raises the curated source-only
inbound total from 684 to 685 and the individually reviewed Surf subset from
114 to 115. See the [Surf component row](ZONE_COVERAGE_REGISTER.csv#L448).

## Typed references and compiled numeric matches

The external QST scan found no command that consumes a Neverwind Valley
prototype. No external SHP product, story-sidecar numeric reference, external
ZON reset-role reference, or foreign mobile/object prototype definition
matched. The area has no local SHP file. The 231 other parsed story sidecars
had no exact structured references to its room, mobile, or object VNUMs.

The external type-25 scan found no destination in a Neverwind Valley room
among 872 declarations from other object files: 713 active and 159
source-only. The active total agrees with the maintained world portal count.

The direct C/C++ numeric scan found one executable room reference:
[`innates.c`](../../../src/classes/innates.c#L2010) includes room #20484 in
the computed Githzerai `shift_prime` arrival table. This is the alternate
arrival already described in the zone dossier, not a local quest hook (the
[room](../../../areas/wld/pyramid.wld#L2007) is “Inside a Cluttered Home”). The
other room-number match, #20515, appears only as `020515` in an old
[comment](../../../src/guild/guildhall.c#L39) inside a commented-out guildhall
binding block, so it creates no executable dependency. The remaining number
hit, 20480, is the socket-buffer-size
[constant](../../../src/net/comm.c#L526). No direct local mobile or object
match was found.

## Title, history, and disposition

The normalized title from [`pyramid.zon`](../../../areas/zon/pyramid.zon#L2)
is unique among the 351 active contributors. The global active-WLD duplicate
scan found only room #51800 in Hell Two and Hell Three; Neverwind Valley has no
duplicate room VNUM.

The followed WLD and ZON histories retain the `pyramid` paths from the
2009-08-12 area-tree move. The current WLD's latest content commit is
2013-11-13; the ZON's latest content commit is 2015-10-13, updating quest-item
flags. The bounded histories and curated duplicate/replacement register
identify no separate predecessor or replacement.

Neverwind Valley remains a current, un-commented
[`areas/AREA` contributor](../../../areas/AREA#L243) and is **DO NOT MOVE**.
The existing dossier covers the three native requests, four active inbound
edges, computed alternate arrival, and local source behavior. Active and
source-only exits do not qualify door/reset state, ordinary access, successful
use, hazard safety, or return travel. Tooling/editor closure, persisted
references, and played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 332: reset-aware access and builder decisions

The source-comprehensive [Neverwind Valley dossier](../zone-stories/NEVERWIND_VALLEY.md)
already records the three independent native quests, authored password and
maze clues, actors and rewards, and ten `ZSQ-PYRAMID-*` follow-ups. This
checkpoint adds an explicit builder decision list and reset-aware local WLD
traversal. No gameplay repair is selected.

### Reset-aware arrival reach

The 199-room WLD has 555 resolved exits in one weak component. Raw directed
traversal reaches 192/199 rooms from each distinct source-backed arrival:
Surface #528459 enters #20400, Surface #525659 and #526060 plus Connector
Zones #53725 enter #20466, and Githzerai `shift_prime` can choose #20484 as
one of 14 destinations. All three local roots reach the same set; the seven
rooms missed by those raw walks are #20592–#20598, where the staged elf and
Goar are reset.

The zone file declares 14 D commands and maps each to a WLD door. Their
post-reset states are eight closed/unlocked exits (state 1), one
closed/locked exit (state 2), and five closed/secret exits (state 5). The
loader treats the WLD exit state as door capability/pick flags and the ZON D
command applies the closed, locked, secret, or blocked state; see
[`setup_dir`](../../../src/world/db.c#L1565) and the
[`D` reset handler](../../../src/world/db.c#L7857). With all reset-closed,
locked, and secret exits unavailable, strict directed reach is 91/199 from
each of #20400, #20466, and #20484. The strict traversal does not model
opening doors, the DUNAN speech handler, secret discovery, reset timing,
mobile wandering, or actual `shift_prime` use. It is source graph evidence,
not a claim of safe or played access.

The six authored fall markers remain in rooms #20475, #20476, and
#20498–#20501; there are no current fields. The stairs have F25 falls and
the four treetops F10 falls. These declarations do not tell us whether a
player route is safe. Keep the three reciprocal Surface pairs and the
incoming-only Connector Zones edge as recorded above. The Surf candidate
already lists the separate source-only `surf.wld` #235767 north exit to
#20400; it remains on candidate row 97 and adds no active ingress.

### Checkpoint 332: builder questions and recommended resolutions

These are decision prompts for the zone owner. The recommendations preserve
current source behavior until an intended change is named and qualified.

| ID | Builder question | Recommended resolution from current source |
| --- | --- | --- |
| NV01 | Are the rod, letter, and four-egg returns meant to stay independent? | Keep the three native quests independent; do not add an egg, ring, password, or conversation prerequisite without an authored campaign decision. |
| NV02 | Must Q30's three wood sections come from three different kings' tombs? | Keep the current count-three rule unless the builder selects provenance. A distinct-tomb rule needs source-parent identity and a policy for gifts, parties, and previously held pieces. |
| NV03 | Is Maern's offering meant to require one egg of every color? | Keep the current four-prototype AND; duplicate copies of one color do not replace a missing kind. |
| NV04 | Can already-owned or gifted loose roots satisfy a request? | Preserve the native offering behavior and decide any personal-first-recovery milestone separately; supplied material is not personal acquisition proof. |
| NV05 | Should roots inside containers, worn, or held items count as prepared material? | Keep the current loose-inventory input contract. Record a different custody rule explicitly before changing it. |
| NV06 | Must each egg, wood section, and letter stay in the particular reset container that declares it? | Treat the O/P rows as intended placement, not instance-parent proof: P selects by prototype through `get_obj_num`. Reproduce a misplaced-root issue across partial resets before choosing a scoped repair. |
| NV07 | Is the apology letter's short explanation sufficient, and should players be expected to read it? | Keep its E text and current READ/LOOK-at behavior; change editorial wording only if the zone owner wants a longer explanation. |
| NV08 | Is Krodn intended to explain the family dispute without owning a quest? | Preserve Krodn's dialogue-only role unless a separate native contract and reward are explicitly chosen. |
| NV09 | Does delivering the letter need to record a meeting or reconciliation with Goar's father? | Treat the current result as letter delivery and Goar extraction. Add a family outcome only after the subject, destination, failure path, and replay policy are defined. |
| NV10 | Are Goar's hideout route and south jail endpoint intentional? | Keep the current exits and qualify ordinary wandering and retrieval from #20596/#20598 before changing staging or adding an endpoint. |
| NV11 | Are the elf's three staging rooms, glade destination, and no-exit jail intentional? | Keep the declared route through #20592–#20595 to #20591 and the branch to #20594 until normal player and mobile behavior is observed. |
| NV12 | Should room prose saying 25% and 12.5% correspond to actual appearance chances, and should daily stock respawn? | Do not read those prose percentages as reset chances: both M rows use cap 1/chance 100, while mundane AI selects among available exits. Qualify behavior and keep daily readiness from recreating departed givers or stock. |
| NV13 | Is the password instruction meant to stop at unlocking, or promise immediate passage? | Keep the documented sequence SAY DUNAN, OPEN DOUBLE, then move. Successful speech clears lock/secret state but leaves CLOSED; qualify both directions and reset behavior. |
| NV14 | Which start and route should the paper sequence N, N, E, S, E, N describe? | Ask the builder to name the intended start and verify each step under ordinary door and movement rules before adding a route achievement or rewriting clues. |
| NV15 | Which of trial starts #20561 or #20580 is intended, and what is the intended endpoint? | Keep the two source observations separate (#20573 and #20576); select an intended route and success endpoint before claiming a universal solution. |
| NV16 | Are the five secret passages sufficiently hinted by room prose and clues? | Preserve current secret flags and clues until the builder checks discovery from both sides, especially the paired passages around #20409, #20467, #20471, #20522, and #20589. |
| NV17 | Are the F25 stair falls and F10 treetop falls intended hazards on the authored routes? | Preserve the six source markers. Qualify fall outcomes and the intended safe route before describing any path as safe or changing a fall chance. |
| NV18 | Is Connector Zones #53725 south to #20466 intentionally one-way? | Preserve it as an active incoming-only edge; keep the three reciprocal Surface boundaries and decide a return only with the connector owner. |
| NV19 | Should Githzerai `shift_prime` arrival at Maern's #20484 remain an alternate entry? | Keep it a possible code-selected arrival, not a quest requirement. Confirm it is acceptable alongside the Surface and Connector routes before changing arrival logic. |
| NV20 | Should future milestones track possession of the ring/staff or actual protection/healing? | Keep native rewards unchanged. A future effect milestone should require committed wear/device use and measured effect; room #20591 healing and #20509 no-heal state are separate. |

No active boundary, source-only candidate, candidate count, or native area file
changed in Checkpoint 332. The existing audit closure still excludes tooling
and editor use, persisted references, and played qualification.
