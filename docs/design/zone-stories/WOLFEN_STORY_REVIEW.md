# The Lost Realms of Shadowfall: source review

Canonical key: `area:wolfen`. Physical coverage row 348. This is a
source-comprehensive review of the active WLD, MOB, OBJ, ZON, QST, and SHP
inputs. There is no `areas/story/wolfen.story.json` sidecar. Player access,
live reset state, merchant availability, and played narrative qualification
remain separate.

## Ownership and source inventory

The active `areas/AREA` entry is `wolfen *180-182`; its zone header is `#180`
and the title is “The Lost Realms of Shadowfall.” The WLD contains 258 rooms
(#18000–#18257), the MOB source 20 prototypes (#18000–#18019), and the OBJ
source 40 prototypes (#18000–#18039). The ZON header gives the area level range
20–30 and defines 170 reset commands. The local QST file has one action-only
giver block for Karig and no accepted mission/turn-in contract. The local SHP
file has one shop record keyed to keeper #18019; it stocks three local
instruments (#18036–#18038). [Active area entry](../../../areas/AREA#L92),
[zone header](../../../areas/zon/wolfen.zon#L1), [room source](../../../areas/wld/wolfen.wld#L1),
[mobiles](../../../areas/mob/wolfen.mob#L1), [objects](../../../areas/obj/wolfen.obj#L1),
[QST actions](../../../areas/qst/wolfen.qst#L1), and
[shop stock](../../../areas/shp/wolfen.shp#L1).

## Present-day Shadowfall

The authored setting is a Kimber Tribe wolfen farming village whose ordinary
routine has collapsed under an outbreak residents call the “burning.” The
entrance text says most villagers have moved inside the city proper under the
guards' nervous watch. The road approaches pass campsites, forest, a blasted
bridge, and signs of attack before reaching Shadowfall's homes, market court,
mayor's estate, and archmage tower. The forest is dark beneath blue lightning;
plantlife rots, animal behavior is distressed, and the map contains diseased
wolfen, bear, bugbear, stag, wolf, and transformed beast-spawn prototypes.
The text frames these as frightening and dangerous conditions without
recording a resolved cause or cure. [Road and village approach](../../../areas/wld/wolfen.wld#L69),
[village entrance](../../../areas/wld/wolfen.wld#L670), [blasted bridge](../../../areas/wld/wolfen.wld#L485),
and [Forest of Chaos](../../../areas/wld/wolfen.wld#L898).

The named community roles sharpen that contrast. Mayor Estidan is described
as self-absorbed while a scrap of writing says hunting stopped on his order
and stores are running low. Captain Vost looks nervous; the paper attributes
to him the view that the attacks have become directed and that an unfamiliar
creature may be involved. Archmage Karig is troubled. Priest Celicus Fahrn
prays for Shadowfall, and druid Eieslinn tends the unnaturally healthy willow
called the Tree of Life. Herbalist Jhary has attempted a remedy, but the note
leaves the treatment unreadable and says it failed. These are different
characters' observations, not a source-confirmed explanation of the outbreak.
[Karig](../../../areas/mob/wolfen.mob#L17), [Vost](../../../areas/mob/wolfen.mob#L30),
[Celicus](../../../areas/mob/wolfen.mob#L44), [Eieslinn](../../../areas/mob/wolfen.mob#L60),
[Estidan](../../../areas/mob/wolfen.mob#L76), [Jhary](../../../areas/mob/wolfen.mob#L271),
[Tree of Life](../../../areas/wld/wolfen.wld#L923),
[archmage's papers and bookcase](../../../areas/wld/wolfen.wld#L970), and
[tattered paper](../../../areas/obj/wolfen.obj#L377).

## Earlier Wolfen history and present-day clues

The book titled *A Concise History of a Wolfen Empire* attributes the Wolfen's
origin to wizard Llevron Dor. Its account says that human servants found their
way back to civilization; Llevron then transformed wolves, with Lionel as his
first success. Abuse by the human supervisors precipitated a Wolfen uprising
and siege. Llevron armed trusted assistants with nine powerful swords, the
battle ended with Llevron's death, and Lionel later ordered the Wolfen out of
Haven and made that city immaterial and invisible. The book is a long authored
history item stored in local containers; the current zone text does not
explicitly identify Shadowfall as Haven or establish that the armed Answerer
carried by Vost is one of those nine swords. [History book](../../../areas/obj/wolfen.obj#L269),
[Vost and the Answerer reset](../../../areas/zon/wolfen.zon#L47), and
[Answerer prototype](../../../areas/obj/wolfen.obj#L1).

The tattered paper preserves only fragments. It mentions an unnamed sick
creature, a caravan attack, a destroyed bridge, unhealthy game, the possibility
of directed attacks, and the village's “burning” name for the affliction. It
also recommends staying away from an unreadable place/item while saying
Eieslinn's cultivation seems to keep “them” away. Its final fragment describes
an unnamed person attempting to escape through an unstable portal with
unreadable companions, followed by the portal's destruction. Those `~%_`,
`<*+*>`, `*|*`, and `*#*#*` gaps are authored unknowns. The nearby, visibly
degenerated celestial dragon Vermithrax is not named by the paper as the
attacker or source of the disease. Keep these identities and causal links
unresolved. [Paper fragments](../../../areas/obj/wolfen.obj#L377),
[Vermithrax](../../../areas/mob/wolfen.mob#L1), and
[forest dragon location](../../../areas/wld/wolfen.wld#L4565).

The priest named Celicus shares a name with the explorer credited by the book
with rediscovering the route home, but the area sources do not say whether they
are the same person. Nor do they reconcile the paper's destroyed portal with
the two reset rifts linking the collapsed tunnel and Circle of Power. Keep the
historical account, current map, and partial note as separate evidence until a
builder supplies the intended relation. [Celicus description](../../../areas/mob/wolfen.mob#L44),
[paired rifts](../../../areas/zon/wolfen.zon#L27).

## Access objects, local services, and non-contract content

The opening three-room collapsed tunnel has two obscured passages and
stone-slab objects whose descriptions say to push for ingress/egress. The
ZON file resets the slab switches and a reciprocal pair of rift objects
between room #18002 and the Circle of Power, #18003. A third type-25 portal is
the merchant's wagon on the campsite road, targeting its interior room
#18256; room #18256 exits back to the campsite, and room #18257 is the wagon
storage room. The same wagon room has the instrument merchant and the local
shop. Static records describe the routes and targets, not that a character
can safely use them in a live state. [Tunnel exits](../../../areas/wld/wolfen.wld#L10),
[reset doors, slabs, and rifts](../../../areas/zon/wolfen.zon#L8),
[rift objects](../../../areas/obj/wolfen.obj#L575),
[wagon portal](../../../areas/obj/wolfen.obj#L646), and
[wagon interior](../../../areas/wld/wolfen.wld#L6597).

The local QST file contains two `qc_action` lines for Karig. Their authored
messages concern making his spellcasting effective in silent environments;
they do not define a local item exchange, accepted player proof, or contract
reward. Aliester's local shop offers the flute, lyre, and drum made from wood,
ivory, and raantha skull-hide. A shop message refuses service to “your people,”
so source stock alone does not qualify who can buy or whether the stock is
currently available. [Karig QST actions](../../../areas/qst/wolfen.qst#L3),
[instrument stock](../../../areas/shp/wolfen.shp#L3), and
[shop refusal text](../../../areas/shp/wolfen.shp#L36).

## Static qualification limits

The WLD has one reciprocal active boundary with Surface and no unresolved
targets, fall-chance rooms, or current rooms. Its two WLD-only weak components
are the 255-room main map and the three-room collapsed tunnel. Source reset
records place the two rifts and the wagon portal so the combined directed
WLD-plus-portal source graph reaches all 258 rooms from #18000. This does not
qualify ordinary entry, hidden-passage discovery, portal invocation, reset
availability, combat safety, or a player's return path. See the
[repository-wide connectivity and dependency review](../quest-story-followups/WOLFEN_AUDIT_REVIEW.md).

## Checkpoint 435: builder question review

Twenty direct builder questions and recommended resolutions collect the
remaining history, outbreak, route, shop, and authored-action decisions in
[Shadowfall builder questions](WOLFEN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
No area source or player-facing behavior changed in this review.
