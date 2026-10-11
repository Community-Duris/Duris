#include "persistence/critical_command.h"
#include "economy/auction_native_command_context.h"
#include "economy/shop_trade_command.h"
#include "item/item_transfer_command.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <new>
#include <openssl/sha.h>
#include <sys/random.h>
#include <utility>

namespace
{
constexpr unsigned char COMMAND_MAGIC[4] = { 'C', 'C', 'M', '1' };

size_t envelope_key_limit(const critical_command &command) noexcept
{
	return critical_command_native_auction_envelope(command) ?
		       CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS :
		       CRITICAL_COMMAND_MAX_KEYS;
}
bool envelope_encoded_size(const critical_command &command, size_t *size) noexcept
{
	if (!size)
		return false;
	size_t bytes = CRITICAL_COMMAND_HEADER_BYTES;
	const auto add = [&](size_t count, size_t width) noexcept
	{
		if (count > (CRITICAL_COMMAND_MAX_ENCODED_BYTES - bytes) / width)
			return false;
		bytes += count * width;
		return true;
	};
	if (!add(command.keys.size(), CRITICAL_COMMAND_ENTITY_KEY_BYTES) ||
	    !add(command.expected_revisions.size(), CRITICAL_COMMAND_EXPECTED_REVISION_BYTES) ||
	    !add(command.payload.size(), 1) ||
	    (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	     (!add(CRITICAL_COMMAND_ACCOUNTING_PREFIX_BYTES, 1) ||
	      !add(command.accounting_intent.size(), 1))))
		return false;
	*size = bytes;
	return true;
}
bool valid_entity_type(critical_entity_type type)
{
	return type >= critical_entity_type::player && type <= critical_entity_type::native_mobile;
}

template <typename T> void append_le(std::vector<uint8_t> &output, T value)
{
	for (size_t index = 0; index < sizeof(T); ++index)
		output.push_back(static_cast<uint8_t>(static_cast<uint64_t>(value) >> (index * 8)));
}

template <typename T> bool read_le(const uint8_t *input, size_t size, size_t *offset, T *value)
{
	if (!offset || !value || *offset > size || sizeof(T) > size - *offset)
		return false;
	uint64_t decoded = 0;
	for (size_t index = 0; index < sizeof(T); ++index)
		decoded |= static_cast<uint64_t>(input[*offset + index]) << (index * 8);
	*offset += sizeof(T);
	*value = static_cast<T>(decoded);
	return true;
}

int hex_value(char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}
} // namespace

// Auction2 preparation/binding is structural only: legacy execution explicitly
// rejects this payload version even when the pure schema1 projection is valid.
bool critical_command_native_auction_envelope(const critical_command &command) noexcept
{
	return command.type == critical_command_type::auction &&
	       command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
	       ((command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		 command.publication_required && !command.accounting_intent.empty()) ||
		(command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
		 !command.publication_required && command.accounting_intent.empty()));
}

bool critical_operation_id_generate(critical_operation_id *operation_id)
{
	if (!operation_id)
		return false;
	size_t offset = 0;
	while (offset < operation_id->bytes.size())
	{
		const ssize_t result = getrandom(operation_id->bytes.data() + offset,
						 operation_id->bytes.size() - offset, 0);
		if (result < 0 && errno == EINTR)
			continue;
		if (result <= 0)
			return false;
		offset += static_cast<size_t>(result);
	}
	return !critical_operation_id_is_zero(*operation_id);
}

bool critical_operation_id_derive(const critical_operation_id &parent, uint32_t domain,
				  uint64_t discriminator, critical_operation_id *operation_id)
{
	if (!operation_id || critical_operation_id_is_zero(parent) || !domain)
		return false;
	std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES + sizeof(domain) + sizeof(discriminator)>
		input = {};
	std::copy(parent.bytes.begin(), parent.bytes.end(), input.begin());
	for (size_t byte = 0; byte < sizeof(domain); ++byte)
		input[CRITICAL_COMMAND_ID_BYTES + byte] =
			static_cast<uint8_t>(domain >> (byte * 8));
	for (size_t byte = 0; byte < sizeof(discriminator); ++byte)
		input[CRITICAL_COMMAND_ID_BYTES + sizeof(domain) + byte] =
			static_cast<uint8_t>(discriminator >> (byte * 8));
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(input.data(), input.size(), digest.data());
	std::copy_n(digest.begin(), operation_id->bytes.size(), operation_id->bytes.begin());
	return !critical_operation_id_is_zero(*operation_id);
}

bool critical_operation_id_is_zero(const critical_operation_id &operation_id)
{
	for (uint8_t value : operation_id.bytes)
		if (value)
			return false;
	return true;
}

bool critical_operation_id_equal(const critical_operation_id &left,
				 const critical_operation_id &right)
{
	return left.bytes == right.bytes;
}

bool critical_operation_id_to_hex(const critical_operation_id &operation_id, char *output,
				  size_t output_size)
{
	if (!output || output_size < CRITICAL_COMMAND_ID_HEX_SIZE)
		return false;
	static constexpr char digits[] = "0123456789abcdef";
	for (size_t index = 0; index < operation_id.bytes.size(); ++index)
	{
		output[index * 2] = digits[operation_id.bytes[index] >> 4];
		output[index * 2 + 1] = digits[operation_id.bytes[index] & 0x0f];
	}
	output[CRITICAL_COMMAND_ID_HEX_SIZE - 1] = '\0';
	return true;
}

bool critical_operation_id_from_hex(const char *input, critical_operation_id *operation_id)
{
	if (!input || !operation_id || strlen(input) != CRITICAL_COMMAND_ID_HEX_SIZE - 1)
		return false;
	for (size_t index = 0; index < operation_id->bytes.size(); ++index)
	{
		const int high = hex_value(input[index * 2]);
		const int low = hex_value(input[index * 2 + 1]);
		if (high < 0 || low < 0)
			return false;
		operation_id->bytes[index] = static_cast<uint8_t>((high << 4) | low);
	}
	return !critical_operation_id_is_zero(*operation_id);
}

bool critical_entity_key_less(const critical_entity_key &left, const critical_entity_key &right)
{
	if (left.type != right.type)
		return left.type < right.type;
	return left.id < right.id;
}

bool critical_entity_key_equal(const critical_entity_key &left, const critical_entity_key &right)
{
	return left.type == right.type && left.id == right.id;
}

bool critical_command_normalize(critical_command *command)
{
	if (!command)
		return false;
	size_t wire_bytes = 0;
	if (command->keys.size() > envelope_key_limit(*command) ||
	    command->expected_revisions.size() > envelope_key_limit(*command) ||
	    command->payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
	    command->accounting_intent.size() > CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES ||
	    !envelope_encoded_size(*command, &wire_bytes))
		return false;
	try
	{
		auto normalized = *command;
		std::sort(normalized.keys.begin(), normalized.keys.end(), critical_entity_key_less);
		if (std::adjacent_find(normalized.keys.begin(), normalized.keys.end(),
				       critical_entity_key_equal) != normalized.keys.end())
			return false;
		std::sort(normalized.expected_revisions.begin(),
			  normalized.expected_revisions.end(),
			  [](const critical_expected_revision &left,
			     const critical_expected_revision &right)
			  { return critical_entity_key_less(left.key, right.key); });
		if (!(critical_command_native_auction_envelope(normalized) ?
			      critical_command_envelope_valid(normalized) :
			      critical_command_valid(normalized)))
			return false;
		*command = std::move(normalized);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool critical_command_legacy_execution_supported(const critical_command &command)
{
	return command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
	       command.accounting_intent.empty() &&
	       std::none_of(command.keys.begin(), command.keys.end(), [](const auto &key)
			    { return key.type == critical_entity_type::native_mobile; }) &&
	       (command.type != critical_command_type::item_transfer ||
		command.payload_version <= ITEM_TRANSFER_PAYLOAD_VERSION) &&
	       (command.type != critical_command_type::shop_trade ||
		command.payload_version <= SHOP_TRADE_PAYLOAD_VERSION) &&
	       (command.type != critical_command_type::auction ||
		command.payload_version <= AUCTION_COMMAND_PAYLOAD_VERSION) &&
	       command.type >= critical_command_type::test &&
	       command.type <= critical_command_type::player_death_restitution;
}

bool critical_command_valid(const critical_command &command)
{
	// Legacy mutation entrypoints stay closed to schema 2. The coordinator
	// separately registers typed accounting admission and atomic owners.
	return critical_command_legacy_execution_supported(command) &&
	       critical_command_envelope_valid(command);
}

bool critical_command_envelope_valid(const critical_command &command)
{
	if ((command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION &&
	     command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION) ||
	    (command.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
	     !command.accounting_intent.empty()) ||
	    (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
	     command.accounting_intent.empty()) ||
	    (command.publication_required &&
	     (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	      (command.type != critical_command_type::account_bank &&
	       command.type != critical_command_type::coin_transfer &&
	       command.type != critical_command_type::item_transfer &&
	       command.type != critical_command_type::collector &&
	       command.type != critical_command_type::native_mobile_birth &&
	       command.type != critical_command_type::zone_reset_item_birth &&
	       !(command.type == critical_command_type::shop_trade &&
		 shop_trade_payload_version_is_accounted(command.payload_version)) &&
	       !(command.type == critical_command_type::auction &&
		 command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)))) ||
	    command.accounting_intent.size() > CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES ||
	    critical_operation_id_is_zero(command.operation_id) || !command.payload_version ||
	    command.type < critical_command_type::test ||
	    command.type > critical_command_type::zone_reset_item_birth ||
	    command.source_site < critical_source_site::command ||
	    command.source_site > critical_source_site::operator_repair ||
	    command.deadline_class < critical_deadline_class::interactive ||
	    command.deadline_class > critical_deadline_class::recovery ||
	    !command.accepted_at_usec || command.keys.empty() ||
	    command.keys.size() > envelope_key_limit(command) ||
	    command.expected_revisions.size() > envelope_key_limit(command) ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	size_t wire_bytes = 0;
	if (!envelope_encoded_size(command, &wire_bytes))
		return false;
	for (size_t index = 0; index < command.keys.size(); ++index)
	{
		if (!valid_entity_type(command.keys[index].type) || !command.keys[index].id ||
		    (command.keys[index].type == critical_entity_type::native_mobile &&
		     command.keys[index].id == UINT64_MAX) ||
		    (index &&
		     !critical_entity_key_less(command.keys[index - 1], command.keys[index])))
			return false;
	}
	for (size_t index = 0; index < command.expected_revisions.size(); ++index)
	{
		const critical_entity_key &key = command.expected_revisions[index].key;
		if (!valid_entity_type(key.type) || !key.id ||
		    (key.type == critical_entity_type::native_mobile && key.id == UINT64_MAX) ||
		    !std::binary_search(command.keys.begin(), command.keys.end(), key,
					critical_entity_key_less) ||
		    (index &&
		     !critical_entity_key_less(command.expected_revisions[index - 1].key, key)))
			return false;
	}
	return true;
}

bool critical_command_equal(const critical_command &left, const critical_command &right)
{
	std::vector<uint8_t> left_encoded;
	std::vector<uint8_t> right_encoded;
	return critical_command_encode(left, &left_encoded) == critical_command_codec_result::ok &&
	       critical_command_encode(right, &right_encoded) ==
		       critical_command_codec_result::ok &&
	       left_encoded == right_encoded;
}

const char *critical_failure_stage_name(critical_failure_stage stage)
{
	switch (stage)
	{
	case critical_failure_stage::none:
		return "none";
	case critical_failure_stage::coin_source_wallet_revision:
		return "coin_source_wallet_revision";
	case critical_failure_stage::coin_source_bank_revision:
		return "coin_source_bank_revision";
	case critical_failure_stage::coin_destination_wallet_revision:
		return "coin_destination_wallet_revision";
	case critical_failure_stage::coin_destination_bank_revision:
		return "coin_destination_bank_revision";
	case critical_failure_stage::coin_source_owner_revision:
		return "coin_source_owner_revision";
	case critical_failure_stage::coin_destination_owner_revision:
		return "coin_destination_owner_revision";
	case critical_failure_stage::coin_source_item_revision:
		return "coin_source_item_revision";
	case critical_failure_stage::coin_destination_item_revision:
		return "coin_destination_item_revision";
	case critical_failure_stage::coin_source_target_parent_revision:
		return "coin_source_target_parent_revision";
	case critical_failure_stage::coin_destination_target_parent_revision:
		return "coin_destination_target_parent_revision";
	case critical_failure_stage::coin_source_coin_payload_revision:
		return "coin_source_coin_payload_revision";
	case critical_failure_stage::coin_destination_coin_payload_revision:
		return "coin_destination_coin_payload_revision";
	case critical_failure_stage::coin_destination_rebase:
		return "coin_destination_rebase";
	case critical_failure_stage::coin_revision_unknown:
		return "coin_revision_unknown";
	}
	return critical_failure_stage_valid(stage) ? "multiple_revision_gates" : "invalid";
}

critical_command_codec_result critical_command_encode(const critical_command &command,
						      std::vector<uint8_t> *encoded)
{
	if (!encoded)
		return critical_command_codec_result::invalid;
	size_t wire_bytes = 0;
	if (!envelope_encoded_size(command, &wire_bytes))
		return critical_command_native_auction_envelope(command) ?
			       critical_command_codec_result::overflow :
			       critical_command_codec_result::invalid;
	if (!critical_command_envelope_valid(command))
		return critical_command_codec_result::invalid;
	std::vector<uint8_t> result;
	try
	{
		result.reserve(wire_bytes);
		result.insert(result.end(), COMMAND_MAGIC, COMMAND_MAGIC + sizeof(COMMAND_MAGIC));
		append_le<uint32_t>(result, command.schema_version);
		result.insert(result.end(), command.operation_id.bytes.begin(),
			      command.operation_id.bytes.end());
		append_le<uint16_t>(result, static_cast<uint16_t>(command.type));
		append_le<uint16_t>(result, command.payload_version);
		append_le<uint16_t>(result, static_cast<uint16_t>(command.source_site));
		result.push_back(static_cast<uint8_t>(command.deadline_class));
		result.push_back(command.publication_required ? 1 : 0);
		append_le<uint64_t>(result, command.accepted_at_usec);
		append_le<uint32_t>(result, static_cast<uint32_t>(command.keys.size()));
		append_le<uint32_t>(result,
				    static_cast<uint32_t>(command.expected_revisions.size()));
		append_le<uint32_t>(result, static_cast<uint32_t>(command.payload.size()));
		for (const critical_entity_key &key : command.keys)
		{
			result.push_back(static_cast<uint8_t>(key.type));
			for (unsigned int pad = 0; pad < 7; ++pad)
				result.push_back(0);
			append_le<uint64_t>(result, key.id);
		}
		for (const critical_expected_revision &revision : command.expected_revisions)
		{
			result.push_back(static_cast<uint8_t>(revision.key.type));
			for (unsigned int pad = 0; pad < 7; ++pad)
				result.push_back(0);
			append_le<uint64_t>(result, revision.key.id);
			append_le<uint64_t>(result, revision.revision);
		}
		result.insert(result.end(), command.payload.begin(), command.payload.end());
		if (command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		{
			append_le<uint32_t>(
				result, static_cast<uint32_t>(command.accounting_intent.size()));
			result.insert(result.end(), command.accounting_intent.begin(),
				      command.accounting_intent.end());
		}
	}
	catch (const std::bad_alloc &)
	{
		return critical_command_codec_result::overflow;
	}
	if (result.size() != wire_bytes || result.size() > CRITICAL_COMMAND_MAX_ENCODED_BYTES)
		return critical_command_codec_result::overflow;
	*encoded = std::move(result);
	return critical_command_codec_result::ok;
}

critical_command_codec_result critical_command_encoder_working_bytes(
	const critical_command &command, size_t *working_bytes) noexcept
{
	using result = critical_command_codec_result;
	if (!working_bytes)
		return result::invalid;
	size_t wire_bytes = 0;
	if (!envelope_encoded_size(command, &wire_bytes))
		return critical_command_native_auction_envelope(command) ? result::overflow :
								 result::invalid;
	if (!critical_command_envelope_valid(command))
		return result::invalid;
#if !defined(__GLIBCXX__) || !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || \
	!defined(_GLIBCXX_USE_CXX11_ABI) || !_GLIBCXX_USE_CXX11_ABI
	return result::unsupported_version;
#else
	// Original result.reserve(wire_bytes) precedes every append. Validated exact
	// wire size prevents all later growth; no previous buffer belongs to result.
	if (wire_bytes > SIZE_MAX - sizeof(std::vector<uint8_t>))
		return result::overflow;
	*working_bytes = sizeof(std::vector<uint8_t>) + wire_bytes;
	return result::ok;
#endif
}

critical_command_codec_result critical_command_encode_bounded(
	const critical_command &command, std::vector<uint8_t> *encoded,
	bool (*reserve_scratch_peak)(size_t, void *) noexcept, void *context,
	size_t outer_live_scratch) noexcept
{
	using result = critical_command_codec_result;
	if (!encoded)
		return result::invalid;
	size_t working_bytes = 0;
	const auto status = critical_command_encoder_working_bytes(command, &working_bytes);
	if (status != result::ok)
		return status;
	if (!reserve_scratch_peak || working_bytes > SIZE_MAX - outer_live_scratch ||
	    !reserve_scratch_peak(outer_live_scratch + working_bytes, context))
		return result::overflow;
	return critical_command_encode(command, encoded);
}

critical_command_codec_result critical_command_decode(const uint8_t *encoded, size_t size,
						      critical_command *command)
{
	if (!encoded || !command || size < CRITICAL_COMMAND_HEADER_BYTES)
		return critical_command_codec_result::truncated;
	if (size > CRITICAL_COMMAND_MAX_ENCODED_BYTES || memcmp(encoded, COMMAND_MAGIC, 4) != 0)
		return critical_command_codec_result::invalid;
	size_t offset = 4;
	critical_command decoded = {};
	uint16_t type = 0, source = 0;
	uint32_t key_count = 0, revision_count = 0, payload_size = 0;
	if (!read_le(encoded, size, &offset, &decoded.schema_version))
		return critical_command_codec_result::truncated;
	if (decoded.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION &&
	    decoded.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		return critical_command_codec_result::unsupported_version;
	memcpy(decoded.operation_id.bytes.data(), encoded + offset,
	       decoded.operation_id.bytes.size());
	offset += decoded.operation_id.bytes.size();
	if (!read_le(encoded, size, &offset, &type) ||
	    !read_le(encoded, size, &offset, &decoded.payload_version) ||
	    !read_le(encoded, size, &offset, &source) || offset + 2 > size)
		return critical_command_codec_result::truncated;
	decoded.type = static_cast<critical_command_type>(type);
	decoded.source_site = static_cast<critical_source_site>(source);
	decoded.deadline_class = static_cast<critical_deadline_class>(encoded[offset]);
	if (encoded[offset + 1] > 1 ||
	    (encoded[offset + 1] &&
	     decoded.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION))
		return critical_command_codec_result::invalid;
	decoded.publication_required = encoded[offset + 1] == 1;
	offset += 2;
	if (!read_le(encoded, size, &offset, &decoded.accepted_at_usec) ||
	    !read_le(encoded, size, &offset, &key_count) ||
	    !read_le(encoded, size, &offset, &revision_count) ||
	    !read_le(encoded, size, &offset, &payload_size))
		return critical_command_codec_result::truncated;
	// Intent bytes are decoded below. This is only the allocation hard gate;
	// final envelope validation rechecks exact schema/publication/intent shape.
	const bool native_auction_header =
		decoded.type == critical_command_type::auction &&
		decoded.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
		((decoded.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		  decoded.publication_required) ||
		 (decoded.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
		  !decoded.publication_required));
	const size_t key_limit = native_auction_header ? CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS :
							 CRITICAL_COMMAND_MAX_KEYS;
	if (!key_count || key_count > CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS ||
	    revision_count > CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS || key_count > key_limit ||
	    revision_count > key_limit || payload_size > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return critical_command_codec_result::overflow;
	uint64_t required =
		static_cast<uint64_t>(key_count) * CRITICAL_COMMAND_ENTITY_KEY_BYTES +
		static_cast<uint64_t>(revision_count) * CRITICAL_COMMAND_EXPECTED_REVISION_BYTES +
		payload_size;
	if (decoded.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
	{
		if (required > size - offset ||
		    size - offset - required < CRITICAL_COMMAND_ACCOUNTING_PREFIX_BYTES)
			return critical_command_codec_result::truncated;
		size_t intent_offset = offset + static_cast<size_t>(required);
		uint32_t intent_size = 0;
		if (!read_le(encoded, size, &intent_offset, &intent_size))
			return critical_command_codec_result::truncated;
		if (!intent_size || intent_size > CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES)
			return critical_command_codec_result::overflow;
		required += CRITICAL_COMMAND_ACCOUNTING_PREFIX_BYTES + intent_size;
	}
	if (required != size - offset)
		return required > size - offset ? critical_command_codec_result::truncated :
						  critical_command_codec_result::invalid;
	try
	{
		for (uint32_t index = 0; index < key_count; ++index)
		{
			critical_entity_key key = {
				static_cast<critical_entity_type>(encoded[offset]), 0
			};
			for (size_t pad = 1; pad < 8; ++pad)
				if (encoded[offset + pad] != 0)
					return critical_command_codec_result::invalid;
			offset += 8;
			if (!read_le(encoded, size, &offset, &key.id))
				return critical_command_codec_result::truncated;
			decoded.keys.push_back(key);
		}
		for (uint32_t index = 0; index < revision_count; ++index)
		{
			critical_expected_revision revision = {
				.key = { static_cast<critical_entity_type>(encoded[offset]), 0 },
				.revision = 0
			};
			for (size_t pad = 1; pad < 8; ++pad)
				if (encoded[offset + pad] != 0)
					return critical_command_codec_result::invalid;
			offset += 8;
			if (!read_le(encoded, size, &offset, &revision.key.id) ||
			    !read_le(encoded, size, &offset, &revision.revision))
				return critical_command_codec_result::truncated;
			decoded.expected_revisions.push_back(revision);
		}
		decoded.payload.assign(encoded + offset, encoded + offset + payload_size);
		if (decoded.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			decoded.accounting_intent.assign(
				encoded + offset + payload_size +
					CRITICAL_COMMAND_ACCOUNTING_PREFIX_BYTES,
				encoded + size);
	}
	catch (const std::bad_alloc &)
	{
		return critical_command_codec_result::overflow;
	}
	if (!critical_command_envelope_valid(decoded))
		return critical_command_codec_result::invalid;
	*command = std::move(decoded);
	return critical_command_codec_result::ok;
}

namespace
{
[[maybe_unused]] bool critical_decode_add(size_t &bytes, size_t amount) noexcept
{
	if (amount > SIZE_MAX - bytes)
		return false;
	bytes += amount;
	return true;
}
[[maybe_unused]] bool critical_decode_heap(const critical_command &command, size_t *output) noexcept
{
	size_t bytes = command.payload.capacity();
	if (!critical_decode_add(bytes, command.accounting_intent.capacity()) ||
	    command.keys.capacity() > SIZE_MAX / sizeof(critical_entity_key) ||
	    !critical_decode_add(bytes, command.keys.capacity() * sizeof(critical_entity_key)) ||
	    command.expected_revisions.capacity() > SIZE_MAX / sizeof(critical_expected_revision) ||
	    !critical_decode_add(bytes, command.expected_revisions.capacity() *
						sizeof(critical_expected_revision)))
		return false;
	*output = bytes;
	return true;
}
[[maybe_unused]] bool critical_decode_admit(const critical_command &command, size_t extra,
					    bool (*reserve)(size_t, void *) noexcept, void *context,
					    size_t live) noexcept
{
	size_t heap = 0;
	return critical_decode_heap(command, &heap) && critical_decode_add(live, heap) &&
	       critical_decode_add(live, extra) && reserve(live, context);
}
} // namespace

critical_command_codec_result
critical_command_decode_bounded(const uint8_t *encoded, size_t size, critical_command *command,
				bool (*reserve)(size_t, void *) noexcept, void *context,
				size_t outer_live, size_t *retained_command_heap_bytes) noexcept
{
	if (!encoded || !command || size < CRITICAL_COMMAND_HEADER_BYTES)
		return critical_command_codec_result::truncated;
	if (size > CRITICAL_COMMAND_MAX_ENCODED_BYTES || memcmp(encoded, COMMAND_MAGIC, 4) != 0)
		return critical_command_codec_result::invalid;
	if (!reserve)
		return critical_command_codec_result::overflow;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)context;
	(void)outer_live;
	(void)retained_command_heap_bytes;
	return critical_command_codec_result::unsupported_version;
#else
	size_t live = outer_live;
	if (!critical_decode_add(live, sizeof(critical_command)) || !reserve(live, context))
		return critical_command_codec_result::overflow;
	size_t offset = 4;
	critical_command decoded = {};
	uint16_t type = 0, source = 0;
	uint32_t key_count = 0, revision_count = 0, payload_size = 0;
	if (!read_le(encoded, size, &offset, &decoded.schema_version))
		return critical_command_codec_result::truncated;
	if (decoded.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION &&
	    decoded.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		return critical_command_codec_result::unsupported_version;
	memcpy(decoded.operation_id.bytes.data(), encoded + offset,
	       decoded.operation_id.bytes.size());
	offset += decoded.operation_id.bytes.size();
	if (!read_le(encoded, size, &offset, &type) ||
	    !read_le(encoded, size, &offset, &decoded.payload_version) ||
	    !read_le(encoded, size, &offset, &source) || offset + 2 > size)
		return critical_command_codec_result::truncated;
	decoded.type = static_cast<critical_command_type>(type);
	decoded.source_site = static_cast<critical_source_site>(source);
	decoded.deadline_class = static_cast<critical_deadline_class>(encoded[offset]);
	if (encoded[offset + 1] > 1 ||
	    (encoded[offset + 1] &&
	     decoded.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION))
		return critical_command_codec_result::invalid;
	decoded.publication_required = encoded[offset + 1] == 1;
	offset += 2;
	if (!read_le(encoded, size, &offset, &decoded.accepted_at_usec) ||
	    !read_le(encoded, size, &offset, &key_count) ||
	    !read_le(encoded, size, &offset, &revision_count) ||
	    !read_le(encoded, size, &offset, &payload_size))
		return critical_command_codec_result::truncated;
	// Intent bytes are decoded below. This is only the allocation hard gate;
	// final envelope validation rechecks exact schema/publication/intent shape.
	const bool native_auction_header =
		decoded.type == critical_command_type::auction &&
		decoded.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
		((decoded.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		  decoded.publication_required) ||
		 (decoded.schema_version == CRITICAL_COMMAND_SCHEMA_VERSION &&
		  !decoded.publication_required));
	const size_t key_limit = native_auction_header ? CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS :
							 CRITICAL_COMMAND_MAX_KEYS;
	if (!key_count || key_count > CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS ||
	    revision_count > CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS || key_count > key_limit ||
	    revision_count > key_limit || payload_size > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return critical_command_codec_result::overflow;
	uint64_t required =
		static_cast<uint64_t>(key_count) * CRITICAL_COMMAND_ENTITY_KEY_BYTES +
		static_cast<uint64_t>(revision_count) * CRITICAL_COMMAND_EXPECTED_REVISION_BYTES +
		payload_size;
	if (decoded.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
	{
		if (required > size - offset ||
		    size - offset - required < CRITICAL_COMMAND_ACCOUNTING_PREFIX_BYTES)
			return critical_command_codec_result::truncated;
		size_t intent_offset = offset + static_cast<size_t>(required);
		uint32_t intent_size = 0;
		if (!read_le(encoded, size, &intent_offset, &intent_size))
			return critical_command_codec_result::truncated;
		if (!intent_size || intent_size > CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES)
			return critical_command_codec_result::overflow;
		required += CRITICAL_COMMAND_ACCOUNTING_PREFIX_BYTES + intent_size;
	}
	if (required != size - offset)
		return required > size - offset ? critical_command_codec_result::truncated :
						  critical_command_codec_result::invalid;
	try
	{
		for (uint32_t index = 0; index < key_count; ++index)
		{
			if (!critical_decode_admit(decoded, sizeof(critical_entity_key), reserve,
						   context, live))
				return critical_command_codec_result::overflow;
			critical_entity_key key = {
				static_cast<critical_entity_type>(encoded[offset]), 0
			};
			for (size_t pad = 1; pad < 8; ++pad)
				if (encoded[offset + pad] != 0)
					return critical_command_codec_result::invalid;
			offset += 8;
			if (!read_le(encoded, size, &offset, &key.id))
				return critical_command_codec_result::truncated;
			if (decoded.keys.size() == decoded.keys.capacity())
			{
				size_t request = decoded.keys.size();
				size_t extra = sizeof(key);
				if (!critical_decode_add(request, std::max(decoded.keys.size(),
									   size_t{ 1 })) ||
				    request > SIZE_MAX / sizeof(critical_entity_key) ||
				    !critical_decode_add(extra,
							 request * sizeof(critical_entity_key)) ||
				    !critical_decode_admit(decoded, extra, reserve, context, live))
					return critical_command_codec_result::overflow;
			}
			decoded.keys.push_back(key);
		}
		for (uint32_t index = 0; index < revision_count; ++index)
		{
			if (!critical_decode_admit(decoded, sizeof(critical_expected_revision),
						   reserve, context, live))
				return critical_command_codec_result::overflow;
			critical_expected_revision revision = {
				.key = { static_cast<critical_entity_type>(encoded[offset]), 0 },
				.revision = 0
			};
			for (size_t pad = 1; pad < 8; ++pad)
				if (encoded[offset + pad] != 0)
					return critical_command_codec_result::invalid;
			offset += 8;
			if (!read_le(encoded, size, &offset, &revision.key.id) ||
			    !read_le(encoded, size, &offset, &revision.revision))
				return critical_command_codec_result::truncated;
			if (decoded.expected_revisions.size() ==
			    decoded.expected_revisions.capacity())
			{
				size_t request = decoded.expected_revisions.size();
				size_t extra = sizeof(revision);
				if (!critical_decode_add(request,
							 std::max(decoded.expected_revisions.size(),
								  size_t{ 1 })) ||
				    request > SIZE_MAX / sizeof(critical_expected_revision) ||
				    !critical_decode_add(
					    extra, request * sizeof(critical_expected_revision)) ||
				    !critical_decode_admit(decoded, extra, reserve, context, live))
					return critical_command_codec_result::overflow;
			}
			decoded.expected_revisions.push_back(revision);
		}
		if (!critical_decode_admit(decoded, payload_size, reserve, context, live))
			return critical_command_codec_result::overflow;
		decoded.payload.assign(encoded + offset, encoded + offset + payload_size);
		if (decoded.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		{
			const size_t intent_size = size - offset - payload_size -
						   CRITICAL_COMMAND_ACCOUNTING_PREFIX_BYTES;
			if (!critical_decode_admit(decoded, intent_size, reserve, context, live))
				return critical_command_codec_result::overflow;
			decoded.accounting_intent.assign(
				encoded + offset + payload_size +
					CRITICAL_COMMAND_ACCOUNTING_PREFIX_BYTES,
				encoded + size);
		}
	}
	catch (const std::bad_alloc &)
	{
		return critical_command_codec_result::overflow;
	}
	if (!critical_command_envelope_valid(decoded))
		return critical_command_codec_result::invalid;
	size_t retained = 0;
	if (!critical_decode_heap(decoded, &retained))
		return critical_command_codec_result::overflow;
	*command = std::move(decoded);
	if (retained_command_heap_bytes)
		*retained_command_heap_bytes = retained;
	return critical_command_codec_result::ok;
#endif
}

namespace
{
constexpr size_t critical_derive_sha_assembly_frames =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t critical_derive_sha_c_small_frames =
	16 * sizeof(unsigned int) + 12 * sizeof(unsigned int) + sizeof(unsigned int) + sizeof(int) +
	sizeof(const uint8_t *);
constexpr size_t critical_derive_sha_c_normal_frames = 16 * sizeof(unsigned int) +
						       11 * sizeof(unsigned int) + 2 * sizeof(int) +
						       2 * sizeof(void *);
constexpr size_t critical_derive_sha_init_frames = sizeof(void *) + sizeof(int);
constexpr size_t critical_derive_sha_update_frames = 2 * sizeof(void *) + sizeof(size_t) +
						     2 * sizeof(void *) + sizeof(unsigned int) +
						     sizeof(size_t) + sizeof(int);
constexpr size_t critical_derive_sha_final_frames = 3 * sizeof(void *) + sizeof(size_t) +
						    sizeof(unsigned long) + sizeof(unsigned int) +
						    sizeof(int);
[[maybe_unused]] constexpr size_t critical_derive_sha_frames =
	std::max(critical_derive_sha_assembly_frames,
		 std::max(critical_derive_sha_c_small_frames,
			  critical_derive_sha_c_normal_frames)) +
	std::max(critical_derive_sha_init_frames,
		 std::max(critical_derive_sha_update_frames, critical_derive_sha_final_frames));
// Pinned GCC13 raw-pointer std::copy and copy_n closures. The copy_n
// public/size-to-integer/__copy_n/category carriers coexist with copy.
// Sum all source declarations; no native/emitted-stack measurement claim.
[[maybe_unused]] constexpr size_t critical_derive_copy_array_frames =
	// copy first/last/result/return; __miter_base three calls and return;
	// __copy_move_a/a1/a2 pointer arguments/results; __niter_base/wrap.
	4 * sizeof(void *) + 3 * (2 * sizeof(void *)) + 3 * (4 * sizeof(void *)) +
	3 * (2 * sizeof(void *)) + 3 * sizeof(void *) +
	// Trivial __copy_m first/last/result/_Num/result and real memmove.
	4 * sizeof(void *) + sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(bool) +
	// copy_n first/n/result/__n2/return, size-to-integer param/result,
	// __copy_n first/n/result/tag/return and category reference/tag.
	3 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(size_t) + 3 * sizeof(void *) +
	sizeof(size_t) + sizeof(std::random_access_iterator_tag) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) +
	// Genuine array begin/end/data this/result plus size and subscripting.
	8 * (2 * sizeof(void *)) + 2 * (sizeof(void *) + sizeof(size_t)) +
	2 * (2 * sizeof(void *) + sizeof(size_t));
}

bool critical_operation_id_derive_bounded(const critical_operation_id &parent, uint32_t domain,
					  uint64_t discriminator,
					  critical_operation_id *operation_id,
					  bool (*reserve)(size_t, void *) noexcept, void *context,
					  size_t outer_live) noexcept
{
	if (!operation_id || !reserve)
		return false;
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	!defined(_GLIBCXX_DEBUG) && defined(OPENSSL_VERSION_MAJOR) &&                             \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return false;
	constexpr size_t frames =
		// Actual function parameter carriers, local live and returned bool;
		// two byte loop variables, parent/result zero predicate reference,
		// array-range endpoints/value and result; add/reserve call scopes.
		4 * sizeof(void *) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(size_t) +
		sizeof(bool) + sizeof(size_t) + 2 * sizeof(size_t) +
		2 * (sizeof(void *) + 2 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool)) +
		2 * sizeof(size_t) + sizeof(void *) + sizeof(bool) + sizeof(size_t) +
		sizeof(void *) + sizeof(bool);
	size_t live = outer_live;
	if (!critical_decode_add(live, frames) || !reserve(live, context))
		return false;
	if (critical_operation_id_is_zero(parent) || !domain)
		return false;
	if (!critical_decode_add(
		    live, sizeof(std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES + sizeof(domain) +
							     sizeof(discriminator)>) +
				  sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>) +
				  sizeof(SHA256_CTX) + critical_derive_copy_array_frames +
				  critical_derive_sha_frames) ||
	    !reserve(live, context))
		return false;
	std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES + sizeof(domain) + sizeof(discriminator)>
		input = {};
	std::copy(parent.bytes.begin(), parent.bytes.end(), input.begin());
	for (size_t byte = 0; byte < sizeof(domain); ++byte)
		input[CRITICAL_COMMAND_ID_BYTES + byte] =
			static_cast<uint8_t>(domain >> (byte * 8));
	for (size_t byte = 0; byte < sizeof(discriminator); ++byte)
		input[CRITICAL_COMMAND_ID_BYTES + sizeof(domain) + byte] =
			static_cast<uint8_t>(discriminator >> (byte * 8));
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256_CTX digest_context;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	if (SHA256_Init(&digest_context) != 1 ||
	    SHA256_Update(&digest_context, input.data(), input.size()) != 1 ||
	    SHA256_Final(digest.data(), &digest_context) != 1)
		return false;
#pragma GCC diagnostic pop
	std::copy_n(digest.begin(), operation_id->bytes.size(), operation_id->bytes.begin());
	return !critical_operation_id_is_zero(*operation_id);
#else
	(void)parent;
	(void)domain;
	(void)discriminator;
	(void)context;
	(void)outer_live;
	return false;
#endif
}

#include <type_traits>
namespace
{
constexpr size_t critical_normalize_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t critical_normalize_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t critical_normalize_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t critical_normalize_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t critical_normalize_vector_frames =
	critical_normalize_allocator_frames + critical_normalize_copy_frames +
	critical_normalize_relocate_frames + critical_normalize_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t critical_normalize_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + critical_normalize_allocator_frames;
constexpr size_t critical_normalize_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<int32_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + critical_normalize_vector_frames;
template <typename T, typename Comparator> constexpr size_t critical_normalize_sort_leaf_frames()
{
	// Same real GCC13 sort/partition/insertion/heap/copy/adjacent call scopes
	// as UID sorting. Values and comparator carriers use their genuine types.
	// Original key less/equal this-free argument/result scopes and revision
	// lambda this/left/right/result plus its nested key less call.
	return 3 * (2 * sizeof(void *) + sizeof(bool)) + 3 * sizeof(void *) + sizeof(bool) +
	       18 * sizeof(void *) + 7 * sizeof(Comparator) + sizeof(T) + 16 * sizeof(void *) +
	       6 * sizeof(Comparator) + 2 * sizeof(T) + 23 * sizeof(void *) +
	       11 * sizeof(std::ptrdiff_t) + 7 * sizeof(Comparator) + 4 * sizeof(T) +
	       8 * sizeof(void *) + 5 * sizeof(Comparator) + 4 * sizeof(bool) +
	       5 * (4 * sizeof(void *)) + 2 * (2 * sizeof(void *)) + 3 * (2 * sizeof(void *)) +
	       2 * sizeof(void *) + sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	       sizeof(size_t) + sizeof(std::ptrdiff_t) + 9 * sizeof(void *) +
	       2 * sizeof(Comparator) + sizeof(bool);
}
// Entire original allocation-free private/public envelope/legacy predicate
// source closure. Calls keep original laws; no duplicate authority predicate.
constexpr size_t critical_normalize_valid_frames =
	// native-auction, key-limit, valid-type and original schema/legacy wrapper
	// parameters/results, actual accepted payload predicate version/result.
	5 * (sizeof(void *) + sizeof(bool)) + sizeof(void *) + sizeof(size_t) +
	sizeof(critical_entity_type) + sizeof(bool) + sizeof(uint16_t) + sizeof(bool) +
	// envelope_encoded_size command/size refs, bytes, add closure refs,
	// count/width/this/returned bool, lambda captures bytes by reference.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(void *) + 2 * sizeof(size_t) +
	sizeof(bool) +
	// Original envelope command/wire_bytes/key+revision indices/key ref/bool,
	// zero predicate reference/range begin/end/current byte/return.
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + 3 * sizeof(void *) +
	sizeof(uint8_t) + sizeof(bool) +
	// legacy none_of/find_if/__find_if wrapper and RA branch trip_count,
	// real stateless lambda and _Iter_pred/constructor/call carriers.
	4 * (3 * sizeof(void *) + sizeof(char) + sizeof(bool)) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(char) + 4 * sizeof(void *) +
	2 * sizeof(char) + sizeof(bool) +
	// binary_search(first,last,val,comp,i,result); __lower_bound
	// first/last/val/comp, len/half/middle/return; real iter_comp_val
	// conversion temporary and call this/iterator/value/result.
	6 * sizeof(void *) + sizeof(bool) + 5 * sizeof(void *) + 2 * sizeof(std::ptrdiff_t) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(bool) +
	// distance/__distance + advance/__advance/category and iterator
	// subtraction/base/deref/++/+=/comparison/ctor carriers (no recursion).
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::ptrdiff_t) + sizeof(void *) + sizeof(std::random_access_iterator_tag) +
	sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(std::random_access_iterator_tag) +
	9 * (2 * sizeof(void *)) + 2 * sizeof(std::ptrdiff_t) + 2 * sizeof(bool) +
	// Real key less/equal parameters/results, vector query/subscript,
	// inline empty/size/begin/end and shop accounted version predicate.
	2 * (2 * sizeof(void *) + sizeof(bool)) + 10 * (sizeof(void *) + sizeof(size_t));
constexpr size_t critical_normalize_command_copy_frames =
	// Actual critical_command generated copy this/source and four vector
	// copy constructors, including exact source.size capacity requests.
	2 * sizeof(void *) + 4 * critical_normalize_vector_constructor_frames +
	// Scalar/fixed operation array member copy construction carries refs.
	2 * sizeof(void *) +
	// Copy failure rollback destroys genuinely constructed earlier members.
	4 * (sizeof(void *) + critical_normalize_allocator_frames);
constexpr size_t critical_normalize_move_frames_total =
	// Genuine critical_command generated move assignment + four standard
	// equal-allocator vector moves, destination old heap destruction, and
	// normalized's eventual four moved-from destructors. No heap request.
	2 * sizeof(void *) + 4 * critical_normalize_move_frames + sizeof(void *) +
	4 * (sizeof(void *) + critical_normalize_allocator_frames);
constexpr size_t critical_normalize_observation_frames =
	// prefix/result/extra/total/heap, peak and checked-add argument carriers,
	// current four-capacity scan, typed fresh-copy size scan and real queries.
	12 * sizeof(void *) + 10 * sizeof(size_t) + 6 * sizeof(bool) +
	8 * (sizeof(void *) + sizeof(size_t));
struct critical_normalize_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *normalized = nullptr;
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = outer, heap = 0;
		if (!critical_decode_add(total, sizeof(*this)) ||
		    !critical_decode_add(total, frames) ||
		    !critical_decode_add(total, critical_normalize_observation_frames))
			return false;
		if (normalized && (!critical_decode_add(total, sizeof(*normalized)) ||
				   !critical_decode_heap(*normalized, &heap) ||
				   !critical_decode_add(total, heap)))
			return false;
		return critical_decode_add(total, extra) && reserve && reserve(total, context);
	}
	template <typename T, typename Comparator> bool sort_frame(size_t count) const noexcept
	{
		size_t levels = 0, remaining = count,
		       request = critical_normalize_sort_leaf_frames<T, Comparator>();
		while (remaining > 1)
		{
			remaining >>= 1;
			++levels;
		}
		constexpr size_t recursion =
			3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(Comparator);
		if (2 * levels + 1 > SIZE_MAX / recursion ||
		    !critical_decode_add(request, (2 * levels + 1) * recursion) ||
		    !critical_decode_add(request,
					 sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool)))
			return false;
		return peak(request);
	}
};
}
bool critical_command_current_heap_bytes(const critical_command &command, size_t *bytes) noexcept
{
	if (!bytes)
		return false;
	return critical_decode_heap(command, bytes);
}
bool critical_command_fresh_copy_request_bytes(const critical_command &command,
					       size_t *bytes) noexcept
{
	if (!bytes)
		return false;
	size_t total = 0;
	if (command.keys.size() > SIZE_MAX / sizeof(critical_entity_key) ||
	    command.expected_revisions.size() > SIZE_MAX / sizeof(critical_expected_revision) ||
	    !critical_decode_add(total, command.keys.size() * sizeof(critical_entity_key)) ||
	    !critical_decode_add(total, command.expected_revisions.size() *
						sizeof(critical_expected_revision)) ||
	    !critical_decode_add(total, command.payload.size()) ||
	    !critical_decode_add(total, command.accounting_intent.size()))
		return false;
	*bytes = total;
	return true;
}
size_t critical_command_copy_frame_bytes() noexcept
{
	return critical_normalize_command_copy_frames + critical_normalize_move_frames_total +
	       critical_normalize_observation_frames + sizeof(bool) + sizeof(size_t);
}
size_t critical_command_valid_frame_bytes() noexcept
{
	return critical_normalize_valid_frames + sizeof(size_t);
}
namespace
{
bool critical_normalize_owned(critical_command *command, critical_normalize_budget &budget)
{
	if (!command)
		return false;
	size_t wire_bytes = 0;
	if (command->keys.size() > envelope_key_limit(*command) ||
	    command->expected_revisions.size() > envelope_key_limit(*command) ||
	    command->payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
	    command->accounting_intent.size() > CRITICAL_COMMAND_MAX_ACCOUNTING_INTENT_BYTES ||
	    !envelope_encoded_size(*command, &wire_bytes))
		return false;
	try
	{
		size_t admission_request = 0;
		if (!critical_command_fresh_copy_request_bytes(*command, &admission_request) ||
		    !critical_decode_add(admission_request,
					 sizeof(critical_command) +
						 critical_normalize_command_copy_frames) ||
		    !budget.peak(admission_request))
			return false;
		auto normalized = *command;
		budget.normalized = &normalized;
		if (!budget.sort_frame<critical_entity_key, decltype(&critical_entity_key_less)>(
			    normalized.keys.size()))
			return false;
		std::sort(normalized.keys.begin(), normalized.keys.end(), critical_entity_key_less);
		if (!budget.peak(critical_normalize_sort_leaf_frames<
				 critical_entity_key, decltype(&critical_entity_key_equal)>()))
			return false;
		if (std::adjacent_find(normalized.keys.begin(), normalized.keys.end(),
				       critical_entity_key_equal) != normalized.keys.end())
			return false;
		// Genuine original stateless revision lambda has one-byte closure;
		// actual comparator expression below is unchanged.
		if (!budget.sort_frame<critical_expected_revision, char>(
			    normalized.expected_revisions.size()))
			return false;
		std::sort(normalized.expected_revisions.begin(),
			  normalized.expected_revisions.end(),
			  [](const critical_expected_revision &left,
			     const critical_expected_revision &right)
			  { return critical_entity_key_less(left.key, right.key); });
		if (!budget.peak(critical_normalize_valid_frames +
				 critical_normalize_move_frames_total))
			return false;
		if (!(critical_command_native_auction_envelope(normalized) ?
			      critical_command_envelope_valid(normalized) :
			      critical_command_valid(normalized)))
			return false;
		*command = std::move(normalized);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
} // namespace
bool critical_command_normalize_bounded(critical_command *command,
					bool (*reserve)(size_t, void *) noexcept, void *context,
					size_t outer_live) noexcept
{
	if (!command || !reserve)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	constexpr size_t frames =
		// Public and owned function arguments/return, genuine wire_bytes,
		// admission_request local, catch bad_alloc reference and helpers.
		6 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool) + sizeof(size_t) +
		sizeof(size_t) + sizeof(void *);
	critical_normalize_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak(critical_normalize_valid_frames))
		return false;
	static_assert(std::is_nothrow_move_assignable_v<critical_command>);
	return critical_normalize_owned(command, budget);
#else
	(void)context;
	(void)outer_live;
	return false;
#endif
}

// Pure source profile for the unchanged allocation-free CURRENT getter.
// The existing observation subtotal explicitly contains the current four-vector
// capacity scan and checked arithmetic, plus the larger fresh-copy size scan.
// Keep that authenticated conservative subtotal; no copy-constructor, allocation,
// storage query, command baseline or retained byte count is used as an allowance.
bool critical_command_current_heap_observer_frame_bytes(size_t *output) noexcept
{
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&             \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                      \
	__cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) && \
	!defined(_GLIBCXX_PARALLEL)
	if (!output)
		return false;
	// Real public CURRENT command/output pointer formals and bool return
	// surround critical_decode_heap. The named observation subtotal dominates
	// its size_t local, four capacity invocations and sequential checked adds.
	*output = 2 * sizeof(void *) + sizeof(bool) + critical_normalize_observation_frames;
	return true;
#else
	(void)output;
	return false;
#endif
}

namespace
{
#if defined(__linux__) && defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) &&               \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                         \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                      \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                   \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
constexpr size_t critical_startup_codec_encode_callers =
	// encode_bounded: command/output/reserve/context; outer/working; status/result.
	4 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(critical_command_codec_result) +
	// encoder_working_bytes and unchanged encode: command/output/wire/result each.
	2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(critical_command_codec_result));
constexpr size_t critical_startup_codec_encode_loops =
	// Both actual range loops: hidden range reference, begin/end const iterators,
	// current key/revision reference and original unsigned pad. Sequential loops
	// are summed conservatively, using each actual owning iterator type.
	2 * sizeof(void *) + 2 * sizeof(std::vector<critical_entity_key>::const_iterator) +
	sizeof(unsigned int) + 2 * sizeof(void *) +
	2 * sizeof(std::vector<critical_expected_revision>::const_iterator) + sizeof(unsigned int);
constexpr size_t critical_startup_codec_append_read =
	// Largest append_le<uint64_t>: output reference/value/index and converted
	// uint8_t argument. Smaller actual uint16/uint32 instances are dominated.
	sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(uint8_t) +
	// read_le: input/offset/value, size/index, decoded uint64 and bool return.
	3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(uint64_t) + sizeof(bool) +
	// Operation-id array begin/end/data/size forwarding source carriers.
	4 * (2 * sizeof(void *)) + 2 * (sizeof(void *) + sizeof(size_t));
constexpr size_t critical_startup_codec_decode_callers =
	// decode_bounded: input/output/reserve/context/retained-output; size/outer;
	// live/offset/key_limit/retained; type/source; key/revision/payload counts;
	// actual native_auction_header/required and returned codec result.
	5 * sizeof(void *) + 6 * sizeof(size_t) + 2 * sizeof(uint16_t) + 3 * sizeof(uint32_t) +
	sizeof(bool) + sizeof(uint64_t) + sizeof(critical_command_codec_result) +
	// Schema-two preflight intent_offset/intent_size; the later intent_size;
	// key and revision loops index/pad/request/extra. These lexical alternatives
	// are summed conservatively, not multiplied by key/revision element count.
	2 * sizeof(size_t) + sizeof(uint32_t) + 2 * (sizeof(uint32_t) + 3 * sizeof(size_t)) +
	// std::max's actual two size_t temporary arguments on the growth branch.
	2 * sizeof(size_t);
constexpr size_t critical_startup_codec_decode_observation =
	// decode_admit(command,extra,reserve,context,live): 3P+2N+B and heap local N;
	// decode_heap(command,output): 2P+N+B; checked add(reference,amount): P+N+B.
	3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + 2 * sizeof(void *) +
	sizeof(size_t) + sizeof(bool) + sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// Genuine vector capacity/size queries are sequential. The existing owning
	// observation subtotal covers their complete source scopes and checked-add
	// descendants; its fresh-copy observation excess is conservatively retained.
	critical_normalize_observation_frames;
constexpr size_t critical_startup_codec_vector_defaults =
	// Default vector, _Vector_base, _Vector_impl, allocator, new_allocator and
	// _Vector_impl_data each own this. Four decoded members plus encoder result.
	// This is the actual default constructor chain, not a command-copy profile.
	5 * 6 * sizeof(void *) +
	// Actual vector/_Vector_base destructor and get-allocator formal/return,
	// surrounding genuine trivial _Destroy/deallocate closure. Sum all five
	// member/result cleanup alternatives, including exception paths.
	5 * (4 * sizeof(void *) + critical_normalize_allocator_frames);
constexpr size_t critical_startup_codec_vector_operations =
	// Same genuine GNU13 source algorithms already named by the normalize owner:
	// allocator/traits/C++20 construct/deallocate; trivial pointer copy/relocate;
	// reserve; forward insert; assign; const/rvalue push/emplace/realloc_insert;
	// iterator/query/check_len/advance. Actual byte/key/revision T is trivial;
	// pointer iterators and returned tagged normal iterators use selected LP64.
	// No sort/copy-command profile is aliased to this codec source allowance.
	critical_normalize_vector_frames +
	// Original encoder result vector move; original decoded command's four
	// member moves/old-destination disposal/moved-from destruction. Authentic
	// inline _M_move_assign temporary vectors are part of these owning closures.
	critical_normalize_move_frames + critical_normalize_move_frames_total;
constexpr size_t critical_startup_codec_c_callers =
	// Actual memcmp(input,magic,size)->int; memcpy/memmove(dst,src,n)->pointer.
	// External libc/operator-new/delete implementation remains qualification-open.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t critical_startup_codec_complete_source =
	critical_startup_codec_encode_callers + critical_startup_codec_encode_loops +
	critical_startup_codec_append_read + critical_startup_codec_decode_callers +
	critical_startup_codec_decode_observation + critical_startup_codec_vector_defaults +
	critical_startup_codec_vector_operations + critical_startup_codec_c_callers +
	// Complete authentic envelope/native-auction/legacy/zero/key/binary-search
	// predicate closure dominates the exact envelope-only codec subset.
	critical_normalize_valid_frames;
#endif
} // namespace

bool critical_command_startup_codec_source_frame_bytes(size_t *output) noexcept
{
	if (!output)
		return false;
#if defined(__linux__) && defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) &&               \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                         \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) && \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                      \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                   \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	// Actual captured ordinary LP64 selected-source policy, not merely a GNU
	// version label. A profile query never scans storage or grants admission.
	if constexpr (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8 ||
		      sizeof(std::allocator<uint8_t>) != 1 ||
		      sizeof(std::allocator<critical_entity_key>) != 1 ||
		      sizeof(std::allocator<critical_expected_revision>) != 1 ||
		      sizeof(std::vector<uint8_t>::iterator) != sizeof(void *) ||
		      sizeof(std::vector<critical_entity_key>::const_iterator) != sizeof(void *) ||
		      sizeof(std::vector<critical_expected_revision>::const_iterator) !=
			      sizeof(void *) ||
		      !std::is_trivially_copyable_v<critical_entity_key> ||
		      !std::is_trivially_copyable_v<critical_expected_revision>)
		return false;
	// Result vector/decoded command and temporary decoded key/revision inline
	// objects are already owned by unchanged real codec admission. Input/prior
	// output heaps and caller frames remain outer. This adds source closure only.
	*output = critical_startup_codec_complete_source;
	return true;
#else
	return false;
#endif
}

// Pure contracts for the unchanged fixed-context derivation above. Captured
// ordinary GNU13 LP64/C++20/OpenSSL3.0.13 source carriers only; emitted stack,
// allocator/libc implementation internals and whole-host qualification are separate.
namespace
{
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	!defined(_GLIBCXX_DEBUG) && defined(OPENSSL_VERSION_MAJOR) &&                             \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
[[maybe_unused]] constexpr size_t critical_derive_existing_local_source =
	4 * sizeof(void *) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(size_t) + sizeof(bool) +
	sizeof(size_t) + 2 * sizeof(size_t) +
	2 * (sizeof(void *) + 2 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool)) +
	2 * sizeof(size_t) + sizeof(void *) + sizeof(bool) + sizeof(size_t) + sizeof(void *) +
	sizeof(bool);
// Both zero predicates have lexical range references. The first occurs before
// the existing array/copy admission, so its active begin/end -> data closure
// also needs retained coverage. The phases below do not overlap.
[[maybe_unused]] constexpr size_t critical_derive_zero_supplement =
	2 * sizeof(void *) + 4 * sizeof(void *);
[[maybe_unused]] constexpr size_t critical_derive_memory_supplement =
	std::max(std::max(3 * sizeof(void *) + sizeof(size_t),
			  2 * sizeof(void *) + sizeof(size_t) + sizeof(int)),
		 std::max((sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
				  (2 * sizeof(void *) + sizeof(size_t) + sizeof(int)),
			  sizeof(void *) + sizeof(size_t) + sizeof(uint64_t)));
[[maybe_unused]] constexpr size_t critical_derive_retained_supplement =
	std::max(critical_derive_zero_supplement, critical_derive_memory_supplement);
[[maybe_unused]] constexpr size_t critical_derive_complete_source =
	critical_derive_existing_local_source + critical_derive_copy_array_frames +
	critical_derive_sha_frames + critical_derive_retained_supplement;
[[maybe_unused]] constexpr size_t critical_derive_initial_inline =
	sizeof(std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES + sizeof(uint32_t) + sizeof(uint64_t)>) +
	sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>) + sizeof(SHA256_CTX);
#endif
}

bool critical_operation_id_derive_source_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	!defined(_GLIBCXX_DEBUG) && defined(OPENSSL_VERSION_MAJOR) &&                             \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return false;
	*bytes = critical_derive_complete_source;
	return true;
#else
	return false;
#endif
}

bool critical_operation_id_derive_initial_inline_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	!defined(_GLIBCXX_DEBUG) && defined(OPENSSL_VERSION_MAJOR) &&                             \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return false;
	*bytes = critical_derive_initial_inline;
	return true;
#else
	return false;
#endif
}

bool critical_operation_id_derive_source_supplement_frame_bytes(size_t *bytes) noexcept
{
	if (!bytes)
		return false;
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	!defined(_GLIBCXX_DEBUG) && defined(OPENSSL_VERSION_MAJOR) &&                             \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
		return false;
	*bytes = critical_derive_retained_supplement;
	return true;
#else
	return false;
#endif
}

// Additive contracts for critical_command_normalize_bounded only. Existing
// startup encode/decode and original CURRENT profiles remain separate.
namespace
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
template <class T> constexpr size_t critical_normalize_move_missing_source()
{
	// _M_move_assign(true) actually constructs vector __tmp(get_allocator()).
	// The old move subtotal owns __tmp inline, swaps/copy-data, destruction,
	// deallocation and the public move receiver. These distinct allocator
	// descendants have no named credit in that subtotal.
	using allocator = std::allocator<T>;
	constexpr size_t get_allocator = 7 * sizeof(void *) + sizeof(allocator);
	constexpr size_t const_allocator_constructor = 11 * sizeof(void *);
	// C++20 __alloc_on_move: two allocator references, std::move's reference
	// argument/return, generated allocator assignment this/source/return and
	// generated base assignment this/source/return. No old tag dispatch.
	constexpr size_t direct_allocator_move = 10 * sizeof(void *);
	// One existing allocator-return pair is named in the old subtotal;
	// the second argument getter is a distinct reached invocation.
	constexpr size_t allocator_arguments = 2 * sizeof(void *);
	// Actual internal std::move(__x)2P and swap-temp data default receiver P.
	// Existing inline3P and swap/copy signatures do not own these scopes.
	return get_allocator + const_allocator_constructor + direct_allocator_move +
	       allocator_arguments + 3 * sizeof(void *);
}
constexpr size_t critical_normalize_missing_source =
	critical_normalize_move_missing_source<critical_entity_key>() +
	critical_normalize_move_missing_source<critical_expected_revision>() +
	2 * critical_normalize_move_missing_source<uint8_t>() +
	// Actual std::move(normalized), distinct from generated member assignment.
	2 * sizeof(void *) +
	// allocator allocate/deallocate each reaches __is_constant_evaluated();
	// old allocation/deallocation signature subtotal has no bool result.
	2 * sizeof(bool) +
	// Actual runtime copy_a/copy_move_a2/trivial_Destroy constant queries3B.
	// Old memmove/assignable bools are different; normalize does not relocate.
	3 * sizeof(bool) +
	// Public constexpr frames and sort_frame constexpr recursion scalars; the
	// sort leaf getter's returned size_t and __lg argument/result -> clzl.
	3 * sizeof(size_t) + 2 * sizeof(std::ptrdiff_t) + sizeof(long) + sizeof(int);

template <class Comparator> constexpr size_t critical_normalize_recursive_source()
{
	size_t levels = 0;
	for (size_t remaining = CRITICAL_COMMAND_MAX_NATIVE_AUCTION_KEYS; remaining > 1;
	     remaining >>= 1)
		++levels;
	// Both actual vectors pass the original envelope limit before sorting;
	// 4099 is the genuine larger accepted native-auction key/revision limit.
	// __introsort_loop: first/last/cut, depth_limit and real comparator.
	return (2 * levels + 1) *
	       (3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(Comparator));
}
constexpr size_t critical_normalize_existing_public_source =
	6 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool) + sizeof(size_t) +
	sizeof(size_t) + sizeof(void *);
constexpr size_t critical_normalize_existing_sort_source =
	critical_normalize_sort_leaf_frames<critical_entity_key,
					    decltype(&critical_entity_key_less)>() +
	critical_normalize_sort_leaf_frames<critical_entity_key,
					    decltype(&critical_entity_key_equal)>() +
	critical_normalize_sort_leaf_frames<critical_expected_revision, char>() +
	critical_normalize_recursive_source<decltype(&critical_entity_key_less)>() +
	critical_normalize_recursive_source<char>() +
	// Two actual sort_frame signatures: this/count/levels/remaining/request/B.
	2 * (sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool));
constexpr size_t critical_normalize_complete_source =
	critical_normalize_existing_public_source + critical_normalize_observation_frames +
	critical_normalize_valid_frames + critical_normalize_command_copy_frames +
	critical_normalize_move_frames_total + critical_normalize_existing_sort_source +
	critical_normalize_missing_source;
constexpr bool critical_normalize_source_layout =
	sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	sizeof(long) == 8 && sizeof(bool) == 1 && sizeof(std::allocator<uint8_t>) == 1 &&
	sizeof(std::allocator<critical_entity_key>) == 1 &&
	sizeof(std::allocator<critical_expected_revision>) == 1 &&
	sizeof(std::vector<critical_entity_key>::iterator) == sizeof(void *) &&
	sizeof(std::vector<critical_expected_revision>::iterator) == sizeof(void *) &&
	std::is_trivially_copyable_v<critical_entity_key> &&
	std::is_trivially_copyable_v<critical_expected_revision> &&
	std::is_nothrow_move_assignable_v<critical_command>;
#endif
}
bool critical_command_normalize_source_frame_bytes(size_t *out) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if (!out || !critical_normalize_source_layout)
		return false;
	*out = critical_normalize_complete_source;
	return true;
#else
	(void)out;
	return false;
#endif
}
bool critical_command_normalize_source_supplement_frame_bytes(size_t *out) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if (!out || !critical_normalize_source_layout)
		return false;
	*out = critical_normalize_missing_source;
	return true;
#else
	(void)out;
	return false;
#endif
}
bool critical_command_normalize_initial_inline_bytes(size_t *out) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if (!out || !critical_normalize_source_layout)
		return false;
	*out = sizeof(critical_normalize_budget); // Constructed before the first peak.
	return true;
#else
	(void)out;
	return false;
#endif
}

// Complete source companions for the actual unchanged bounded wire codecs.
// The preserved historical startup getter lacks the named move descendants.
namespace
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
constexpr size_t critical_startup_codec_move_missing_source =
	// Decoded command's four real members plus encoded result byte vector.
	critical_normalize_move_missing_source<critical_entity_key>() +
	critical_normalize_move_missing_source<critical_expected_revision>() +
	3 * critical_normalize_move_missing_source<uint8_t>() +
	// Both real original final std::move expressions: decoded and result.
	2 * (2 * sizeof(void *)) +
	// Actual allocator allocate/deallocate constant-evaluation returns,
	// outside the unchanged allocator signature subtotal.
	2 * sizeof(bool) +
	// Copy_a/copy_move_a2/trivial_Destroy query3B, plus actual nonempty
	// relocate_a_1 queryB during repeated startup key/revision push growth.
	// Never project this relocation into exact-copy normalization.
	4 * sizeof(bool) +
	// Original encoder and actual bounded decoder each catch const bad_alloc&.
	// Neither real reference is named in the old startup scalar subtotal.
	2 * sizeof(void *) +
	// Original memcpy's returned destination is distinct from its three
	// argument pointers already owned by critical_startup_codec_c_callers.
	sizeof(void *);
constexpr size_t critical_startup_codec_complete_corrected_source =
	critical_startup_codec_complete_source + critical_startup_codec_move_missing_source;
#endif
} // namespace
bool critical_command_startup_codec_complete_source_frame_bytes(size_t *out) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if (!out || !critical_normalize_source_layout)
		return false;
	*out = critical_startup_codec_complete_corrected_source;
	return true;
#else
	(void)out;
	return false;
#endif
}
bool critical_command_startup_codec_source_supplement_frame_bytes(size_t *out) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if (!out || !critical_normalize_source_layout)
		return false;
	*out = critical_startup_codec_complete_corrected_source;
	return true;
#else
	(void)out;
	return false;
#endif
}
bool critical_command_startup_codec_initial_inline_bytes(size_t *out) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0)
	if (!out || !critical_normalize_source_layout)
		return false;
	*out = 0;
	return true;
#else
	(void)out;
	return false;
#endif
}
