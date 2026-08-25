#ifndef _SPECS_ETH2_H_
#define _SPECS_ETH2_H_

// mob procs
int eth2_forest_animal(P_char ch, P_char pl, int cmd, char *arg);
int eth2_little_girl(P_char ch, P_char pl, int cmd, char *arg);
int eth2_demon_princess(P_char ch, P_char pl, int cmd, char *arg);
int eth2_aramus(P_char ch, P_char pl, int cmd, char *arg);

// obj procs
int eth2_tree_obj(P_obj obj, P_char ch, int cmd, char *arg);
int eth2_godsfury(P_obj obj, P_char ch, int cmd, char *arg);
int eth2_aramus_crown(P_obj obj, P_char ch, int cmd, char *arg);
void event_aramus_crown_sleep_check(P_char ch, P_char vict, P_obj obj, void *data);

#endif // _SPECS_ETH2_H
