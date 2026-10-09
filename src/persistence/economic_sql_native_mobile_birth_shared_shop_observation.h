#ifndef ECONOMIC_SQL_NATIVE_MOBILE_BIRTH_SHARED_SHOP_OBSERVATION_H
#define ECONOMIC_SQL_NATIVE_MOBILE_BIRTH_SHARED_SHOP_OBSERVATION_H

#include "economy/native_mobile_birth_cash_role_command.h"
#include "persistence/economic_accounting_repository.h"
#include "persistence/shop_item_runtime_payload.h"
#include <optional>

// Original-session BEFORE values only. These do not authenticate inbox/source,
// a factory, native-ID locks, admission, CAS, publication or ACK. Zero cash and
// zero owner revision remain distinct from missing keeper/owner rows.
struct economic_sql_native_mobile_birth_shared_shop_before
{
	economic_sql_authority_snapshot authority;
	unsigned long original_session = 0;
	std::vector<uint8_t> original_command;
	uint32_t shop_id = 0, keeper_id = 0;
	item_owner_identity shop_owner{};
	bool keeper_present = false, owner_present = false;
	uint64_t shop_revision = 0, owner_revision = 0;
	int64_t keeper_cash = 0;
	int32_t keeper_vnum = 0;
	bool keeper_roaming = false, payload_checkpoint_present = false;
	uint64_t payload_checkpoint_revision = 0;
	// Exact id/shop_id/mob_vnum/room_vnum/save_time/cash/shop_revision/
	// keeper_roaming/updated_at/runtime_payload_checkpoint_revision SQL cells.
	// NULL/uninterpreted saved room/time metadata are never replaced by defaults.
	std::vector<std::optional<std::string>> keeper_cells;
};

#ifndef __NO_MYSQL__
struct economic_sql_native_mobile_birth_shared_shop_custody
{
	economic_item_snapshot snapshot;
	int32_t vnum = 0;
	// Actual presence/length for every row. Bytes are retained only for the
	// selected SHOP history; foreign claims never gain literal-property proof.
	bool coin_payload_present = false;
	uint64_t coin_payload_bytes = 0;
	std::optional<std::vector<uint8_t>> coin_payload;
};
struct economic_sql_native_mobile_birth_shared_shop_stock
{
	economic_sql_native_mobile_birth_shared_shop_before before;
	// Entire selected SHOP history and explicit born IDs/active foreign links,
	// globally sorted, including genuine inactive/unmatched custody rows.
	std::vector<economic_sql_native_mobile_birth_shared_shop_custody> custody;
	// Actual current native row IDs serve only routing; no historical ID test.
	std::map<uint64_t, uint64_t> physical_routes;
	shop_item_runtime_image keeper_items;
	// Exact id/shopkeeper_id/type/duration/modifier/location/bitvector1..5.
	std::vector<std::vector<std::optional<std::string>>> keeper_affects;
};
#else
struct economic_sql_native_mobile_birth_shared_shop_stock;
#endif

// Phase 1: after the root's genuine inbox/source exclusion, BEFORE native-ID
// locks. Full original NMB4/shared NBC4 correlation -> original lineage shared
// lock -> actual keeper by configured slot -> canonical SHOP-only owner counter.
// Does not read custody/physical rows or create a missing owner/keeper/mapping.
unsigned int economic_sql_native_mobile_birth_shared_shop_observe_before_locked(
	MYSQL *, const critical_command &,
	economic_sql_native_mobile_birth_shared_shop_before *) noexcept;

// Phase 2: root has now acquired its original ascending native-ID locks. Keep
// the SAME reconnect-disabled transaction across both phases; supplied BEFORE
// values cannot prove that lock lifetime. Recheck exact command/session/row/
// owner, then global sorted custody before native stock/sidecars. The caller
// still owns complete native/source/history absence and every subsequent DML.
// Nonempty legacy stock without full UID/literal evidence refuses ENODATA.
// SELECT-only; no begin/commit/rollback/retry, AFTER clock or wallet inference.
// Both functions preserve every output on failure, including SQL errors.
unsigned int economic_sql_native_mobile_birth_shared_shop_observe_stock_locked(
	MYSQL *, const critical_command &,
	const economic_sql_native_mobile_birth_shared_shop_before &,
	economic_sql_native_mobile_birth_shared_shop_stock *) noexcept;

// Distinct CURRENT/AFTER stock counterpart. Caller already owns phase1
// SHOP/owner and ascending native-ID locks on the same original session.
// Requires the born UID forest present under the selected keeper; the original
// BEFORE observer continues to require those identities absent. Full current
// custody is locked globally before physical rows. Values grant no write/ACK.
unsigned int economic_sql_native_mobile_birth_shared_shop_observe_current_stock_locked(
	MYSQL *, const critical_command &,
	const economic_sql_native_mobile_birth_shared_shop_before &,
	economic_sql_native_mobile_birth_shared_shop_stock *) noexcept;

#endif
