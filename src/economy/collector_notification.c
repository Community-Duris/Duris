#include "economy/collector_notification.h"

#include "economy/collector_config.h"
#include "economy/collector_runtime.h"
#include "economy/collector_transaction.h"
#include "core/prototypes.h"
#include "persistence/critical_command.h"
#include "sql/sql.h"

#include <cerrno>
#include <climits>
#include <ctime>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr size_t notification_scan_limit = 256;
constexpr size_t notification_result_limit = 16;

struct pending_delivery
{
	collector::record entry = {};
	std::string message;
	critical_operation_id message_id = {};
	uint64_t claim_revision = 0;
	bool claim_in_flight = false;
	bool delivery_accepted = false;
	bool acknowledgement_in_flight = false;
};

std::map<uint64_t, pending_delivery> pending;
uint64_t candidate_cursor = 0;

uint64_t now_seconds(void)
{
	const time_t now = time(nullptr);
	return now > 0 ? static_cast<uint64_t>(now) : 0;
}

bool available_now(const collector::record &entry, uint64_t now)
{
	return entry.status == collector::state::available && !entry.holding_paused &&
	       entry.available_at <= now && now < entry.expires_at;
}

std::string deadline_text(uint64_t deadline)
{
	std::ostringstream output;
	output << deadline << " (Unix seconds)";
	if (deadline <= static_cast<uint64_t>(std::numeric_limits<time_t>::max()))
	{
		const time_t value = static_cast<time_t>(deadline);
		struct tm utc = {};
		if (gmtime_r(&value, &utc))
		{
			char formatted[32] = {};
			if (strftime(formatted, sizeof(formatted), "%Y-%m-%d %H:%M:%S UTC", &utc))
				output << "; " << formatted;
		}
	}
	return output.str();
}

bool message_id_for(const collector::record &entry, critical_operation_id *message_id)
{
	return message_id &&
	       critical_operation_id_from_hex(entry.death_operation.data(), message_id) &&
	       !critical_operation_id_is_zero(*message_id);
}

void hint_completed(P_char character, bool committed, const collector_command_result &result,
		    unsigned int error_code, const collector_command_payload &payload);
void hint_acknowledged(P_char character, bool committed, const collector_command_result &result,
		       unsigned int error_code, const collector_command_payload &payload);

bool submit_acknowledgement(pending_delivery &delivery)
{
	if (delivery.acknowledgement_in_flight || !delivery.delivery_accepted ||
	    collector_transaction_listing_busy(delivery.entry.listing))
		return false;
	const uint64_t now = now_seconds();
	if (!now || !delivery.claim_revision)
		return false;
	collector_command_payload payload = {};
	payload.action = collector_action::hint_ack;
	payload.listing = delivery.entry.listing;
	// This is the revision recorded by the claim, not necessarily the listing's
	// current revision. The repository permits an acknowledgement to race with
	// purchase/expiry while still binding it to the claimed hint.
	payload.expected_listing_revision = delivery.claim_revision;
	payload.observed_at = now;
	if (!collector_transaction_submit_background(payload, hint_acknowledged))
		return false;
	delivery.acknowledgement_in_flight = true;
	return true;
}

bool deliver(pending_delivery &delivery)
{
	if (delivery.delivery_accepted)
		return submit_acknowledgement(delivery);
	const uint64_t now = now_seconds();
	collector::record current = {};
	if (!now || !collector_runtime_find(delivery.entry.listing, &current) ||
	    !available_now(current, now))
		return false;
	if (current.revision != delivery.entry.revision)
	{
		std::string refreshed;
		if (!collector_notification_format(current, &refreshed))
			return false;
		delivery.entry = current;
		delivery.message = std::move(refreshed);
	}
	if (!delivery.entry.beneficiary || delivery.entry.beneficiary > INT_MAX)
		return false;
	if (send_to_pid(delivery.message.c_str(), static_cast<int>(delivery.entry.beneficiary)))
	{
		delivery.delivery_accepted = true;
		(void)submit_acknowledgement(delivery);
		return true;
	}
	if (send_to_pid_offline_deduplicated(delivery.message.c_str(),
					     static_cast<int>(delivery.entry.beneficiary),
					     delivery.message_id.bytes.data()))
	{
		delivery.delivery_accepted = true;
		(void)submit_acknowledgement(delivery);
		return true;
	}
	return false;
}

void start_delivery(const collector::record &entry, uint64_t claim_revision, bool claim_in_flight)
{
	if (!entry.listing || pending.find(entry.listing) != pending.end())
		return;
	const uint64_t now = now_seconds();
	if (!now || !available_now(entry, now) || !claim_revision)
		return;
	std::string message;
	critical_operation_id message_id = {};
	if (!collector_notification_format(entry, &message) || !message_id_for(entry, &message_id))
		return;
	pending_delivery delivery;
	delivery.entry = entry;
	delivery.message = std::move(message);
	delivery.message_id = message_id;
	delivery.claim_revision = claim_revision;
	delivery.claim_in_flight = claim_in_flight;
	try
	{
		pending.emplace(entry.listing, std::move(delivery));
	}
	catch (...)
	{
		return;
	}
}

void schedule(const collector::record &entry)
{
	if (!entry.listing || pending.find(entry.listing) != pending.end() ||
	    collector_transaction_listing_busy(entry.listing))
		return;
	const uint64_t now = now_seconds();
	if (!now || !available_now(entry, now))
		return;
	collector_death_snapshot death = {};
	if (!collector_runtime_find_death(entry.beneficiary, entry.death_time, &death) ||
	    death.hint_state == COLLECTOR_HINT_DELIVERED)
		return;
	if (death.hint_state == COLLECTOR_HINT_PENDING)
	{
		start_delivery(entry, death.hint_revision, false);
		return;
	}
	if (death.hint_state == COLLECTOR_HINT_NONE)
	{
		start_delivery(entry, entry.revision, true);
		const auto found = pending.find(entry.listing);
		if (found == pending.end())
			return;
		collector_command_payload payload = {};
		payload.action = collector_action::hint;
		payload.listing = entry.listing;
		payload.expected_listing_revision = entry.revision;
		payload.observed_at = now;
		if (!collector_transaction_submit_background(payload, hint_completed))
			pending.erase(found);
	}
}

void hint_completed(P_char character, bool committed, const collector_command_result &result,
		    unsigned int error_code, const collector_command_payload &payload)
{
	(void)character;
	(void)error_code;
	const auto found = pending.find(payload.listing);
	if (found == pending.end())
		return;
	pending_delivery &delivery = found->second;
	if (!committed || result.action != collector_action::hint || !result.record_present)
	{
		pending.erase(found);
		return;
	}
	const uint64_t claim_revision = payload.expected_listing_revision;
	if (!claim_revision || !collector_notification_format(result.entry, &delivery.message))
	{
		pending.erase(found);
		return;
	}
	delivery.entry = result.entry;
	delivery.claim_revision = claim_revision;
	delivery.claim_in_flight = false;
	(void)deliver(delivery);
}

void hint_acknowledged(P_char character, bool committed, const collector_command_result &result,
		       unsigned int error_code, const collector_command_payload &payload)
{
	(void)character;
	(void)result;
	const auto found = pending.find(payload.listing);
	if (found == pending.end())
		return;
	if (committed || error_code == EALREADY)
	{
		pending.erase(found);
		return;
	}
	found->second.acknowledgement_in_flight = false;
}
} // namespace

bool collector_notification_format(const collector::record &entry, std::string *message)
{
	if (!message || !collector::valid_record(entry) ||
	    entry.status != collector::state::available || entry.holding_paused ||
	    !entry.beneficiary || !entry.available_at || !entry.expires_at ||
	    entry.expires_at <= entry.available_at)
		return false;
	std::ostringstream output;
	output << "The Collector's ledger now holds an eligible recovery for you.\r\n"
	       << "Find the Collector of Antiquities in any registered auction room.\r\n"
	       << "Commands: collector list\r\n"
	       << "Inspect a listing with: collector inspect <number>\r\n"
	       << "Buy a listing with: collector buy <number>\r\n"
	       << "Carry the fee: " << entry.price_value << " copper.\r\n"
	       << "Holding deadline: " << deadline_text(entry.expires_at) << ".\r\n";
	*message = output.str();
	return !message->empty();
}

void collector_notification_on_available(const collector::record &entry)
{
	if (!collector_config_enabled())
		return;
	schedule(entry);
}

void collector_notification_pulse(void)
{
	std::vector<uint64_t> listing_ids;
	try
	{
		listing_ids.reserve(pending.size());
		for (const auto &[listing, delivery] : pending)
		{
			(void)delivery;
			listing_ids.push_back(listing);
		}
	}
	catch (...)
	{
		return;
	}
	for (uint64_t listing : listing_ids)
	{
		auto found = pending.find(listing);
		if (found == pending.end())
			continue;
		if (found->second.claim_in_flight)
			continue;
		if (!deliver(found->second) && !found->second.delivery_accepted)
		{
			collector::record current = {};
			const uint64_t now = now_seconds();
			if (!now || !collector_runtime_find(listing, &current) ||
			    !available_now(current, now))
				pending.erase(found);
		}
	}
	if (!collector_config_enabled())
		return;
	std::vector<collector::record> candidates;
	uint64_t next_cursor = 0;
	bool reached_end = false;
	if (!collector_runtime_hint_candidates(candidate_cursor, notification_scan_limit,
					       notification_result_limit, now_seconds(),
					       &candidates, &next_cursor, &reached_end))
		return;
	candidate_cursor = reached_end ? 0 : next_cursor;
	for (const collector::record &entry : candidates)
		schedule(entry);
}

void collector_notification_player_ready(void)
{
	collector_notification_pulse();
}

void collector_notification_death_enrolled(void)
{
	collector_notification_pulse();
}

void collector_notification_reset_for_tests(void)
{
	pending.clear();
	candidate_cursor = 0;
}

size_t collector_notification_pending_count(void)
{
	return pending.size();
}
