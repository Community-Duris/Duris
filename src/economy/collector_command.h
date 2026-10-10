#ifndef DURIS_COLLECTOR_COMMAND_H
#define DURIS_COLLECTOR_COMMAND_H

#include "economy/collector_codec.h"
#include "economy/currency_command.h"
#include "item/item_transfer_command.h"
#include "persistence/critical_command.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

constexpr uint16_t COLLECTOR_COMMAND_PAYLOAD_VERSION = 1;
constexpr uint8_t COLLECTOR_COMMAND_RESULT_VERSION = 2;
constexpr uint8_t COLLECTOR_COMMAND_PREVIOUS_RESULT_VERSION = 1;
constexpr size_t COLLECTOR_COMMAND_MAX_ITEMS = ITEM_TRANSFER_MAX_ITEMS;
constexpr size_t COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES = ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES;
constexpr size_t COLLECTOR_COMMAND_RESULT_BYTES = 272;
constexpr uint16_t COLLECTOR_OUTBOX_DESTINATION = 11;
constexpr uint16_t COLLECTOR_OUTBOX_EVENT_MUTATED = 1;

enum class collector_action : uint8_t
{
	unknown = 0,
	collect,
	activate,
	purchase,
	expire,
	cancel,
	pause,
	resume,
	// Claim and acknowledge the durable first-availability hint. These metadata
	// actions never transfer an item or change a listing's price/eligibility.
	hint,
	hint_ack,
};

// Collection carries every row in the selected item's current source root. The
// repository can therefore prove the root is complete before detaching only the
// selected item and repairing the topology of the items that remain. Other
// custody actions carry the single held collector item.
struct collector_command_payload
{
	collector_action action = collector_action::unknown;
	collector::reason cancel_reason = collector::reason::none;
	item_custody_state target_state = item_custody_state::absent;
	bool capacity_admitted = false;
	uint64_t listing = 0;
	uint64_t expected_listing_revision = 0;
	uint64_t observed_at = 0;
	uint32_t actor_pid = 0;
	uint8_t racewar = 0;
	std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name = {};
	uint64_t expected_wallet_revision = 0;
	uint64_t expected_bank_revision = 0;
	item_owner_identity from_owner = { item_owner_type::unknown, 0, 0 };
	item_owner_identity to_owner = { item_owner_type::unknown, 0, 0 };
	uint64_t expected_from_owner_revision = 0;
	uint64_t expected_to_owner_revision = 0;
	uint64_t selected_item_uid = 0;
	uint16_t item_count = 0;
	std::array<item_transfer_entry, COLLECTOR_COMMAND_MAX_ITEMS> items = {};
	uint32_t item_blob_size = 0;
	std::array<uint8_t, COLLECTOR_COMMAND_ITEM_BLOB_MAX_BYTES> item_blob = {};
};

// A successful result publishes the complete post-transition record so the
// game thread can update the indexed due queue without another durable read.
// Failed commands use record_present=false; their errno-style result code is
// carried by the critical-command envelope.
struct collector_command_result
{
	collector_action action = collector_action::unknown;
	bool record_present = false;
	uint64_t catalog_revision = 0;
	uint64_t from_owner_revision = 0;
	uint64_t to_owner_revision = 0;
	currency_vector wallet = {};
	currency_vector bank = {};
	uint64_t wallet_revision = 0;
	uint64_t bank_revision = 0;
	uint32_t materialized_item_id = 0;
	collector::record entry = {};
};

bool collector_command_encode_payload(const collector_command_payload &payload,
				      std::vector<uint8_t> *encoded);
bool collector_command_decode_payload(const critical_command &command,
				      collector_command_payload *payload);
bool collector_command_encode_result(const collector_command_result &result,
				     std::array<uint8_t, COLLECTOR_COMMAND_RESULT_BYTES> *encoded);
bool collector_command_decode_result(const uint8_t *encoded, size_t encoded_size,
				     collector_command_result *result);
bool collector_command_build(critical_command *command, critical_operation_id operation_id,
			     const collector_command_payload &payload,
			     critical_source_site source_site,
			     critical_deadline_class deadline_class);

// Full original parse and canonical fence reconstruction with prospective actual
// vector growth and source-carrier admission. Original partial payload semantics
// remain; caller owns input, output inline, old output and foreign retained state.
bool collector_command_decode_payload_bounded(const critical_command &, collector_command_payload *,
					      bool (*)(size_t, void *) noexcept, void *context,
					      size_t outer_live,
					      bool *capacity_refused = nullptr) noexcept;

// Complete original payload decoder SOURCE preentry is transient. Retain only
// the disjoint uncovered SOURCE supplement in actual caller outer through the
// unchanged bounded child; input/output/prior heap and caller frames stay own.
bool collector_command_decode_payload_source_frame_bytes(size_t *) noexcept;
bool collector_command_decode_payload_source_supplement_frame_bytes(size_t *) noexcept;
bool collector_command_decode_payload_initial_inline_bytes(size_t *) noexcept;
constexpr size_t collector_command_decode_payload_source_query_frame_bytes() noexcept
{
	// Public query output P, complete/supplement2N and returnedB; source-parts
	// output pair2P, returnedB and policy returnedB. Remaining carriers:
	// account/item pairs and owned/levels/extra locals; two genuine native
	// critical-source getter results, policy and checked-add arguments; the
	// account-only and item-owner nested fixed queries. Sequential
	// getters reuse this same complete fixed source graph.
	return 3 * sizeof(void *) + 11 * sizeof(size_t) + 3 * sizeof(bool) + sizeof(void *) +
	       sizeof(size_t) + sizeof(bool) + currency_account_key_source_query_frame_bytes() +
	       item_owner_key_source_query_frame_bytes();
}
#endif
