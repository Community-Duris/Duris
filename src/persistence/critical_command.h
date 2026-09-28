#ifndef CRITICAL_COMMAND_H
#define CRITICAL_COMMAND_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

constexpr uint32_t CRITICAL_COMMAND_SCHEMA_VERSION = 1;
constexpr uint32_t CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION = 2;
constexpr size_t CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES = 8192;
constexpr size_t CRITICAL_COMMAND_ID_BYTES = 16;
constexpr size_t CRITICAL_COMMAND_ID_HEX_SIZE = 33;
constexpr size_t CRITICAL_COMMAND_MAX_KEYS = 3003;
constexpr size_t CRITICAL_COMMAND_MAX_PAYLOAD_BYTES = 384 * 1024;
constexpr size_t CRITICAL_COMMAND_MAX_ENCODED_BYTES = 512 * 1024;
static_assert(52 + CRITICAL_COMMAND_MAX_KEYS * 40 + CRITICAL_COMMAND_MAX_PAYLOAD_BYTES + 4 +
		      CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES <=
	      CRITICAL_COMMAND_MAX_ENCODED_BYTES);

struct critical_operation_id
{
	std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES> bytes;
};

enum class critical_entity_type : uint8_t
{
	player = 1,
	account,
	item,
	guild,
	locker,
	corpse,
	auction,
	room,
	system,
	artifact,
	zone,
	shopkeeper,
	collector,
	pet,
};

struct critical_entity_key
{
	critical_entity_type type;
	uint64_t id;
};

enum class critical_command_type : uint16_t
{
	test = 1,
	epic,
	account_bank,
	wallet,
	item_transfer,
	locker_transfer,
	auction,
	combat_outcome,
	artifact,
	guild,
	boon_reward,
	zone,
	session_audit,
	boon_shop,
	shop_trade,
	corpse_lifecycle,
	coin_transfer,
	collector,
	// Appended so existing durable command type numbers stay unchanged.
	player_death_restitution,
};

enum class critical_source_site : uint16_t
{
	unknown = 0,
	command,
	combat,
	zone_event,
	login,
	recovery,
	operator_repair,
};

enum class critical_deadline_class : uint8_t
{
	interactive = 1,
	terminal,
	background,
	recovery,
};

// A bounded, aggregate-safe explanation for a terminal optimistic-concurrency
// rejection. Values are bit flags so a command that observed more than one
// mismatched revision can retain that fact without retaining command payloads,
// entity IDs, or amounts in diagnostics.
enum class critical_failure_stage : uint16_t
{
	none = 0,
	coin_source_wallet_revision = 1u << 0,
	coin_source_bank_revision = 1u << 1,
	coin_destination_wallet_revision = 1u << 2,
	coin_destination_bank_revision = 1u << 3,
	coin_source_owner_revision = 1u << 4,
	coin_destination_owner_revision = 1u << 5,
	coin_source_item_revision = 1u << 6,
	coin_destination_item_revision = 1u << 7,
	coin_source_target_parent_revision = 1u << 8,
	coin_destination_target_parent_revision = 1u << 9,
	coin_source_coin_payload_revision = 1u << 10,
	coin_destination_coin_payload_revision = 1u << 11,
	coin_destination_rebase = 1u << 12,
	coin_revision_unknown = 1u << 13,
};

constexpr uint16_t CRITICAL_FAILURE_STAGE_MASK =
	static_cast<uint16_t>(critical_failure_stage::coin_revision_unknown) |
	(static_cast<uint16_t>(critical_failure_stage::coin_revision_unknown) - 1);

inline bool critical_failure_stage_valid(critical_failure_stage stage)
{
	return !(static_cast<uint16_t>(stage) & ~CRITICAL_FAILURE_STAGE_MASK);
}

struct critical_expected_revision
{
	critical_entity_key key;
	uint64_t revision;
};

struct critical_command
{
	uint32_t schema_version;
	critical_operation_id operation_id;
	critical_command_type type;
	uint16_t payload_version;
	critical_source_site source_site;
	critical_deadline_class deadline_class;
	uint64_t accepted_at_usec;
	std::vector<critical_entity_key> keys;
	std::vector<critical_expected_revision> expected_revisions;
	std::vector<uint8_t> payload;
	// Schema 2 wire evidence only until a typed accounting executor is connected.
	std::vector<uint8_t> accounting_intent = {};
};

enum class critical_command_codec_result : uint8_t
{
	ok,
	invalid,
	truncated,
	overflow,
	unsupported_version,
};

bool critical_operation_id_generate(critical_operation_id *operation_id);
bool critical_operation_id_derive(const critical_operation_id &parent, uint32_t domain,
				  uint64_t discriminator, critical_operation_id *operation_id);
bool critical_operation_id_is_zero(const critical_operation_id &operation_id);
bool critical_operation_id_equal(const critical_operation_id &left,
				 const critical_operation_id &right);
bool critical_operation_id_to_hex(const critical_operation_id &operation_id, char *output,
				  size_t output_size);
bool critical_operation_id_from_hex(const char *input, critical_operation_id *operation_id);
bool critical_entity_key_less(const critical_entity_key &left, const critical_entity_key &right);
bool critical_entity_key_equal(const critical_entity_key &left, const critical_entity_key &right);
bool critical_command_normalize(critical_command *command);
// Wire validity is distinct from support by the legacy mutation entrypoints.
bool critical_command_envelope_valid(const critical_command &command);
bool critical_command_legacy_execution_supported(const critical_command &command);
bool critical_command_valid(const critical_command &command);
bool critical_command_equal(const critical_command &left, const critical_command &right);
const char *critical_failure_stage_name(critical_failure_stage stage);
critical_command_codec_result critical_command_encode(const critical_command &command,
						      std::vector<uint8_t> *encoded);
critical_command_codec_result critical_command_decode(const uint8_t *encoded, size_t size,
						      critical_command *command);

#endif
