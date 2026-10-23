# Temple of Laduguer audit review

Checkpoint 381 completes the static story, graph, typed-reference,
title/history, and builder-question review for active temple_laduguer. Keep
the source in place. All findings below are based on repository files and
history; no runtime or player outcome is inferred.

## Story and environment closure

The [comprehensive dossier](../zone-stories/TEMPLE_OF_LADUGUER.md) maps all 103
room descriptions, the 24 mobile prototypes, the 28 object prototypes, the
mode-2 reset file, and their thematic ties. The source depicts a volcanic
Duergar Temple and forge settlement with guard and training quarters, circular
temple halls, two shrine clusters, volcanic passages, visitor inn, and
steeder stables.

There is no local QST file or giver, no story sidecar, and no local SHP file.
No active external QST contract, ZON reset, or SHP product targets local
Temple prototypes. The audit found no active same-type mobile or object
definition collision for local IDs #8100–#8127. No active code binding
configures the named innkeeper as a functional inn or binds a Temple mobile to
the legacy guild-guard branches described below.

## Active connectivity and door state

The active WLD has 103 rooms and 215/215 resolving authored directions: 214
internal and one outward. Its weak graph is one component of 103 rooms. Raw
directed traversal from #8100 reaches 103/103 rooms, and reverse traversal
back to #8100 also reaches 103/103, with door, hazard, and script state
ignored.

The only active boundary is Temple #8100 west to Underdark #827455, paired by
Underdark #827455 east to Temple #8100. No active WLD room enters or leaves
through any other boundary. There are no negative targets, unresolved
positive targets, range mismatches, or active-WLD targets into an unregistered
WLD source. The coverage inventory records two fall-chance rooms and zero
current markers.

The source contains 71 exits with the EX_ISDOOR bit and 72 matching D reset
commands. The paired forge entries #8196 east to #8197 and #8197 west to
#8196 are both closed and locked by reset state 2. The forward exit names key
object #8121, but the reverse exit has key number zero; the reset distributes
#8121 to one elite forge guard group in #8196.

The #8167 down exit to #8168 carries key #8125 and reset state 2, but its WLD
exit flags are 2 and omit EX_ISDOOR. The reciprocal #8168 up exit is a door
reset closed with state 1, has no key, and is not locked by its reset. Key
#8125 is given to the last of four veteran guards at #8167. Confirm whether
these flag and reciprocal-state differences are intentional before treating
the exits as a reliable lock-and-key route.

## Inactive Sylvandawn approaches and room identity

The bounded scan of all 95 unregistered WLD files finds only sylvandawn.wld
targeting Temple room numbers. The file has 223 raw directions into
#8100–#8202:

- Seventeen approach directions start in Sylvandawn room IDs with no active
  room owner. Preserve these on existing [Sylvandawn candidate row
  101](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L101). They are source-only
  approaches, not active ingress.
- The other 206 directions start from room numbers that also occur in active
  Temple WLD. All 206 source room titles differ from their Temple titles.
  These are same-number records with distinct text and must not be presented
  as 206 current routes.

| Inactive Sylvandawn source room | Direction | Active Temple target |
| --- | --- | --- |
| #8007, City Center | south | #8163 |
| #8007, City Center | west | #8110 |
| #8026, An Intersection of Pathways | east | #8172 |
| #8026, An Intersection of Pathways | south | #8171 |
| #8030, A Split in the Pathways | east | #8180 |
| #8086, A split in the Paths | south | #8128 |
| #8091, A Split in the Paths | east | #8102 |
| #8091, A Split in the Paths | south | #8119 |
| #8094, The Sylvan Trail | east | #8101 |
| #8098, The Sylvan Trail | east | #8100 |
| #8203, Practice Room | north | #8202 |
| #8206, An Overgrown Path | west | #8195 |
| #8210, The Trail of Tears | north | #8185 |
| #8211, The City Dump | east | #8121 |
| #8311, A Burnt Down and Abandoned Tower | north | #8180 |
| #8317, The Commons of the Sylvandawn Grovetree Theatre | south | #8188 |
| #8327, Alkatia's Fletcher Shop | south | #8172 |

Because Sylvandawn is absent from current areas/AREA, its routes do not add
active inbound exits. The existing active-inbound and unresolved counts on
candidate row 101 remain zero. No migration candidate is added for the active
Temple.

## Typed source and code dependencies

The unregistered [Sylvandawn SHP](../../../areas/shp/sylvandawn.shp) has eight
product references to active Temple object prototypes. These are inactive
source records, not stock in a loaded shop:

| Sylvandawn shop | Object product | Source line |
| --- | ---: | ---: |
| #8002 | #8112 | 22 |
| #8003 | #8108 | 61 |
| #8005 | #8106 | 101 |
| #8006 | #8105 | 140 |
| #8007 | #8103 | 178 |
| #8043 | #8111 | 528 |
| #8304 | #8116 | 771 |
| #8305 | #8118 | 812 |

The unregistered [Sylvandawn reset file](../../../areas/zon/sylvandawn.zon)
contains 115 M/O/D commands whose room argument falls in #8100–#8202. Some
numeric room arguments overlap the active Temple IDs; they remain commands in
an unregistered source file and must not be treated as active placement or
active door state. Its M/O/G/E prototype arguments do not reference Temple
mobile or object prototypes; the 115 matches are room-argument references.
The file has other existing typed consumers recorded on
candidate row 101, including reset or shop references to prototypes owned by
Lizardman Swamps, Orrak, Dragonnia, Kobold Settlement, Plane of Fire, and the
Faerie Realm.

src/specs/specs.guards.c has class-gate cases for numeric rooms #8200
(Rogue-only west) and #8113 (Sorcerer-only south). The #8200 branch matches
legacy Sylvandawn code context: specs.assign.c binds Sylvandawn mobile #8019
to guild_guard, and the inactive zone resets #8019 into its own room #8200.
Current Temple #8200 instead has one south exit to #8130 and resets Valkar
there; no Temple mobile is assigned to guild_guard. No mobile is currently
reset to Temple #8113 either. Treat these numeric code cases as
identity-sensitive references, not proof that current Temple passage has an
active class gate. The obsolete commented object bindings near #8110–#8113
are not active procedure assignments.

## Title, same-type identity, and followed history

Color-token removal and title normalization leave “The Temple of Laduguer”
unique among the 351 current active .zon titles. It is a separate title from
Temple of Flames, Temple of the Earth, Temple of the Sun, and Temple to
Skrentherlog.

Followed WLD/ZON history records the new area, credited to Nauraki, in commit
f74b1c035 (2016-04-14) and a change from zone header number 1 to 81 in
2c97f027b (2016-06-02). Current component history remains on the temple_laduguer
stem. The unregistered Sylvandawn WLD reuses every Temple room number, but
the 103 overlapping titles differ; its source history and files do not
establish an alternate-stem rename or replacement claim. No whole-area
predecessor or replacement is documented by these paths.

## Twenty builder questions and conservative recommendations

1. **Temple identity and deity language:** Should Laduguer be described with
   the “patroness” wording found in some room prose, or consistently as the
   god named elsewhere? **Recommendation:** have the lore owner settle the
   identity and then update only the specific descriptions they intend.
2. **Temple entrance:** Are the three initially closed gatehouse exits meant
   to be an access puzzle, ordinary doors, or descriptive barriers? **Recommendation:**
   document the intended entry route and preserve current reset states until
   confirmed.
3. **Underdark boundary:** Is the reciprocal #8100/#827455 boundary the
   complete intended connection? **Recommendation:** preserve its active
   owner pair and require separate gameplay evidence before adding another
   route or promising safe return.
4. **Inn service:** Is room #8200 and Valkar meant to provide lodging, or is
   “innkeeper” only narrative identity? **Recommendation:** treat the room and
   mobile prose as flavor until a supported service is specified.
5. **Steeder stables:** Are #8157/#8201/#8202 intended to support mount
   storage, purchase, or simply describe the settlement's mounts?
   **Recommendation:** do not imply a stable service; none is configured by
   the local source files.
6. **Local story scope:** Is the absence of a native request or journal
   intentional for the Temple, or should a separate story feature be designed?
   **Recommendation:** preserve the source-only story mapping and do not
   invent an objective, giver, or reward.
7. **Shrine offerings:** Do the offering-pedestal descriptions imply a
   specific offering action? **Recommendation:** keep them as environmental
   prose unless the builder identifies a command, object, state change, and
   intended repeat/reset policy.
8. **Craftsman pilgrims:** Should the pilgrims #8110/#8111 have an explicit
   relationship to the ash badge or forged equipment? **Recommendation:**
   describe only the roles and placements already in source; define custody
   and behavior before making the items objectives.
9. **Trainer behavior:** Do the five named instructors offer a particular
   training path or eligibility rule? **Recommendation:** rely on actual
   configured service evidence, not names alone, and retain each instructor's
   existing source ownership.
10. **Gear distribution:** Are the axes, artisan badge, armor, and clerical
    set meant as resident equipment, rewards, or sale products?
    **Recommendation:** preserve current local reset ownership and do not
    treat inactive Sylvandawn shop products as loaded stock.
11. **Fire mass and potion:** What role, if any, should objects #8122 and
    #8126 play in an encounter or offering? **Recommendation:** keep their
    reset and prototype roles descriptive until a source-backed interaction
    is specified.
12. **Lower forge key:** Is #8125 intended to open the #8167 down exit? Its
    WLD flags omit EX_ISDOOR while the reset sets it closed and locked.
    **Recommendation:** confirm the authored flag and reset intent together
    before changing either side.
13. **Lower stair return:** Should #8168 up remain closed but unlocked while
    the #8167 down exit is reset locked? **Recommendation:** confirm both
    states and the expected return route as one builder decision.
14. **Forge key identity:** Is #8121 the intended key for the locked #8196
    east exit? The reciprocal #8197 west exit names key zero.
    **Recommendation:** specify whether key access should be symmetric and
    keep the reset distribution at #8196 until the key contract is clear.
15. **Forge reset state:** Should both forge exits remain closed and locked
    after reset, or should a character who can open one side gain the return?
    **Recommendation:** document the intended operation and reset/relock
    behavior before changing the reciprocal state.
16. **Volcanic hazards:** Which exact rooms marked with fall chances should
    be treated as dangerous, and what should their consequences be?
    **Recommendation:** inspect and name the intended hazard rooms in a
    separate gameplay review; static connectivity is not a safety result.
17. **Steam bridge:** Does “difficult to breathe” denote atmosphere or an
    implemented breathing hazard? **Recommendation:** preserve the prose and
    make no safety or damage promise without live mechanics evidence.
18. **Xaphan and elementals:** Are the keeper, rogue elementals, summoned
    elementals, and fire mass one intended forge encounter?
    **Recommendation:** keep the authored placements distinct; no local
    quest chain or encounter trigger joins them.
19. **Assassin:** Does the stable assassin have a specific intended target,
    disguise, or follow-up? **Recommendation:** do not infer a victim or
    reward from the mobile description and daggers.
20. **Legacy Sylvandawn identity:** Should its seventeen source-only routes,
    same-number rooms, reset arguments, and Temple-object products remain
    separate source context? **Recommendation:** preserve them on candidate
    row 101 as source-only, keep Sylvandawn RETAIN IN PLACE, and require a
    separate owner decision before changing its files or path.

## In-place disposition and verification boundary

Keep active temple_laduguer **DO NOT MOVE** as a current areas/AREA
contributor. Keep unregistered sylvandawn **RETAIN IN PLACE** on existing
candidate row 101. Preserve source ownership of the seventeen approaches,
206 differently titled same-number transitions, 115 reset room arguments,
and eight SHP product references. No candidate or migration clearance is
created by this audit.

No gameplay, code, source area, account, database, migration, server, or
generated area file was changed. The work is a static source and Git-history
review only; no build, regression test, or live access journey was run.

## Checkpoint 836: Sylvandawn dependencies remain source-only

An exact-source refresh against the unchanged pinned snapshot confirms the already reviewed Sylvandawn relationships:

- The inactive Sylvandawn WLD still has 17 distinct approach directions into active Temple rooms. Its other 206 target matches start from source-room VNUMs also present in Temple, with different room titles; they are not additional entrances.
- The unregistered Sylvandawn ZON still has 115 `M/O/D` records whose room argument is in #8100–#8202. Its M/O/G/E prototype arguments do not import Temple prototypes, and none of these source-only room placements or door resets is active Temple state.
- The unregistered Sylvandawn SHP still lists eight active Temple objects: #8112, #8108, #8106, #8105, #8103, #8111, #8116, and #8118. These are source-only product references, not stock in a loaded shop. Preserve them with existing Sylvandawn candidate row 101 **RETAIN IN PLACE**.
- The numeric guard-code references at #8200 and #8113 remain identity-sensitive historical/source matches; no current Temple mobile is assigned to `guild_guard`, and no Temple mobile is reset at #8113. They do not establish current class gates.

No active external QST, ZON prototype reset, SHP stock, same-type mobile/object definition, or other inactive-WLD approach was found. Keep the active Temple in place and preserve Sylvandawn's candidate-specific source identities; this refresh adds no route, candidate, or migration change.

