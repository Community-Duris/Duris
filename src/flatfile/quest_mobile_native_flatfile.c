#include "flatfile/quest_mobile_native_flatfile.h"
#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <cerrno>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <utility>

namespace
{
int codec_error(player_snapshot_codec_result result) noexcept
{
	if (result == player_snapshot_codec_result::ok)
		return 0;
	if (result == player_snapshot_codec_result::allocation_failure)
		return ENOMEM;
	if (result == player_snapshot_codec_result::limit_exceeded)
		return E2BIG;
	return EBADMSG;
}

bool nonzero(const critical_operation_id &id) noexcept
{
	return std::any_of(id.bytes.begin(), id.bytes.end(),
			   [](uint8_t byte) { return byte != 0; });
}

std::string filename(uint64_t id)
{
	return "quest-mobile-native-" + std::to_string(id) + ".qmn";
}

int stable_birth(const quest_mobile_native_reference &before,
		 const quest_mobile_native_reference &after) noexcept
{
	auto normalized = after;
	normalized.mobile_revision = before.mobile_revision;
	normalized.stock_revision = before.stock_revision;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> a = {}, b = {};
	int error = codec_error(quest_mobile_native_reference_encode(before, &a));
	if (!error)
		error = codec_error(quest_mobile_native_reference_encode(normalized, &b));
	return error ? error : a == b ? 0 : ESTALE;
}

int read_locked(const std::string &root, const flatfile_authority_lock &lock, uint64_t id,
		quest_mobile_native_flatfile_row *output)
{
	if (!lock.matches(root))
		return EINVAL;
	std::vector<uint8_t> bytes;
	// nullptr avoids allocation-backed diagnostic strings inside raw FD IO.
	errno = 0;
	const auto read = flatfile_read(root + "/domains", filename(id),
					PLAYER_SNAPSHOT_MAX_BYTES + 1, &bytes, nullptr);
	const int saved_error = errno;
	quest_mobile_native_flatfile_row candidate;
	candidate.mobile_instance_id = id;
	if (read == flatfile_read_result::not_found)
	{
		if (!lock.matches(root))
			return EINVAL;
		*output = std::move(candidate);
		return 0;
	}
	if (read == flatfile_read_result::invalid)
		return EBADMSG;
	if (read != flatfile_read_result::ok)
		return saved_error ? saved_error : EIO;
	if (bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
		return E2BIG;
	int error = codec_error(quest_mobile_native_image_decode(bytes, &candidate.image));
	if (error)
		return error;
	if (candidate.image.reference.mobile_instance_id != id)
		return EBADMSG;
	std::vector<uint8_t> canonical;
	error = codec_error(quest_mobile_native_image_encode(candidate.image, &canonical));
	if (error)
		return error;
	if (canonical != bytes)
		return EBADMSG;
	if (!lock.matches(root))
		return EINVAL;
	candidate.present = true;
	*output = std::move(candidate);
	return 0;
}

static_assert(std::is_nothrow_move_assignable_v<quest_mobile_native_flatfile_row>);
static_assert(std::is_nothrow_move_assignable_v<flatfile_authority_operation>);

// QNO1 is only storage framing around the unchanged actual NMR1 attachment.
// The explicit phase is the observed terminal phase supplied at preparation.
// SHA256 covers every header/body byte, including the actual revision.
constexpr size_t origin_header_bytes = 16;
constexpr size_t origin_overhead_bytes = origin_header_bytes + SHA256_DIGEST_LENGTH;
constexpr size_t origin_maximum_bytes =
	origin_overhead_bytes + CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES;
static_assert(origin_maximum_bytes < flatfile_authority_transaction_maximum_bytes);

std::string origin_filename(uint64_t id)
{
	return "quest-mobile-native-" + std::to_string(id) + ".qno";
}

int birth_codec_error(economic_accounting_error error) noexcept
{
	if (error == economic_accounting_error::ok)
		return 0;
	return error == economic_accounting_error::capacity ? ENOMEM : EBADMSG;
}

int origin_encode(const critical_native_recovery_envelope &original, std::vector<uint8_t> *output)
{
	if (original.attachment.empty() ||
	    original.attachment.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES)
		return E2BIG;
	critical_command retained_command;
	int error = birth_codec_error(native_mobile_birth_recovery_original_command_decode(
		original.attachment, &retained_command));
	if (error)
		return error;
	if (!critical_command_equal(retained_command, original.command) ||
	    !native_mobile_birth_recovery_terminal(original))
		return EBADMSG;
	std::vector<uint8_t> bytes(origin_overhead_bytes + original.attachment.size());
	bytes[0] = 'Q';
	bytes[1] = 'N';
	bytes[2] = 'O';
	bytes[3] = '1';
	bytes[4] = 1;
	bytes[5] = 0;
	bytes[6] = static_cast<uint8_t>(original.phase);
	bytes[7] = 0;
	for (size_t index = 0; index < sizeof(original.revision); ++index)
		bytes[8 + index] = static_cast<uint8_t>(original.revision >> (8 * index));
	std::copy(original.attachment.begin(), original.attachment.end(),
		  bytes.begin() + origin_header_bytes);
	const size_t checked_size = bytes.size() - SHA256_DIGEST_LENGTH;
	if (!SHA256(bytes.data(), checked_size, bytes.data() + checked_size))
		return EIO;
	*output = std::move(bytes);
	return 0;
}

int origin_decode(std::span<const uint8_t> bytes, critical_native_recovery_envelope *output)
{
	// Bound the entire actual file and fixed fields before any allocation.
	if (bytes.size() <= origin_overhead_bytes || bytes.size() > origin_maximum_bytes ||
	    bytes[0] != 'Q' || bytes[1] != 'N' || bytes[2] != 'O' || bytes[3] != '1' ||
	    bytes[4] != 1 || bytes[5] != 0 ||
	    bytes[6] !=
		    static_cast<uint8_t>(critical_native_recovery_phase::continuation_pending) ||
	    bytes[7] != 0)
		return EBADMSG;
	uint64_t revision = 0;
	for (size_t index = 0; index < sizeof(revision); ++index)
		revision |= uint64_t{ bytes[8 + index] } << (8 * index);
	if (!revision)
		return EBADMSG;
	const size_t checked_size = bytes.size() - SHA256_DIGEST_LENGTH;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest{};
	if (!SHA256(bytes.data(), checked_size, digest.data()))
		return EIO;
	if (CRYPTO_memcmp(digest.data(), bytes.data() + checked_size, digest.size()))
		return EBADMSG;
	const auto body = bytes.subspan(origin_header_bytes, checked_size - origin_header_bytes);
	critical_native_recovery_envelope candidate;
	int error = birth_codec_error(
		native_mobile_birth_recovery_original_command_decode(body, &candidate.command));
	if (error)
		return error;
	candidate.revision = revision;
	// This is the actual checked storage field, not an inferred journal state.
	candidate.phase = static_cast<critical_native_recovery_phase>(bytes[6]);
	candidate.attachment.assign(body.begin(), body.end());
	if (!native_mobile_birth_recovery_terminal(candidate))
		return EBADMSG;
	*output = std::move(candidate);
	return 0;
}

int origin_read_locked(const std::string &root, const flatfile_authority_lock &lock,
		       const quest_mobile_native_reference &reference,
		       quest_mobile_native_flatfile_origin_row *output)
{
	// Require real current native storage even when historical origin is absent.
	quest_mobile_native_flatfile_row current;
	int error = read_locked(root, lock, reference.mobile_instance_id, &current);
	if (error)
		return error;
	if (!current.present)
		return ENOENT;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> requested{}, actual{};
	error = codec_error(quest_mobile_native_reference_encode(reference, &requested));
	if (!error)
		error = codec_error(
			quest_mobile_native_reference_encode(current.image.reference, &actual));
	if (error)
		return error;
	if (requested != actual)
		return ESTALE;
	std::vector<uint8_t> bytes;
	errno = 0;
	const auto read = flatfile_read(root + "/domains",
					origin_filename(reference.mobile_instance_id),
					origin_maximum_bytes, &bytes, nullptr);
	const int saved_error = errno;
	quest_mobile_native_flatfile_origin_row candidate;
	if (read == flatfile_read_result::not_found)
	{
		if (!lock.matches(root))
			return EINVAL;
		*output = std::move(candidate);
		return 0;
	}
	if (read == flatfile_read_result::invalid)
		return EBADMSG;
	if (read != flatfile_read_result::ok)
		return saved_error ? saved_error : EIO;
	error = origin_decode(bytes, &candidate.original);
	if (error)
		return error;
	quest_mobile_native_image birth;
	error = birth_codec_error(
		native_mobile_birth_command_decode(candidate.original.command, &birth));
	if (!error)
		error = stable_birth(reference, birth.reference);
	if (error)
		return error;
	if (candidate.original.command.operation_id.bytes != reference.birth_operation.bytes)
		return EBADMSG;
	if (!lock.matches(root))
		return EINVAL;
	candidate.present = true;
	*output = std::move(candidate);
	return 0;
}

static_assert(std::is_nothrow_move_assignable_v<critical_native_recovery_envelope>);
static_assert(std::is_nothrow_move_assignable_v<quest_mobile_native_flatfile_origin_row>);
} // namespace

int quest_mobile_native_flatfile_read_locked(const std::string &root,
					     const flatfile_authority_lock &lock, uint64_t id,
					     quest_mobile_native_flatfile_row *output) noexcept
{
	if (!output || root.empty() || !id || id == UINT64_MAX)
		return EINVAL;
	try
	{
		return read_locked(root, lock, id, output);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

int quest_mobile_native_flatfile_prepare_locked(const std::string &root,
						const flatfile_authority_lock &lock,
						const critical_operation_id &parent,
						const quest_mobile_native_flatfile_row &before,
						const quest_mobile_native_image &after,
						flatfile_authority_operation *output) noexcept
{
	if (!output || root.empty() || !before.mobile_instance_id ||
	    before.mobile_instance_id == UINT64_MAX || !nonzero(parent) ||
	    after.reference.mobile_instance_id != before.mobile_instance_id ||
	    after.last_transition_operation.bytes != parent.bytes ||
	    !quest_mobile_native_cash_transition_valid(before.present ? &before.image : nullptr,
						       after))
		return EINVAL;
	try
	{
		if (!lock.matches(root))
			return EINVAL;
		std::vector<uint8_t> after_bytes, before_bytes;
		int error = codec_error(quest_mobile_native_image_encode(after, &after_bytes));
		if (error)
			return error;
		if (before.present)
		{
			if (before.image.reference.mobile_instance_id !=
				    before.mobile_instance_id ||
			    (before.image.state == quest_mobile_lifetime_state::retired &&
			     after.state != quest_mobile_lifetime_state::retired))
				return ESTALE;
			error = stable_birth(before.image.reference, after.reference);
			if (!error)
				error = codec_error(quest_mobile_native_image_encode(
					before.image, &before_bytes));
			if (error)
				return error;
		}
		else if (after.reference.birth_operation.bytes != parent.bytes)
			return EINVAL;
		quest_mobile_native_flatfile_row current;
		error = read_locked(root, lock, before.mobile_instance_id, &current);
		if (error)
			return error;
		if (current.present != before.present)
			return ESTALE;
		if (current.present)
		{
			std::vector<uint8_t> actual;
			error = codec_error(
				quest_mobile_native_image_encode(current.image, &actual));
			if (error)
				return error;
			if (actual != before_bytes)
				return ESTALE;
		}
		flatfile_authority_operation candidate;
		candidate.store = flatfile_authority_store::domains;
		candidate.kind = flatfile_authority_operation_kind::write;
		candidate.filename = filename(before.mobile_instance_id);
		candidate.bytes = std::move(after_bytes);
		if (!lock.matches(root))
			return EINVAL;
		*output = std::move(candidate);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

int quest_mobile_native_flatfile_origin_read_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const quest_mobile_native_reference &reference,
	quest_mobile_native_flatfile_origin_row *output) noexcept
{
	if (!output || root.empty() || !reference.mobile_instance_id ||
	    reference.mobile_instance_id == UINT64_MAX)
		return EINVAL;
	try
	{
		return origin_read_locked(root, lock, reference, output);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}

int quest_mobile_native_flatfile_origin_prepare_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original,
	flatfile_authority_operation *output) noexcept
{
	if (!output || root.empty() || !lock.matches(root))
		return EINVAL;
	try
	{
		std::vector<uint8_t> bytes;
		int error = origin_encode(original, &bytes);
		if (error)
			return error;
		quest_mobile_native_image birth;
		error = birth_codec_error(
			native_mobile_birth_command_decode(original.command, &birth));
		if (error)
			return error;
		quest_mobile_native_flatfile_row current;
		error = read_locked(root, lock, birth.reference.mobile_instance_id, &current);
		if (error)
			return error;
		if (!current.present)
			return ENOENT;
		std::vector<uint8_t> born_bytes, current_bytes;
		error = codec_error(quest_mobile_native_image_encode(birth, &born_bytes));
		if (!error)
			error = codec_error(
				quest_mobile_native_image_encode(current.image, &current_bytes));
		if (error)
			return error;
		if (born_bytes != current_bytes)
			return ESTALE;
		quest_mobile_native_flatfile_origin_row retained;
		error = origin_read_locked(root, lock, birth.reference, &retained);
		if (error)
			return error;
		if (retained.present)
		{
			std::vector<uint8_t> prior;
			error = origin_encode(retained.original, &prior);
			if (error)
				return error;
			if (prior != bytes)
				return ESTALE;
		}
		flatfile_authority_operation candidate;
		candidate.store = flatfile_authority_store::domains;
		candidate.kind = flatfile_authority_operation_kind::write;
		candidate.filename = origin_filename(birth.reference.mobile_instance_id);
		candidate.bytes = std::move(bytes);
		if (!lock.matches(root))
			return EINVAL;
		*output = std::move(candidate);
		return 0;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}
