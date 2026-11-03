# The Adventurers Guildhalls

Reviewed October 10, 2026 for active `area:player_castles`, zone 110,
VNUMs #11000–#11130. Its source files are
[`player_castles.wld`](../../../areas/wld/player_castles.wld),
[`player_castles.mob`](../../../areas/mob/player_castles.mob),
[`player_castles.obj`](../../../areas/obj/player_castles.obj),
[`player_castles.zon`](../../../areas/zon/player_castles.zon), and
[`player_castles.shp`](../../../areas/shp/player_castles.shp). The area has
three rooms, ten mobile prototypes, and 59 object prototypes. It has no local
QST file, native Q/QA contract, or story sidecar.

## What the authored story says

The WLD is a small foyer, a jokingly named “Fafhrd’s Castle of Testing,” and
a generic “shop room.” The testing-room description explicitly reads as a
builder aside. The source does not establish a player objective, quest
sequence, named antagonist, or completion outcome. The area title and shared
object catalog point to guildhall support, but the source does not describe
how a player receives or builds a hall. Keep those undocumented steps as
builder questions rather than filling them in as story.

The object file is broader than the three-room map: it defines doorway and
building objects, crude lock and key templates, a bonding altar, numbered
magic mouths, and a sequence of board prototypes. Most boards retain generic
names; #11118 and #11130 are identified as the Player Council meeting and
discussion boards. The mobile file defines a testing platoon, guards and
golems, a guild merchant, and Dalvik the sailor. These are prototype
descriptions; their existence does not establish current placement.

## Authored placement and shop stock

The zone file has reset mode 0. It records the wooden door object #11008 in
room #11001, the merchant #11005 in the isolated shop room #11005, and twelve
external stock objects on that keeper: #5725, #364, #412, #3050, #95547,
#6089, #53, #204, #203, #95540, #399, and #5. The local SHP file for keeper
#11005 lists the same twelve products. Eleven resolve to prototypes in
currently active object files; no checked-in object prototype for #6089 was
found. The file records remain authored configuration, not proof that the
reset or shop is live.

## Shared guildhall bindings and consumers

Compiled object bindings give this source a support role beyond its local
map. Objects #11002 and #11051–#11065 use `magic_mouth`; #11008 uses
`guildwindow`; and #11013 uses `guildhome`. The latter records a character’s
birthplace when the character says “home” to the altar. Code also recognizes
#11007 and #11008 as house or guild entry/exit doors. The #11010 lock template
and #11011 key template are consumed by lock and key commands.

Active source consumers include CBL Hall resets of six Player Castles object
prototypes, a Heavens reset of the Player Council meeting board #11118, a
Surface reset of mobile #11006, and Tharnadia shop stock for #11010 and
#11011. Forty-one existing unregistered area candidates have source-only
guildhall resets of Player Castles objects; `surf.zon` has a source-only
reset of mobile #11006. Two other unregistered SHP candidates list four
Player Castles board products. The audit review records their exact source
lines and keeps those references attached to their existing candidate rows.

The separate active `guildhalls` contributor has a documented history as the
successor to `castle_rooms` and includes a few object prototypes copied from
Player Castles. Those facts do not establish that `player_castles` was renamed
or replaced. Keep the two active contributors distinct.

## Title, history, and retention

After color-code removal and whitespace normalization, “The Adventurers
Guildhalls” is unique among 442 current `.zon` titles and in the active title
set. Followed WLD, MOB, OBJ, ZON, and SHP histories remain on the
`player_castles` stem from the 2009 move into the current area tree through
later same-stem edits. No alternate-stem rename or whole-zone replacement is
established by those paths. See the
[audit review](../quest-story-followups/PLAYER_CASTLES_AUDIT_REVIEW.md#title-followed-history-and-retention).

Keep active `player_castles` **DO NOT MOVE**. Preserve the three authored
rooms, object and mobile VNUMs, shared callback bindings, current active
resets and stock references, and the source-only guildhall consumers on
their existing candidate rows. No route, reset, gameplay, or migration change
is proposed.
