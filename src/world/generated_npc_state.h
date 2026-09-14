#ifndef GENERATED_NPC_STATE_H
#define GENERATED_NPC_STATE_H

#include "core/structs.h"
#include "player/pet_restore_state.h"

#include <string>

constexpr size_t GENERATED_NPC_EXTENSION_HEADER_BYTES = 8;
constexpr size_t GENERATED_NPC_STATE_WALLET_BYTES = 16;
constexpr size_t GENERATED_NPC_EXTENSION_MAX_BYTES = GENERATED_NPC_EXTENSION_HEADER_BYTES +
						     GENERATED_NPC_STATE_WALLET_BYTES +
						     PET_RESTORE_STATE_MAX_BYTES;

bool generated_npc_state_encode(const pet_restore_state &state,
				const std::array<int32_t, 4> &wallet, std::string *encoded);
bool generated_npc_state_decode(const std::string &encoded, pet_restore_state *state,
				std::array<int32_t, 4> *wallet);

bool generated_npc_vnum(int vnum);
bool generated_npc_capture(P_char mob, std::string *encoded, bool include_currency = true);
bool generated_npc_apply(P_char mob, const std::string &encoded);
bool generated_npc_state_valid(int vnum, const std::string &encoded);
bool generated_npc_extension_encode(int vnum, const std::string &encoded, std::string *extension);
bool generated_npc_extension_decode(int vnum, const char *data, size_t size, std::string *encoded);

#endif
