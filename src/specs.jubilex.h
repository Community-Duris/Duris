#ifndef _SPECS_JUBILEX_H_
#define _SPECS_JUBILEX_H_

#define CLEANSED_LAKE_VNUM    87517
#define SLIME_SHEEN_VNUM      87524
#define JUBILEX_WORMHOLE_VNUM 87519
#define WILDMAGIC_MASK_VNUM   87546

#define JUBILEX_DEATH_FROM_ROOM 87626
#define JUBILEX_DEATH_TO_ROOM   87598

int slime_lake(P_char ch, P_char pl, int cmd, char *arg);
int jubilex_one(P_char ch, P_char pl, int cmd, char *arg);
int mask_of_wildmagic(P_obj obj, P_char ch, int cmd, char *arg);
int ebb_vambraces(P_obj obj, P_char ch, int cmd, char *arg);
int flow_amulet(P_obj obj, P_char ch, int cmd, char *arg);
int jubilex_grid_mob_generator(P_obj obj, P_char ch, int cmd, char *arg);

#endif
