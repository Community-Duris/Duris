#ifndef DURIS_SQL_ROOM_ITEM_PAYLOAD_H
#define DURIS_SQL_ROOM_ITEM_PAYLOAD_H

#include "player/player_load_repository.h"

constexpr uint16_t SQL_ROOM_ITEM_PAYLOAD_VERSION = 1;
constexpr size_t SQL_ROOM_ITEM_ROOT_MAX = PLAYER_SNAPSHOT_MAX_OBJECTS;
constexpr size_t SQL_ROOM_ITEM_GRAPH_MAX_BYTES =
	ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES + 4 * ITEM_TRANSFER_MAX_ITEMS;

// Payload/provenance only. Native item_current_owner remains the sole placement
// authority. This token belongs to one existing coordinator transaction/session.
struct sql_room_item_payload_batch
{
	unsigned long session_id = 0;
	uint64_t season_epoch = 0;
	std::vector<player_item_snapshot> items;
	std::vector<std::vector<uint8_t>> payloads;
};

struct sql_room_item_graph
{
	item_owner_identity owner = { item_owner_type::unknown, 0, 0 };
	uint64_t owner_revision = 0;
	std::vector<player_item_snapshot> items;
	std::vector<player_load_item_identity> identities;
};

// Pure, bounded capture validation. Incomplete/prototype-dependent text is not
// admissible; ordinary producer capture is deliberately not widened here.
bool sql_room_item_payload_capture(const item_transfer_payload &payload,
				   sql_room_item_payload_batch *batch);
bool sql_room_item_payload_lock_season(MYSQL *connection, uint64_t *epoch);
bool sql_room_item_payload_prepare(MYSQL *connection, const item_transfer_payload &payload,
				   sql_room_item_payload_batch *batch);
bool sql_room_item_payload_record(MYSQL *connection, const critical_command &command,
				  const item_transfer_payload &payload,
				  const sql_room_item_payload_batch &batch);
// Caller owns a consistent transaction. Reads never commit, adopt, or repair.
bool sql_room_item_payload_roots(MYSQL *connection, std::vector<uint64_t> *roots);
bool sql_room_item_payload_available(MYSQL *connection, bool *available);
bool sql_room_item_payload_read(MYSQL *connection, uint64_t root_uid, sql_room_item_graph *graph);
// Legacy rows with any retained exact payload cannot independently publish a
// second/older representation. Failure is a refusal, not a legacy fallback.
bool sql_room_item_payload_present(MYSQL *connection, uint64_t uid, bool *present);

#endif
