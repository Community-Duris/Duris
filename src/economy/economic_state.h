#ifndef DURIS_ECONOMIC_STATE_H
#define DURIS_ECONOMIC_STATE_H

#include "economy/currency_command.h"

#include <string>

// Owned exclusively by the game thread. A wallet's lifetime is the server's
// lifetime, not its character/descriptor's lifetime. Account banks are shared.
struct economic_progress_state
{
	int64_t epics = 0;
	uint64_t epic_revision = 0;
	int64_t frags = 0;
	int64_t old_frags = 0;
	uint64_t frag_revision = 0;
};

struct economic_player_state
{
	uint32_t pid = 0;
	std::string account;
	std::string name;
	uint8_t racewar = 0;
	currency_command_result balances = {};
	economic_progress_state progress = {};
};

bool economic_state_seed(const economic_player_state &player);
bool economic_state_find(uint32_t pid, economic_player_state *player);
bool economic_state_balances(uint32_t pid, const char *account, uint8_t racewar,
			     currency_command_result *balances);
// Preflight allocates nothing and checks both halves before either is changed.
bool economic_state_can_publish(uint32_t pid, const char *account, uint8_t racewar,
				const currency_command_result &balances);
bool economic_state_publish(uint32_t pid, const char *account, uint8_t racewar,
			    const currency_command_result &balances);
bool economic_state_progress(uint32_t pid, economic_progress_state *progress);
bool economic_state_publish_progress(uint32_t pid, const economic_progress_state &progress);
bool economic_state_can_publish_progress(uint32_t pid, const economic_progress_state &progress);
void economic_state_reset();
void economic_state_forget(uint32_t pid);
void economic_state_activate();
bool economic_state_active();

#endif
