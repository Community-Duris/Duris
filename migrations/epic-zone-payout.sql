---- non-destructive, can be applied like:
-- mysql -uduris -pduris duris_dev <epic-zone-payout.sql

---- lets not have group size affect payout until we get the numbers right
UPDATE zones SET suggested_group_size = 100 WHERE epic_type != '0';

---- later on we might want to try something like this:
-- UPDATE zones SET suggested_group_size = 4 WHERE epic_type = '1';
-- UPDATE zones SET suggested_group_size = 8 WHERE epic_type = '2';
-- UPDATE zones SET suggested_group_size = 12 WHERE epic_type = '3';

-- adjusted values based on discord feedback, generally raised the low zones and lowered the high
UPDATE zones SET epic_payout = 0 WHERE number = 1389; --ironstar (broken)
UPDATE zones SET epic_payout = 80 WHERE number = 400; --nizari
UPDATE zones SET epic_payout = 80 WHERE number = 93; --tower of high sorcery
UPDATE zones SET epic_payout = 80 WHERE number = 740; --bloodstone keep
UPDATE zones SET epic_payout = 80 WHERE number = 14; --kobold settlement
UPDATE zones SET epic_payout = 80 WHERE number = 90; --orrak
UPDATE zones SET epic_payout = 80 WHERE number = 383; --werrun
UPDATE zones SET epic_payout = 90 WHERE number = 264; --myrloch vale
UPDATE zones SET epic_payout = 90 WHERE number = 140; --faerie realm
UPDATE zones SET epic_payout = 90 WHERE number = 823; --swamp lab
UPDATE zones SET epic_payout = 90 WHERE number = 370; --crystalspyre
UPDATE zones SET epic_payout = 90 WHERE number = 38; --elemental groves
UPDATE zones SET epic_payout = 90 WHERE number = 879; --kelek
UPDATE zones SET epic_payout = 90 WHERE number = 113; --outcast tower
UPDATE zones SET epic_payout = 90 WHERE number = 143; --nakral
UPDATE zones SET epic_payout = 100 WHERE number = 191; --high moor
UPDATE zones SET epic_payout = 100 WHERE number = 342; --clan stoutdorf (drst)
UPDATE zones SET epic_payout = 100 WHERE number = 285; --pharr valley
UPDATE zones SET epic_payout = 100 WHERE number = 67; --court of the muse
UPDATE zones SET epic_payout = 100 WHERE number = 381; --fort boyard
UPDATE zones SET epic_payout = 100 WHERE number = 27; --headless
UPDATE zones SET epic_payout = 100 WHERE number = 429; --drustl
UPDATE zones SET epic_payout = 100 WHERE number = 805; --skrentherlog (yan-ti)
UPDATE zones SET epic_payout = 100 WHERE number = 133; --caverns of armageddon
UPDATE zones SET epic_payout = 100 WHERE number = 183; --temple of flames
UPDATE zones SET epic_payout = 100 WHERE number = 130; --citadel
UPDATE zones SET epic_payout = 100 WHERE number = 666; --torrhan
UPDATE zones SET epic_payout = 100 WHERE number = 1320; --githyanki fortress
UPDATE zones SET epic_payout = 100 WHERE number = 220; --pits of cerebus
UPDATE zones SET epic_payout = 100 WHERE number = 755; --dark stone tower
UPDATE zones SET epic_payout = 110 WHERE number = 758; --rogue plains
UPDATE zones SET epic_payout = 110 WHERE number = 73; --carthapia
UPDATE zones SET epic_payout = 110 WHERE number = 824; --temple of the sun
UPDATE zones SET epic_payout = 110 WHERE number = 662; --tharn ruins
UPDATE zones SET epic_payout = 110 WHERE number = 664; --battlefield
UPDATE zones SET epic_payout = 120 WHERE number = 430; --boyard prison
UPDATE zones SET epic_payout = 120 WHERE number = 773; --desolate under fire
UPDATE zones SET epic_payout = 120 WHERE number = 490; --venan'trut
UPDATE zones SET epic_payout = 120 WHERE number = 710; --fields between
UPDATE zones SET epic_payout = 130 WHERE number = 200; --krethik keep
UPDATE zones SET epic_payout = 130 WHERE number = 766; --jade empire
UPDATE zones SET epic_payout = 150 WHERE number = 760; --ultarium
UPDATE zones SET epic_payout = 150 WHERE number = 570; --trakkia
UPDATE zones SET epic_payout = 150 WHERE number = 91; --mountain of the banished
UPDATE zones SET epic_payout = 175 WHERE number = 318; --ice tower
UPDATE zones SET epic_payout = 175 WHERE number = 50; --labyrinth
UPDATE zones SET epic_payout = 200 WHERE number = 970; --icecrag castle
UPDATE zones SET epic_payout = 200 WHERE number = 920; --undermountain
UPDATE zones SET epic_payout = 200 WHERE number = 213; --mazzolin
UPDATE zones SET epic_payout = 225 WHERE number = 24; --quintaragon
UPDATE zones SET epic_payout = 225 WHERE number = 244; --plane of air
UPDATE zones SET epic_payout = 225 WHERE number = 254; --plane of fire
UPDATE zones SET epic_payout = 225 WHERE number = 197; --astral plane
UPDATE zones SET epic_payout = 250 WHERE number = 151; --new cave city
UPDATE zones SET epic_payout = 250 WHERE number = 780; --tribal oasis
UPDATE zones SET epic_payout = 250 WHERE number = 412; --shadamehr
UPDATE zones SET epic_payout = 260 WHERE number = 87; --gibberling
UPDATE zones SET epic_payout = 260 WHERE number = 368; --domain
UPDATE zones SET epic_payout = 275 WHERE number = 35; --forgotten mansion
UPDATE zones SET epic_payout = 275 WHERE number = 448; --keep of evil
UPDATE zones SET epic_payout = 275 WHERE number = 756; --obsidian citadel
UPDATE zones SET epic_payout = 275 WHERE number = 261; --swamp troll
UPDATE zones SET epic_payout = 285 WHERE number = 419; --forest of mir
UPDATE zones SET epic_payout = 285 WHERE number = 162; --transparent tower
UPDATE zones SET epic_payout = 300 WHERE number = 709; --hall of knighthood
UPDATE zones SET epic_payout = 300 WHERE number = 238; --plane of earth
UPDATE zones SET epic_payout = 300 WHERE number = 124; --ethereal plane
UPDATE zones SET epic_payout = 315 WHERE number = 784; --tharn rifts
UPDATE zones SET epic_payout = 315 WHERE number = 831; --alatorin
UPDATE zones SET epic_payout = 325 WHERE number = 386; --sevenoaks
UPDATE zones SET epic_payout = 325 WHERE number = 229; --ny'neth
UPDATE zones SET epic_payout = 325 WHERE number = 289; --kingdom of torg
UPDATE zones SET epic_payout = 325 WHERE number = 960; --jotunheim
UPDATE zones SET epic_payout = 335 WHERE number = 441; --tikitzopl (51)
UPDATE zones SET epic_payout = 345 WHERE number = 215; --aravne
UPDATE zones SET epic_payout = 350 WHERE number = 989; --tezcat
UPDATE zones SET epic_payout = 350 WHERE number = 315; --sea kingdom
UPDATE zones SET epic_payout = 350 WHERE number = 367; --arachdrathos guilds
UPDATE zones SET epic_payout = 350 WHERE number = 1200; --depths
UPDATE zones SET epic_payout = 350 WHERE number = 1398; --smoke plane
UPDATE zones SET epic_payout = 350 WHERE number = 232; --plane of water
UPDATE zones SET epic_payout = 400 WHERE number = 328; --shaboath (51)
UPDATE zones SET epic_payout = 400 WHERE number = 159; --pit of dragons
UPDATE zones SET epic_payout = 400 WHERE number = 435; --temple of earth (52)
UPDATE zones SET epic_payout = 400 WHERE number = 712; --scorched valley
UPDATE zones SET epic_payout = 400 WHERE number = 326; --fortress of dreams
UPDATE zones SET epic_payout = 425 WHERE number = 910; --barovia
UPDATE zones SET epic_payout = 425 WHERE number = 877; --snow ogres
UPDATE zones SET epic_payout = 425 WHERE number = 777; --hall of ancients
UPDATE zones SET epic_payout = 450 WHERE number = 883; --charcoal palace (51)
UPDATE zones SET epic_payout = 450 WHERE number = 1316; --tempest court
UPDATE zones SET epic_payout = 500 WHERE number = 814; --ceothia (53)
UPDATE zones SET epic_payout = 500 WHERE number = 230; --ny'neth 2
UPDATE zones SET epic_payout = 500 WHERE number = 1390; --brass (52)
UPDATE zones SET epic_payout = 550 WHERE number = 444; --githzerai stronghold (52)
UPDATE zones SET epic_payout = 550 WHERE number = 588; --barovia 2
UPDATE zones SET epic_payout = 550 WHERE number = 1424; --mausoleum
UPDATE zones SET epic_payout = 600 WHERE number = 1300; --vecna (54)
UPDATE zones SET epic_payout = 600 WHERE number = 68; --dragonnia
UPDATE zones SET epic_payout = 650 WHERE number = 196; --tiamat (53)
UPDATE zones SET epic_payout = 700 WHERE number = 345; --apocalypse castle (54)
UPDATE zones SET epic_payout = 700 WHERE number = 257; --bahamut (54)
UPDATE zones SET epic_payout = 800 WHERE number = 266; --negative plane (55)
UPDATE zones SET epic_payout = 800 WHERE number = 324; --bronze citadel (55)
UPDATE zones SET epic_payout = 850 WHERE number = 4200; --dreadnaught
UPDATE zones SET epic_payout = 900 WHERE number = 387; --ny'neth 3 (56)
UPDATE zones SET epic_payout = 900 WHERE number = 455; --celestia (56)
UPDATE zones SET epic_payout = 950 WHERE number = 875; --222 (56)
UPDATE zones SET epic_payout = 1000 WHERE number = 583; --ravenloft (56)

