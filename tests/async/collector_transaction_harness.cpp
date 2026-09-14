#include "economy/collector_transaction.h"
#include "economy/collector_runtime.h"
#include "economy/currency_transaction.h"
#include "item/item_ownership_runtime.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

namespace
{
char_data character = {};
pc_only_data pc = {};
bool player_online = true;
critical_command submitted_command = {};
size_t submit_count = 0;
size_t wallet_publications = 0;
size_t ownership_publications = 0;
size_t runtime_publications = 0;
size_t outbox_publications = 0;
size_t outbox_resumes = 0;
size_t passthrough_deliveries = 0;
bool completion_called = false;
bool completion_committed = false;
bool ownership_publication_succeeds = true;
unsigned int completion_error = 0;
collector_action completion_action = collector_action::unknown;

constexpr char death_operation[] = "123456789abcdef0123456789abcdef0";

collector::record available_record()
{
	collector::rules policy;
	policy.enabled = true;
	collector::record entry;
	assert(collector::enroll(77, death_operation, 42, 101, 5, 1000, policy, &entry) ==
	       collector::outcome::applied);
	assert(collector::collect(&entry, entry.revision, 5, 5, true, 75, entry.collect_at) ==
	       collector::outcome::applied);
	assert(collector::activate(&entry, entry.revision, entry.sale_at) ==
	       collector::outcome::applied);
	return entry;
}

collector_command_payload purchase_payload(const collector::record &entry)
{
	collector_command_payload payload;
	payload.action = collector_action::purchase;
	payload.target_state = item_custody_state::active;
	payload.capacity_admitted = true;
	payload.listing = entry.listing;
	payload.expected_listing_revision = entry.revision;
	payload.observed_at = entry.available_at;
	payload.actor_pid = 42;
	payload.racewar = 1;
	strcpy(payload.account_name.data(), "CollectorTester");
	payload.expected_wallet_revision = 10;
	payload.expected_bank_revision = 11;
	payload.from_owner = { item_owner_type::collector, item_collector_owner_id(entry.listing),
			       0 };
	payload.to_owner = { item_owner_type::player, 42, 0 };
	payload.expected_from_owner_revision = 1;
	payload.expected_to_owner_revision = 20;
	payload.selected_item_uid = entry.uid;
	payload.item_count = 1;
	payload.items[0] = { entry.uid,		  entry.uid, 0,
			     entry.item_revision, 501,	     item_custody_state::active };
	payload.item_blob_size = 4;
	payload.item_blob[0] = 1;
	payload.item_blob[1] = 2;
	payload.item_blob[2] = 3;
	payload.item_blob[3] = 4;
	return payload;
}

collector_command_result purchase_result(collector::record entry)
{
	assert(collector::purchase(&entry, entry.revision, 42, entry.price_value, true,
				   entry.available_at) == collector::outcome::applied);
	collector_command_result result;
	result.action = collector_action::purchase;
	result.record_present = true;
	result.catalog_revision = 50;
	result.from_owner_revision = 2;
	result.to_owner_revision = 21;
	result.wallet = { { 1, 2, 3, 4 } };
	result.bank = { { 5, 6, 7, 8 } };
	result.wallet_revision = 11;
	result.bank_revision = 12;
	result.entry = entry;
	return result;
}

collector_command_payload collect_payload(collector::record entry)
{
	collector_command_payload payload;
	payload.action = collector_action::collect;
	payload.target_state = item_custody_state::active;
	payload.listing = entry.listing;
	payload.expected_listing_revision = entry.revision;
	payload.observed_at = entry.collect_at;
	payload.from_owner = { item_owner_type::corpse, item_corpse_owner_id(42, 3), 0 };
	payload.to_owner = { item_owner_type::collector, item_collector_owner_id(entry.listing),
			     0 };
	payload.expected_from_owner_revision = 9;
	payload.expected_to_owner_revision = 0;
	payload.selected_item_uid = entry.uid;
	payload.item_count = 1;
	payload.items[0] = { entry.uid,		  entry.uid, 0,
			     entry.item_revision, 501,	     item_custody_state::active };
	payload.item_blob_size = 1;
	payload.item_blob[0] = 0xaa;
	return payload;
}

collector_command_result collect_result(collector::record entry)
{
	assert(collector::collect(&entry, entry.revision, entry.item_revision, entry.item_revision,
				  true, 75, entry.collect_at) == collector::outcome::applied);
	collector_command_result result;
	result.action = collector_action::collect;
	result.record_present = true;
	result.catalog_revision = 40;
	result.from_owner_revision = 10;
	result.to_owner_revision = 1;
	result.entry = entry;
	return result;
}

critical_completion completion(const collector_command_result &result,
			       critical_apply_outcome outcome = critical_apply_outcome::applied,
			       unsigned int error = 0)
{
	critical_completion value = {};
	value.operation_id = submitted_command.operation_id;
	value.outcome = outcome;
	value.error_code = error;
	std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> encoded = {};
	assert(collector_command_encode_result(result, &encoded));
	value.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), value.result_payload.begin());
	return value;
}

void completed(P_char completed_character, bool committed, const collector_command_result &result,
	       unsigned int error_code, const collector_command_payload &payload)
{
	assert(completed_character == (payload.actor_pid ? &character : nullptr));
	completion_called = true;
	completion_committed = committed;
	completion_error = error_code;
	completion_action = result.action;
}
}

critical_submit_result critical_command_coordinator_submit(critical_command command)
{
	submitted_command = std::move(command);
	++submit_count;
	return critical_submit_result::accepted;
}

P_char find_player_by_pid(int pid)
{
	return player_online && pid == 42 ? &character : nullptr;
}

bool currency_transaction_publish_balances(P_char published_character, const char *account_name,
					   uint8_t racewar, const currency_vector &,
					   const currency_vector &, uint64_t wallet_revision,
					   uint64_t bank_revision)
{
	assert(published_character == &character && !strcmp(account_name, "CollectorTester") &&
	       racewar == 1 && wallet_revision == 11 && bank_revision == 12);
	++wallet_publications;
	return true;
}

bool item_ownership_runtime_apply_collector(const collector_command_payload &payload,
					    const collector_command_result &result)
{
	assert(payload.action == result.action && result.record_present);
	++ownership_publications;
	return ownership_publication_succeeds;
}

bool collector_runtime_publish(const collector_command_result &result)
{
	assert(result.record_present);
	++runtime_publications;
	return true;
}

bool collector_publish_committed_event(const collector_command_result &result,
				       unsigned long long outbox_id)
{
	assert(result.record_present && outbox_id == 99);
	++outbox_publications;
	return true;
}

critical_outbox_delivery_result critical_outbox_test_destination(const critical_outbox_record &,
								 void *)
{
	++passthrough_deliveries;
	return critical_outbox_delivery_result::delivered;
}

void critical_outbox_resume(void)
{
	++outbox_resumes;
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	abort();
}

int main()
{
	character.only.pc = &pc;
	pc.pid = 42;
	auto available = available_record();
	auto purchase = purchase_payload(available);
	auto purchased = purchase_result(available);
	critical_operation_id purchase_operation = {};
	purchase_operation.bytes[0] = 0xa5;
	critical_operation_id zero_operation = {};
	assert(!collector_transaction_submit_identified(&character, zero_operation, purchase,
							completed));
	assert(collector_transaction_submit_identified(&character, purchase_operation, purchase,
						       completed));
	assert(critical_operation_id_equal(submitted_command.operation_id, purchase_operation));
	assert(collector_transaction_player_busy(&character));
	assert(!collector_transaction_submit(&character, purchase, completed));
	auto purchase_completion = completion(purchased);
	player_online = false;
	collector_transaction_handle_completions(&purchase_completion, 1);
	assert(collector_transaction_player_busy(&character) && !completion_called &&
	       !wallet_publications && !ownership_publications && !runtime_publications);
	player_online = true;
	collector_transaction_player_ready(&character);
	assert(!collector_transaction_player_busy(&character) && completion_called &&
	       completion_committed && completion_error == 0 &&
	       completion_action == collector_action::purchase && wallet_publications == 1 &&
	       ownership_publications == 1 && runtime_publications == 1);

	completion_called = completion_committed = false;
	collector_command_result rejected;
	rejected.action = collector_action::purchase;
	assert(collector_transaction_submit(&character, purchase, completed));
	auto rejected_completion =
		completion(rejected, critical_apply_outcome::terminal_failure, EAGAIN);
	collector_transaction_handle_completions(&rejected_completion, 1);
	assert(completion_called && !completion_committed && completion_error == EAGAIN &&
	       wallet_publications == 1 && ownership_publications == 1 &&
	       runtime_publications == 1);

	completion_called = completion_committed = false;
	collector::rules policy;
	policy.enabled = true;
	collector::record candidate;
	assert(collector::enroll(88, death_operation, 42, 202, 7, 1000, policy, &candidate) ==
	       collector::outcome::applied);
	auto collect = collect_payload(candidate);
	auto collected = collect_result(candidate);
	assert(collector_transaction_submit_background(collect, completed));
	auto collect_completion = completion(collected);
	collector_transaction_handle_completions(&collect_completion, 1);
	assert(completion_called && completion_committed &&
	       completion_action == collector_action::collect && ownership_publications == 2 &&
	       runtime_publications == 2 && wallet_publications == 1);

	// Once durable authority commits, a local cache failure must never be
	// reported as a rejected custody or wallet operation.
	completion_called = completion_committed = false;
	auto recovery_candidate = candidate;
	recovery_candidate.listing = 89;
	recovery_candidate.uid = 203;
	auto recovery_collect = collect_payload(recovery_candidate);
	recovery_collect.listing = 89;
	recovery_collect.to_owner = { item_owner_type::collector, item_collector_owner_id(89), 0 };
	recovery_collect.selected_item_uid = 203;
	recovery_collect.items[0].item_uid = 203;
	recovery_collect.items[0].root_item_uid = 203;
	auto recovery_collected = collect_result(recovery_candidate);
	ownership_publication_succeeds = false;
	assert(collector_transaction_submit_background(recovery_collect, completed));
	auto recovery_completion = completion(recovery_collected);
	collector_transaction_handle_completions(&recovery_completion, 1);
	assert(completion_called && completion_committed && completion_error == ESTALE &&
	       completion_action == collector_action::collect && ownership_publications == 3 &&
	       runtime_publications == 2);
	ownership_publication_succeeds = true;

	completion_called = completion_committed = false;
	auto malformed_candidate = candidate;
	malformed_candidate.listing = 90;
	malformed_candidate.uid = 204;
	auto malformed_collect = collect_payload(malformed_candidate);
	malformed_collect.listing = 90;
	malformed_collect.to_owner = { item_owner_type::collector, item_collector_owner_id(90), 0 };
	malformed_collect.selected_item_uid = 204;
	malformed_collect.items[0].item_uid = 204;
	malformed_collect.items[0].root_item_uid = 204;
	assert(collector_transaction_submit_background(malformed_collect, completed));
	critical_completion malformed_completion = {};
	malformed_completion.operation_id = submitted_command.operation_id;
	malformed_completion.outcome = critical_apply_outcome::applied;
	collector_transaction_handle_completions(&malformed_completion, 1);
	assert(completion_called && completion_committed && completion_error == EBADMSG &&
	       completion_action == collector_action::unknown && ownership_publications == 3 &&
	       runtime_publications == 2);

	std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> encoded = {};
	assert(collector_command_encode_result(collected, &encoded));
	critical_outbox_record record;
	record.outbox_id = 99;
	record.destination = COLLECTOR_OUTBOX_DESTINATION;
	record.event_type = COLLECTOR_OUTBOX_EVENT_MUTATED;
	record.payload_version = COLLECTOR_COMMAND_RESULT_VERSION;
	record.payload.assign(encoded.begin(), encoded.end());
	assert(collector_transaction_outbox_delivery(record, nullptr) ==
	       critical_outbox_delivery_result::retryable_failure);
	collector_transaction_publish_outbox();
	assert(outbox_publications == 1 && outbox_resumes == 1);
	assert(collector_transaction_outbox_delivery(record, nullptr) ==
	       critical_outbox_delivery_result::delivered);
	record.event_type++;
	assert(collector_transaction_outbox_delivery(record, nullptr) ==
	       critical_outbox_delivery_result::terminal_failure);
	record.destination = 1;
	assert(collector_transaction_outbox_delivery(record, nullptr) ==
		       critical_outbox_delivery_result::delivered &&
	       passthrough_deliveries == 1);

	collector_transaction_reset_for_tests();
	assert(submit_count == 5);
}
