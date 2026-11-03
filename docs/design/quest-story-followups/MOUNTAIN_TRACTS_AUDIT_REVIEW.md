# Mountain Tracts of the Untamed audit supplement

**Checkpoint 47 — active and source-only connectivity, exact typed and code
references, title, and tracked-path history.** The existing
[comprehensive source map](../zone-stories/MOUNTAIN_TRACTS_OF_THE_UNTAMED.md)
remains the authority for local story, quest, room, mobile, object, reset,
shop, and travel details. This supplement records bounded cross-area evidence
without changing native sources.

## Active WLD boundary

The active ledger records 15 reciprocal cross-area pairs (30 directed
declarations) and six authored Mountain exits to absent room targets. The
Mountain's 15 outbound declarations occupy rows 342–356 of the
[cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L342), with
their reverse declarations under the corresponding external source areas.
Connectorzones' return at room 53746 is at
[ledger row 674](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L674). Five positive
unresolved exits target rooms 21202, 21204, 21206, and 21208; room
20999's south exit has target −1. The existing source map records that boot
renumbering removes unresolved exits and leaves the isolated rooms and stale
Lava Caves clue for builder review.

Tracked WLD history includes `bf6db2707` (2009), which added the north exit
from room 20973 to Connectorzones room 53746. Its current north/south pair is
recorded at [Mountain row 350](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L350) and
[Connectorzones row 674](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L674). Later
reviewed changes in `fac285bc7` and
`71d731c5b` alter lava-tube room title formatting; they do not change exit
endpoints. The static declarations do not establish traversal, door state,
safety, or return.

## Unregistered WLD target matches

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored exits with zero parse issues. Mountain Tracts has 245 active room
records and 578 active exits; none of the 527,791 unregistered source rooms
reuse a Mountain room VNUM. The only three numeric target matches are all in
unregistered `surf.wld`:

| Unregistered source record | Direction and active target | Evidence |
| --- | --- | --- |
| Room 277280, “A Dark Forest of Tall Evergreens” | West (`D3`) to Mountain room #20951 | [source room](../../../areas/wld/surf.wld#L1621652), [exit](../../../areas/wld/surf.wld#L1621668), [target](../../../areas/wld/surf.wld#L1621671), [Mountain room](../../../areas/wld/mountaintracks.wld#L1101) |
| Room 277864, “A Wooded and Breezy Pasture” | South (`D2`) to Mountain room #21110 | [source room](../../../areas/wld/surf.wld#L1633904), [exit](../../../areas/wld/surf.wld#L1633916), [target](../../../areas/wld/surf.wld#L1633919), [Mountain room](../../../areas/wld/mountaintracks.wld#L5302) |
| Room 280870, “A Grassy Outback Filled with Small Plants” | North (`D0`) to Mountain room #20901 | [source room](../../../areas/wld/surf.wld#L1696962), [exit](../../../areas/wld/surf.wld#L1696966), [target](../../../areas/wld/surf.wld#L1696969), [Mountain room](../../../areas/wld/mountaintracks.wld#L24) |

`surf.wld` is absent from `areas/AREA`, and no unregistered source-room VNUM
overlaps the active Mountain room set. These three records remain
source-only route candidates, not active-world ingress. They raise the
curated source-only total from 583 to 586 and the reviewed `surf.wld` subset
from 23 to 26.

## Bounded typed and code references

An exact scan of active external QST, ZON, SHP, and story-sidecar sources found
one foreign native QST recipe consumer: Winterhaven's Ohnagra exchange accepts
local gloves #20950 and a fresh potion #20949
([glove input](../../../areas/qst/wh.qst#L2203),
[potion input](../../../areas/qst/wh.qst#L2204)). The Mountain dossier
maps the complete exchange and distinguishes it from the four local recipes.
No other active external QST record, reset, shop record, or story-sidecar
VNUM reference to Mountain rooms, mobiles, or objects was found.

The exact C/C++ scan found three static chaos-kit slot values: ordinary boots
[#20924](../../../areas/obj/mountaintracks.obj#L342) appear twice in
[`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L90) and
[`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L342); bone leggings
[#20937](../../../areas/obj/mountaintracks.obj#L543) appear once at
[`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L528). These are
equipment-table values; they do not establish kit selection, creation, or
player issuance. No other direct C/C++ numeric reference to a Mountain room,
mobile, or object VNUM was found. The Mountain source map's bounded scan of
all active type-25 teleport objects found no foreign fixed destination into
the zone.

## Active title and tracked history

The color-stripped active title “Mountain Tracts of the Untamed” is unique in
the current `areas/AREA` list ([manifest](../../../areas/AREA#L108),
[zone title](../../../areas/zon/mountaintracks.zon#L2)). Tracked WLD history
shows the 2009 Connectorzones endpoint addition and later room-title
corrections; no tracked predecessor or replacement is identified in the
reviewed path history. Broader semantic alias and map-identity review remains
pending.

## Scope and disposition

Mountain Tracts remains an active `areas/AREA` contributor. Its 15 reciprocal
active cross-area pairs, six unresolved authored exits, and three unregistered
Surf target matches remain separate evidence sets. Actual access, terrain and
combat admission, door/reset state, safe return, dynamic effects, and
persisted-player outcomes remain unverified. This documentation-only review
did not run tests, a build, server/runtime/gameplay, database, migration, or
operational script.

## Checkpoint 294: all-442 title and builder questions

A normalized scan of all 442 current .zon files finds one exact title owner
for “Mountain Tracts of the Untamed”: active stem mountaintracks. The
[comprehensive source dossier](../zone-stories/MOUNTAIN_TRACTS_OF_THE_UNTAMED.md)
and [Checkpoint 47 supplement](MOUNTAIN_TRACTS_AUDIT_REVIEW.md) retain the
four native exchanges, three source-only Surf approaches, foreign Ohnagra
consumer, typed shop and kit references, travel objects, controls, and
unresolved exits. These questions preserve the native recipes and identify
the decisions still requiring builder intent or gameplay qualification.

1. **Question:** Should mountaintracks remain the authoritative owner of the
   exact title “Mountain Tracts of the Untamed”?
   **Recommendation:** Keep the unique normalized title match across all 442
   current .zon files. Retain broad semantic alias and map-identity questions
   separately; the title does not merge the Banished, Peril Peaks, or other
   mountain-named areas.

2. **Question:** Should the fifteen reciprocal active cross-area pairs and
   three one-way source-only Surf approaches retain their current endpoints?
   **Recommendation:** Preserve the active pairs and keep the three Surf
   records on existing candidate row 97 as inactive source evidence. No new
   route is established by the 95-file scan.

3. **Question:** What should happen to the six authored exits whose positive
   targets are absent, the negative south exit at #20999, and isolated rooms
   #21127, #21128, and #21149?
   **Recommendation:** Keep the boot-time unresolved-exit behavior and ask the
   builder whether these are reserve remnants, stale clues, or intended
   endpoints before adding rooms or reciprocal routes. Do not invent
   #21202–#21208 or an exit from Lava Caves from prose alone.

4. **Question:** Should the four local native recipes remain one exact
   offering per independent receipt?
   **Recommendation:** Preserve carved piece #20923 for leggings #20930,
   marble #20954 for bracer #20955, scale #20947 plus tooth #20948 for potion
   #20949, and fresh potion #20949 for gloves #20950. Keep each no-departure
   contract and the four separate accepted histories.

5. **Question:** Should the monk continue accepting the small wearable carved
   piece #20923 rather than the large stationary statue #20907?
   **Recommendation:** Preserve the hidden floor source, exact prototype, and
   native WORN/TAKE behavior. A worn copy must be removed before transfer;
   statue prose does not create a second proof or a carve/worship objective.

6. **Question:** Should the half-orc mage’s Q remain item-based without a
   personal apprentice kill?
   **Recommendation:** Preserve the exact marble input and accepted bracer
   reward. The mage’s threat and apprentice lore do not add a kill
   prerequisite or create a learning achievement.

7. **Question:** Should Futni’s two-item recipe and Bumble’s fresh-potion
   recipe stay independent when potion #20949 also feeds Winterhaven’s
   Ohnagra exchange?
   **Recommendation:** Keep the foreign Winterhaven outcome separate and
   explain that completing both paths requires another potion copy after
   Bumble’s accepted use. An earlier Futni receipt cannot replace a consumed
   input or earn foreign credit.

8. **Question:** Should the hidden cap-one proof sources remain scarce and
   NORENT as authored?
   **Recommendation:** Preserve the source objects and reset terms while
   qualifying actual generation and recovery separately. A cap or daily
   candidate does not guarantee renewed stock, and supplied valid proof does
   not establish personal first acquisition.

9. **Question:** Should the 19 addressed aliases continue to resolve to four
   response families, with the source-side S-only headers left without a
   recipe?
   **Recommendation:** Preserve the existing Q bindings. Do not add an
   achievement for each alias, turn Putsie or farmer headings into contracts,
   or confuse accepted GIVE with addressed ASK/TELL.

10. **Question:** Should the blank accepted responses be clarified without
    inventing extra lessons, faith, apprentice, or lost-tower endings?
    **Recommendation:** Ask for intended player-facing wording and keep it
    separate from new stateful outcomes. Any authored search or learning
    objective needs its own successful result and recovery authority, not a
    keyword, timer, or descriptive paragraph.

11. **Question:** Should the journal distinguish the potion’s use, transfer,
    consumption, and competing recipe needs?
    **Recommendation:** Explain current admitted quantity and selected use;
    do not reserve an item based on projection alone. Keep QUAFF optional for
    the local exchanges and preserve native combat, timer, and NO_MAGIC
    admission.

12. **Question:** Should PUSH controls record a completed unblock separately
    from opening and crossing the exit?
    **Recommendation:** Keep D8/D9 blocked and closed states as authored.
    Qualify the selected object, target exit, actor, reciprocal state, later
    OPEN, and committed arrival as distinct outcomes. Existing open state,
    a room message, or another actor’s push is not personal traversal.

13. **Question:** Should GRAB remain the current command for the vine route,
    separate from the CLIMB skill?
    **Recommendation:** Preserve GRAB65 and the fixed destination behavior.
    Record actual object selection and arrival before assigning travel credit;
    do not rebind the opcode or treat CLIMB preparation as using the vine.

14. **Question:** Should unplaced type-25 portals and type-29 switches remain
    dormant until their targets and purpose are confirmed?
    **Recommendation:** Keep tower, black-hole, mound, and boulder objects
    unplaced. Resolve their reserve or retirement status before adding resets,
    missing directions, activation, or portable mobility.

15. **Question:** Should the mild-current prose at #20913 and the isolated
    Lava Caves clue at #21149 be reconciled with their actual source controls?
    **Recommendation:** Confirm intended environment and geography, then make
    truthful text or metadata changes in a separate scoped proposal. Do not
    infer a current, fall, route, or safe return from the room description.

16. **Question:** Should Riffleraffle’s existing shop stay separate from native
    quest inputs and receipts?
    **Recommendation:** Preserve the one defined keeper and its priced stock.
    Qualify present keeper, actual stock, price, admission, and transfer before
    describing a trade; shop inventory is not a quest source or contribution.

17. **Question:** Should the two compiled shaman-kit uses of boots #20924 and
    leggings #20937 remain source references rather than proof of player
    issuance?
    **Recommendation:** Keep both kit slots documented as typed object
    consumers. Selection, creation, delivery, and player custody need their
    own evidence before either becomes a recovery prerequisite or journal
    receipt.

18. **Question:** Should Winterhaven’s Ohnagra exchange remain a separate
    consumer of local gloves #20950 and potion #20949?
    **Recommendation:** Preserve the active external QST recipe and its
    separate giver, inputs, and reward. Do not make it a Mountain objective
    or infer its credit from merely carrying the local items.

19. **Question:** What policy should govern cap-one supplies, reset mode 2,
    and any future daily renewal?
    **Recommendation:** Keep actual source generation, legal recovery,
    accepted receipt, reward settlement, frozen recovery, and reset epoch
    distinct. Do not enable a daily promise until renewal and replay are
    qualified without defeating the source scarcity.

20. **Question:** What evidence should qualify a new Mountain story as READY,
    accepted, and persistent?
    **Recommendation:** Bind it to the actual source, selected actor,
    controller result, recipient, durable outcome, and recovery boundary.
    Synthetic projection and source review do not prove played acquisition,
    movement, settlement, database persistence, or replay.

Checkpoint 294 confirms the all-442 title owner and adds twenty direct
builder questions. It preserves the existing active graph and typed closure,
adds no route or candidate row, and claims no played outcome.