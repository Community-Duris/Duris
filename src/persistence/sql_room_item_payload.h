#ifndef DURIS_SQL_ROOM_ITEM_PAYLOAD_H
#define DURIS_SQL_ROOM_ITEM_PAYLOAD_H

#include "player/player_load_repository.h"
#include "persistence/economic_sql_source_snapshot.h"
#include "economy/zone_reset_item_command.h"
#include "economy/zone_reset_item_origin.h"

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
	// Set only by the authenticated creation cold reader. The full original
	// forest/recipes survive removed descendants; values alone grant no admission.
	std::optional<zone_reset_item_retained_origin> creation_origin;
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
// Successful historical receipt proof: original-operation literal payload and
// native ledger/reference bindings, independent of later custody or season.
// Caller owns its transaction and verifies the accounting root separately.
bool sql_room_item_payload_verify_retained(MYSQL *connection, const critical_command &command,
					   const item_transfer_result &result);

// Complete room-reset creation forest, including explicit actual coin outputs.
// Never widens player-drop capture. Bounded literal/denomination validation is
// not source or issuance authority. Artifacts retain their separate owner.
bool sql_room_item_payload_capture_creation(const zone_reset_item_image &,
					    sql_room_item_payload_batch *);
// Caller owns global reservation/source/operation authority and rollback. These
// borrow one original reconnect-disabled transaction, season before room/items.
// Prepare locks direct custody/literal/physical absence and preserves output on
// refusal. Record inserts immutable revision1 literals after exact root custody
// and creation ledger; failure after DML requires caller rollback. Never commits.
bool sql_room_item_payload_prepare_creation(MYSQL *, const zone_reset_item_image &,
					    sql_room_item_payload_batch *);
bool sql_room_item_payload_record_creation(MYSQL *, const critical_command &,
					   const zone_reset_item_image &,
					   sql_room_item_payload_batch *);
bool sql_room_item_payload_verify_creation_retained(MYSQL *, const critical_command &,
						    const item_transfer_result &);
// Caller owns a consistent transaction. Reads never commit, adopt, or repair.
bool sql_room_item_payload_roots(MYSQL *connection, std::vector<uint64_t> *roots);
bool sql_room_item_payload_available(MYSQL *connection, bool *available);
bool sql_room_item_payload_read(MYSQL *connection, uint64_t root_uid, sql_room_item_graph *graph);
// Conservative routing observation only. Set may_need_owner=true immediately;
// SQL/schema/root/proof failures leave it true and return false. Only a valid
// later revision or successfully proven absence can select the existing route.
// Caller retains pending creation handles independently. Never consumes, commits
// or rolls back the original reconnect-disabled transaction.
bool sql_room_item_payload_classify_creation(MYSQL *, uint64_t root_uid,
					     bool *may_need_owner) noexcept;
// Creation money recovery additionally locks the actual current book before
// season/room/custody and proves each present pile's unique original birth head.
// A progressed or new-book opening head needs its own owner; history alone
// never authorizes its publication. Historical receipt verification stays separate.
// Legacy rows with any retained exact payload cannot independently publish a
// second/older representation. Failure is a refusal, not a legacy fallback.
bool sql_room_item_payload_present(MYSQL *connection, uint64_t uid, bool *present);

// Supplemental persisted evidence. Indices refer to immutable captured tables,
// not database/materializer identities. A historical UID/revision is not an
// additional current physical item. No witness confers activation authority.
enum sql_room_item_source_flag : uint32_t
{
	SQL_ROOM_SOURCE_CURRENT = 1U << 0,
	SQL_ROOM_SOURCE_HISTORY = 1U << 1,
	SQL_ROOM_SOURCE_OLD_SEASON = 1U << 2,
	SQL_ROOM_SOURCE_MISSING_CUSTODY = 1U << 3,
	SQL_ROOM_SOURCE_MALFORMED_LITERAL = 1U << 4,
	SQL_ROOM_SOURCE_MISSING_PROOF = 1U << 5,
	SQL_ROOM_SOURCE_AMBIGUOUS_PROOF = 1U << 6,
	SQL_ROOM_SOURCE_COMPETING_LEGACY = 1U << 7,
	SQL_ROOM_SOURCE_MISSING_LITERAL = 1U << 8,
	SQL_ROOM_SOURCE_BAD_GRAPH = 1U << 9,
	SQL_ROOM_SOURCE_MISSING_OWNER_REVISION = 1U << 10,
	SQL_ROOM_SOURCE_MALFORMED_IDENTITY = 1U << 11,
	SQL_ROOM_SOURCE_MOVED_FROM_ROOM = 1U << 12,
	SQL_ROOM_SOURCE_STALE_PROVENANCE = 1U << 13,
	SQL_ROOM_SOURCE_NATIVE_ROOT_LIMIT = 1U << 14
};
struct sql_room_item_source_witness
{
	uint64_t item_uid = 0, item_revision = 0, season_epoch = 0;
	uint64_t root_item_uid = 0, parent_item_uid = 0, room = 0, owner_revision = 0;
	size_t payload_row = SIZE_MAX, custody_row = SIZE_MAX;
	size_t ledger_row = SIZE_MAX, reference_row = SIZE_MAX;
	uint32_t flags = 0;
	std::optional<player_item_snapshot> literal;
};
struct sql_room_item_source_graph
{
	uint64_t root_item_uid = 0, room = 0, owner_revision = 0;
	// Native parent-before-child order, referencing evidence.witnesses.
	std::vector<size_t> witness_indices;
	bool valid = false;
};
struct sql_room_item_source_diagnostic
{
	uint32_t flags = 0;
	size_t witness_index = SIZE_MAX;
};
struct sql_room_item_source_evidence
{
	uint64_t current_season = 0;
	bool season_active = false, diagnostics_truncated = false;
	// Per-graph validity cannot establish native-loadability when the complete
	// root inventory exceeds the existing recovery enumeration ceiling.
	bool native_root_limit_exceeded = false;
	std::vector<sql_room_item_source_witness> witnesses;
	std::vector<sql_room_item_source_graph> graphs;
	std::vector<sql_room_item_source_diagnostic> diagnostics;
};
struct sql_room_item_source_snapshot
{
	// Only the five additional owning raw projections. The unchanged base is
	// borrowed, never copied or recaptured. Exact NULL/binary bytes are retained.
	std::vector<economic_sql_source_table> tables;
	economic_sql_source_digest physical_digest = {}, digest = {};
	uint64_t rows = 0, cells = 0, cell_bytes = 0;
	sql_room_item_source_evidence evidence;
};
// Pure bounded inspection of all five raw projections against a structurally
// validated EPH1 base. Operational failure preserves output; malformed,
// unmatched and historical rows remain evidence with explicit diagnostics.
unsigned int sql_room_item_payload_inspect_sources(const economic_sql_physical_source_snapshot &,
						   const std::vector<economic_sql_source_table> &,
						   const economic_sql_source_limits &,
						   size_t maximum_diagnostics,
						   sql_room_item_source_evidence *) noexcept;
// Caller guarantees the base belongs to THIS original session and still-open
// consistent RR cut. Its hashes do not authenticate that guarantee. Accepts a
// writable borrowed RR transaction but performs ordinary SELECTs only; never
// starts/ends/locks current data, hydrates, repairs or activates. Caller rolls
// back on every failure and discards the connection when session is uncertain.
// Schema 0055 is mandatory. All five projections share the base's aggregate
// ceilings; no truncation or second budget. Output is unchanged on failure.
unsigned int sql_room_item_payload_capture_sources_in_transaction(
	MYSQL *, const economic_sql_source_limits &, const economic_sql_physical_source_snapshot &,
	sql_room_item_source_snapshot *, size_t maximum_diagnostics = 512) noexcept;

// Complete original passive ordinary-drop capture under actual prospective
// requests. Caller owns input/prior output and authentic sibling CURRENT.
// Strong output; no SQL call, custody, publication, admission or activation.
bool sql_room_item_payload_capture_bounded(const item_transfer_payload &,
					   sql_room_item_payload_batch *,
					   bool (*)(size_t, void *) noexcept, void *,
					   size_t) noexcept;
bool sql_room_item_payload_batch_current_heap_bytes(const sql_room_item_payload_batch &,
						    size_t *) noexcept;

#endif
