# The Lost Temple of the North: source review

Zone 529, source stem multiclass_temple, contains 20 rooms (#52901–#52920), two local mobile prototypes, two local object prototypes, and eight reset commands. The active room graph, cross-area records, object/mobile prototypes, and compiled room procedure were reviewed as static source. This dossier records authored structure and code dispatch; it does not qualify player access, safe traversal, class-change persistence, or live behavior.

The .zon title is “the Lost Temple of the North.” The all-442 normalized title scan finds one exact owner, multiclass_temple. tikitt (“Lost Temple of Tikitzopl”) is a distinct area and title. Both are retained as separate source identities unless an owner establishes a broader naming relationship.

## Source identity and inventory

| Source | Reviewed facts |
| --- | --- |
| [Zone header and resets](../../../areas/zon/multiclass_temple.zon#L1) | Zone #529, reset mode 2, range through room #52920; eight commands: two door-state commands, one object reset, four mobile resets, and one give reset. |
| [Rooms](../../../areas/wld/multiclass_temple.wld#L1) | All 20 room records, their authored descriptions, exits, two 100-percent fall records, and the keyed ice gate. |
| [Local mobiles](../../../areas/mob/multiclass_temple.mob#L1) | Ancient white dragon #52901 and ice golem #52902. |
| [Local objects](../../../areas/obj/multiclass_temple.obj#L1) | Touch teleport object #52901 and ice key #52902. |
| Active registration | [areas/AREA](../../../areas/AREA#L225) lists multiclass_temple *529; [areas/qst/AREA](../../../areas/qst/AREA#L206) also lists it, but there is no local areas/qst/multiclass_temple.qst, shop, or story sidecar. |
| Compiled room behavior | [Assignment](../../../src/specs/specs.assign.c#L2275) binds multiclass_proc to room #52904; [handler](../../../src/specs/specs.room.c#L159) handles the room command. |

## Glacier layout and authored connections

Local WLD exits form three weak components when cross-area edges are excluded:

| Local group | Rooms | Authored role |
| --- | --- | --- |
| Shaft and ice bubble | #52901–#52913 (13 rooms) | Surface shaft #52901–#52903 drops toward the large ice bubble. Room #52904 holds the flux object and multiclass procedure. The ancient white dragon loads at #52913, across a gate from #52904. |
| Western glacier tunnel | #52914–#52917 (4 rooms) | A separate through-tunnel with two Surface boundaries. |
| Southern glacier tunnel | #52918–#52920 (3 rooms) | Another separate tunnel. The descriptions place ice guardians and a carved rune in #52919; no local mobile reset targets that room. The flux object points here as its teleport destination. |

Five active Surface boundary pairs are reciprocal in the WLD source and cross-area exit register:

| Temple edge | Surface return edge | Evidence |
| --- | --- | --- |
| #52901 up → Surface #511126 | #511126 down → #52901 | [Temple boundary register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L651), [Surface return](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1317) |
| #52914 west → Surface #513537 | #513537 east → #52914 | [Temple boundary register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L652), [Surface return](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1329) |
| #52917 east → Surface #513539 | #513539 west → #52917 | [Temple boundary register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L653), [Surface return](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1330) |
| #52918 south → Surface #518740 | #518740 north → #52918 | [Temple boundary register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L654), [Surface return](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1352) |
| #52920 north → Surface #517940 | #517940 south → #52920 | [Temple boundary register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L655), [Surface return](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1348) |

The shaft rooms #52902 and #52903 each carry F 100. The authored graph and reciprocal Surface declarations do not establish that the descent is survivable, that every path is traversable, or that an actor can return safely. The isolated southern tunnel is linked to the main area only by the flux object's item-teleport behavior in current local sources.

## Gate, resets, and item behavior

The gate between #52904 and #52913 is authored as closed. The east exit from #52913 names key #52902; the reverse west exit from #52904 names key 0. The zone applies state 2 to both directions ([reset commands](../../../areas/zon/multiclass_temple.zon#L8)); the room loader interprets state 2/3 as closed and locked ([door reset handling](../../../src/world/db.c#L7857)). The key declaration therefore differs by direction and needs builder intent confirmation.

The eight zone commands load flux object #52901 into #52904; local golem #52902 into #52904; imported ice golem #31402 and crystal #31403 into #52909; ancient white dragon #52901 into #52913; and an ice key #52902 with a G command under the preceding dragon M reset. The object description says the key lies on the ground ([key prototype](../../../areas/obj/multiclass_temple.obj#L15)), while the reset structure gives it to the dragon. Treat the prose/reset mismatch as unresolved source intent.

Flux object #52901 is type 25 with values 52918 320 -1 ...: fixed destination #52918, command 320 (CMD_TOUCH), and a negative charge count. The generic [item-teleport handler](../../../src/magic/spell_travel.c#L932) selects type-25 objects by their configured command and resolves the fixed destination. No local special handler for burn damage was found. The object's extra description warns that touching it burns the actor ([prototype](../../../areas/obj/multiclass_temple.obj#L1)); builder review should decide whether this is intended flavor or an unimplemented effect.

## Multiclass room procedure

The compiled assignment binds the room procedure to #52904. It reacts to CMD_MULTICLASS (702; [command constant](../../../src/cmd/interp.h#L739)); a room-external multiclass command is only the general interface/listing path.

The procedure rejects NPCs and morphs, accepts only Human or Orc characters, and refuses characters already marked multiclass. Its configurable minimum level defaults to 50; the epic-experience minimum defaults to 250. It lists options when the player has no class argument, validates a class abbreviation and can_char_multi_to_class, then calls do_start for an accepted change. It preserves played time, sets the secondary-class bit, clears the character's spec, restores the character, forgets spells, refunds epic skills, and clears the zone trophy. The code does not require a local quest, token, or coin offering. These source operations do not establish that all relevant character state persists or recovers correctly in live play.

## Bounded typed-reference closure

The zone imports two mobile prototypes from the active mobs_surf file: ice golem #31402 and crystal #31403 ([prototype #31402](../../../areas/mob/mobs_surf.mob#L12), [prototype #31403](../../../areas/mob/mobs_surf.mob#L23)). They are actual reset dependencies and should remain visible in any future source or area disposition review.

An exact bounded scan of active external QST, ZON, SHP, story-sidecar, and source-code references found no external QST item consumer, shop product, reset of the local Temple mobile/object prototypes, or typed sidecar item reference. The only direct code assignment for Temple room/prototype numbers was the expected procedure binding at #52904. This closes the static typed scan described here; indirect tooling, runtime use, and persisted character effects were not established.

## Review status

Keep the active contributor in place. No WLD route, archive candidate, migration, or native source change is proposed by this review. The [CP296 audit supplement](../quest-story-followups/MULTICLASS_TEMPLE_AUDIT_REVIEW.md#checkpoint-296-all-442-title-and-direct-builder-questions) records the title/history result, typed closure, and 20 direct builder/owner questions.
