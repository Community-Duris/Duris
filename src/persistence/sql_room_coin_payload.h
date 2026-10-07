#ifndef DURIS_SQL_ROOM_COIN_PAYLOAD_H
#define DURIS_SQL_ROOM_COIN_PAYLOAD_H

#include "item/item_ownership_runtime.h"
#include "player/player_snapshot.h"
#include <mysql/mysql.h>

constexpr size_t SQL_ROOM_COIN_ROOT_MAX = PLAYER_SNAPSHOT_MAX_OBJECTS;

// Current native value/custody plus authenticated retained ordinary-coin proof.
// This is a value snapshot borrowed from one caller-owned SQL transaction/session,
// not an original command envelope, publication token, save reservation or ACK.
struct sql_room_coin_pile
{
	unsigned long session_id = 0;
	uint64_t season_epoch = 0;
	critical_operation_id lineage = {}, epoch = {}, root_operation_id = {},
			      pile_child_operation_id = {};
	item_ownership_runtime_entry identity = {};
	player_item_snapshot item;
	item_transfer_result retained_pile_result = {};
	uint64_t retained_root_revision = 0;
};

// Bounded schema classification only. No accounting core tables means a proved
// pre-accounting legacy schema (available=false). Partial typed prerequisites
// refuse; they must never authorize legacy fallback. No epoch is activated.
bool sql_room_coin_payload_available(MYSQL *connection, bool *available);

// Caller owns the original active SQL transaction with autocommit enabled and
// automatic reconnect disabled. Neither function begins/ends/retries/adopts it.
// Enumeration grants no authority. Read locks current lineage authority, then
// season, then current room counter before custody, and authenticates the exact current
// pile effect and retained typed root/children/ledgers/receipts. Historical wallet
// receipts/mappings need not remain current. No native mutation or wallet repair.
// Output stays unchanged on failure; caller must confirm original-session cleanup.
bool sql_room_coin_payload_roots(MYSQL *connection, std::vector<uint64_t> *roots);
bool sql_room_coin_payload_read(MYSQL *connection, uint64_t uid, sql_room_coin_pile *pile);

// Per-UID classification only, usable in the legacy loader's existing session.
// Current payload or recognizable retained typed-coin history fences stale saved
// rows, including destroyed/consumed coins and corrupted proof. True grants no
// authority. Failure preserves output and must also suppress legacy publication.
// History is intentionally independent of the current epoch and payload lifetime.
bool sql_room_coin_payload_present(MYSQL *connection, uint64_t uid, bool *present);

#endif
