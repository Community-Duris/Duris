#include "critical_command.h"

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

bool valid_entity_type(critical_entity_type type)
{
	return type >= critical_entity_type::player && type <= critical_entity_type::shopkeeper;
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
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	if (std::adjacent_find(command->keys.begin(), command->keys.end(),
			       critical_entity_key_equal) != command->keys.end())
		return false;
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  [](const critical_expected_revision &left,
		     const critical_expected_revision &right)
		  { return critical_entity_key_less(left.key, right.key); });
	return critical_command_valid(*command);
}

bool critical_command_valid(const critical_command &command)
{
	if (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(command.operation_id) || !command.payload_version ||
	    command.type < critical_command_type::test ||
	    command.type > critical_command_type::boon_shop ||
	    command.source_site < critical_source_site::command ||
	    command.source_site > critical_source_site::operator_repair ||
	    command.deadline_class < critical_deadline_class::interactive ||
	    command.deadline_class > critical_deadline_class::recovery ||
	    !command.accepted_at_usec || command.keys.empty() ||
	    command.keys.size() > CRITICAL_COMMAND_MAX_KEYS ||
	    command.expected_revisions.size() > CRITICAL_COMMAND_MAX_KEYS ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	for (size_t index = 0; index < command.keys.size(); ++index)
	{
		if (!valid_entity_type(command.keys[index].type) || !command.keys[index].id ||
		    (index &&
		     !critical_entity_key_less(command.keys[index - 1], command.keys[index])))
			return false;
	}
	for (size_t index = 0; index < command.expected_revisions.size(); ++index)
	{
		const critical_entity_key &key = command.expected_revisions[index].key;
		if (!valid_entity_type(key.type) || !key.id ||
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

critical_command_codec_result critical_command_encode(const critical_command &command,
						      std::vector<uint8_t> *encoded)
{
	if (!encoded || !critical_command_valid(command))
		return critical_command_codec_result::invalid;
	encoded->clear();
	try
	{
		encoded->reserve(64 + command.keys.size() * 16 +
				 command.expected_revisions.size() * 24 + command.payload.size());
		encoded->insert(encoded->end(), COMMAND_MAGIC,
				COMMAND_MAGIC + sizeof(COMMAND_MAGIC));
		append_le<uint32_t>(*encoded, command.schema_version);
		encoded->insert(encoded->end(), command.operation_id.bytes.begin(),
				command.operation_id.bytes.end());
		append_le<uint16_t>(*encoded, static_cast<uint16_t>(command.type));
		append_le<uint16_t>(*encoded, command.payload_version);
		append_le<uint16_t>(*encoded, static_cast<uint16_t>(command.source_site));
		encoded->push_back(static_cast<uint8_t>(command.deadline_class));
		encoded->push_back(0);
		append_le<uint64_t>(*encoded, command.accepted_at_usec);
		append_le<uint32_t>(*encoded, static_cast<uint32_t>(command.keys.size()));
		append_le<uint32_t>(*encoded,
				    static_cast<uint32_t>(command.expected_revisions.size()));
		append_le<uint32_t>(*encoded, static_cast<uint32_t>(command.payload.size()));
		for (const critical_entity_key &key : command.keys)
		{
			encoded->push_back(static_cast<uint8_t>(key.type));
			for (unsigned int pad = 0; pad < 7; ++pad)
				encoded->push_back(0);
			append_le<uint64_t>(*encoded, key.id);
		}
		for (const critical_expected_revision &revision : command.expected_revisions)
		{
			encoded->push_back(static_cast<uint8_t>(revision.key.type));
			for (unsigned int pad = 0; pad < 7; ++pad)
				encoded->push_back(0);
			append_le<uint64_t>(*encoded, revision.key.id);
			append_le<uint64_t>(*encoded, revision.revision);
		}
		encoded->insert(encoded->end(), command.payload.begin(), command.payload.end());
	}
	catch (const std::bad_alloc &)
	{
		encoded->clear();
		return critical_command_codec_result::overflow;
	}
	return encoded->size() <= CRITICAL_COMMAND_MAX_ENCODED_BYTES ?
		       critical_command_codec_result::ok :
		       critical_command_codec_result::overflow;
}

critical_command_codec_result critical_command_decode(const uint8_t *encoded, size_t size,
						      critical_command *command)
{
	if (!encoded || !command || size < 52)
		return critical_command_codec_result::truncated;
	if (size > CRITICAL_COMMAND_MAX_ENCODED_BYTES || memcmp(encoded, COMMAND_MAGIC, 4) != 0)
		return critical_command_codec_result::invalid;
	size_t offset = 4;
	critical_command decoded = {};
	uint16_t type = 0, source = 0;
	uint32_t key_count = 0, revision_count = 0, payload_size = 0;
	if (!read_le(encoded, size, &offset, &decoded.schema_version))
		return critical_command_codec_result::truncated;
	if (decoded.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION)
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
	if (encoded[offset + 1] != 0)
		return critical_command_codec_result::invalid;
	offset += 2;
	if (!read_le(encoded, size, &offset, &decoded.accepted_at_usec) ||
	    !read_le(encoded, size, &offset, &key_count) ||
	    !read_le(encoded, size, &offset, &revision_count) ||
	    !read_le(encoded, size, &offset, &payload_size))
		return critical_command_codec_result::truncated;
	if (!key_count || key_count > CRITICAL_COMMAND_MAX_KEYS ||
	    revision_count > CRITICAL_COMMAND_MAX_KEYS ||
	    payload_size > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return critical_command_codec_result::overflow;
	const uint64_t required = static_cast<uint64_t>(key_count) * 16 +
				  static_cast<uint64_t>(revision_count) * 24 + payload_size;
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
		decoded.payload.assign(encoded + offset, encoded + size);
	}
	catch (const std::bad_alloc &)
	{
		return critical_command_codec_result::overflow;
	}
	if (!critical_command_valid(decoded))
		return critical_command_codec_result::invalid;
	*command = std::move(decoded);
	return critical_command_codec_result::ok;
}
