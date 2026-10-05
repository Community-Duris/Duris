#include "flatfile/quest_mobile_native_flatfile.h"
#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <cerrno>
#include <new>
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
	    after.last_transition_operation.bytes != parent.bytes)
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
