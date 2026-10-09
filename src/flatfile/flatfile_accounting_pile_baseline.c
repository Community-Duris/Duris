#include "flatfile/flatfile_accounting_pile_baseline.h"

#include "flatfile/flatfile_item_repository.h"
#include "player/player_snapshot_codec.h"
#include "flatfile/flatfile_accounting_baseline.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "core/defines.h"
#include <algorithm>
#include <climits>


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

// Authenticate an existing opening baseline without re-running genesis capture.
// The caller selects the original command using the retained head operation ID.
unsigned int flatfile_accounting_pile_baseline_read_room_locked(const std::string &root,
								const flatfile_authority_lock &lock,
								const critical_command &original,
								uint64_t uid,
								flatfile_room_coin_pile *output,
								std::string *error) noexcept
try
{
	if (root.empty() || !lock.matches(root) || !uid || !output ||
	    original.type != critical_command_type::economic_baseline ||
	    original.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(original))
		return EINVAL;
	const auto accounting_error = [](flatfile_accounting_status status)
	{
		return status == flatfile_accounting_status::not_found ? ENOENT :
		       status == flatfile_accounting_status::io_error  ? EIO :
		       status == flatfile_accounting_status::capacity  ? ENOMEM :
									 EILSEQ;
	};
	const auto native_error = [](flatfile_item_repository_result status)
	{
		return status == flatfile_item_repository_result::not_found ? ENOENT :
		       status == flatfile_item_repository_result::io_error  ? EIO :
									      EILSEQ;
	};
	flatfile_accounting_pile_state head;
	auto status = flatfile_accounting_pile_state_read(root, lock, uid, &head, error);
	if (status != flatfile_accounting_status::ok)
		return accounting_error(status);
	if (head.retired || !head.item_revision ||
	    head.account.kind != economic_account_kind::pile || head.account.authority_id != uid ||
	    head.account.context_id || head.operation_id.bytes != original.operation_id.bytes)
		return ESTALE;
	flatfile_economic_authority_snapshot authority;
	const auto authority_error = economic_flatfile_lock_authority(
		root, lock, head.account.lineage, head.epoch, {}, &authority, error);
	if (authority_error)
		return authority_error;
	flatfile_accounting_record receipt;
	std::vector<uint8_t> encoded_witness;
	status = flatfile_accounting_baseline_lookup(root, lock, original, &receipt,
						     &encoded_witness, error);
	if (status != flatfile_accounting_status::ok)
		return accounting_error(status);
	std::optional<economic_prepared_baseline> prepared;
	if (economic_baseline_decode(encoded_witness, &prepared) != economic_accounting_error::ok ||
	    !prepared)
		return EILSEQ;
	const auto &witness = prepared->witness();
	if (witness.lineage.bytes != head.account.lineage.bytes ||
	    witness.epoch.bytes != head.epoch.bytes ||
	    prepared->plan().metadata.operation_id.bytes != original.operation_id.bytes)
		return ESTALE;
	const economic_baseline_holding *holding = nullptr;
	const economic_baseline_item *item = nullptr;
	for (const auto &entry : witness.holdings)
		if (economic_account_key_equal(entry.account, head.account))
		{
			if (holding)
				return EILSEQ;
			holding = &entry;
		}
	for (const auto &entry : witness.items)
		if (entry.snapshot.uid == uid || entry.snapshot.position.root_uid == uid ||
		    entry.snapshot.position.parent_uid == uid)
		{
			if (item || entry.snapshot.uid != uid)
				return EILSEQ;
			item = &entry;
		}
	if (!holding || !item || holding->balance != head.balance ||
	    holding->native_revision != head.item_revision ||
	    holding->source_digest != item->source_digest)
		return EILSEQ;
	const auto &position = item->snapshot.position;
	if (position.root_uid != uid || position.parent_uid || position.equipment_slot ||
	    position.state != item_custody_state::active ||
	    position.revision != head.item_revision ||
	    position.owner.type != item_owner_type::room || !position.owner.id ||
	    position.owner.id > INT_MAX || position.owner.context_id)
		return EOPNOTSUPP;
	// Read the complete catalog, including inactive rows, before selecting the UID.
	std::vector<flatfile_item_ownership_record> catalog;
	auto native_status =
		flatfile_item_repository_recovery_catalog_locked(root, lock, &catalog, error);
	if (native_status != flatfile_item_repository_result::ok)
		return native_error(native_status);
	const flatfile_item_ownership_record *selected = nullptr;
	for (const auto &entry : catalog)
		if (entry.item_uid == uid || entry.root_item_uid == uid ||
		    entry.parent_item_uid == uid)
		{
			if (selected || entry.item_uid != uid || entry.root_item_uid != uid ||
			    entry.parent_item_uid || entry.equipment_slot ||
			    entry.state != position.state ||
			    entry.item_revision != position.revision ||
			    !item_owner_identity_equal(entry.owner, position.owner))
				return EILSEQ;
			selected = &entry;
		}
	if (!selected)
		return ENOENT;
	std::vector<flatfile_item_ownership_record> owned;
	uint64_t owner_revision = 0;
	native_status = flatfile_item_repository_load_owner_locked(root, lock, position.owner,
								   &owner_revision, &owned, error);
	if (native_status != flatfile_item_repository_result::ok)
		return native_error(native_status);
	if (!owner_revision || std::count_if(owned.begin(), owned.end(), [&](const auto &entry)
					     { return entry.item_uid == uid; }) != 1)
		return EILSEQ;
	flatfile_coin_pile_source native;
	native_status =
		flatfile_item_repository_read_coin_pile_locked(root, lock, uid, &native, error);
	if (native_status != flatfile_item_repository_result::ok)
		return native_error(native_status);
	const auto &literal = native.item;
	if (literal.object_uid != uid || literal.vnum != selected->vnum ||
	    literal.type != ITEM_MONEY || literal.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
	    literal.equipment_slot != -1)
		return EILSEQ;
	for (size_t index = 0; index < 4; ++index)
		if (literal.values[index] < 0 || literal.values[index] != head.balance[index])
			return EILSEQ;
	std::vector<uint8_t> canonical;
	if (player_item_snapshot_list_encode({ literal }, &canonical) !=
		    player_snapshot_codec_result::ok ||
	    (!selected->coin_payload.empty() && canonical != selected->coin_payload) ||
	    source_digest(native,
			  selected->coin_payload.empty() ? canonical : selected->coin_payload) !=
		    holding->source_digest)
		return EILSEQ;
	flatfile_room_coin_pile candidate;
	candidate.lineage = authority.lineage;
	candidate.epoch = authority.epoch;
	candidate.lineage_revision = authority.lineage_revision;
	candidate.root_operation = original.operation_id;
	candidate.retained_root_revision = receipt.durable_revision;
	candidate.identity = { uid,
			       uid,
			       0,
			       position.owner,
			       head.item_revision,
			       owner_revision,
			       selected->vnum,
			       item_custody_state::active };
	candidate.item = std::move(native.item);
	*output = std::move(candidate);
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
