#ifndef CRITICAL_COMMAND_H
#define CRITICAL_COMMAND_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

constexpr uint32_t CRITICAL_COMMAND_SCHEMA_VERSION = 1;
constexpr size_t CRITICAL_COMMAND_ID_BYTES = 16;
constexpr size_t CRITICAL_COMMAND_ID_HEX_SIZE = 33;
constexpr size_t CRITICAL_COMMAND_MAX_KEYS = 32;
constexpr size_t CRITICAL_COMMAND_MAX_PAYLOAD_BYTES = 256 * 1024;
constexpr size_t CRITICAL_COMMAND_MAX_ENCODED_BYTES = 272 * 1024;

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
bool critical_command_valid(const critical_command &command);
bool critical_command_equal(const critical_command &left, const critical_command &right);
critical_command_codec_result critical_command_encode(const critical_command &command,
						      std::vector<uint8_t> *encoded);
critical_command_codec_result critical_command_decode(const uint8_t *encoded, size_t size,
						      critical_command *command);

#endif
