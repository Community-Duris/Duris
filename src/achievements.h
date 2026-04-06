// achievements
int  get_frags(P_char);
void do_achievements(P_char ch, char *arg, int cmd);
void update_achievements(P_char ch, P_char victim, int cmd, int ach);
int  notch_achievement(P_char ch, int achievement);

affected_type *apply_achievement(P_char ch, int ach);

// addicted to blood
void do_addicted_blood(P_char ch, char *arg, int cmd);
void update_addicted_to_blood(P_char ch, P_char victim);
