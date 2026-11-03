# The Human Outpost

The active outpost contributor uses zone header #821 and the title The Human
Outpost. It contains 40 rooms, 22 mobile prototypes, 21 object prototypes, a
local QST dialogue for Lord Perrin, and one shop. It appears in both the full
area list and the QST area index. There is no story sidecar or native Q/QA
completion contract. See the [active area list](../../../areas/AREA#L293),
[zone header](../../../areas/zon/outpost.zon#L1), [QST area
index](../../../areas/qst/AREA#L261), and [coverage row
227](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L227).

## The garrison and its work

The outpost is an underground Tharnadian military base. Solid rock, dust,
magical lighting, guard walks, narrow passages, barracks, map rooms, a
recreation area, training rooms, an armory, a treasury, and a lower prison
make up the authored layout. The armory describes new equipment being forged
there and magical finds from raids being brought for identification. Visitors
are generally unwelcome, although wealthy visitors may buy equipment at
inflated prices. The [entry and armory descriptions](../../../areas/wld/outpost.wld#L1)
and [armorer profile](../../../areas/mob/outpost.mob#L323) establish these
details.

The mobile text describes an active military operation. The entrance is
guarded by a Tharnadian war priest, a young mage, a hired mercenary, and
soldiers. Officers study maps of Duris and plan attacks on Duergar and Drow
settlements; another map mentions Isoril's bandit fort. Lord Perrin is
described as Winterhaven's outpost commander and discusses upcoming raids
with his officers. These are authored descriptions, not proof of a current
campaign. See the [entrance mobile records](../../../areas/mob/outpost.mob#L1),
[map-room description](../../../areas/wld/outpost.wld#L489), and
[Lord Perrin profile](../../../areas/mob/outpost.mob#L308).

Several reset-placed sick guards occupy clean sleeping quarters marked by
bloody bandages and used medical equipment. Their mobile text says a strange
sickness has spread through them. Three imported Heavens shop products are
medical supplies: gauze bandage #363, gauze tourniquet #368, and large
medicated bandage #369. The [sick guards](../../../areas/mob/outpost.mob#L94),
[quarters](../../../areas/wld/outpost.wld#L231), [shop record](../../../areas/shp/outpost.shp#L1),
and [Heavens object definitions](../../../areas/obj/heavens.obj#L3783) support
those details; the source does not explain the sickness.

The lower rooms are a prison and interrogation area. A reset goblin prisoner
is described as shackled and in great pain while a torturer questions him.
The room text says prisoners are usually from evil races and that magical
light keeps them in constant pain during interrogation. Reset guards carry
brilliant white orbs, matching the dark-room description that guards must
bring their own light. See the [prison rooms](../../../areas/wld/outpost.wld#L741),
[prisoner and torturer profiles](../../../areas/mob/outpost.mob#L247),
[orb object](../../../areas/obj/outpost.obj#L239), and
[prison resets](../../../areas/zon/outpost.zon#L214).

## Lord Perrin, the key, and the Cathedral

Lord Perrin's local QST dialogue has the trigger phrase “key ethereal
cathedral.” He says he has the ethereal key but will not give it to the player
because a skeleton would devastate the world if it were not destroyed. His
reset places key #82110 on him in the officers' meeting room. See the [local
QST](../../../areas/qst/outpost.qst#L1), [Lord Perrin reset and key
placement](../../../areas/zon/outpost.zon#L184), and [key
object](../../../areas/obj/outpost.obj#L150).

The cross-area source records identify the Cathedral in active
Tharnadian Ruin. That zone's QST text names Lord Perrin at the Winterhaven
Outpost as the holder of its ethereal key, and describes a skeletal knight
sealed inside the Cathedral. The Cathedral's outer door says only an ethereal
key can open it; both sides of the door use local key #82110 in their WLD
key fields and are reset locked. The [Tharnadian Ruin quest
text](../../../areas/qst/tharnadian_ruin.qst#L49), [Cathedral entrance key
field](../../../areas/wld/tharnadian_ruin.wld#L971), [reverse key
field](../../../areas/wld/tharnadian_ruin.wld#L1016), and door resets at
[room #5540](../../../areas/zon/tharnadian_ruin.zon#L40) and
[room #5541](../../../areas/zon/tharnadian_ruin.zon#L44) make this a direct
source-defined key relationship.

Perrin also carries a small black box containing an azure holy symbol and a
papyrus scroll signed by Lord Kaynor. The scroll calls the symbol a key to
entering the Cathedral in the Tharnadian ruins and warns that the undead
inside could destroy new Tharnadia if the symbol reached the wrong hands.
The symbol is itself an ITEM_KEY prototype, but no scanned WLD exit key field
uses #82118; the Cathedral door fields use #82110. The source does not say
whether the key and symbol open separate seals or which interaction is
intended. See the [Lord Perrin item resets](../../../areas/zon/outpost.zon#L184),
[symbol and scroll objects](../../../areas/obj/outpost.obj#L264),
[Tharnadian Ruin door key field](../../../areas/wld/tharnadian_ruin.wld#L971), and
[skeletal knight profile](../../../areas/mob/tharnadian_ruin.mob#L49).

The same ethereal key #82110 is also named in both WLD door fields for the
Outpost treasury entrance, between rooms #82106 and #82125. The treasury is
described as a temporary store for loot from raids on cities of evil races.
This is a second source-defined use of the key. See the [local armory door key
field](../../../areas/wld/outpost.wld#L150), [treasury door key
field](../../../areas/wld/outpost.wld#L554), and the paired treasury resets at
[room #82106](../../../areas/zon/outpost.zon#L8) and
[room #82125](../../../areas/zon/outpost.zon#L12).

## Reset-placed stock and local quest scope

The local resets reference all 22 mobile prototypes and all 21 object
prototypes. No local mobile or object prototype is left unplaced by the
current zone file. Lord Perrin's room contains the key and box; the three
imported Heavens medical supplies are also loaded onto the armorer and listed
in the shop. The [local reset file](../../../areas/zon/outpost.zon#L105),
[shop inventory](../../../areas/shp/outpost.shp#L1), and [imported
bandages](../../../areas/obj/heavens.obj#L3783) show these relationships.

The shop's configured refusal text says “I do not sell to your people!” The
source does not identify the buyer condition that selects this message or
qualify actual shop access. The QST file provides dialogue for Lord Perrin,
but there are no native Q/QA completion contracts. No runtime interaction was
checked.

## Questions for the area owner

- **OUTPOST-Q01 — Does Cathedral access require ethereal key #82110, holy symbol
  #82118, or both?** The Tharnadian Ruin door fields name #82110, while the
  scroll calls the symbol the key to entering the Cathedral. **Conservative
  recommendation:** retain #82110 as the only key named by both door fields;
  treat any additional use for #82118 as unresolved until the owner identifies
  its intended interaction. See the [key and scroll reset](../../../areas/zon/outpost.zon#L184),
  [symbol and scroll](../../../areas/obj/outpost.obj#L264), and both Cathedral
  [door key fields](../../../areas/wld/tharnadian_ruin.wld#L971).
- **OUTPOST-Q02 — What is the intended key handoff?** Outpost dialogue says
  Perrin will not give the key under any condition, while Tharnadian Ruin quest
  text describes him as granting it to someone he judges worthy and elsewhere
  says he grants access to no one. **Conservative recommendation:** preserve
  both source statements as unresolved; do not claim Perrin grants the key or
  author a new handoff until the owner resolves the contradiction. See the
  [local dialogue](../../../areas/qst/outpost.qst#L1) and
  [Cathedral quest text](../../../areas/qst/tharnadian_ruin.qst#L49).
- **OUTPOST-Q03 — Is it intentional for ethereal key #82110 to unlock both the
  Cathedral and the Outpost treasury?** The reset-open graph also leaves
  Perrin's room behind a closed door; this static source reading does not
  qualify how a player can reach him or use the key. **Conservative
  recommendation:** preserve the declared double use and all reset door states;
  do not add a bypass or promise access without a qualified route. See the
  [Perrin/key reset](../../../areas/zon/outpost.zon#L184),
  [armory-side door field](../../../areas/wld/outpost.wld#L150),
  [treasury door field](../../../areas/wld/outpost.wld#L554), and
  [paired treasury resets](../../../areas/zon/outpost.zon#L8).
- **OUTPOST-Q04 — What causes the sickness affecting the guards, and is it tied
  to a quest or nearby story?** **Conservative recommendation:** leave the
  sickness as authored environmental story until its cause and intended remedy
  are confirmed; do not invent a diagnosis, cure, or reward from the medical
  stock alone. See the [sick guards](../../../areas/mob/outpost.mob#L94),
  [quarters](../../../areas/wld/outpost.wld#L231), and
  [medical shop stock](../../../areas/shp/outpost.shp#L1).
- **OUTPOST-Q05 — Which buyer condition triggers the shop's “your people”
  refusal, and are the imported medical supplies its intended outside stock?**
  **Conservative recommendation:** preserve the configured refusal and listed
  stock without inferring a race/faction predicate or claiming current service
  availability. See the [shop record](../../../areas/shp/outpost.shp#L1) and
  [imported medical items](../../../areas/obj/heavens.obj#L3783).
- **OUTPOST-Q06 — Are Lord Perrin's planned raids against Duergar, Drow, and
  Isoril's bandit fort intended as an active campaign for this area, or as
  military background?** **Conservative recommendation:** treat the maps and
  officer dialogue as setting until the owner names an objective, successful
  action, and outcome; do not infer a current raid state. See the
  [map-room description](../../../areas/wld/outpost.wld#L489) and
  [Perrin profile](../../../areas/mob/outpost.mob#L308).
- **OUTPOST-Q07 — Is the goblin in the lower prison intended to have a rescue
  or release outcome beyond the current interrogation scene?**
  **Conservative recommendation:** preserve the captivity text without claiming
  rescue, consent, or release; add an outcome only after its attainable remedy
  and beneficiary are defined. See the [prison rooms](../../../areas/wld/outpost.wld#L741),
  [prisoner and torturer profiles](../../../areas/mob/outpost.mob#L247), and
  [prison resets](../../../areas/zon/outpost.zon#L214).

## Current source resolution

Keep active outpost **DO NOT MOVE** and preserve its room and prototype VNUMs,
Surface boundary, Lord Perrin dialogue and key placement, Cathedral key
dependency, treasury lock, and shop stock. Preserve the single inactive
source-only Surf approach on Surf candidate row 97 **RETAIN IN PLACE**; it is
not active ingress. No source move or gameplay change is proposed. The
repository-wide audit records the exact graph, typed references, title
history, and candidate disposition.
