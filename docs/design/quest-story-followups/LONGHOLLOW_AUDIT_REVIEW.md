# Defense of Longhollow (long): world, dependency, and owner review

**Scope:** checkpoint 266 adds connectivity, title/history, typed dependency,
attic, and builder-owner reconciliation to the existing source-comprehensive
[Defense of Longhollow dossier](../zone-stories/DEFENSE_OF_LONGHOLLOW.md) and
[native source index](../../reference/zone-story-audits/long.md). It does not
qualify live player access, reset stock, accounting, or gameplay outcomes.

## Story coverage and authored inventory

The active contributor is zone 344, titled Defense of Longhollow. It has 100
rooms, 77 mobiles, 74 objects, 41 QST giver records, fifteen native Q/QA
contracts, one shop, and a revision-1 story sidecar. The sidecar describes
fourteen stories and 41 contacts. The existing dossier reviews all room, mobile,
object, reset, shop, native QST, story, and relevant compiled-support sources;
it should remain the source of record for exact offerings, rewards, scenery,
environmental scenes, progression, item effects, and unimplemented endpoints.

The mapped narrative distinguishes the siege proof delivery and Solar's
separate bracer exchange; the witch's five ingredient requests and two
combination finales; the trapper's clothing services; and Rolane's empty
placeholder. It records the captive wife without a local rescue endpoint, the
unconscious templar without a confirmed healing endpoint, and the surrounding
Fomorian violence and survivors as scene content unless an implemented action
adds a real outcome. Source classification does not qualify these journeys in
the active world.

The source inventory also has one complete shop in [long.shp](../../../areas/shp/long.shp#L1):
shop #34438 stocks objects #34471, #34472, and #34473. The coverage row's former
zero-shop count was stale and is corrected to one.

## Directed WLD graph and access boundary

All 100 authored rooms and 251 exit records resolve to active WLD rooms: 249
exits stay within Longhollow and two leave room #34401 for Surface rooms
#520903 and #521302. The internal directed graph has one weak component and one
strongly connected component of all 100 rooms. Starting at forest entrance
#34401 reaches every room and every room has a raw authored route back to it.
The [cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L458) records
Longhollow's north exit to #520903, and [row 459](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L459)
records its west exit to #521302. Surface returns to #34401 on
[rows 1361](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1361) and
[1363](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1363); these are two reciprocal
Surface pairs. There are no active unresolved WLD targets, range-mismatched
source rooms, fall-chance rooms, or current-marked rooms.

The all-95 inactive-WLD sweep also finds one source-only Surf approach:
unregistered surf room #222252 exits south to Longhollow #34401
([surf.wld exit](../../../areas/wld/surf.wld#L466844)). This is not active
ingress. Preserve the route on Surf's existing retained candidate row 97; it
adds one record to the curated Surf and source-only totals without adding a
candidate ([Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)).

The fourteen local D resets set their listed exits open and unlocked
([long.zon lines 8–60](../../../areas/zon/long.zon#L8)). Thus the raw WLD
traversal does not prove arrival, a secret discovery, a closed-door bypass, or
safe travel for a character. Player access, loaded reset state, scripts and
transport, and a live safe-return journey remain unqualified.

## Typed dependencies and source identity

The 213 ZON commands are 105 M, 45 O, 36 G, fourteen D, six F, four E, and
three R records. No active foreign ZON reset references a Longhollow mobile or
object VNUM. No active foreign SHP product, QST giver, or story-sidecar typed
field points to a Longhollow mobile, object, or room. One active foreign QST
does reward Longhollow object #34464: Airship Graveyard's QST has an R item
entry at [airshipgrave.qst line 62](../../../areas/qst/airshipgrave.qst#L62).
That is a foreign producer of the moonstone, not a Longhollow story receipt.
The Longhollow proof delivery independently awards the same local moonstone
along with the bracer.

The active compiled bindings include the epic-teacher table entry for mobile
#34446 and a general mount-skill read of mobile #34429
([epic_skills.c line 174](../../../src/classes/epic_skills.c#L174),
[new_skills.c line 2194](../../../src/classes/new_skills.c#L2194)). The apparent
object #34404 assignment in specs.assign.c is commented out and inactive. The
existing dossier reviews these bindings and explains their relation to the
witch and skeletal horse.

An unregistered source component, temple2, also declares zone #344 and distinct
content in overlapping numeric ranges: 89 WLD rooms #34400–#34488, 30 mobiles
#34400–#34429, and 31 objects #34400–#34430. For example, Longhollow object
#34405 is a token of Selune, while temple2 object #34405 is a black cultist
hood. Its QST and ZON files belong to that inactive source family. These
file-local uses are not active Longhollow dependencies and do not show that
either area replaced the other. Keep the existing temple2 candidate separate;
do not load, merge, move, or renumber the families together without an explicit
owner mapping. See candidate row 102 in
[ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L102).

The unregistered player_guilds shop source has another unresolved VNUM owner:
shop #34402 lists product #34405 at
[player_guilds.shp line 121](../../../areas/shp/player_guilds.shp#L121). That
number names the active Longhollow holy-symbol token and the inactive temple2
black hood. The row remains source-only; no areas/world.shp loader manifest
establishes it as active stock. Preserve it on candidate row 84 until its
intended prototype owner and shop-loader exposure are confirmed.

## Title, history, and attic disposition

After stripping color codes and normalizing whitespace and case, Defense of
Longhollow has one owner among the 351 active .zon contributors. Followed
long.zon, long.wld, long.mob, long.obj, and long.qst history remains on the same
source stem; the reviewed changes show no separate predecessor, title rename,
or whole-area replacement. The old areas-tree move and later source edits are
not a migration claim. The distinct inactive Home of the Enlightened source
has its own tracked temple2 family and overlapping VNUMs, but no evidence
establishes it as Longhollow's predecessor or replacement. See
[DUPLICATE_AND_REPLACED_ZONES.md](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-266-defense-of-longhollow-title-and-source-history).

Keep active long DO NOT MOVE. Preserve the two reciprocal Surface boundaries,
the source-only Surf approach, the Airship Graveyard moonstone producer, the
compiled teacher/mount bindings, the local shop record, and the exact item and
mobile owners. No new attic candidate is added for Longhollow. Keep temple2
retained as its separate existing candidate while its source identity and
tools remain unresolved; its matching numbers make blind activation, merge,
or renumbering unsafe.

## Builder and owner questions

These are direct questions for the area builder or relevant code/source owner.
The recommended resolution records what must be decided before changing source
content; it does not authorize those changes.

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Who owns the overlapping #344xx identities in active long and inactive temple2, and are the areas intended to remain separate? | [long zone header](../../../areas/zon/long.zon#L1), [temple2 zone header](../../../areas/zon/temple2.zon#L1), [temple2 candidate row 102](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L102) | Keep the inactive source isolated; do not co-load or renumber either family until the owner identifies an authoritative mapping. |
| 2 | Should source-only player_guilds shop #34402 product #34405 mean Longhollow's holy symbol or temple2's black hood? | [shop product](../../../areas/shp/player_guilds.shp#L121), [Longhollow object](../../../areas/obj/long.obj#L73), [temple2 object](../../../areas/obj/temple2.obj#L45) | Preserve the row on candidate 84 and identify its source owner and loader exposure before treating it as active stock. |
| 3 | Are both Surface entries intended as reciprocal approaches to forest entrance #34401? | [Longhollow exits](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L458), [Surface returns](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1361) | Preserve both pairs and confirm the intended approach paths with an in-world route review. |
| 4 | Should the asymmetric grass and briar connections remain as authored where prose suggests a different direction? | [room and exit inventory](../../../areas/wld/long.wld#L18), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-266-defense-of-longhollow-directed-wld-graph) | Keep current topology until a builder walkthrough resolves each prose and direction mismatch. |
| 5 | Are all fourteen reset-controlled doors meant to load open and unlocked despite temple, boulder, briar, and mansion prose? | [D reset records](../../../areas/zon/long.zon#L8) | Confirm the intended access model; do not infer keys or bracer prerequisites from description text. |
| 6 | Should Harv's shop operate all configured hours while his QST dialogue says he must be on his way or the shop is closed? | [shop #34438](../../../areas/shp/long.shp#L1), [Harv QST block](../../../areas/qst/long.qst#L191) | Align hours and dialogue with the intended service schedule while preserving current product identities. |
| 7 | Is the mayor's captive wife meant to have a rescue outcome? | [existing captive-wife source review](../zone-stories/DEFENSE_OF_LONGHOLLOW.md#access-custom-support-and-quest-like-orphans) | If intended, define a release, escort, arrival, failure, and completion event before crediting rescue. |
| 8 | Is the unconscious templar meant to have a player-driven healing outcome? | [templar scene review](../zone-stories/DEFENSE_OF_LONGHOLLOW.md#access-custom-support-and-quest-like-orphans) | Credit aid only after a defined successful aid event and terminal recipient state. |
| 9 | Should Rolane's empty native Q block be retired or given an explicit prayer outcome? | [Rolane Q block](../../../areas/qst/long.qst#L95) | Retire it explicitly or define an exact offering, reward or state change, and success text; do not award completion from the empty loop. |
| 10 | Is there an intended world-state endpoint that ends the siege after the five-head delivery and Solar exchange? | [mayor and Solar contracts](../../../areas/qst/long.qst#L126), [mayor proof contract](../../../areas/qst/long.qst#L300) | Keep the two existing hand-ins distinct; add siege closure only after a concrete world or player outcome is approved. |
| 11 | Should the bracer remain the exact Solar offering when the mayor also awards moonstone and another zone can produce that moonstone? | [mayor reward line](../../../areas/qst/long.qst#L339), [Solar contract](../../../areas/qst/long.qst#L126), [foreign moonstone reward](../../../areas/qst/airshipgrave.qst#L62) | Preserve the bracer gate and keep the foreign moonstone receipt separate unless the builder approves a different story contract. |
| 12 | How should the four-skin boots recipe work when each snake mob can produce only one skin under a live object cap of one? | [boots contract](../../../areas/qst/long.qst#L267), [snake-skin object](../../../areas/obj/long.obj#L189) | Ask the builder to approve a sufficient cap or a committed gathering design while retaining the stated four-skin cost until changed deliberately. |
| 13 | How should the two-fur scarf recipe work when red fox fur has a live cap of one? | [scarf contract](../../../areas/qst/long.qst#L285), [fox-fur object](../../../areas/obj/long.obj#L253) | Approve a cap of at least two or an explicit collection design; do not silently change the hand-in count. |
| 14 | How should Viper's Delight obtain two venom sacs when the ordinary reset cap is one? | [Viper's Delight contract](../../../areas/qst/long.qst#L505), [venom-sac object](../../../areas/obj/long.obj#L90) | Approve a source/cap repair or a deliberate escrow path; preserve exact ownership and replay behavior. |
| 15 | Should the witch and boots continue to compete for the same snake skin? | [witch and boots contracts](../../../areas/qst/long.qst#L267), [Viper's Delight contract](../../../areas/qst/long.qst#L505) | Decide whether competition is intentional and document supply expectations before adjusting reset counts or recipes. |
| 16 | Is the boots payment meant to be 2,000 copper as implemented or 1,500 as described to the player? | [boots contract](../../../areas/qst/long.qst#L267), [trapper dialogue](../../../areas/qst/long.qst#L221) | Confirm one price and align the displayed and accepted amounts. |
| 17 | How should the four clothing offers that require both coins and items settle under active accounting? | [trapper service contracts](../../../areas/qst/long.qst#L261) | Keep fees unavailable until the coin and item settlement is durably supported, or approve a coordinated implementation and recovery path. |
| 18 | Should the witch's fixed potion values replace the darkvision, variable-batch, and weapon-coating claims, or are those effects intended gameplay? | [potion and poison review](../zone-stories/DEFENSE_OF_LONGHOLLOW.md#advertised-potion-poison-effects) | Choose corrected descriptions or an approved durable effect design; do not infer mechanics from the recipe prose. |
| 19 | Is the Kiss of Talona output intended to be a wearable armor item or an applied poison? | [Kiss of Talona contract](../../../areas/qst/long.qst#L554), [glowing leaf prototype](../../../areas/obj/long.obj#L675) | Confirm output identity and use before changing its item type, application rules, or story credit. |
| 20 | Are all ingredient requests intended to match independently, or should the witch require a strictly ordered experiment history? | [witch request and finale contracts](../../../areas/qst/long.qst#L410), [source progression review](../zone-stories/DEFENSE_OF_LONGHOLLOW.md#classification-and-progression) | Preserve current independent matching unless the builder approves an explicit prerequisite and its lost, gifted, spent, and replay policies. |

