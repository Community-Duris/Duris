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
