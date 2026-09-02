#ifndef _ENHANCE_H_
#define _ENHANCE_H_

#include "core/structs.h"

/* Boot-time index entry */
struct enhance_index_entry
{
	int vnum;
	int ival;
	unsigned int wear_flags;
	int material;
	int apply_loc[4];
	int apply_mod[4];
	struct enhance_index_entry *next;
};

/* Hash table lookup result */
struct enhance_index_result
{
	struct enhance_index_entry *entry;
	int ival;
};

/* Prototypes */
bool is_enhance_banned(P_obj item);
void enhance(P_char ch, P_obj source, P_obj material);
void do_enhance(P_char ch, char *argument, int cmd);
void modenhance(P_char ch, P_obj source, P_obj material);
void boot_enhancement_system(void);
bool enhancement_system_is_ready(void);
void enhance_on_eligible_npc_death(P_char ch, P_char killer);
void enhance_on_npc_item_reset_skipped(P_char mob, P_obj missing_item);

/* Global index tables (externed for access) */
#define ENHANCE_IVAL_TABLE_SIZE 4099
#define ENHANCE_STAT_TABLE_SIZE 4099

extern struct enhance_index_entry *enhance_ival_table[ENHANCE_IVAL_TABLE_SIZE];
extern struct enhance_index_entry *enhance_stat_table[ENHANCE_STAT_TABLE_SIZE];

/* Config settings — read from lib/enhance.cfg at boot */
extern int enhance_ival_cap;
extern int enhance_material_ival_delta;
extern int enhance_guild_insignia_ival_bonus;
extern int enhance_cost_low_ival_threshold;
extern int enhance_cost_low_amount;
extern int enhance_cost_high_amount;
extern int enhance_search_vnum_min;
extern int enhance_search_vnum_max;
extern int enhance_search_max_attempts;
extern int enhance_wear_skip_mask;
extern int enhance_original_max_roll;
extern int enhance_original_cascade_down_first;
extern int enhance_luck_extreme_range;
extern int enhance_luck_very_range;
extern int enhance_luck_lucky_range;
extern int enhance_ival_gain_extreme;
extern int enhance_ival_gain_very;
extern int enhance_ival_gain_lucky;
extern int enhance_ival_gain_normal;
extern int enhance_stat_enabled;
extern int enhance_stat_npc_material_fallback_enabled;

/* Bitvector allow masks built from config */
extern unsigned long enhance_allow_mask;
extern unsigned long enhance_allow_mask2;
extern unsigned long enhance_allow_mask3;
extern unsigned long enhance_allow_mask4;
extern unsigned long enhance_allow_mask5;

#endif /* _ENHANCE_H_ */
