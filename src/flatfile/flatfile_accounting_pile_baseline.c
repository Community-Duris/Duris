#include "flatfile/flatfile_accounting_pile_baseline.h"

#include "flatfile/flatfile_item_repository.h"
#include "player/player_snapshot_codec.h"

#include <cerrno>
#include <new>
#include <openssl/sha.h>

namespace
{
void number(std::vector<uint8_t> &bytes, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		bytes.push_back(static_cast<uint8_t>(value >> (index * 8)));
}

economic_digest source_digest(const flatfile_coin_pile_source &source,
			      const std::vector<uint8_t> &payload)
{
	std::vector<uint8_t> bytes{ 'D', 'U', 'R', 'I', 'S', '-', 'F', 'L', 'A', 'T', '-', 'P', 'I',
				    'L', 'E', '-', 'S', 'O', 'U', 'R', 'C', 'E', '-', 'V', '1' };
	const auto &item = source.ownership;
	number(bytes, item.item_uid);
	number(bytes, item.root_item_uid);
	number(bytes, item.parent_item_uid);
	number(bytes, static_cast<uint8_t>(item.owner.type));
	number(bytes, item.owner.id);
	number(bytes, item.owner.context_id);
	number(bytes, item.item_revision);
	number(bytes, static_cast<uint8_t>(item.state));
	number(bytes, static_cast<uint32_t>(item.vnum));
	number(bytes, payload.size());
	bytes.insert(bytes.end(), payload.begin(), payload.end());
	economic_digest digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	return digest;
}
} // namespace

unsigned int flatfile_accounting_pile_baseline_capture(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &lineage, const critical_operation_id &epoch,
	const critical_operation_id &operation_id, uint64_t uid,
	flatfile_accounting_pile_baseline_source *source, std::string *error) noexcept
try
{
	if (root.empty() || !lock.matches(root) || critical_operation_id_is_zero(lineage) ||
	    critical_operation_id_is_zero(epoch) || critical_operation_id_is_zero(operation_id) ||
	    !uid || !source)
		return EINVAL;
	flatfile_coin_pile_source native;
	const auto loaded =
		flatfile_item_repository_read_coin_pile_locked(root, lock, uid, &native, error);
	if (loaded != flatfile_item_repository_result::ok)
		return loaded == flatfile_item_repository_result::not_found ? ENOENT :
		       loaded == flatfile_item_repository_result::io_error  ? EIO :
									      EILSEQ;
	if (!native.ownership.item_revision || native.ownership.item_uid != uid)
		return EILSEQ;
	flatfile_accounting_pile_state prior;
	const auto head_status =
		flatfile_accounting_pile_state_read(root, lock, uid, &prior, error);
	if (head_status != flatfile_accounting_status::not_found)
		return head_status == flatfile_accounting_status::ok	   ? EALREADY :
		       head_status == flatfile_accounting_status::io_error ? EIO :
									     EILSEQ;
	std::vector<uint8_t> payload;
	if (player_item_snapshot_list_encode({ native.item }, &payload) !=
	    player_snapshot_codec_result::ok)
		return EILSEQ;
	flatfile_accounting_pile_baseline_source captured;
	const economic_account_key key{ lineage, economic_account_kind::pile, uid, 0 };
	if (!economic_account_key_valid(key))
		return EINVAL;
	for (size_t index = 0; index < 4; ++index)
		captured.holding.balance[index] = native.item.values[index];
	captured.holding.account = key;
	captured.holding.native_revision = native.ownership.item_revision;
	captured.holding.source_digest = source_digest(
		native,
		native.ownership.coin_payload.empty() ? payload : native.ownership.coin_payload);
	captured.item.snapshot.uid = uid;
	captured.item.snapshot.position.owner = native.ownership.owner;
	captured.item.snapshot.position.root_uid = native.ownership.root_item_uid;
	captured.item.snapshot.position.parent_uid = native.ownership.parent_item_uid;
	captured.item.snapshot.position.revision = native.ownership.item_revision;
	captured.item.snapshot.position.state = native.ownership.state;
	captured.item.source_digest = captured.holding.source_digest;
	captured.head = {
		key,  epoch, operation_id, captured.holding.balance, native.ownership.item_revision,
		false
	};
	*source = std::move(captured);
	return 0;
}
catch (const std::bad_alloc &)
{
	return ENOMEM;
}
catch (...)
{
	return EIO;
}
