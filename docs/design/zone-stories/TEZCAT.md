# Ruined Temple of Tezcatlipoca: source story map

This dossier maps the active `tezcat` contributor from its native room,
mobile, object, and reset files. It covers source-authored environmental
story and typed cross-area references; it does not claim a played journey or
runtime hazard test. See the [Tezcat audit supplement](../quest-story-followups/TEZCAT_AUDIT_REVIEW.md)
for the reconciled graph, title/history, external dependencies, and builder
questions.

## Source inventory and local contract boundary

The current active contributor is the 100-room zone numbered 989, rooms
#98900–#98999. Its [WLD](../../../areas/wld/tezcat.wld), [MOB](../../../areas/mob/tezcat.mob),
[OBJ](../../../areas/obj/tezcat.obj), and [ZON](../../../areas/zon/tezcat.zon)
files define 82 mobile prototypes and 64 object prototypes. The ZON reset
header uses mode 1. Its source has 14 door resets, 7 floor-object placements,
6 container placements, 145 mobile placements, 53 equipment placements, 36
follower placements, and 17 carried-object placements.

There is no local QST, SHP, or story-sidecar file. The independent
[QST/AREA index](../../../areas/qst/AREA#L316) nevertheless lists `tezcat`;
that is a source-list mismatch, not evidence of a local quest contract. The
active story contracts that use Tezcat items belong to Kastle and Tharnadia
and are mapped below.

## Room and environmental story map

| Rooms | Source story and layout |
| --- | --- |
| #98900–#98902 | Dark, narrow entrance hall; remains, oppressive stench, and the branches into a private chapel. #98900 is the active Surface arrival and ordinary return point. |
| #98903–#98921 | Private chapel dedicated in prose to an ancient and dark god. Blood, obsidian, broken pews, worshippers, and servants condemned to endless service establish a violent cult environment. Several rooms warn that visitors do not leave. |
| #98922–#98936 | Altar and adjoining chapel. The altar text asks how many lives and souls it has consumed. A reset places the bloody sacrificial dagger on a Tezcazul follower at the altar; passages connect into the chapel and the catacombs. |
| #98937–#98938 | High-priest office and bedroom. The blue/azure treatment gives this suite a relative calm that contrasts with the surrounding rooms. Stone and bone keys are reset on occupants of nearby rooms. |
| #98939–#98947 | Catacombs beneath the temple, with prose about blood, death, fallen warriors, and souls in the walls. A hidden side chamber (#98944) contains an obsidian coffin. |
| #98948–#98949 | Cave-in and the Night Crawler nest below it. The nest description says the only exit is upward. A follower Night Crawler in the nest carries two bloody talons for Kastle's separate return contract. |
| #98950–#98973 | Further catacombs and chambers: repeated flesh-wall and trapped-soul imagery, violent warnings, undead worshippers, vampires, and death-fiend encounters. A second etched wall clue appears at #98957/#98962. |
| #98974–#98999 | A second arrival point from Winterhaven (#98974), then deeper catacombs with Tezcazul, vampire, lost-soul, death-stalker, and tortured-soul groups. Named high-level figures include Taurtheim, Bard Faust, Vorphalack, Diabolus, Raism, and Verethorn. |

The room descriptions contain severe stench, blindness, cold, death, and
trapped-soul language. No local room extra-description blocks, QST, SHP,
sidecar, or identified special behavior turns that prose into a verified
mechanic. Treat it as narrative until a maintained behavior source confirms
otherwise.

## Mobile groups

The 82 local mobile prototypes are authored in [tezcat.mob](../../../areas/mob/tezcat.mob)
and placed by [tezcat.zon](../../../areas/zon/tezcat.zon).

| Mobile range or prototype | Source identity |
| --- | --- |
| #98900–#98908 | Nine Night Crawler variants, from the nest leader to worm-like, gas, and fluid forms. The follower at #98904 carries the two talons used by Kastle's contract. |
| #98909–#98912, #98926–#98927, #98938 | Blodzul variants and guards. |
| #98913–#98918, #98924–#98925, #98933, #98936, #98943–#98946 | Zombie priests, priestesses, worshippers, and cultists. |
| #98919–#98920, #98928–#98930, #98937, #98974–#98977 | Tezcazul priests, guards, and followers. Mobile #98920 is the carrier selected for object #98928 at the altar. |
| #98921, #98939 | Blood rats. |
| #98922–#98923 | Raism, described as Arkturas's Blood Devil, and Arkturas, the Ancient Blood Priest. |
| #98931–#98935, #98941, #98944–#98946, #98954–#98955, #98962, #98964–#98965, #98969 | Shadow and vampire cultists, priests, priestesses, and worshippers. |
| #98940–#98942 | Vargher, a Shadow Spawn of Muinroht, a shadow cultist, and a drow shadow reaper. |
| #98947–#98957, #98963, #98970–#98973 | Death fiends and death stalkers; #98947–#98957 include several distinct variants. |
| #98958 | Taurtheim, the Thrice Damned. |
| #98959 | Bard Faust, Lord of Darkness; also referenced by maintained random-equipment data. |
| #98960–#98961 | Vorphalack, Consumer of Souls, and Diabolus, Dragon of Darkness. |
| #98966–#98968 | Lost souls. |
| #98978–#98980 | Tortured souls. |
| #98981 | Verethorn, Greater Daemon of Tezcatlipoca. |

These identities establish a dense undead and cult encounter ecology, but
they do not resolve whether the names Tezcatlipoca, Dreptu Allur, Doden,
Arkturas, Muinroht, and the several titled figures represent one faction,
competing cults, or historical layers. Keep that relation open for builders.

## Object catalogue

The local object prototypes are in [tezcat.obj](../../../areas/obj/tezcat.obj).
The catalogue below preserves their distinct object type and local identity;
same-numbered mobile prototypes are separate records.

| VNUM | Object | VNUM | Object |
| --- | --- | --- | --- |
| #98900 | Sacrificial altar | #98901 | Blood dagger of Tezcatlipoca |
| #98902 | Frayed black robe | #98903 | Bloody black hooded robe |
| #98904 | Death ring of Tezcatlipoca | #98905 | Unholy robes |
| #98906 | Cobra scale boots | #98907 | Bloody mace |
| #98908 | Blodzul soul ring | #98909 | Soulstone amulet |
| #98910 | Spiked fluted dragonscale | #98911 | Huge Zanthium broad sword |
| #98912 | Bone key | #98913 | Stone key |
| #98914 | Bloody Night Crawler talon | #98915 | Bloody key |
| #98916 | Soulstone helm | #98917 | Obsidian coffin |
| #98918 | Obsidian key | #98919 | Zanthium belt |
| #98920 | Iron box | #98921 | Ancient grimoire |
| #98922 | Soulstone claymore | #98923 | Swirling mist potion |
| #98924 | Soulstone leggings | #98925 | Zanthium ear stud |
| #98926 | Soulstone collar | #98927 | Cobra scale shield |
| #98928 | Bloody sacrificial dagger | #98929 | Troll corpse/container |
| #98930 | Soulstone scabbard | #98931 | Obsidian desk/container |
| #98932 | Cobra skull badge | #98933 | Robe of tattered souls |
| #98934 | Totem of shadows | #98935 | Black hooded robe |
| #98936 | Soulstone visor | #98937 | Mask of lost souls |
| #98938 | Dagger of twisted souls | #98939 | Glowing azure potion |
| #98940 | Onyx potion | #98941 | Armor of Darkness |
| #98942 | Black Opal Eye of Diabolus | #98943 | Spiked Zanthium collar |
| #98944 | Spiked Zanthium bracer | #98945 | Spiked Zanthium gloves |
| #98946 | Bloody cloth | #98947 | Emerald nose ring |
| #98948 | Karthian tail spike | #98949 | Necklace of halfling ears |
| #98950 | Spiked ring | #98951 | Dreadveil visor |
| #98952 | Boots of dread | #98953 | Well-crafted backpack |
| #98954 | Mask of Darkness | #98955 | Chipped soulstone dagger |
| #98956 | Faded skullstone boots | #98957 | Faded dragonscale quiver |
| #98958 | Verethorn sleeves | #98959 | Scale of pulsating darkness |
| #98960 | Brimstone bracelet | #98961 | Pulsating gemstone earring |
| #98962 | Mask of shadows | #98963 | Faded skullstone sleeves |

The zone reset places fixed props and containers in the altar, office, bedroom,
and hidden chamber. Container contents include the scabbard and backpack in a
troll corpse, a badge in the desk, a grimoire in the iron box, and two mist
potions in the coffin. The reset also equips and gives many objects to local
encounters. It imports four externally owned objects; their ownership and
placement are detailed in the audit supplement.

## Reset and keyed-route structure

The [ZON file](../../../areas/zon/tezcat.zon) has 14 D resets matching the
14 flagged door directions, arranged as seven bidirectional pairs.

- The entrance branches #98902–#98903 and #98902–#98926 reset closed but
  unlocked.
- The high-priest office pair #98904/#98937 uses stone key #98913 and resets
  closed and locked. The bone key #98912 opens the bedroom pair #98937/#98938.
- The altar trapdoor #98922/#98939 uses bloody key #98915; reset state 6 sets
  closed, locked, and secret flags on both directions.
- The east wall from #98943 into chamber #98944 carries the clue “Dreptu
  Allur.” The current WLD exit flags are asymmetric; paired reset state 5
  adds the secret flag to both directions.
- The north wall #98957/#98962 carries the clue “Doden” and is flagged secret
  in both WLD directions; its reset closes the pair.

The stone key is on zombie priestess #98925 at #98904; the bone key is on
Arkturas #98923 at #98937; the bloody key is on zombie priest #98913 at
#98922. These are source placements, not proof of key availability or a
completed safe route. The nest room #98949 is reached down from cave-in room
#98948; its prose describes the upward direction as the sole way out.

## Active story and external prototype references

Two active foreign story contracts consume Tezcat objects:

- **Kastle / Volordles:** [kastle.qst](../../../areas/qst/kastle.qst#L37)
  requires object #98914 twice. The [story sidecar](../../../areas/story/kastle.story.json#L124)
  describes two loose Night Crawler talons, a twisted blood dagger #99447 and
  384,000 copper, and giver departure. The two E resets for object #98914 are
  on Night Crawler follower #98904 at nest #98949
  ([reset rows](../../../areas/zon/tezcat.zon#L192)). Supplied exact items
  satisfy the listed item proof; this contract does not establish personal
  recovery or defeat of a particular carrier.
- **Tharnadia / Zaberetornaz:** [tharnadia.qst](../../../areas/qst/tharnadia.qst#L90)
  accepts object #98928. The [story sidecar](../../../areas/story/tharnadia.story.json#L648)
  gives disguise kit #500, robe #132629, and potion #132630, with
  `disappear=0`. The local reset places object #98928 on Tezcazul follower
  #98920 at altar #98922 ([reset](../../../areas/zon/tezcat.zon#L129)).
  Delivery belongs to Tharnadia; it does not establish personal combat or
  receipt of any Tezcat reward.

The numbers #98914 and #98928 also identify local MOB records. The foreign
contract inputs above are explicitly `I` objects, and sidecar `item_vnums`
identify OBJ prototypes. Preserve that type distinction: the object #98914 is
a talon while mobile #98914 is a zombie priest; the object #98928 is a dagger
while mobile #98928 is a Tezcazul.

## Source-only limits

The [audit supplement](../quest-story-followups/TEZCAT_AUDIT_REVIEW.md)
reconciles the 351 active WLD graph with the inactive Surf and Underworld2
approaches, the two active arrivals, title uniqueness, followed history, and
bounded typed references. Those findings describe repository data and tracked
history. They do not confirm current loaded resets, player access, actual
visibility of secret walls, combat difficulty, item stock, or return safety.
