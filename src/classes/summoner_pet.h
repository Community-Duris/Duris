#ifndef DURIS_SUMMONER_PET_H
#define DURIS_SUMMONER_PET_H

#include "core/structs.h"
#include <array>

void summoner_chaos_recipes(P_char owner);
bool summoner_capture(P_char pet);
bool summoner_balanced_body(P_char pet);
bool summoner_owned_pet(P_char pet);
int summoner_pet_strength_factor(int race);
int summoner_pet_attribute_factor(P_char pet, int race, int attribute);
void summoner_pet_apply_traits(P_char pet, const std::array<uint64_t, 5> &traits);
void summoner_pet_cast_buffs(P_char pet, const std::array<uint64_t, 5> &traits);
int summoner_pet_level(P_char pet, P_char owner);
void summoner_pet_configure(P_char pet, P_char owner, bool preview = false, bool restoring = false);
void summoner_pet_finish_affects(P_char pet);
int summoner_pet_memtime(P_char pet, int time);
double summoner_pet_melee_damage(P_char pet, double damage);
double summoner_pet_physical_damage(P_char pet, double damage);
int summoner_pet_hit_regen(P_char pet, int gain);
int summoner_pet_heal_cap(P_char pet, int requested);
double summoner_pet_vamp_rate(P_char pet, double requested, bool undead = false);
void summoner_pet_sync_resources(P_char pet);
bool summoner_pet_recovery_blocked(P_char pet);
void summoner_pet_note_command(P_char ch, int command);
bool summoner_pet_skill(P_char pet, int command);
void summoner_pet_exhausted(P_char pet);
bool summoner_pet_spend(P_char pet, int mana);
bool summoner_pet_spell_ready(P_char pet, int circle);
bool summoner_pet_song(P_char bard, int song, bool aggressive, bool self_only, bool allies_only,
		       int room, int extra_cost = 0);
bool summoner_pet_flight_regen(P_char target, unsigned long long elapsed);
void summoner_pet_start_recovery(P_char owner);
void summoner_pet_resume_slots(P_char pet);
void summoner_elemental_body(P_char pet, P_char owner, bool greater, int terrain,
			     int template_hits = 0, int template_damage = 0);

#endif
