# Active zone journal evidence inventory

Generated from the authoritative `areas/AREA` list, native Q bindings, active prototypes,
and literal special assignments. Counts describe static evidence, not gameplay qualification.
Links between an exchange output and another input are review candidates; they do not
prove mandatory order, personal sourcing, branches, or a scripted completion.

Scanned 350 catalog zones; 221 have native Q contracts;
2668 distinct Q contracts; 176 authored journals.

Regenerate with:

```bash
python3 scripts/zone_story_quest_zone_inventory.py --output docs/reference/ZONE_STORY_ZONE_INVENTORY.md
```

The [priority roadmap](../design/ZONE_STORY_ZONE_PRIORITIES.md) supplies the reviewed order
and proposed story families. Every unmapped area still has its native fallback journal
when accounting is active and the area/giver has been discovered/encountered.

## Areas with native quests

Q = distinct native contracts; M = player-addressable dialogue response blocks;
links = distinct items connecting separate local exchanges. Special names are inspection
leads from literal assignments only, not an exhaustive script audit. Empty special lists
do not establish the absence of shared, generated, or dynamically assigned procedures.

| Area / source | Q | M | Links | Authored | Sample native offering → outcome | Assigned special leads |
| --- | ---: | ---: | ---: | --- | --- | --- |
| Apocalypse Castle (`4horse`) | 2 | 5 | 0 | Yes | [1 × the skull of war; 1 × the skull of death; 1 × the skull of famine; other required items → a golden vault key](../../areas/qst/4horse.qst#L27) | brainripper, mankiller |
| The Royal Mausoleum of Castle IceCrag (`Voluntown`) | 2 | 6 | 0 | Yes | [1 × a key fragment; 1 × a key fragment; 1 × a key fragment; other required items → The Drakenstone key](../../areas/qst/Voluntown.qst#L64) | — |
| The Tempest Court (`airp`) | 8 | 16 | 2 | Yes | [1 × a wisp of wind; 1 × a living breeze; 1 × the boots of the four winds; other required items → Cloudseeker, the Unseen Breeze of the Four Winds](../../areas/qst/airp.qst#L98) | dagger_of_wind |
| The Mountain Valley of Dawndale (`airshipgrave`) | 13 | 4 | 4 | Yes | [2 × a vial of liquid sunlight; 1 × a handful of combustable rock dust; 1 × a bucket of rank pool water; other required items → the lost blade of the Astral Dancer, 'Ender'](../../areas/qst/airshipgrave.qst#L101) | — |
| Alatorin - the Forge City (`alatorin`) | 495 | 291 | 47 | Yes | [8 × some rune-covered silk; 2 × a gnomish shopkeepers token → the bindings of the arcane](../../areas/qst/alatorin.qst#L6433) | earring_powers, generic_parry_proc, miners_helmet, rentacleric, ship_shop_proc |
| Arachdrathos - Drow City (`arac-web`) | 1 | 1 | 0 | Yes | [native payment/conditions → a dildo-shaped key](../../areas/qst/arac-web.qst#L6) | inn, money_changer, pet_shops, world_quest |
| Arcium, the Plagued Kingdom (`arcium`) | 1 | 1 | 0 | Fallback | [1 × a bleeding heart of Kovii; 1 × a bleeding heart of Granra; 1 × a bleeding heart of Kurlon; other required items → the shield proclaimed 'Hope'](../../areas/qst/arcium.qst#L11) | world_quest |
| Ashrumite Village (`ashrumite`) | 12 | 13 | 8 | Yes | [1 × a necklace of silver set with gems; 1 × an amethyst; 1 × an exotic tigers-eye gem; other required items → a small gold nugget](../../areas/qst/ashrumite.qst#L101) | cityguard, drunk_one, dump, guild_guard, inn, janitor |
| The Lair of Tiamat (`azhural`) | 2 | 2 | 0 | Fallback | [8 × a shard of bone → a key of fused bone shards](../../areas/qst/azhural.qst#L18) | sphinx_prefect_crown |
| Bahamut's Palace (`bahamut`) | 1 | 3 | 0 | Yes | [1 × Bahamut's personal seal → a glowing white key](../../areas/qst/bahamut.qst#L24) | artifact_stone, bahamut, bloodfeast, dragonlord_plate, mrinlor_whip, sunblade |
| The Bandit Canyons (`bandit`) | 1 | 0 | 0 | Fallback | [1 × the still-beating heart of Bel; 1 × the orb of unmaking; 1 × A mystic runed stone tablet → the scroll of Strength](../../areas/qst/bandit.qst#L13) | — |
| Bandit Camp (`banditca`) | 4 | 4 | 0 | Yes | [1 × a shackle key → native reward/response](../../areas/qst/banditca.qst#L38) | — |
| The Realm of Barovia (`barovia`) | 9 | 41 | 2 | Yes | [1 × a silver horse shoe; 1 × a gold chalice; 1 × a rare engraved electrum coin; other required items → the time warp mask](../../areas/qst/barovia.qst#L169) | — |
| The Realm of Barovia Continued (`barovia2`) | 5 | 16 | 0 | Yes | [1 × a tiny locket on a platinum chain; 1 × an old, cloudy glass eye → a holy symbol of the Ravenkind, the key to the outer gates of Castle Ravenloft](../../areas/qst/barovia2.qst#L132) | barovia_undead_necklace |
| The Basin Wastes (`basin_wa`) | 10 | 6 | 1 | Yes | [1 × a smoldering dragon scale; 1 × a glowing potion; 1 × an ancient signet ring → a glowing white-gold ring](../../areas/qst/basin_wa.qst#L35) | block_dir |
| Bastine Castle (`bastine`) | 14 | 4 | 0 | Yes | [1 × the wand of Life → a seal of the order of the bastine knights, the platemail of knighthood](../../areas/qst/bastine.qst#L141) | — |
| The Battlefield (`battlefi`) | 7 | 12 | 0 | Yes | [1 × some swirling mist; 1 × a holy cross; 1 × a skull; other required items → a holy gleaming longsword 'Righteous'](../../areas/qst/battlefi.qst#L67) | righteous_blade, undead_inn |
| The Bronze Citadel (`bctdl`) | 2 | 1 | 0 | Fallback | [1 × the seal of the Dungeon Master → an intricate black stone key](../../areas/qst/bctdl.qst#L11) | artifact_invisible, bel_sword |
| The Black Pearl (`blackpearl`) | 31 | 14 | 19 | Yes | [1 × a sword fragment; 1 × a sword fragment; 1 × a sword fragment; other required items → a thick broadsword named 'dragonslayer'](../../areas/qst/blackpearl.qst#L214) | — |
| Braddistock Mansion (`brad`) | 5 | 9 | 1 | Yes | [1 × the first piece of the Star Stone; 1 × the second piece of the Star Stone; 1 × the third piece of the Star Stone; other required items → the Star Key](../../areas/qst/lortower.qst#L266) | braddistock |
| Braddistock Mansion (`braddistock`) | 2 | 1 | 1 | Yes | [1 × a collar with an inscription, "Slippers" → the remains of Lord Braddistock](../../areas/qst/braddistock.qst#L30) | jet_black_maul |
| Plane of Fire, Brass (`brass`) | 7 | 12 | 2 | Yes | [1 × the flaming head of Artoon Branan; 1 × the flaming head of the arch-magi; 1 × the flaming head of Ornon Kasoon; other required items → a vortex of air forming a sash](../../areas/qst/brass.qst#L182) | artifact_hide, holy_mace, inn |
| The Town of Breale (`breale`) | 6 | 6 | 0 | Yes | [2 × a red mushroom; 1 × some ash; 2 × a tiny pile of wood → the amulet of the triad of witches](../../areas/qst/breale.qst#L72) | breale_townsfolk |
| The BrimStone Forge (`brimeforge`) | 2 | 1 | 0 | Fallback | [1 × the locket of the first; 1 × the locket of the second; 1 × the locket of the third → a wand of fiery power, an ethereal key](../../areas/qst/brimeforge.qst#L8) | — |
| Bloodstone Keep (`bs`) | 65 | 74 | 35 | Yes | [1 × the essence of Ogremoch; 1 × the essence of Imix; 1 × the essence of Yan-c-bin; other required items → the elixir of power](../../areas/qst/bs.qst#L1082) | bs_baron, bs_barons_mistress, bs_boar, bs_boss, bs_brat, bs_citizen |
| The Bugger Caves (`bugger`) | 2 | 1 | 0 | Fallback | [1 × a bugger egg; 1 × a bugger egg; 1 × a buggers carapace → some spiked carapace armor](../../areas/qst/bugger.qst#L7) | — |
| The Twin Keeps of Devastated Tharnadia (`caertannad`) | 30 | 37 | 10 | Yes | [3 × a Bren'Shan harpy feather; 3 × a Straka harpy feather → a necklace of frost harpy feathers](../../areas/qst/caertannad.qst#L395) | caertannad_summon |
| Quintaragon Castle (`castle`) | 2 | 2 | 0 | Fallback | [1 × a white bone studded with fine diamonds → the flaming orb of revenge](../../areas/qst/castle.qst#L31) | — |
| Caves of Mt. Skelenak (`caves_skelenak`) | 9 | 8 | 0 | Yes | [1 × a bronze scepter; 1 × an engraved bracelet of human bones → a copper mask](../../areas/qst/caves_skelenak.qst#L143) | guild_guard, piercer |
| Centaur Villages (`centaur_zone`) | 7 | 4 | 2 | Yes | [2 × a half amulet → a centaurian bracelet of honor, the centaurian legplates of honor](../../areas/qst/centaur_zone.qst#L152) | — |
| Ceothia (`ceofutur`) | 1 | 1 | 0 | Fallback | [1 × a bluestone vial → a shard of bluestone](../../areas/qst/ceofutur.qst#L27) | — |
| Ceothia (`ceopast`) | 6 | 8 | 1 | Yes | [1 × a shard of bluestone; 1 × a lock of green hair; 1 × a blood red feather → a bluestone key, a bluestone vial](../../areas/qst/ceopast.qst#L104) | — |
| Ceothia (`ceothia`) | 9 | 9 | 1 | Yes | [1 × the badge of the jade wyrm thief guild; 1 × the badge of the red shadow thief guild; 1 × the badge of the violet death thief guild → a black leather eyepatch rimmed with platinum, the badge of the Ceothian thief guild](../../areas/qst/ceothia.qst#L20) | crew_shop_proc, inn, ogre_warlords_sword |
| Pits of Cerberus (`cerebusp`) | 9 | 5 | 0 | Yes | [1 × a large coconut; 1 × a broken coconut; 2 × a small coconut; other required items → a coconut belt](../../areas/qst/cerebusp.qst#L20) | cerberus_load, master_set, revenant_helm |
| The Church of the Eternal Dusk (`church`) | 2 | 2 | 0 | Fallback | [1 × a badge of holy patronage → native reward/response](../../areas/qst/church.qst#L14) | — |
| The Citadel (`citadel`) | 3 | 39 | 0 | Yes | [1 × a bluish key → native reward/response](../../areas/qst/citadel.qst#L47) | — |
| Cloud Giant Kingdom (`cldgt`) | 3 | 4 | 0 | Yes | [5 × a yeti pelt; 1 × a thick leather strap → a girdle lined with fur](../../areas/qst/cldgt.qst#L7) | — |
| The Forest City of Aravne (`clfhaven`) | 3 | 7 | 0 | Yes | [1 × the heart of Enzekail; 1 × the heart of the female weretor; 1 × the heart of the male weretor; other required items → an ornate gold key](../../areas/qst/clfhaven.qst#L32) | inn, llyren, wh_corpse_to_object |
| Father Tel's Holy Cloister (`cloister`) | 8 | 19 | 1 | Yes | [1 × the ring of a duergar elder; 1 × a vial of poison → a rib bone](../../areas/qst/cloister.qst#L256) | — |
| The Clawed Caverns (`clwcvrn`) | 20 | 8 | 4 | Yes | [1 × a large, flat, blue crystal → a large, flat, blue crystal](../../areas/qst/clwcvrn.qst#L11) | burn_touch_obj, claw_cavern_drow_mage, clwcvrn_golem_shatter, clwcvrn_protect |
| The Great Realm of Duris (`connectorzones`) | 8 | 12 | 0 | Yes | [1 × the skin of a shadow eel; 1 × a tight green cloak; 1 × a foggy visor; other required items → the handle of the shadowy staff of damnation, the ring of the Netherworld](../../areas/qst/connectorzones.qst#L171) | crew_shop_proc, damnation_staff, nuke_damnation, world_quest |
| The Sky City of Ultarium (`cosmic`) | 17 | 5 | 0 | Yes | [1 × the soul of Zeenium; 1 × the soul of Ignor; 1 × the soul of Strata; other required items → the bracer of the whirlwinds, the robes of the maelstrom](../../areas/qst/cosmic.qst#L59) | mob_do_rename_hook, pet_shops, proc_whirlwinds |
| Court of the Muse (`court`) | 9 | 6 | 4 | Yes | [12 × a scale of a koi fish → a druidic necklace of fish scales](../../areas/qst/court.qst#L87) | — |
| Crakkaros' Liar (`crakkaro`) | 11 | 9 | 3 | Yes | [17 × a piece of animal fur → native reward/response](../../areas/qst/crakkaro.qst#L166) | — |
| Nakral's Crypt (`crypt`) | 5 | 6 | 1 | Yes | [4 × a small collection of sticks and twigs; 2 × a medium-sized branch → native reward/response](../../areas/qst/crypt.qst#L35) | inn |
| Darkfall Forest (`darkfall`) | 1 | 0 | 0 | Fallback | [1 × the still-beating heart of Bel; 1 × the orb of unmaking; 1 × A mystic runed stone tablet → the scroll of Agility](../../areas/qst/darkfall.qst#L37) | — |
| The Motte and Bailey of Duke Delwyn (`delwyn`) | 11 | 9 | 6 | Yes | [1 × a spool of fine white yarn → a spool of crimson yarn](../../areas/qst/delwyn.qst#L160) | — |
| The Desert City of Venan'Trut (`desert`) | 8 | 8 | 1 | Yes | [1 × a large glowing potion → a leather studded mining belt](../../areas/qst/desert.qst#L10) | crew_shop_proc, ship_shop_proc, world_quest |
| Desolate (`desolate`) | 11 | 1 | 3 | Yes | [1 × an iron rod; 1 × a broken wheel → a repaired wheel](../../areas/qst/desolate.qst#L81) | master_set |
| Desolate Under Fire (`desolateinv`) | 17 | 0 | 1 | Yes | [8 × some cut leather bindings → the cloak of the forest goddess](../../areas/qst/desolateinv.qst#L116) | inn |
| Dirk'nspire Stronghold (`dirkn`) | 2 | 1 | 0 | Fallback | [1 × a tattered piece of silk-paper → native reward/response](../../areas/qst/dirkn.qst#L19) | — |
| Home of the Divine (`divhome`) | 32 | 20 | 4 | Yes | [1 × a token of earth; 1 × a token of water; 1 × a token of air; other required items → a silky black dress of the sirens, a harp of the sirens](../../areas/qst/divhome.qst#L8) | — |
| Domain of Lost Souls (`dlsc`) | 2 | 2 | 0 | Fallback | [3 × the skull of a seasoned warrior → a sheath of stitched together skulls](../../areas/qst/dlsc.qst#L47) | critical_attack_proc, kvasir_dagger |
| Drifting Realm (`dream`) | 2 | 2 | 0 | Fallback | [1 × a strange crystal skull; 1 × a strange crystal skull; 1 × a strange crystal skull; other required items → a swirling force of light and darkness](../../areas/qst/dream.qst#L46) | — |
| Clan Stoutdorf Settlement (`drst`) | 1 | 3 | 0 | Yes | [1 × a shiny ruby; 1 × a scalp of a drider; 1 × a small figurine; other required items → the gauntlets of dwarven kind](../../areas/qst/drst.qst#L34) | dwarfslayer |
| Treasure Caves (`dungeon`) | 1 | 1 | 0 | Fallback | [1 × a small brass figurine → native reward/response](../../areas/qst/dungeon.qst#L13) | blue_sword_armor |
| Temple of the Earth (`earth`) | 2 | 22 | 0 | Yes | [1 × a badge of gloomhaven → a vial of blood](../../areas/qst/earth.qst#L27) | eligoth_rift_spawn, patrol_shops, toe_chamber_switch |
| Grumbar's Domain (`earthp`) | 2 | 9 | 0 | Yes | [10 × a shard of planar granite → a huge key of blazing white flame](../../areas/qst/earthp.qst#L65) | purple_worm |
| The Elemental Groves (`element`) | 3 | 60 | 0 | Yes | [1 × a cube of ethereal matter → a greenstone earring](../../areas/qst/element.qst#L385) | glades_dagger |
| Barrow of the Quiosho (`elftomb`) | 1 | 0 | 0 | Fallback | [1 × the bloody head of the elven king → a amulet of the Blood-eye](../../areas/qst/elftomb.qst#L2) | — |
| Abandoned Elven Homestead (`elvish`) | 4 | 6 | 3 | Yes | [4 × some unhatched spider eggs → a spider-shaped key forged from adamantium](../../areas/qst/elvish.qst#L76) | — |
| The Spires of the Elder Eternal Evil (`eternal`) | 6 | 2 | 0 | Fallback | [1 × the scale of the Death Serpent; 1 × the scale of the Night Serpent; 1 × the scale of the Shadow Serpent; other required items → the key of the Elder Eternal Evil](../../areas/qst/eternal.qst#L2) | — |
| The Keep of Evil (`evkeep`) | 1 | 0 | 0 | Fallback | [1 × a golden desk key → native reward/response](../../areas/qst/evkeep.qst#L2) | — |
| The Fields Between (`fields_between`) | 7 | 10 | 1 | Yes | [1 × the head of a wildmage; 1 × the head of a wildmage; 1 × the head of a wildmage; other required items → the bloody crown of wildmage heads](../../areas/qst/fields_between.qst#L95) | — |
| The Charcoal Palace (`firep`) | 1 | 3 | 0 | Yes | [1 × a magnificent pile of black and red dragonscales; 1 × a pair of battered and worn gauntlets → a pair of vampiric dragonscale gauntlets, the broken rays of morning Sunrise](../../areas/qst/firep.qst#L33) | block_dir, charcoal_guard, fruaack_shout, kossuth, zion_fnf |
| The Altar of the Firesworn (`firesworn_altar`) | 1 | 7 | 0 | Yes | [1 × the divine essence of blood; 1 × the divine essence of fire; 1 × the divine essence of mist; other required items → Tiliwibble's skeleton key of unlocking, a jagged hilt studded with gems](../../areas/qst/firesworn_altar.qst#L93) | — |
| Fishermans Wharf (`fishermans_wharf`) | 5 | 8 | 2 | Yes | [1 × a full-size eagle egg; 4 × a bundle of sticks; 3 × a soft beaver pelt → a petrified fanged snake](../../areas/qst/fishermans_wharf.qst#L14) | — |
| The Forgotten Forest (`forgotten_forest`) | 4 | 0 | 0 | Fallback | [1 × a chunk of meat → native reward/response](../../areas/qst/forgotten_forest.qst#L16) | — |
| Lair of the Gibberling King (`gibber`) | 2 | 3 | 0 | Yes | [1 × a wand of dismissal; 1 × an essence of Crymson → a robe of the earth](../../areas/qst/gibber.qst#L32) | — |
| Githzerai Stronghold (`githzer`) | 13 | 20 | 0 | Yes | [5 × a signet ring with a kingly crest → a bright marble key](../../areas/qst/githzer.qst#L283) | lucky_weapon |
| The Gagga'Jobo Cave System (`goblincave`) | 6 | 7 | 0 | Yes | [4 × a strip of chothe hide → a pair of goblin-made gloves](../../areas/qst/goblincave.qst#L46) | — |
| The Town of Moregeeth (`goblinht`) | 11 | 9 | 1 | Yes | [5 × a bat skull → a necklace of bat skulls](../../areas/qst/goblinht.qst#L144) | inn, world_quest |
| Golden Hall of the Crown (`gold_hal`) | 16 | 17 | 9 | Yes | [1 × a brown strip of bear hide; 1 × a grey strip of olyx hide → a small earthen statue](../../areas/qst/gold_hal.qst#L46) | inn, reliance_pegasus, world_quest |
| The Halfcut Hills (`halfcut`) | 13 | 15 | 4 | Yes | [1 × the orc's scalp; 1 × the drow's scalp; 1 × the goblin's scalp; other required items → a duergar belt](../../areas/qst/halfcut.qst#L177) | crossbow_ambusher |
| The Hall of the Ancients (`hall`) | 11 | 14 | 6 | Yes | [2 × a necklace of tiny steel links; 2 × the bangle of the Dreamer; 2 × an adamantium rock; other required items → Jadem's magical device of protection](../../areas/qst/hall.qst#L77) | akckx, artifact_stone, hoa_death, hoa_plat, hoa_sin, human_girl |
| The Mountain Settlement of the Harpies (`harpyht`) | 3 | 0 | 1 | Yes | [1 × a rusted key → some prisoner shackles](../../areas/qst/harpyht.qst#L9) | gargoyle_master, harpy_evil, harpy_good, money_changer |
| Harrow -The Gnome Village (`harrow`) | 8 | 4 | 1 | Yes | [1 × a token; 1 × leather alchemist sack; 1 × faerie dust → lucky alchemist sack](../../areas/qst/harrow.qst#L73) | inn |
| Zalkapfaan, City of the Headless Horde (`headless`) | 8 | 2 | 0 | Fallback | [1 × a purchase requisition; 1 × the scales of a sea serpent → platemail of the sea serpent](../../areas/qst/headless.qst#L42) | — |
| Heaven (`heavens`) | 9 | 3 | 0 | Fallback | [6 × a pile of brittle alloy; 1 × a gauze bandage → Token of the Gods](../../areas/qst/heavens.qst#L173) | annoying_mob, archer, artifact_biofeedback, artifact_hide, artifact_monolith, artifact_stone |
| The Behemoth Herders (`herders`) | 12 | 29 | 0 | Yes | [1 × some scales of diorite; 2 × a large piece of bone; 3 × a piece of a bone fragment → some bone-reinforced boots of diorite](../../areas/qst/herders.qst#L157) | — |
| Southern Coastal Highway (`highway`) | 3 | 8 | 0 | Yes | [1 × a harpy tooth; 1 × the hair from a harpy → an ear clasp of petrified dragon claws](../../areas/qst/highway.qst#L90) | hewards_mystical_organ, kearonor_hide, wand_of_wonder |
| The Caverns of Armageddon (`hunt`) | 18 | 15 | 2 | Yes | [1 × a pair archangel wings; 1 × a beholder eyestalk; 1 × a wispy tendril of flame; other required items → the amulet of kilospanatis, a hazy, blue amulet](../../areas/qst/hunt.qst#L310) | — |
| IceCrag Castle (`icecrag`) | 11 | 55 | 1 | Yes | [1 × a red fox pelt; 1 × an ogres brain; 1 × a plate of clams in a spicy black bean sauce; other required items → a map of Icecrag Castle, a juicy onion](../../areas/qst/icecrag.qst#L141) | artifact_hide, ice_artist, ice_bodyguards, ice_cleaning_crew, ice_commander, ice_garden_attendant |
| Ice Tower (`icetower`) | 2 | 3 | 0 | Yes | [1 × a silver wedding ring → some ivory bracers](../../areas/qst/icetower.qst#L34) | — |
| Ixarkon (`ixarkon`) | 3 | 15 | 1 | Yes | [1 × a red skullcap → a small spider amulet of Lloth](../../areas/qst/ixarkon.qst#L116) | illithid_teleport_veil, inn, money_changer, pet_shops |
| Ixxillikor (`ixxillikor`) | 2 | 5 | 0 | Yes | [1 × the still-beating heart of Bel; 1 × the orb of unmaking; 1 × A mystic runed stone tablet → the scroll of Power](../../areas/qst/ixxillikor.qst#L10) | — |
| The Jade Empire (`jade`) | 37 | 3 | 10 | Yes | [5 × a rice harvest → a full harvest bag](../../areas/qst/jade.qst#L71) | crew_shop_proc, ship_shop_proc |
| The Rice Fields (`jademini`) | 4 | 0 | 0 | Fallback | [native payment/conditions → a map of jade](../../areas/qst/jademini.qst#L2) | archer |
| Jindon the Deathwood Forest (`jin`) | 1 | 1 | 0 | Fallback | [1 × arms of a Thri-kreen → an eerie longsword named 'Illithid Bane'](../../areas/qst/jin.qst#L8) | jindo_ticket_master |
| Jotunheim (`jotun`) | 15 | 34 | 0 | Yes | [1 × an eerily glowing jade bracelet; 1 × a jagged lightning sword; 1 × a barbed whip; other required items → a wooden spear entwined with glowing runes](../../areas/qst/jotun.qst#L39) | deva_cloak, faith, giantbane, icicle_cloak, jotun_balor, jotun_mimer |
| The 222nd Layer of the Abyss (`juiblex`) | 23 | 25 | 3 | Yes | [1 × the head of the lost wildmage; 1 × the head of the lost wildmage; 1 × the head of the lost great wildmage; other required items → a necklace of wildmage scalps](../../areas/qst/juiblex.qst#L593) | doombringer, flow_amulet, juiblex_grid_mob_generator, juiblex_one, mask_of_wildmagic, slime_lake |
| Varathorn Keep (`kastle`) | 3 | 3 | 0 | Yes | [2 × a bloody talon of a night crawler → a twisted blood dagger of the night crawler](../../areas/qst/kastle.qst#L24) | nightcrawler_dagger, zarthos_vampire_slayer |
| The Stone Tomb of Kelek (`kelek`) | 3 | 2 | 0 | Fallback | [1 × the ears of a troll; 1 × the skull of an ogre; 1 × the shriveled hand of a duergar → a hastily scribbled note](../../areas/qst/church.qst#L62) | deliverer_hammer, world_quest |
| Khildarak Stronghold (`khildarak`) | 2 | 8 | 0 | Yes | [1 × a steeders egg sack → native reward/response](../../areas/qst/khildarak.qst#L36) | archer, assoc_founder, devour, guild_guard, inn, khildarak_warhammer |
| Killing Fields (`killing_fields`) | 1 | 2 | 0 | Fallback | [1 × a small note made of fine paper → native reward/response](../../areas/qst/killing_fields.qst#L14) | — |
| Kimordril (`kimordril`) | 4 | 5 | 0 | Yes | [1 × a potato; 1 × a carrot; 1 × a cutting knife → native reward/response](../../areas/qst/kimordril.qst#L20) | archer, inn, kimordril_shout, money_changer, world_quest |
| Kobold Settlement (`kobold`) | 4 | 6 | 2 | Yes | [8 × a small nugget of silver → a large block of solid silver](../../areas/qst/kobold.qst#L65) | chicken, inn, item_switch, kobold_priest, stone_crumble, stone_golem |
| Krethik Keep (`krethik`) | 3 | 2 | 0 | Fallback | [1 × a small note → native reward/response](../../areas/qst/krethik.qst#L34) | mindbreaker |
| Lord Krimeneha's Mansion (`krimman`) | 9 | 9 | 4 | Yes | [1 × a boy's signet ring; 1 × a girl's broach; 1 × an elegant silver broach → a heavy iron key, a silver bastard sword](../../areas/qst/krimman.qst#L101) | — |
| Labyrinth of No Return (`labyrinth`) | 3 | 8 | 0 | Yes | [1 × a bat wing; 1 × a tuft of sasquatch hair; 1 × a beetle shell; other required items → a sigil of the Golden Flame](../../areas/qst/labyrinth.qst#L85) | — |
| Lava Springs (`lava`) | 1 | 1 | 0 | Fallback | [5 × a chunk of lava rock; 1 × a voucher for 'a flaming longsword' → a flaming longsword emblazoned 'Cinder'](../../areas/qst/lava.qst#L14) | inn |
| The Underground Lava Caves (`lavcav`) | 2 | 4 | 1 | Yes | [1 × a pair of platinum horns → a golden wrist chain](../../areas/qst/lavcav.qst#L43) | — |
| The Arcaneum of L'srillizzin (`library`) | 6 | 14 | 0 | Yes | [1 × a mystical tome with glowing elemental glyphs; 1 × an illithid-skin spellbook of wyld magick; 1 × a leather-bound book covered in shifting shadows; other required items → a mystical tome of the otherworldly scholars](../../areas/qst/library.qst#L137) | — |
| The Lizardman Swamps of Clavikord (`lizard`) | 2 | 2 | 0 | Fallback | [1 × the severed head of Sslith → a bracelet of lights](../../areas/qst/lizard.qst#L14) | — |
| Defense of Longhollow (`long`) | 15 | 59 | 6 | Yes | [1 × the bloody head of a knight; 1 × the bloody head of the necromancer; 1 × the bloody head of the chieftan; other required items → a bracer bearing the Longhollow symbol, an exquisite moonstone](../../areas/qst/long.qst#L333) | — |
| The Ancient Halls of Ironstar (`lornecro`) | 7 | 15 | 2 | Yes | [1 × the mold of a small mithril parrying dagger; 1 × a scroll of demonhide; 1 × a broken demonic weapon called 'The Fury of Demons' → a blazing dagger called 'The Fury of Demons'](../../areas/qst/lornecro.qst#L124) | — |
| The Tower of Darkness (`lortower`) | 7 | 11 | 2 | Yes | [5 × a black iron two-handed sword; 1 × a gold locket → a black iron shield of Dubneth](../../areas/qst/lortower.qst#L119) | — |
| Lylr-Meop (`lylr`) | 1 | 1 | 0 | Fallback | [1 × an ogre's scalp → a pair of drow skin boots](../../areas/qst/lylr.qst#L11) | inn, money_changer, world_quest |
| The Para-Elemental Plane of Magma (`magma`) | 1 | 2 | 0 | Fallback | [1 × the smoldering heart of an ancient magma drake → a wand of writhing magma, Palenian's pipe of neverending flavors](../../areas/qst/magma.qst#L34) | ship_shop_proc |
| Malch'Hor Ganl the Goblin City (`malch`) | 3 | 3 | 0 | Yes | [3 × a dark shadowy circle → native reward/response](../../areas/qst/malch.qst#L24) | — |
| The Forgotten Mansion (`mansion`) | 3 | 12 | 0 | Yes | [1 × the bloody head of Gaultair; 1 × the horn of Tyrlos → a golden key with the Englehardt emblem](../../areas/qst/mansion.qst#L126) | — |
| The Maze of Undead Army (`maze_are`) | 7 | 4 | 0 | Yes | [5 × a bloody finger → a golden key to the Catacomb](../../areas/qst/maze_are.qst#L96) | — |
| Mazzolin (`mazzolin`) | 1 | 2 | 0 | Fallback | [1 × a hematite shard; 1 × a hematite shard; 1 × a hematite shard; other required items → the sleeves of zephyrs, the wand of the Spider Queen](../../areas/qst/mazzolin.qst#L15) | — |
| Menden-on-the-Deep (`menden`) | 1 | 3 | 0 | Yes | [1 × the holy elven relic of life → native reward/response](../../areas/qst/menden.qst#L19) | crystal_golem_die, hippogriff_die, llyms_altar, magic_pool, menden_figurine, menden_figurine_die |
| The Halfling Silver Mine (`mining`) | 1 | 1 | 0 | Fallback | [1 × a balor's whip → a badge of purity](../../areas/qst/mining.qst#L8) | poison, wanderer |
| Mini Zones (`minizones`) | 7 | 11 | 1 | Yes | [1 × a pair of ghostly sleeves; 5 × a huge blood crystal → a set of jagged blood crystal arm plates](../../areas/qst/minizones.qst#L143) | dryad, navagator, pet_shops, sword_named_magik, world_quest |
| The Deep Ravine of Passage (`minopass`) | 4 | 9 | 0 | Yes | [1 × the unholy relic of life and death; 1 × the holy elven relic of life → a spectral holy symbol of Berronar Truesilver](../../areas/qst/minopass.qst#L130) | — |
| Forest of Mir (`mir`) | 1 | 1 | 0 | Fallback | [1 × a light and dark scroll → a satanic token](../../areas/qst/mir.qst#L10) | amphisbean, blade_of_paladins, blue_wyrm_shout, elfdawn_sword, fade_drusus, flame_of_north_sword |
| Myrabolus (`mira`) | 16 | 1 | 2 | Yes | [1 × a token of light; 1 × a token of evil; 1 × a token of undeath → the mage boots of the maelstrom, the axe of the maelstrom](../../areas/qst/mira.qst#L140) | crew_shop_proc, generic_parry_proc, inn, master_set, money_changer, ship_shop_proc |
| The Shadow Forest (`mist`) | 1 | 1 | 0 | Fallback | [1 × the bloody head of Darnac → a crown of light](../../areas/qst/mist.qst#L6) | mist_protect |
| The Chasm of the Misty Vale (`mist_chasm`) | 4 | 3 | 0 | Yes | [2 × a small salamander scale; 2 × a large salamander scale → a salamander scale shield](../../areas/qst/mist_chasm.qst#L69) | — |
| Mistywood (`mistywood`) | 3 | 14 | 0 | Yes | [1 × a bear claw necklace → native reward/response](../../areas/qst/mistywood.qst#L32) | — |
| Mitashi - Capital City of the Jade Empire (`mitashi`) | 1 | 3 | 0 | Fallback | [1 × a katana called 'Shadows Breath'; 1 × a katana called 'The North Wind'; 1 × a katana called 'Divine Fury'; other required items → a katana called 'Retribution'](../../areas/qst/mitashi.qst#L48) | — |
| Du'Maathe Castle (`mntcastl`) | 8 | 12 | 1 | Yes | [1 × a small handful of fine sand; 1 × a recipe for a granular potion → a granular potion](../../areas/qst/mntcastl.qst#L237) | inn |
| Moonshae Island (`moonshae`) | 2 | 5 | 0 | Yes | [1 × the lost sword of Cymrych Hugh → a scarlet ring](../../areas/qst/moonshae.qst#L18) | sister_knight |
| Neverwinter Woods (`moria`) | 5 | 2 | 0 | Yes | [1 × the amethyst rune; 1 × the sapphire rune; 1 × the diamond rune; other required items → a ruby-encrusted eyepatch](../../areas/qst/moria.qst#L65) | nw_agatha, nw_ammaster, nw_ansal, nw_brock, nw_builder, nw_carpen |
| Mosswood (`moss`) | 1 | 1 | 0 | Fallback | [1 × a large beet → native reward/response](../../areas/qst/moss.qst#L9) | — |
| The Mountain of the Banished (`mount`) | 3 | 4 | 0 | Yes | [1 × the essence of Tolog; 1 × the essence of Pakar; 1 × the essence of Zooox; other required items → the symbol of chaos, the visor of destruction](../../areas/qst/mount.qst#L56) | — |
| Mountain Tracts of the Untamed (`mountaintracks`) | 4 | 4 | 1 | Yes | [1 × a green dragon scale; 1 × a vampire's tooth → a glowing green potion](../../areas/qst/mountaintracks.qst#L57) | — |
| Miaeril Village (`mril`) | 3 | 3 | 0 | Yes | [5 × a freshly killed salmon → the lost book of 'Magic'](../../areas/qst/mril.qst#L48) | — |
| the Mushroom Caverns (`mushroom_caverns`) | 3 | 11 | 2 | Yes | [1 × a half of an ancient amulet; 1 × a half of an ancient amulet → the bronze Zarbonesti seal of Kryz'Kyssik, an adamantium-hafted flail](../../areas/qst/mobs_underdark.qst#L127) | — |
| Myconid Mushroom Forest (`myconid`) | 1 | 2 | 0 | Fallback | [1 × spores of the giant mushroom → native reward/response](../../areas/qst/myconid.qst#L18) | — |
| Myrloch Vale (`myrloch_vale`) | 4 | 4 | 0 | Yes | [1 × an old key → a flaming key](../../areas/qst/myrloch_vale.qst#L33) | inn, item_switch, unblock_on_death |
| Negative Material Plane (`negplane`) | 2 | 6 | 0 | Yes | [1 × the star of ash; 1 × the star of salt; 1 × the star of dust; other required items → an elaborate rune covered sword named 'Mournblade', the key of unmaking](../../areas/qst/negplane.qst#L24) | artifact_stone, elvenkind_cloak, neg_orb, neg_pocket, orb_of_destruction, sanguine |
| New Cave city (`new_cavecity`) | 1 | 0 | 0 | Fallback | [1 × A mystic runed stone tablet; 1 × the still-beating heart of Bel; 1 × the orb of unmaking → the scroll of Intelligence](../../areas/qst/new_cavecity.qst#L16) | dranum_jurtrem, torment |
| Ailvio, Duris Newbie Outpost (`newbie`) | 116 | 160 | 4 | Yes | [1 × eyes of a bullfrog; 1 × a green herb; 1 × a few drops of dragons blood; other required items → a ceramic chillum, a hefty bag](../../areas/qst/newbie.qst#L529) | burbul_map_obj, chyron_search_obj, inn, newbie_portal, newbie_spellup_mob, pet_shops |
| The City of Newhaven (`newhaven`) | 9 | 2 | 0 | Yes | [1 × a frayed cloth collar; 1 × the bloody heart of Mixt → a Veldian collar](../../areas/qst/newhaven.qst#L331) | inn, magic_pool |
| The Village of New Hope (`newhope`) | 4 | 15 | 0 | Yes | [1 × a piece of mithral; 1 × a mithral dagger → a shining mithral dagger](../../areas/qst/newhope.qst#L60) | tentacler_death |
| the Mountain of Peril Peaks (`nexus`) | 10 | 11 | 3 | Yes | [1 × a python scale; 1 × a serpent scale; 1 × a pair of slimy scales of an anaconda → some slimy reptilian snakescales](../../areas/qst/nexus.qst#L36) | — |
| The Reliquary Nexus of the Roper Den (`nexus_roper`) | 1 | 1 | 0 | Fallback | [4 × an ancient roper tentacle → a flaming key](../../areas/qst/nexus_roper.qst#L6) | — |
| Northern Lakes and Settlements (`nlakes`) | 6 | 7 | 2 | Yes | [2 × a green scale; 1 × a jeweled demonic amulet → an inspiring visage of a Dragon](../../areas/qst/nlakes.qst#L44) | — |
| The Cimmerian Nomad Encampment (`nomads`) | 2 | 3 | 1 | Yes | [1 × the severed head of the half-orc shaman; 1 × the severed head of the half-orc conjurer; 1 × a hallowed Cimmerian ring → the diamond crown of Winduin](../../areas/qst/nomads.qst#L64) | — |
| Ny'Neth's Stronghold (`nyneth2`) | 1 | 1 | 0 | Fallback | [1 × a chunk of illithite; 1 × a chunk of malicite; 1 × a chunk of evilite; other required items → a mine key](../../areas/qst/nyneth2.qst#L8) | lifereaver |
| Ny'Neth's Stronghold Continued (`nyneth3`) | 1 | 1 | 0 | Fallback | [1 × an elusive yet immortal soul; 1 × an elusive yet immortal soul; 1 × an elusive yet immortal soul; other required items → a key of utter blackness](../../areas/qst/nyneth3.qst#L6) | generic_shield_block_proc, nyneth, platemail_of_defense, ring_of_regeneration, stormbringer |
| Tribal Oasis (`oasis`) | 9 | 12 | 0 | Yes | [1 × the still-beating heart of Bel; 1 × the orb of unmaking; 1 × A mystic runed stone tablet → the scroll of Constitution](../../areas/qst/oasis.qst#L11) | — |
| The Obsidian Citadel (`obcita`) | 5 | 3 | 0 | Yes | [1 × a minuscule shard of shadow → a shadowsteel ring](../../areas/qst/obcita.qst#L37) | obsid_cit_death_knight, obsid_cit_satar_ghulan |
| Enclave of the Opal Phoenix (`opalphoenix`) | 3 | 3 | 2 | Yes | [1 × a large container of fine sand → a midnight blue tome with silver bindings, an eagle feather quill](../../areas/qst/opalphoenix.qst#L14) | alch_bag, alch_rod, circlet_of_light, dragon_skull_helm, head_guard_sword, ljs_armor |
| Orrak (`orrak`) | 1 | 1 | 0 | Fallback | [1 × a glittering staff → a bracelet of red rocks](../../areas/qst/orrak.qst#L8) | — |
| Pharr Valley (`pharrvly`) | 2 | 14 | 0 | Yes | [1 × an apple; 1 × an orange; 1 × a crow's nest → native reward/response](../../areas/qst/pharrvly.qst#L87) | — |
| Pine Hollow (`pineholl`) | 7 | 10 | 1 | Yes | [1 × a green-hued gold dragon scale; 1 × a gold dragon scale; 1 × the brown-hued scale of an elder gold dragon; other required items → a dragonscale earring, a golden dagger](../../areas/qst/pineholl.qst#L35) | — |
| Pharr Valley Swamp (`pods`) | 4 | 24 | 0 | Yes | [6 × a dark feather; 1 × a strip of neberihide → a birdfeather headdress](../../areas/qst/pods.qst#L187) | — |
| The Prisons of Carthapia (`prison`) | 2 | 6 | 0 | Yes | [1 × some scales of the Great Dragon Smaug; 1 × some scales of the Great Dragon Smaug; 1 × some scales of the Great Dragon Smaug → the shield of flames](../../areas/qst/prison.qst#L47) | flaming_axe_of_azer, nexus, warden_shout |
| Prison of Fort Boyard (`prisonb`) | 1 | 5 | 0 | Yes | [1 × a white scale; 1 × a blue scale; 1 × a green scale; other required items → a cloak of dragons](../../areas/qst/prisonb.qst#L43) | — |
| Lair of the Purple Worm (`pworm`) | 1 | 5 | 0 | Yes | [1 × the family amulet of Drakhov; 7 × some gargantuan worm hide → a pair of drider skin leggings](../../areas/qst/pworm.qst#L31) | — |
| Neverwind Valley (`pyramid`) | 3 | 9 | 0 | Yes | [1 × a speckled egg; 1 × a blue egg; 1 × a white egg; other required items → a ring of protection from evil](../../areas/qst/pyramid.qst#L84) | — |
| The Docks of Quietus Quay (`quietus`) | 16 | 14 | 4 | Yes | [1 × a short note; 1 × the bloody head of Aresliean → a sleek mithril dagger](../../areas/qst/quietus.qst#L107) | crew_shop_proc, inn, ship_shop_proc, world_quest |
| Castle Ravenloft (`ravenloft`) | 5 | 17 | 0 | Yes | [1 × an abyssal essence; 1 × a Zionyn essence → a key to the Ravenloft royal courtyard](../../areas/qst/ravenloft.qst#L18) | artifact_shadow_shield, ravenloft_bell, ravenloft_vistani_shout, shimmer_shortsword |
| The Ravenloft Catacombs (`ravenloft2`) | 37 | 135 | 1 | Yes | [5 × a spectral coin called 'A Fortune of Ravenloft'; 1 × a mystical scroll, 'Favor of the Chaplain' → the priestly stole of the /> Dark Alliance <\, the crozier of the hollow sun](../../areas/qst/ravenloft2.qst#L1507) | — |
| Drustl's Yerdonia Enslaved (`raxthan`) | 10 | 14 | 0 | Yes | [3 × a cave shroom → an azure potion](../../areas/qst/raxthan.qst#L398) | — |
| Faerie Realm (`realm`) | 7 | 10 | 1 | Yes | [1 × the blazing heat of a forge; 1 × the billowing wind of a forge; 1 × the earthen hammer of forging; other required items → a tightly wrapped vellum scroll named 'Fix'](../../areas/qst/realm.qst#L125) | bridge_troll, cricket, faerie, finn, tree_spirit |
| Rift Valley Jungle (`rftjngle`) | 28 | 37 | 2 | Yes | [5 × a quetzel feather; 5 × a quetzel feather; 5 × a quetzel feather → a quetzel feather cloak](../../areas/qst/rftjngle.qst#L615) | world_quest |
| Rogue Plains (`roguerai`) | 9 | 3 | 3 | Yes | [1 × a strip of buffalo flesh; 1 × a strip of buffalo flesh; 1 × a strip of buffalo flesh; other required items → some enormous buffalo hides](../../areas/qst/roguerai.qst#L29) | master_set |
| Village of Refugees (`ruins`) | 2 | 11 | 0 | Yes | [1 × a pitch-black raven's feather; 1 × a bright white feather; 1 × a red-tailed hawk's feather; other required items → a soft feathered ring](../../areas/qst/ruins.qst#L16) | — |
| Sarmiz'Duul (`sarmiz`) | 8 | 12 | 6 | Yes | [1 × a scroll covered with magical formulas; 1 × a demon's heart; 1 × a bag of magical dust; other required items → a staff of power](../../areas/qst/sarmiz.qst#L328) | crew_shop_proc, erzul_proc, inn, money_changer, ship_shop_proc |
| The Savannah of Broken Trusts (`savannah`) | 17 | 17 | 6 | Yes | [3 × an elephant's tusk → an ivory curio](../../areas/qst/savannah.qst#L406) | — |
| The Scorched Valley (`scorchvalley`) | 9 | 11 | 4 | Yes | [1 × the red ring of perfection; 1 × the blue ring of perfection; 1 × the green ring of perfection; other required items → a necklace of perfection](../../areas/qst/scorchvalley.qst#L256) | artifact_invisible, block_up, yeenoghu |
| Sea Kingdom (`seakngdm`) | 1 | 1 | 0 | Fallback | [1 × a half of a silver amulet → a sapphire eye](../../areas/qst/seakngdm.qst#L9) | SeaKingdom_Tsunami, glowing_necklace |
| The Great Shaboath (`shabo`) | 2 | 7 | 0 | Yes | [1 × a sphere of crystallized magic; 1 × a sphere of crystallized magic; 1 × a sphere of crystallized magic; other required items → an insubstantial key](../../areas/qst/shabo.qst#L87) | aboleth_pendant, artifact_invisible, artifact_stone, cyvrand_shout, finslayer_air, flayed_mind_mask |
| Shady Grove (`shady`) | 3 | 5 | 0 | Yes | [1 × a diamond-studded collar → the blood sword of the ancients](../../areas/qst/shady.qst#L11) | hardworking_fisherman, inn, orcish_jailkeeper, orcish_woman, pet_shops, stray_dog |
| The Shaughin Settlement (`shaughin`) | 4 | 2 | 0 | Fallback | [2 × the heart of a constrictor → a potion of constrictor blood](../../areas/qst/shaughin.qst#L40) | inn |
| The Ship Yards (`shipy`) | 26 | 20 | 2 | Yes | [5 × a snapjaw turtle shell; 5 × a fire gland → native reward/response](../../areas/qst/shipy.qst#L38) | crew_shop_proc, money_changer, ship_shop_proc |
| The Orcish Slave Camp (`shortc`) | 3 | 4 | 1 | Yes | [1 × a small metallic key → a large steak](../../areas/qst/shortc.qst#L16) | — |
| The Para-Elemental Plane of Smoke (`smoke`) | 6 | 8 | 2 | Yes | [1 × A wretched broadsword named 'Discontent'; 1 × A wretched broadsword named 'Hate' → A massive double-bladed greatsword named 'Hate & Discontent'](../../areas/qst/smoke.qst#L105) | — |
| Shairak and Smokeveil Forest (`smokev`) | 11 | 32 | 1 | Yes | [1 × the black heart of a Tentabeast; 1 × a bloody dragon talon → a blood soaked helm bearing a skull crest](../../areas/qst/smokev.qst#L570) | — |
| Valley of the Snow Ogres (`snogres`) | 9 | 6 | 4 | Yes | [6 × a substantial chunk of remorhaz hide; 1 × an onyx-hilted cold-iron claymore; 1 × an onyx-hilted cold-iron greatsword → a magical pair of remorhaz hide vambraces](../../areas/qst/snogres.qst#L159) | berserker_toss, block_dir, flesh_golem_repop, hellfire_axe, illithid_whip, remo_burn |
| Vast Hidden Grove (`solonar`) | 15 | 12 | 10 | Yes | [1 × a small glowing orb; 1 × some mithril linked scales; 1 × some radiant glowing lavander thread; other required items → some robes of the arch-magi](../../areas/qst/solonar.qst#L88) | inn |
| Storm Port Stronghold (`spshold`) | 4 | 0 | 1 | Yes | [1 × a pile of coal; 1 × a broken furnance valve → a travel ticket](../../areas/qst/spshold.qst#L31) | crew_shop_proc, master_set |
| Strathor Valley of the Storm Giants (`stormht`) | 1 | 8 | 0 | Yes | [1 × a bronze sword → a mighty crown of thunder](../../areas/qst/stormht.qst#L63) | world_quest |
| Storm Port (`stormport`) | 2 | 1 | 0 | Fallback | [1 × a set of shackles from the prison; 1 × a captain's badge; 1 × a verbeeg tooth; other required items → the magnificent mantle of Storm Port](../../areas/qst/stormport.qst#L12) | clear_epic_task_spec, crew_shop_proc, inn, ship_shop_proc |
| The Temple of the Sun (`suntmpl`) | 3 | 3 | 0 | Yes | [1 × the twisted heart of a pine; 1 × the demented mind of a bear → a vine covered key](../../areas/qst/suntmpl.qst#L11) | — |
| The Surface Realm of Duris (`surface`) | 31 | 36 | 6 | Yes | [1 × the crumbling locket of fire; 1 × the crumbling locket of water; 1 × the crumbling locket of earth; other required items → the mystical sash of the Netherworld, a mystical key](../../areas/qst/surface.qst#L71) | Baltazo, goodie_guardian, ship_shop_proc, tharnrifts_portal, wh_corpse_to_object |
| The Depths of Duris (`surfacekeeps`) | 15 | 43 | 1 | Yes | [10 × bodypart to be created → a commendation token of stealth, a smoke bomb potion](../../areas/qst/surfacekeeps.qst#L562) | wh_corpse_decay, wh_corpse_to_object |
| The Minizones of the Surface (`surfacemini`) | 25 | 32 | 3 | Yes | [8 × a fire gland → an elixir of the pyro-mage](../../areas/qst/surfacemini.qst#L261) | collar_flames, collar_frost, elemental_wand |
| The Dark Stone Tower of the Northern Realms (`teka2`) | 1 | 3 | 0 | Fallback | [1 × a small rhinestone → a shiny golden mask of Teka, a flaming mace of the Ruzdo](../../areas/qst/teka2.qst#L32) | — |
| Temple of Flames (`temple`) | 6 | 33 | 0 | Yes | [2 × a yellow dagger; 1 × a green token; 1 × a blue wooden sword → native reward/response](../../areas/qst/temple.qst#L233) | temple_illyn |
| Tharnadia - City of Humans (`tharnadia`) | 19 | 24 | 1 | Yes | [1 × A small reed flute; 1 × A small clumsily made mandolin; 1 × a small bamboo lyre → native reward/response](../../areas/qst/tharnadia.qst#L239) | assoc_founder, crew_shop_proc, die_roller, inn, janitor, learn_tradeskill |
| The Tharnadian Ruin (`tharnadian_ruin`) | 5 | 7 | 0 | Yes | [1 × the remains of Lord Braddistock; 1 × the remains of the master of the house; 1 × the remains of the warrior guildmaster; other required items → a shiny key](../../areas/qst/tharnadian_ruin.qst#L71) | bouncer_four, bouncer_one, bouncer_three, bouncer_two, dagger_submission, frost_elb_dagger |
| Thetis's Realm (`thetis`) | 3 | 2 | 0 | Fallback | [1 × a torn treasure map → a deep-sea spade, a torn treasure map](../../areas/qst/thetis.qst#L37) | — |
| Tiamat (`tiamat`) | 8 | 3 | 0 | Yes | [1 × a fragment of a ruby encrusted key; 1 × a fragment of a ruby encrusted key; 1 × a fragment of a ruby encrusted key → a ruby-encrusted key](../../areas/qst/tiamat.qst#L19) | block_dir, tiamat_human_to_rareloads, zion_shield_absorb_proc |
| Lost City of Tikitzopl (`tikit`) | 1 | 0 | 0 | Fallback | [1 × a scared kitty cat → the temple key](../../areas/qst/tikit.qst#L2) | — |
| Lost Temple of Tikitzopl (`tikitt`) | 29 | 9 | 11 | Yes | [10 × a standard issue sword → a magical steel sword](../../areas/qst/tikitt.qst#L110) | artifact_hide, madman_mangler, madman_shield, mentality_mace, unmulti_altar |
| The Kingdom of Torg (`torg`) | 15 | 15 | 4 | Yes | [1 × Llznixor's cleaned skull symbol; 1 × Dorn's steel forehead plate; 1 × Tibor's tankard; other required items → a curtain of elemental fire, the Legend of The Lore Keeper](../../areas/qst/torg.qst#L188) | inn, lanella_heart, timoro_die |
| City of Torrhan (`torrhan`) | 26 | 18 | 14 | Yes | [1 × a dirty seashell → a dark purple potion, some blackened shark-skin gloves](../../areas/qst/torrhan.qst#L140) | crew_shop_proc, ship_shop_proc |
| Tower of High Sorcery (`tower`) | 2 | 2 | 0 | Fallback | [1 × a piece of calcite; 1 × a large flask; 1 × a small diamond tattoo → a mysterious ear stud](../../areas/qst/tower.qst#L15) | bulette |
| The Trakkia Mountains (`trakkia`) | 4 | 2 | 0 | Fallback | [4 × a grangle root → a strange, circular gem](../../areas/qst/trakkia.qst#L52) | pet_shops |
| Tribal Forest (`tribal`) | 10 | 15 | 2 | Yes | [1 × a severed left leg; 1 × a severed left hand; 1 × a severed right hand; other required items → a redwood longbow, an elegant deerskin quiver](../../areas/qst/tribal.qst#L266) | amethyst_orb |
| The Transparent Tower (`trnsptow`) | 4 | 26 | 2 | Yes | [3 × a pale purple token; 1 × the scepter of illusion → a key made of mist](../../areas/qst/trnsptow.qst#L195) | artifact_stone, trans_tower_shadow_globe, transp_tow_acerlade, zion_light_dark |
| Troll Caves (`troll_caves`) | 5 | 5 | 2 | Yes | [1 × small obsidian stones → an obsidian dagger](../../areas/qst/troll_caves.qst#L61) | — |
| The Troll Hills (`troll_hills`) | 1 | 3 | 0 | Fallback | [1 × a small stone ogre idol → a vial of boiling goo](../../areas/qst/troll_hills.qst#L23) | bridge_troll |
| The Twin Towers (`ttowers`) | 7 | 9 | 0 | Yes | [1 × the bloody heart of Mixt; 1 × the bloody heart of Blaevyna; 1 × the bloody heart of Lyena → Lord Talfyn's Armor of Darkness](../../areas/qst/ttowers.qst#L112) | — |
| Tundra (`tundra`) | 7 | 12 | 1 | Yes | [1 × an old book; 1 × a dark green book; 1 × a dark magenta book; other required items → a pair of snowy adventurer boots](../../areas/qst/tundra.qst#L40) | inn |
| Turolopolis Zoo (`turolzoo`) | 1 | 1 | 0 | Fallback | [1 × a saber tooth; 1 × a gorilla tooth → a thin helmet of bark](../../areas/qst/turolzoo.qst#L11) | — |
| Twin Towers Forest (`twin_towers_forest`) | 84 | 58 | 24 | Yes | [10 × a fox fur → a fox fur cape](../../areas/qst/twin_towers_forest.qst#L771) | forest_animals, forest_corpse, gardener_block |
| A Dark and Twisted Wood (`twstwd`) | 1 | 3 | 0 | Fallback | [1 × a full suit of black platemail; 1 × a broken mithral lance → an iridescent faerie collar](../../areas/qst/twstwd.qst#L29) | — |
| The Twisting Tunnels of the Durian Underdark (`underdark`) | 2 | 3 | 0 | Yes | [1 × the first half of an ancient amulet; 1 × the second half of an ancient amulet → the bronze Zarbonesti seal of Kryz'Kyssik](../../areas/qst/underdark.qst#L35) | purple_worm |
| The Ruins of Undermountain (`undermountain`) | 2 | 3 | 1 | Yes | [1 × a crude note → a scimitar named 'Convalescence'](../../areas/qst/undermountain.qst#L13) | flame_of_north, flying_dagger, generic_drow_eq, generic_parry_proc, helmed_horror, iron_flindbar |
| The Underworld (`underworld`) | 1 | 3 | 0 | Fallback | [1 × the unholy relic of life and death → native reward/response](../../areas/qst/underworld.qst#L18) | hammer, magic_pool, piercer, purple_worm, underdark_track |
| Vargan II (`v2`) | 1 | 2 | 0 | Fallback | [1 × an ancient hilt; 1 × an ancient cross-piece; 1 × a broken blade → the sword of Vurlok](../../areas/qst/v2.qst#L13) | — |
| The Valoisian Castle (`val`) | 8 | 5 | 1 | Yes | [1 × a battle mace; 1 × a huge polearm; 1 × some steel sleeves; other required items → a pair of leggings of clan crunch head](../../areas/qst/val.qst#L38) | — |
| Phantasmagoric Caverns (`valdrak`) | 1 | 2 | 0 | Fallback | [1 × a blood soaked longsword; 1 × a blood stained claymore; 1 × a heart of living darkness → native reward/response](../../areas/qst/valdrak.qst#L40) | — |
| Valley of Crushk (`valley_crushk`) | 1 | 1 | 0 | Fallback | [1 × a bandit shiv → native reward/response](../../areas/qst/valley_crushk.qst#L16) | — |
| Vargan (`vargan`) | 1 | 5 | 0 | Yes | [2 × an ancient shoulder plate; 1 × an ancient breast plate → a suit of ancient dwarven plate](../../areas/qst/vargan.qst#L26) | — |
| Vecna's Tomb (`vecna`) | 2 | 2 | 0 | Fallback | [1 × a rotting brain → the breastplate of preservation](../../areas/qst/vecna.qst#L11) | block_dir, chressan_shout, mob_vecna_procs, vecna_black_mass, vecna_boneaxe, vecna_bubble_room |
| vehicles (`vehicles`) | 255 | 19 | 1 | Fallback | [2 × a gnomish shopkeepers token; 5 × item 400941 → a crystal harp named 'The Vokstron'](../../areas/qst/vehicles.qst#L2545) | — |
| Verspin (`verspin`) | 12 | 9 | 1 | Yes | [5 × a small gnomish totem → an extemely large pair of silken pants](../../areas/qst/verspin.qst#L20) | crew_shop_proc, stat_shops |
| Village of Werrun (`werrun`) | 4 | 16 | 0 | Yes | [1 × an old book by Revan → a small white key](../../areas/qst/werrun.qst#L80) | world_quest |
| The City of Winterhaven (`wh`) | 221 | 191 | 87 | Yes | [1 × a strange glowing flint of armor; 1 × the gnomish orb of binding; 1 × a pair of magical mithril legplates; other required items → the legplates of the Storm](../../areas/qst/wh.qst#L1170) | artifact_stone, attribute_scroll, blackjack_table, blur_shortsword, board, buckler_saints |
| The Ruins of Turolopolis (`willem`) | 6 | 19 | 1 | Yes | [1 × a brown Glory Badge; 1 × a green Glory Badge; 1 × a blue Glory Badge; other required items → a lesser bloodsaber](../../areas/qst/willem.qst#L117) | — |
| Woodseer (`woodseer`) | 7 | 11 | 0 | Yes | [1 × some raw turtle meat → a bowl of turtle soup](../../areas/qst/woodseer.qst#L140) | artifact_invisible, guild_guard, inn, pet_shops, world_quest |
| The Caverns of the Worms (`worms`) | 16 | 17 | 0 | Yes | [1 × a purple wormhide; 1 × a glowing wormhide; 2 × a piece of red wormskin; other required items → a thick wormhide-plated shield](../../areas/qst/worms.qst#L187) | — |
| The Temple to Skrentherlog (`yuan_ti`) | 1 | 9 | 0 | Yes | [1 × blood of Skrentherlog → the scimitar of speed](../../areas/qst/yuan_ti.qst#L51) | dragonarmor, drowcrusher, squelcher |

## Areas without native Q contracts

These are not automatically empty of stories. Addressable dialogue and special assignments
identify candidates for explicit semantic adapters. Do not invent turn-in achievements.

| Area / source | M | Authored | Assigned special leads |
| --- | ---: | --- | --- |
| The Ruins of Port Skythic (`PortSkythic`) | 0 | No | — |
| Ako Village (`ako`) | 0 | No | ako_cow, ako_hypersquirrel, ako_songbird, ako_vulture, ako_wildmare |
| An Ancient Bridge (`ancientb`) | 0 | No | — |
| Ant Forest (`ants`) | 0 | No | — |
| Arachdrathos Wilderness (`arac_wild`) | 0 | No | — |
| Arachdrathos Guilds (`aracguil`) | 0 | No | rod_of_zarbon, shimmering_longsword |
| Ard'gral, plane of dreams (`ardgral`) | 0 | No | inn, teleporting_map_pool, teleporting_pool |
| CTF Tournament (`arena`) | 0 | No | arenaobj_proc |
| Astral Plane, Side Areas (`astral_areas`) | 0 | No | — |
| The Astral Plane (`astral_main`) | 0 | No | astral_succubus, avernus, demogorgon, guild_guard, magic_pool, tiamat |
| Astral Plane, Tiamat (`astral_tiamat`) | 0 | No | GithyankiCave, artifact_hide, demogorgon_shout, githyanki |
| The Temple of Lizizania (`asylum`) | 0 | No | — |
| The Plane of Avernus (`avernus`) | 0 | No | sinister_tactics_staff, staff_shadow_summoning |
| Black Tunnels (`black_tunnels`) | 0 | No | — |
| The Bloody Plains (`bloody_plains`) | 0 | No | — |
| BogenTok (`bogentok`) | 0 | No | inn, world_quest |
| Vella's Bordello (`bordello`) | 0 | No | Padh_bouncer, Vella_slut, Vem_rouge, sex_crazed_prostitute, sleezy_prostitute, tired_young_man |
| Bugentolen (`bugentol`) | 0 | No | — |
| Buildings (`buildings`) | 0 | No | — |
| The Guildhall of Clan BloodLust (`cbl_hall`) | 0 | No | inn |
| Celestial Plane (`celestia`) | 0 | No | Einjar, Malevolence, Malevolence_vapor, celestia_pulsar, master_set, serpent_of_miracles |
| Charing (`charing`) | 0 | Yes | guild_guard, inn, pet_shops, world_quest |
| The Ruins of Tharnadia's Old Quarter (`cityruin`) | 1 | No | money_changer |
| The Tunnels of Cyric the Destroyer (`cyrictunnel`) | 0 | No | — |
| Dark Forest (`dark_forest`) | 0 | No | archer |
| Pit of Dragons (`dracopit`) | 0 | No | mace_dragondeath |
| Dragonnia (`dragonnia`) | 0 | No | baby_dragon, demodragon, dragon_guard, dragonkind, dragonnia_heart, resurrect_totem |
| Dreggan Woods (`dreggan_wood`) | 0 | No | — |
| The End of the World (`end`) | 0 | No | — |
| The Fortress of Dreams (`eth2`) | 0 | No | block_dir, eth2_aramus, eth2_aramus_crown, eth2_demon_princess, eth2_forest_animal, eth2_godsfury |
| The Ethereal Plane (`ethereal_main`) | 0 | No | artifact_hide, magic_pool, menzellon_shout, teleporting_pool |
| Faang (`faang`) | 0 | Yes | boulder_pusher, inn, poison |
| The Minor Plane of Planitude (`flind`) | 0 | No | inn |
| The Fog Enshrouded Wood (`foggy_woods`) | 0 | No | fw_ruby_monocle, fw_warning_room, item_switch |
| Fort Boyard (`fortb`) | 7 | No | crew_shop_proc |
| Ghore (`ghore`) | 0 | Yes | devour, dump, ghore_paradise, inn, money_changer, poison |
| Githyanki Hometown (`gith_ht`) | 0 | Yes | — |
| The Githyanki Fortress (`githyanki_fortress`) | 8 | No | — |
| Towering Glacier of Dayedenvale (`glacier`) | 0 | No | — |
| The Goblin Camp (`goblincamp`) | 0 | No | — |
| The Old Graveyard (`grave`) | 0 | No | dump, guild_guard, ogrebane, pet_shops, ship_shop_proc, world_quest |
| The Crypts of Rays (`graves`) | 0 | No | random_glass, random_slab, random_tomb |
| Guildhalls (`guildhalls`) | 0 | No | — |
| Hell, Phlegethos (`hell_four_phlegethos`) | 0 | No | — |
| Hell, Avernus (`hell_one_avernus`) | 0 | No | holy_weapon, tiamat_stinger |
| Hell, Minauros (`hell_three_minauros`) | 0 | No | — |
| Hell, Dis (`hell_two_dis`) | 0 | No | dispator |
| The High Moor Forest (`highmoor`) | 0 | No | — |
| The Haunted Woods (`hwood`) | 0 | No | inn |
| Arena of the Elder Brain (`illithid_arena`) | 0 | No | ship_exit_room, ship_look_out_room |
| Interspace (`interspa`) | 0 | No | — |
| The Hall of Knighthood (`knight`) | 0 | No | pathfinder |
| The Temple of Blibdoolpoolp (`kuotoa`) | 0 | No | — |
| Kvark's Great Connector (`kvarkpass_connector`) | 0 | No | — |
| The Swamp Laboratory of Khul'Lor (`lab`) | 0 | No | — |
| Lava Tubes (`lava_tubes`) | 0 | No | agthrodos, automaton_lever, automaton_trapdoor, automaton_unblock, crystal_spike, moonstone_fragment |
| Llyrath Forest (`llyrath`) | 0 | No | — |
| Llzazan Ghetto of Arachdrathos (`llzazan`) | 0 | Yes | inn, rentacleric |
| Adventurer's Vaults (`lockers`) | 0 | No | — |
| Fort Marigot (`marigot`) | 1 | Yes | artillery_one, demon_chick, fisherman_one, fisherman_two, gesen, inn |
| the surface world (`mobs_surf`) | 0 | No | — |
| The Underdark (`mobs_underdark`) | 0 | No | — |
| Morg's Place (`morg`) | 0 | No | — |
| the Lost Temple of the North (`multiclass_temple`) | 0 | No | multiclass_proc |
| The Mushroom Forest (`mush1`) | 0 | No | — |
| Nax (`nax`) | 0 | Yes | inn, world_quest |
| The Plains of Life (`newbie2`) | 0 | Yes | newbie_paladin, newbie_sign1, newbie_sign2, stream_of_life |
| The Forest of Serenity (`newfor`) | 0 | No | — |
| The Reliquary Nexus of Aggression (`nexus_ic`) | 0 | No | — |
| The Reliquary Nexus of the Corsairs (`nexus_island`) | 0 | No | — |
| The Reliquary Nexus of The Dead (`nexus_uc`) | 0 | No | obj_tp_no_high_levels |
| Nhavan Island (`nhavan_island_one`) | 0 | No | — |
| Nizari (`nizari`) | 29 | No | thief |
| Northern Wilderness (`northern_wilderness`) | 0 | No | — |
| Ny'Neth (`nyneth`) | 0 | No | construct, hammer_titans |
| Olympus (`olympus`) | 0 | No | olympus_portal |
| Orog Encampment (`orogs`) | 0 | Yes | money_changer, rentacleric |
| The Outcasts Tower (`otower`) | 0 | No | ship_exit_room, ship_look_out_room |
| The Human Outpost (`outpost`) | 1 | No | — |
| The Patrol Guard Headquarters (`patrols`) | 0 | No | inn |
| Plane of Air (`plane_air_one`) | 0 | No | lightning, magic_pool, yancbin_shout |
| Plane of Earth (`plane_earth_one`) | 0 | No | artifact_hide, artifact_stone, blind_boots, earth_treant, earthquake_gauntlet, magic_pool |
| Plane of Fire (`plane_fire_one`) | 1 | No | guild_guard, imix_shout, magic_pool, ring_elemental_control, staff_of_blue_flames, unblock_on_death |
| Plane of Water (`plane_water_one`) | 0 | No | artifact_invisible, magic_pool, olhydra_shout, orb_of_the_sea |
| The Adventurers Guildhalls (`player_castles`) | 0 | No | guildhome, guildwindow, magic_mouth, ship_exit_room |
| The Potion Treasure Vault (`potions`) | 0 | No | — |
| Pragtog's Domain (`pragtog`) | 0 | No | — |
| Rabble (`rabble`) | 0 | No | dragonslayer, fooquest_boss, fooquest_mob, newbie_quest |
| Ogre Raiders (`raiders`) | 0 | No | — |
| Randomness (`random`) | 0 | No | — |
| The River Styx (`river_styx`) | 0 | No | — |
| Tunnels of Payang (`rtun`) | 0 | Yes | money_changer |
| Graendiae (`sea`) | 0 | Yes | inn, world_quest |
| The Seacaves (`seacaves`) | 0 | No | cutting_dagger, unspec_altar |
| Sevenoaks (`seveno`) | 0 | No | sevenoaks_longsword |
| Arbre's Forest (`sforest`) | 0 | No | — |
| Shadamehr Keep (`shadamehr_keep`) | 0 | No | artifact_invisible, unholy_avenger_bloodlust |
| Shadowclave (`shadowcl`) | 0 | No | — |
| The Underworld Shafts (`shafts`) | 0 | No | — |
| The Shaughin Hunting Grounds (`shaughin2`) | 0 | No | — |
| The Adventurers Shipyards (`ship`) | 0 | No | — |
| The Undead Fog (`shortb`) | 0 | No | — |
| Skulldrach (`skulldrach`) | 0 | No | — |
| Village of Split Shield (`split_shield`) | 0 | No | gate_guard, shady_man |
| Swamp (`swamp_one`) | 0 | No | nightbringer |
| The Lair of the Swamp Troll King (`swamp_two`) | 0 | No | — |
| The Monestary of Tranquility (`tchan`) | 0 | No | magic_pool, newbie_guard_east |
| The Temple of Laduguer (`temple_laduguer`) | 0 | No | cityguard, guild_guard, inn, janitor, money_changer, pet_shops |
| The Mountain Village of Tentro (`tentro2`) | 0 | No | — |
| Ruined Temple of Tezcatlipoca (`tezcat`) | 0 | No | — |
| Tharnadia Rifts (`tharnrifts`) | 0 | No | — |
| Taliccicopiid (`thri`) | 0 | Yes | inn |
| Troll Slave Compound (`torture`) | 0 | No | — |
| Tradeskills (`tradeskills`) | 0 | No | epic_store, huntsman_ward, learn_recipe, outpost_captain, patrol_leader, patrol_leader_road |
| Traoul's Lair (`traoul`) | 0 | No | — |
| Truktakyagyo (`truk`) | 0 | No | world_quest |
| The Minizones of the Underdark (`udmini`) | 0 | No | — |
| Ugta (`ugta`) | 31 | Yes | inn |
| The Underworld (`underworld3`) | 0 | No | inn |
| The Unique's Vault (`unique`) | 0 | No | artifact_biofeedback, artifact_stone, church_door, demo_scimitar, dranum_mask, golem_chunk |
| The Undead Outpost (`unoutpst`) | 0 | No | money_changer |
| Valinhaven (`valinhav`) | 0 | No | — |
| Verzanan (`verz1`) | 0 | No | assassin_one, baker_one, baker_two, blob, brigand_one, casino_four |
| Crystalspyre Mountains (`vrzawild`) | 0 | No | — |
| The Wandering Fields (`wander`) | 0 | No | — |
| Wellvolen (`wemic`) | 0 | No | — |
| The Wildland Trails (`wildland_trails`) | 0 | No | barbarian_spiritist, plant_attacks_blindness, plant_attacks_paralysis, plant_attacks_poison |
| The Lost Realms of Shadowfall (`wolfen`) | 0 | No | generic_riposte_proc |
| Village of the Damned (`yuan_ti2`) | 0 | No | — |
