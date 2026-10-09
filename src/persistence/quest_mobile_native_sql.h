#ifndef QUEST_MOBILE_NATIVE_SQL_H
#define QUEST_MOBILE_NATIVE_SQL_H

#include "world/quest_mobile_native.h"
#ifdef __NO_MYSQL__
#include "no_mysql/mysql.h"
#else
#include <mysql/mysql.h>
#endif

// Value/session observation, not a birth/source/epoch/inbox authority or ACK.
// An absent row retains only the exact requested ID/session; image is empty.
struct quest_mobile_native_sql_row
{
	uint64_t mobile_instance_id = 0;
	unsigned long original_session = 0;
	bool present = false;
	quest_mobile_native_image image;
};
// Caller owns one original reconnect-disabled IN_TRANS session, canonical
// authority/inbox/exclusion and ascending mobile-ID locks BEFORE item custody.
// Never starts, commits, rolls back, replaces a session, issues IDs or touches world.
// Return 0 on exact value proof; otherwise errno/native SQL code, output unchanged.
int quest_mobile_native_sql_lock(MYSQL *, uint64_t mobile_instance_id,
				 quest_mobile_native_sql_row *) noexcept;
// Rechecks the complete original before under the same lock/session, then uses
// binary prepared DML and exact after readback. Parent owns typed admitted
// transition/increment validation and the entire item-custody atomic operation.
// Birth requires actual missing locked row and original parent == birth operation;
// bare values/session never authenticate an admitted birth. Existing birth facts
// cannot change and RETIRED cannot revive.
// Cash-aware birth and ordinary transitions require known v2 cash with the
// checked cash/mobile revision policy. Historical unknown cash is read-only here.
// Failure after DML requires caller transaction rollback/retirement;
// output preservation is not native rollback.
int quest_mobile_native_sql_apply_locked(MYSQL *, const critical_operation_id &parent,
					 const quest_mobile_native_sql_row &original_before,
					 const quest_mobile_native_image &after,
					 quest_mobile_native_sql_row *verified_after) noexcept;

// SELECT-only persisted catalog observation. Existing native/source2 formats,
// enums and SQL lock/apply authority remain unchanged.
#include "persistence/economic_sql_source_snapshot.h"
#include "persistence/sql_room_item_payload.h"
namespace quest_mobile_native_catalog_flags
{
inline constexpr uint64_t invalid_row = uint64_t{ 1 } << 0;
inline constexpr uint64_t duplicate_instance = uint64_t{ 1 } << 1;
inline constexpr uint64_t cash_unknown = uint64_t{ 1 } << 2;
inline constexpr uint64_t owner_clock_missing = uint64_t{ 1 } << 3;
inline constexpr uint64_t owner_clock_mismatch = uint64_t{ 1 } << 4;
inline constexpr uint64_t duplicate_item_uid = uint64_t{ 1 } << 5;
inline constexpr uint64_t custody_mismatch = uint64_t{ 1 } << 6;
inline constexpr uint64_t extra_related_custody = uint64_t{ 1 } << 7;
inline constexpr uint64_t unmatched_active_custody = uint64_t{ 1 } << 8;
inline constexpr uint64_t malformed_custody = uint64_t{ 1 } << 9;
inline constexpr uint64_t absent_catalog_owner = uint64_t{ 1 } << 10;
inline constexpr uint64_t retired_catalog_owner = uint64_t{ 1 } << 11;
inline constexpr uint64_t invalid_image = uint64_t{ 1 } << 12;
}
struct quest_mobile_native_sql_catalog_reference
{
	size_t row = 0;
	economic_sql_source_digest digest = {};
};
struct quest_mobile_native_sql_catalog_row
{
	// Exact raw6 selected bytes/NULLs, not a new storage or framing format.
	std::array<std::optional<std::string>, 6> cells;
	std::optional<uint64_t> mobile_instance_id, mobile_revision, stock_revision;
	std::optional<uint8_t> lifetime_state;
	std::optional<quest_mobile_native_image> image;
	// Addresses borrowed source2.tables[12], including an empty live forest.
	std::optional<quest_mobile_native_sql_catalog_reference> owner_revision;
	uint64_t flags = 0;
};
struct quest_mobile_native_sql_catalog_item
{
	size_t catalog_row = 0, image_item = 0;
	uint64_t root_item_uid = 0;
	std::optional<uint64_t> parent_item_uid, observed_item_revision;
	// Address borrowed source2.tables[11], tables[12], EIE2[0], respectively.
	std::optional<quest_mobile_native_sql_catalog_reference> custody, owner_revision, equipment;
	uint64_t flags = 0;
	bool current_field_correspondence = false;
};
struct quest_mobile_native_sql_catalog_finding
{
	uint64_t flag = 0;
	size_t catalog_row = SIZE_MAX, item = SIZE_MAX;
	// Optional borrowed source2.tables[11] observation; no raw values in details.
	std::optional<quest_mobile_native_sql_catalog_reference> custody;
};
struct quest_mobile_native_sql_catalog
{
	unsigned long original_session = 0;
	economic_sql_source_digest physical_digest = {};
	std::array<economic_sql_source_digest, 5> validated_room_table_digests = {};
	// Combined base+room+new raw6 counters, charging the existing packets once.
	uint64_t rows = 0, cells = 0, cell_bytes = 0;
	std::vector<quest_mobile_native_sql_catalog_row> catalog;
	std::vector<quest_mobile_native_sql_catalog_item> items;
	// All owner12 rows, including inactive history; each vector addresses table11.
	std::vector<quest_mobile_native_sql_catalog_reference> native_custody;
	std::vector<quest_mobile_native_sql_catalog_reference> related_custody, extra_custody,
		unmatched_active_custody, malformed_custody;
	// All owner12 clocks address table12; related equipment addresses EIE2[0].
	std::vector<quest_mobile_native_sql_catalog_reference> native_owner_revisions,
		related_equipment;
	// Complete findings are bounded by captured rows/codec items, independent of
	// the fixed512 detail ceiling. cash_unknown alone is not an item refusal.
	std::vector<quest_mobile_native_sql_catalog_finding> findings, diagnostics;
	std::array<uint64_t, 13> issue_counts = {};
	bool diagnostics_truncated = false;
};
// Caller owns the immutable EPH1/room packets and same original reconnect-disabled
// IN_TRANS RR consistent cut (READ ONLY or writable), prior writes/quiescence.
// Session-default isolation is not independently treated as active-cut proof.
// Takes MDL, verifies immutable0059 shape/InnoDB, SELECTs ALL rows without locks or
// lifetime filters, and borrows existing custody/clocks/equipment by references.
// Native body uses its original total4MiB codec ceiling; generic1MiB cells and
// shared262144 rows/4M cells/64MiB budgets remain unchanged. No source2/hash/schema
// changes, birth/lineage/origin proof, hydration, mutation or activation decision.
// Retired images are history. Decoded birth/reference facts do not authenticate
// origin; item revisions are observed custody values, never stock-derived.
// Malformed semantic observations publish complete findings; operational/schema/
// session/structural/allocation/bound errors preserve output and leave caller TX.
int quest_mobile_native_sql_capture_catalog_in_transaction(
	MYSQL *, const economic_sql_physical_source_snapshot &,
	const sql_room_item_source_snapshot &, const economic_sql_source_limits &,
	quest_mobile_native_sql_catalog *) noexcept;
#endif
