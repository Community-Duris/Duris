#include "flatfile/flatfile_accounting_lifecycle_transaction.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_accounting_staging_view.h"
#include "flatfile/flatfile_accounting_pile_baseline.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_snapshot_file.h"
#include "core/defines.h"
#include "world/vnum.obj.h"
#include <iterator>
#include "flatfile/flatfile_store.h"
#include "economy/currency_command.h"
#include "economy/economic_accounting_plan.h"
#include "economy/economic_gameplay_authority.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <map>
#include <openssl/sha.h>
#include <set>
#include <type_traits>

namespace
{
struct failure
{
	unsigned int code;
};

void need(bool condition, unsigned int error = EILSEQ)
{
	if (!condition)
		throw failure{ error };
}

bool nonzero(const critical_operation_id &id)
{
	return !critical_operation_id_is_zero(id);
}

std::string canonical_identity_account(const std::string &account)
{
	std::string canonical = account;
	for (char &character : canonical)
		if (character >= 'A' && character <= 'Z')
			character = static_cast<char>(character - 'A' + 'a');
	return canonical;
}

economic_digest hash(std::span<const uint8_t> data)
{
	economic_digest value = {};
	SHA256(data.data(), data.size(), value.data());
	return value;
}

void append_u32(std::vector<uint8_t> *data, uint32_t value)
{
	for (size_t index = 0; index < 4; ++index)
		data->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

void append_u64(std::vector<uint8_t> *data, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		data->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

constexpr std::array<uint8_t, 8> lifecycle_receipt_magic = { 'D', 'U', 'R', 'E', 'L', 'R', 0, 0 };
constexpr size_t lifecycle_receipt_maximum_bytes =
	1024 + ECONOMIC_BASELINE_MAX_HOLDINGS * (156 + 2 * CURRENCY_ACCOUNT_NAME_MAX_BYTES) +
	CRITICAL_COMMAND_MAX_ENCODED_BYTES + ECONOMIC_BASELINE_MAX_BYTES +
	ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES;
static_assert(lifecycle_receipt_maximum_bytes < UINT32_MAX);
static_assert(lifecycle_receipt_maximum_bytes < flatfile_authority_transaction_maximum_bytes);

struct retained_lifecycle
{
	uint32_t frame_version = 1;
	flatfile_accounting_lifecycle_request request;
	economic_account_key opening;
	flatfile_accounting_lifecycle_receipt receipt;
	critical_operation_id lineage_creating_operation = {};
	uint64_t selected_control_revision = 0;
	flatfile_economic_epoch epoch;
	flatfile_accounting_lifecycle_native_sources sources;
	std::vector<uint8_t> baseline_command, baseline_witness, baseline_plan;
};

economic_digest holding_source_digest(uint8_t, uint64_t, uint8_t, const std::string &, uint64_t,
				      const economic_coin_vector &);
economic_digest
compute_coverage_digest(const std::vector<flatfile_accounting_lifecycle_wallet_source> &,
			const std::vector<flatfile_accounting_lifecycle_bank_source> &);

void source_name_valid(const std::string &name)
{
	need(!name.empty() && name.size() <= CURRENCY_ACCOUNT_NAME_MAX_BYTES);
	for (char character : name)
		need((character >= 'a' && character <= 'z') ||
		     (character >= '0' && character <= '9') || character == '_' ||
		     character == '-');
}

void append_number(std::vector<uint8_t> &data, uint64_t number, size_t width)
{
	for (size_t index = 0; index < width; ++index)
		data.push_back(static_cast<uint8_t>(number >> (8 * index)));
}

void append_raw(std::vector<uint8_t> &data, std::span<const uint8_t> bytes)
{
	data.insert(data.end(), bytes.begin(), bytes.end());
}

void checked(economic_accounting_error code)
{
	need(code == economic_accounting_error::ok,
	     code == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
}

void checked(critical_command_codec_result code)
{
	need(code == critical_command_codec_result::ok,
	     code == critical_command_codec_result::overflow ? ENOMEM : EILSEQ);
}

void append_key(std::vector<uint8_t> &data, const economic_account_key &key)
{
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded = {};
	checked(economic_account_key_encode(key, &encoded));
	append_raw(data, encoded);
}

struct lifecycle_reader
{
	std::span<const uint8_t> bytes;
	size_t position = 0;
	std::span<const uint8_t> take(size_t count)
	{
		need(position <= bytes.size() && count <= bytes.size() - position);
		auto value = bytes.subspan(position, count);
		position += count;
		return value;
	}
	uint64_t number(size_t width = 8)
	{
		auto input = take(width);
		uint64_t value = 0;
		for (size_t index = 0; index < width; ++index)
			value |= uint64_t(input[index]) << (8 * index);
		return value;
	}
	template <size_t Count> std::array<uint8_t, Count> fixed()
	{
		const auto input = take(Count);
		std::array<uint8_t, Count> value = {};
		std::copy(input.begin(), input.end(), value.begin());
		return value;
	}
	critical_operation_id id() { return { fixed<CRITICAL_COMMAND_ID_BYTES>() }; }
	economic_account_key key()
	{
		economic_account_key value;
		checked(economic_account_key_decode(take(ECONOMIC_ACCOUNT_KEY_BYTES), &value));
		return value;
	}
	std::vector<uint8_t> blob(size_t maximum)
	{
		const size_t size = static_cast<size_t>(number(4));
		need(size <= maximum);
		const auto value = take(size);
		return { value.begin(), value.end() };
	}
	void done() { need(position == bytes.size()); }
};

bool same_epoch(const flatfile_economic_epoch &a, const flatfile_economic_epoch &b)
{
	return a.epoch.bytes == b.epoch.bytes && a.predecessor.bytes == b.predecessor.bytes &&
	       a.creating_operation.bytes == b.creating_operation.bytes && a.ordinal == b.ordinal &&
	       a.transition_kind == b.transition_kind &&
	       a.transition_digest == b.transition_digest &&
	       a.baseline_initialization == b.baseline_initialization &&
	       a.baseline_initializing_operation.bytes == b.baseline_initializing_operation.bytes &&
	       economic_account_key_equal(a.baseline_opening, b.baseline_opening);
}

bool same_request(const flatfile_accounting_lifecycle_request &a,
		  const flatfile_accounting_lifecycle_request &b)
{
	return a.operation_id.bytes == b.operation_id.bytes && a.lineage.bytes == b.lineage.bytes &&
	       a.epoch.bytes == b.epoch.bytes && a.actor_id == b.actor_id &&
	       a.accepted_at_usec == b.accepted_at_usec && a.coverage_digest == b.coverage_digest &&
	       a.boundary_digest == b.boundary_digest &&
	       a.frozen_boundary_proven == b.frozen_boundary_proven &&
	       a.virgin_state_proven == b.virgin_state_proven;
}

std::string lifecycle_receipt_name(const critical_operation_id &id)
{
	char value[CRITICAL_COMMAND_ID_HEX_SIZE] = {};
	need(critical_operation_id_to_hex(id, value, sizeof(value)), EINVAL);
	// The original ID is unique within this root, even if lineage changes.
	return std::string("lifecycle-") + value + ".elr";
}

std::vector<flatfile_accounting_pile_baseline_source>
retained_pile_sources(const economic_baseline_batch &witness,
		      const flatfile_accounting_lifecycle_receipt &receipt)
{
	need(receipt.pile_uids.size() <= ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES &&
	     std::is_sorted(receipt.pile_uids.begin(), receipt.pile_uids.end()) &&
	     std::adjacent_find(receipt.pile_uids.begin(), receipt.pile_uids.end()) ==
		     receipt.pile_uids.end() &&
	     witness.items.size() == receipt.pile_uids.size());
	std::vector<flatfile_accounting_pile_baseline_source> sources;
	sources.reserve(receipt.pile_uids.size());
	for (uint64_t uid : receipt.pile_uids)
	{
		const auto holding = std::find_if(
			witness.holdings.begin(), witness.holdings.end(),
			[&](const auto &entry) {
				return entry.account.kind == economic_account_kind::pile &&
				       entry.account.authority_id == uid;
			});
		const auto item = std::find_if(witness.items.begin(), witness.items.end(),
					       [&](const auto &entry)
					       { return entry.snapshot.uid == uid; });
		need(uid && holding != witness.holdings.end() && item != witness.items.end());
		const auto &position = item->snapshot.position;
		need(holding->account.lineage.bytes == receipt.lineage.bytes &&
		     !holding->account.context_id && holding->native_revision &&
		     position.root_uid == uid && !position.parent_uid &&
		     position.owner.type == item_owner_type::room && position.owner.id &&
		     position.owner.id <= INT_MAX && !position.owner.context_id &&
		     position.state == item_custody_state::active && !position.equipment_slot &&
		     position.revision == holding->native_revision &&
		     holding->source_digest != economic_digest{} &&
		     item->source_digest == holding->source_digest);
		for (int64_t amount : holding->balance)
			need(amount >= 0 && amount <= INT32_MAX);
		flatfile_accounting_pile_baseline_source source;
		source.holding = *holding;
		source.item = *item;
		source.head = { holding->account,
				receipt.epoch,
				receipt.baseline_operation_id,
				holding->balance,
				holding->native_revision,
				false };
		sources.push_back(std::move(source));
	}
	return sources;
}

economic_digest
compute_pile_coverage_digest(const flatfile_accounting_lifecycle_native_sources &sources,
			     const std::vector<flatfile_accounting_pile_baseline_source> &piles)
{
	// Keep the original wallet/bank V1 digest untouched. V2 adds the complete
	// ordered native room-pile cut; its EAB descriptors are retained history.
	std::vector<uint8_t> data{ 'D', 'U', 'R', 'I', 'S', '-', 'F', 'L', 'A', 'T', 'F', 'I', 'L',
				   'E', '-', 'C', 'O', 'V', 'E', 'R', 'A', 'G', 'E', '-', 'V', '2' };
	append_raw(data, compute_coverage_digest(sources.wallets, sources.banks));
	append_u64(&data, piles.size());
	uint64_t prior_uid = 0;
	for (const auto &source : piles)
	{
		const auto &position = source.item.snapshot.position;
		need(source.item.snapshot.uid > prior_uid);
		prior_uid = source.item.snapshot.uid;
		append_u64(&data, prior_uid);
		append_u64(&data, position.owner.id);
		append_u64(&data, source.holding.native_revision);
		for (int64_t amount : source.holding.balance)
			append_u64(&data, static_cast<uint64_t>(amount));
		append_raw(data, source.holding.source_digest);
	}
	return hash(data);
}

void reject_selected_player_money(const std::string &root,
				  const flatfile_identity_lock &identity_lock,
				  const flatfile_authority_lock &lock,
				  const flatfile_accounting_lifecycle_native_sources &sources,
				  std::string *error)
{
	// Existing atomic file reader takes no player lock: preserve identity then
	// authority order. A missing/partial selected snapshot is not an empty forest.
	for (const auto &wallet : sources.wallets)
	{
		flatfile_identity_record identity;
		const auto identity_status = flatfile_identity_lookup_pid_locked(
			root, identity_lock, lock, wallet.pid, &identity, error);
		need(identity_status == flatfile_identity_result::ok,
		     identity_status == flatfile_identity_result::io_error ? EIO : EILSEQ);
		need(identity.pid == static_cast<int32_t>(wallet.pid) &&
		     canonical_identity_account(identity.account) == wallet.account_name &&
		     identity.racewar == wallet.racewar);
		flatfile_player_domain_record domain;
		const auto domain_status = flatfile_player_domain_load_locked(
			root, lock, wallet.pid, wallet.account_name, wallet.racewar, &domain,
			error);
		need(domain_status == flatfile_player_domain_result::ok,
		     domain_status == flatfile_player_domain_result::io_error ? EIO : EILSEQ);
		need(domain.pid == identity.pid && domain.account_name == wallet.account_name &&
		     domain.racewar == wallet.racewar &&
		     domain.domains.wallet_revision == wallet.native_revision);
		for (size_t index = 0; index < 4; ++index)
			need(domain.domains.wallet[index] <= INT64_MAX &&
			     static_cast<int64_t>(domain.domains.wallet[index]) ==
				     wallet.balance[index]);
		player_snapshot snapshot;
		const auto status =
			flatfile_player_snapshot_read(root, wallet.pid, &snapshot, error);
		need(status == flatfile_player_load_result::ok,
		     status == flatfile_player_load_result::not_found ? ENODATA :
		     status == flatfile_player_load_result::io_error  ? EIO :
									EILSEQ);
		need(snapshot.pid == identity.pid && snapshot.revision &&
		     snapshot.components == PLAYER_CHECKPOINT_COMPONENT_ALL);
		const std::string *name = nullptr;
		for (const auto &entry : snapshot.status_strings)
			if (entry.field == player_status_string_field::name)
			{
				need(!name);
				name = &entry.value;
			}
		need(name && canonical_identity_account(*name) ==
				     canonical_identity_account(identity.name));
		bool racewar_found = false;
		for (const auto &entry : snapshot.status_integers)
			if (entry.field == player_status_field::racewar)
			{
				need(!racewar_found &&
				     (entry.is_unsigned ? entry.unsigned_value == wallet.racewar :
							  entry.signed_value == wallet.racewar));
				racewar_found = true;
			}
		need(racewar_found);
		// Checkpoint revision and domain wallet revision have distinct lifetimes.
		// Bind each through its original reader; never compare them as one counter.
		const auto inspect = [](const std::vector<player_item_snapshot> &items)
		{
			for (const auto &item : items)
				need(item.type != ITEM_MONEY && item.vnum != VOBJ_COINS,
				     EOPNOTSUPP);
		};
		inspect(snapshot.items);
		for (const auto &pet : snapshot.pets)
			inspect(pet.items);
	}
}

void validate_pile_capacity(size_t wallets, size_t banks, size_t piles)
{
	need(wallets <= ECONOMIC_BASELINE_MAX_HOLDINGS &&
		     banks <= ECONOMIC_BASELINE_MAX_HOLDINGS - wallets &&
		     piles <= ECONOMIC_BASELINE_MAX_HOLDINGS - wallets - banks &&
		     piles <= ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES,
	     ENOSPC);
}

std::vector<flatfile_accounting_pile_baseline_source> capture_room_pile_sources(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &lock, const flatfile_accounting_lifecycle_request &request,
	const critical_operation_id &baseline_operation,
	const flatfile_accounting_lifecycle_native_sources &sources, std::string *error)
{
	validate_pile_capacity(sources.wallets.size(), sources.banks.size(), 0);
	reject_selected_player_money(root, identity_lock, lock, sources, error);
	std::vector<flatfile_coin_pile_source> native;
	const auto code =
		flatfile_item_repository_capture_room_coin_piles_locked(root, lock, &native, error);
	need(!code, code);
	validate_pile_capacity(sources.wallets.size(), sources.banks.size(), native.size());
	std::vector<flatfile_accounting_pile_baseline_source> captured;
	captured.reserve(native.size());
	uint64_t prior_uid = 0;
	for (const auto &pile : native)
	{
		const auto &owner = pile.ownership;
		const uint64_t uid = owner.item_uid;
		// The current cold boot owner requires a positive native money literal.
		// Refuse empty genesis here; do not manufacture retirement or native effects.
		need(std::any_of(pile.item.values.begin(), pile.item.values.begin() + 4,
				 [](int32_t amount) { return amount > 0; }),
		     EILSEQ);
		need(uid > prior_uid && owner.root_item_uid == uid && !owner.parent_item_uid &&
		     owner.owner.type == item_owner_type::room && owner.owner.id &&
		     owner.owner.id <= INT_MAX && !owner.owner.context_id && owner.item_revision &&
		     owner.state == item_custody_state::active && !owner.equipment_slot &&
		     pile.item.type == ITEM_MONEY && pile.item.object_uid == uid &&
		     pile.item.vnum == owner.vnum &&
		     pile.item.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
		     pile.item.equipment_slot == -1);
		prior_uid = uid;
		flatfile_accounting_pile_baseline_source source;
		const auto capture = flatfile_accounting_pile_baseline_capture(
			root, lock, request.lineage, request.epoch, baseline_operation, uid,
			&source, error);
		need(!capture, capture);
		need(source.holding.native_revision == owner.item_revision &&
		     source.item.snapshot.position.root_uid == owner.root_item_uid &&
		     source.item.snapshot.position.parent_uid == owner.parent_item_uid &&
		     item_owner_identity_equal(source.item.snapshot.position.owner, owner.owner));
		for (size_t index = 0; index < 4; ++index)
			need(source.holding.balance[index] == pile.item.values[index]);
		captured.push_back(std::move(source));
	}
	return captured;
}

void validate_retained_lifecycle(const retained_lifecycle &value)
{
	const auto &request = value.request;
	const auto &receipt = value.receipt;
	need(value.frame_version == 1 || value.frame_version == 2);
	need(receipt.mappings.size() <= ECONOMIC_BASELINE_MAX_HOLDINGS &&
	     receipt.pile_uids.size() <= ECONOMIC_BASELINE_MAX_HOLDINGS - receipt.mappings.size());
	need(nonzero(request.operation_id) && nonzero(request.lineage) && nonzero(request.epoch) &&
	     request.actor_id && request.accepted_at_usec && request.frozen_boundary_proven &&
	     request.virgin_state_proven && request.boundary_digest != economic_digest{});
	need(economic_account_key_valid(value.opening) &&
	     value.opening.kind == economic_account_kind::opening &&
	     value.opening.lineage.bytes == request.lineage.bytes);
	need(receipt.operation_id.bytes == request.operation_id.bytes &&
	     receipt.lineage.bytes == request.lineage.bytes &&
	     receipt.epoch.bytes == request.epoch.bytes &&
	     receipt.coverage_digest != economic_digest{} &&
	     (request.coverage_digest == economic_digest{} ||
	      request.coverage_digest == receipt.coverage_digest) &&
	     receipt.boundary_digest == request.boundary_digest && receipt.baseline_revision &&
	     nonzero(value.lineage_creating_operation) && value.selected_control_revision &&
	     receipt.mappings.size() <= ECONOMIC_BASELINE_MAX_HOLDINGS);
	critical_operation_id baseline_id = {};
	need(critical_operation_id_derive(request.operation_id, ECONOMIC_BASELINE_OPERATION_DOMAIN,
					  0, &baseline_id));
	need(receipt.baseline_operation_id.bytes == baseline_id.bytes);
	const auto &epoch = value.epoch;
	need(epoch.epoch.bytes == request.epoch.bytes &&
	     epoch.creating_operation.bytes == request.operation_id.bytes && epoch.ordinal &&
	     epoch.ordinal <= FLATFILE_ECONOMIC_MAX_EPOCHS && epoch.transition_kind == 1 &&
	     epoch.transition_digest == receipt.coverage_digest &&
	     epoch.baseline_initialization == flatfile_baseline_initialization::initialized &&
	     epoch.baseline_initializing_operation.bytes == request.operation_id.bytes &&
	     economic_account_key_equal(epoch.baseline_opening, value.opening));
	std::optional<economic_prepared_baseline> prepared;
	checked(economic_baseline_decode(value.baseline_witness, &prepared));
	need(prepared.has_value());
	const auto &witness = prepared->witness();
	need(witness.lineage.bytes == request.lineage.bytes &&
	     witness.epoch.bytes == request.epoch.bytes &&
	     witness.preparation_id.bytes == request.operation_id.bytes &&
	     witness.actor_id == request.actor_id && witness.batch_index == 0 &&
	     witness.coverage_digest == receipt.coverage_digest &&
	     witness.boundary_digest == request.boundary_digest &&
	     economic_account_key_equal(witness.opening_account, value.opening) &&
	     (value.frame_version == 2 || (witness.items.empty() && receipt.pile_uids.empty())) &&
	     witness.holdings.size() == receipt.mappings.size() + receipt.pile_uids.size());
	critical_command command;
	checked(critical_command_decode(value.baseline_command.data(),
					value.baseline_command.size(), &command));
	need(command.operation_id.bytes == baseline_id.bytes &&
	     command.accepted_at_usec == request.accepted_at_usec);
	economic_accounting_plan plan;
	checked(economic_baseline_command_plan(command, *prepared, &plan));
	std::vector<uint8_t> encoded_plan;
	checked(economic_plan_encode(plan, &encoded_plan));
	need(encoded_plan == value.baseline_plan);
	std::set<std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES>> accounts;
	std::set<uint64_t> wallet_ids;
	std::set<std::pair<std::string, uint64_t>> bank_names;
	for (const auto &mapping : receipt.mappings)
	{
		need(mapping.account.lineage.bytes == request.lineage.bytes &&
		     mapping.account.authority_id <= FLATFILE_ECONOMIC_MAX_MAPPINGS &&
		     (mapping.revision ||
		      mapping.last_operation.bytes == mapping.creating_operation.bytes) &&
		     nonzero(mapping.creating_operation) && nonzero(mapping.last_operation) &&
		     !nonzero(mapping.retiring_operation));
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> account = {};
		checked(economic_account_key_encode(mapping.account, &account));
		need(accounts.insert(account).second);
		if (mapping.account.kind == economic_account_kind::wallet)
			need(mapping.account.context_id == 0 && mapping.locator.kind == 1 &&
			     mapping.locator.native_id && mapping.locator.native_id <= INT32_MAX &&
			     mapping.locator.name.empty() &&
			     wallet_ids.insert(mapping.locator.native_id).second);
		else
		{
			need(mapping.account.kind == economic_account_kind::bank &&
			     mapping.account.context_id <= INT8_MAX && mapping.locator.kind == 2 &&
			     mapping.locator.native_id == mapping.account.authority_id &&
			     !mapping.locator.name.empty() &&
			     mapping.locator.name.size() <= CURRENCY_ACCOUNT_NAME_MAX_BYTES &&
			     bank_names.insert({ mapping.locator.name, mapping.account.context_id })
				     .second);
			for (char character : mapping.locator.name)
				need((character >= 'a' && character <= 'z') ||
				     (character >= '0' && character <= '9') || character == '_' ||
				     character == '-');
		}
	}
	for (const auto &holding : witness.holdings)
	{
		if (holding.account.kind == economic_account_kind::pile)
		{
			need(value.frame_version == 2);
			continue;
		}
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> account = {};
		checked(economic_account_key_encode(holding.account, &account));
		need(accounts.erase(account) == 1);
	}
	need(accounts.empty());
	// Original native descriptors bind the retained locator/PID and positional
	// mapping order to independent baseline source fingerprints and coverage.
	// Mapping revision/operation metadata remains immutable .elr authority;
	// neither current mappings nor current native contents reconstruct it.
	const auto &sources = value.sources;
	need(sources.wallets.size() <= receipt.mappings.size() &&
	     sources.banks.size() == receipt.mappings.size() - sources.wallets.size());
	std::map<std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES>, const economic_baseline_holding *>
		holdings;
	for (const auto &holding : witness.holdings)
	{
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> account = {};
		checked(economic_account_key_encode(holding.account, &account));
		need(holdings.emplace(account, &holding).second);
	}
	auto bind_source = [&](size_t index, const economic_coin_vector &balance, uint64_t revision,
			       const economic_digest &digest)
	{
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> account = {};
		checked(economic_account_key_encode(receipt.mappings[index].account, &account));
		const auto found = holdings.find(account);
		need(found != holdings.end());
		need(found->second->balance == balance &&
		     found->second->native_revision == revision &&
		     found->second->source_digest == digest);
		for (int64_t amount : balance)
			need(amount >= 0);
	};
	for (size_t index = 0; index < sources.wallets.size(); ++index)
	{
		const auto &source = sources.wallets[index];
		const auto &mapping = receipt.mappings[index];
		source_name_valid(source.account_name);
		need(source.pid && source.pid <= INT32_MAX && source.racewar <= INT8_MAX &&
		     mapping.account.kind == economic_account_kind::wallet &&
		     mapping.locator.native_id == source.pid);
		const auto digest = holding_source_digest(1, source.pid, source.racewar,
							  source.account_name,
							  source.native_revision, source.balance);
		need(source.source_digest == digest);
		bind_source(index, source.balance, source.native_revision, digest);
	}
	for (size_t index = 0; index < sources.banks.size(); ++index)
	{
		const auto &source = sources.banks[index];
		const auto &mapping = receipt.mappings[sources.wallets.size() + index];
		source_name_valid(source.name);
		need(source.racewar <= INT8_MAX &&
		     mapping.account.kind == economic_account_kind::bank &&
		     mapping.account.context_id == source.racewar &&
		     mapping.locator.name == source.name);
		const auto digest = holding_source_digest(2, 0, source.racewar, source.name,
							  source.native_revision, source.balance);
		need(source.source_digest == digest);
		bind_source(sources.wallets.size() + index, source.balance, source.native_revision,
			    digest);
	}
	if (value.frame_version == 1)
		need(compute_coverage_digest(sources.wallets, sources.banks) ==
		     receipt.coverage_digest);
	else
	{
		const auto piles = retained_pile_sources(witness, receipt);
		need(compute_pile_coverage_digest(sources, piles) == receipt.coverage_digest);
	}
}

std::vector<uint8_t> encode_lifecycle_receipt(const retained_lifecycle &value)
{
	validate_retained_lifecycle(value);
	std::vector<uint8_t> body;
	const auto &request = value.request;
	const auto &receipt = value.receipt;
	append_raw(body, request.operation_id.bytes);
	append_raw(body, request.lineage.bytes);
	append_raw(body, request.epoch.bytes);
	append_u64(&body, request.actor_id);
	append_u64(&body, request.accepted_at_usec);
	append_raw(body, request.coverage_digest);
	append_raw(body, receipt.coverage_digest);
	append_raw(body, request.boundary_digest);
	append_number(body,
		      (request.frozen_boundary_proven ? 1u : 0u) |
			      (request.virgin_state_proven ? 2u : 0u),
		      1);
	append_key(body, value.opening);
	append_raw(body, receipt.baseline_operation_id.bytes);
	append_u64(&body, receipt.baseline_revision);
	append_raw(body, value.lineage_creating_operation.bytes);
	append_u64(&body, value.selected_control_revision);
	const auto &epoch = value.epoch;
	append_raw(body, epoch.epoch.bytes);
	append_raw(body, epoch.predecessor.bytes);
	append_raw(body, epoch.creating_operation.bytes);
	append_u64(&body, epoch.ordinal);
	append_number(body, epoch.transition_kind, 2);
	append_raw(body, epoch.transition_digest);
	append_number(body, static_cast<uint8_t>(epoch.baseline_initialization), 1);
	append_raw(body, epoch.baseline_initializing_operation.bytes);
	append_key(body, epoch.baseline_opening);
	append_u32(&body, static_cast<uint32_t>(receipt.mappings.size()));
	for (const auto &mapping : receipt.mappings)
	{
		append_key(body, mapping.account);
		append_number(body, mapping.locator.kind, 2);
		append_u64(&body, mapping.locator.native_id);
		append_number(body, mapping.locator.name.size(), 2);
		append_raw(body, { reinterpret_cast<const uint8_t *>(mapping.locator.name.data()),
				   mapping.locator.name.size() });
		append_raw(body, mapping.creating_operation.bytes);
		append_raw(body, mapping.retiring_operation.bytes);
		append_raw(body, mapping.last_operation.bytes);
		append_u64(&body, mapping.revision);
	}
	append_u32(&body, static_cast<uint32_t>(value.sources.wallets.size()));
	append_u32(&body, static_cast<uint32_t>(value.sources.banks.size()));
	auto append_source = [&](const std::string &name, uint8_t racewar, uint64_t revision,
				 const economic_coin_vector &balance)
	{
		append_number(body, name.size(), 2);
		append_raw(body, { reinterpret_cast<const uint8_t *>(name.data()), name.size() });
		append_number(body, racewar, 1);
		append_u64(&body, revision);
		for (int64_t amount : balance)
			append_u64(&body, static_cast<uint64_t>(amount));
	};
	for (const auto &source : value.sources.wallets)
	{
		append_u32(&body, source.pid);
		append_source(source.account_name, source.racewar, source.native_revision,
			      source.balance);
	}
	for (const auto &source : value.sources.banks)
		append_source(source.name, source.racewar, source.native_revision, source.balance);
	for (const auto *blob :
	     { &value.baseline_command, &value.baseline_witness, &value.baseline_plan })
	{
		need(blob->size() <= UINT32_MAX);
		append_u32(&body, static_cast<uint32_t>(blob->size()));
		append_raw(body, *blob);
	}
	need(body.size() <= lifecycle_receipt_maximum_bytes - 48, ENOSPC);
	std::vector<uint8_t> result(lifecycle_receipt_magic.begin(), lifecycle_receipt_magic.end());
	append_u32(&result, value.frame_version);
	append_u32(&result, static_cast<uint32_t>(body.size()));
	append_raw(result, hash(body));
	append_raw(result, body);
	return result;
}

retained_lifecycle decode_lifecycle_receipt(std::span<const uint8_t> encoded)
{
	need(encoded.size() >= 48 && encoded.size() <= lifecycle_receipt_maximum_bytes);
	lifecycle_reader envelope{ encoded };
	need(envelope.fixed<8>() == lifecycle_receipt_magic);
	const auto frame_version = envelope.number(4);
	need((frame_version == 1 || frame_version == 2) &&
	     envelope.number(4) == encoded.size() - 48);
	const auto digest = envelope.fixed<32>();
	const auto body = envelope.take(encoded.size() - 48);
	need(digest == hash(body));
	envelope.done();
	lifecycle_reader in{ body };
	retained_lifecycle value;
	value.frame_version = static_cast<uint32_t>(frame_version);
	auto &request = value.request;
	auto &receipt = value.receipt;
	request.operation_id = in.id();
	request.lineage = in.id();
	request.epoch = in.id();
	request.actor_id = in.number();
	request.accepted_at_usec = in.number();
	request.coverage_digest = in.fixed<32>();
	receipt.coverage_digest = in.fixed<32>();
	request.boundary_digest = in.fixed<32>();
	const auto flags = in.number(1);
	need(flags == 3);
	request.frozen_boundary_proven = true;
	request.virgin_state_proven = true;
	value.opening = in.key();
	receipt.operation_id = request.operation_id;
	receipt.lineage = request.lineage;
	receipt.epoch = request.epoch;
	receipt.boundary_digest = request.boundary_digest;
	receipt.baseline_operation_id = in.id();
	receipt.baseline_revision = in.number();
	value.lineage_creating_operation = in.id();
	value.selected_control_revision = in.number();
	value.epoch.epoch = in.id();
	value.epoch.predecessor = in.id();
	value.epoch.creating_operation = in.id();
	value.epoch.ordinal = in.number();
	value.epoch.transition_kind = static_cast<uint16_t>(in.number(2));
	value.epoch.transition_digest = in.fixed<32>();
	value.epoch.baseline_initialization =
		static_cast<flatfile_baseline_initialization>(in.number(1));
	value.epoch.baseline_initializing_operation = in.id();
	value.epoch.baseline_opening = in.key();
	const auto mapping_count = in.number(4);
	need(mapping_count <= ECONOMIC_BASELINE_MAX_HOLDINGS);
	value.receipt.mappings.reserve(static_cast<size_t>(mapping_count));
	for (size_t index = 0; index < mapping_count; ++index)
	{
		flatfile_economic_mapping mapping;
		mapping.account = in.key();
		mapping.locator.kind = static_cast<uint16_t>(in.number(2));
		mapping.locator.native_id = in.number();
		const auto name_size = in.number(2);
		need(name_size <= CURRENCY_ACCOUNT_NAME_MAX_BYTES);
		const auto name = in.take(static_cast<size_t>(name_size));
		mapping.locator.name.assign(name.begin(), name.end());
		mapping.creating_operation = in.id();
		mapping.retiring_operation = in.id();
		mapping.last_operation = in.id();
		mapping.revision = in.number();
		receipt.mappings.push_back(std::move(mapping));
	}
	const auto wallet_count = in.number(4);
	const auto bank_count = in.number(4);
	need(wallet_count <= mapping_count && bank_count == mapping_count - wallet_count);
	value.sources.wallets.reserve(static_cast<size_t>(wallet_count));
	value.sources.banks.reserve(static_cast<size_t>(bank_count));
	auto read_source = [&](std::string &name, uint8_t &racewar, uint64_t &revision,
			       economic_coin_vector &balance)
	{
		const auto name_size = in.number(2);
		need(name_size && name_size <= CURRENCY_ACCOUNT_NAME_MAX_BYTES);
		const auto bytes = in.take(static_cast<size_t>(name_size));
		name.assign(bytes.begin(), bytes.end());
		racewar = static_cast<uint8_t>(in.number(1));
		revision = in.number();
		for (auto &amount : balance)
		{
			const auto number = in.number();
			need(number <= INT64_MAX);
			amount = static_cast<int64_t>(number);
		}
	};
	for (size_t index = 0; index < wallet_count; ++index)
	{
		flatfile_accounting_lifecycle_wallet_source source;
		source.pid = static_cast<uint32_t>(in.number(4));
		read_source(source.account_name, source.racewar, source.native_revision,
			    source.balance);
		source.source_digest =
			holding_source_digest(1, source.pid, source.racewar, source.account_name,
					      source.native_revision, source.balance);
		value.sources.wallets.push_back(std::move(source));
	}
	for (size_t index = 0; index < bank_count; ++index)
	{
		flatfile_accounting_lifecycle_bank_source source;
		read_source(source.name, source.racewar, source.native_revision, source.balance);
		source.source_digest = holding_source_digest(
			2, 0, source.racewar, source.name, source.native_revision, source.balance);
		value.sources.banks.push_back(std::move(source));
	}
	value.baseline_command = in.blob(CRITICAL_COMMAND_MAX_ENCODED_BYTES);
	value.baseline_witness = in.blob(ECONOMIC_BASELINE_MAX_BYTES);
	value.baseline_plan = in.blob(ECONOMIC_ACCOUNTING_MAX_PLAN_BYTES);
	in.done();
	if (value.frame_version == 2)
	{
		std::optional<economic_prepared_baseline> prepared;
		checked(economic_baseline_decode(value.baseline_witness, &prepared));
		need(prepared.has_value());
		for (const auto &item : prepared->witness().items)
			value.receipt.pile_uids.push_back(item.snapshot.uid);
	}
	validate_retained_lifecycle(value);
	const auto canonical = encode_lifecycle_receipt(value);
	need(canonical.size() == encoded.size() &&
	     std::equal(canonical.begin(), canonical.end(), encoded.begin()));
	return value;
}

bool read_retained_lifecycle(const std::string &root, const critical_operation_id &operation,
			     retained_lifecycle *out, std::string *error)
{
	std::vector<uint8_t> encoded;
	errno = 0;
	const auto read = flatfile_read(root + "/economic-evidence",
					lifecycle_receipt_name(operation),
					lifecycle_receipt_maximum_bytes, &encoded, error);
	if (read == flatfile_read_result::not_found)
		return false;
	need(read == flatfile_read_result::ok, errno == ENOMEM			      ? ENOMEM :
					       read == flatfile_read_result::io_error ? EIO :
											EILSEQ);
	auto retained = decode_lifecycle_receipt(encoded);
	need(retained.request.operation_id.bytes == operation.bytes);
	*out = std::move(retained);
	return true;
}

void verify_retained_lifecycle(const std::string &root, const flatfile_authority_lock &lock,
			       const retained_lifecycle &retained,
			       const flatfile_economic_control &control, std::string *error)
{
	need(control.lineage.bytes == retained.request.lineage.bytes &&
	     control.creating_operation.bytes == retained.lineage_creating_operation.bytes &&
	     control.revision >= retained.selected_control_revision &&
	     control.epoch_count >= retained.epoch.ordinal);
	flatfile_economic_epoch current_epoch;
	const auto epoch_error = flatfile_economic_epoch_read(root, lock, retained.request.lineage,
							      retained.request.epoch,
							      &current_epoch, error);
	need(!epoch_error, epoch_error);
	// Receipt v1 stays canonical. Origin is authenticated by the retained catalog,
	// never inferred from the surviving sibling receipt or promoted from history.
	need(current_epoch.initialization_origin !=
	     flatfile_baseline_initialization_origin::baseline_participant);
	need(same_epoch(current_epoch, retained.epoch));
	critical_command original;
	checked(critical_command_decode(retained.baseline_command.data(),
					retained.baseline_command.size(), &original));
	flatfile_accounting_record baseline;
	std::vector<uint8_t> witness;
	// This native lookup verifies the retained book, canonical plan, witness and
	// opening reservations. It can finish an authenticated pending original
	// authority journal; it never prepares a new lifecycle or native mutation.
	const auto lookup = flatfile_accounting_baseline_lookup(root, lock, original, &baseline,
								&witness, error);
	need(lookup == flatfile_accounting_status::ok,
	     lookup == flatfile_accounting_status::capacity ? ENOMEM :
	     lookup == flatfile_accounting_status::io_error ? EIO :
							      EILSEQ);
	std::vector<uint8_t> command;
	checked(critical_command_encode(baseline.command, &command));
	need(command == retained.baseline_command && witness == retained.baseline_witness &&
	     baseline.plan == retained.baseline_plan &&
	     baseline.durable_revision == retained.receipt.baseline_revision &&
	     !baseline.result_code && baseline.failure_stage == critical_failure_stage::none &&
	     baseline.result.empty());
}

bool retry_retained_lifecycle(const std::string &root, const flatfile_authority_lock &lock,
			      const flatfile_accounting_lifecycle_request &request,
			      const economic_account_key &opening,
			      const flatfile_economic_control &control,
			      flatfile_accounting_lifecycle_receipt *receipt, std::string *error)
{
	retained_lifecycle retained;
	if (!read_retained_lifecycle(root, request.operation_id, &retained, error))
		return false;
	need(same_request(retained.request, request) &&
		     economic_account_key_equal(retained.opening, opening),
	     EEXIST);
	verify_retained_lifecycle(root, lock, retained, control, error);
	static_assert(std::is_nothrow_move_assignable_v<flatfile_accounting_lifecycle_receipt>);
	if (receipt)
		*receipt = std::move(retained.receipt);
	return true;
}

economic_digest holding_source_digest(uint8_t kind, uint64_t id, uint8_t context,
				      const std::string &name, uint64_t revision,
				      const economic_coin_vector &balance)
{
	std::vector<uint8_t> data{ 'D', 'U', 'R', 'I', 'S', '-', 'H', 'O', 'L',
				   'D', 'I', 'N', 'G', '-', 'V', '1', kind };
	append_u64(&data, id);
	data.push_back(context);
	append_u32(&data, static_cast<uint32_t>(name.size()));
	data.insert(data.end(), name.begin(), name.end());
	append_u64(&data, revision);
	for (int64_t amount : balance)
		append_u64(&data, static_cast<uint64_t>(amount));
	return hash(data);
}

unsigned int capture_sources(const std::string &root, const flatfile_identity_lock &identity_lock,
			     const flatfile_authority_lock &authority_lock,
			     flatfile_accounting_lifecycle_native_sources *sources,
			     std::string *error)
{
	need(!root.empty() && identity_lock.matches(root) && authority_lock.matches(root) &&
		     sources,
	     EINVAL);
	std::vector<flatfile_identity_record> identities;
	const auto identity_result = flatfile_identity_list_all_locked(
		root, identity_lock, authority_lock, &identities, error);
	need(identity_result == flatfile_identity_result::ok,
	     identity_result == flatfile_identity_result::io_error ? EIO : EILSEQ);
	flatfile_player_domain_native_sources native;
	const auto native_result = flatfile_player_domain_capture_native_sources_locked(
		root, authority_lock, &native, error);
	need(native_result == flatfile_player_domain_result::ok,
	     native_result == flatfile_player_domain_result::io_error ? EIO : EILSEQ);
	need(native.wallets.size() + native.banks.size() <= ECONOMIC_BASELINE_MAX_HOLDINGS, ENOSPC);
	std::map<int32_t, flatfile_identity_record> identities_by_pid;
	for (const auto &identity : identities)
	{
		need(identity.pid > 0 && identity.racewar >= 0, EILSEQ);
		need(identities_by_pid.emplace(identity.pid, identity).second, EILSEQ);
	}
	std::set<int32_t> wallet_pids;
	std::set<std::pair<std::string, uint8_t>> bank_keys;
	for (const auto &bank : native.banks)
		bank_keys.insert({ bank.account_name, static_cast<uint8_t>(bank.racewar) });
	flatfile_accounting_lifecycle_native_sources candidate;
	candidate.wallets.reserve(native.wallets.size());
	candidate.banks.reserve(native.banks.size());
	for (const auto &wallet : native.wallets)
	{
		auto identity = identities_by_pid.find(wallet.pid);
		const std::string identity_account =
			identity == identities_by_pid.end() ?
				std::string{} :
				canonical_identity_account(identity->second.account);
		need(identity != identities_by_pid.end() &&
			     identity_account == wallet.account_name &&
			     identity->second.racewar == wallet.racewar,
		     EILSEQ);
		wallet_pids.insert(wallet.pid);
		flatfile_accounting_lifecycle_wallet_source source;
		source.pid = static_cast<uint32_t>(wallet.pid);
		source.account_name = wallet.account_name;
		source.racewar = static_cast<uint8_t>(wallet.racewar);
		source.native_revision = wallet.revision;
		for (size_t index = 0; index < source.balance.size(); ++index)
		{
			need(wallet.balance[index] <= static_cast<uint64_t>(INT64_MAX), EOVERFLOW);
			source.balance[index] = static_cast<int64_t>(wallet.balance[index]);
		}
		source.source_digest =
			holding_source_digest(1, source.pid, source.racewar, source.account_name,
					      source.native_revision, source.balance);
		candidate.wallets.push_back(std::move(source));
	}
	for (const auto &identity : identities)
		if (identity.active)
		{
			need(wallet_pids.count(identity.pid) == 1, EILSEQ);
			need(bank_keys.count({ canonical_identity_account(identity.account),
					       static_cast<uint8_t>(identity.racewar) }) == 1,
			     EILSEQ);
		}
	for (const auto &bank : native.banks)
	{
		need(bank.racewar >= 0, EILSEQ);
		flatfile_accounting_lifecycle_bank_source source;
		source.name = bank.account_name;
		source.racewar = static_cast<uint8_t>(bank.racewar);
		source.native_revision = bank.revision;
		for (size_t index = 0; index < source.balance.size(); ++index)
		{
			need(bank.balance[index] <= static_cast<uint64_t>(INT64_MAX), EOVERFLOW);
			source.balance[index] = static_cast<int64_t>(bank.balance[index]);
		}
		source.source_digest = holding_source_digest(
			2, 0, source.racewar, source.name, source.native_revision, source.balance);
		candidate.banks.push_back(std::move(source));
	}
	*sources = std::move(candidate);
	return 0;
}

economic_digest
compute_coverage_digest(const std::vector<flatfile_accounting_lifecycle_wallet_source> &wallets,
			const std::vector<flatfile_accounting_lifecycle_bank_source> &banks)
{
	std::vector<uint8_t> data{ 'D', 'U', 'R', 'I', 'S', '-', 'F', 'L', 'A', 'T', 'F', 'I', 'L',
				   'E', '-', 'C', 'O', 'V', 'E', 'R', 'A', 'G', 'E', '-', 'V', '1' };
	append_u64(&data, wallets.size());
	append_u64(&data, banks.size());
	for (const auto &w : wallets)
	{
		data.push_back('W');
		for (size_t i = 0; i < 4; ++i)
			data.push_back(static_cast<uint8_t>(w.pid >> (i * 8)));
		data.push_back(w.racewar);
		append_u32(&data, static_cast<uint32_t>(w.account_name.size()));
		data.insert(data.end(), w.account_name.begin(), w.account_name.end());
		append_u64(&data, w.native_revision);
		for (size_t i = 0; i < 4; ++i)
		{
			uint64_t val = static_cast<uint64_t>(w.balance[i]);
			for (size_t j = 0; j < 8; ++j)
				data.push_back(static_cast<uint8_t>(val >> (j * 8)));
		}
	}
	for (const auto &b : banks)
	{
		data.push_back('B');
		data.push_back(b.racewar);
		append_u32(&data, static_cast<uint32_t>(b.name.size()));
		data.insert(data.end(), b.name.begin(), b.name.end());
		append_u64(&data, b.native_revision);
		for (size_t i = 0; i < 4; ++i)
		{
			uint64_t val = static_cast<uint64_t>(b.balance[i]);
			for (size_t j = 0; j < 8; ++j)
				data.push_back(static_cast<uint8_t>(val >> (j * 8)));
		}
	}
	return hash(data);
}

template <typename F> unsigned int guarded(F &&action, std::string *error) noexcept
{
	try
	{
		action();
		return 0;
	}
	catch (const failure &f)
	{
		try
		{
			if (error && error->empty())
				*error = "lifecycle transaction failure code=" +
					 std::to_string(f.code);
		}
		catch (const std::bad_alloc &)
		{
		}
		return f.code;
	}
	catch (const std::bad_alloc &)
	{
		try
		{
			if (error && error->empty())
				*error = "out of memory";
		}
		catch (const std::bad_alloc &)
		{
		}
		return ENOMEM;
	}
	catch (...)
	{
		try
		{
			if (error && error->empty())
				*error = "unexpected lifecycle exception";
		}
		catch (const std::bad_alloc &)
		{
		}
		return EIO;
	}
}

} // namespace

unsigned int flatfile_accounting_lifecycle_transaction::capture_native_sources_locked(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &authority_lock,
	flatfile_accounting_lifecycle_native_sources *sources, std::string *error) noexcept
{
	return guarded(
		[&]
		{
			need(sources != nullptr, EINVAL);
			flatfile_accounting_lifecycle_native_sources candidate;
			const auto code = capture_sources(root, identity_lock, authority_lock,
							  &candidate, error);
			need(!code, code);
			if (sources)
				*sources = std::move(candidate);
		},
		error);
}

unsigned int flatfile_accounting_lifecycle_transaction::install(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &lock, const flatfile_accounting_lifecycle_request &request,
	const economic_account_key &opening_account, flatfile_accounting_lifecycle_receipt *receipt,
	std::string *error) noexcept
{
	return guarded(
		[&]
		{
			need(!root.empty() && identity_lock.matches(root) && lock.matches(root),
			     EINVAL);
			need(nonzero(request.operation_id), EINVAL);
			// Recover any previously committed original bundle under the exact
			// authority lock, then resolve immutable original-ID retry before
			// active-epoch rejection or recapturing mutable native holdings.
			flatfile_economic_control control;
			const auto control_code =
				flatfile_economic_control_read(root, lock, &control, error);
			need(!control_code, control_code);
			if (retry_retained_lifecycle(root, lock, request, opening_account, control,
						     receipt, error))
				return;
			need(nonzero(request.operation_id) && nonzero(request.lineage) &&
				     nonzero(request.epoch),
			     EINVAL);
			need(request.accepted_at_usec != 0, EINVAL);
			need(request.frozen_boundary_proven, EPERM);
			need(request.boundary_digest != economic_digest{}, EINVAL);
			need(economic_account_key_valid(opening_account), EINVAL);
			need(opening_account.kind == economic_account_kind::opening, EINVAL);
			need(opening_account.lineage.bytes == request.lineage.bytes, EINVAL);

			// Epoch activation waits for receipt and never_activated / virgin_state proof.
			need(request.virgin_state_proven, EPERM);

			need(control.lineage.bytes == request.lineage.bytes, ESTALE);
			// An initialized own transition without its lifecycle receipt is lost
			// completed history, not permission to regenerate mutable mappings.
			flatfile_economic_epoch previous_epoch;
			const auto previous_epoch_code = flatfile_economic_epoch_read(
				root, lock, request.lineage, request.epoch, &previous_epoch, error);
			need(!previous_epoch_code || previous_epoch_code == ENODATA,
			     previous_epoch_code);
			if (!previous_epoch_code &&
			    previous_epoch.creating_operation.bytes == request.operation_id.bytes &&
			    previous_epoch.baseline_initialization ==
				    flatfile_baseline_initialization::initialized)
				need(false, EILSEQ);

			// Never-activated invariant: active_epoch must not already be selected.
			need(!nonzero(control.active_epoch), EALREADY);
			// The shared receipt bucket must already be initialized. This owner
			// does not manufacture that independent storage/lifecycle prerequisite.
			critical_operation_id baseline_operation = {};
			need(critical_operation_id_derive(request.operation_id,
							  ECONOMIC_BASELINE_OPERATION_DOMAIN, 0,
							  &baseline_operation),
			     EINVAL);
			const size_t receipt_bucket = baseline_operation.bytes[0];
			need(control.evidence_initialized[receipt_bucket / 8] &
				     (1u << (receipt_bucket % 8)),
			     ENODATA);
			const auto bucket_status = flatfile_accounting_check_bucket(
				root, lock, request.lineage, receipt_bucket, error);
			need(bucket_status == flatfile_accounting_status::ok,
			     bucket_status == flatfile_accounting_status::capacity ? ENOMEM :
			     bucket_status == flatfile_accounting_status::io_error ? EIO :
										     EILSEQ);
			flatfile_accounting_lifecycle_native_sources sources;
			const auto capture_code = capture_native_sources_locked(
				root, identity_lock, lock, &sources, error);
			need(!capture_code, capture_code);
			// Complete physical room/saved-item census comes from the original
			// borrowed native provider, never from catalog membership alone.
			const auto piles = capture_room_pile_sources(root, identity_lock, lock,
								     request, baseline_operation,
								     sources, error);
			std::vector<flatfile_authority_operation> pile_operations;
			for (const auto &pile : piles)
			{
				const auto staged = flatfile_accounting_pile_state_stage_baseline(
					root, lock, pile.head, &pile_operations, error);
				need(staged == flatfile_accounting_status::ok,
				     staged == flatfile_accounting_status::io_error ? EIO : EILSEQ);
			}

			// Complete native wallet/shared-bank selection remains unchanged.
			std::set<uint32_t> seen_wallets;
			for (const auto &w : sources.wallets)
			{
				need(w.pid != 0, EINVAL);
				need(seen_wallets.insert(w.pid).second, EINVAL);
			}

			std::set<std::pair<std::string, uint8_t>> seen_banks;
			for (const auto &b : sources.banks)
			{
				need(!b.name.empty(), EINVAL);
				need(seen_banks.insert({ b.name, b.racewar }).second, EINVAL);
			}

			// Bind coverage digest.
			economic_digest coverage = compute_pile_coverage_digest(sources, piles);
			if (request.coverage_digest != economic_digest{})
			{
				need(coverage == request.coverage_digest, EINVAL);
			}

			// Reconcile shared-bank lifetimes one-to-one. Missing or duplicate
			// shared-bank lifetimes must stop baseline staging.
			std::vector<flatfile_economic_mapping> mappings;
			mappings.reserve(sources.wallets.size() + sources.banks.size());

			std::vector<flatfile_authority_operation> ops;
			flatfile_accounting_staging_view view(root, lock, ops);

			// Reconcile wallets.
			for (const auto &w : sources.wallets)
			{
				flatfile_economic_locator loc{ 1, w.pid, "" };
				flatfile_economic_mapping mapping;
				unsigned int code = flatfile_economic_native_lookup(
					root, lock, economic_account_kind::wallet, 0, loc, &mapping,
					error);
				if (!code)
				{
					flatfile_economic_mapping verified;
					unsigned int read_code = flatfile_economic_mapping_read(
						root, lock, mapping.account, &verified, error);
					need(!read_code, read_code);
					need(!nonzero(verified.retiring_operation), ESTALE);
					mappings.push_back(verified);
				}
				else if (code == ENODATA)
				{
					flatfile_economic_mapping created;
					unsigned int create_code =
						flatfile_accounting_authority_storage::
							create_mapping_staged(
								root, lock, control.revision,
								economic_account_kind::wallet, 0,
								loc, request.operation_id, &created,
								&ops, error, &view);
					need(!create_code, create_code);
					const auto staged_control = view.control(&control, error);
					need(!staged_control, staged_control);
					mappings.push_back(created);
				}
				else
				{
					need(false, code);
				}
			}

			// Reconcile shared banks: one-to-one shared-bank lifetime reconciliation.
			for (const auto &b : sources.banks)
			{
				flatfile_economic_locator loc{ 2, 0, b.name };
				flatfile_economic_mapping mapping;
				unsigned int code = flatfile_economic_native_lookup(
					root, lock, economic_account_kind::bank, b.racewar, loc,
					&mapping, error);
				if (!code)
				{
					flatfile_economic_mapping verified;
					unsigned int read_code = flatfile_economic_mapping_read(
						root, lock, mapping.account, &verified, error);
					need(!read_code, read_code);
					need(!nonzero(verified.retiring_operation), ESTALE);
					mappings.push_back(verified);
				}
				else if (code == ENODATA)
				{
					flatfile_economic_mapping created;
					unsigned int create_code =
						flatfile_accounting_authority_storage::
							create_mapping_staged(
								root, lock, control.revision,
								economic_account_kind::bank,
								b.racewar, loc,
								request.operation_id, &created,
								&ops, error, &view);
					need(!create_code, create_code);
					const auto staged_control = view.control(&control, error);
					need(!staged_control, staged_control);
					mappings.push_back(created);
				}
				else
				{
					need(false, code);
				}
			}

			// Membership must precede marker initialization in the staged view.
			flatfile_economic_epoch epoch_info;
			const auto epoch_read_code =
				view.epoch(request.lineage, request.epoch, &epoch_info);
			if (epoch_read_code == ENODATA)
			{
				flatfile_economic_epoch next_epoch{
					request.epoch,
					control.last_epoch,
					request.operation_id,
					uint64_t(control.epoch_count) + 1,
					1,
					coverage
				};
				const auto append_code =
					flatfile_accounting_authority_storage::append_epoch_staged(
						root, lock, control.revision, next_epoch, &ops,
						error, &view);
				need(!append_code, append_code);
				const auto staged_control = view.control(&control, error);
				need(!staged_control, staged_control);
			}
			else
			{
				need(!epoch_read_code, epoch_read_code);
				// This installer may resume only its own matching transition.
				// General baseline initialization retains separate-ID semantics.
				need(epoch_info.creating_operation.bytes ==
						     request.operation_id.bytes &&
					     epoch_info.transition_kind == 1 &&
					     epoch_info.transition_digest == coverage,
				     ESTALE);
			}
			// Selection targets the requested epoch, never a different catalog tail.
			need(control.last_epoch.bytes == request.epoch.bytes, ESTALE);
			// Initialize baseline storage namespace for this epoch.
			flatfile_accounting_status init_status =
				flatfile_accounting_baseline_storage::initialize_lifecycle_staged(
					root, lock, request.lineage, request.epoch, opening_account,
					request.operation_id, &ops, error, &view);
			if (init_status != flatfile_accounting_status::ok &&
			    init_status != flatfile_accounting_status::already_exists)
			{
				need(false, EIO);
			}
			const auto initialized_control = view.control(&control, error);
			need(!initialized_control, initialized_control);

			// Prepare and stage the baseline batch.
			economic_baseline_batch batch;
			batch.lineage = request.lineage;
			batch.epoch = request.epoch;
			batch.preparation_id = request.operation_id;
			batch.actor_id = request.actor_id;
			batch.batch_index = 0;
			batch.opening_account = opening_account;
			batch.coverage_digest = coverage;
			batch.boundary_digest = request.boundary_digest;

			for (size_t i = 0; i < mappings.size(); ++i)
			{
				economic_baseline_holding holding;
				holding.account = mappings[i].account;
				if (i < sources.wallets.size())
				{
					holding.balance = sources.wallets[i].balance;
					holding.native_revision =
						sources.wallets[i].native_revision;
					holding.source_digest = sources.wallets[i].source_digest;
				}
				else
				{
					size_t bank_idx = i - sources.wallets.size();
					holding.balance = sources.banks[bank_idx].balance;
					holding.native_revision =
						sources.banks[bank_idx].native_revision;
					holding.source_digest =
						sources.banks[bank_idx].source_digest;
				}
				batch.holdings.push_back(holding);
			}

			for (const auto &pile : piles)
			{
				batch.holdings.push_back(pile.holding);
				batch.items.push_back(pile.item);
			}
			std::optional<economic_prepared_baseline> prepared;
			economic_accounting_error prep_err =
				economic_baseline_prepare(batch, &prepared);
			need(prep_err == economic_accounting_error::ok && prepared.has_value(),
			     EINVAL);

			critical_command baseline_cmd;
			economic_accounting_error cmd_err = economic_baseline_command_build(
				*prepared, request.accepted_at_usec, &baseline_cmd);
			need(cmd_err == economic_accounting_error::ok, EINVAL);
			need(baseline_cmd.operation_id.bytes == baseline_operation.bytes);

			// Stage baseline reservations and witness.
			uint64_t verified_baseline_revision = 0;
			flatfile_accounting_status stage_status =
				flatfile_accounting_baseline_storage::stage_staged(
					root, lock, baseline_cmd, *prepared, &ops, error, &view,
					&verified_baseline_revision);
			if (stage_status != flatfile_accounting_status::ok &&
			    stage_status != flatfile_accounting_status::already_exists)
			{
				need(false, EIO);
			}
			need(verified_baseline_revision != 0, EILSEQ);

			// Receipt-bearing, never-activated-gated epoch selection.
			// Selection follows baseline receipts and external virgin-state proof.
			unsigned int select_code =
				flatfile_accounting_authority_storage::select_epoch_staged(
					root, lock, control.revision, true, request.operation_id,
					&ops, error, &view);
			need(!select_code, select_code);
			retained_lifecycle retained;
			retained.frame_version = 2;
			retained.request = request;
			retained.opening = opening_account;
			retained.lineage_creating_operation = control.creating_operation;
			const auto selected_control_code = view.control(&control, error);
			need(!selected_control_code, selected_control_code);
			need(control.active_epoch.bytes == request.epoch.bytes &&
			     control.last_epoch.bytes == request.epoch.bytes &&
			     control.last_operation.bytes == request.operation_id.bytes);
			retained.selected_control_revision = control.revision;
			const auto selected_epoch_code =
				view.epoch(request.lineage, request.epoch, &retained.epoch, error);
			need(!selected_epoch_code, selected_epoch_code);
			auto &candidate_receipt = retained.receipt;
			candidate_receipt.operation_id = request.operation_id;
			candidate_receipt.lineage = request.lineage;
			candidate_receipt.epoch = request.epoch;
			candidate_receipt.baseline_operation_id = baseline_cmd.operation_id;
			candidate_receipt.coverage_digest = coverage;
			candidate_receipt.boundary_digest = request.boundary_digest;
			candidate_receipt.baseline_revision = verified_baseline_revision;
			candidate_receipt.mappings = std::move(mappings);
			for (const auto &pile : piles)
				candidate_receipt.pile_uids.push_back(pile.item.snapshot.uid);
			retained.sources = std::move(sources);
			checked(critical_command_encode(baseline_cmd, &retained.baseline_command));
			checked(economic_baseline_encode(*prepared, &retained.baseline_witness));
			economic_accounting_plan baseline_plan;
			checked(economic_baseline_command_plan(baseline_cmd, *prepared,
							       &baseline_plan));
			checked(economic_plan_encode(baseline_plan, &retained.baseline_plan));
			auto encoded_receipt = encode_lifecycle_receipt(retained);
			const auto receipt_begin = view.begin(root, lock, &ops);
			need(!receipt_begin, receipt_begin);
			std::vector<flatfile_authority_operation> receipt_operations;
			receipt_operations.push_back({ flatfile_authority_store::economic_evidence,
						       flatfile_authority_operation_kind::write,
						       lifecycle_receipt_name(request.operation_id),
						       std::move(encoded_receipt) });
			const auto receipt_merge = view.merge(receipt_operations, &ops);
			need(!receipt_merge, receipt_merge);
			// Native .eph heads use their original codec, not the accounting
			// staging-view wrapper. Join them only after the final view merge;
			// the original single root commit publishes every image together.
			need(ops.size() <= flatfile_authority_transaction_maximum_operations &&
				     pile_operations.size() <=
					     flatfile_authority_transaction_maximum_operations -
						     ops.size(),
			     ENOSPC);
			ops.insert(ops.end(), std::make_move_iterator(pile_operations.begin()),
				   std::make_move_iterator(pile_operations.end()));
			static_assert(std::is_nothrow_move_assignable_v<
				      flatfile_accounting_lifecycle_receipt>);

			// Commit authority and accounting operations atomically.
			flatfile_authority_transaction_result commit_res =
				flatfile_accounting_storage::commit(root, lock, ops, error);
			need(commit_res == flatfile_authority_transaction_result::ok, EIO);

			// All receipt construction preceded durable commit. Only a nothrow
			// move remains; no later lookup or reconstruction can change success.
			if (receipt)
				*receipt = std::move(candidate_receipt);
		},
		error);
}

unsigned int flatfile_accounting_lifecycle_transaction::recover_runtime_locked(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &lock, bool *active_out, std::string *error) noexcept
{
	return guarded(
		[&]
		{
			need(active_out && !root.empty() && identity_lock.matches(root) &&
				     lock.matches(root),
			     EINVAL);
			// Trusted startup owns the verified cut. An already installed projection is
			// never replaced by this reader, including on any subsequent proof failure.
			need(!economic_gameplay_authority::active(), EALREADY);
			const auto recovered =
				flatfile_player_domain_recover_locked(root, lock, error);
			need(recovered == flatfile_player_domain_result::ok,
			     recovered == flatfile_player_domain_result::io_error ? EIO : EILSEQ);
			const auto gate = flatfile_economic_legacy_domain_gate(root, lock, nullptr);
			if (!gate)
			{
				*active_out = false;
				return;
			}
			// Only the existing gate's explicit selected-active refusal proceeds. Missing
			// or corrupt evidence is not inferred activation or initialization authority.
			need(gate == EAGAIN, gate);
			flatfile_economic_control control;
			auto code = flatfile_economic_control_read(root, lock, &control, error);
			need(!code, code);
			need(nonzero(control.active_epoch));
			flatfile_economic_epoch epoch;
			code = flatfile_economic_epoch_read(root, lock, control.lineage,
							    control.active_epoch, &epoch, error);
			need(!code, code);
			// Only catalog v3 can provide this origin. v1/v2 stay unknown even if an
			// unrelated lifecycle receipt happens to survive beside their book.
			need(epoch.baseline_initialization ==
				     flatfile_baseline_initialization::initialized &&
			     epoch.initialization_origin ==
				     flatfile_baseline_initialization_origin::lifecycle_owner &&
			     nonzero(epoch.baseline_initializing_operation));
			retained_lifecycle retained;
			need(read_retained_lifecycle(root, epoch.baseline_initializing_operation,
						     &retained, error));
			need(retained.request.lineage.bytes == control.lineage.bytes &&
			     retained.request.epoch.bytes == control.active_epoch.bytes &&
			     economic_account_key_equal(retained.opening, epoch.baseline_opening));
			// Consume the decoded original request, including its retained assertions;
			// never fabricate current request flags, descriptors or opening holdings.
			verify_retained_lifecycle(root, lock, retained, control, error);
			const auto structure =
				flatfile_accounting_baseline_storage::verify_structure_locked(
					root, lock, control.lineage, control.active_epoch,
					epoch.baseline_opening, error);
			need(structure == flatfile_accounting_status::ok,
			     structure == flatfile_accounting_status::capacity ? ENOMEM :
			     structure == flatfile_accounting_status::io_error ? EIO :
										 EILSEQ);
			std::vector<flatfile_economic_mapping> mappings;
			code = flatfile_accounting_authority_storage::read_all_mappings_locked(
				root, lock, &mappings, error);
			need(!code, code);
			flatfile_accounting_lifecycle_native_sources sources;
			code = capture_sources(root, identity_lock, lock, &sources, error);
			need(!code, code);
			std::map<uint32_t, const flatfile_accounting_lifecycle_wallet_source *>
				wallets;
			std::map<std::pair<std::string, uint8_t>,
				 const flatfile_accounting_lifecycle_bank_source *>
				banks;
			for (const auto &wallet : sources.wallets)
				need(wallets.emplace(wallet.pid, &wallet).second);
			for (const auto &bank : sources.banks)
				need(banks.emplace(std::make_pair(bank.name, bank.racewar), &bank)
					     .second);
			std::vector<economic_gameplay_wallet_mapping> wallet_projection;
			std::vector<economic_gameplay_bank_mapping> bank_projection;
			wallet_projection.reserve(wallets.size());
			bank_projection.reserve(banks.size());
			// Current aliases and lifetimes, including mappings created after the original
			// baseline, come only from the complete current authority/native census.
			// Original receipt names, balances and source digests are historical proof.
			for (const auto &mapping : mappings)
			{
				if (nonzero(mapping.retiring_operation))
					continue;
				if (mapping.account.kind == economic_account_kind::wallet)
				{
					need(mapping.locator.native_id <= INT32_MAX &&
					     mapping.account.context_id == 0);
					const auto pid =
						static_cast<uint32_t>(mapping.locator.native_id);
					const auto at = wallets.find(pid);
					need(at != wallets.end());
					wallet_projection.push_back({ pid, mapping.account });
					wallets.erase(at);
				}
				else
				{
					need(mapping.account.kind == economic_account_kind::bank &&
					     mapping.account.context_id <= INT8_MAX);
					const auto racewar =
						static_cast<uint8_t>(mapping.account.context_id);
					const auto at =
						banks.find({ mapping.locator.name, racewar });
					need(at != banks.end());
					bank_projection.push_back(
						{ mapping.locator.name, racewar, mapping.account });
					banks.erase(at);
				}
			}
			need(wallets.empty() && banks.empty());
			const auto installed = economic_gameplay_authority::install(
				control.lineage, control.active_epoch,
				retained.receipt.baseline_operation_id, wallet_projection,
				bank_projection);
			need(installed == economic_accounting_error::ok,
			     installed == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
			// Publication is the final potentially allocating operation; no later failure
			// can leave a published projection paired with a reported unsuccessful boot.
			*active_out = true;
		},
		error);
}
