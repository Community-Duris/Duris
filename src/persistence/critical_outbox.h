#ifndef CRITICAL_OUTBOX_H
#define CRITICAL_OUTBOX_H

#include <cstddef>
#include <cstdint>
#include <vector>

// Coin parents are audit receipts; their two child records publish the effects.
constexpr uint16_t CRITICAL_OUTBOX_COIN_RECEIPT_DESTINATION = 10;
constexpr uint16_t CRITICAL_OUTBOX_COIN_RECEIPT_EVENT = 1;
constexpr size_t CRITICAL_OUTBOX_COIN_RECEIPT_BYTES = 32;

constexpr size_t CRITICAL_OUTBOX_BATCH_MAX = 64;
constexpr size_t CRITICAL_OUTBOX_RECORD_MAX_BYTES = 65535;
constexpr size_t CRITICAL_OUTBOX_QUEUE_MAX_BYTES = 4 * 1024 * 1024;
constexpr unsigned int CRITICAL_OUTBOX_MAX_ATTEMPTS = 8;

enum class critical_outbox_delivery_result : uint8_t
{
	delivered,
	already_delivered,
	retryable_failure,
	terminal_failure,
};

struct critical_outbox_record
{
	uint64_t outbox_id;
	uint16_t destination;
	uint16_t event_type;
	uint16_t payload_version;
	unsigned int attempt;
	std::vector<uint8_t> payload;
};

struct critical_outbox_health
{
	uint64_t pending;
	uint64_t dead_letter;
	uint64_t oldest_age_msec;
	uint64_t incomplete_inbox;
	uint64_t committed_without_outbox;
	uint64_t fetched;
	uint64_t delivered;
	uint64_t duplicates;
	uint64_t retries;
	uint64_t terminal_failures;
	uint64_t db_failures;
	uint64_t high_water_records;
	uint64_t high_water_bytes;
	bool initialized;
	bool accepting;
	bool running;
};

struct critical_reconciliation_report
{
	uint64_t incomplete_inbox;
	uint64_t committed_without_outbox;
	uint64_t pending_outbox;
	uint64_t dead_letter_outbox;
};

using critical_outbox_deliver_fn =
	critical_outbox_delivery_result (*)(const critical_outbox_record &record, void *context);

bool critical_outbox_init(critical_outbox_deliver_fn deliver, void *context);
void critical_outbox_shutdown(void);
void critical_outbox_quiesce(void);
void critical_outbox_resume(void);
bool critical_outbox_drain(uint64_t timeout_msec);
critical_outbox_health critical_outbox_health_copy(void);
bool critical_outbox_reconcile(critical_reconciliation_report *report);
bool critical_outbox_retry_dead_letter(uint64_t outbox_id);
critical_outbox_delivery_result
critical_outbox_test_destination(const critical_outbox_record &record, void *context);

#endif
