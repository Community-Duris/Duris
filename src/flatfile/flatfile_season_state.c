#include "flatfile/flatfile_season_state.h"

#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> state_magic{ 'D', 'U', 'R', 'S', 'E', 'A', 'S', 0 };
constexpr std::array<uint8_t, 8> enrollment_magic{ 'D', 'U', 'R', 'S', 'E', 'E', 'N', 0 };
constexpr uint32_t season_format_version = 1;
constexpr size_t record_bytes = 64, checksum_offset = 32;
constexpr const char *state_filename = "season_state";
constexpr const char *enrollment_filename = "season_enrollment";

void put(uint8_t *bytes, uint64_t value, size_t count) noexcept
{
	for (size_t i = 0; i < count; ++i)
		bytes[i] = static_cast<uint8_t>(value >> (8 * i));
}
uint64_t get(const uint8_t *bytes, size_t count) noexcept
{
	uint64_t value = 0;
	for (size_t i = 0; i < count; ++i)
		value |= static_cast<uint64_t>(bytes[i]) << (8 * i);
	return value;
}
bool checked_record(const std::vector<uint8_t> &bytes, const std::array<uint8_t, 8> &magic) noexcept
{
	if (bytes.size() != record_bytes || !std::equal(magic.begin(), magic.end(), bytes.begin()))
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
	return SHA256(bytes.data(), checksum_offset, digest.data()) &&
	       CRYPTO_memcmp(digest.data(), bytes.data() + checksum_offset, digest.size()) == 0;
}
bool decode(const std::vector<uint8_t> &state, const std::vector<uint8_t> &marker,
	    flatfile_season_state *output) noexcept
{
	if (!output || !checked_record(state, state_magic) ||
	    !checked_record(marker, enrollment_magic) ||
	    get(state.data() + 8, 4) != season_format_version || get(state.data() + 24, 8) ||
	    get(marker.data() + 8, 4) != season_format_version ||
	    get(marker.data() + 12, 4) != season_format_version ||
	    get(marker.data() + 16, 8) != 1 ||
	    get(marker.data() + 24, 4) != static_cast<uint32_t>(flatfile_season_status::active) ||
	    get(marker.data() + 28, 4))
		return false;
	const uint64_t epoch = get(state.data() + 16, 8);
	const uint32_t status = static_cast<uint32_t>(get(state.data() + 12, 4));
	if (!epoch || epoch == UINT64_MAX ||
	    (status != static_cast<uint32_t>(flatfile_season_status::active) &&
	     status != static_cast<uint32_t>(flatfile_season_status::resetting)))
		return false;
	*output = { epoch, static_cast<flatfile_season_status>(status) };
	return true;
}
bool genesis(std::vector<uint8_t> *state, std::vector<uint8_t> *marker)
{
	state->assign(record_bytes, 0);
	marker->assign(record_bytes, 0);
	std::copy(state_magic.begin(), state_magic.end(), state->begin());
	put(state->data() + 8, season_format_version, 4);
	put(state->data() + 12, static_cast<uint32_t>(flatfile_season_status::active), 4);
	put(state->data() + 16, 1, 8);
	std::copy(enrollment_magic.begin(), enrollment_magic.end(), marker->begin());
	put(marker->data() + 8, season_format_version, 4);
	put(marker->data() + 12, season_format_version, 4);
	put(marker->data() + 16, 1, 8);
	put(marker->data() + 24, static_cast<uint32_t>(flatfile_season_status::active), 4);
	return SHA256(state->data(), checksum_offset, state->data() + checksum_offset) &&
	       SHA256(marker->data(), checksum_offset, marker->data() + checksum_offset);
}
flatfile_season_state_result read_records(const std::string &root, std::vector<uint8_t> *state,
					  std::vector<uint8_t> *marker)
{
	const std::string directory = root + "/metadata";
	const auto a = flatfile_read(directory, state_filename, record_bytes, state, nullptr);
	const auto b = flatfile_read(directory, enrollment_filename, record_bytes, marker, nullptr);
	if (a == flatfile_read_result::io_error || b == flatfile_read_result::io_error)
		return flatfile_season_state_result::io_error;
	if (a == flatfile_read_result::not_found && b == flatfile_read_result::not_found)
		return flatfile_season_state_result::not_found;
	if (a != flatfile_read_result::ok || b != flatfile_read_result::ok)
		return flatfile_season_state_result::invalid;
	return flatfile_season_state_result::ok;
}
} // namespace

flatfile_season_state_result
flatfile_season_state_read_locked(const std::string &root, const flatfile_authority_lock &lock,
				  flatfile_season_state *output) noexcept
{
	if (root.empty() || !output || !lock.matches(root))
		return flatfile_season_state_result::invalid;
	try
	{
		std::vector<uint8_t> state, marker;
		const auto read = read_records(root, &state, &marker);
		if (read != flatfile_season_state_result::ok)
			return read;
		flatfile_season_state observed;
		if (!decode(state, marker, &observed) ||
		    observed.status != flatfile_season_status::active || !lock.matches(root))
			return flatfile_season_state_result::invalid;
		*output = observed;
		return flatfile_season_state_result::ok;
	}
	catch (...)
	{
		return flatfile_season_state_result::io_error;
	}
}

flatfile_season_state_result
flatfile_season_bootstrap::enroll_fresh_locked(const std::string &root,
					       const flatfile_authority_lock &lock,
					       flatfile_season_state *output) noexcept
{
	if (root.empty() || !output || !lock.matches(root))
		return flatfile_season_state_result::invalid;
	bool commit_started = false;
	flatfile_authority_commit_outcome outcome =
		flatfile_authority_commit_outcome::not_published;
	try
	{
		std::vector<uint8_t> old_state, old_marker;
		if (read_records(root, &old_state, &old_marker) !=
		    flatfile_season_state_result::not_found)
			return flatfile_season_state_result::invalid;
		std::vector<flatfile_authority_operation> operations(2);
		operations[0].store = operations[1].store = flatfile_authority_store::metadata;
		operations[0].kind = operations[1].kind = flatfile_authority_operation_kind::write;
		operations[0].filename = state_filename;
		operations[1].filename = enrollment_filename;
		if (!genesis(&operations[0].bytes, &operations[1].bytes) || !lock.matches(root))
			return flatfile_season_state_result::invalid;
		commit_started = true;
		const auto committed =
			flatfile_authority_transaction_commit_operations_with_outcome(
				root, lock, operations, nullptr, &outcome);
		if (committed != flatfile_authority_transaction_result::ok ||
		    outcome != flatfile_authority_commit_outcome::committed)
			return outcome == flatfile_authority_commit_outcome::not_published ?
				       flatfile_season_state_result::io_error :
				       flatfile_season_state_result::publication_uncertain;
		std::vector<uint8_t> state, marker;
		flatfile_season_state observed;
		if (read_records(root, &state, &marker) != flatfile_season_state_result::ok ||
		    state != operations[0].bytes || marker != operations[1].bytes ||
		    !decode(state, marker, &observed) || observed.epoch != 1 ||
		    observed.status != flatfile_season_status::active || !lock.matches(root))
			return flatfile_season_state_result::publication_uncertain;
		*output = observed;
		return flatfile_season_state_result::ok;
	}
	catch (...)
	{
		// A throw after issuing commit cannot prove that no publication occurred.
		return commit_started ? flatfile_season_state_result::publication_uncertain :
					flatfile_season_state_result::io_error;
	}
}
