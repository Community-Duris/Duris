#ifndef ECONOMIC_SQL_ZONE_RESET_ITEM_TRANSACTION_H
#define ECONOMIC_SQL_ZONE_RESET_ITEM_TRANSACTION_H
#include "economy/zone_reset_item_accounting.h"
#include "item/item_transfer_command.h"
#ifdef __NO_MYSQL__
#include "no_mysql/mysql.h"
#else
#include <mysql/mysql.h>
#endif
#include <memory>
#include <span>

// Existing item custody outbox framing. Command22 classification belongs to the
// original coordinator; an outbox value never grants publication or admission.
constexpr uint16_t ZONE_RESET_ITEM_OUTBOX_DESTINATION = 4;
constexpr uint16_t ZONE_RESET_ITEM_OUTBOX_EVENT = 1;
constexpr uint16_t ZONE_RESET_ITEM_RESULT_VERSION = 1;

// Borrow the original root's reconnect-disabled IN_TRANS session and inbox.
// Root owns journal/admission, genuine reserved UIDs and generation/source
// provenance. Complete ordinary forests, including zero/nested coin piles,
// share one root; artifacts still require their actual owner. No commit,
// rollback, retry, UID issuance, world effects or ACK; any apply/finalize error
// requires original-root rollback/retirement rather than a durable refusal.
class economic_sql_zone_reset_item_transaction
{
    public:
	~economic_sql_zone_reset_item_transaction();
	economic_sql_zone_reset_item_transaction(const economic_sql_zone_reset_item_transaction &) =
		delete;
	economic_sql_zone_reset_item_transaction &
	operator=(const economic_sql_zone_reset_item_transaction &) = delete;
	static unsigned int prepare(MYSQL *, const critical_command &,
				    std::unique_ptr<economic_sql_zone_reset_item_transaction> *);
	unsigned int apply();
	unsigned int finalize();
	unsigned int verify_root_completion();
	const item_transfer_result &result() const;
	unsigned int result_code() const;

    private:
	struct implementation;
	std::unique_ptr<implementation> state_;
	explicit economic_sql_zone_reset_item_transaction(std::unique_ptr<implementation>);
};
// Structural accepted-command classification only. Ordinary execution still
// requires real literal/source/UID/session/season/room proofs during prepare.
bool economic_sql_zone_reset_item_command_supported(const critical_command &) noexcept;
// Exact command, canonical intent/plan, source claim, all indexed item and coin
// evidence, successful inbox and typed48/outbox. Historical proof deliberately
// does not consult current custody, room counter, season or accounting epoch.
// It does not invoke literal/origin readers (their retained proof calls here).
unsigned int
economic_sql_zone_reset_item_verify_retained(MYSQL *, const critical_command &,
					     unsigned int result_code,
					     std::span<const uint8_t> result_payload) noexcept;
#endif
