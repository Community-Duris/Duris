#include "economy/auction_listing_accounting.h"
#include "economy/auction_native_command_context.h"
#include <openssl/sha.h>
#include <unordered_set>

#include "economy/auction_accounting.h"

#include <algorithm>
#include <climits>
#include <new>
#include <span>
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

bool accounts_valid(const economic_account_key &wallet, const economic_account_key &bank,
		    uint8_t racewar)
{
	return economic_account_key_valid(wallet) && economic_account_key_valid(bank) &&
	       wallet.kind == economic_account_kind::wallet && !wallet.context_id &&
	       bank.kind == economic_account_kind::bank && bank.context_id == racewar &&
	       wallet.lineage.bytes == bank.lineage.bytes &&
	       wallet.authority_id != bank.authority_id;
}

bool listing_valid(const auction_command_payload &payload)
{
	if (payload.action != auction_action::list || !payload.actor_pid || payload.auction_id ||
	    !payload.item_count || payload.listing_fee < 0 || payload.listing_fee > UINT_MAX ||
	    payload.start_price < 0 || payload.start_price > UINT_MAX || payload.buy_price < 0 ||
	    payload.buy_price > UINT_MAX ||
	    (payload.buy_price && payload.buy_price < payload.start_price) ||
	    !payload.object_blob_size)
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &item = payload.items[index];
		if (!item.item_uid || item.expected_item_revision == UINT64_MAX)
			return false;
		for (size_t previous = 0; previous < index; ++previous)
			if (payload.items[previous].item_uid == item.item_uid)
				return false;
	}
	return true;
}

std::vector<uint8_t> facts_for(const economic_account_key &wallet, const economic_account_key &bank)
{
	std::vector<uint8_t> facts;
	facts.reserve(16);
	for (uint64_t value : { wallet.authority_id, bank.authority_id })
		for (size_t byte = 0; byte < 8; ++byte)
			facts.push_back(static_cast<uint8_t>(value >> (byte * 8)));
	return facts;
}

economic_source_event source_for(const critical_command &command)
{
	return { economic_source_kind::auction, command.operation_id, command.operation_id, 0, 0 };
}

economic_coin_vector copper(int64_t amount)
{
	return { amount, 0, 0, 0 };
}

economic_coin_vector canonical_wallet(int64_t amount)
{
	economic_coin_vector coins = {};
	constexpr int64_t units[] = { 1, 10, 100, 1000 };
	for (size_t index = coins.size(); index-- > 0;)
	{
		coins[index] = amount / units[index];
		amount %= units[index];
	}
	return coins;
}
} // namespace

economic_accounting_error
auction_listing_accounting_observe_native_facts(const economic_frozen_intent &intent,
						auction_accounting_native_facts *out) noexcept
{
	const auto &facts = intent.admission.facts;
	if (!out || intent.admission.facts_version != 1 ||
	    intent.admission.metadata.writer_id != ECONOMIC_WRITER_AUCTION_LISTING ||
	    intent.admission.metadata.reason != economic_reason::auction_listing)
		return error::invalid_identity;
	if (facts.size() == 16)
		return error::invalid_version;
	if (facts.size() != 16 + native_fact_extension_bytes)
		return error::corrupt_evidence;
	return observe_native_facts(std::span<const uint8_t>(facts).subspan(16), out);
}

economic_accounting_error auction_listing_accounting_intent(const critical_command &command,
							    const critical_operation_id &epoch,
							    const economic_account_key &wallet,
							    const economic_account_key &bank,
							    std::vector<uint8_t> *encoded)
{
	if (!encoded || command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION ||
	    critical_operation_id_is_zero(epoch))
		return error::invalid_version;
	auction_command_payload payload = {};
	if (!decode_payload(command, &payload))
		return error::corrupt_evidence;
	if (!listing_valid(payload) || !accounts_valid(wallet, bank, payload.racewar))
		return error::invalid_identity;
	try
	{
		economic_admission_facts facts;
		facts.metadata.lineage = wallet.lineage;
		facts.metadata.epoch = epoch;
		facts.metadata.actor_kind = economic_actor_kind::domain;
		facts.metadata.actor_id = payload.actor_pid;
		facts.metadata.writer_id = ECONOMIC_WRITER_AUCTION_LISTING;
		facts.metadata.reason = economic_reason::auction_listing;
		facts.metadata.source_event = source_for(command);
		facts.facts = facts_for(wallet, bank);
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

economic_accounting_error auction_listing_accounting_decode(const critical_command &command,
							    economic_frozen_intent *intent,
							    auction_command_payload *payload,
							    economic_account_key *wallet,
							    economic_account_key *bank)
{
	if (!intent || !payload || !wallet || !bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return error::invalid_version;
	try
	{
		auction_command_payload parsed_payload = {};
		if (!decode_payload(command, &parsed_payload) ||
		    parsed_payload.action != auction_action::list)
			return error::invalid_identity;
		economic_frozen_intent parsed_intent;
		if (economic_intent_decode(command.accounting_intent, &parsed_intent) !=
			    error::ok ||
		    economic_intent_verify_binding(command, parsed_intent) != error::ok)
			return error::corrupt_evidence;
		const auto facts = std::span<const uint8_t>(parsed_intent.admission.facts);
		if (facts.size() !=
		    16 + (command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ?
				  native_fact_extension_bytes :
				  0))
			return error::invalid_identity;
		const auto number = [&](size_t offset)
		{
			uint64_t value = 0;
			for (size_t byte = 0; byte < 8; ++byte)
				value |= static_cast<uint64_t>(facts[offset + byte]) << (byte * 8);
			return value;
		};
		const auto &lineage = parsed_intent.admission.metadata.lineage;
		const economic_account_key parsed_wallet = { lineage, economic_account_kind::wallet,
							     number(0), 0 };
		const economic_account_key parsed_bank = { lineage, economic_account_kind::bank,
							   number(8), parsed_payload.racewar };
		critical_command projected = command;
		projected.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		projected.accounting_intent.clear();
		projected.publication_required = false;
		std::vector<uint8_t> expected;
		const auto frozen = auction_listing_accounting_intent(
			projected, parsed_intent.admission.metadata.epoch, parsed_wallet,
			parsed_bank, &expected);
		if (frozen != error::ok || expected != command.accounting_intent)
			return error::unauthorized;
		*intent = std::move(parsed_intent);
		*payload = parsed_payload;
		*wallet = parsed_wallet;
		*bank = parsed_bank;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
}

economic_accounting_error auction_listing_accounting_plan(
	const critical_command &command, const economic_frozen_intent &intent,
	const auction_listing_accounting_authority &authority, const auction_command_result &result,
	economic_accounting_plan *plan)
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
		if (!listing_valid(payload) ||
		    !accounts_valid(authority.wallet, authority.bank, payload.racewar) ||
		    !economic_account_key_valid(authority.escrow) ||
		    authority.escrow.kind != economic_account_kind::auction_escrow ||
		    authority.escrow.context_id ||
		    authority.escrow.lineage.bytes != authority.wallet.lineage.bytes ||
		    authority.escrow.authority_id == authority.wallet.authority_id ||
		    authority.escrow.authority_id == authority.bank.authority_id)
			return error::invalid_identity;
		const auto &meta = intent.admission.metadata;
		const auto source = source_for(command);
		auto expected_facts = facts_for(authority.wallet, authority.bank);
		status = append_native_facts(command, &expected_facts);
		if (status != error::ok)
			return status;
		if (meta.writer_id != ECONOMIC_WRITER_AUCTION_LISTING ||
		    meta.reason != economic_reason::auction_listing ||
		    meta.actor_kind != economic_actor_kind::domain ||
		    meta.actor_id != payload.actor_pid || !meta.source_event ||
		    meta.source_event->kind != source.kind ||
		    meta.source_event->source.bytes != source.source.bytes ||
		    meta.source_event->generation.bytes != source.generation.bytes ||
		    meta.source_event->sequence != source.sequence ||
		    meta.source_event->slot != source.slot ||
		    meta.lineage.bytes != authority.wallet.lineage.bytes ||
		    meta.epoch.bytes != authority.epoch.bytes ||
		    !critical_operation_id_is_zero(meta.original_operation_id) ||
		    intent.admission.facts != expected_facts)
			return error::unauthorized;
		if (authority.items_before.size() !=
			    (command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ?
				     authority.native_selected_literals.size() :
				     payload.item_count) ||
		    authority.balances_before.wallet_revision != payload.expected_wallet_revision ||
		    authority.balances_before.bank_revision != payload.expected_bank_revision ||
		    authority.balances_before.wallet_revision == UINT64_MAX ||
		    authority.balances_before.bank_revision == UINT64_MAX ||
		    authority.player_owner_revision_before == UINT64_MAX)
			return error::stale_revision;
		int64_t wallet_before = 0;
		status = economic_coin_value(authority.balances_before.wallet.amount,
					     &wallet_before);
		if (status != error::ok)
			return status;
		if (wallet_before < payload.listing_fee)
			return error::negative_holding;
		if (result.action != auction_action::list ||
		    result.event_type != auction_event_type::listed || !result.auction_id ||
		    result.status != 1 || result.seller_pid != payload.actor_pid ||
		    result.winner_pid || result.previous_bidder_pid || result.final_price ||
		    result.wallet_value_delta != -payload.listing_fee ||
		    result.wallet.amount != canonical_wallet(wallet_before - payload.listing_fee) ||
		    result.bank.amount != authority.balances_before.bank.amount ||
		    result.wallet_revision != authority.balances_before.wallet_revision + 1 ||
		    result.bank_revision != authority.balances_before.bank_revision + 1 ||
		    result.auction_revision != 1 ||
		    result.player_owner_revision != authority.player_owner_revision_before + 1 ||
		    result.auction_owner_revision != 1 || result.item_count != payload.item_count)
			return error::corrupt_evidence;
		economic_accounting_plan candidate;
		status = economic_intent_plan_metadata(command, intent, &candidate.metadata);
		if (status != error::ok)
			return status;
		candidate.accounts.push_back(
			{ authority.wallet, authority.balances_before.wallet.amount,
			  result.wallet.amount, authority.balances_before.wallet_revision,
			  result.wallet_revision });
		candidate.accounts.push_back({ authority.escrow, {}, {}, 0, 1 });
		economic_coin_vector delta = {};
		status = economic_coin_delta(candidate.accounts[0].before,
					     candidate.accounts[0].after, &delta);
		if (status != error::ok)
			return status;
		if (std::any_of(delta.begin(), delta.end(), [](int64_t part) { return part != 0; }))
			candidate.postings.push_back({ 0, 0, 0, delta, -payload.listing_fee });
		if (payload.listing_fee)
		{
			candidate.accounts.push_back(
				{ { authority.wallet.lineage, economic_account_kind::sink,
				    ECONOMIC_AUCTION_LISTING_FEE_SINK_ID, 0 },
				  {},
				  {},
				  0,
				  0 });
			candidate.postings.push_back(
				{ 1, 2, 0, copper(payload.listing_fee), payload.listing_fee });
		}

		if (command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
		{
			std::vector<size_t> roots;
			status = native_items(command, payload, authority.native_selected_literals,
					      authority.items_before, item_owner_type::player,
					      payload.actor_pid, &roots);
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
				after.owner = { item_owner_type::auction, result.auction_id, 0 };
				after.revision++;
				candidate.items_before.push_back(item);
				candidate.items_after.push_back({ item.uid, after });
				candidate.item_events.push_back({ static_cast<uint32_t>(i), 0,
								  item.uid, item.position, after });
			}
		}
		else
		{
			for (size_t index = 0; index < payload.item_count; ++index)
			{
				const auto &item = authority.items_before[index];
				const auto &position = item.position;
				if (item.uid != payload.items[index].item_uid ||
				    position.owner.type != item_owner_type::player ||
				    position.owner.id != payload.actor_pid ||
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
				after.owner = { item_owner_type::auction, result.auction_id, 0 };
				after.revision++;
				candidate.items_before.push_back(item);
				candidate.items_after.push_back({ item.uid, after });
				candidate.item_events.push_back({ static_cast<uint32_t>(index), 0,
								  item.uid, position, after });
			}
		}
		for (size_t index = payload.item_count; index < AUCTION_COMMAND_MAX_ITEMS; ++index)
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
