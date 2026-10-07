#ifndef ECONOMIC_SQL_NATIVE_MOBILE_BIRTH_TRANSACTION_H
#define ECONOMIC_SQL_NATIVE_MOBILE_BIRTH_TRANSACTION_H
#include "economy/native_mobile_birth_result.h"
#include "item/item_ownership_runtime.h"
#include "persistence/critical_command_completion.h"
#ifdef __NO_MYSQL__
#include "no_mysql/mysql.h"
#else
#include <mysql/mysql.h>
#endif
#include <memory>
#include <span>
#include <vector>

constexpr uint16_t ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR = 7;

// Authenticated LIVE native-mobile wallet lifetime, separate from player/PID
// wallets. native_revision is the CURRENT cash revision; birth_epoch remains
// the actual historical constructor epoch. Retirement has no owner here.
struct economic_sql_native_mobile_wallet_lifetime
{
	economic_account_key account{};
	uint64_t native_id = 0;
	economic_coin_vector balance{};
	uint64_t native_revision = 0;
	critical_operation_id creating_operation_id{}, birth_epoch{}, retiring_operation_id{};
	uint64_t mapping_revision = 0, active_native_id = 0;
	quest_mobile_lifetime_state native_state = {};
};
// Borrow the original reconnect-disabled IN_TRANS session while the caller owns
// global lifecycle/writer exclusion. Call BEFORE taking current mapping/native
// locks. Authenticates every selected historical birth inbox first, then exact
// mappings, ascending current native IDs, and the immutable published origin.
// Current LIVE cash/stock may have progressed; unknown origins/cash, inactive
// mappings and retirement explicitly refuse. No transaction boundary, writes,
// world/cache effects or player IDs. Existing snapshot row/cell limits apply;
// every failure, including SQL/session/allocation errors, leaves output unchanged.
// This is namespace/current-value proof, not complete native-source capture,
// independent financial reconciliation or reconstruction/publication authority.
unsigned int economic_sql_native_mobile_birth_lock_wallet_lifetimes(
	MYSQL *, const critical_operation_id &lineage,
	std::vector<economic_sql_native_mobile_wallet_lifetime> *) noexcept;

// Authenticated historical origin, separate from current cash/admission and
// publication/ACK proof. Original epoch is never inferred from current epoch.
struct native_mobile_wallet_origin
{
	critical_operation_id birth_operation{}, lineage{}, birth_epoch{};
	uint64_t mobile_instance_id = 0, wallet_mapping_id = 0;
};
// Exact committed canonical plan/intent, typed MBR1 result, mapping and complete
// original financial/item/source/outbox evidence. Caller owns reconnect-disabled
// IN_TRANS session. No command/image reconstruction, schema changes or writes.
// Strong output on every refusal; current fee/native proof is separately needed.
unsigned int
economic_sql_native_mobile_birth_observe_origin(MYSQL *, const quest_mobile_native_reference &,
						native_mobile_wallet_origin *) noexcept;

// Root first owns the actual journal/admission, reserved identities and original
// generation/source decision. Borrow its reconnect-disabled IN_TRANS session and
// inbox. Values do not prove producer provenance. No transaction commit/rollback,
// retry, native/item UID issue, world effects or ACK. Any error after apply starts requires
// original-root rollback or retirement, never a fabricated durable refusal.
class economic_sql_native_mobile_birth_transaction
{
    public:
	~economic_sql_native_mobile_birth_transaction();
	economic_sql_native_mobile_birth_transaction(
		const economic_sql_native_mobile_birth_transaction &) = delete;
	economic_sql_native_mobile_birth_transaction &
	operator=(const economic_sql_native_mobile_birth_transaction &) = delete;
	static unsigned int
	prepare(MYSQL *, const critical_command &,
		std::unique_ptr<economic_sql_native_mobile_birth_transaction> *);
	unsigned int apply();
	unsigned int finalize();
	unsigned int verify_root_completion();
	const native_mobile_birth_result &result() const;
	unsigned int result_code() const;

    private:
	struct implementation;
	std::unique_ptr<implementation> state_;
	explicit economic_sql_native_mobile_birth_transaction(std::unique_ptr<implementation>);
};
bool economic_sql_native_mobile_birth_command_supported(const critical_command &) noexcept;
// Original retained success only: exact mapping/plan/evidence/receipt/outbox.
// No current epoch/body or active-mapping requirement; original session required.
unsigned int
economic_sql_native_mobile_birth_verify_retained(MYSQL *, const critical_command &,
						 unsigned int result_code,
						 std::span<const uint8_t> result_payload) noexcept;
// Authenticate the original completed inbox/result/receipt, then lock and verify
// the exact still-current born image, wallet lifetime and complete item custody.
// Caller owns the reconnect-disabled IN_TRANS session and publication boundary.
// No world effects, ID issuance, transaction commit/rollback or ACK. Outputs
// remain unchanged on failure; historical receipt proof alone cannot publish.
unsigned int economic_sql_native_mobile_birth_lock_publication(
	MYSQL *, const critical_command &, const critical_completion &, quest_mobile_native_image *,
	std::vector<item_ownership_runtime_entry> *) noexcept;
#endif
