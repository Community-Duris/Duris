#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_native_command_context.h"
#include <openssl/sha.h>
#include <unordered_set>

#include <algorithm>
#include <climits>
#include <new>
#include <utility>

namespace
{
using error = economic_accounting_error;

constexpr size_t native_fact_extension_bytes = 124;
constexpr uint32_t native_fact_magic = 0x32464e41; // ANF2

template <class T> void native_fact_append(std::vector<uint8_t> &bytes, T value)
{
	for (size_t i = 0; i < sizeof(T); ++i)
		bytes.push_back(static_cast<uint8_t>(static_cast<uint64_t>(value) >> (8 * i)));
}
template <class T> bool native_fact_read(std::span<const uint8_t> bytes, size_t &at, T *value)
{
	if (at > bytes.size() || sizeof(T) > bytes.size() - at)
		return false;
	uint64_t number = 0;
	for (size_t i = 0; i < sizeof(T); ++i)
		number |= uint64_t(bytes[at++]) << (8 * i);
	*value = static_cast<T>(number);
	return true;
}

error append_native_facts(const critical_command &command, std::vector<uint8_t> *facts)
{
	if (command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
		return error::ok;
	auction_native_command_context native;
	if (auction_native_command_decode(command, &native) != error::ok)
		return error::corrupt_evidence;
	if (native.selected_node_count > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS)
		return error::capacity;
	native_fact_append(*facts, native_fact_magic);
	native_fact_append(*facts, uint16_t{ 2 });
	native_fact_append(*facts, uint16_t{ 0 });
	native_fact_append(*facts, native.original_level);
	native_fact_append(*facts, native.acknowledged_save_revision);
	for (const auto *hash :
	     { &native.before_digest, &native.after_digest, &native.selected_digest })
		facts->insert(facts->end(), hash->begin(), hash->end());
	native_fact_append(*facts, native.selected_node_count);
	native_fact_append(*facts, native.selected_root_count);
	native_fact_append(*facts, uint16_t{ 0 });
	return error::ok;
}

error observe_native_facts(std::span<const uint8_t> extension, auction_accounting_native_facts *out)
{
	if (!out || extension.size() != native_fact_extension_bytes)
		return error::corrupt_evidence;
	size_t at = 0;
	uint32_t magic = 0;
	uint16_t version = 0, reserved = 0;
	auction_accounting_native_facts value;
	if (!native_fact_read(extension, at, &magic) || magic != native_fact_magic ||
	    !native_fact_read(extension, at, &version) || version != 2 ||
	    !native_fact_read(extension, at, &reserved) || reserved ||
	    !native_fact_read(extension, at, &value.original_level) ||
	    !native_fact_read(extension, at, &value.acknowledged_save_revision))
		return error::corrupt_evidence;
	for (auto *hash : { &value.before_digest, &value.after_digest, &value.selected_digest })
	{
		if (extension.size() - at < hash->size())
			return error::corrupt_evidence;
		std::copy_n(extension.begin() + at, hash->size(), hash->begin());
		at += hash->size();
	}
	if (!native_fact_read(extension, at, &value.selected_node_count) ||
	    !native_fact_read(extension, at, &value.selected_root_count) ||
	    !native_fact_read(extension, at, &reserved) || reserved || at != extension.size())
		return error::corrupt_evidence;
	if (!value.original_level || value.original_level > 255 ||
	    !value.acknowledged_save_revision || !value.selected_root_count ||
	    value.selected_root_count > AUCTION_COMMAND_MAX_ITEMS ||
	    value.selected_node_count < value.selected_root_count ||
	    value.selected_node_count > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return error::invalid_identity;
	*out = value;
	return error::ok;
}

bool decode_payload(const critical_command &command, auction_command_payload *out)
{
	if (command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
		return auction_command_decode_payload(command, out);
	auction_native_command_context context;
	if (auction_native_command_decode(command, &context) != error::ok)
		return false;
	*out = context.payload;
	return true;
}

// Original supplied literal values have no authority by themselves. Every node
// is bound by the native command digest/topology and its actual command UID fence.
error native_items(const critical_command &command, const auction_command_payload &payload,
		   std::span<const player_item_snapshot> literals,
		   std::span<const economic_item_snapshot> before, item_owner_type owner_type,
		   uint64_t owner_id, std::vector<size_t> *root_indices)
{
	auction_native_command_context native;
	if (auction_native_command_decode(command, &native) != error::ok)
		return error::corrupt_evidence;
	if (literals.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
		return error::capacity;
	if (literals.size() != native.selected_node_count || before.size() != literals.size())
		return error::stale_revision;
	std::vector<uint8_t> encoded;
	std::array<uint8_t, 32> hash{};
	if (player_item_snapshot_list_encode(std::vector<player_item_snapshot>(literals.begin(),
									       literals.end()),
					     &encoded) != player_snapshot_codec_result::ok ||
	    !SHA256(encoded.data(), encoded.size(), hash.data()) || hash != native.selected_digest)
		return error::corrupt_evidence;
	std::unordered_set<uint64_t> unique;
	std::array<size_t, PLAYER_SNAPSHOT_MAX_DEPTH> ancestors{};
	size_t depth = 0, root = 0;
	std::vector<size_t> roots;
	for (size_t i = 0; i < literals.size(); ++i)
	{
		const auto &literal = literals[i];
		const auto &item = before[i];
		const auto &position = item.position;
		if (!literal.object_uid || literal.object_uid == UINT64_MAX ||
		    literal.string_mask != 15 || literal.equipment_slot ||
		    !unique.insert(literal.object_uid).second)
			return error::corrupt_evidence;
		if (literal.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			size_t slot = roots.size();
			if (slot >= payload.item_count ||
			    literal.object_uid != payload.items[slot].item_uid ||
			    literal.vnum != payload.items[slot].vnum)
				return error::corrupt_evidence;
			roots.push_back(i);
			root = i;
			depth = 1;
			ancestors[0] = i;
		}
		else
		{
			if (literal.parent_index < 0 ||
			    static_cast<size_t>(literal.parent_index) >= i)
				return error::topology;
			while (depth &&
			       ancestors[depth - 1] != static_cast<size_t>(literal.parent_index))
				--depth;
			if (!depth || depth == ancestors.size())
				return error::topology;
			ancestors[depth++] = i;
		}
		const uint64_t parent =
			literal.parent_index < 0 ? 0 : literals[literal.parent_index].object_uid;
		if (item.uid != literal.object_uid || position.owner.type != owner_type ||
		    position.owner.id != owner_id || position.owner.context_id ||
		    position.root_uid != literals[root].object_uid ||
		    position.parent_uid != parent || position.equipment_slot ||
		    position.state != item_custody_state::active || position.revision == UINT64_MAX)
			return error::stale_revision;
		size_t key_count = 0, fence_count = 0;
		for (const auto &key : command.keys)
			if (key.type == critical_entity_type::item && key.id == item.uid)
				++key_count;
		for (const auto &fence : command.expected_revisions)
			if (fence.key.type == critical_entity_type::item &&
			    fence.key.id == item.uid)
			{
				++fence_count;
				if (fence.revision != position.revision)
					return error::stale_revision;
			}
		if (key_count != 1 || fence_count != 1)
			return error::unauthorized;
		if (i == root &&
		    position.revision != payload.items[roots.size() - 1].expected_item_revision)
			return error::stale_revision;
	}
	if (roots.size() != payload.item_count || roots.size() != native.selected_root_count)
		return error::corrupt_evidence;
	*root_indices = std::move(roots);
	return error::ok;
}

constexpr size_t claim_fact_header_bytes = 82;
constexpr size_t claim_fact_row_bytes = 27;

template <typename T> T read_number(std::span<const uint8_t> bytes, size_t offset)
{
	T value = 0;
	for (size_t index = 0; index < sizeof(T); ++index)
		value |= static_cast<T>(bytes[offset + index]) << (8 * index);
	return value;
}

void append_u64(std::vector<uint8_t> *bytes, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

void append_u32(std::vector<uint8_t> *bytes, uint32_t value)
{
	for (size_t index = 0; index < 4; ++index)
		bytes->push_back(static_cast<uint8_t>(value >> (index * 8)));
}

void append_u16(std::vector<uint8_t> *bytes, uint16_t value)
{
	bytes->push_back(static_cast<uint8_t>(value));
	bytes->push_back(static_cast<uint8_t>(value >> 8));
}

bool valid_accounts(const economic_account_key &wallet, const economic_account_key &bank,
		    uint8_t racewar)
{
	return economic_account_key_valid(wallet) && economic_account_key_valid(bank) &&
	       wallet.kind == economic_account_kind::wallet && !wallet.context_id &&
	       bank.kind == economic_account_kind::bank && bank.context_id == racewar &&
	       wallet.lineage.bytes == bank.lineage.bytes &&
	       wallet.authority_id != bank.authority_id;
}

bool valid_claim(const auction_command_payload &payload, const auction_item_claim_state &claim)
{
	if (payload.action != auction_action::claim_item || !payload.actor_pid ||
	    !payload.item_count || claim.auction_id != payload.auction_id || !claim.auction_id ||
	    !claim.seller_pid || claim.claimant_pid != payload.actor_pid ||
	    (claim.status != 2 && claim.status != 3) || claim.custody_state != 1 ||
	    !claim.auction_revision || claim.auction_revision == UINT64_MAX ||
	    claim.item_count != payload.item_count ||
	    critical_operation_id_is_zero(claim.listing_operation) ||
	    critical_operation_id_is_zero(claim.claim_source_operation) ||
	    critical_operation_id_equal(claim.listing_operation, claim.claim_source_operation))
		return false;
	const uint32_t staged_claimant = claim.status == 3 || !claim.winner_pid ? claim.seller_pid :
										  claim.winner_pid;
	if (claim.claimant_pid != staged_claimant)
		return false;
	for (size_t index = 0; index < claim.item_count; ++index)
	{
		const auto &row = claim.rows[index];
		const auto &item = payload.items[index];
		if (!row.uid || row.uid != item.item_uid ||
		    row.revision != item.expected_item_revision || row.revision == UINT64_MAX ||
		    row.vnum != item.vnum || row.claim_pid != payload.actor_pid || row.claimed)
			return false;
		for (size_t previous = 0; previous < index; ++previous)
			if (claim.rows[previous].uid == row.uid ||
			    claim.rows[previous].slot == row.slot)
				return false;
	}
	return true;
}

std::vector<uint8_t> frozen_facts(const economic_account_key &wallet,
				  const economic_account_key &bank,
				  const auction_item_claim_state &claim)
{
	std::vector<uint8_t> facts;
	facts.reserve(82 + claim.item_count * 27);
	append_u64(&facts, wallet.authority_id);
	append_u64(&facts, bank.authority_id);
	append_u32(&facts, claim.auction_id);
	append_u32(&facts, claim.seller_pid);
	append_u32(&facts, claim.winner_pid);
	append_u32(&facts, claim.claimant_pid);
	append_u32(&facts, claim.status);
	append_u32(&facts, claim.custody_state);
	append_u64(&facts, claim.auction_revision);
	facts.insert(facts.end(), claim.listing_operation.bytes.begin(),
		     claim.listing_operation.bytes.end());
	facts.insert(facts.end(), claim.claim_source_operation.bytes.begin(),
		     claim.claim_source_operation.bytes.end());
	append_u16(&facts, claim.item_count);
	for (size_t index = 0; index < claim.item_count; ++index)
	{
		const auto &row = claim.rows[index];
		append_u64(&facts, row.uid);
		append_u64(&facts, row.revision);
		append_u16(&facts, row.slot);
		append_u32(&facts, static_cast<uint32_t>(row.vnum));
		append_u32(&facts, row.claim_pid);
		facts.push_back(row.claimed ? 1 : 0);
	}
	return facts;
}

economic_source_event source_for(const auction_item_claim_state &claim)
{
	return { economic_source_kind::auction, claim.claim_source_operation,
		 claim.listing_operation, claim.auction_revision, 0 };
}
} // namespace

economic_accounting_error
auction_item_claim_accounting_observe_native_facts(const economic_frozen_intent &intent,
						   auction_accounting_native_facts *out) noexcept
{
	const auto &facts = intent.admission.facts;
	if (!out || intent.admission.facts_version != 1 ||
	    intent.admission.metadata.writer_id != ECONOMIC_WRITER_AUCTION_ITEM_CLAIM ||
	    intent.admission.metadata.reason != economic_reason::auction_claim)
		return error::invalid_identity;
	if (facts.size() < claim_fact_header_bytes)
		return error::corrupt_evidence;
	const auto count = read_number<uint16_t>(facts, 80);
	if (!count || count > AUCTION_COMMAND_MAX_ITEMS)
		return error::corrupt_evidence;
	const size_t base = claim_fact_header_bytes + count * claim_fact_row_bytes;
	if (facts.size() == base)
		return error::invalid_version;
	if (facts.size() != base + native_fact_extension_bytes)
		return error::corrupt_evidence;
	auction_accounting_native_facts value;
	const auto status =
		observe_native_facts(std::span<const uint8_t>(facts).subspan(base), &value);
	if (status != error::ok)
		return status;
	if (value.selected_root_count != count)
		return error::corrupt_evidence;
	*out = value;
	return error::ok;
}

economic_accounting_error auction_item_claim_accounting_intent(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const auction_item_claim_state &claim, std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!decode_payload(command, &payload))
		return error::corrupt_evidence;
	if (!valid_accounts(wallet, bank, payload.racewar) || !valid_claim(payload, claim) ||
	    critical_operation_id_equal(command.operation_id, claim.listing_operation) ||
	    critical_operation_id_equal(command.operation_id, claim.claim_source_operation))
		return error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.original_operation_id = claim.listing_operation;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_AUCTION_ITEM_CLAIM;
		facts.metadata.reason = economic_reason::auction_claim;
		facts.metadata.source_event = source_for(claim);
		facts.facts = frozen_facts(wallet, bank, claim);
		const auto native_status = append_native_facts(command, &facts.facts);
		if (native_status != error::ok)
			return native_status;
		return economic_intent_freeze(command, facts, encoded);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_item_claim_accounting_decode(const critical_command &command,
							       economic_frozen_intent *intent,
							       auction_command_payload *payload,
							       auction_item_claim_state *claim,
							       economic_account_key *wallet,
							       economic_account_key *bank)
{
	if (!intent || !payload || !claim || !wallet || !bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!decode_payload(command, &parsed_payload) ||
		    parsed_payload.action != auction_action::claim_item)
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() < claim_fact_header_bytes)
			return error::invalid_identity;
		const uint16_t count = read_number<uint16_t>(facts, 80);
		if (!count || count > AUCTION_COMMAND_MAX_ITEMS ||
		    facts.size() !=
			    claim_fact_header_bytes + count * claim_fact_row_bytes +
				    (command.payload_version ==
						     AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ?
					     native_fact_extension_bytes :
					     0))
			return error::invalid_identity;
		const auto &lineage = parsed_intent.admission.metadata.lineage;
		const economic_account_key parsed_wallet = { lineage, economic_account_kind::wallet,
							     read_number<uint64_t>(facts, 0), 0 };
		const economic_account_key parsed_bank = { lineage, economic_account_kind::bank,
							   read_number<uint64_t>(facts, 8),
							   parsed_payload.racewar };
		auction_item_claim_state parsed_claim;
		parsed_claim.auction_id = read_number<uint32_t>(facts, 16);
		parsed_claim.seller_pid = read_number<uint32_t>(facts, 20);
		parsed_claim.winner_pid = read_number<uint32_t>(facts, 24);
		parsed_claim.claimant_pid = read_number<uint32_t>(facts, 28);
		parsed_claim.status = read_number<uint32_t>(facts, 32);
		parsed_claim.custody_state = read_number<uint32_t>(facts, 36);
		parsed_claim.auction_revision = read_number<uint64_t>(facts, 40);
		std::copy_n(facts.begin() + 48, 16, parsed_claim.listing_operation.bytes.begin());
		std::copy_n(facts.begin() + 64, 16,
			    parsed_claim.claim_source_operation.bytes.begin());
		parsed_claim.item_count = count;
		for (size_t index = 0; index < count; ++index)
		{
			const size_t offset =
				claim_fact_header_bytes + index * claim_fact_row_bytes;
			auto &entry = parsed_claim.rows[index];
			entry.uid = read_number<uint64_t>(facts, offset);
			entry.revision = read_number<uint64_t>(facts, offset + 8);
			entry.slot = read_number<uint16_t>(facts, offset + 16);
			entry.vnum =
				static_cast<int32_t>(read_number<uint32_t>(facts, offset + 18));
			entry.claim_pid = read_number<uint32_t>(facts, offset + 22);
			if (facts[offset + 26] > 1)
				return error::invalid_identity;
			entry.claimed = facts[offset + 26] == 1;
		}
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		const auto frozen = auction_item_claim_accounting_intent(
			projected, parsed_intent.admission.metadata.epoch, parsed_wallet,
			parsed_bank, parsed_claim, &expected);
		if (frozen != error::ok || expected != command.accounting_intent)
			return error::unauthorized;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*claim = parsed_claim;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_item_claim_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_item_claim_accounting_authority &authority,
	const auction_command_result &result, economic_accounting_plan *plan)
{
	if (!plan)
		return error::corrupt_evidence;
	try
	{
		auto status = economic_intent_verify_binding(command, intent);
		if (status != error::ok)
			return status;
		auction_command_payload payload = {};
		if (!decode_payload(command, &payload))
			return error::corrupt_evidence;
		const auto &claim = authority.claim;
		if (!valid_accounts(authority.wallet_account, authority.bank_account,
				    payload.racewar) ||
		    !valid_claim(payload, claim))
			return error::invalid_identity;
		const auto &meta = intent.admission.metadata;
		const auto source = source_for(claim);
		auto expected_facts =
			frozen_facts(authority.wallet_account, authority.bank_account, claim);
		status = append_native_facts(command, &expected_facts);
		if (status != error::ok)
			return status;
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_ITEM_CLAIM ||
		    meta.reason != economic_reason::auction_claim ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != payload.actor_pid || !meta.source_event ||
		    meta.source_event->kind != source.kind ||
		    meta.source_event->source.bytes != source.source.bytes ||
		    meta.source_event->generation.bytes != source.generation.bytes ||
		    meta.source_event->sequence != source.sequence ||
		    meta.source_event->slot != source.slot ||
		    meta.lineage.bytes != authority.wallet_account.lineage.bytes ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    meta.original_operation_id.bytes != claim.listing_operation.bytes ||
		    intent.admission.facts != expected_facts)
			return error::unauthorized;
		if (authority.items_before.size() !=
			    (command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ?
				     authority.native_selected_literals.size() :
				     claim.item_count) ||
		    authority.player_owner_revision_before == UINT64_MAX ||
		    authority.auction_owner_revision_before == UINT64_MAX ||
		    authority.balances_before.wallet_revision != payload.expected_wallet_revision ||
		    authority.balances_before.bank_revision != payload.expected_bank_revision)
			return error::stale_revision;
		if (result.action != auction_action::claim_item ||
		    result.event_type != auction_event_type::item_claimed ||
		    result.auction_id != claim.auction_id || result.status != claim.status ||
		    result.seller_pid != claim.seller_pid ||
		    result.winner_pid != payload.actor_pid || result.previous_bidder_pid ||
		    result.final_price || result.wallet_value_delta ||
		    result.wallet.amount != authority.balances_before.wallet.amount ||
		    result.bank.amount != authority.balances_before.bank.amount ||
		    result.wallet_revision != authority.balances_before.wallet_revision ||
		    result.bank_revision != authority.balances_before.bank_revision ||
		    result.auction_revision != claim.auction_revision + 1 ||
		    result.player_owner_revision != authority.player_owner_revision_before + 1 ||
		    result.auction_owner_revision != authority.auction_owner_revision_before + 1 ||
		    result.item_count != claim.item_count)
			return error::corrupt_evidence;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;

		if (command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
		{
			std::vector<size_t> roots;
			status = native_items(command, payload, authority.native_selected_literals,
					      authority.items_before, item_owner_type::auction,
					      claim.auction_id, &roots);
			if (status != error::ok)
				return status;
			for (size_t slot = 0; slot < roots.size(); ++slot)
			{
				const auto &item = authority.items_before[roots[slot]];
				if (result.item_uids[slot] != item.uid ||
				    result.item_revisions[slot] != item.position.revision + 1)
					return error::corrupt_evidence;
			}
			for (size_t i = 0; i < authority.items_before.size(); ++i)
			{
				const auto &item = authority.items_before[i];
				economic_item_position after = item.position;
				after.owner = { item_owner_type::player, payload.actor_pid, 0 };
				after.revision++;
				candidate.items_before.push_back(item);
				candidate.items_after.push_back({ item.uid, after });
				candidate.item_events.push_back({ static_cast<uint32_t>(i), 0,
								  item.uid, item.position, after });
			}
		}
		else
		{
			for (size_t index = 0; index < claim.item_count; ++index)
			{
				const auto &item = authority.items_before[index];
				const auto &position = item.position;
				if (item.uid != payload.items[index].item_uid ||
				    position.owner.type != item_owner_type::auction ||
				    position.owner.id != claim.auction_id ||
				    position.owner.context_id || position.root_uid != item.uid ||
				    position.parent_uid ||
				    position.revision !=
					    payload.items[index].expected_item_revision ||
				    position.state != item_custody_state::active)
					return error::stale_revision;
				if (result.item_uids[index] != item.uid ||
				    result.item_revisions[index] != position.revision + 1)
					return error::corrupt_evidence;
				economic_item_position after = position;
				after.owner = { item_owner_type::player, payload.actor_pid, 0 };
				after.revision++;
				candidate.items_before.push_back(item);
				candidate.items_after.push_back({ item.uid, after });
				candidate.item_events.push_back({ static_cast<uint32_t>(index), 0,
								  item.uid, position, after });
			}
		}
		for (size_t index = claim.item_count; index < AUCTION_COMMAND_MAX_ITEMS; ++index)
			if (result.item_uids[index] || result.item_revisions[index])
				return error::corrupt_evidence;
		status = economic_plan_normalize(&candidate);
		if (status != error::ok)
			return status;
		*plan = std::move(candidate);
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

#include "economy/auction_native_command_context.h"

#include <type_traits>
namespace
{
constexpr size_t auction_codec_allocator_frames =
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
constexpr size_t auction_codec_copy_frames =
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
constexpr size_t auction_codec_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t auction_codec_default_frames =
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
constexpr size_t auction_codec_vector_frames =
	auction_codec_allocator_frames + auction_codec_copy_frames + auction_codec_relocate_frames +
	auction_codec_default_frames +
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
constexpr size_t auction_codec_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + auction_codec_allocator_frames;
constexpr size_t auction_codec_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + auction_codec_vector_frames;

// Genuine additional selected typed library scopes beside the vector's
// reserve/forward-insert profile. The owning inline DTOs remain in the actual
// enclosing object sizes, rather than a fabricated encoded-envelope baseline.
static_assert(std::is_trivially_copyable_v<economic_source_event>);
static_assert(std::is_trivially_destructible_v<economic_source_event>);
static_assert(std::is_trivially_copyable_v<auction_command_payload>);
static_assert(std::is_trivially_copyable_v<economic_account_key>);
constexpr size_t auction_codec_optional_frames =
	// Actual metadata/frozen/admission default/generated move/copy member
	// functions: this/source refs; optional/_Optional_base/_payload/_Storage
	// default constructors and trivial storage destructor this carriers.
	6 * sizeof(void *) + 5 * sizeof(void *) + sizeof(void *) +
	// source_event assignment operator=(T&&): this/u/ref-result; real
	// is_engaged/get/construct wrappers, payload _M_construct and forward.
	3 * sizeof(void *) + (sizeof(void *) + sizeof(bool)) + 2 * (2 * sizeof(void *)) +
	2 * sizeof(void *) + 2 * sizeof(void *) +
	// __addressof -> _Construct -> forward -> placement new -> trivial
	// economic_source_event generated move this/source. No extra DTO copy.
	2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	sizeof(size_t) + sizeof(void *) + 2 * sizeof(void *) +
	// Actual optional operator bool/operator->/base get/payload get and
	// addressof parameter and reference/pointer/bool result carriers.
	2 * (sizeof(void *) + sizeof(bool)) + 3 * (2 * sizeof(void *));
constexpr size_t auction_codec_equal_frames =
	// array/vector operator== actual lhs/rhs and returned bool; genuine
	// container size/begin/end and array_traits::_S_ptr pointer returns.
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	6 * (2 * sizeof(void *)) +
	// equal/__equal_aux/__equal_aux1/__equal<true>::equal each three
	// iterator arguments and returned bool; __simple and __len locals;
	// niter_base calls and __memcmp's genuine pointers/length/int result.
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t) +
	3 * (2 * sizeof(void *)) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int) +
	// Actual normal_iterator copied argument/ctor/base source carriers.
	4 * (2 * sizeof(void *));
constexpr size_t auction_codec_copy_n_frames =
	// Original copy_n(count literal16) owns first/count/result/__n2/result,
	// __size_to_integer(int), iterator_category and __copy_n<RA> tag.
	3 * sizeof(void *) + 2 * sizeof(int) + 2 * sizeof(int) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + 3 * sizeof(void *) + sizeof(int) +
	sizeof(std::random_access_iterator_tag);
constexpr size_t auction_codec_scalar_source_frames =
	// Original read lambda (facts-reference capture/this, offset,width,
	// byte index,value/result); typed read_number alternatives; span data,
	// index,size/constructor parameters and returned pointer/reference.
	2 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	sizeof(std::span<const uint8_t>) + 2 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	4 * (sizeof(void *) + sizeof(size_t) + sizeof(void *)) +
	// Actual account/empty/key_valid/kind_valid helper params/results,
	// operation_id_equal two refs/result and zero's byte range loop.
	3 * sizeof(void *) + sizeof(economic_account_kind) + sizeof(uint64_t) + sizeof(bool) +
	2 * (sizeof(void *) + sizeof(bool)) + sizeof(economic_account_kind) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(bool) + 4 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool) +
	// Original append_u64/u32/u16 and native_fact_append<T> pointer/value/
	// index/byte result lifetimes, plus initializer-list begin/end/size.
	4 * (sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(uint8_t)) +
	3 * (sizeof(void *) + sizeof(void *)) + sizeof(std::initializer_list<uint64_t>) +
	// Original metadata assignment generated function this/source and the
	// returned source_for event temporary, optional typed path above.
	2 * sizeof(void *) + sizeof(economic_source_event) + auction_codec_optional_frames +
	auction_codec_equal_frames + auction_codec_copy_n_frames;
bool auction_codec_add(size_t &total, size_t value) noexcept
{
	if (value > SIZE_MAX - total)
		return false;
	total += value;
	return true;
}
struct auction_codec_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	size_t native_frames = 0;
	size_t payload_frames = 0;
	const economic_frozen_intent *intent = nullptr;
	const economic_admission_facts *admission = nullptr;
	const critical_command *projection = nullptr;
	const std::vector<uint8_t> *expected = nullptr;
	const auction_native_command_context *native = nullptr;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &self = *static_cast<auction_codec_budget *>(opaque);
		if (self.denied || !self.reserve || !self.reserve(amount, self.context))
		{
			self.denied = true;
			return false;
		}
		return true;
	}
	bool prefix(size_t &value, size_t extra = 0) noexcept
	{
		value = outer;
		size_t heap = 0;
		// Genuine observer/relay/prefix/checked-add parameter, local and result
		// lifetimes, plus retained-vector public capacity/size/data carriers.
		constexpr size_t observer_frames = 13 * sizeof(void *) + 10 * sizeof(size_t) +
						   6 * sizeof(bool) +
						   8 * (sizeof(void *) + sizeof(size_t));
		if (!auction_codec_add(value, sizeof(*this)) || !auction_codec_add(value, frames) ||
		    !auction_codec_add(value, native_frames) ||
		    !auction_codec_add(value, payload_frames) ||
		    !auction_codec_add(value, observer_frames) ||
		    (intent && !auction_codec_add(value, intent->admission.facts.capacity())) ||
		    (admission && !auction_codec_add(value, admission->facts.capacity())) ||
		    (expected && !auction_codec_add(value, expected->capacity())) ||
		    (projection && (!critical_command_current_heap_bytes(*projection, &heap) ||
				    !auction_codec_add(value, heap))) ||
		    (native && (native->before_item_uids.capacity() > SIZE_MAX / sizeof(uint64_t) ||
				!auction_codec_add(value, native->base_v1_payload.capacity()) ||
				!auction_codec_add(value, native->before_item_uids.capacity() *
								  sizeof(uint64_t)))) ||
		    !auction_codec_add(value, extra))
		{
			denied = true;
			return false;
		}
		return true;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t value = 0;
		return prefix(value, extra) && forward(value, this);
	}
};
// Runtime non-debug C++20 GCC13/C++11 ABI byte-vector growth law. These are
// source-level requested payload bytes; emitted/libc/native qualification is
// separate. Refuse unsupported policy before any fallible private operation.
bool auction_codec_policy() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
	return sizeof(void *) == 8 && sizeof(size_t) == 8;
#else
	return false;
#endif
}
bool auction_codec_growth_peak(size_t size, size_t capacity, size_t count, size_t &request) noexcept
{
	if (count > SIZE_MAX - size)
		return false;
	const size_t required = size + count;
	if (required <= capacity)
	{
		request = 0;
		return true;
	}
	request = size;
	const size_t added = size > count ? size : count;
	return auction_codec_add(request, added);
}
[[maybe_unused]] bool auction_codec_native_extension_peak(const std::vector<uint8_t> &facts,
							  auction_codec_budget &budget) noexcept
{
	size_t size = facts.size(), capacity = facts.capacity(), largest = 0, request = 0;
	// Exact original ANF2 append sequence: integer bytes use individual
	// push_back; each digest uses one forward insert of 32 actual bytes.
	constexpr size_t runs[] = { 4, 2, 2, 4, 8, 32, 32, 32, 4, 2, 2 };
	for (size_t run = 0; run < std::size(runs); ++run)
	{
		const bool block = run >= 5 && run <= 7;
		const size_t count = block ? runs[run] : 1;
		const size_t iterations = block ? 1 : runs[run];
		for (size_t i = 0; i < iterations; ++i)
		{
			if (!auction_codec_growth_peak(size, capacity, count, request))
				return false;
			if (request)
			{
				size_t simultaneous = capacity;
				if (!auction_codec_add(simultaneous, request))
					return false;
				if (simultaneous > largest)
					largest = simultaneous;
				capacity = request;
			}
			if (!auction_codec_add(size, count))
				return false;
		}
	}
	// prefix already owns the actual old facts allocation; replace that term
	// by the largest authentic simultaneous old/new growth request.
	const size_t extra = largest > facts.capacity() ? largest - facts.capacity() : 0;
	// This forecast's actual automatic runs[11], size/capacity/largest/request,
	// run/count/iterations/i and growth helper required/added/ref arguments
	// remain live during its reserve callback. They are source owners, not a
	// reserve-capacity multiplier or a claim about emitted stack bytes.
	constexpr size_t forecast_frames =
		11 * sizeof(size_t) + 10 * sizeof(size_t) + 3 * sizeof(bool) + 6 * sizeof(void *);
	size_t peak_extra = extra;
	return auction_codec_add(peak_extra, forecast_frames) && budget.peak(peak_extra);
}
bool auction_codec_payload(const critical_command &command, auction_command_payload *out,
			   auction_codec_budget &budget, bool native_allowed) noexcept
{
	size_t nested = 0;
	// Keep this genuine helper's params/locals/result carriers live across
	// every nested absolute callback, including the original nonnative path.
	budget.payload_frames =
		5 * sizeof(void *) + 2 * sizeof(bool) + sizeof(size_t) + sizeof(error);
	if (!budget.peak())
		return false;
	if (!budget.prefix(nested))
		return false;
	if (native_allowed && command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
	{
		// Original decode_payload constructs its separate native context.
		budget.native_frames = sizeof(auction_native_command_context);
		if (!budget.peak())
			return false;
		auction_native_command_context native;
		budget.native = &native;
		if (!budget.prefix(nested))
			return false;
		const auto status = auction_native_command_decode_bounded(
			command, &native, auction_codec_budget::forward, &budget, nested);
		if (status != error::ok)
			return false;
		*out = native.payload;
		budget.native = nullptr;
		budget.native_frames = 0;
		budget.payload_frames = 0;
		return true;
	}
	const bool result = auction_command_decode_payload_bounded(
		command, out, auction_codec_budget::forward, &budget, nested, &budget.denied);
	budget.payload_frames = 0;
	return result;
}
} // namespace

namespace
{
error append_native_facts_bounded(const critical_command &command, std::vector<uint8_t> *facts,
				  auction_codec_budget &budget)
{
	if (command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
		return error::ok;
	size_t nested = 0;
	// Original helper's hash initializer array and loop/pointers coexist with
	// its real native context; the payload DTO is already inline in context.
	budget.native_frames = sizeof(auction_native_command_context) +
			       3 * sizeof(const economic_digest *) +
			       sizeof(std::initializer_list<const economic_digest *>) +
			       7 * sizeof(void *) + sizeof(size_t) + sizeof(error);
	if (!budget.peak())
		return error::capacity;
	auction_native_command_context native;
	budget.native = &native;
	if (!budget.prefix(nested))
		return error::capacity;
	const auto decoded = auction_native_command_decode_bounded(
		command, &native, auction_codec_budget::forward, &budget, nested);
	if (decoded != error::ok)
		return budget.denied ? error::capacity : error::corrupt_evidence;
	if (native.selected_node_count > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS)
		return error::capacity;
	if (!auction_codec_native_extension_peak(*facts, budget))
		return error::capacity;
	native_fact_append(*facts, native_fact_magic);
	native_fact_append(*facts, uint16_t{ 2 });
	native_fact_append(*facts, uint16_t{ 0 });
	native_fact_append(*facts, native.original_level);
	native_fact_append(*facts, native.acknowledged_save_revision);
	for (const auto *hash :
	     { &native.before_digest, &native.after_digest, &native.selected_digest })
		facts->insert(facts->end(), hash->begin(), hash->end());
	native_fact_append(*facts, native.selected_node_count);
	native_fact_append(*facts, native.selected_root_count);
	native_fact_append(*facts, uint16_t{ 0 });
	budget.native = nullptr;
	budget.native_frames = 0;
	return error::ok;
}
}
economic_accounting_error auction_item_claim_accounting_intent_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const auction_item_claim_state &claim, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	const size_t frames =
		sizeof(auction_command_payload) + sizeof(economic_admission_facts) +
		// Public parameters/status/prefix/request, original typed read lambdas,
		// loop indices/references, fixed arrays/comparisons and helper results.
		18 * sizeof(void *) + 13 * sizeof(size_t) + 8 * sizeof(bool) + 3 * sizeof(error) +
		6 * sizeof(uint64_t) + 4 * sizeof(uint32_t) + 3 * sizeof(uint16_t) +
		sizeof(std::array<uint8_t, 4>) + auction_codec_scalar_source_frames +
		auction_codec_vector_frames + auction_codec_move_frames +
		auction_codec_vector_constructor_frames +
		// valid_claim staged_claimant/index/prior/row/item refs; valid_accounts
		// wallet/bank/racewar args. Original frozen_facts actual row loop, fresh
		// returned vector and source_for two owner-reference parameters.
		sizeof(uint32_t) + 2 * sizeof(size_t) + 2 * sizeof(void *) + 2 * sizeof(void *) +
		sizeof(uint8_t) + sizeof(bool) + sizeof(size_t) + 3 * sizeof(void *) +
		sizeof(std::vector<uint8_t>);
	auction_codec_budget budget{ reserve, context, outer_live, frames };
	size_t nested = 0;
	if (!auction_codec_policy() || !budget.peak(critical_command_valid_frame_bytes()))
		return error::capacity;
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!auction_codec_payload(command, &payload, budget, true))
		return budget.denied ? error::capacity : error::corrupt_evidence;
	if (!valid_accounts(wallet, bank, payload.racewar) || !valid_claim(payload, claim) ||
	    critical_operation_id_equal(command.operation_id, claim.listing_operation) ||
	    critical_operation_id_equal(command.operation_id, claim.claim_source_operation))
		return budget.denied ? error::capacity : error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		budget.admission = &facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.original_operation_id = claim.listing_operation;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_AUCTION_ITEM_CLAIM;
		facts.metadata.reason = economic_reason::auction_claim;
		facts.metadata.source_event = source_for(claim);
		if (!budget.peak(82 + claim.item_count * 27))
			return error::capacity;
		facts.facts = frozen_facts(wallet, bank, claim);
		const auto native_status =
			append_native_facts_bounded(command, &facts.facts, budget);
		if (native_status != error::ok)
			return native_status;
		if (!budget.prefix(nested))
			return error::capacity;
		return economic_intent_freeze_fixed_bounded(
			command, facts, encoded, auction_codec_budget::forward, &budget, nested);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_item_claim_accounting_decode_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_item_claim_state *claim,
	economic_account_key *wallet, economic_account_key *bank,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	const size_t frames =
		sizeof(auction_command_payload) + sizeof(auction_item_claim_state) +
		2 * sizeof(economic_account_key) + sizeof(economic_frozen_intent) +
		sizeof(std::span<const uint8_t>) + sizeof(critical_command) +
		sizeof(std::vector<uint8_t>) +
		// Public parameters/status/prefix/request, original typed read lambdas,
		// loop indices/references, fixed arrays/comparisons and helper results.
		18 * sizeof(void *) + 13 * sizeof(size_t) + 8 * sizeof(bool) + 3 * sizeof(error) +
		6 * sizeof(uint64_t) + 4 * sizeof(uint32_t) + 3 * sizeof(uint16_t) +
		sizeof(std::array<uint8_t, 4>) + auction_codec_scalar_source_frames +
		auction_codec_vector_frames + auction_codec_move_frames +
		auction_codec_vector_constructor_frames +
		// valid_claim staged_claimant/index/prior/row/item refs; valid_accounts
		// wallet/bank/racewar args. Original frozen_facts actual row loop, fresh
		// returned vector and source_for two owner-reference parameters.
		sizeof(uint32_t) + 2 * sizeof(size_t) + 2 * sizeof(void *) + 2 * sizeof(void *) +
		sizeof(uint8_t) + sizeof(bool) + sizeof(size_t) + 3 * sizeof(void *) +
		sizeof(std::vector<uint8_t>);
	auction_codec_budget budget{ reserve, context, outer_live, frames };
	size_t nested = 0;
	if (!auction_codec_policy() || !budget.peak(critical_command_valid_frame_bytes()))
		return error::capacity;
	if (!intent || !payload || !claim || !wallet || !bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!auction_codec_payload(command, &parsed_payload, budget, true) ||
		    parsed_payload.action != auction_action::claim_item)
			return budget.denied ? error::capacity : error::invalid_identity;
		economic_frozen_intent parsed_intent;
		budget.intent = &parsed_intent;
		if (!budget.prefix(nested))
			return error::capacity;
		const auto decoded = economic_intent_decode_bounded(command.accounting_intent,
								    &parsed_intent,
								    auction_codec_budget::forward,
								    &budget, nested);
		if (decoded != error::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		if (!budget.prefix(nested))
			return error::capacity;
		const auto binding = economic_intent_verify_binding_bounded(
			command, parsed_intent, auction_codec_budget::forward, &budget, nested);
		if (binding != error::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() < claim_fact_header_bytes)
			return budget.denied ? error::capacity : error::invalid_identity;
		const uint16_t count = read_number<uint16_t>(facts, 80);
		if (!count || count > AUCTION_COMMAND_MAX_ITEMS ||
		    facts.size() !=
			    claim_fact_header_bytes + count * claim_fact_row_bytes +
				    (command.payload_version ==
						     AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ?
					     native_fact_extension_bytes :
					     0))
			return budget.denied ? error::capacity : error::invalid_identity;
		const auto &lineage = parsed_intent.admission.metadata.lineage;
		const economic_account_key parsed_wallet = { lineage, economic_account_kind::wallet,
							     read_number<uint64_t>(facts, 0), 0 };
		const economic_account_key parsed_bank = { lineage, economic_account_kind::bank,
							   read_number<uint64_t>(facts, 8),
							   parsed_payload.racewar };
		auction_item_claim_state parsed_claim;
		parsed_claim.auction_id = read_number<uint32_t>(facts, 16);
		parsed_claim.seller_pid = read_number<uint32_t>(facts, 20);
		parsed_claim.winner_pid = read_number<uint32_t>(facts, 24);
		parsed_claim.claimant_pid = read_number<uint32_t>(facts, 28);
		parsed_claim.status = read_number<uint32_t>(facts, 32);
		parsed_claim.custody_state = read_number<uint32_t>(facts, 36);
		parsed_claim.auction_revision = read_number<uint64_t>(facts, 40);
		std::copy_n(facts.begin() + 48, 16, parsed_claim.listing_operation.bytes.begin());
		std::copy_n(facts.begin() + 64, 16,
			    parsed_claim.claim_source_operation.bytes.begin());
		parsed_claim.item_count = count;
		for (size_t index = 0; index < count; ++index)
		{
			const size_t offset =
				claim_fact_header_bytes + index * claim_fact_row_bytes;
			auto &entry = parsed_claim.rows[index];
			entry.uid = read_number<uint64_t>(facts, offset);
			entry.revision = read_number<uint64_t>(facts, offset + 8);
			entry.slot = read_number<uint16_t>(facts, offset + 16);
			entry.vnum =
				static_cast<int32_t>(read_number<uint32_t>(facts, offset + 18));
			entry.claim_pid = read_number<uint32_t>(facts, offset + 22);
			if (facts[offset + 26] > 1)
				return budget.denied ? error::capacity : error::invalid_identity;
			entry.claimed = facts[offset + 26] == 1;
		}
		size_t copy_request = 0;
		if (!critical_command_fresh_copy_request_bytes(command, &copy_request) ||
		    !auction_codec_add(copy_request, critical_command_copy_frame_bytes()) ||
		    !budget.peak(copy_request))
			return error::capacity;
		critical_command projected = command;
		budget.projection = &projected;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		budget.expected = &expected;
		if (!budget.prefix(nested))
			return error::capacity;
		const auto frozen = auction_item_claim_accounting_intent_bounded(
			projected, parsed_intent.admission.metadata.epoch, parsed_wallet,
			parsed_bank, parsed_claim, &expected, auction_codec_budget::forward,
			&budget, nested);
		if (frozen != error::ok || expected != command.accounting_intent)
			return budget.denied ? error::capacity : error::unauthorized;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*claim = parsed_claim;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

// Additive original fixed reconstruction/Source candidate; finite review pending.
namespace
{
enum class auction_fixed_child : uint8_t;
}

// PRIVATE genuine named primitive/lifecycle Source fragments. Complete family totals remain OPEN.
namespace
{
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_byte_reserve =
	// vector.reserve(this,n), old_size, tmp; size/capacity and max_size.
	sizeof(void *) + 3 * sizeof(size_t) + sizeof(uint8_t *) +
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) + 3 * sizeof(void *) +
	// _M_allocate -> traits::allocate -> allocator -> new_allocator,
	// actual result/constant-evaluation bool/operator new argument/result.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + 2 * sizeof(size_t) +
	sizeof(void *) + sizeof(bool) +
	// _S_relocate -> __relocate_a -> __relocate_a_1 trivial byte memmove.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// original old buffer deallocation, including the zero-pointer branch.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_byte_append =
	// append_u64(vector*,value) byte index and cast byte temporary; rvalue
	// push_back -> emplace_back, forward -> construct -> construct_at and
	// placement-new result. The freshly empty facts vector was reserve'd
	// to its exact 16/24-byte length before these original two/three loops;
	// genuine cap>=length proof excludes the reallocation branch here.
	sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(uint8_t) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// actual emplace return back/end iterator and dereference/base scopes.
	7 * sizeof(void *) + sizeof(std::ptrdiff_t);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_equal =
	// vector<byte> == size/begin/end -> equal -> equal_aux/aux1/true::equal,
	// real iterator/base/pointer wrappers, len and final memcmp boundary.
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	3 * 2 * sizeof(void *) + 6 * 2 * sizeof(void *) + 4 * (3 * sizeof(void *) + sizeof(bool)) +
	2 * sizeof(bool) + 3 * 2 * sizeof(void *) + sizeof(std::ptrdiff_t) + 2 * sizeof(void *) +
	sizeof(size_t) + sizeof(int);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_array_equal =
	// Actual selected installed array5904: operator==(two refs;bool),
	// begin/end/begin each(this;pointer) and their three direct data calls.
	// No _S_ptr/_S_ref or size() descendant exists in this selected path.
	2 * sizeof(void *) + sizeof(bool) + 6 * (2 * sizeof(void *)) +
	// std::equal -> equal_aux -> equal_aux1 -> equal<true>::equal;
	// pointer niter bases, simple/integer bools, len and memcmp leaf.
	4 * (3 * sizeof(void *) + sizeof(bool)) + 3 * 2 * sizeof(void *) + 2 * sizeof(bool) +
	sizeof(std::ptrdiff_t) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int);
// Exact fitting byte push descendant excludes the original append_u64
// caller, which each actual family prices explicitly with its true types.
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_byte_fitting_push =
	auction_reconstruction_fixed_byte_append -
	(sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(uint8_t));
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_array_index_source =
	// Actual const/mutable array::operator[](this,index;reference), direct
	// _M_elems[index]. Source policy excludes assertion/debug calls.
	2 * sizeof(void *) + sizeof(size_t);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_array_id_range_source =
	// Actual ID input begin/end and their two direct data calls.
	4 * (2 * sizeof(void *));
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_span_source =
	// span(vector&): this/range -> ranges::_Data(this,range)->vector.data
	// -> _M_data_ptr(this,pointer); ranges::_Size(this,range)->vector.size.
	// Actual ranges noexcept expressions are required constant expressions.
	2 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) +
	// Delegating span(pointer,count) -> std::to_address(pointer) and actual
	// dynamic __extent_storage(this,count), not a static extent surrogate.
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	// span.size -> extent::_M_extent and operator[] receiver/index/reference.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_optional_source =
	// One actual fresh admission metadata optional, selecting eight default receivers:
	// optional/_Enable_copy_move/_Optional_base/_Optional_base_impl/
	// _Optional_payload/_Optional_payload_base/_Storage. The selected
	// source_event is trivial, so their seven defaulted cleanup receivers
	// have no reset/destroy call or contained-value destructor dispatch.
	// _Storage() value-initializes its actual _Empty_byte member.
	(8 * sizeof(void *) + 7 * sizeof(void *)) +
	// operator=(source_event&&): this/source/reference result and
	// _M_is_engaged(this,bool). Fresh admission.source_event is disengaged;
	// only the actual construction branch is reached, not _M_get/assignment.
	3 * sizeof(void *) + sizeof(void *) + sizeof(bool) +
	// base_impl::_M_construct -> payload_base::_M_construct; three forward
	// scopes, addressof, _Construct, construct_at and placement-new.
	4 * sizeof(void *) + 3 * 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(bool) + 2 * sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	// Generated source_event move(this,source), each ID wrapper and its
	// array member move pair; returned source_for aggregate copies
	// its two actual command IDs in the separate source_assignment graph.
	// Temporary cleanup visits source_event and both wrapper/array pairs.
	// No optional stored destructor dispatch in this trivial specialization.
	2 * sizeof(void *) + 2 * (2 * sizeof(void *) + 2 * sizeof(void *)) +
	// source_for returned ID copies are owned separately exactly once.
	5 * sizeof(void *);
}

// PRIVATE named genuine lifecycle subgraphs, no whole family alias.
namespace
{
template <class T> constexpr size_t auction_reconstruction_fixed_vector_default =
	6 * sizeof(void *);
// Vector destructor -> _Destroy trivial dispatch -> base destructor,
// _M_deallocate -> traits/allocator/new_allocator -> sized delete, followed
// by actual allocator and new_allocator base cleanup. No nontrivial T here.
template <class T> constexpr size_t auction_reconstruction_fixed_vector_cleanup =
	sizeof(std::vector<T> *) + 2 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(void *) + 4 * (2 * sizeof(void *) + sizeof(size_t)) +
	// Sized delete plus real _Vector_impl, allocator, new_allocator and
	// _Vector_impl_data cleanup receivers (base destructor above owns P).
	sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
	// Genuine C++20 _Destroy and allocator::deallocate runtime false
	// constant-evaluation result carriers; both selected calls still occur.
	2 * sizeof(bool);
// Move assignment operator=(this,source), true_type -> _M_move_assign:
// real tmp(get_allocator()), allocator temporary, two _M_swap_data calls
// with actual _Vector_impl_data temporary, copy_data receivers, std::move,
// allocator_on_move and the tmp/allocator cleanup. std::allocator propagates.
template <class T> constexpr size_t auction_reconstruction_fixed_vector_move =
	// Public operator= receivers/result and its actual constexpr policy bool;
	// _M_move_assign(this,source,true_type) formals.
	3 * sizeof(void *) + sizeof(bool) +
	// Public std::move(__x) argument/reference result before _M_move_assign.
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(std::true_type) + sizeof(std::vector<T>) +
	sizeof(std::allocator<T>) +
	// get_allocator + const _M_get_Tp_allocator, allocator/new_allocator copy;
	// vector(allocator) -> base -> impl -> allocator/base copy -> data default.
	sizeof(void *) + sizeof(std::allocator<T>) + 2 * sizeof(void *) + 4 * sizeof(void *) +
	5 * 2 * sizeof(void *) + sizeof(void *) +
	2 * (2 * sizeof(void *) + sizeof(typename std::vector<T>::pointer) * 3 + sizeof(void *) +
	     3 * 2 * sizeof(void *) +
	     // Each actual swap temporary's trivial _Vector_impl_data cleanup.
	     sizeof(void *)) +
	// Two _M_get_Tp_allocator scopes; __alloc_on_move -> std::move and
	// genuine defaulted allocator/new_allocator copy-assignment results.
	4 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + 2 * 3 * sizeof(void *) +
	auction_reconstruction_fixed_vector_cleanup<T> + 2 * sizeof(void *);

[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_admission_lifetime_source =
	// Genuine admission and metadata default constructor receivers;
	// cleanup admission,metadata,four ID wrappers+arrays. Metadata has no digest member.
	2 * sizeof(void *) + (2 + 4 * 2) * sizeof(void *) +
	auction_reconstruction_fixed_vector_default<uint8_t> +
	auction_reconstruction_fixed_vector_cleanup<uint8_t>;
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_frozen_lifetime_source =
	// Genuine frozenâ†’admissionâ†’metadata default constructor receivers.
	// Cleanup same containing graph, four IDs+arrays and two digest arrays.
	3 * sizeof(void *) + (3 + 4 * 2 + 2) * sizeof(void *) +
	auction_reconstruction_fixed_vector_default<uint8_t> +
	auction_reconstruction_fixed_vector_cleanup<uint8_t> +
	// Optional actual eight default receivers include _Empty_byte();
	// seven cleanup receivers: trivial union has no active-member traversal.
	(8 + 7) * sizeof(void *);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_frozen_output_source =
	// Full strong generated frozenâ†’admissionâ†’metadata assignments, four
	// ID wrappers+array assignments, two direct digest array assignments,
	// seven optional selected assignment receivers. Source argument passed
	// by actual std::move has argument/ref-result, vector move graph once.
	3 * (3 * sizeof(void *)) + 4 * (2 * 3 * sizeof(void *)) + 2 * (3 * sizeof(void *)) +
	7 * (3 * sizeof(void *)) + 2 * sizeof(void *) +
	auction_reconstruction_fixed_vector_move<uint8_t>;
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_native_lifetime_source =
	// Containing native default+cleanup, two byte/u64 vectors genuine
	// lifecycle graphs, payload seven-arrays/nine-items cleanup and three
	// native digest-array cleanup. {} digest/payload members have aggregate
	// initialization, not an invented called default constructor.
	2 * sizeof(void *) + auction_reconstruction_fixed_vector_default<uint8_t> +
	auction_reconstruction_fixed_vector_cleanup<uint8_t> +
	auction_reconstruction_fixed_vector_default<uint64_t> +
	auction_reconstruction_fixed_vector_cleanup<uint64_t> +
	(1 + 7 + AUCTION_COMMAND_MAX_ITEMS + 3) * sizeof(void *) +
	// Real native-helper *payload=native.payload generated assignment.
	(1 + 7 + AUCTION_COMMAND_MAX_ITEMS) * 3 * sizeof(void *);

}

// PRIVATE typed subgraphs for original accounting copy_n(...,int{16},...)
// and returned byte-vector move construction. No runtime algorithm change.
namespace
{
using auction_fact_iterator = std::span<const uint8_t>::iterator;
[[maybe_unused]] constexpr size_t auction_fact_P = sizeof(void *), auction_fact_N = sizeof(size_t),
				  auction_fact_D = sizeof(std::ptrdiff_t),
				  auction_fact_B = sizeof(bool);
// Actual GCC13 span.iterator is a normal iterator, never a pointer surrogate.
static_assert(
	std::is_same_v<auction_fact_iterator,
		       __gnu_cxx::__normal_iterator<const uint8_t *, std::span<const uint8_t>>>);
[[maybe_unused]] constexpr size_t auction_fact_iterator_plus_source =
	// Normal iterator +(this,difference;iterator result), real temporary
	// pointer passed to its pointer-reference constructor(this,arg).
	sizeof(auction_fact_iterator *) + auction_fact_D + sizeof(auction_fact_iterator) +
	sizeof(const uint8_t *) + 2 * auction_fact_P;
[[maybe_unused]] constexpr size_t auction_fact_copy_n_int_source =
	// copy_n(normal-first,int-count,pointer-result; __n2 int, pointer return).
	sizeof(auction_fact_iterator) + 2 * sizeof(int) + 2 * auction_fact_P +
	// __size_to_integer(int argument/result); iterator_category(first-ref,
	// actual RA tag result); __copy_n(normal-first,int,pointer,RA tag;ptr).
	2 * sizeof(int) + auction_fact_P + sizeof(std::random_access_iterator_tag) +
	sizeof(auction_fact_iterator) + sizeof(int) + 2 * auction_fact_P +
	sizeof(std::random_access_iterator_tag) +
	// Genuine lvalue first copy into __copy_n and into std::copy, their
	// normal-iterator ctor/cleanup scopes, plus copy_n's first cleanup.
	2 * (2 * auction_fact_P + auction_fact_P) + auction_fact_P +
	auction_fact_iterator_plus_source +
	// std::copy and __copy_move_a(normal-first/last,ptr-result;ptr return).
	2 * (2 * sizeof(auction_fact_iterator) + 2 * auction_fact_P) +
	// Two selected generic __miter_base(normal-by-value;normal return),
	// actual argument copy/returned move constructors and their cleanups.
	2 * (2 * sizeof(auction_fact_iterator) + 2 * auction_fact_P + 2 * auction_fact_P +
	     auction_fact_P) +
	// Both std::copy input normal iterators and copy_move_a inputs clean up.
	4 * auction_fact_P +
	// Two normal __niter_base(by-value;ptr return), each calls base(this,
	// const pointer-reference result), real argument copy and cleanup.
	2 * (sizeof(auction_fact_iterator) + auction_fact_P + 2 * auction_fact_P +
	     2 * auction_fact_P + auction_fact_P) +
	// Pointer output niter_base; pointer niter_wrap(ref,ptr;ptr return).
	2 * auction_fact_P + 3 * auction_fact_P +
	// Exact raw-pointer a1/a2/simple-copy_m: three pointers+return each.
	// a2's actual false is_constant_evaluated return; copy_m's _Num.
	3 * (4 * auction_fact_P) + auction_fact_B + auction_fact_D +
	// Original copy has literal16 elements: _Num>1 always, so only bulk
	// memmove declaration is selected; no fabricated one-element assignment.
	3 * auction_fact_P + auction_fact_N;
[[maybe_unused]] constexpr size_t auction_fact_copy_n_caller_source =
	// Original span.begin(this,normal result)+pointer-ref iterator ctor,
	// caller +(offset) selected above, temporary begin iterator cleanup.
	auction_fact_P + sizeof(auction_fact_iterator) + 2 * auction_fact_P +
	// The shared typed operator+ union is retained once by copy_n_int_source.
	// This original caller still owns its distinct begin iterator cleanup.
	auction_fact_P +
	// Selected target array.begin -> direct data; no array_traits helper.
	2 * (2 * auction_fact_P);
[[maybe_unused]] constexpr size_t auction_fact_returned_vector_move_source =
	// Optional NRVO path's actual vector, base, impl move constructor pairs.
	3 * 2 * auction_fact_P +
	// _Vector_impl performs two actual std::move(this allocator/data), each
	// argument/reference result; std::allocator's move resolves its const-copy
	// constructor and real new_allocator base default constructor receiver.
	2 * 2 * auction_fact_P + 2 * auction_fact_P + auction_fact_P +
	// Actual _Vector_impl_data move(this,source) and pointer() reset value.
	2 * auction_fact_P + sizeof(uint8_t *);
}

// PRIVATE actual vector<byte>.insert(end(),const-byte-first,last) source graph.
// Original bid ID/AEC1 inserts have reserve(140), so only fitting end is selected.
// Native ANF2 digest insertion can select fitting OR reallocation; both are here.
// Every real call uses end(): elems_after=0 excludes move_backward and the
// __elems_after>__n arm. No arbitrary whole-codec/vector allowance is imported.
namespace
{
[[maybe_unused]] constexpr size_t auction_insert_P = sizeof(void *),
				  auction_insert_N = sizeof(size_t),
				  auction_insert_D = sizeof(std::ptrdiff_t),
				  auction_insert_B = sizeof(bool);
using auction_insert_iterator = std::vector<uint8_t>::iterator;
using auction_insert_const_iterator = std::vector<uint8_t>::const_iterator;
using auction_insert_move_iterator = std::move_iterator<uint8_t *>;
[[maybe_unused]] constexpr size_t auction_insert_pointer_copy_source =
	// copy(first,last,result;return), two pointer miter_base(arg,result),
	// copy_move_a/a1/a2/copy_m: actual 3 pointer inputs+pointer result each.
	5 * (4 * auction_insert_P) + 2 * (2 * auction_insert_P) +
	// Three genuine pointer niter_base(arg,result), niter_wrap(ref,ptr;ptr),
	// a2's constant-evaluation bool and simple copy_m's signed count.
	3 * (2 * auction_insert_P) + 3 * auction_insert_P + auction_insert_B + auction_insert_D +
	// copy_m true bulk declaration; original ranges are 0,4,16,32 bytes,
	// never1, so assign_one is not selected. __builtin_expect declaration
	// has actual long expression/expected/result (not a function baseline).
	3 * sizeof(long) + 3 * auction_insert_P + auction_insert_N;
[[maybe_unused]] constexpr size_t auction_insert_uninitialized_pointer_source =
	// allocator<byte> specialized uninitialized_copy_a(first,last,result,
	// allocator-ref;return) with false constant-evaluation bool.
	5 * auction_insert_P + auction_insert_B +
	// uninitialized_copy(first,last,result;return), its can_memmove and
	// assignable bools; true uninit_copy(first,last,result;return).
	2 * (4 * auction_insert_P) + 2 * auction_insert_B + auction_insert_pointer_copy_source;
[[maybe_unused]] constexpr size_t auction_insert_move_iterator_source =
	// make_move_iterator(pointer;move-iterator-result) and pointer-specific
	// make_move_if_noexcept_iterator(pointer;move-iterator-result).
	2 * (auction_insert_P + sizeof(auction_insert_move_iterator)) +
	// Actual move_iterator(pointer) ctor this/pointer + std::move(arg/ref).
	2 * auction_insert_P + 2 * auction_insert_P +
	// Move-iterator parameters in allocator uninitialized_copy_a,
	// uninitialized_copy and true uninit_copy have copy constructors and
	// cleanup receiver scopes, independently of their actual value carriers.
	3 * 2 * (2 * auction_insert_P + auction_insert_P) +
	// std::copy takes two move iterators by value, constructs its two
	// __miter_base parameters, and cleans its own parameter values.
	2 * (2 * auction_insert_P + auction_insert_P) +
	2 * (2 * auction_insert_P + auction_insert_P) +
	// move_iterator miter_base(arg by value;ptr return) -> base() const&
	// (this/ref result), then actual pointer miter_base(arg/result).
	2 * (sizeof(auction_insert_move_iterator) + auction_insert_P + 2 * auction_insert_P +
	     2 * auction_insert_P) +
	// Genuine implicit move_iterator cleanup for both miter parameters.
	2 * auction_insert_P;
[[maybe_unused]] constexpr size_t auction_insert_uninitialized_move_source =
	// move_a and move_if_noexcept_a inputs(first,last,result,alloc;return).
	// Their two explicit move-iterator temporaries survive child call and
	// each has a real trivial cleanup receiver. Runtime byte move is noexcept.
	2 * (5 * auction_insert_P) + 2 * auction_insert_P + auction_insert_move_iterator_source +
	auction_insert_uninitialized_pointer_source;
[[maybe_unused]] constexpr size_t auction_insert_normal_output_source =
	// Fitting branch copy(first,mid,normal-position) returns normal iterator,
	// through copy_move_a. Pointer-sized value carriers already in pointer
	// copy source; these genuine constructors/cleanups are additional.
	2 * (2 * auction_insert_P + auction_insert_P) +
	// Output niter_base(normal-by-value;ptr) -> base(this,const-ref),
	// including actual parameter copy/cleanup.
	2 * auction_insert_P + 2 * auction_insert_P + 2 * auction_insert_P + auction_insert_P +
	// niter_wrap(original-ref,raw-result;normal-result) performs
	// original + (result-niter_base(original)); exact plus(this,difference,
	// normal-result), pointer temporary and ctor(this,pointer-ref).
	auction_insert_P + auction_insert_P + sizeof(auction_insert_iterator) +
	2 * auction_insert_P + 2 * auction_insert_P + auction_insert_P + auction_insert_P +
	auction_insert_D + sizeof(auction_insert_iterator) + auction_insert_P +
	2 * auction_insert_P +
	// Returned output normal iterator cleanup in original discarded copy.
	auction_insert_P;
[[maybe_unused]] constexpr size_t auction_insert_end_public_source =
	// insert(this,const-position,first,last;iterator-result), offset local.
	3 * auction_insert_P + sizeof(auction_insert_const_iterator) +
	sizeof(auction_insert_iterator) + auction_insert_D +
	// Caller end()→normal pointer-ref ctor and mutable→const converting
	// ctor, followed by actual end temporary/position argument cleanup.
	auction_insert_P + sizeof(auction_insert_iterator) + 2 * auction_insert_P +
	2 * auction_insert_P + 2 * auction_insert_P + auction_insert_P +
	// cbegin/begin/end each this/normal result and pointer-ref ctor.
	3 * (auction_insert_P + sizeof(auction_insert_iterator) + 2 * auction_insert_P) +
	// const normal subtraction two refs/difference, two base const-ref pairs.
	2 * auction_insert_P + auction_insert_D + 2 * (2 * auction_insert_P) +
	// begin()+offset this/difference/result; actual pointer temporary +ctor.
	auction_insert_P + auction_insert_D + sizeof(auction_insert_iterator) + auction_insert_P +
	2 * auction_insert_P +
	// Pointer iterator_category(ref,RA result); forward tag conversion,
	// range_insert(this,normal-position,first,last,forward-tag); bool guard.
	auction_insert_P + sizeof(std::random_access_iterator_tag) +
	sizeof(std::forward_iterator_tag) + 3 * auction_insert_P + sizeof(auction_insert_iterator) +
	sizeof(std::forward_iterator_tag) + auction_insert_B +
	// Real by-value normal range-position copy+cleanup, generated cleanup
	// of begin/cbegin/end/plus return temporaries used by public insert.
	2 * auction_insert_P + auction_insert_P + 4 * auction_insert_P +
	// range_insert n; distance(pointer,pointer;difference) then
	// __distance(pointer,pointer,RA-tag;difference), category(ref,RA result).
	auction_insert_N + 2 * (2 * auction_insert_P + auction_insert_D) +
	sizeof(std::random_access_iterator_tag) + auction_insert_P +
	sizeof(std::random_access_iterator_tag);
[[maybe_unused]] constexpr size_t auction_insert_end_fitting_source =
	// elems_after size_t, old_finish pointer, mid input pointer. end()-pos
	// two normal refs/difference + two actual base receiver/reference pairs.
	auction_insert_N + 2 * auction_insert_P + 2 * auction_insert_P + auction_insert_D +
	2 * (2 * auction_insert_P) +
	// advance(mid-ref,size_t count): local difference; category(ref,RA),
	// __advance(pointer-ref,difference,RA-tag). mid advances by literal0
	// end-position distance; ++/-- alternatives are never selected.
	auction_insert_P + auction_insert_N + auction_insert_D + auction_insert_P +
	sizeof(std::random_access_iterator_tag) + auction_insert_P + auction_insert_D +
	sizeof(std::random_access_iterator_tag) +
	// Current allocator reference getter, position.base() and normal output
	// by-value copy into the empty tail-copy call.
	2 * auction_insert_P + 2 * auction_insert_P + auction_insert_uninitialized_pointer_source +
	auction_insert_uninitialized_move_source + auction_insert_normal_output_source;
[[maybe_unused]] constexpr size_t auction_insert_end_growth_source =
	// Exact old_start/old_finish/new_start/new_finish pointers and len.
	4 * auction_insert_P + auction_insert_N +
	// _M_check_len(this,n,diagnostic;size-result), local len,
	// max_size(this,result)→_S_max_size(alloc-ref,result), diffmax/allocmax;
	// traits max_size(alloc-ref,result)→new_allocator _M_max_size(this,result).
	2 * auction_insert_P + 3 * auction_insert_N + 4 * (auction_insert_P + auction_insert_N) +
	2 * auction_insert_N +
	// size(this,result), min/max two refs+returned ref; diagnostic throw
	// length_error message pointer is admitted even on growth-policy refusal.
	auction_insert_P + auction_insert_N + 2 * (3 * auction_insert_P) + auction_insert_P +
	// allocator reference getter; allocate→traits→allocator→new_allocator,
	// actual hint/default argument, operator-new n/returned-pointer and
	// false constant-evaluation result in allocator::allocate.
	2 * auction_insert_P + 3 * (2 * auction_insert_P + auction_insert_N) +
	3 * auction_insert_P + auction_insert_N + auction_insert_P + auction_insert_N +
	auction_insert_B +
	// Both move-if-noexcept prefix/tail and actual copy middle use the same
	// selected named source functions sequentially; union contains each once.
	auction_insert_uninitialized_move_source + auction_insert_uninitialized_pointer_source +
	// _Destroy(first,last,alloc), _Destroy(first,last), destroy_aux<true>,
	// false constant-evaluation bool; bytes have no per-element destructor.
	3 * auction_insert_P + 2 * auction_insert_P + 2 * auction_insert_P + auction_insert_B +
	// Actual deallocation four scopes and sized-delete pointer/count.
	4 * (2 * auction_insert_P + auction_insert_N) + auction_insert_P + auction_insert_N;
[[maybe_unused]] constexpr size_t auction_insert_end_fitting_complete_source =
	auction_insert_end_public_source + auction_insert_end_fitting_source;
[[maybe_unused]] constexpr size_t auction_insert_end_any_complete_source =
	auction_insert_end_public_source + auction_insert_end_fitting_source +
	auction_insert_end_growth_source;
}

// PRIVATE genuine family generated assignment/cleanup inventories.
// Brace-valued aggregate DMIs do not call ID/array default constructors.
namespace
{
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_bid_listing_source =
	// Local parsed_listing implicit default receiver, containing and two ID/
	// array cleanup chains; strong final listing copy traverses same five scopes.
	sizeof(void *) + (1 + 2 * 2) * sizeof(void *) + (1 + 2 * 2) * 3 * sizeof(void *);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_bid_accounts_source =
	// parsed_accounts default receiver; six key/ID/array cleanup chains.
	sizeof(void *) + (1 + 6 * 3) * sizeof(void *) +
	// Six original returned key aggregates copy lineage through wrapper+array,
	// then assign account/ID/array and clean temporary account/ID/array.
	6 * (2 * 2 * sizeof(void *) + 3 * 3 * sizeof(void *) + 3 * sizeof(void *)) +
	// Final strong accounts assignment traverses containing and six keys.
	(1 + 6 * 3) * 3 * sizeof(void *);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_settlement_listing_source =
	// Default containing receiver; cleanup visits two IDs/arrays, original
	// items array and all nine trivial row members. Strong output same graph.
	sizeof(void *) + (1 + 2 * 2 + 1 + AUCTION_COMMAND_MAX_ITEMS) * sizeof(void *) +
	(1 + 2 * 2 + 1 + AUCTION_COMMAND_MAX_ITEMS) * 3 * sizeof(void *);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_settlement_accounts_source =
	sizeof(void *) + (1 + 4 * 3) * sizeof(void *) +
	// Four authentic aggregate account temporaries and final strong assignment.
	4 * (2 * 2 * sizeof(void *) + 3 * 3 * sizeof(void *) + 3 * sizeof(void *)) +
	(1 + 4 * 3) * 3 * sizeof(void *);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_claim_state_source =
	sizeof(void *) + (1 + 2 * 2 + 1 + AUCTION_COMMAND_MAX_ITEMS) * sizeof(void *) +
	(1 + 2 * 2 + 1 + AUCTION_COMMAND_MAX_ITEMS) * 3 * sizeof(void *);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_two_returned_accounts_source =
	// Original two parsed wallet/bank aggregates return-copy their lineage;
	// they have no separate called default ctor due aggregate initialization.
	2 * (2 * 2 * sizeof(void *) + 3 * sizeof(void *) + 3 * 3 * sizeof(void *));
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_payload_output_cleanup_source =
	// Exact payload seven-array/nine-item strong final assignment and local
	// aggregate cleanup. Initialization remains the original payload{}.
	(1 + 7 + AUCTION_COMMAND_MAX_ITEMS) * (3 * sizeof(void *) + sizeof(void *));
}

// PRIVATE exact common callback/helper/current/scalar Source inventories.
namespace
{
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_zero_source =
	// ID ref/bool, actual array range ref/begin/end/current byte;
	// selected installed array begin/end each call direct data, no size().
	sizeof(void *) + sizeof(bool) + 3 * sizeof(void *) + sizeof(uint8_t) +
	4 * (2 * sizeof(void *));
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_key_valid_source =
	// economic_account_key_valid(key-ref;bool)â†’zero and kind_valid(kind,bool).
	sizeof(void *) + sizeof(bool) + auction_reconstruction_fixed_zero_source +
	sizeof(economic_account_kind) + sizeof(bool);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_id_equal_source =
	// critical_operation_id_equal(left-ref,right-ref;bool)â†’actual array16==.
	2 * sizeof(void *) + sizeof(bool) + auction_reconstruction_fixed_array_equal;
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_budget_source =
	// Exact checked_add(ref,value;bool), forward(amount,opaque;budget-ref,bool).
	sizeof(void *) + sizeof(size_t) + sizeof(bool) + 2 * sizeof(void *) + sizeof(size_t) +
	sizeof(bool) +
	// prefix(this,total-ref,extra;heap,bool), peak(this,extra;total,bool).
	2 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(bool) +
	// The actual admission/intent/expected facts byte capacity invocations,
	// native base/u64 capacity are separate call sites in the retained census.
	5 * (sizeof(void *) + sizeof(size_t)) +
	// Genuine trivial budget cleanup receiver, no called aggregate constructor.
	sizeof(void *);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_child_entry_source =
	// budget-ref,kind,nested-ref;source/initial/supplement and selected
	// required constexpr query local; admitted/result bools.
	2 * sizeof(void *) + sizeof(auction_fixed_child) + 4 * sizeof(size_t) + 2 * sizeof(bool);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_payload_helper_source =
	// command/out/budget formals,native_allowed; nested,status,returned bool,
	// completed generic payload result bool. Context is actual INLINE below.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(error) + 3 * sizeof(bool);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_native_helper_source =
	// append command/facts/budget refs, nested,decoded/result errors.
	3 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(error) +
	// Original hash range's actual initializer-list object/backing3-pointer
	// array,range ref,begin/end,current hash pointer and real begin/end/size
	// accessor declaration carriers. Not an entire native payload allowance.
	sizeof(std::initializer_list<const economic_digest *>) + 3 * sizeof(void *) +
	4 * sizeof(void *) +
	// Real initializer_list ctor(this,array,len), begin/end(this,result),
	// size(this,result) and generated trivial cleanup receiver.
	2 * sizeof(void *) + sizeof(size_t) + 2 * (2 * sizeof(void *)) + sizeof(void *) +
	sizeof(size_t) + sizeof(void *) +
	// Selected array begin/end each calls direct data; no _S_ptr/size().
	4 * (2 * sizeof(void *));
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_native_forecast_source =
	// Original exact forecast params, size/capacity/largest/request,run,
	// count/iterations/i/simultaneous/extra; runs11 array,block and result.
	2 * sizeof(void *) + 10 * sizeof(size_t) + 11 * sizeof(size_t) + 2 * sizeof(bool) +
	// std::size(array-ref;N), two facts.capacity and facts.size calls.
	sizeof(void *) + sizeof(size_t) + 3 * (sizeof(void *) + sizeof(size_t)) +
	// Original growth_peak(size,capacity,count,request-ref;bool): exact
	// required and added locals. It uses a plain conditional expression,
	// no std::max/allocator query. checked_add is in budget Source once.
	sizeof(void *) + 5 * sizeof(size_t) + sizeof(bool);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_source_assignment_source =
	// Authentic source_for returned event contains two ID copy wrapper+array
	// constructor pairs. Caller fresh optional construction is independently
	// mapped in fixed_optional_source, including returned event cleanup.
	sizeof(void *) + 2 * (2 * 2 * sizeof(void *));
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_metadata_assignment_source =
	// Three actual explicit metadata lineage/epoch/original ID copy-assign
	// scopes; each distinguishes ID wrapper and its std::array member.
	3 * (2 * 3 * sizeof(void *));
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_decode_scalar_source =
	// Seven public refs/pointers; outer and seven exact locals (including
	// later entry_peak), decoded/binding/reconstructed/returned error,
	// original bad_alloc catch ref and copy_request.
	7 * sizeof(void *) + 9 * sizeof(size_t) + 4 * sizeof(error) + sizeof(void *) +
	// Original vectorâ†’span conversion temporary and later named facts span
	// are sequential but genuinely distinct source sites. Full exact span
	// constructor/data/index/size Source is named separately.
	2 * sizeof(std::span<const uint8_t>) + auction_reconstruction_fixed_span_source +
	// Temporary conversion and named facts each clean span + dynamic
	// extent_storage; const-ref decoder child owns neither caller object.
	2 * (2 * sizeof(void *)) +
	// Actual lineage const reference; expected byte-vector own lifecycle,
	// projected accounting vector.clear(this)â†’erase_at_end(this,pos;n)
	// â†’_Destroy(first,last,alloc)â†’trivial destructors dispatch.
	sizeof(void *) + auction_reconstruction_fixed_vector_default<uint8_t> +
	auction_reconstruction_fixed_vector_cleanup<uint8_t> + sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(bool) + auction_reconstruction_fixed_payload_output_cleanup_source +
	auction_reconstruction_fixed_frozen_lifetime_source +
	auction_reconstruction_fixed_frozen_output_source + auction_reconstruction_fixed_equal;
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_number_lambda_source =
	// facts-ref capture object and operator() this; offset,width,byte,
	// value and returned uint64. Actual implicit closure cleanup receiver.
	2 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t) + sizeof(void *);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_listing_number_lambda_source =
	// Listing's original lambda has only offset and byte, no width argument.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(uint64_t) + sizeof(void *);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_item_read_number_source =
	// Three genuine read_number<T>(span value,offset) instantiations:
	// u16/u32/u64; local index,value and actual T result. Default span copy
	// and extent-storage copy/cleanup are explicit source scopes.
	3 * (sizeof(std::span<const uint8_t>) + 2 * sizeof(size_t) + 6 * sizeof(void *)) +
	2 * (sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint64_t));
}

// PRIVATE native integer push paths: exact byte-vector fitting/reallocation.
namespace
{
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_byte_growth_push =
	// push_back(value&&)→emplace_back(value&&), actual forward ref/result,
	// end() iterator receiver/result/ctor; realloc_insert(this,position,arg).
	2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	sizeof(std::vector<uint8_t>::iterator) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(std::vector<uint8_t>::iterator) +
	// _M_realloc_insert true byte-relocate selected locals len,old_start,
	// old_finish,elems_before,new_start,new_finish and actual pointer() value.
	2 * sizeof(size_t) + 5 * sizeof(void *) +
	// position-begin() real normal refs/difference, base pairs and begin ctor.
	2 * sizeof(void *) + sizeof(std::ptrdiff_t) + 2 * 2 * sizeof(void *) + sizeof(void *) +
	sizeof(std::vector<uint8_t>::iterator) + 2 * sizeof(void *) +
	// Real position and begin temporary generated cleanup receivers;
	// begin/position parameter generated copy/move constructor pair.
	2 * sizeof(void *) + 2 * sizeof(void *) +
	// _M_check_len(this,n,diagnostic;result), len local and all actual
	// max_size/_S_max_size/traits/new_allocator maxima +diffmax/allocmax.
	2 * sizeof(void *) + 3 * sizeof(size_t) + 4 * (sizeof(void *) + sizeof(size_t)) +
	2 * sizeof(size_t) + sizeof(void *) + sizeof(size_t) + 2 * 3 * sizeof(void *) +
	sizeof(void *) +
	// Allocation true selected chain, hint/default pointer and operator new,
	// actual allocator's false constant-evaluation result.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// Actual allocator get, forward, construct→construct_at→forward,
	// placement-new pointer/result/size. Byte construction cannot throw.
	2 * sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) + 3 * sizeof(void *) +
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// _S_use_relocate calls are required constant expressions. Actual true
	// _S_relocate→relocate_a→relocate_a_1 byte memmove, three niter_base
	// pointer argument/results, signed count and false const-evaluation B.
	3 * (5 * sizeof(void *)) + 3 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) +
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// Exact old byte buffer deallocation and sized delete.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t);
// All original native template instantiations are u16/u32/i32/u64.
// They share one actual push graph; each genuine caller template scope has
// facts-vector reference,value,index,cast-byte. No guessed largest scalar.
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_native_number_source =
	4 * (sizeof(void *) + sizeof(size_t) + sizeof(uint8_t)) + sizeof(uint16_t) +
	sizeof(uint32_t) + sizeof(int32_t) + sizeof(uint64_t) +
	auction_reconstruction_fixed_byte_fitting_push +
	auction_reconstruction_fixed_byte_growth_push;
}

namespace
{
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_aec1_decode_source =
	// Real temporary array4 bytes, cleanup receiver, begin()->direct data.
	4 * sizeof(uint8_t) + sizeof(void *) + 2 * (2 * sizeof(void *)) +
	// Normal-span equal: four actual equal bodies, niter/base wrappers,
	// two compile-time bool locals, signed length and memcmp declaration.
	4 * (3 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(bool) + sizeof(std::ptrdiff_t) +
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) +
	2 * (sizeof(auction_fact_iterator) + sizeof(void *) + 2 * sizeof(void *) +
	     2 * sizeof(void *) + sizeof(void *)) +
	2 * sizeof(void *);
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_facts_append_source =
	// Original append_u64/u32 loops: ref/value/index/cast-byte. u16 has
	// two cast byte temporary sites and no loop index. Fitting push once.
	2 * (sizeof(void *) + sizeof(size_t) + sizeof(uint8_t)) + sizeof(uint64_t) +
	sizeof(uint32_t) + sizeof(void *) + sizeof(uint16_t) + 2 * sizeof(uint8_t) +
	auction_reconstruction_fixed_byte_fitting_push;
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_facts_vector_source =
	auction_reconstruction_fixed_byte_reserve + auction_fact_returned_vector_move_source +
	auction_reconstruction_fixed_vector_default<uint8_t> +
	auction_reconstruction_fixed_vector_cleanup<uint8_t> +
	auction_reconstruction_fixed_vector_move<uint8_t>;
[[maybe_unused]] constexpr size_t auction_reconstruction_fixed_native_owned_source =
	auction_reconstruction_fixed_native_lifetime_source +
	auction_reconstruction_fixed_native_helper_source +
	auction_reconstruction_fixed_native_forecast_source +
	auction_reconstruction_fixed_native_number_source + auction_insert_end_any_complete_source;
}

// PRIVATE complete reconstruction budget ownership body; pure Source maps
// remain draft until exact selected primitive/family joins are reviewed.
namespace
{
struct auction_reconstruction_fixed_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, source, entry_inline;
	size_t native_inline = 0;
	const economic_frozen_intent *intent = nullptr;
	const economic_admission_facts *admission = nullptr;
	const critical_command *projection = nullptr;
	const std::vector<uint8_t> *expected = nullptr;
	const auction_native_command_context *native = nullptr;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &b = *static_cast<auction_reconstruction_fixed_budget *>(opaque);
		if (b.denied || !b.reserve || !b.reserve(amount, b.context))
		{
			b.denied = true;
			return false;
		}
		return true;
	}
	bool prefix(size_t &total, size_t extra = 0) noexcept
	{
		total = outer;
		size_t heap = 0;
		if (denied || !auction_codec_add(total, source) ||
		    !auction_codec_add(total, entry_inline) ||
		    !auction_codec_add(total, sizeof(*this)) ||
		    !auction_codec_add(total, native_inline) ||
		    (intent && !auction_codec_add(total, intent->admission.facts.capacity())) ||
		    (admission && !auction_codec_add(total, admission->facts.capacity())) ||
		    (expected && !auction_codec_add(total, expected->capacity())) ||
		    (projection && (!critical_command_current_heap_bytes(*projection, &heap) ||
				    !auction_codec_add(total, heap))) ||
		    (native && (native->before_item_uids.capacity() > SIZE_MAX / sizeof(uint64_t) ||
				!auction_codec_add(total, native->base_v1_payload.capacity()) ||
				!auction_codec_add(total, native->before_item_uids.capacity() *
								  sizeof(uint64_t)))) ||
		    !auction_codec_add(total, extra))
		{
			denied = true;
			return false;
		}
		return true;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && forward(total, this);
	}
};
}

namespace
{
[[maybe_unused]] constexpr size_t auction_item_claim_fixed_predicate_source =
	// valid_accounts(wallet,bank,racewar;bool) and selected key/equal leaves.
	2 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool) +
	auction_reconstruction_fixed_key_valid_source + auction_reconstruction_fixed_array_equal +
	// valid_claim(payload,claim;bool),staged_claimant32,index/previous,
	// row and item refs; actual two distinct const array specializations.
	2 * sizeof(void *) + sizeof(bool) + sizeof(uint32_t) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 2 * auction_reconstruction_fixed_array_index_source +
	auction_reconstruction_fixed_id_equal_source;
[[maybe_unused]] constexpr size_t auction_item_claim_fixed_facts_source =
	// frozen_facts(wallet,bank,claim), row loop/index/ref and claimed byte.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(uint8_t) +
	auction_reconstruction_fixed_array_index_source +
	auction_reconstruction_fixed_facts_append_source +
	auction_reconstruction_fixed_facts_vector_source +
	2 * auction_reconstruction_fixed_array_id_range_source +
	auction_insert_end_fitting_complete_source;
[[maybe_unused]] constexpr size_t auction_item_claim_fixed_intent_source =
	8 * sizeof(void *) + 8 * sizeof(size_t) + 2 * sizeof(error) + sizeof(void *) +
	auction_reconstruction_fixed_source_assignment_source +
	auction_reconstruction_fixed_metadata_assignment_source +
	auction_reconstruction_fixed_optional_source +
	auction_reconstruction_fixed_admission_lifetime_source +
	(1 + 7 + AUCTION_COMMAND_MAX_ITEMS) * sizeof(void *) +
	auction_item_claim_fixed_predicate_source + auction_item_claim_fixed_facts_source +
	auction_reconstruction_fixed_native_owned_source;
[[maybe_unused]] constexpr size_t auction_item_claim_fixed_decode_source =
	auction_reconstruction_fixed_decode_scalar_source +
	// This public decoder has eight rather than seven pointer formals.
	sizeof(void *) + auction_reconstruction_fixed_item_read_number_source +
	auction_reconstruction_fixed_claim_state_source +
	auction_reconstruction_fixed_two_returned_accounts_source +
	auction_fact_copy_n_caller_source + auction_fact_copy_n_int_source +
	// Actual count16,row loop index/offset/entry reference and mutable[] leaf.
	sizeof(uint16_t) + 2 * sizeof(size_t) + sizeof(void *) +
	auction_reconstruction_fixed_array_index_source +
	auction_reconstruction_fixed_native_lifetime_source;
}

bool auction_item_claim_accounting_intent_own_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !auction_codec_policy())
		return false;
	*output = auction_item_claim_fixed_intent_source +
		  auction_reconstruction_fixed_budget_source +
		  auction_reconstruction_fixed_child_entry_source +
		  auction_reconstruction_fixed_payload_helper_source;
	return true;
}
bool auction_item_claim_accounting_intent_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !auction_codec_policy())
		return false;
	*output = sizeof(auction_reconstruction_fixed_budget) + sizeof(auction_command_payload) +
		  sizeof(economic_admission_facts) + sizeof(economic_source_event) +
		  sizeof(std::vector<uint8_t>);
	return true;
}

bool auction_item_claim_accounting_decode_own_source_frame_bytes(size_t *output) noexcept
{
	size_t observed = 0, total = 0, critical_source = 0, valid_source = 0;
	if (!output || !auction_codec_policy() ||
	    !critical_command_current_heap_observer_frame_bytes(&observed))
		return false;
	critical_source = critical_command_copy_frame_bytes();
	valid_source = critical_command_valid_frame_bytes();
	if (valid_source > critical_source)
		critical_source = valid_source;
	if (observed > critical_source)
		critical_source = observed;
	// The actual envelope/copy/CURRENT phases are sequential, and their
	// returned primitive closures overlap. Own complete caller scopes above.
	total = auction_item_claim_fixed_decode_source +
		auction_reconstruction_fixed_budget_source +
		auction_reconstruction_fixed_child_entry_source +
		auction_reconstruction_fixed_payload_helper_source;
	if (!auction_codec_add(total, critical_source))
		return false;
	*output = total;
	return true;
}
bool auction_item_claim_accounting_decode_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !auction_codec_policy())
		return false;
	*output = sizeof(auction_reconstruction_fixed_budget) + sizeof(auction_command_payload) +
		  sizeof(economic_frozen_intent) + sizeof(auction_item_claim_state) +
		  2 * sizeof(economic_account_key) + sizeof(critical_command) +
		  sizeof(std::vector<uint8_t>);
	return true;
}

[[maybe_unused]] bool
auction_reconstruction_native_extension_peak(const std::vector<uint8_t> &facts,
					     auction_reconstruction_fixed_budget &budget) noexcept
{
	size_t size = facts.size(), capacity = facts.capacity(), largest = 0, request = 0;
	// Exact original ANF2 append sequence: integer bytes use individual
	// push_back; each digest uses one forward insert of 32 actual bytes.
	constexpr size_t runs[] = { 4, 2, 2, 4, 8, 32, 32, 32, 4, 2, 2 };
	for (size_t run = 0; run < std::size(runs); ++run)
	{
		const bool block = run >= 5 && run <= 7;
		const size_t count = block ? runs[run] : 1;
		const size_t iterations = block ? 1 : runs[run];
		for (size_t i = 0; i < iterations; ++i)
		{
			if (!auction_codec_growth_peak(size, capacity, count, request))
				return false;
			if (request)
			{
				size_t simultaneous = capacity;
				if (!auction_codec_add(simultaneous, request))
					return false;
				if (simultaneous > largest)
					largest = simultaneous;
				capacity = request;
			}
			if (!auction_codec_add(size, count))
				return false;
		}
	}
	// prefix already owns the actual old facts allocation; replace that term
	// by the largest authentic simultaneous old/new growth request.
	const size_t extra = largest > facts.capacity() ? largest - facts.capacity() : 0;
	// Exact original runs/loop/growth Source persists in the new own
	// inventory. Heap forecast replaces only the actual buffer ownership.
	return budget.peak(extra);
}

namespace
{
enum class auction_fixed_child : uint8_t
{
	payload,
	native,
	intent_decode,
	intent_proof,
	intent_freeze
};
bool auction_fixed_child_entry(auction_reconstruction_fixed_budget &budget,
			       auction_fixed_child kind, size_t &nested) noexcept
{
	size_t source = 0, initial = 0, supplement = 0;
	bool admitted = false;
	if (budget.denied)
		return false;
	switch (kind)
	{
	case auction_fixed_child::payload:
	{
		constexpr size_t query = auction_command_decode_payload_source_query_frame_bytes();
		admitted =
			budget.peak(query) &&
			auction_command_decode_payload_source_frame_bytes(&source) &&
			auction_command_decode_payload_initial_inline_bytes(&initial) &&
			auction_command_decode_payload_source_supplement_frame_bytes(&supplement);
		break;
	}
	case auction_fixed_child::native:
	{
		constexpr size_t query = auction_native_command_decode_source_query_frame_bytes();
		admitted = budget.peak(query) &&
			   auction_native_command_decode_source_frame_bytes(&source) &&
			   auction_native_command_decode_initial_inline_bytes(&initial) &&
			   auction_native_command_decode_source_supplement_frame_bytes(&supplement);
		break;
	}
	case auction_fixed_child::intent_decode:
	{
		constexpr size_t query = economic_intent_decode_source_query_frame_bytes();
		admitted = budget.peak(query) &&
			   economic_intent_decode_source_frame_bytes(&source) &&
			   economic_intent_decode_initial_inline_bytes(&initial) &&
			   economic_intent_decode_source_supplement_frame_bytes(&supplement);
		break;
	}
	case auction_fixed_child::intent_proof:
	{
		constexpr size_t query =
			economic_intent_verify_binding_fixed_source_query_frame_bytes();
		admitted = budget.peak(query) &&
			   economic_intent_verify_binding_fixed_source_frame_bytes(&source) &&
			   economic_intent_verify_binding_fixed_initial_inline_bytes(&initial);
		break;
	}
	case auction_fixed_child::intent_freeze:
	{
		constexpr size_t query = economic_intent_freeze_fixed_source_query_frame_bytes();
		admitted = budget.peak(query) &&
			   economic_intent_freeze_fixed_source_frame_bytes(&source) &&
			   economic_intent_freeze_fixed_initial_inline_bytes(&initial) &&
			   economic_intent_freeze_fixed_source_supplement_frame_bytes(&supplement);
		break;
	}
	}
	if (!admitted || !auction_codec_add(source, initial) || !budget.peak(source) ||
	    !budget.prefix(nested) || !auction_codec_add(nested, supplement))
	{
		// Only actual query/policy/arithmetic/admission refusal is sticky.
		// A completed child error (including caught allocation capacity)
		// does not set this marker and keeps original semantic mapping.
		budget.denied = true;
		return false;
	}
	return true;
}
}

namespace
{
bool auction_codec_payload_fixed(const critical_command &command, auction_command_payload *out,
				 auction_reconstruction_fixed_budget &budget,
				 bool native_allowed) noexcept
{
	size_t nested = 0;
	// Keep this genuine helper's params/locals/result carriers live across
	// every nested absolute callback, including the original nonnative path.
	if (!budget.peak())
		return false;
	if (!budget.prefix(nested))
		return false;
	if (native_allowed && command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
	{
		// Original decode_payload constructs its separate native context.
		budget.native_inline = sizeof(auction_native_command_context);
		if (!budget.peak())
			return false;
		auction_native_command_context native;
		budget.native = &native;
		if (!auction_fixed_child_entry(budget, auction_fixed_child::native, nested))
			return false;
		const auto status = auction_native_command_decode_fixed_bounded(
			command, &native, auction_reconstruction_fixed_budget::forward, &budget,
			nested);
		if (status != error::ok)
			return false;
		*out = native.payload;
		budget.native = nullptr;
		budget.native_inline = 0;
		return true;
	}
	if (!auction_fixed_child_entry(budget, auction_fixed_child::payload, nested))
		return false;
	const bool result = auction_command_decode_payload_fixed_bounded(
		command, out, auction_reconstruction_fixed_budget::forward, &budget, nested,
		&budget.denied);
	return result;
}
error append_native_facts_fixed_bounded(const critical_command &command,
					std::vector<uint8_t> *facts,
					auction_reconstruction_fixed_budget &budget)
{
	if (command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
		return error::ok;
	size_t nested = 0;
	// Original helper's hash initializer array and loop/pointers coexist with
	// its real native context; the payload DTO is already inline in context.
	budget.native_inline = sizeof(auction_native_command_context);
	if (!budget.peak())
		return error::capacity;
	auction_native_command_context native;
	budget.native = &native;
	if (!auction_fixed_child_entry(budget, auction_fixed_child::native, nested))
		return error::capacity;
	const auto decoded = auction_native_command_decode_fixed_bounded(
		command, &native, auction_reconstruction_fixed_budget::forward, &budget, nested);
	if (decoded != error::ok)
		return budget.denied ? error::capacity : error::corrupt_evidence;
	if (native.selected_node_count > ECONOMIC_ACCOUNTING_MAX_ITEM_EVENTS)
		return error::capacity;
	if (!auction_reconstruction_native_extension_peak(*facts, budget))
		return error::capacity;
	native_fact_append(*facts, native_fact_magic);
	native_fact_append(*facts, uint16_t{ 2 });
	native_fact_append(*facts, uint16_t{ 0 });
	native_fact_append(*facts, native.original_level);
	native_fact_append(*facts, native.acknowledged_save_revision);
	for (const auto *hash :
	     { &native.before_digest, &native.after_digest, &native.selected_digest })
		facts->insert(facts->end(), hash->begin(), hash->end());
	native_fact_append(*facts, native.selected_node_count);
	native_fact_append(*facts, native.selected_root_count);
	native_fact_append(*facts, uint16_t{ 0 });
	budget.native = nullptr;
	budget.native_inline = 0;
	return error::ok;
}
}

// PRIVATE algorithm candidate. Complete own/lower SOURCE profiles remain OPEN.

// PRIVATE owned Source/inline entry body. Complete Source maps remain unselected.
economic_accounting_error auction_item_claim_accounting_intent_fixed_bounded(
	const critical_command &command, const critical_operation_id &epoch,
	const economic_account_key &wallet, const economic_account_key &bank,
	const auction_item_claim_state &claim, std::vector<uint8_t> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	size_t own_source = 0, entry_inline = 0, nested = 0, query_peak = outer_live;
	constexpr size_t query =
		auction_item_claim_accounting_intent_own_source_query_frame_bytes();
	constexpr size_t entry_source =
		8 * sizeof(void *) + 7 * sizeof(size_t) + sizeof(bool) + sizeof(error) +
		// Actual checked-add query construction before first reserve.
		sizeof(void *) + sizeof(size_t) + sizeof(bool);
	if (!reserve || !auction_codec_add(query_peak, entry_source) ||
	    !auction_codec_add(query_peak, query) || !reserve(query_peak, context) ||
	    !auction_item_claim_accounting_intent_own_source_frame_bytes(&own_source) ||
	    !auction_item_claim_accounting_intent_initial_inline_bytes(&entry_inline))
		return error::capacity;
	size_t entry_peak = outer_live;
	if (!auction_codec_add(entry_peak, own_source) ||
	    !auction_codec_add(entry_peak, entry_inline) || !reserve(entry_peak, context))
		return error::capacity;
	auction_reconstruction_fixed_budget budget{
		reserve, context, outer_live, own_source,
		entry_inline - sizeof(auction_reconstruction_fixed_budget)
	};

	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!auction_codec_payload_fixed(command, &payload, budget, true))
		return budget.denied ? error::capacity : error::corrupt_evidence;
	if (!valid_accounts(wallet, bank, payload.racewar) || !valid_claim(payload, claim) ||
	    critical_operation_id_equal(command.operation_id, claim.listing_operation) ||
	    critical_operation_id_equal(command.operation_id, claim.claim_source_operation))
		return budget.denied ? error::capacity : error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		budget.admission = &facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.original_operation_id = claim.listing_operation;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_AUCTION_ITEM_CLAIM;
		facts.metadata.reason = economic_reason::auction_claim;
		facts.metadata.source_event = source_for(claim);
		if (!budget.peak(82 + claim.item_count * 27))
			return error::capacity;
		facts.facts = frozen_facts(wallet, bank, claim);
		const auto native_status =
			append_native_facts_fixed_bounded(command, &facts.facts, budget);
		if (native_status != error::ok)
			return native_status;
		if (!auction_fixed_child_entry(budget, auction_fixed_child::intent_freeze, nested))
			return error::capacity;
		return economic_intent_freeze_fixed_bounded(
			command, facts, encoded, auction_reconstruction_fixed_budget::forward,
			&budget, nested);
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_item_claim_accounting_decode_fixed_bounded(
	const critical_command &command, economic_frozen_intent *intent,
	auction_command_payload *payload, auction_item_claim_state *claim,
	economic_account_key *wallet, economic_account_key *bank,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	size_t own_source = 0, entry_inline = 0, nested = 0, query_peak = outer_live;
	constexpr size_t query =
		auction_item_claim_accounting_decode_own_source_query_frame_bytes();
	constexpr size_t entry_source =
		8 * sizeof(void *) + 7 * sizeof(size_t) + sizeof(bool) + sizeof(error) +
		// Actual checked-add query construction before first reserve.
		sizeof(void *) + sizeof(size_t) + sizeof(bool);
	if (!reserve || !auction_codec_add(query_peak, entry_source) ||
	    !auction_codec_add(query_peak, query) || !reserve(query_peak, context) ||
	    !auction_item_claim_accounting_decode_own_source_frame_bytes(&own_source) ||
	    !auction_item_claim_accounting_decode_initial_inline_bytes(&entry_inline))
		return error::capacity;
	size_t entry_peak = outer_live;
	if (!auction_codec_add(entry_peak, own_source) ||
	    !auction_codec_add(entry_peak, entry_inline) || !reserve(entry_peak, context))
		return error::capacity;
	auction_reconstruction_fixed_budget budget{
		reserve, context, outer_live, own_source,
		entry_inline - sizeof(auction_reconstruction_fixed_budget)
	};

	if (!intent || !payload || !claim || !wallet || !bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!auction_codec_payload_fixed(command, &parsed_payload, budget, true) ||
		    parsed_payload.action != auction_action::claim_item)
			return budget.denied ? error::capacity : error::invalid_identity;
		economic_frozen_intent parsed_intent;
		budget.intent = &parsed_intent;
		if (!auction_fixed_child_entry(budget, auction_fixed_child::intent_decode, nested))
			return error::capacity;
		const auto decoded = economic_intent_decode_bounded(
			command.accounting_intent, &parsed_intent,
			auction_reconstruction_fixed_budget::forward, &budget, nested);
		if (decoded != error::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		if (!auction_fixed_child_entry(budget, auction_fixed_child::intent_proof, nested))
			return error::capacity;
		const auto binding = economic_intent_verify_binding_fixed_bounded(
			command, parsed_intent, auction_reconstruction_fixed_budget::forward,
			&budget, nested);
		if (binding != error::ok)
			return budget.denied ? error::capacity : error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() < claim_fact_header_bytes)
			return budget.denied ? error::capacity : error::invalid_identity;
		const uint16_t count = read_number<uint16_t>(facts, 80);
		if (!count || count > AUCTION_COMMAND_MAX_ITEMS ||
		    facts.size() !=
			    claim_fact_header_bytes + count * claim_fact_row_bytes +
				    (command.payload_version ==
						     AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ?
					     native_fact_extension_bytes :
					     0))
			return budget.denied ? error::capacity : error::invalid_identity;
		const auto &lineage = parsed_intent.admission.metadata.lineage;
		const economic_account_key parsed_wallet = { lineage, economic_account_kind::wallet,
							     read_number<uint64_t>(facts, 0), 0 };
		const economic_account_key parsed_bank = { lineage, economic_account_kind::bank,
							   read_number<uint64_t>(facts, 8),
							   parsed_payload.racewar };
		auction_item_claim_state parsed_claim;
		parsed_claim.auction_id = read_number<uint32_t>(facts, 16);
		parsed_claim.seller_pid = read_number<uint32_t>(facts, 20);
		parsed_claim.winner_pid = read_number<uint32_t>(facts, 24);
		parsed_claim.claimant_pid = read_number<uint32_t>(facts, 28);
		parsed_claim.status = read_number<uint32_t>(facts, 32);
		parsed_claim.custody_state = read_number<uint32_t>(facts, 36);
		parsed_claim.auction_revision = read_number<uint64_t>(facts, 40);
		std::copy_n(facts.begin() + 48, 16, parsed_claim.listing_operation.bytes.begin());
		std::copy_n(facts.begin() + 64, 16,
			    parsed_claim.claim_source_operation.bytes.begin());
		parsed_claim.item_count = count;
		for (size_t index = 0; index < count; ++index)
		{
			const size_t offset =
				claim_fact_header_bytes + index * claim_fact_row_bytes;
			auto &entry = parsed_claim.rows[index];
			entry.uid = read_number<uint64_t>(facts, offset);
			entry.revision = read_number<uint64_t>(facts, offset + 8);
			entry.slot = read_number<uint16_t>(facts, offset + 16);
			entry.vnum =
				static_cast<int32_t>(read_number<uint32_t>(facts, offset + 18));
			entry.claim_pid = read_number<uint32_t>(facts, offset + 22);
			if (facts[offset + 26] > 1)
				return budget.denied ? error::capacity : error::invalid_identity;
			entry.claimed = facts[offset + 26] == 1;
		}
		size_t copy_request = 0;
		if (!critical_command_fresh_copy_request_bytes(command, &copy_request) ||
		    !auction_codec_add(copy_request, critical_command_copy_frame_bytes()) ||
		    !budget.peak(copy_request))
			return error::capacity;
		critical_command projected = command;
		budget.projection = &projected;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		budget.expected = &expected;
		if (!budget.prefix(nested))
			return error::capacity;
		const auto frozen = auction_item_claim_accounting_intent_fixed_bounded(
			projected, parsed_intent.admission.metadata.epoch, parsed_wallet,
			parsed_bank, parsed_claim, &expected,
			auction_reconstruction_fixed_budget::forward, &budget, nested);
		if (frozen != error::ok || expected != command.accounting_intent)
			return budget.denied ? error::capacity : error::unauthorized;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*claim = parsed_claim;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

// Complete actual sequential-child Source candidate; review pending.
bool auction_item_claim_accounting_intent_source_frame_bytes(size_t *output) noexcept
{
	size_t own = 0, child0 = 0, child1 = 0, child2 = 0, total = 0;
	if (!output || !auction_item_claim_accounting_intent_own_source_frame_bytes(&own) ||
	    !auction_command_decode_payload_source_frame_bytes(&child0) ||
	    !auction_native_command_decode_source_frame_bytes(&child1) ||
	    !economic_intent_freeze_fixed_source_frame_bytes(&child2))
		return false;
	total = child0;
	if (child1 > total)
		total = child1;
	if (child2 > total)
		total = child2;
	if (!auction_codec_add(total, own))
		return false;
	*output = total;
	return true;
}
bool auction_item_claim_accounting_intent_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !auction_codec_policy())
		return false;
	*output = 0;
	return true;
}
bool auction_item_claim_accounting_decode_source_frame_bytes(size_t *output) noexcept
{
	size_t own = 0, child0 = 0, child1 = 0, child2 = 0, child3 = 0, child4 = 0, total = 0;
	if (!output || !auction_item_claim_accounting_decode_own_source_frame_bytes(&own) ||
	    !auction_command_decode_payload_source_frame_bytes(&child0) ||
	    !auction_native_command_decode_source_frame_bytes(&child1) ||
	    !economic_intent_decode_source_frame_bytes(&child2) ||
	    !economic_intent_verify_binding_fixed_source_frame_bytes(&child3) ||
	    !auction_item_claim_accounting_intent_source_frame_bytes(&child4))
		return false;
	total = child0;
	if (child1 > total)
		total = child1;
	if (child2 > total)
		total = child2;
	if (child3 > total)
		total = child3;
	if (child4 > total)
		total = child4;
	if (!auction_codec_add(total, own))
		return false;
	*output = total;
	return true;
}
bool auction_item_claim_accounting_decode_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !auction_codec_policy())
		return false;
	*output = 0;
	return true;
}
