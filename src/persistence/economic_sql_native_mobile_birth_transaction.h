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

constexpr uint16_t ECONOMIC_NATIVE_MOBILE_WALLET_LOCATOR = 7;
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
