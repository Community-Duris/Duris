#ifndef DURIS_NATIVE_QUEST_TRANSPORT_H
#define DURIS_NATIVE_QUEST_TRANSPORT_H

#include "persistence/critical_command.h"

// Immutable transport classification only. Typed domain/source/held-owner
// validation, journal durability, SQL and publication authority remain separate.
// v12 = item recovery; v14 = fee recovery; v16 = coin-GIVE recovery.
// Unacknowledged v11/v13/v15 and legacy schema1 never gain native transport.
inline bool native_quest_transport_command(const critical_command &command) noexcept
{
	return command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	       command.type == critical_command_type::item_transfer &&
	       (command.payload_version == 12 || command.payload_version == 14 ||
		command.payload_version == 16);
}

#endif
