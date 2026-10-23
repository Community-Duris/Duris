# Tezcat audit review — Checkpoint 383

Checkpoint 383 completes the source-story, active-connectivity, title/history,
typed-dependency, and builder-question review for active `tezcat`, the Ruined
Temple of Tezcatlipoca. Findings use the current native repository files and
followed history. This is a documentation-only review; no native gameplay,
source placement, database, or migration behavior changed, and no runtime
journey was performed.

The [source story dossier](../zone-stories/TEZCAT.md) accounts for all 100
rooms, 82 local mobile prototypes, 64 object prototypes, keyed routes, reset
groups, and the two foreign story contracts using Tezcat objects.

## Story and local contract boundary

Tezcat's environmental story combines an ancient-god chapel, sacrificial
altar, hidden tomb, a buried Night Crawler nest, and catacombs filled with
undead, worshippers, tortured souls, and named dark figures. The source calls
the god Tezcatlipoca; the east hidden wall says “Dreptu Allur,” the north wall
says “Doden,” and the mobile roster separately names Arkturas, Muinroht,
Raism, Vargher, and Verethorn. The files do not resolve whether these are
aliases, factions, or distinct historical layers.

The zone has no local QST, SHP, or story sidecar, although [QST/AREA lists
tezcat](../../../areas/qst/AREA#L316). Treat that as a source-list mismatch.
The active item contracts are owned by other areas: Kastle consumes two local
talons, and Tharnadia consumes the local sacrificial dagger. Their accepting
givers and rewards remain outside Tezcat.

The room prose describes severe stench, blindness, cold, souls in walls, and
almost certain death. There are no local room extra-description blocks, QST,
SHP, sidecar, or identified special behavior defining those statements as
runtime hazards. Keep those effects as prose unless a verified behavior
source supports a stronger claim.

## Active connectivity and keyed routes

The active [Tezcat WLD](../../../areas/wld/tezcat.wld) contains 100 rooms and
239 authored directions: 238 internal and one outbound. All local targets
resolve. The local weak graph has one 100-room component. Treating authored
directions as open, the two active arrival targets reach all 100 rooms, and
all 100 have a directed route back to the outbound source. These are raw WLD
traversals; locks, secret discovery, resets, scripts, dynamic travel, hazards,
and player safety are not simulated.

| Relationship | Source record | Qualification |
| --- | --- | --- |
| Active inbound | Surface #575445 east → Tezcat #98900 | Paired with Tezcat's west exit back to #575445. |
| Active inbound | Winterhaven #55632 west → Tezcat #98974 | Winterhaven describes this as its first teleport room for the gnomish key. Tezcat #98974 has no outward WLD route back to Winterhaven. |
| Active outbound | Tezcat #98900 west → Surface #575445 | Paired Surface return. |
| Source-only inactive approach | surf.wld #265973 east → #98900 | surf is unregistered; the source room has no active WLD owner. |
| Source-only inactive approach | underworld2.wld #429194 south → #98900 | underworld2 is unregistered; the source room has no active WLD owner. |

The inactive sources are not active ingress and do not change the current
active-inbound counts for Surf or Underworld2. Their room numbers do not
overlap any active room owner in the corresponding WLD scan. Retain both
directions with their existing source-map candidates; they do not establish
that either inactive map should be reactivated or merged.

All 14 D resets match the 14 door directions. The state-bearing routes are:

- #98904/#98937 to the high-priest office, stone key #98913, closed/locked;
- #98937/#98938 to the bedroom, bone key #98912, closed/locked;
- #98922/#98939 through the altar trapdoor, bloody key #98915, reset state 6
  (closed, locked, secret);
- #98943/#98944 into the secret chamber, with “Dreptu Allur” in the WLD
  description. WLD secret flags are asymmetric, while reset state 5 adds
  secret to both directions;
- #98957/#98962 with “Doden” etched into the wall, both WLD sides secret and
  reset closed;
- two entrance branch pairs, #98902/#98903 and #98902/#98926, reset closed
  but unlocked.

The local zone reset places stone key #98913 on zombie priestess #98925 at
#98904, bone key #98912 on Arkturas #98923 at #98937, and bloody key #98915
on zombie priest #98913 at #98922. The Night Crawler nest #98949 lies below
cave-in room #98948; its prose says the only exit is upward. Source placements
do not prove that keys remain available or routes are safe.

The active WLD evidence is also present in the
[cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1182),
[Winterhaven inbound row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L782),
[Surface return row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1498), and
[cross-area pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L606).

## Typed dependencies and ownership

The bounded typed scan found two active outside QST/story contracts using
Tezcat object prototypes, four external objects imported by Tezcat resets,
and five maintained code-data hits across three local prototype IDs.

| Direction | Exact source evidence | Typed relationship |
| --- | --- | --- |
| Tezcat → Kastle | [kastle.qst](../../../areas/qst/kastle.qst#L37), [sidecar](../../../areas/story/kastle.story.json#L124), local [talon reset](../../../areas/zon/tezcat.zon#L192) | Volordles requires two OBJ #98914 bloody talons. The follower #98904 at nest #98949 has two E placements. The sidecar records dagger #99447, 384,000 copper, and giver departure. Supplied proof does not establish source recovery or a kill. |
| Tezcat → Tharnadia | [tharnadia.qst](../../../areas/qst/tharnadia.qst#L90), [sidecar](../../../areas/story/tharnadia.story.json#L648), local [altar reset](../../../areas/zon/tezcat.zon#L129) | Zaberetornaz accepts OBJ #98928 from a Tezcazul follower #98920 at #98922. Sidecar reward is objects #500, #132629, and #132630, with `disappear=0`. City delivery credit is separate from Tezcat combat. |
| External object → Tezcat | [Winterhaven reset](../../../areas/zon/tezcat.zon#L76) | OBJ #55273 is reset on the floor at #98974; its keywords identify a “tormented spirit loving mother” object. Prototype owner is Winterhaven. |
| External object → Tezcat | [Heavens reset](../../../areas/zon/tezcat.zon#L77) | OBJ #359, a rune-covered stone, is reset at #98999; prototype owner is Heavens. |
| External object → Tezcat | [Unique reset](../../../areas/zon/tezcat.zon#L193) | OBJ #67280, death ring of Tezcatlipoca, is given to the Night Crawler leader #98900 at #98949; owner is Unique. Keep it distinct from local OBJ #98904, also a death ring of Tezcatlipoca. |
| External object → Tezcat | [Smokeveil reset](../../../areas/zon/tezcat.zon#L245) | OBJ #20255, Grishnak's head, is given to Diabolus follower #98961 at #98974; owner is Smokeveil. The active source reset identifies the carrier, not a local Grishnak mobile. |
| Maintained code → Tezcat | [Chaos equipment data](../../../src/account/chaos_eq_data.h#L75), [random equipment list](../../../src/item/randomeq.c#L128) | OBJ #98911 appears in standard and enhanceable warrior kits (slot 27); OBJ #98949 appears in standard and enhanceable ranger kits (slot 3). MOB #98959 Bard Faust appears once in `highdrop_mobs`. These are numeric data bindings, not quest or recovery proof. |

The object and mobile records sharing #98914 and #98928 are different types.
The two external QST instructions use `G I` object proofs and the sidecars
declare those numbers as item VNUMs. Do not interpret those lines as external
mobile #98914/#98928 dependencies.

No active foreign ZON reset imports a Tezcat-owned mobile or object. No SHP
product reference to a Tezcat object or type-25 portal destination into a
Tezcat room was found. This is a bounded source scan; it does not claim live
inventory or reset state.

## Title, identity, and followed history

After color-code removal, terminator trimming, whitespace normalization, and
case folding, the title normalizes to “Ruined Temple of Tezcatlipoca.” It is
unique among all 442 current ZON title records and belongs to one of the 351
active contributors. Same-type active WLD, MOB, and OBJ scans found no VNUM
overlap with Tezcat; all 95 inactive WLD sources were also checked for exact
room VNUM overlap and none was found. The #98914 and #98928 cross-type pairs
within the zone remain distinct mobile and object prototypes.

Followed WLD and ZON histories stay on the `tezcat` stem; no alternate-stem
predecessor, named replacement, or whole-zone replacement was established.
The 2011 WLD edit `5794d8067` changed entrance #98900 west from Surface
#575445 to #249233 and added a down edge to #249534. The 2013 edit
`71d731c5b` removed that down edge and restored the west exit to #575445. This
records a temporary routing detour and its reversal; the commit titles do not
establish why it changed. Followed ZON history also includes the 2015
quest-item-flag pass and a 2014 worshipper spelling correction. There is no
local QST or sidecar history because those files are absent.

Keep active `tezcat` **DO NOT MOVE**. No alternate candidate or migration
clearance is supported by title or history.

## Builder questions and recommended resolutions

These questions preserve source ownership and avoid inventing lore or
mechanics before intent is confirmed.

1. **Cult names:** Are Tezcatlipoca, “Dreptu Allur,” and “Doden” names for one
   deity, separate powers, or historical clues? Keep each as authored until
   the builder confirms their relationship.
2. **Named factions:** How should Arkturas, Raism, Muinroht, Vargher, and
   Verethorn relate to the temple cult? Keep their current titles and room
   text distinct until their allegiances are confirmed.
3. **Public entrance and return:** Is Surface #575445 ↔ Tezcat #98900 the
   intended ordinary entrance and return? Preserve the current reciprocal
   pair unless the builder specifies another destination.
4. **Winterhaven arrival:** Is Winterhaven #55632 west into #98974 meant to
   be a one-way teleport arrival? The local WLD has no ordinary return edge;
   retain that shape until one-way access is confirmed or a return is designed.
5. **Historical detour:** Did the temporary 2011 route through #249233 and
   #249534 express a short-lived design, or only map work? Keep the restored
   Surface route as current and record the history without reviving the detour.
6. **Inactive-map routes:** Are inactive Surf #265973 and Underworld2 #429194
   intentional historical approaches to #98900? Preserve each on its current
   candidate row; do not infer live ingress or move either map.
7. **Quest registration:** Should the `tezcat` row in QST/AREA be backed by a
   local QST file or removed from the index? Resolve the list mismatch without
   creating an inferred quest.
8. **Environmental hazards:** Do stench, cold, blindness, and trapped-soul
   descriptions correspond to mechanics? Keep them prose-only unless an
   existing maintained behavior is identified and confirmed.
9. **Entrance branches:** Are the closed but unlocked pairs #98902/#98903
   and #98902/#98926 intended to keep the entry branches gated? Retain the
   resets and reconcile text only after their access role is confirmed.
10. **Office key:** Is stone key #98913 on #98925 the intended key source for
    the office pair #98904/#98937? Preserve the current placement while
    confirming expected search and combat access.
11. **Bedroom key:** Is bone key #98912 on Arkturas #98923 at #98937 intended
    for the adjacent bedroom pair #98937/#98938? Keep the key and pair
    together until the route is reviewed as a player journey.
12. **Trapdoor:** Is bloody key #98915 on #98913 the intended way through
    altar/catacomb pair #98922/#98939, whose reset also makes it secret?
    Confirm the locked and hidden states as one design.
13. **Secret chamber:** Should the #98943/#98944 wall pair remain asymmetric
    in WLD flags when its reset makes both sides secret? Preserve the reset
    while the builder decides the initial discovery state.
14. **Second wall clue:** Is “Doden” at #98957/#98962 a clue, name, or
    independent passage? Keep both secret flags and source text aligned
    pending that explanation.
15. **Nest escape:** Is the prose that the Night Crawler nest #98949 has only
    an upward exit accurate for the intended route from #98948? Check the
    source text and actual room links together before changing either.
16. **Kastle proof:** Should Volordles continue to accept any two loose
    exact #98914 objects regardless of who recovered them? Preserve the
    two-item proof and keep personal source or kill history separate.
17. **Tharnadia proof:** Should Zaberetornaz accept supplied object #98928
    with `disappear=0`? Preserve the foreign city delivery, reward, and
    ownership boundary; do not add personal combat credit implicitly.
18. **Imported resets:** Are the Winterhaven #55273, Heavens #359, Unique
    #67280, and Smokeveil #20255 resets intentional? Keep their prototype
    owners and local carrier/room placements distinct; in particular, retain
    local death ring #98904 separately from Unique's imported #67280.
19. **Code data:** Are warrior-kit object #98911, ranger-kit object #98949,
    and Bard Faust mobile #98959 intended to remain maintained numeric
    bindings? Review those equipment/drop policies with their owning systems;
    do not duplicate the prototypes.
20. **Reward and danger tone:** Should the room warnings and boss-held gear
    advertise a specific combat challenge or remain atmospheric? Keep the
    current prose and reset catalogue separate from any unimplemented
    hazard, reward guarantee, or quest promise.

## Coverage references

- [Coverage row 296](ZONE_COVERAGE_REGISTER.csv#L296)
- [Connectivity checkpoint 383](WORLD_CONNECTIVITY.md#checkpoint-383-tezcat-connectivity-and-inactive-approaches)
- [Title/history checkpoint 383](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-383-tezcat-title-identity-and-history)
- [Retention checkpoint 383](ATTIC_MIGRATION_PLAN.md#checkpoint-383-tezcat-retention-and-existing-map-candidates)
- [Ruined Temple of Tezcatlipoca dossier](../zone-stories/TEZCAT.md)

## Checkpoint 838: Current typed closure refresh

A bounded current-source pass against the unchanged pinned `areas/` and `src/`
trees rechecks Tezcat's typed edges and preserves their owners:

- **Kastle item proof:** Kastle's two exact `G I 98914` requirements consume two
  Tezcat OBJ #98914 talons. The local reset equips both on one follower MOB
  #98904 in nest room #98949. The accepting quest and its reward remain Kastle-
  owned; these source records do not prove personal recovery or a kill.
  ([Kastle QST](../../../areas/qst/kastle.qst#L37),
  [Kastle sidecar](../../../areas/story/kastle.story.json#L124),
  [Tezcat reset](../../../areas/zon/tezcat.zon#L192))
- **Tharnadia item proof:** Tharnadia's QST accepts OBJ #98928 from the Tezcat
  altar follower MOB #98920. Its sidecar records the three rewards and
  `disappear=0`; the city owns the contract and delivery credit. Keep the
  local dagger and follower roles separate.
  ([Tharnadia QST](../../../areas/qst/tharnadia.qst#L90),
  [Tharnadia sidecar](../../../areas/story/tharnadia.story.json#L648),
  [Tezcat reset](../../../areas/zon/tezcat.zon#L129))
- **Imported objects:** Tezcat continues to reset Winterhaven OBJ #55273,
  Heavens OBJ #359, Unique OBJ #67280, and Smokeveil OBJ #20255. Their
  prototypes remain owned by those areas. In particular, external #67280 is
  distinct from local Tezcat OBJ #98904, even though both are death rings.
  ([Tezcat reset: Winterhaven and Heavens](../../../areas/zon/tezcat.zon#L76),
  [Unique](../../../areas/zon/tezcat.zon#L193),
  [Smokeveil](../../../areas/zon/tezcat.zon#L245))
- **Maintained code data:** OBJ #98911 and #98949 remain numeric equipment-kit
  references; MOB #98959 Bard Faust remains in the high-drop list. These data
  bindings do not establish a quest, item recovery route, or guaranteed stock.
  ([chaos equipment](../../../src/account/chaos_eq_data.h#L75),
  [random equipment](../../../src/item/randomeq.c#L128))

The negative closure remains bounded and unchanged: no active foreign ZON reset
imports a Tezcat-owned mobile or object, no outside SHP product names a Tezcat
object, and no type-25 object targets a Tezcat room. Tezcat still has no local
QST, sidecar, or SHP; the QST/AREA listing remains a source-list mismatch.
These findings add no WLD edge and do not establish runtime reset availability.
Keep active Tezcat **DO NOT MOVE**, with typed ownership attached to its current
areas. The existing Surf and Underworld2 source-only approaches stay on
candidate rows 97 and 116.
