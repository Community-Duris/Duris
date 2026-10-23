# Astral Plane, Tiamat source story dossier

## What the source says

`astral_tiamat` is active zone 199, titled **Astral Plane, Tiamat**. Its four
active area files contain 31 rooms (#19900–#19955), 11 mobile prototypes,
44 object prototypes, and a reset file with mode 0. There is no local quest
source, shop file, or story sidecar. The zone title belongs to this contributor;
`tiamat` and `azhural` are separate active areas
([area list](../../../areas/AREA#L102),
[zone header](../../../areas/zon/astral_tiamat.zon#L1),
[rooms](../../../areas/wld/astral_tiamat.wld#L1),
[mobiles](../../../areas/mob/astral_tiamat.mob#L1),
[objects](../../../areas/obj/astral_tiamat.obj#L1)).

The first developed story branch follows a fragment of black granite torn from
the Prime Material Plane. The trails climb through reduced gravity to a cave
burned into the mountain. A dead band of humanoid warriors lies on one trail;
their melted armor and a damaged shield bearing Lord Redferne's crest suggest a
lost stand-off. A crude, incomplete inscription at another trail base records
a failed defense against arriving mages, but does not identify the writers or
explain the attack ([battle trail](../../../areas/wld/astral_tiamat.wld#L112),
[inscription](../../../areas/wld/astral_tiamat.wld#L131),
[cave approach](../../../areas/wld/astral_tiamat.wld#L325)).

Past the mountain, the source presents a set of lair and vault spaces associated
with Tiamat. A huge stone door opens toward a red, evil-looking pool. A nearby
milky pool sits in a hot, smoky cavern that smells of dragons, beside a boulder
door to a chamber whose prose says the main plane of Avernus is back that way.
The southern rooms describe lava caverns, a back entrance to Tiamat's layer, a
burning hidden passage, and hundreds of charred bodies kept as trophies. The
Vault of Avernus is described as Tiamat's private treasury, apparently unseen
for millennia, and its north wall appears to show a throne of Avernus
([large and circular caverns](../../../areas/wld/astral_tiamat.wld#L437),
[milky pool chamber](../../../areas/wld/astral_tiamat.wld#L484),
[lair entrance](../../../areas/wld/astral_tiamat.wld#L544),
[hidden passage](../../../areas/wld/astral_tiamat.wld#L588),
[vault](../../../areas/wld/astral_tiamat.wld#L611)).

A separate path descends from Tiamat's Pit of Sacrifice into a great lava cavern
and joins that southern lair chain. The reset set places fire, ice, shock,
assassin, dark, and guardian wyrms around the pit, the cavern, and the vault,
as well as a guardian dragon highlord and a huge slaad. Tiamat herself is not a
mobile prototype in this contributor; her main lair and mobile belong to the
separate active `tiamat` source. Three plainly named load rooms hold or route
armor and a sword, while the “Demogorogn” room spelling and two unnamed linked
rooms have no explanatory prose ([pit and lair branch](../../../areas/wld/astral_tiamat.wld#L21),
[mobile prototypes](../../../areas/mob/astral_tiamat.mob#L26),
[load rooms](../../../areas/wld/astral_tiamat.wld#L506)).

The source contains no local quest contract explaining the mountain's battle,
the vault's claim of long isolation, or the relationship between the Astral
Plane and Avernus. Other active areas do use Astral Tiamat objects in story
progression: Heavens requires bone-white skull-faced helmet #19996, and
Winterhaven requires Isha's cloak #19916. The cloak and Volo's boots #19911 can
be produced by the death callback for the pitiful slave in the separate Tiamat
area; #19996 has no active reset placement or C++ creation reference found by
this audit
([Heavens quest input](../../../areas/qst/heavens.qst#L148),
[Winterhaven quest input](../../../areas/qst/wh.qst#L279),
[Winterhaven story contract](../../../areas/story/wh.story.json#L870),
[slave death callback](../../../src/specs/specs.winterhaven.c#L4626)).

## Builder questions

1. Who were Lord Redferne's warriors, what event killed them, and can the
   damaged inscription be completed or tied to a known mage force?
2. Is the Vault of Avernus intended to be an undiscovered private treasury
   despite the eight active one-way WLD entrances to it from other areas and
   its imported stock? Is #19953 a player destination, a reset room, or both?
3. Should the exit from the vault that reads “throne of Avernus” lead to a
   specific throne scene, and what should its `-2` WLD key sentinel mean?
4. Should the type-25 acid pool #19907 transport entrants to Tiamat #19600?
   Its value names that target and it is reset twice, but no active object
   handler is assigned to it. Confirm intended behavior before treating it as a
   route.
5. Which keys should open the locked or secret passages? The source resets the
   local rusty and magma keys, but the door records also name Astral Main keys
   #19876 and #19878 that this zone does not reset.
6. Should the guardian dragon highlord #19908 enforce the southward restrictions
   encoded in `guild_guard` for rooms #19950, #19951, and #19954? The compiled
   assignment is to mobile #19705, for which no active reset was found, while
   the local reset loads #19908 without that callback.
7. What is the intended source for the Heavens quest's #19996 helmet input? No
   active placement or code creation reference was found, although the object
   prototype exists.
8. What story or operation belongs in unnamed rooms #19900/#19901 and the
   “Demogorogn loading room” #19947? Are these deliberate staging rooms?
9. Are the Redferne battle, the Avernus vault, and the Tiamat lair meant to be
   parts of a shared timeline? Current prose and reset links leave that
   relationship unstated.
10. Should `astral_tiamat` have a command-target alias beyond its normalized
    title “Astral Plane, Tiamat”? The title is unique among 442 scanned `.zon`
    files, and no lexical duplicate was found. Keep the source key and display
    title; add an alias only after the owner names its exact target and intended
    distinction.

**Recommended source disposition:** keep `astral_tiamat` active and in place.
Preserve its Astral Main, Realm, Tiamat, and Winterhaven boundaries and its
cross-area reset references while the builder questions above are resolved.
This dossier reports authored source and repository references; it does not
assert live player access, successful quest acquisition, or safe return.
