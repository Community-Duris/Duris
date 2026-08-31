#ifndef _SPECS_WINTERHAVEN_H_
#define _SPECS_WINTERHAVEN_H_

#define WELL_ROOM 55126
#define WELL 100
#define WINTERHAVEN 550

int winterhaven_shout_one(P_char, P_char, int, char *);
int winterhaven_shout_two(P_char, P_char, int, char *);
int dagger_ra(P_obj, P_char, int, char *);
int wh_corpse_to_object(P_char, P_char, int, char *);
int wh_corpse_decay(P_obj, P_char, int, char *);
int illithid_axe(P_obj, P_char, int, char *);
int deathseeker_mace(P_obj, P_char, int, char *);
int snowogre_warhammer(P_obj, P_char, int, char *);
int volo_longsword(P_obj, P_char, int, char *);
int blur_shortsword(P_obj, P_char, int, char *);
int storm_legplates(P_obj, P_char, int, char *);
int newbie_spellup_mob(P_char ch, P_char victim, int cmd, char *arg);
int welfare_well(int room, P_char ch, int cmd, char *arg);
int no_kill_priest_obj(P_obj, P_char, int, char *);
int demon_slayer(P_obj, P_char, int, char *);
int helmet_vampires(P_obj, P_char, int, char *);
int buckler_saints(P_obj, P_char, int, char *);
// int boots_abyss(P_obj, P_char, int, char *);
int rapier_penetration(P_obj, P_char, int, char *);
int sword_random(P_obj, P_char, int, char *);
int gauntlets_legend(P_obj, P_char, int, char *);
int poseidon_trident(P_obj, P_char, int, char *);
// int platemail_fame(P_obj, P_char, int, char *);
int attribute_scroll(P_obj, P_char, int, char *);
int earring_powers(P_obj, P_char, int, char *);
int lorekeeper_scroll(P_obj, P_char, int, char *);
int gladius_backstabber(P_obj, P_char, int, char *);
int elemental_wand(P_obj, P_char, int, char *);
int damnation_staff(P_obj, P_char, int, char *);
int nuke_damnation(P_obj, P_char, int, char *);
int collar_frost(P_obj, P_char, int, char *);
int collar_flames(P_obj, P_char, int, char *);
int tiamat_human_to_rareloads(P_char, P_char, int, char *);
int dragonnia_heart(P_char, P_char, int, char *);
int dragon_heart_decay(P_obj, P_char, int, char *);
int lanella_heart(P_char, P_char, int, char *);
int lancer_gift(P_obj, P_char, int, char *);
int cerberus_load(P_char, P_char, int, char *);

#endif
