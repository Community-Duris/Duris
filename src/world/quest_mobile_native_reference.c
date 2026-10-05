#include "world/quest_mobile_native_reference.h"
#include "player/player_snapshot_codec.h"
#include "economy/economic_accounting_types.h"

#include <algorithm>
#include <bit>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>

namespace
{
constexpr std::array<uint8_t, 8> reference_magic = { 'Q', 'M', 'N', 'R', 'E', 'F', 0, 0 };

bool nonzero(const critical_operation_id &id)
{
	return std::any_of(id.bytes.begin(), id.bytes.end(),
			   [](uint8_t value) { return value != 0; });
}

template <typename T> void put(uint8_t *bytes, size_t &offset, T value)
{
	using U = std::make_unsigned_t<T>;
	U bits = static_cast<U>(value);
	for (size_t n = 0; n < sizeof(T); ++n)
	{
		bytes[offset++] = static_cast<uint8_t>(bits & 0xff);
		bits >>= 8;
	}
}

template <typename T> T get(const uint8_t *bytes, size_t &offset)
{
	using U = std::make_unsigned_t<T>;
	U bits = 0;
	for (size_t n = 0; n < sizeof(T); ++n)
		bits |= static_cast<U>(bytes[offset++]) << (8 * n);
	return std::bit_cast<T>(bits);
}

bool checksum(std::span<const uint8_t> bytes)
{
	if (bytes.size() < SHA256_DIGEST_LENGTH)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	const size_t sealed = bytes.size() - digest.size();
	return SHA256(bytes.data(), sealed, digest.data()) &&
	       CRYPTO_memcmp(digest.data(), bytes.data() + sealed, digest.size()) == 0;
}

} // namespace

bool quest_mobile_native_reference_valid(const quest_mobile_native_reference &value) noexcept
{
	return value.mobile_instance_id && value.mobile_instance_id != UINT64_MAX &&
	       nonzero(value.birth_operation) && economic_source_event_valid(value.birth_source) &&
	       value.mobile_vnum >= 0 && value.mobile_revision && value.stock_revision &&
	       ((value.provenance == quest_mobile_birth_provenance::reset &&
		 value.reset_zone_vnum >= 0) ||
		(value.provenance == quest_mobile_birth_provenance::spawn &&
		 value.reset_zone_vnum == -1));
}

player_snapshot_codec_result quest_mobile_native_reference_encode(
	const quest_mobile_native_reference &value,
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> *output) noexcept
{
	if (!output || !quest_mobile_native_reference_valid(value))
		return player_snapshot_codec_result::invalid_value;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> candidate = {};
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source = {};
	if (economic_source_event_encode(value.birth_source, &source) !=
	    economic_accounting_error::ok)
		return player_snapshot_codec_result::invalid_value;
	std::copy(reference_magic.begin(), reference_magic.end(), candidate.begin());
	size_t offset = reference_magic.size();
	put<uint16_t>(candidate.data(), offset, QUEST_MOBILE_NATIVE_VERSION);
	put<uint8_t>(candidate.data(), offset, static_cast<uint8_t>(value.provenance));
	put<uint8_t>(candidate.data(), offset, 0);
	put<uint32_t>(candidate.data(), offset, candidate.size());
	put<uint64_t>(candidate.data(), offset, value.mobile_instance_id);
	std::copy(value.birth_operation.bytes.begin(), value.birth_operation.bytes.end(),
		  candidate.begin() + offset);
	offset += value.birth_operation.bytes.size();
	std::copy(source.begin(), source.end(), candidate.begin() + offset);
	offset += source.size();
	put<int32_t>(candidate.data(), offset, value.mobile_vnum);
	put<int32_t>(candidate.data(), offset, value.birthplace_vnum);
	put<int32_t>(candidate.data(), offset, value.reset_zone_vnum);
	put<uint64_t>(candidate.data(), offset, value.mobile_revision);
	put<uint64_t>(candidate.data(), offset, value.stock_revision);
	if (offset != candidate.size() - SHA256_DIGEST_LENGTH ||
	    !SHA256(candidate.data(), offset, candidate.data() + offset))
		return player_snapshot_codec_result::invalid_value;
	*output = candidate;
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result
quest_mobile_native_reference_decode(std::span<const uint8_t> bytes,
				     quest_mobile_native_reference *output) noexcept
{
	if (!output || bytes.size() != QUEST_MOBILE_NATIVE_REFERENCE_BYTES)
		return player_snapshot_codec_result::invalid_value;
	if (!std::equal(reference_magic.begin(), reference_magic.end(), bytes.begin()) ||
	    !checksum(bytes))
		return player_snapshot_codec_result::invalid_value;
	size_t offset = reference_magic.size();
	if (get<uint16_t>(bytes.data(), offset) != QUEST_MOBILE_NATIVE_VERSION)
		return player_snapshot_codec_result::unsupported_version;
	quest_mobile_native_reference candidate;
	candidate.provenance =
		static_cast<quest_mobile_birth_provenance>(get<uint8_t>(bytes.data(), offset));
	if (get<uint8_t>(bytes.data(), offset) ||
	    get<uint32_t>(bytes.data(), offset) != bytes.size())
		return player_snapshot_codec_result::invalid_value;
	candidate.mobile_instance_id = get<uint64_t>(bytes.data(), offset);
	std::copy_n(bytes.data() + offset, candidate.birth_operation.bytes.size(),
		    candidate.birth_operation.bytes.begin());
	offset += candidate.birth_operation.bytes.size();
	if (economic_source_event_decode(bytes.subspan(offset, ECONOMIC_SOURCE_EVENT_BYTES),
					 &candidate.birth_source) != economic_accounting_error::ok)
		return player_snapshot_codec_result::invalid_value;
	offset += ECONOMIC_SOURCE_EVENT_BYTES;
	candidate.mobile_vnum = get<int32_t>(bytes.data(), offset);
	candidate.birthplace_vnum = get<int32_t>(bytes.data(), offset);
	candidate.reset_zone_vnum = get<int32_t>(bytes.data(), offset);
	candidate.mobile_revision = get<uint64_t>(bytes.data(), offset);
	candidate.stock_revision = get<uint64_t>(bytes.data(), offset);
	if (!quest_mobile_native_reference_valid(candidate))
		return player_snapshot_codec_result::invalid_value;
	*output = candidate;
	return player_snapshot_codec_result::ok;
}
