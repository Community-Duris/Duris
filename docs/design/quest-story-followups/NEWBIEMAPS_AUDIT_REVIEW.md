# NEWBIEMAPS source and map audit

Canonical key: component:newbiemaps. Coverage data row 428 / CSV line 429;
candidate dependency data row 77 / CSV line 78. Reviewed October 11, 2026,
Checkpoint 515, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74.

This is a static source, map-identity, connectivity, history, and typed-reference
review. It does not establish that either map is loaded in a live world, that
the unusual ZON fields are accepted at runtime, or that external editor and
persisted data are clear.

## Source package and authored map

The plural source stem has only two same-stem area files:
[WLD](../../../areas/wld/newbiemaps.wld#L1) and
[ZON](../../../areas/zon/newbiemaps.zon#L1). There is no same-stem MOB, OBJ,
QST, SHP, editor-map JSON, or story sidecar. Neither
[full area manifest](../../../areas/AREA) nor
[quest area manifest](../../../areas/qst/AREA) lists newbiemaps; the
[RANDOM_AREA list](../../../areas/RANDOM_AREA) does not list it either.

The WLD contains 89,847 rooms from #600000 through #689999. It has 24 exact
room-title strings: 67,109 “The Ocean,” 13,419 “Slow, Leisurely River,”
2,464 “Fast-Moving River,” 2,004 “Newly Formed Mountains,” 1,290 “Great
Cavern of the Dwarven Mines,” and smaller terrain and landmark groups.
Single-room labels include NEWBIE1–NEWBIE7, “GnomeDwarf,” “Connector to top,”
and “HARROW” (for example [NEWBIE1](../../../areas/wld/newbiemaps.wld#L655498),
[Connector to top](../../../areas/wld/newbiemaps.wld#L614402), and
[HARROW](../../../areas/wld/newbiemaps.wld#L727603)). This is a large authored
map with named regions and helper markers; it has no local quest, mobile, or
object package that explains the map’s intended deployment.

The [ZON](../../../areas/zon/newbiemaps.zon#L1) calls its area “newbiemap,”
while the filename is plural. That normalized title is unique among the 442
current ZON titles checked. Its header is #10879352, while the WLD occupies
#600000–#689999 and the ZON last VNUM is #689999. The current inventory parses
the raw reset field as 1635218021. The following numeric row is 300 300,
then S; there are no M/O/G or other typed reset commands. Preserve these raw
fields and ask the area owner to confirm the header and reset-field format
before any activation or edit.

## Candidate graph and current Surface ownership

A static parse found 359,196 WLD direction records. All use north/east/south/
west; each has its opposite direction back to the source. Every target exists
in the candidate WLD, so its local exit graph is one weak component containing
all 89,847 candidate rooms, with no negative or missing local targets. The
90,000-number interval has 153 absent room records. Among existing records,
eight each lack a north or south exit and 88 each lack an east or west exit.
These are source facts, not proof of a broken map.

Active surface.wld currently owns 59,851 of the same room numbers, all with
different room-title strings from the candidate records (compare candidate
[#600000](../../../areas/wld/newbiemaps.wld#L1) and active
[#600000](../../../areas/wld/surface.wld#L2099393)). The other 29,996
candidate rooms, #660004–#689999, have no current active WLD owner. Of the
candidate’s exits, 239,212 numerically target active Surface rooms. Of those,
238,610 start in candidate rooms whose source VNUM is already owned by active
Surface; these are overlapping map-grid identities, not evidence of 238,610
routes from a distinct inactive place.

The remaining 602 directions start in 600 candidate rooms without an active
room owner and point into active Surface. They comprise 300 north exits from
#660004–#660303 to #659704–#660003, 300 south exits from #689700–#689999 to
#600000–#600299, plus #660004 west to #660003 and #660299 east to #660000.
The existing [Surface audit](SURFACE_AUDIT_REVIEW.md#inactive-wld-surface-approaches)
records sample source lines. These are one-way source-only approaches: a scan
of current active WLD exits found no targets in the candidate-only
#660004–#689999 range. The same 602 (source, direction, target) tuples also
occur in surface2011-temp.wld; preserve both source-file identities and do
not count the matching tuple set twice as distinct geography.

A raw active-WLD scan found 239,396 exit targets whose numbers fall anywhere
in the candidate’s room set. Every one is currently owned by active Surface
in the overlapping #600000–#660003 range. None targets one of the 29,996
candidate-only room VNUMs, so these numeric matches are active Surface
references rather than active inbounds to newbiemaps.

## Copy history and competing map identities

Git follows the newbiemaps.wld and .zon paths to the August 12, 2009
areas-tree move; no later same-stem edit, rename, or explicit retirement was
found. Git records surface2011.wld as a February 20, 2011 copy from
newbiemaps.wld (51% similarity), and surface2011-temp.wld as a May 12,
2011 copy from newbiemaps.wld (72% similarity). The copied WLDs are
separate source paths, not a recorded decision that one replaces the other.

Current data reinforces that separate-identity finding. The 90,000-room
surface2011-temp.wld shares all 89,847 newbiemaps room numbers, and its
360,000 exits contain every one of the candidate’s 359,196 exit tuples plus
804 additional directions. Only 72 of the shared room numbers have exactly
the same title string. The active Surface WLD shares 59,851 room numbers
with newbiemaps, has no exact title matches at those numbers, and shares
118,889 exit tuples. Numeric overlap and copied geometry do not establish
which map was meant to be authoritative or whether either was intended to
replace active Surface.

## Typed references and limits

Repository-scoped checks found no current active WLD exit, ZON reset, SHP
product, story-sidecar field, or compiled C/C++ room literal targeting
candidate-only rooms #660004–#689999. The only QST number match in that range
is shipy.qst line 887, R C 666666; R C is a currency reward field, not a
room reference (see the [source line](../../../areas/qst/shipy.qst#L887) and
[typed QST example](../../../areas/qst/surfacekeeps.qst#L392)). Numeric matches
in active MOB/OBJ records are in their separate prototype or item-data
namespaces and are not room-owner evidence. No active portal destination into
the candidate-only room range was identified.

The 602 candidate-to-Surface directions, the 59,851 active Surface room-number
overlaps, and the duplicate map copy remain explicit dependencies to resolve
before any map merge or activation. Editor projects, deployed file manifests,
live room tables, and persisted player/object references are outside this
repository review and remain unknown.

## Disposition

**RETAIN IN PLACE.** Preserve newbiemaps.wld and newbiemaps.zon as a separate
source identity. Resolve the #10879352 header and unusual reset field;
determine the 153 omitted room records and 804-direction difference against
the 2011 copy; confirm ownership of the 59,851 Surface overlaps and the 602
source-only approaches; and locate editor, deployment, and persisted identity
references before any activation, renumber, merge, move, or archive decision.
No migration clearance is implied. See the
[18 builder questions](NEWBIEMAPS_BUILDER_QUESTIONS.md),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-515-newbiemaps-source-map-graph),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-515-newbiemaps-source-identity-and-copy-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-515-newbiemaps-retain-both-map-identities),
[candidate dependency line 78](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L78), and
[coverage line 429](ZONE_COVERAGE_REGISTER.csv#L429).
