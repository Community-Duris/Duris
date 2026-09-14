#ifndef DURIS_COLLECTOR_POLICY_H
#define DURIS_COLLECTOR_POLICY_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

// Policy decisions only. Callers must commit the resulting record together with
// custody, payload, and wallet changes before publishing anything to the world.
namespace collector
{
constexpr uint16_t record_version = 1;
constexpr size_t death_operation_hex_size = 32;
using death_operation_id = std::array<char, death_operation_hex_size + 1>;

enum class state : uint8_t
{
	candidate = 1,
	collected,
	available,
	purchased,
	cancelled,
	expired,
};

enum class reason : uint8_t
{
	none = 0,
	claimed,
	destroyed,
	quarantined,
	excluded,
	character_deleted,
	season_reset,
	holding_elapsed,
};

struct rules
{
	bool enabled = false;
	uint64_t collection_delay = 12 * 60 * 60;
	uint64_t sale_delay = 24 * 60 * 60;
	uint64_t holding_duration = 7 * 24 * 60 * 60;
	uint64_t price_percent = 200;
	uint64_t minimum_value = 100; // Copper: one gold in the existing economy.
};

struct record
{
	uint16_t version = record_version;
	uint64_t listing = 0;
	death_operation_id death_operation = {};
	uint32_t beneficiary = 0;
	uint64_t uid = 0;
	uint64_t death_time = 0;
	uint64_t collect_at = 0;
	uint64_t sale_at = 0;
	uint64_t available_at = 0;
	uint64_t expires_at = 0;
	bool holding_paused = false;
	uint64_t paused_at = 0;
	uint64_t revision = 0;
	uint64_t item_revision = 0;
	uint64_t price_value = 0;
	rules policy;
	state status = state::candidate;
	reason closed_reason = reason::none;
};

enum class outcome
{
	applied,
	not_due,
	conflict,
	invalid,
	overflow,
	forbidden,
	insufficient_funds,
	capacity,
};

bool valid_rules(const rules &policy);
bool valid_record(const record &entry);
bool terminal(state status);
// All monetary values use copper, like obj_data::cost and auction transactions.
outcome price(int64_t base_value, const rules &policy, uint64_t *value);
outcome enroll(uint64_t listing, std::string_view death_operation, uint32_t beneficiary,
	       uint64_t uid, uint64_t item_revision, uint64_t death_time, const rules &policy,
	       record *result);
outcome cancel(record *entry, uint64_t expected_revision, reason why);
outcome collect(record *entry, uint64_t expected_revision, uint64_t expected_item_revision,
		uint64_t current_item_revision, bool active_unclaimed, int64_t current_base_value,
		uint64_t now);
outcome activate(record *entry, uint64_t expected_revision, uint64_t now);
outcome purchase(record *entry, uint64_t expected_revision, uint32_t actor, uint64_t carried_value,
		 bool has_capacity, uint64_t now);
outcome expire(record *entry, uint64_t expected_revision, uint64_t now);
outcome pause(record *entry, uint64_t expected_revision, uint64_t now);
outcome resume(record *entry, uint64_t expected_revision, uint64_t now);

// Bounded indexed scheduling. Leased work is moved to lease_until so a failed
// batch cannot starve later deadlines. Successful commit publication replaces or
// removes its entry; failure leaves the lease to expire. Leases are deliberately
// ephemeral and disappear when the queue is rebuilt from durable records.
class due_queue
{
    public:
	bool update(const record &entry);
	void erase(uint64_t listing);
	std::vector<uint64_t> lease_due(uint64_t now, size_t limit, uint64_t lease_until);
	size_t size() const { return by_listing.size(); }

    private:
	std::set<std::pair<uint64_t, uint64_t>> by_deadline;
	std::map<uint64_t, uint64_t> by_listing;
};
}

#endif
