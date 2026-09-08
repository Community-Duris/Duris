#include "economy/coin_transfer_command.h"

#include "economy/currency_command.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot_codec.h"
#include "world/vnum.obj.h"
#include "core/structs.h"

#include <algorithm>
#include <new>
#include <utility>

namespace
{
constexpr uint32_t COIN_OPERATION_DOMAIN = 0x434f494e;
constexpr size_t ENDPOINT_HEADER_BYTES = 36;

void append_u32(std::vector<uint8_t> *output, uint32_t value)
{
	for (unsigned int byte = 0; byte < 4; ++byte)
		output->push_back(static_cast<uint8_t>(value >> (byte * 8)));
}

uint32_t read_u32(const uint8_t *input)
{
	uint32_t value = 0;
	for (unsigned int byte = 0; byte < 4; ++byte)
		value |= static_cast<uint32_t>(input[byte]) << (byte * 8);
	return value;
}

int64_t value(const std::array<int32_t, 4> &amounts)
{
	constexpr int64_t denominations[4] = { 1, 10, 100, 1000 };
	int64_t total = 0;
	for (size_t index = 0; index < amounts.size(); ++index)
	{
		if (amounts[index] < 0)
			return -1;
		total += amounts[index] * denominations[index];
	}
	return total;
}

bool validate_endpoint(const coin_transfer_endpoint &endpoint, bool source,
		       critical_entity_key *identity)
{
	const auto before = value(endpoint.before), after = value(endpoint.after);
	if (before < 0 || after < 0 || (source ? before <= after : after <= before))
		return false;
	if (endpoint.change.type == critical_command_type::account_bank)
	{
		currency_command_payload wallet = {};
		if (!currency_command_decode_payload(endpoint.change, &wallet) ||
		    wallet.reason != currency_reason_type::coin_transfer ||
		    endpoint.change.expected_revisions[0].revision == UINT64_MAX ||
		    endpoint.change.expected_revisions[1].revision == UINT64_MAX)
			return false;
		for (size_t index = 0; index < endpoint.before.size(); ++index)
			if (wallet.bank_delta.amount[index] ||
			    wallet.wallet_delta.amount[index] !=
				    static_cast<int64_t>(endpoint.after[index]) -
					    endpoint.before[index])
				return false;
		*identity = { critical_entity_type::player, wallet.pid };
		return true;
	}
	if (endpoint.change.type != critical_command_type::item_transfer)
		return false;
	item_transfer_payload pile = {};
	if (!item_transfer_command_decode_payload(endpoint.change, &pile) || pile.item_count != 1 ||
	    pile.multi_root || pile.items[0].vnum != VOBJ_COINS ||
	    pile.items[0].item_uid != pile.selected_item_uid)
		return false;
	const bool creation = pile.from_owner.type == item_owner_type::system;
	const bool consumed = pile.to_owner.type == item_owner_type::destruction;
	if (creation ? (source || before != 0 || consumed) :
		       (!before ||
			(!consumed && !item_owner_identity_equal(pile.from_owner, pile.to_owner))))
		return false;
	if (consumed != (after == 0) ||
	    (!creation && !consumed &&
	     (pile.target_root_item_uid != pile.items[0].root_item_uid ||
	      pile.target_parent_item_uid != pile.items[0].parent_item_uid)))
		return false;
	std::vector<player_item_snapshot> snapshots;
	if (player_item_snapshot_list_decode(pile.item_blob.data(), pile.item_blob_size,
					     &snapshots) != player_snapshot_codec_result::ok ||
	    snapshots.size() != 1 || snapshots[0].object_uid != pile.selected_item_uid ||
	    snapshots[0].vnum != VOBJ_COINS || snapshots[0].type != ITEM_MONEY ||
	    snapshots[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT)
		return false;
	for (size_t index = 0; index < endpoint.before.size(); ++index)
		if ((source ? endpoint.after[index] > endpoint.before[index] :
			      endpoint.after[index] < endpoint.before[index]) ||
		    snapshots[0].values[index] !=
			    (consumed ? endpoint.before[index] : endpoint.after[index]))
			return false;
	*identity = { critical_entity_type::item, pile.selected_item_uid };
	return true;
}

bool validate_payload(const coin_transfer_payload &payload)
{
	critical_entity_key source = {}, destination = {};
	return validate_endpoint(payload.source, true, &source) &&
	       validate_endpoint(payload.destination, false, &destination) &&
	       !critical_entity_key_equal(source, destination) &&
	       value(payload.source.before) - value(payload.source.after) ==
		       value(payload.destination.after) - value(payload.destination.before);
}

bool append_endpoint(critical_command *command, const coin_transfer_endpoint &endpoint,
		     uint64_t index)
{
	critical_command change = endpoint.change;
	if (!critical_operation_id_derive(command->operation_id, COIN_OPERATION_DOMAIN, index,
					  &change.operation_id))
		return false;
	change.source_site = command->source_site;
	change.deadline_class = command->deadline_class;
	// The parent receives the admission timestamp. Subcommands are deterministic
	// ledger identities, not separately admitted work.
	change.accepted_at_usec = 1;
	std::vector<uint8_t> encoded;
	if (!critical_command_normalize(&change) ||
	    critical_command_encode(change, &encoded) != critical_command_codec_result::ok)
		return false;
	for (int32_t amount : endpoint.before)
		append_u32(&command->payload, static_cast<uint32_t>(amount));
	for (int32_t amount : endpoint.after)
		append_u32(&command->payload, static_cast<uint32_t>(amount));
	append_u32(&command->payload, static_cast<uint32_t>(encoded.size()));
	command->payload.insert(command->payload.end(), encoded.begin(), encoded.end());
	command->keys.insert(command->keys.end(), change.keys.begin(), change.keys.end());
	return command->payload.size() <= CRITICAL_COMMAND_MAX_PAYLOAD_BYTES;
}

bool read_endpoint(const critical_command &command, size_t *offset,
		   coin_transfer_endpoint *endpoint)
{
	if (*offset > command.payload.size() ||
	    command.payload.size() - *offset < ENDPOINT_HEADER_BYTES)
		return false;
	const uint8_t *data = command.payload.data() + *offset;
	for (size_t index = 0; index < 4; ++index)
	{
		const uint32_t before = read_u32(data + index * 4);
		const uint32_t after = read_u32(data + 16 + index * 4);
		if (before > INT32_MAX || after > INT32_MAX)
			return false;
		endpoint->before[index] = static_cast<int32_t>(before);
		endpoint->after[index] = static_cast<int32_t>(after);
	}
	const size_t size = read_u32(data + 32);
	*offset += ENDPOINT_HEADER_BYTES;
	if (size > command.payload.size() - *offset ||
	    critical_command_decode(command.payload.data() + *offset, size, &endpoint->change) !=
		    critical_command_codec_result::ok)
		return false;
	*offset += size;
	return true;
}
} // namespace

bool coin_transfer_command_build(critical_command *command,
				 const critical_operation_id &operation_id,
				 const coin_transfer_payload &payload,
				 critical_source_site source_site,
				 critical_deadline_class deadline_class)
{
	if (!command || critical_operation_id_is_zero(operation_id) || !validate_payload(payload))
		return false;
	try
	{
		critical_command built = {};
		built.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		built.operation_id = operation_id;
		built.type = critical_command_type::coin_transfer;
		built.payload_version = COIN_TRANSFER_PAYLOAD_VERSION;
		built.source_site = source_site;
		built.deadline_class = deadline_class;
		built.accepted_at_usec = 1;
		if (!append_endpoint(&built, payload.source, 0) ||
		    !append_endpoint(&built, payload.destination, 1))
			return false;
		std::sort(built.keys.begin(), built.keys.end(), critical_entity_key_less);
		built.keys.erase(std::unique(built.keys.begin(), built.keys.end(),
					     critical_entity_key_equal),
				 built.keys.end());
		// Wallet and custody revisions can share a player key but describe
		// different domains. Keep their exact fences in the endpoint commands.
		if (!critical_command_valid(built))
			return false;
		built.accepted_at_usec = 0;
		*command = std::move(built);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool coin_transfer_command_decode_payload(const critical_command &command,
					  coin_transfer_payload *payload)
{
	if (!payload || command.type != critical_command_type::coin_transfer ||
	    command.payload_version != COIN_TRANSFER_PAYLOAD_VERSION ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
	    !command.expected_revisions.empty())
		return false;
	try
	{
		coin_transfer_payload decoded;
		size_t offset = 0;
		if (!read_endpoint(command, &offset, &decoded.source) ||
		    !read_endpoint(command, &offset, &decoded.destination) ||
		    offset != command.payload.size())
			return false;
		critical_command expected = {};
		if (!coin_transfer_command_build(&expected, command.operation_id, decoded,
						 command.source_site, command.deadline_class) ||
		    expected.payload != command.payload ||
		    expected.keys.size() != command.keys.size() ||
		    !std::equal(expected.keys.begin(), expected.keys.end(), command.keys.begin(),
				critical_entity_key_equal))
			return false;
		*payload = std::move(decoded);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool coin_transfer_command_encode_result(const coin_transfer_payload &payload,
					 const coin_transfer_result &result,
					 std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> *encoded)
{
	if (!encoded)
		return false;
	encoded->fill(0);
	const coin_transfer_endpoint *endpoints[2] = { &payload.source, &payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		auto at = encoded->begin() +
			  index * (CURRENCY_RESULT_PAYLOAD_BYTES + ITEM_TRANSFER_RESULT_BYTES);
		if (endpoints[index]->change.type == critical_command_type::account_bank)
		{
			std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> wallet = {};
			if (!currency_command_encode_result(result.wallets[index], &wallet))
				return false;
			std::copy(wallet.begin(), wallet.end(), at);
		}
		else if (endpoints[index]->change.type == critical_command_type::item_transfer)
		{
			std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> pile = {};
			if (!item_transfer_command_encode_result(result.piles[index], &pile))
				return false;
			std::copy(pile.begin(), pile.end(), at + CURRENCY_RESULT_PAYLOAD_BYTES);
		}
		else
			return false;
	}
	return true;
}

bool coin_transfer_command_destination_after_source(const coin_transfer_payload &payload,
						    const coin_transfer_result &result,
						    critical_command *destination)
{
	if (!destination)
		return false;
	*destination = payload.destination.change;
	const auto &source = payload.source.change;
	if (source.type != destination->type)
		return true;
	if (source.type == critical_command_type::account_bank)
	{
		if (source.expected_revisions.size() != 2 ||
		    destination->expected_revisions.size() != 2)
			return false;
		if (critical_entity_key_equal(source.keys[1], destination->keys[1]))
		{
			if (source.expected_revisions[1].revision !=
			    destination->expected_revisions[1].revision)
				return false;
			destination->expected_revisions[1].revision =
				result.wallets[0].bank_revision;
		}
		return true;
	}
	item_transfer_payload first = {}, second = {};
	if (!item_transfer_command_decode_payload(source, &first) ||
	    !item_transfer_command_decode_payload(*destination, &second))
		return false;
	auto advance = [&](const item_owner_identity &owner, uint64_t *revision)
	{
		if (item_owner_identity_equal(owner, first.from_owner))
		{
			if (*revision != first.expected_from_revision)
				return false;
			*revision = result.piles[0].from_owner_revision;
		}
		else if (item_owner_identity_equal(owner, first.to_owner))
		{
			if (*revision != first.expected_to_revision)
				return false;
			*revision = result.piles[0].to_owner_revision;
		}
		return true;
	};
	if (!advance(second.from_owner, &second.expected_from_revision) ||
	    !advance(second.to_owner, &second.expected_to_revision))
		return false;
	const auto accepted = destination->accepted_at_usec;
	if (!item_transfer_command_build(destination, payload.destination.change.operation_id,
					 second, source.source_site, source.deadline_class))
		return false;
	destination->accepted_at_usec = accepted;
	return true;
}

bool coin_transfer_command_decode_result(const coin_transfer_payload &payload,
					 const uint8_t *encoded, size_t size,
					 coin_transfer_result *result)
{
	if (!encoded || !result || size != COIN_TRANSFER_RESULT_BYTES)
		return false;
	coin_transfer_result decoded;
	const coin_transfer_endpoint *endpoints[2] = { &payload.source, &payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		const uint8_t *at = encoded + index * (CURRENCY_RESULT_PAYLOAD_BYTES +
						       ITEM_TRANSFER_RESULT_BYTES);
		if (endpoints[index]->change.type == critical_command_type::account_bank)
		{
			if (!currency_command_decode_result(at, CURRENCY_RESULT_PAYLOAD_BYTES,
							    &decoded.wallets[index]) ||
			    std::any_of(at + CURRENCY_RESULT_PAYLOAD_BYTES,
					at + CURRENCY_RESULT_PAYLOAD_BYTES +
						ITEM_TRANSFER_RESULT_BYTES,
					[](uint8_t byte) { return byte != 0; }))
				return false;
		}
		else if (endpoints[index]->change.type == critical_command_type::item_transfer)
		{
			if (!item_transfer_command_decode_result(at + CURRENCY_RESULT_PAYLOAD_BYTES,
								 ITEM_TRANSFER_RESULT_BYTES,
								 &decoded.piles[index]) ||
			    std::any_of(at, at + CURRENCY_RESULT_PAYLOAD_BYTES,
					[](uint8_t byte) { return byte != 0; }))
				return false;
		}
		else
			return false;
	}
	*result = decoded;
	return true;
}
