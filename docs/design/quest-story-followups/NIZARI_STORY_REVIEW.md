# Nizari: source story review

**Scope:** static review of Nizari's local QST dialogue, room and mobile text,
objects, resets, special handlers, local topology, file history, and typed
cross-area references. No live route, quest, reset, or shop behavior was tested.

## Source identity and story coverage

Nizari is an active full-area contributor in range 400–401
([area list](../../../areas/AREA#L191)). Its source defines 192 rooms, 26
mobiles, 28 objects, 25 QST giver prototypes with 29 M-style response blocks,
zero native Q/QA contracts, and no SHP source. QST record #40055 is empty
([empty record](../../../areas/qst/nizari.qst#L27)). No Nizari story sidecar or
mapped comprehensive source dossier was present before this review. The QST
records are mobile responses, not formal quest contracts
([QST file](../../../areas/qst/nizari.qst#L1),
[room/mobile/object/reset census](ZONE_COVERAGE_REGISTER.csv#L213)).

The local QST adds short greetings and topic responses. Street merchant #40056
describes a town of thieves and cutthroats and the Forty Thieves' livelihood
([responses](../../../areas/qst/nizari.qst#L29)). Merchant #40064 says the
Sultan may pay the thieves to keep citizens in terror and mentions the chief's
new “flame” ([rumors](../../../areas/qst/nizari.qst#L92)). These are local
story cues; the file contains no Q or QA contract, item requirement, or reward.

## Environmental story

- **A divided desert city.** The route passes from the mountain road and Sphinx
  gate into a heavy, oppressive city. The northern market is noisy, filthy,
  and crowded with beggars and pickpockets; another market room explicitly
  describes fish, dates, and enslaved people being sold together. A guarded
  southern district separates this poverty from the wealthy streets and
  Droijim Palace ([Sphinx entry](../../../areas/wld/nizari.wld#L160),
  [market](../../../areas/wld/nizari.wld#L1041),
  [slave market and southern gate](../../../areas/wld/nizari.wld#L1344),
  [palace gates](../../../areas/wld/nizari.wld#L2479)).
- **The Forty Thieves and the Sultan.** Blood graffiti, a deserted house
  marked as a traitor's fate, and the hidden treasure-vault clue lead into the
  thieves' lair. The vault contains a letter from the Sultan to the Chief
  Thief, copied to the Vizier, directing them to keep the populace afraid and
  promising lax pursuit and royal-catacomb plunder. The letter confirms the
  QST rumor that the Sultan protects the thieves
  ([abandoned house](../../../areas/wld/nizari.wld#L1158),
  [lair](../../../areas/wld/nizari.wld#L1202),
  [vault and letter](../../../areas/wld/nizari.wld#L1259)).
- **An unresolved “flame” clue.** The QST rumor about the thief chief's new
  love interest appears alongside graffiti naming Jasmine and a vault-door
  keyword of jasmine. The current mobile prototypes contain no matching
  Jasmine character. The sources make Jasmine a clue, but do not establish
  whether the intended character is absent, renamed, or only a rumor
  ([QST topic](../../../areas/qst/nizari.qst#L103),
  [graffiti](../../../areas/wld/nizari.wld#L1085),
  [vault clue](../../../areas/wld/nizari.wld#L1190)).
- **Palace power and repression.** The gold-plated palace contrasts with the
  northern streets. Its throne room displays the skulls of people executed for
  minor offenses; prison rooms describe chained corpses and execution as a
  cheaper substitute for imprisonment. The Vizier's rooms depict flayed and
  dismembered bodies, surgical implements, and a trapped demon. Empty royal
  storage and the Sultan's lavish, hostile private tower connect the
  oppression to palace corruption
  ([throne room](../../../areas/wld/nizari.wld#L2869),
  [prison](../../../areas/wld/nizari.wld#L3904),
  [Vizier's chambers](../../../areas/wld/nizari.wld#L3972),
  [Chamber of Shadows](../../../areas/wld/nizari.wld#L4090),
  [empty treasury](../../../areas/wld/nizari.wld#L4104),
  [Sultan's tower](../../../areas/wld/nizari.wld#L4143)).
- **The royal tombs and mirrors.** A plundered tomb opens into a sacred
  Hierophant burial room with a Death's Angel. The extensive mirror corridors
  connect the prison, concubine rooms, Vizier's chambers, empty treasury, and
  Sultan's tower. This makes the palace's hidden understructure part of the
  same fear-and-theft story, not a separate town route
  ([tombs](../../../areas/wld/nizari.wld#L3518),
  [mirror corridors](../../../areas/wld/nizari.wld#L3580),
  [pillow rooms](../../../areas/wld/nizari.wld#L3936)).
- **An older Thalos fragment.** Object #40050 is a diary scrap from refugees
  fleeing Thalos. It says Nizari has an evil reputation and hints at the
  Sultan's pact with Evil. No current ZON object reset places this object, so
  its text is defined but not shown as an ordinary reset placement
  ([object](../../../areas/obj/nizari.obj#L1),
  [local reset records](../../../areas/zon/nizari.zon#L305)).

## Mobiles, objects, and code

Mobile #40069 is described as a young woman who is “totally drugged”; its QST
greeting also sexualizes her behavior. Other sources show a slave market, a
Forty Thieves sleeping cave with a curtained-off bed for “recreational
activities,” chained prisoners, executions, and graphic torture
([concubine mobile](../../../areas/mob/nizari.mob#L265),
[greeting](../../../areas/qst/nizari.qst#L141),
[thieves' beds](../../../areas/wld/nizari.wld#L1202),
[slave market](../../../areas/wld/nizari.wld#L1344),
[prison](../../../areas/wld/nizari.wld#L3904),
[torture rooms](../../../areas/wld/nizari.wld#L3972)).
This coercive and explicit material deserves a deliberate tone and consent
review before it is turned into promoted story copy.

Mobile #40073 is the enraged Sultan, and #40074 is the blood-handed Vizier
([Sultan](../../../areas/mob/nizari.mob#L305),
[Vizier](../../../areas/mob/nizari.mob#L320)). The reset equips the Sultan
with the local bronze sword #40073, worn turban #40065, and ring #40067
([Sultan reset](../../../areas/zon/nizari.zon#L623)).
Prototype numbers overlap across types: mobile #40073 is the Sultan while
object #40073 is the bronze sword; mobile #40067 is a servant while object
#40067 is the ring. References below are type-specific.

Nizari's thief mobiles #40053–#40055 are bound to the native thief procedure
([assignments](../../../src/specs/specs.assign.c#L1063)). The procedure runs
on periodic calls while the thief is standing and not fighting; it may select
a visible, non-trusted player who is not fighting and call npc_steal
([handler](../../../src/specs/specs.mobile.c#L748)). This is a code-defined
player interaction, not a QST chain.

The WLD includes a lamp-merchant tent and resets mobile #40056 there, but no
SHP file defines a formal shop ([tent](../../../areas/wld/nizari.wld#L1434),
[merchant reset](../../../areas/zon/nizari.zon#L469)). Treat it as a merchant
scene unless a builder confirms that stock and shop behavior were intended.

## Word-open routes and physical keys

The Sphinx entry and the Forty Thieves' vault use key value -2. The say handler
matches the final word in the exit keyword, unlocks the paired exit, and clears
the secret bit when one is set ([handler](../../../src/cmd/actcomm.c#L186),
[say call](../../../src/cmd/actcomm.c#L496)). The Sphinx keyword ends in
“hi,” matching its easy-greeting clue; the vault keyword ends in “jasmine,”
matching the scratched clue and graffiti. This is a source-confirmed coded
route; actual play was not tested
([Sphinx clue](../../../areas/wld/nizari.wld#L178),
[vault clue and keyword](../../../areas/wld/nizari.wld#L1190),
[vault exit](../../../areas/wld/nizari.wld#L1242)).

| Route and key | Source placement | Builder check |
| --- | --- | --- |
| North Nizari gate #40066/#40079, ornate key #40051 | Key is given to a eunuch guard in #40079; both exits name #40051 ([WLD](../../../areas/wld/nizari.wld#L1521), [return](../../../areas/wld/nizari.wld#L1794), [key reset](../../../areas/zon/nizari.zon#L532)). | Confirm the guard's key is the intended access method for the rich district. |
| Forty Thieves entrance #40072/#40073, black stone key #40052 | A thief in the dark back alley receives the key; both exits name it ([WLD](../../../areas/wld/nizari.wld#L1656), [return](../../../areas/wld/nizari.wld#L1680), [reset](../../../areas/zon/nizari.zon#L450)). | Confirm this visible key path is intended alongside the separate jasmine vault route. |
| City back door #40108/#40109, old key #40056 | The object is explicitly called “old, unused,” is placed in Merchant's Hovel #40045, and is also the WLD key for this locked city door ([WLD](../../../areas/wld/nizari.wld#L2344), [return](../../../areas/wld/nizari.wld#L2369), [object](../../../areas/obj/nizari.obj#L57), [reset](../../../areas/zon/nizari.zon#L306)). | Resolve the “unused” label against the live key reference; align the description or lock if the mismatch is accidental. |
| Servants' entrance #40110/#40111, key #40057 | The simple key is reset in palace garden #40113; both WLD sides name it ([WLD](../../../areas/wld/nizari.wld#L2382), [return](../../../areas/wld/nizari.wld#L2411), [reset](../../../areas/zon/nizari.zon#L309)). | Confirm as authored; the placement and route appear aligned. |
| Palace gate #40115/#40128, golden key #40058 | A gate guard wears one key and a second is given to a guard in the palace kitchen; the outer WLD exit names #40058 while the return-side exit has key 0 ([WLD](../../../areas/wld/nizari.wld#L2500), [return](../../../areas/wld/nizari.wld#L2760), [gate key](../../../areas/zon/nizari.zon#L566), [second key](../../../areas/zon/nizari.zon#L598)). | Confirm the two copies and one-sided key requirement are deliberate. |
| Noble's house #40117/#40130, silver key #40059 | A glass serpent in the Glass Road area wears the key; both WLD sides name it ([WLD](../../../areas/wld/nizari.wld#L2561), [return](../../../areas/wld/nizari.wld#L2800), [key reset](../../../areas/zon/nizari.zon#L556)). | Confirm this route and guardian placement. |
| Library tunnel #40151/#40161, odd book #40060 | The door names #40060, but the object is an odd, apparently blank book placed in Noble's Bedroom #40132 ([WLD](../../../areas/wld/nizari.wld#L3221), [return](../../../areas/wld/nizari.wld#L3423), [object](../../../areas/obj/nizari.obj#L109), [reset](../../../areas/zon/nizari.zon#L311)). | Explain the book-as-key clue or align the item description/placement with the locked tunnel. |
| Jail cell #40185/#40186, key #40061 | The key is placed on the wall in the Guard Room; both WLD sides name it ([WLD](../../../areas/wld/nizari.wld#L3916), [return](../../../areas/wld/nizari.wld#L3929), [object/reset](../../../areas/zon/nizari.zon#L317)). | Confirm whether the imprisoned dead are meant to be unlockable in ordinary play. |

## Static connectivity

Nizari's 409 authored exits all resolve. Its 192 rooms form one local weak
component; it has no unresolved targets, fall fields, or current fields. One
paired WLD boundary joins room #40002 north to Surface room #575327, with the
return south to #40002
([Nizari exit](../../../areas/wld/nizari.wld#L10),
[outbound ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L550),
[return ledger row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1497)).
The weak component treats exits as links and does not prove passage through
locked doors, ordinary access, safety, or return behavior.

## Typed dependencies and history

Active sources consume Nizari prototypes:

- Pharrvly resets Nizari book object #40072 into room #40351
  ([reset](../../../areas/zon/pharrvly.zon#L309),
  [prototype](../../../areas/obj/nizari.obj#L381)).
- Verspin gives Nizari barbed stiletto object #40132 to a Bloodhawk spy in two
  reset records ([resets](../../../areas/zon/verspin.zon#L537),
  [prototype](../../../areas/obj/nizari.obj#L574)).
- Stormht's King Stragathor QST asks for Nizari bronze sword object #40073
  after the Sultan is defeated; Nizari equips that sword on Sultan mobile
  #40073 ([request](../../../areas/qst/stormht.qst#L61),
  [item term](../../../areas/qst/stormht.qst#L69),
  [sword prototype](../../../areas/obj/nizari.obj#L412),
  [sword reset](../../../areas/zon/nizari.zon#L626)).
- The standard and enhanceable Chaos equipment tables both reference Nizari
  ring object #40067 ([first table](../../../src/account/chaos_eq_data.h#L568),
  [second table](../../../src/account/chaos_eq_data.h#L573),
  [prototype](../../../areas/obj/nizari.obj#L219)).

Nizari also imports two active foreign objects onto the Sultan: Heavens stone
object #358 and the “memory of Nizari” object #55438 from Wh
([Sultan reset](../../../areas/zon/nizari.zon#L623),
[foreign object imports](../../../areas/zon/nizari.zon#L627),
[Heavens prototype](../../../areas/obj/heavens.obj#L3709),
[Wh prototype](../../../areas/obj/wh.obj#L5189)). No active foreign mobile
reset or shop reference to a Nizari prototype was found. These are static
prototype links, not proof that each reset, QST, or equipment path succeeds at
runtime.

The normalized scan of all 442 current `.zon` titles finds one exact “Nizari”
owner: active contributor `nizari`. Followed WLD history includes content
commits 28c8b2ab5 and 71d731c5b and the 2009 areas-path migration 642da5400;
it provides no direct retirement or replacement mapping. Nizari remains an
active full-area contributor and must not be moved.

## Cross-world semantic and source-map comparison

The similar city/palace/desert themes and map references do not identify an
alternate Nizari contributor:

- Active Basin Wastes room #34008 describes the paths from Nizari to the Basin
  Wastes, but its D2 edge leads to Basin room #34009 within `basin_wa`; the
  prose does not add an Nizari exit ([description](../../../areas/wld/basin_wa.wld#L194),
  [local exit](../../../areas/wld/basin_wa.wld#L208)). Nizari's single active
  boundary remains the paired link to Surface room #575327, documented above.
- The unregistered `surface2011.wld` and `surface2011-temp.wld` each contain a
  title-only “Nizari” map cell (#227488 and #627488). Both descriptions are
  empty and each room's four cardinal exits lead to neighboring VNUMs within
  that source map, not into active Nizari's #40000–#40199 range
  ([first map cell](../../../areas/wld/surface2011.wld#L577249),
  [temporary map cell](../../../areas/wld/surface2011-temp.wld#L577249)).
  These are coarse source-map labels, not duplicate 192-room city interiors.
- Active `desert` has a different title, “The Desert City of Venan'Trut,” and
  range *490–497. Its local source uses a Sultan/palace setting but contains no
  `Nizari`, `Droijim`, or “Forty Thieves” name. Shared genre terms do not show a
  predecessor or replacement ([title](../../../areas/zon/desert.zon#L2),
  [source dossier](../zone-stories/DESERT_CITY_OF_VENAN_TRUT.md)).
- Other exact references name Nizari as an external place or origin: a Sultan's
  sword in Stormht, an ambassador in Winterhaven, and Nizarian clothing on an
  Icecrag bounty hunter. Their source roles are covered in the typed-reference
  review above; none defines an alternate Nizari map.

The tracked same-stem history and this bounded semantic comparison establish
no retired predecessor, alternate-stem successor, or whole-zone replacement.
They do not resolve the remaining command-target alias or owner questions.

## Builder questions and recommended resolutions

1. **Is the “new flame” a real local character or an unresolved rumor?** The
   QST says the thieves do not know her identity, the world names Jasmine, and
   no current mobile is named Jasmine. Keep this as rumor unless the builder
   confirms a missing actor; avoid presenting a romance or rescue as a
   completed local chain.
2. **Which exact words should open the two key -2 doors?** The compiled handler
   uses the final exit-keyword token: “hi” for the Sphinx and “jasmine” for the
   vault. Keep these trigger words in sync with the clues; clarify that “hi,”
   rather than the broader synonym “hello,” is the Sphinx keyword if that is
   intended.
3. **Are the physical key placements deliberate?** In particular, reconcile
   the “unused” key #40056 with the city door that requires it and the odd
   book #40060 with the library tunnel that requires it. Confirm the duplicate
   golden key #40058 and the one-sided palace gate lock.
4. **Should the diary scrap be obtainable?** Object #40050 carries the Thalos
   refugee and Sultan-pact clue but has no current ZON placement. Place it only
   if it is intended player-facing evidence; otherwise keep the dossier clear
   that the text is dormant source content.
5. **Should the lamp merchant be a real vendor?** The room and mobile imply a
   stall, while Nizari has no SHP file. Preserve it as scenery unless a formal
   shop, keeper, and stock list are explicitly intended.
6. **How should the coercive material be framed?** Review the drugged
   concubine, enslaved market, implied sexual activity, executions, and torture
   for tone, consent, and agency before promoting these scenes as story
   content.
7. **Are the cross-area item links intentional?** Preserve the Nizari sword
   hand-in for Stormht and the Pharrvly, Verspin, and Chaos references if those
   active consumers remain. Confirm the Heavens stone and Wh memory carried by
   the Sultan are intentional imports.

This review records source declarations and code only. Live reset results,
locked-route reachability, return safety, shop behavior, and player experience
remain unqualified.
