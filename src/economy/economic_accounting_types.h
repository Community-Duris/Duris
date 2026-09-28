#ifndef DURIS_ECONOMIC_ACCOUNTING_TYPES_H
#define DURIS_ECONOMIC_ACCOUNTING_TYPES_H

#include "persistence/critical_command.h"
#include "item/item_transfer_command.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

constexpr uint16_t ECONOMIC_ACCOUNTING_VERSION = 1;
constexpr size_t ECONOMIC_ACCOUNT_KEY_BYTES = 40;
constexpr size_t ECONOMIC_ACCOUNTING_MAX_INTENT_BYTES = 8192;
constexpr size_t ECONOMIC_ACCOUNTING_MAX_ACCOUNTS = 3072;
constexpr size_t ECONOMIC_ACCOUNTING_MAX_POSTINGS = 6144;
constexpr size_t ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS = 3000;
constexpr size_t ECONOMIC_ACCOUNTING_MAX_CHILDREN = 64;
constexpr size_t ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES = 4 * 1024 * 1024;

// Persist these explicit numbers. Names and aliases never identify an account.
enum class economic_account_kind : uint16_t
{
	wallet = 1,
	bank = 2,
	pile = 3,
	auction_escrow = 4,
	pending_claim = 5,
	treasury = 6,
	issuance = 7,
	sink = 8,
	opening = 9,
	restitution = 10,
};

enum class economic_accounting_error : uint8_t
{
	ok,
	invalid_version,
	invalid_identity,
	invalid_reason,
	unauthorized,
	unbalanced,
	overflow,
	negative_holding,
	capacity,
	duplicate_event,
	payload_conflict,
	stale_revision,
	topology,
	incomplete_coverage,
	unresolved,
	corrupt_evidence,
};

struct economic_account_key
{
	critical_operation_id lineage = {};
	economic_account_kind kind = {};
	uint64_t authority_id = 0;
	uint64_t context_id = 0;
};

// Parent indexes are zero for the root or one-based references to earlier
// children. Derivation is checked against the existing critical-operation API.
struct economic_child_link
{
	critical_operation_id operation_id = {};
	uint32_t domain = 0;
	uint64_t discriminator = 0;
	uint16_t parent_index = 0;
	uint16_t relationship = 1; // version 1: component of the parent commit.
};

constexpr size_t ECONOMIC_CHILD_LINK_BYTES = 32;
economic_accounting_error economic_child_links_validate(const critical_operation_id &root,
							std::span<const economic_child_link> links);

using economic_coin_vector = std::array<int64_t, 4>;

// Every audit leg survives; do not net two legs away just because they refer
// to the same account. The vector is the actual denomination change.
struct economic_coin_posting
{
	uint32_t event_index = 0;
	uint16_t account_index = 0;
	uint16_t child_index = 0; // zero is the root; other values are one-based.
	economic_coin_vector delta = {};
	int64_t copper = 0;
};

struct economic_account_effect
{
	economic_account_key key = {};
	economic_coin_vector before = {};
	economic_coin_vector after = {};
	uint64_t before_revision = 0;
	uint64_t after_revision = 0;
};

bool economic_account_kind_valid(economic_account_kind kind);
bool economic_account_is_ordinary(economic_account_kind kind);
bool economic_account_key_valid(const economic_account_key &key);
bool economic_account_key_equal(const economic_account_key &left,
				const economic_account_key &right);
bool economic_account_key_less(const economic_account_key &left, const economic_account_key &right);
// Exact fixed-width little-endian bytes; no struct padding or host endianness.
// Output arguments remain unchanged on failure.
economic_accounting_error
economic_account_key_encode(const economic_account_key &key,
			    std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> *encoded);
economic_accounting_error economic_account_key_decode(std::span<const uint8_t> encoded,
						      economic_account_key *key);
economic_accounting_error economic_coin_value(const economic_coin_vector &vector, int64_t *value);
economic_accounting_error economic_coin_delta(const economic_coin_vector &before,
					      const economic_coin_vector &after,
					      economic_coin_vector *delta);

// Structural conservation/effect check only. A typed domain adapter must still
// authorize reason, system signs/source events and account identities, compare
// these frozen effects to locked authority, and atomically record the result.
// An unchanged ordinary balance can omit postings only if its revision advances.
// This function has no storage, game pointers, publication, or retry side effects.
economic_accounting_error
economic_coin_effects_validate(std::span<const economic_account_effect> effects,
			       std::span<const economic_coin_posting> postings, size_t child_count);

// Borrow the existing custody authority's identities/states. These are bounded
// value snapshots for validation, never another writable ownership catalog.
struct economic_item_position
{
	item_owner_identity owner = {};
	uint64_t root_uid = 0;
	uint64_t parent_uid = 0;
	uint64_t revision = 0;
	item_custody_state state = item_custody_state::absent;
};

struct economic_item_snapshot
{
	uint64_t uid = 0;
	economic_item_position position = {};
};

struct economic_item_event
{
	uint32_t event_index = 0;
	uint16_t child_index = 0;
	uint64_t uid = 0;
	economic_item_position before = {};
	economic_item_position after = {};
};

// Includes affected UIDs and unchanged ancestor witnesses. Adapters must supply
// the complete affected forest from locked authority; the validator cannot prove
// anything about rows an adapter did not read. Absent/destroyed entries remain
// explicit so an omitted UID cannot silently disappear or be recreated.
constexpr size_t ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES = 6000;
bool economic_item_position_equal(const economic_item_position &left,
				  const economic_item_position &right);
economic_accounting_error
economic_item_effects_validate(std::span<const economic_item_snapshot> before,
			       std::span<const economic_item_snapshot> after,
			       std::span<const economic_item_event> events, size_t child_count);

#endif
