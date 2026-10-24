# Lava Springs: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Lava Springs
source dossier](../zone-stories/LAVA_SPRINGS.md). It reconciles active and
source-only WLD edges, raw directed reach, typed reset dependencies, compiled
references, title ownership, and bounded same-stem history. Static source
review does not establish that a player can traverse a route or complete an
interaction.

## Active WLD boundaries and directed reach

Active `lava`, zone 997, has 65 rooms from #99700 through #99764. Its 147
authored exits all resolve: 146 are local and one leaves the area. The local
map is one weak component. Raw directed traversal from #99700 reaches all 65
rooms, and all 65 have a path back; no internal exit has flag 8.

Its only active WLD boundary is reciprocal with Surface: Lava #99700 south
to Surface #565027 and Surface #565027 north to Lava #99700
([pair rows 243 and 531](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L243), [Lava
exit](../../../areas/wld/lava.wld#L1), [Surface room](../../../areas/wld/surface.wld#L1365051)).

## Inactive WLD sweep

All 95 current WLD sources outside the active areas/AREA list parsed cleanly:
527,791 rooms and 2,080,773 exits, with no room-VNUM overlap with Lava. One
non-overlapping source-only exit targets Lava: `surf.wld` room #259346 north
to #99700 ([source room](../../../areas/wld/surf.wld#L1245318)). It is not
active ingress. Curated source-only totals rise to 3,335 overall, 166 for the
reviewed Surf subset, and 15 for `ud_map2`.

## Exact typed and compiled dependencies

Lava's active reset file contains two exact cross-area object references:

| Source | Typed reference | Target source and status |
| --- | --- | --- |
| Lava ZON line 105 | O reset places object #3097 in Lava room #99725. | Active `verz1.obj` defines #3097 as a counter ([reset](../../../areas/zon/lava.zon#L105), [object](../../../areas/obj/verz1.obj#L1199)). |
| Lava ZON line 180 | G reset gives object #31318 to Voinak mobile #99713 in room #99759. | Active `dream.obj` defines #31318 as the strange crystal skull ([reset](../../../areas/zon/lava.zon#L180), [mobile](../../../areas/mob/lava.mob#L197), [object](../../../areas/obj/dream.obj#L259)). |

Both references are active reset dependencies. They are separate from the
commission's item contract.

The selected all-zone QST, SHP, sidecar, and reset-role scans found no exact
external reference to a Lava room, mobile, or object. Lava's own selected
QST, SHP, and story fields contain no typed item or prototype reference
outside its local object set. The 231 other story sidecars parsed with no
numeric-token hit into Lava. No external duplicate mobile or object
prototype, or external type-25 target into Lava, was found among 872 scanned
type-25 declarations.

Compiled references resolve to distinct prototype namespaces:

| Compiled source | Typed reference | Static context |
| --- | --- | --- |
| `chaos_eq_data.h` | Object #99700 and object #99702 | Chaos equipment tables contain both objects ([#99700](../../../src/account/chaos_eq_data.h#L83), [#99702](../../../src/account/chaos_eq_data.h#L174)); local prototypes are a flaming cinder longsword and a fiery Hades chain ([longsword](../../../areas/obj/lava.obj#L1), [chain](../../../areas/obj/lava.obj#L29)). |
| `chaos.c` | Room #99715 | The CHAOS destination aliases include Lava Springs, and the route resolves the mapped VNUM as a room ([mapping](../../../src/combat/chaos.c#L37), [resolution](../../../src/combat/chaos.c#L64)). |
| `specs.assign.c` | Room #99715 | The Forgotten Inn room receives the compiled inn function ([assignment](../../../src/specs/specs.assign.c#L2309), [room](../../../areas/wld/lava.wld#L364)). |
| `tradeskill.c` | Mobile #99716 | The smith table lists this mobile and binds smith handling through the mobile index ([entry](../../../src/economy/tradeskill.c#L208), [binding](../../../src/economy/tradeskill.c#L959), [mobile](../../../areas/mob/lava.mob#L243)). |

These are source bindings, not evidence that a code-driven route or service
is reachable in a running game.

## Title and bounded history

The normalized title “Lava Springs” belongs only to active stem `lava` among
the 442 scanned `.zon` files ([title source](../../../areas/zon/lava.zon#L2)).
Followed `lava.wld` and `lava.zon` histories reach the 2009 areas-tree move
and later same-stem edits, including 2013, 2025, and 2026 changes. The
reviewed paths establish no alternate-stem rename, distinct predecessor, or
whole-zone replacement. See the [duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active `lava` DO NOT MOVE. Preserve its reciprocal Surface boundary,
active resets to Verz1 object #3097 and Dream object #31318, compiled Chaos
equipment/destination, inn and smith bindings, and the source-only Surf
approach. Keep `surf` RETAIN IN PLACE and add the approach to its existing
candidate row. No new candidate row is added. Editor, administrative,
installed-data, persisted-player, indirect-runtime, and actual traversal
closure remain outside this bounded scan; no gameplay, migration, or source
change is proposed.

## Builder questions and recommended resolutions

The existing source dossier contains forty owned integration follow-ups. These
direct questions surface the intended choices for builders while preserving
the native commission, supplied-item eligibility, typed outside services,
and boundaries between story prose and persistent outcomes.

| Question | Source evidence | Recommended resolution |
| --- | --- | --- |
| Should Drembel's QA14 remain one six-item commission for five exact rocks #99701 and one voucher #99706, returning Cinder #99700? | QA14 has five rock goals and one voucher goal; the dossier records one accepted completion and no D retirement ([commission](../zone-stories/LAVA_SPRINGS.md#native-commission-and-material-ownership)). | Preserve the exact recipe and single local receipt. Add other services or story stages only as separately owned contracts; do not create prerequisites from greeting, hunt, purchase, or foreign quests. |
| Do the eight aliases on Drembel's response represent one addressed answer or eight accomplishments? | The MA block has eight aliases in one room-wide response, with A controlling the echo rather than separate item goals ([commission](../zone-stories/LAVA_SPRINGS.md#native-commission-and-material-ownership)). | Keep aliases as response vocabulary for one interaction. Do not create one achievement per alias or award every listener a personal response receipt. |
| Must the five rocks be recovered from the listed para-elementals, or may a player offer matching supplied rocks? | The native recipe accepts five exact loose rock objects, and the dossier states that supplied exact materials fit without personal hunts ([materials](../zone-stories/LAVA_SPRINGS.md#native-commission-and-material-ownership)). | Preserve supplied-item eligibility. Keep source actor, recovery, kill history, custody, and commission acceptance as separate facts unless a builder explicitly chooses a provenance requirement. |
| Should follower and equipment-parent resets establish that the player personally earned a rock? | Five source declarations have parent and cap behavior; F changes the equipment parent to a follower and does not establish party credit ([owned follow-ups](../zone-stories/LAVA_SPRINGS.md#owned-integration-and-fair-repair-follow-ups)). | Preserve the exact producer, parent, root, and reset generation. Do not infer player's kill or contribution from a follower's ownership or a room reset declaration. |
| Does Drembel's recommendation to buy Fonlak's voucher make a personal purchase mandatory? | QA14 accepts the exact voucher but has no purchase-history lock; Fonlak carries voucher stock and operates a local shop ([voucher and shop](../zone-stories/LAVA_SPRINGS.md#voucher-purchase-access-and-terrain)). | Keep buying and offering as separate paths. A supplied exact voucher remains valid unless the builder explicitly defines a separate purchase-based contract. |
| Is 500,000 coins the guaranteed price for voucher #99706? | That amount is configured while the actual sale can vary with charisma, race, barter, and epic policy ([shop review](../zone-stories/LAVA_SPRINGS.md#voucher-purchase-access-and-terrain)). | Describe it as configured nominal cost, not an invariant price. Any purchase receipt should use the admitted transaction's resolved stock, buyer, seller, price, currency result, and save outcome. |
| How should voucher purchase behave under active accounting when the native purchase caller refuses mutation? | The dossier records authority refusal before purchase mutations and identifies retained payment, stock, delivery, and recovery decisions ([accounting and integration follow-ups](../zone-stories/LAVA_SPRINGS.md#owned-integration-and-fair-repair-follow-ups)). | Preserve refusal and economic balance until an owner approves a separate purchase integration. Do not provide a free voucher or bypass the shop's admitted continuation. |
| Should the commission accept the six exact objects as one all-or-nothing exchange, and how should partial offers recover? | The active selector chooses distinct actor-carried roots for repeated goals, keeps incomplete sets on the player, and submits all six inputs in one batch ([commission analysis](../zone-stories/LAVA_SPRINGS.md#native-commission-and-material-ownership)). | Preserve distinct root identities and the single receipt. Qualify partial offers, replay, contributor and recipient policy, consumption, capacity, and save/recovery separately. |
| What durable outcome should the Cinder exchange record, and does losing Cinder undo it? | Acceptance awards one Cinder root without a D retirement; the dossier separates reward custody from receipt history ([commission analysis](../zone-stories/LAVA_SPRINGS.md#native-commission-and-material-ownership)). | Keep the committed receipt separate from current Cinder possession. Record actual recipient, reward issuance, revisions, save, and replay; losing the reward must not erase the accepted commission. |
| Should the old Surf edge #259346 north into Lava count as a current arrival? | The active manifest selects reciprocal Surface rooms #565027/#99700; inactive surf.wld #259346 points to Lava as a source-only approach ([active boundary](#active-wld-boundaries-and-directed-reach), [inactive sweep](#inactive-wld-sweep)). | Keep the active Surface pair as the selected route and Surf's existing candidate as RETAIN IN PLACE. Do not count the inactive file as active ingress or add another candidate. |
| Should the twelve reset-closed local door pairs be described as keyed or blocked routes? | Twenty-four D rows close twelve ordinary pairs, local exit key fields are zero, and selected hidden-door exceptions have separate source declarations ([terrain and doors](../zone-stories/LAVA_SPRINGS.md#voucher-purchase-access-and-terrain)). | Preserve each reset and exit flag. Define opening, crossing, and return separately; do not invent keys or clear closed/secret state from terrain prose. |
| Does entry to the crater and tubes guarantee a safe crossing or surviving return? | Their sectors dispatch fire behavior; ordinary actors can take damage, die, or have the event rescheduled, with no authored E/F fall or current rule ([terrain review](../zone-stories/LAVA_SPRINGS.md#voucher-purchase-access-and-terrain)). | Preserve terrain and actual event effects. Do not infer a safe route from the names, scheduled event, or a protective description; qualify admission and survival separately. |
| May an escort or service route into the throne room bypass its restrictive flags? | Room #99759 has NO_MOB, NO_TELEPORT, NO_SUMMON, TWILIGHT, and BLOCKS_SIGHT; room #99700 also has DARK, NO_MOB, and SINGLE_FILE ([terrain review](../zone-stories/LAVA_SPRINGS.md#voucher-purchase-access-and-terrain)). | Keep current flags and admission checks. Define legal escort routes and eligible beneficiaries before claiming a rescue or granting a shortcut. |
| Does rent at the Forgotten Inn make room #99715 a safe storage or protection state? | #99715 has ROOM_INN and an explicit inn binding; the selected helper updates home and saves after rent before extracting the actor ([service review](../zone-stories/LAVA_SPRINGS.md#separate-skull-services-and-wider-progression)). | Keep rental, home update, extraction, regeneration, storage, and lasting safety distinct. Claim only the outcome confirmed by the admitted service path and saved state. |
| Should the foreign Verz1 counter object #3097 be treated as a local bank or shop? | Lava resets the Verz1 object into #99725; its compiled hook handles ENTER locker, with paid-service and account admission ([typed audit](#exact-typed-and-compiled-dependencies), [service review](../zone-stories/LAVA_SPRINGS.md#separate-skull-services-and-wider-progression)). | Keep the object, stock owner, and locker handler under Verz1's typed ownership. Do not count it as a Lava shop or advertise banking without a successfully admitted and saved locker action. |
| Should Durka's fireweed and replica guards create a hire service or new local contract? | Durka carries fireweed and a replica; follower replicas also appear with Drembel and Fonlak, but no separate local shop or Q contract is declared ([wider progression](../zone-stories/LAVA_SPRINGS.md#separate-skull-services-and-wider-progression)). | Treat side-hustle, hire, escort, and liberation outcomes as unimplemented until owners define service admission, agency, beneficiary, result, and persistence. |
| Does the elixir's name promise healing, or does QUAFF apply the configured aging effect? | The elixir is configured at level 41 with AGE 75; selected spell and command paths mutate birth time and have refusal, spill, and consumption branches ([effect review](../zone-stories/LAVA_SPRINGS.md#separate-skull-services-and-wider-progression)). | Describe the configured effect accurately. Do not promise a cure; qualify actual use, recipient, effect, consumption, and save, and make any name/effect correction a separate balanced change. |
| Does Voinak's skull #31318 create a Lava completion or an additional local quest reward? | Dream Q46 consumes five distinct skulls #31316–#31320; Lava supplies #31318 through Voinak, while the recipient and receipt belong to Dream ([skull review](../zone-stories/LAVA_SPRINGS.md#separate-skull-services-and-wider-progression)). | Keep the Dream exchange and discovery under Dream ownership. Skull custody or defeating Voinak alone is not Cinder acceptance or the foreign five-skull receipt. |
| Should town defense, hired guards, or the undead and demonic homes become one required campaign? | Invaders, replicas, occupied homes, and restoration language support several possible stories, but their lasting outcomes need builder choices ([wider progression](../zone-stories/LAVA_SPRINGS.md#separate-skull-services-and-wider-progression)). | Keep these as optional story leads until each branch has an owner, prerequisites, contribution rules, actor and recipient, and persistent result. Do not gate QA14 retroactively. |
| What source availability and renewal should a builder promise for capped reset items? | The area header is mode 1 with lifespan 20–30 and active authority guards item issuance; declarations and caps do not prove current stock or renewal ([accounting](../zone-stories/LAVA_SPRINGS.md#accounting-and-qualification)). | Define generation, source ownership, current availability, renewal, refusal, and recovery before enabling daily or repeatable play. Keep daily policy disabled until admitted sources and saves qualify. |
