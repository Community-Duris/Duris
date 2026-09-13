# Native artifact migrations

These opt-in adapters belong to #296 and use the runtime and physical-UID mana
authority described in [ITEM_ACTIONS.md](ITEM_ACTIONS.md) and
[ARTIFACT_MANA.md](ARTIFACT_MANA.md). The tracked templates are source pilots;
their presence is not evidence that they are obtainable in the live world.

## Configuration and resource contract

`itemActions.enabled` and `itemActions.mana.enabled` must both be one for paid
powers. Each pilot additionally requires `itemActions.artifact.<vnum>.enabled=1`.
All shipped pilot flags are zero. Per-template properties are:

| Property | Contract |
| --- | --- |
| `manaCost` | Positive integer in thousandths of one mana point; default zero makes an enabled pilot unavailable. |
| `manaCapacity`, `manaRegen`, `passiveFloor` | Integer units, units/second, and units respectively; each bounded at 10,000,000. Capacity must cover cost and floor cannot exceed capacity. |
| `manaRevision` | Positive profile revision. Changing a resource field requires a higher revision; existing reserve settles and clamps without filling. |
| `windupPulses` | 4–600, default 8; the common runtime's reaction floor and maximum also apply. Ignored by the synchronous ioun intercept. |

Profile identity is the template vnum; individual reserve is keyed by item UID.
Every accepted magical activation pays once, after common admission, and retains
that cost on cancellation. Native curse penalties and equipment stats do not
depend on a positive balance. Invalid settings suppress the migrated power.
Master off or a valid individual off setting restores its legacy route. Settings
changes cancel that template's pending actions; neither re-enable nor custody
changes revive them. No production balance preset is enabled here.

## Tsunami: 31514, `SeaKingdom_Tsunami`

| Ability | Legacy | Migrated contract |
| --- | --- | --- |
| `tap tsunami` / `tap trident` | Immediate group vitality; timer 0, 300 seconds. Existing vitality could accidentally refresh on unrelated room occupants. | Active 8-pulse windup with a progress beat, one configured mana cost and the same cooldown. Capture wielder level. Re-enumerate current room identities at release; both initial application and duration-15 refresh require self or the same non-null group. |
| `thrust` / `raise` | Timer 1, 500 seconds, a one-second room event, terrain-dependent knockdown rolls. No wave damage. Trusted wielders bypass the cooldown. | Active windup and progress, one configured mana cost, 500-second cooldown for every wielder. Thrust requires dry footing; raise requires water or lack of footing. Recheck terrain, mobility, ownership and magic permission. Re-enumerate legal hostile targets at release and retain the native water/footing/air knockdown rolls. |
| Periodic hum | Native presentation. | Retained immediately, no mana debit. |

The source must remain in WIELD. Native group spell callbacks are followed by
identity reacquisition before the next recipient; the bounded room list contains
IDs, never saved creature pointers. Departure, source removal, abort and disable
discard the effect while retaining mana and cooldown. Command filtering precedes
string parsing, so combat payloads cannot be interpreted as command text in new
mode. Intentional changes are the reaction window, shared mana, filtered refresh,
per-target offensive legality and removal of the trusted cooldown bypass.

## Mirrored ioun: 922, `deflect_ioun`

The equipped GOT_NUKED call in `spell_damage` precedes the incoming hit. Returning
true suppresses that hit. The adapter therefore runs synchronously through
`resolve_item_interception`, not through a timer. It retains the callback's
one-in-four selection and chooses uniformly among legal, non-trusted alternative
room targets. It copies damage, attack type and flags, pays the configured
passive cost, then invokes native `spell_damage` with `SPLDAM_NODEFLECT`. The
damage-message pointer is borrowed only for this synchronous call.

Cold/failed storage, conservation floor, insufficient reserve, common source or
wielder caps, no eligible target and invalid permissions grant no interception:
the incoming hit continues. There is no free legacy fallback after new-mode
failure. Successful resolution suppresses the original hit even if reflected
damage extracts the defender or recipient; the adapter uses no pointers after
the native damage call. The shared runtime retains the source reservation during
that call, preventing recursive use of the same item. Intentional changes are
shared mana, conservation and exclusion of unlawful bystander redirects.

## Living necroplasm: 67243, `living_necroplasm`

The periodic native equipment lifecycle remains immediate: crawling onto a
non-trusted PC, class eligibility, refusal of other equipped artifacts, removal
from HOLD, replacement of body equipment, NODROP, nausea HP loss and curse rolls.
Base equipment stats and those penalties have no mana requirement. New mode fixes
the centaur body-slot fallthrough and checks actor/source identity after equipping,
because an equipment enchantment can invoke a native spell.

Only the magical transformation uses a paid passive windup. Neither vampire nor
angelic form may already be present, the source must remain equipped outside HOLD,
and no other artifact may be equipped. The native level-55 vampire spell retains
its pet/order and memorized-slot behavior. Newly created vampire or angelic affects
are linked to this physical source with the existing object-affect link mechanism.
They are not saved as detached character effects. Ordinary save-time unequip keeps
the live links, while real removal, transfer or extraction removes the owned grant.
Natural expiration requires a new paid activation. A restart requires reacquiring
the grant without refilling the persistent item reserve.

Disabling or changing the pilot settings also removes its linked form. Unrelated
vampire/angelic effects are neither adopted nor stripped. If a transition occurs
inside the native spell before links can attach, its newly created form is removed.
The link-removal primitive now respects the requested break flag throughout the
list and emits expiry text before freeing an affect, including middle entries.

## Mayhem / Symmetry: 21 / 22, `good_evil_sword`

The native swords share a bounded, item-owned state machine. Every selected bundle
contains at most three stages, pays their summed cost once, and remains passive
alongside ordinary character casting. The source must remain in the primary weapon
slot. The initial mode is off, independently for each vnum. Positive stale legacy
energy cannot route an enabled sword through packed weapon spells.

Equipment configuration, automatic primary equipping, NODROP, race-war rejection,
and class-dependent weapon stats remain native. These are immediate and do not
consume mana. The normal one-hand configuration remains 5d5, +5 hit/damage, weight
7; the alternate two-hand configuration remains 6d6, +6 hit/damage, weight 15.
Wrong-race rejection still costs 100 HP and makes the sword disappear. The callback
now returns immediately after disappearance, avoiding use of an extracted source.

| State or trigger | Migrated contract |
| --- | --- |
| Ordinary periodic combat | Preserve Mayhem's random selection from 15 powers and Symmetry's ordered cursor. Optional missing shield, optional missing stone skin and one combat power form a single paid bundle. |
| Ordinary periodic defense | Preserve the one-in-fifteen selection, Mayhem's random choice and Symmetry's five-state cursor. A cursor left by combat is reduced into the defense range, fixing the legacy out-of-bounds index. |
| Shield / skin | Level 55 native fire shield for alignment between -900 and 900, otherwise soul shield; level 55 stone skin with timer 0 and a 10-second interval. Each costs one base unit. |
| Opposing sword in the room | Legal, non-trusted nemesis is captured. A four-base-cost challenge winds up, prepares level-60 blur and deflect on both participants, then starts the native mutual attacks. Timer 1 limits accepted challenges to one per ten seconds. |
| Already fighting the nemesis | Preserve the command compulsion against disengagement. Reactive notifications select deflect preparation, an ordinary combat power or a three-to-five-hit flurry. Deflect costs one base unit; a flurry costs four. |
| Leaving the nemesis state | Clear the item flag and retain the native immediate self-dispel penalty. No reserve credit. |

All preparations normally take eight pulses with a midpoint progress message. Nova
uses at least 32 pulses (eight seconds), preserving its native minimum preparation
time while replacing its unbounded random repeat timer with one cancellable owned
windup. At release it calls the extracted native `resolve_nova` payload, retaining
sunray, native PC/NPC level scaling, area selection and configured nova hit chances.
Ordinary `spell_nova` retains its existing event and randomness in legacy mode.

Combat payloads are accounted for individually below. A base cost means the positive
configured `manaCost`; these multipliers do not prescribe a production budget.

| Cursor | Native effect retained | Cost multiplier |
| --- | --- | --- |
| 0 | Visual dazzle, no mechanical payload | 1 |
| 1 | Blindness 60, spell call, temporary +15 saving-throw adjustment | 1 |
| 2 | Curse 60, spell call, temporary +15 saving-throw adjustment | 1 |
| 3 | Bigby's crushing hand 60, spell call | 2 |
| 4, 9, 13 | Drain redesigned as described below | 1 |
| 5 | Heal 55 for self/current group, then heal 20 for the actor | 2 |
| 6 | Bigby's clenched fist 60, spell call | 2 |
| 7 | Immolate 60, native call type zero | 1 |
| 8 | Earthquake 60, native spell call and terrain rules | 2 |
| 10 | Stornog's spheres 56 for self/current group | 2 |
| 11 | Poison 30, spell call, temporary +15 saving-throw adjustment | 1 |
| 12 | Holy / unholy word 60, native call type zero and alignment gate | 2 |
| 14 | Native nova payload after the owned longer preparation | 4 |

Defense cursor 0 costs three base units and uses Stornog's spheres 60 or stone skin
45 with the original conditional roll. Cursors 1–4 cost one each: native group-heal
arithmetic, vigorize critical 50, the five elemental protections 50 in native order,
or armor then bless 50. Heal falls back to vigor when no eligible recipient is
wounded. Group effects require self or the same non-null group, fixing accidental
buffs on unrelated ungrouped bystanders. Armor/bless now affect the eligible
recipient rather than repeatedly targeting the actor. The group helper powers
also work for the ungrouped wielder; the legacy group-list helpers did nothing
without a group. Recipient order follows the bounded current-room identity list,
with the actor first for group healing; legacy group-list traversal order is not
claimed as identical.

The drain is an explicit retune: replace direct, unguarded HP subtraction and
uncapped self-healing with native 50-point negative damage, NODEFLECT, and healing
capped by damage reported, the target's pre-hit remaining life and the actor's
maximum HP. It never regenerates sword mana. The old combat +100 energy, drain
+100 energy, trusted/nemesis 10,000 refills, periodic three-energy loss and
negative-energy hunger punishment/refill are retired in enabled mode. Passive
regeneration is the only recovery mechanism. None of these energy changes apply
when the pilot is disabled.

Cursor/cooldown changes occur only after successful payment and mark equipment
dirty. Cancellation retains the payment and those transitions. Shared UID mana
persists independently of object/player saves; the cursor and native timers use
ordinary equipment persistence. Native aftermath and base-stat penalties have no
positive-balance requirement. Flurries capture a bounded hit count in the action,
recheck identities and opponent state before every hit, and never use the legacy
global recursion counter. Reactive callback returns remain false: preparing
deflect or applying aftermath cannot intercept the incoming hit. Deflect's normal
subsystem may protect later hits after its preparation completes.

## Reproducible balance decision

The test profile is an experimental fixture, not a live setting: 10 MP capacity,
1 MP base cost, 0.1 MP/second recovery, zero conservation floor, eight-pulse normal
windup. `test_sword_actions_runtime.py` starts with 10 MP and selects blindness
every three seconds for sixty seconds. Fifteen of twenty selections release;
the projected reserve at the immediate hostile encounter is 1 MP. A 2 MP crushing
hand is suppressed. Ten more seconds recover enough for exactly one hand, leaving
zero. The fixture uses actual scheduler and resource arithmetic with controlled
spell/storage boundaries. #297 separately owns disposable-server evidence.

This demonstrates the intended depletion constraint and supports keeping the
prototype's relative combat costs (50:100:200 becomes 1:2:4). Drain now has a
positive minimum cost so it cannot finance indefinite activation. Defense retains
its 30:10 relative cost. Actual kill rate, natural proc throughput, class matchups
and production budget selection are not inferred from this single sequence.

## Initial migration wave and remaining source dispositions

The public server wave consists of Avernus (#293), ordinary devices and wonder
(#294), Studio's explicit typed actions (#295), and the five vnums described here.
The private editor integration is stubbed in #295 and assigned to Faemill in #329.
No callback is removed, no area record or live loot table is rewritten, and all new
categories remain off in shipped configuration.

`python3 scripts/artifact_source_inventory.py --check` verifies the checked-in
[source inventory](artifact_source_inventory.json). It follows tracked `areas/AREA`,
parses the actual extra-flags field, excludes the index sentinel and resolves
literal chained callback assignments with the compiler's active configuration.
It reproduces 169 templates, 78 literal bindings, 67 distinct callbacks and 91
without literal bindings. There are 29 golden-token placeholders, including the
short-description typo “Ttken” whose keywords identify a token.

Every record includes native/type/dynamic review paths and a disposition. Genuine
artifacts outside this initial wave retain legacy behavior and require their own
complete effect contract and measured budget before migration. A missing literal
binding does not establish inertness: packed values, device types, affect fields,
equipment enchantments and dynamic Studio/proc-library assignment remain possible.
Token placeholders receive no invented power. The inventory is a source boundary,
not verified live availability or approval to enable all artifacts. Future
per-artifact issues should be opened only with concrete contracts and balance
policies, as required by #296.

## Focused verification

`tests/async/test_native_artifact_runtime.py` compiles the actual Tsunami, ioun and
necroplasm entry points, the native vampire spell, the actual object-link remover,
the owned scheduler and the mana arithmetic model under ASan/UBSan. Controlled
world/storage boundaries exercise legacy/new routing, exact last payment,
insufficient/cold storage, reserve floors, legal targets, terrain, abort,
leave-and-return, removal, configuration changes, linked forms, save-time versus
real unequip, theurgist forms, class/body slots and extraction during effects.
These focused tests do not substitute for the disposable-server rollout evidence
owned by #297.

`test_sword_actions_runtime.py` compiles the actual old sword helpers, callback,
weapon dispatcher and new state machine. It exercises all fifteen combat choices
for both swords, all defense choices, exact cost/floor/cold-storage failures,
legacy payload comparisons, groups, temporary-save restoration, source/actor/
target extraction, wrong-race disappearance, native packed-path exclusion,
nemesis/deflect/flurry behavior, owned nova cancellation and the measured sequence.
