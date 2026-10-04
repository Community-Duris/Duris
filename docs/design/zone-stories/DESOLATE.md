# Desolate: comprehensive source story map

**Source-comprehensive, revision one — October 4, 2026. Gameplay qualification
remains open.** The [journal](../../../areas/story/desolate.story.json) explains
all eleven exchanges as nine story outcomes and two services. Twenty-seven
contacts include the one native addressed family, Jandar's computed teacher
role and the foreign Balance referral. Sixteen optional checks comprise twelve
current-material checks and four earlier exchange histories.

**Shipped native repair:** the Tests of Endurance and Courage now describe their
buttons on the southern and western walls, matching their placed switches and
progression exits. Separate commit
[`b1ff082bc03ae579aceec97b5d3ebd878bf2eea7`](https://github.com/Community-Duris/Duris/commit/b1ff082bc03ae579aceec97b5d3ebd878bf2eea7),
`fix: correct Desolate Master trial button directions`, changes only those two
room descriptions plus its regression test. See the explicit news handoff below.
Missing shop stock, drink-state intent and new payment/trial/phase capabilities
remain proposals, separate from this implemented repair.

Active, ready accounting remains mandatory for discovery, encounters, journals
and new credit. Frozen obligation recovery/persistence remains separate. Current
possession does not distinguish personal recovery from a player handoff. An
accepted exchange does not prove learning a keyword, rescuing a live animal,
opening a route, defeating a guardian, repairing a moving wagon or restoring
Desolate after invasion.

## Evidence and review boundary

Read all twelve [native blocks](../../../areas/qst/desolate.qst): eleven Q and
one addressed M, with no ambient/default families. The
[reproducible source index](../../reference/zone-story-audits/desolate.md)
retains exact recipes, native bindings, aliases, dialogue and declared sources.

Read all 168 [rooms](../../../areas/wld/desolate.wld), physically 22200–22367:
143 exact prose groups, nineteen numeric headers, twenty-two non-exit metadata
groups and 135 exit families. Read all 114
[mobiles](../../../areas/mob/desolate.mob), 22200–22313, all 92
[objects](../../../areas/obj/desolate.obj), 22200–22291, all three
[shops](../../../areas/shp/desolate.shp), and all 400
[reset commands](../../../areas/zon/desolate.zon) in 289 families: M195, E61,
D60, O45, G24, F9, P4 and R2. Both R mount assignments were followed explicitly;
G/E ownership after a mount reset must follow the actual current mobile.

Reviewed the sole literal local assignment, Master armor 22237 → `master_set`,
its direct legacy implementation and separate equipment adapter; computed
Jandar → `teacher`; generic room-inn/shop behavior; shared quest matching,
durable item-tree destruction, rewards/retirement, switches, door/reset states,
entered portals, falls and timed random-exit activation. Bounded foreign review
covers the original letter, its drawer/room/door/source and Balance's actual
Myrabolus referral, alternate Master armor supply, reciprocal surface approaches,
administrative load rooms and Desolate Under Fire's actual destination. It does
not claim the entire foreign zones have been comprehensively audited here.

Every local quest input/reward prototype resolves. Three unrelated trinket
stock references do not; see the pending repairs. No other foreign quest consumer
was found for the local delivery materials or rewards. The letter has the actual
foreign accepted-and-returned referral described below.

## Complete exchange classification

| Interaction | Exact native offering → reward | Journal boundary |
|---|---|---|
| [Tired halfling](../../../areas/qst/desolate.qst#L2) | Ale kind 22230 → C50000 | Story; optional tankard-fill history |
| [Minotaur](../../../areas/qst/desolate.qst#L10) | Chain 22215 → C10000 + iron rod 22220 | Story; chain is inside the monkey |
| [Eclipse mercenary](../../../areas/qst/desolate.qst#L29) | Empty tankard 22229 → full ale 22230 | Free supply service, no achievement/daily |
| [Monkey hunter](../../../areas/qst/desolate.qst#L39) | Monkey container 22214 → mandolin 22269 + C15000 | Story; remove chain before consuming container |
| [Wagon driver](../../../areas/qst/desolate.qst#L49) | Repaired wheel 22251 → brass token 22252 | Story; driver retires; supplied wheel fits |
| [Beregan](../../../areas/qst/desolate.qst#L61) | Hand 22284 + storm badge 22283 → earring 22288 | Story; two distinct proofs together |
| [Myrabolus delegate](../../../areas/qst/desolate.qst#L70) | Armageddon badge 22216 → ring 22286 | Story; different badge and recipient |
| [Scotson](../../../areas/qst/desolate.qst#L81) | Rod 22220 + broken wheel 22231 + C5000 → repaired wheel 22251 | Story stage; mixed payment currently guarded |
| [Travelling merchant](../../../areas/qst/desolate.qst#L92) | C10000 → potion 22255 | Purchase service; guarded coin-only path; retires |
| [Shaggy dog](../../../areas/qst/desolate.qst#L102) | Bones 22211 → collar 22250 | Story; no adoption/escort objective |
| [Seraphim](../../../areas/qst/desolate.qst#L111) | Original letter 82543 → totem 22289 + C10000 | Story; optional foreign referral history |

C1000 is one platinum in these native recipes: the halfling pays fifty, minotaur
ten, hunter fifteen and Seraphim ten; Scotson asks five, merchant ten. Fee and
reward values remain unchanged. Nine native definitions retain potential daily
shape. Free filling is a service, leaving eight authored story candidates. The
mixed wheel repair remains ineligible with `Unsupported durable offering`; the
coin-only purchase has `No repeatable item offering` and no story credit.
Native definitions, identity/fingerprint, revision two and zone registry remain
unchanged. Reset mode two does not guarantee immediate renewed stock, a returned
driver/merchant, a surviving recipient or a safe repeat.

## Ale, the monkey and the stranded wagon

Bar patron 22268 holds empty tankard 22229 in the North Star Tavern. Eclipse
mercenary 22234 starts at 22281, answers `ale`, and fills that exact tankard free.
The halfling at 22274 accepts the full-ale kind. Decorative tankard 22228 is a
different trash item. Full ale 22230 is a drink container; matching in the current
quest path uses vnum, without requiring remaining liquid or a particular drink
state. If builders want a genuinely full drink, add an accepted content predicate
and tests for depleted/different/poisoned drink state before changing semantics.

The monkey is a closed, unlocked item container at bear den 22293, with chain
22215 placed inside it. Remove the chain before returning the monkey. The durable
turn-in captures the selected item tree, and publication recursively extracts the
root and remaining children. A historical monkey receipt cannot recreate a chain
that was still inside. Journal readiness sees loose carried roots, not nested
contents; carrying the monkey does not make its chain ready. This is a concrete
destructive hand-in hazard, not evidence that the accounting path loses custody.

The minotaur at 22259 accepts the loose chain and gives the iron rod. No ordinary
rod reset or other producer was found. Broken wheel 22231 loads on road 22205.
Scotson at forge 22322 combines rod, wheel and five platinum into repaired wheel.
The current durable offering accepts item-only goals; a currency goal makes this
mixed recipe unsupported. The legacy fallback is also guarded while accounting
is active. The merchant's coin-only purchase is guarded by the same active-mode
fallback condition. Do not remove fees or turn accounting off to hide this gap.

Plan one accepted mixed-wallet/item transaction: preflight exact distinct roots,
wallet denomination/value and expected revisions; atomically consume materials,
debit fee, create reward and persist completion/recipient policy; publish only
after commit, with rejection, replay, reconnect and crash recovery proof. Coin-
only purchases need the corresponding commerce route. Avoid separate item debit
and money debit that can leave a partial repair. This proposal does not enable
Scotson or the merchant in this checkpoint.

The driver at wagon 22206 accepts any supplied exact repaired wheel, even without
the guarded smith history, gives the token and leaves. His disappearance is the
native terminal; neither the large wagon object nor its horses receive a repair,
movement or escort action. Optional minotaur → smith → driver histories explain
the chain while preserving supplied materials and independent outcome credit.

## Two threats, the dog and the foreign letter

Three outcasts start at hideout 22342: one M and two F followers. Only the first
F-loaded follower has hand 22284. Captain 22306 wears storm badge 22283 inside
tent 22365; scout 22239 wears armageddon badge 22216 at northern forest 22224.
Beregan needs hand plus storm badge; the delegate needs the other badge. They
are separate kinds, sources and narratives. A matching proof fits without a
personal defeat. Deliveries do not implement an army response or restored peace.

Bones and persistent bronze key 22210 share old garbage container 22209 at 22260.
It is closed/unlocked and trapped. Bones fit the dog; the key opens the separate
locked trophy cabinet 22208 at 22242. Cabinet flags fifteen do not include
pickproof sixteen: the hardpick bit is unused. Key possession is not a dog
prerequisite or successful cabinet access. Dog/hunter/delegate can wander beyond
their load zones; contacts and hints describe origins, not fixed current rooms.

Letter 82543 is inside drawer 82544 in rented room 82579 of Myrabolus' Rising Sun
Inn, behind an ordinary closed, unlocked door. The drawer is also closed and
unlocked despite open-drawer prose. Its extra description names Nansuo as the
addressee and Ajandaner as signatory; keep those roles when interpreting the plot.
Balance 82569 starts at pier 82686. His
[actual referral](../../../areas/qst/mira.qst#L126) accepts and returns the same
letter kind and directs the player to the Seraphim. This is a real foreign-owned
exchange, not an invented local keyword stage or duplicate achievement.

Seraphim 22294 at secret ledge 22367 accepts the original letter. Its Balance
history is optional; supplied original letters fit. A referral receipt cannot
restore a spent letter. The native secret closed trail connects forest 22351 to
the ledge. Current schema cannot prove finding/opening that route or the promised
future use of the information.

## Hidden controls and the Master's trial

Jandar's ACT_TEACHER binds the generic class-sensitive `ask ... level` guidance
when no literal function exists. That role does not open his floor or define a
quest award. Torch 22276 at 22263 uses TOUCH to clear the downward blocked bit;
the secret closed route still needs finding/opening. Soil 22275 below at 22264
uses PUSH to clear the reverse upward block. Secret switch handling clears only
the addressed side; ordinary non-secret trial switches clear the reciprocal
blocked side as well. Rock controls at 22251/22340 and branches at 22285 follow
the same actual distinction. Owning or invoking a control is not proof of arrival.

The Master chamber's closed rooms contain ordinary casket containers and a
special entered casket 22204 at 22273 leading to trial start 22309. Casket 22222
at 22309 returns to 22273. Trial progression is:

| Source | Control | Destination |
|---|---|---|
| 22309, start | PUSH 22223, east | 22310, Speed |
| 22310, Speed | PUSH 22224, east | 22311, Endurance |
| 22311, Endurance | PUSH 22247, south | 22312, Strength |
| 22312, Strength | PUSH 22248, west | 22313, Courage |
| 22313, Courage | PUSH 22236, west | 22314, Will |
| 22314, Will | PUSH 22249, north | 22309, start |

Speed's three rogues load in upstairs side room 22315, linked to Speed. Endurance
has an ethereal knight, Strength a spectral titan, Courage two behemoths and Will
a celestial dragon. Switch acceptance does not check guardian defeat or an
ordered attempt record; ordinary combat/movement restrictions remain relevant.
The last button returns to the start rather than granting a hidden reward. No
native Q trial-entry, five-defeats, full-gauntlet or return terminal was found.

The dragon's Master armor 22237 and other equipment are ordinary loot. Armor
also has a foreign Sorem source. The direct seven-member legacy Master procedure
counts matching worn slots, caps at six and adds effects from two pieces onward;
the separate adapter has a different membership/count policy. Armor acquisition
or one equip does not prove a full set or trial victory. Define accepted scoped
entry/control/pass/defeat/return events and attempt/reset rules before a trial
campaign. Define actual equip/effect/cleanup/recovery endpoints before set credit.

Blue and black jousting knights have actual R-loaded unicorn/nightmare mounts.
The gambler has no active casino binding, and no local wager/joust result contract
was found. Trophy access and themed prose do not implement a tournament finale.
Treat that as an unimplemented story extension, not a demonstrated broken wager.

## Timed invasion route, falls and availability

Random-exit object 22291 may load at northern exterior room 22200 with twenty-
percent reset admission. Loader schedules `event_random_exit` after three pulses.
Its value 100 passes the callback's random check whenever the admitted object
and route are valid. It replaces north's normal destination 22201 with invaded
room 77302, sets the former destination zone's closed flag and clears the target
zone's flag, then extracts itself. The invaded target's south exit is rewritten
back to 22200. Initial invaded-zone flags are closed; this is a real live phase
change, not merely portal lore or a twenty-percent callback probability.

Normal Desolate and Desolate Under Fire have different native owners. Current
tracking uses its static zone catalog and physical arrivals; it does not capture
this control mutation, an episode identity or the flags as a journal prerequisite.
Do not infer restoration, an invasion objective, admission or safe return from
an unrelated normal-zone receipt. Plan accepted world-control/phase transitions,
actual route generation/destination, causal episode and builder-owned cross-zone
campaign terminals, with resets/restart/shared participants and recovery tests.
Desolate Under Fire remains priority 92 for its own comprehensive review.

Two normal surface approaches are reciprocal. Foreign administrative load rooms
are not additional player entrances. Four decline rooms have F90 fields and two
watchtower stairs F20. Loader assigns falling chance; command interpretation
checks that chance and falling eligibility. These values are risks, not room
destinations or guarantees. Actual successful movement, fall survival and return
need qualified events, including ordinary no-magic and equipment constraints.

## Repair reporting, pending work and verification

**Implemented — Master trial button directions.** Looking in Endurance formerly
said eastern while its actual switch and exit were south; Courage formerly said
eastern while its actual switch and exit were west. The descriptions now match
the controls and progression. The focused
[source regression](../../../tests/async/test_desolate_master_trial_directions.py)
fails against the original descriptions and passes after the two edits; it checks
the room prose against actual button values/description, placement and blocked
exit/reset. No exit, switch, guardian, payout or trial rule changes.

**News:** “Desolate's Master trial now gives the correct button directions in
the Tests of Endurance and Courage.” Live look/control/return gameplay remains
unqualified; source consistency is verified.

**Pending native stock decision:** trinket vendor 22290 has shop and G references
to absent prototypes 6070, 6109 and 6110. Reset renumbering disables unresolved
G commands; the shop loader translates positive references to unresolved rnums.
The checked-in stock cannot supply those objects. This does not establish a boot
crash or a broken quest recipe. Confirm intended retired/replacement items, then
restore or remove both shop and reset references in a separate fix with commerce
proof and news. Do not invent objects or remove stock silently during mapping.

**Pending presentation/intent:** closed drawer versus open prose, drink fullness,
garbage/monkey container affordances, collar spelling and whole-campaign endings
need builder decisions. Empty descriptions, ordinary loot, loaded wandering mobs
and guardian/control independence are not automatically defects. Payment support,
nested destructive-offering previews, source/handoff identity, accepted controls,
world episodes, falls, foreign scope and trial/set effects expand the shared plan.

Verification covers exact recipes/categories/fees/rewards, foreign owner, actual
sources and F/R ownership, nested containers, drink-kind matching, control routes,
random-exit admission/activation and missing stock. Actual C++ journal tests cover
encounter visibility, wrong badges, nested/worn/spent proof, supplied branches,
separate history/outcome credit, no read-side mutation, replay and cold recovery.
Production catalog/daily report, accounting gates/tracking, maintained build,
formatting/whitespace and source-line links are checked separately.

Catalog: 73 maps, 1654 achievements, 1471 potential dailies and 2216 projected
rows. All 2668 native definitions, content revision two, fingerprint, registry
and seventy-two prior journals remain unchanged. Live source/container hand-ins,
mixed payment, wandering contacts, trial/phase/fall/return/set and renewed stock
journeys remain unqualified. Roadmap: 52/220 source-comprehensive, 168 pending;
Rift Valley Jungle is next. No database/account/server operation, generated-world
edit, accounting activation or merge occurred. The full roadmap remains active.
