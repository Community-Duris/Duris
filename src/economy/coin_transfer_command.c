#include "economy/coin_transfer_command.h"

#include "economy/currency_command.h"
#include "item/item_transfer_command.h"
#include "player/player_snapshot_codec.h"
#include "world/vnum.obj.h"
#include "core/structs.h"

#include <algorithm>
#include <climits>
#include <limits>
#include <new>
#include <utility>

namespace
{
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
		       critical_entity_key *identity, const char **error)
{
	auto reject = [&](const char *reason)
	{
		if (error)
			*error = reason;
		return false;
	};
	const auto before = value(endpoint.before), after = value(endpoint.after);
	if (before < 0 || after < 0 || (source ? before <= after : after <= before))
		return reject("invalid coin amounts");
	if (endpoint.change.type == critical_command_type::account_bank)
	{
		currency_command_payload wallet = {};
		if (!currency_command_decode_payload(endpoint.change, &wallet) ||
		    wallet.reason != currency_reason_type::coin_transfer ||
		    endpoint.change.expected_revisions[0].revision == UINT64_MAX ||
		    endpoint.change.expected_revisions[1].revision == UINT64_MAX)
			return reject("invalid wallet endpoint");
		for (size_t index = 0; index < endpoint.before.size(); ++index)
			if (wallet.bank_delta.amount[index] ||
			    wallet.wallet_delta.amount[index] !=
				    static_cast<int64_t>(endpoint.after[index]) -
					    endpoint.before[index])
				return reject("wallet delta does not match amounts");
		*identity = { critical_entity_type::player, wallet.pid };
		return true;
	}
	if (endpoint.change.type != critical_command_type::item_transfer)
		return reject("unsupported endpoint type");
	item_transfer_payload pile = {};
	if (!item_transfer_command_decode_payload(endpoint.change, &pile) || pile.item_count != 1 ||
	    pile.multi_root || pile.items[0].item_uid != pile.selected_item_uid)
		return reject("invalid pile identity");
	const bool creation = pile.from_owner.type == item_owner_type::system;
	const bool consumed = pile.to_owner.type == item_owner_type::destruction;
	if (creation ? (source || before != 0 || consumed) :
		       (!before ||
			(!consumed && !item_owner_identity_equal(pile.from_owner, pile.to_owner))))
		return reject("invalid pile ownership transition");
	if (consumed != (after == 0) ||
	    (!creation && !consumed &&
	     (pile.target_root_item_uid != pile.items[0].root_item_uid ||
	      pile.target_parent_item_uid != pile.items[0].parent_item_uid)))
		return reject("invalid pile topology");
	std::vector<player_item_snapshot> snapshots;
	if (player_item_snapshot_list_decode(pile.item_blob.data(), pile.item_blob_size,
					     &snapshots) != player_snapshot_codec_result::ok ||
	    snapshots.size() != 1 || snapshots[0].object_uid != pile.selected_item_uid ||
	    snapshots[0].vnum != pile.items[0].vnum || snapshots[0].type != ITEM_MONEY ||
	    snapshots[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT)
		return reject("pile snapshot identity or type mismatch");
	for (size_t index = 0; index < endpoint.before.size(); ++index)
		if ((source ? endpoint.after[index] > endpoint.before[index] :
			      endpoint.after[index] < endpoint.before[index]) ||
		    snapshots[0].values[index] !=
			    (consumed ? endpoint.before[index] : endpoint.after[index]))
			return reject("pile snapshot amount mismatch");
	*identity = { critical_entity_type::item, pile.selected_item_uid };
	return true;
}

bool validate_payload(const coin_transfer_payload &payload, const char **error)
{
	critical_entity_key source = {}, destination = {};
	if (!validate_endpoint(payload.source, true, &source, error) ||
	    !validate_endpoint(payload.destination, false, &destination, error))
		return false;
	if (critical_entity_key_equal(source, destination) ||
	    value(payload.source.before) - value(payload.source.after) !=
		    value(payload.destination.after) - value(payload.destination.before))
	{
		if (error)
			*error = "duplicate endpoints or nonconserving transfer";
		return false;
	}
	return true;
}

bool append_endpoint(critical_command *command, const coin_transfer_endpoint &endpoint,
		     uint64_t index)
{
	critical_command change = endpoint.change;
	if (!critical_operation_id_derive(command->operation_id, COIN_TRANSFER_OPERATION_DOMAIN,
					  index, &change.operation_id))
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

bool stale_currency_endpoint(const coin_transfer_payload &payload,
			     critical_failure_stage failure_stage, size_t *endpoint_index,
			     bool *wallet_stale, bool *bank_stale)
{
	if (!endpoint_index || !wallet_stale || !bank_stale ||
	    !critical_failure_stage_valid(failure_stage))
		return false;
	const uint16_t raw = static_cast<uint16_t>(failure_stage);
	const uint16_t source_wallet =
		static_cast<uint16_t>(critical_failure_stage::coin_source_wallet_revision);
	const uint16_t source_bank =
		static_cast<uint16_t>(critical_failure_stage::coin_source_bank_revision);
	const uint16_t destination_wallet =
		static_cast<uint16_t>(critical_failure_stage::coin_destination_wallet_revision);
	const uint16_t destination_bank =
		static_cast<uint16_t>(critical_failure_stage::coin_destination_bank_revision);
	const uint16_t source = source_wallet | source_bank;
	const uint16_t destination = destination_wallet | destination_bank;
	if (!raw || (raw & ~(source | destination)) ||
	    static_cast<bool>(raw & source) == static_cast<bool>(raw & destination))
		return false;
	*endpoint_index = raw & source ? 0 : 1;
	if ((*endpoint_index ? payload.destination : payload.source).change.type !=
	    critical_command_type::account_bank)
		return false;
	*wallet_stale = raw & (*endpoint_index ? destination_wallet : source_wallet);
	*bank_stale = raw & (*endpoint_index ? destination_bank : source_bank);
	return *wallet_stale || *bank_stale;
}
} // namespace

bool coin_transfer_command_stale_result_expected(const coin_transfer_payload &payload,
						 critical_failure_stage failure_stage)
{
	size_t endpoint_index = 0;
	bool wallet_stale = false, bank_stale = false;
	return stale_currency_endpoint(payload, failure_stage, &endpoint_index, &wallet_stale,
				       &bank_stale);
}

bool coin_transfer_command_build(critical_command *command,
				 const critical_operation_id &operation_id,
				 const coin_transfer_payload &payload,
				 critical_source_site source_site,
				 critical_deadline_class deadline_class, const char **error)
{
	if (error)
		*error = "invalid command identity";
	if (!command || critical_operation_id_is_zero(operation_id) ||
	    !validate_payload(payload, error))
		return false;
	if (error)
		*error = "coin command encoding failed";
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
		if (error)
			*error = nullptr;
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

bool coin_transfer_command_encode_stale_result(
	const coin_transfer_payload &payload, const coin_transfer_result &result,
	critical_failure_stage failure_stage,
	std::array<uint8_t, COIN_TRANSFER_STALE_RESULT_BYTES> *encoded)
{
	size_t endpoint_index = 0;
	bool wallet_stale = false, bank_stale = false;
	std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> current = {};
	currency_command_result selected = {};
	if (!encoded || !stale_currency_endpoint(payload, failure_stage, &endpoint_index,
						 &wallet_stale, &bank_stale))
		return false;
	if (wallet_stale)
	{
		selected.wallet = result.wallets[endpoint_index].wallet;
		selected.wallet_revision = result.wallets[endpoint_index].wallet_revision;
	}
	if (bank_stale)
	{
		selected.bank = result.wallets[endpoint_index].bank;
		selected.bank_revision = result.wallets[endpoint_index].bank_revision;
	}
	if (!currency_command_encode_result(selected, &current))
		return false;
	encoded->fill(0);
	(*encoded)[0] = COIN_TRANSFER_STALE_RESULT_VERSION;
	(*encoded)[1] = static_cast<uint8_t>(endpoint_index);
	(*encoded)[2] = static_cast<uint8_t>((wallet_stale ? 1 : 0) | (bank_stale ? 2 : 0));
	std::copy(current.begin(), current.end(), encoded->begin() + 3);
	return true;
}

bool coin_transfer_command_decode_stale_result(const coin_transfer_payload &payload,
					       critical_failure_stage failure_stage,
					       const uint8_t *encoded, size_t size,
					       coin_transfer_stale_result *result)
{
	if (!result || size != COIN_TRANSFER_STALE_RESULT_BYTES)
		return false;
	coin_transfer_stale_result decoded;
	if (!stale_currency_endpoint(payload, failure_stage, &decoded.endpoint_index,
				     &decoded.wallet_stale, &decoded.bank_stale) ||
	    !encoded || encoded[0] != COIN_TRANSFER_STALE_RESULT_VERSION ||
	    encoded[1] != decoded.endpoint_index ||
	    encoded[2] != static_cast<uint8_t>((decoded.wallet_stale ? 1 : 0) |
					       (decoded.bank_stale ? 2 : 0)) ||
	    !currency_command_decode_result(encoded + 3, CURRENCY_RESULT_PAYLOAD_BYTES,
					    &decoded.current))
		return false;
	if ((!decoded.wallet_stale && (decoded.current.wallet_revision ||
				       std::any_of(decoded.current.wallet.amount.begin(),
						   decoded.current.wallet.amount.end(),
						   [](int64_t amount) { return amount != 0; }))) ||
	    (!decoded.bank_stale &&
	     (decoded.current.bank_revision ||
	      std::any_of(decoded.current.bank.amount.begin(), decoded.current.bank.amount.end(),
			  [](int64_t amount) { return amount != 0; }))))
		return false;
	const critical_command &endpoint = decoded.endpoint_index ? payload.destination.change :
								    payload.source.change;
	currency_command_payload endpoint_payload = {};
	if (endpoint.expected_revisions.size() != 2 ||
	    !currency_command_decode_payload(endpoint, &endpoint_payload))
		return false;
	if ((decoded.wallet_stale &&
	     (decoded.current.wallet_revision == std::numeric_limits<uint64_t>::max() ||
	      decoded.current.wallet_revision <= endpoint.expected_revisions[0].revision ||
	      std::any_of(decoded.current.wallet.amount.begin(), decoded.current.wallet.amount.end(),
			  [](int64_t amount) { return amount < 0 || amount > INT_MAX; }))) ||
	    (decoded.bank_stale &&
	     (decoded.current.bank_revision == std::numeric_limits<uint64_t>::max() ||
	      decoded.current.bank_revision <= endpoint.expected_revisions[1].revision ||
	      std::any_of(decoded.current.bank.amount.begin(), decoded.current.bank.amount.end(),
			  [](int64_t amount) { return amount < 0 || amount > INT_MAX; }))))
		return false;
	*result = decoded;
	return true;
}

namespace
{
bool coin_value_add(size_t &total, size_t value) noexcept
{
	if (value > SIZE_MAX - total)
		return false;
	total += value;
	return true;
}
template <typename T>
bool coin_value_vector_heap(const std::vector<T> &value, size_t &total) noexcept
{
	return value.capacity() <= SIZE_MAX / sizeof(T) &&
	       coin_value_add(total, value.capacity() * sizeof(T));
}
struct coin_value_validation_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const item_transfer_payload *pile = nullptr;
	const std::vector<player_item_snapshot> *snapshots = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation = 9 * sizeof(void *) + 7 * sizeof(size_t) +
					       4 * sizeof(bool) +
					       4 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer, heap = 0;
		if (!coin_value_add(total, sizeof(*this)) || !coin_value_add(total, frames) ||
		    !coin_value_add(total, observation) ||
		    !coin_value_add(total, item_transfer_payload_copy_frame_bytes()) ||
		    !coin_value_add(total, player_item_snapshot_copy_frame_bytes()))
			return false;
		if (pile && (!coin_value_add(total, sizeof(*pile)) ||
			     !item_transfer_payload_current_heap_bytes(*pile, &heap) ||
			     !coin_value_add(total, heap)))
			return false;
		if (snapshots)
		{
			if (!coin_value_add(total, sizeof(*snapshots)) ||
			    !coin_value_vector_heap(*snapshots, total))
				return false;
			for (const auto &item : *snapshots)
				if (!player_item_snapshot_current_heap_bytes(item, &heap) ||
				    !coin_value_add(total, heap))
					return false;
		}
		if (!coin_value_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
};
constexpr size_t coin_value_fixed_frames =
	// Actual value(amounts) parameter/result, units[4], total/index, array
	// query/operator[] and real amount arithmetic. Called sequentially.
	sizeof(void *) + sizeof(int64_t) + sizeof(int64_t[4]) + sizeof(int64_t) + sizeof(size_t) +
	3 * (sizeof(void *) + sizeof(size_t)) +
	// Original fixed owner identity/key equality, three pointer/bool scopes;
	// genuine array/vector begin/end/size/subscript query carriers.
	6 * sizeof(void *) + 3 * sizeof(bool) + 8 * (sizeof(void *) + sizeof(size_t));
constexpr size_t coin_value_payload_default_frames =
	// Same actual ten aggregate defaults/destructors, six vector/base/impl/
	// data defaults, six string/hider/local/NUL defaults in item payload.
	2 * 10 * sizeof(void *) + 6 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
	6 * (7 * sizeof(void *) + sizeof(std::allocator<char>) + sizeof(size_t) + sizeof(char));
bool coin_value_validate_endpoint_owned(const coin_transfer_endpoint &endpoint, bool source,
					critical_entity_key *identity, const char **error,
					coin_value_validation_budget &budget)
{
	auto reject = [&](const char *reason)
	{
		if (error)
			*error = reason;
		return false;
	};
	size_t admission_prefix = 0;
	const auto before = value(endpoint.before), after = value(endpoint.after);
	if (before < 0 || after < 0 || (source ? before <= after : after <= before))
		return reject("invalid coin amounts");
	if (endpoint.change.type == critical_command_type::account_bank)
	{
		if (!budget.peak(sizeof(currency_command_payload) +
				 sizeof(currency_command_payload) + 2 * sizeof(void *)))
			return false;
		currency_command_payload wallet = {};
		if (!(budget.prefix(admission_prefix, sizeof(wallet)) &&
		      currency_command_decode_payload_bounded(endpoint.change, &wallet,
							      budget.reserve, budget.context,
							      admission_prefix)) ||
		    wallet.reason != currency_reason_type::coin_transfer ||
		    endpoint.change.expected_revisions[0].revision == UINT64_MAX ||
		    endpoint.change.expected_revisions[1].revision == UINT64_MAX)
			return reject("invalid wallet endpoint");
		for (size_t index = 0; index < endpoint.before.size(); ++index)
			if (wallet.bank_delta.amount[index] ||
			    wallet.wallet_delta.amount[index] !=
				    static_cast<int64_t>(endpoint.after[index]) -
					    endpoint.before[index])
				return reject("wallet delta does not match amounts");
		*identity = { critical_entity_type::player, wallet.pid };
		return true;
	}
	if (endpoint.change.type != critical_command_type::item_transfer)
		return reject("unsupported endpoint type");
	if (!budget.peak(sizeof(item_transfer_payload) + coin_value_payload_default_frames))
		return false;
	item_transfer_payload pile = {};
	budget.pile = &pile;
	if (!(budget.prefix(admission_prefix) &&
	      item_transfer_command_decode_payload_bounded(endpoint.change, &pile, budget.reserve,
							   budget.context, admission_prefix)) ||
	    pile.item_count != 1 || pile.multi_root ||
	    pile.items[0].item_uid != pile.selected_item_uid)
		return reject("invalid pile identity");
	const bool creation = pile.from_owner.type == item_owner_type::system;
	const bool consumed = pile.to_owner.type == item_owner_type::destruction;
	if (creation ? (source || before != 0 || consumed) :
		       (!before ||
			(!consumed && !item_owner_identity_equal(pile.from_owner, pile.to_owner))))
		return reject("invalid pile ownership transition");
	if (consumed != (after == 0) ||
	    (!creation && !consumed &&
	     (pile.target_root_item_uid != pile.items[0].root_item_uid ||
	      pile.target_parent_item_uid != pile.items[0].parent_item_uid)))
		return reject("invalid pile topology");
	if (!budget.peak(sizeof(std::vector<player_item_snapshot>) + 4 * sizeof(void *) +
			 sizeof(std::allocator<player_item_snapshot>)))
		return false;
	std::vector<player_item_snapshot> snapshots;
	budget.snapshots = &snapshots;
	if ((!budget.prefix(admission_prefix) ?
		     player_snapshot_codec_result::invalid_value :
		     player_item_snapshot_list_decode_bounded(
			     pile.item_blob.data(), pile.item_blob_size, &snapshots, budget.reserve,
			     budget.context, admission_prefix, nullptr)) !=
		    player_snapshot_codec_result::ok ||
	    snapshots.size() != 1 || snapshots[0].object_uid != pile.selected_item_uid ||
	    snapshots[0].vnum != pile.items[0].vnum || snapshots[0].type != ITEM_MONEY ||
	    snapshots[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT)
		return reject("pile snapshot identity or type mismatch");
	for (size_t index = 0; index < endpoint.before.size(); ++index)
		if ((source ? endpoint.after[index] > endpoint.before[index] :
			      endpoint.after[index] < endpoint.before[index]) ||
		    snapshots[0].values[index] !=
			    (consumed ? endpoint.before[index] : endpoint.after[index]))
			return reject("pile snapshot amount mismatch");
	*identity = { critical_entity_type::item, pile.selected_item_uid };
	return true;
}

bool coin_value_validate_payload_owned(const coin_transfer_payload &payload, const char **error,
				       coin_value_validation_budget &budget)
{
	critical_entity_key source = {}, destination = {};
	size_t admission_prefix = 0;
	if (!(budget.prefix(admission_prefix) &&
	      coin_transfer_endpoint_valid_bounded(payload.source, true, &source, error,
						   budget.reserve, budget.context,
						   admission_prefix)) ||
	    !(budget.prefix(admission_prefix) &&
	      coin_transfer_endpoint_valid_bounded(payload.destination, false, &destination, error,
						   budget.reserve, budget.context,
						   admission_prefix)))
		return false;
	if (critical_entity_key_equal(source, destination) ||
	    value(payload.source.before) - value(payload.source.after) !=
		    value(payload.destination.after) - value(payload.destination.before))
	{
		if (error)
			*error = "duplicate endpoints or nonconserving transfer";
		return false;
	}
	return true;
}
} // namespace
bool coin_transfer_endpoint_valid_bounded(const coin_transfer_endpoint &endpoint, bool source,
					  critical_entity_key *identity, const char **error,
					  bool (*reserve)(size_t, void *) noexcept, void *context,
					  size_t outer_live) noexcept
{
	if (!identity || !reserve)
		return false;
	constexpr size_t frames =
		10 * sizeof(void *) + 6 * sizeof(size_t) + 6 * sizeof(bool) + 2 * sizeof(int64_t) +
		// Original reject closure captures error-pointer by reference and real
		// call this/reason/false return; local before/after, creation/consumed.
		sizeof(void *) + 2 * sizeof(void *) + sizeof(bool) + coin_value_fixed_frames;
	coin_value_validation_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return coin_value_validate_endpoint_owned(endpoint, source, identity, error,
							  budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool coin_transfer_payload_valid_bounded(const coin_transfer_payload &payload, const char **error,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer_live) noexcept
{
	if (!reserve)
		return false;
	constexpr size_t frames = 7 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool) +
				  2 * sizeof(critical_entity_key) + coin_value_fixed_frames;
	coin_value_validation_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	try
	{
		return coin_value_validate_payload_owned(payload, error, budget);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

#include <type_traits>
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
namespace
{
constexpr size_t coin_wire_allocator_frames =
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
constexpr size_t coin_wire_copy_frames =
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
constexpr size_t coin_wire_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t coin_wire_default_frames =
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
constexpr size_t coin_wire_vector_frames =
	coin_wire_allocator_frames + coin_wire_copy_frames + coin_wire_relocate_frames +
	coin_wire_default_frames +
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
constexpr size_t coin_wire_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + coin_wire_allocator_frames;
constexpr size_t coin_wire_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<int32_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + coin_wire_vector_frames;
template <typename T, typename Comparator> constexpr size_t coin_wire_sort_leaf_frames()
{
	// Same real GCC13 sort/partition/insertion/heap/copy/adjacent call scopes
	// as UID sorting. Values and comparator carriers use their genuine types.
	// Original key less/equal this-free argument/result scopes and revision
	// lambda this/left/right/result plus its nested key less call.
	return 3 * (2 * sizeof(void *) + sizeof(bool)) + 3 * sizeof(void *) + sizeof(bool) +
	       18 * sizeof(void *) + 7 * sizeof(Comparator) + sizeof(T) + 16 * sizeof(void *) +
	       6 * sizeof(Comparator) + 2 * sizeof(T) + 23 * sizeof(void *) +
	       11 * sizeof(std::ptrdiff_t) + 7 * sizeof(Comparator) + 4 * sizeof(T) +
	       8 * sizeof(void *) + 5 * sizeof(Comparator) + 4 * sizeof(bool) +
	       5 * (4 * sizeof(void *)) + 2 * (2 * sizeof(void *)) + 3 * (2 * sizeof(void *)) +
	       2 * sizeof(void *) + sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	       sizeof(size_t) + sizeof(std::ptrdiff_t) + 9 * sizeof(void *) +
	       2 * sizeof(Comparator) + sizeof(bool);
}
constexpr size_t coin_wire_command_default_frames =
	// Real command generated default/destructor and four vector default
	// constructor/_Vector_base/_Vector_impl/_Vector_impl_data/allocator
	// carriers; current object inline is separately owned by its lifetime.
	2 * sizeof(void *) + 4 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
	4 * (sizeof(void *) + coin_wire_allocator_frames);
constexpr size_t coin_wire_critical_codec_frames =
	// Original encoder, working-bytes and bounded-encode parameter/return/
	// wire_bytes/status scopes, loop key+revision refs/endpoints/pad locals.
	10 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(critical_command_codec_result) +
	6 * sizeof(void *) + 2 * sizeof(unsigned int) +
	// append_le genuine widest uint64_t value plus byte loop and vector
	// reference; array begin/end and data query sources.
	sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + 6 * (sizeof(void *) + sizeof(size_t)) +
	// Actual original decoder/bounded counterpart fixed scalar locals:
	// encoded/size/destination/reserve/context/outer/heap output, live,
	// offset/type/source/3 counts/auction flag/limit/required/intent locals.
	5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(critical_command_codec_result) +
	2 * sizeof(size_t) + 2 * sizeof(uint16_t) + 3 * sizeof(uint32_t) + sizeof(bool) +
	sizeof(size_t) + sizeof(uint64_t) + 2 * sizeof(size_t) + sizeof(uint32_t) +
	// Key/revision loop indices and padding, prospective request/extra,
	// retained scalar and original bad_alloc reference. Object carriers
	// decoded/key/revision are admitted by existing real decoder itself.
	2 * sizeof(uint32_t) + 2 * sizeof(size_t) + 4 * sizeof(size_t) + sizeof(size_t) +
	sizeof(void *) +
	// Genuine widest read_le input/size/offset/value/decoded/index/return;
	// decode_add/admit/heap actual parameters/locals/query scopes.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(uint64_t) + sizeof(size_t) + sizeof(bool) +
	10 * sizeof(void *) + 9 * sizeof(size_t) + 3 * sizeof(bool) +
	// vector constructions/destruction/calls, allocator and all fitting
	// insert/assign/append profiles, original command nonthrow final move.
	coin_wire_command_default_frames + coin_wire_vector_frames + 4 * coin_wire_move_frames;
using coin_wire_key_iterator = std::vector<critical_entity_key>::iterator;
using coin_wire_key_const_iterator = std::vector<critical_entity_key>::const_iterator;
using coin_wire_key_equal_predicate = decltype(&critical_entity_key_equal);
using coin_wire_key_equal_wrapper =
	__gnu_cxx::__ops::_Iter_comp_iter<coin_wire_key_equal_predicate>;
constexpr size_t coin_wire_unique_erase_frames =
	// unique(first,last,function): two iterators, actual function pointer,
	// returned iterator. __iter_comp_iter: pointer argument and returned
	// wrapper; wrapper constructor: this and pointer, real move refs.
	2 * sizeof(coin_wire_key_iterator) + sizeof(coin_wire_key_equal_predicate) +
	sizeof(coin_wire_key_iterator) + sizeof(coin_wire_key_equal_predicate) +
	sizeof(coin_wire_key_equal_wrapper) + sizeof(void *) +
	sizeof(coin_wire_key_equal_predicate) + 2 * (2 * sizeof(void *)) +
	// __unique parameters, dest and return; __adjacent_find parameters,
	// next and return. These pass the actual function-pointer wrapper.
	4 * sizeof(coin_wire_key_iterator) + sizeof(coin_wire_key_equal_wrapper) +
	4 * sizeof(coin_wire_key_iterator) + sizeof(coin_wire_key_equal_wrapper) +
	// wrapper::operator(): this/two by-value iterators/bool; authentic
	// critical_entity_key_equal: two key references and bool return.
	sizeof(void *) + 2 * sizeof(coin_wire_key_iterator) + sizeof(bool) + 2 * sizeof(void *) +
	sizeof(bool) +
	// Actual dereference (two), prefix increment (two), iterator equality
	// (two refs, bool, two base calls), move reference pair, and trivial
	// key assignment source/destination refs on the retained unique path.
	2 * (2 * sizeof(void *)) + 2 * (2 * sizeof(void *)) + 2 * sizeof(void *) + sizeof(bool) +
	2 * (2 * sizeof(void *)) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	// Range erase: this/two const iterators, begin/cbegin locals and
	// return iterator; _M_erase: this/two iterators/return. last is the
	// original vector end, so the MOVE3 branch is genuinely not reached.
	sizeof(void *) + 3 * sizeof(coin_wire_key_const_iterator) +
	2 * sizeof(coin_wire_key_iterator) + sizeof(void *) + 3 * sizeof(coin_wire_key_iterator) +
	// Two iterator conversions (this/input ref plus base call), both
	// subtraction calls (two refs/ptrdiff plus two base calls), both
	// iterator addition calls (this/ptrdiff, temporary pointer, returned
	// iterator, constructor this/input ref), and begin/cbegin/end getters.
	2 * (4 * sizeof(void *)) + 2 * (6 * sizeof(void *) + sizeof(std::ptrdiff_t)) +
	2 * (4 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(coin_wire_key_iterator)) +
	3 * (3 * sizeof(void *) + sizeof(coin_wire_key_iterator)) +
	// _M_erase comparisons/base/end-minus-last; _M_erase_at_end this,
	// pointer and n; allocator getter this/return ref; allocator _Destroy
	// two pointers/ref -> _Destroy two pointers -> trivial __destroy pair.
	2 * (6 * sizeof(void *) + sizeof(bool)) + 2 * sizeof(void *) + 6 * sizeof(void *) +
	sizeof(std::ptrdiff_t) + 2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) +
	3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(bool);
constexpr size_t coin_wire_equal_frames =
	// Original vector== and equal(first,last,first2,equal-function) closure:
	// vector refs/sizes/begins, equal/aux/aux1/__equal parameters/result,
	// real binary function-pointer comparator conversion/call and iterators.
	2 * sizeof(void *) + sizeof(bool) + 8 * (sizeof(void *) + sizeof(size_t)) +
	4 * (4 * sizeof(void *) + sizeof(bool)) + 4 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	3 * sizeof(void *) + sizeof(size_t) + sizeof(int) + 2 * sizeof(void *) + sizeof(bool) +
	6 * (2 * sizeof(void *));
struct coin_wire_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *built = nullptr, *change = nullptr, *expected = nullptr;
	const std::vector<uint8_t> *encoded = nullptr;
	const coin_transfer_payload *decoded = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation = 11 * sizeof(void *) + 8 * sizeof(size_t) +
					       6 * sizeof(bool) +
					       4 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer, heap = 0;
		if (!coin_value_add(total, sizeof(*this)) || !coin_value_add(total, frames) ||
		    !coin_value_add(total, observation) ||
		    !coin_value_add(total, critical_command_copy_frame_bytes()) ||
		    !coin_value_add(total, critical_command_valid_frame_bytes()))
			return false;
		// Actual scalar observer lambda captures total and heap by reference;
		// its this/argument/result scopes are included in observation.
		const auto command = [&](const critical_command *value) noexcept
		{
			return !value || (coin_value_add(total, sizeof(*value)) &&
					  critical_command_current_heap_bytes(*value, &heap) &&
					  coin_value_add(total, heap));
		};
		if (!command(built) || !command(change) || !command(expected))
			return false;
		if (encoded && (!coin_value_add(total, sizeof(*encoded)) ||
				!coin_value_vector_heap(*encoded, total)))
			return false;
		if (decoded && (!coin_value_add(total, sizeof(*decoded)) ||
				!coin_transfer_payload_current_heap_bytes(*decoded, &heap) ||
				!coin_value_add(total, heap)))
			return false;
		if (!coin_value_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	template <typename T> bool growth(const std::vector<T> &value, size_t count) const noexcept
	{
		size_t request = coin_wire_vector_frames;
		if (count > value.max_size() - value.size())
			return false;
		if (count > value.capacity() - value.size())
		{
			size_t next = value.size();
			if (!coin_value_add(next, std::max(value.size(), count)) ||
			    next > value.max_size())
				next = value.max_size();
			if (next > SIZE_MAX / sizeof(T) ||
			    !coin_value_add(request, next * sizeof(T)))
				return false;
		}
		return coin_value_add(request,
				      2 * sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool)) &&
		       peak(request);
	}
	template <typename T, typename Comparator> bool sort_frame(size_t count) const noexcept
	{
		size_t levels = 0, remaining = count,
		       request = coin_wire_sort_leaf_frames<T, Comparator>();
		while (remaining > 1)
		{
			remaining >>= 1;
			++levels;
		}
		constexpr size_t recursion =
			3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(Comparator);
		return 2 * levels + 1 <= SIZE_MAX / recursion &&
		       coin_value_add(request, (2 * levels + 1) * recursion) &&
		       coin_value_add(request,
				      sizeof(void *) + 4 * sizeof(size_t) + sizeof(bool)) &&
		       peak(request);
	}
};
bool coin_wire_append_u32_owned(std::vector<uint8_t> *output, uint32_t value,
				coin_wire_budget &budget)
{
	for (unsigned int byte = 0; byte < 4; ++byte)
	{
		if (!budget.growth(*output, 1))
			return false;
		output->push_back(static_cast<uint8_t>(value >> (byte * 8)));
	}
	return true;
}

bool coin_wire_append_endpoint_owned(critical_command *command,
				     const coin_transfer_endpoint &endpoint, uint64_t index,
				     coin_wire_budget &budget)
{
	size_t admission_prefix = 0, admission_request = 0;
	if (!critical_command_fresh_copy_request_bytes(endpoint.change, &admission_request) ||
	    !coin_value_add(admission_request,
			    sizeof(critical_command) + critical_command_copy_frame_bytes()) ||
	    !budget.peak(admission_request))
		return false;
	critical_command change = endpoint.change;
	budget.change = &change;
	if (!(budget.prefix(admission_prefix) &&
	      critical_operation_id_derive_bounded(
		      command->operation_id, COIN_TRANSFER_OPERATION_DOMAIN, index,
		      &change.operation_id, budget.reserve, budget.context, admission_prefix)))
		return false;
	change.source_site = command->source_site;
	change.deadline_class = command->deadline_class;
	// The parent receives the admission timestamp. Subcommands are deterministic
	// ledger identities, not separately admitted work.
	change.accepted_at_usec = 1;
	if (!budget.peak(sizeof(std::vector<uint8_t>) + coin_wire_command_default_frames))
		return false;
	std::vector<uint8_t> encoded;
	budget.encoded = &encoded;
	if (!(budget.prefix(admission_prefix) &&
	      critical_command_normalize_bounded(&change, budget.reserve, budget.context,
						 admission_prefix)) ||
	    (!budget.prefix(admission_prefix, coin_wire_critical_codec_frames) ?
		     critical_command_codec_result::overflow :
		     critical_command_encode_bounded(change, &encoded, budget.reserve,
						     budget.context, admission_prefix)) !=
		    critical_command_codec_result::ok)
		return false;
	for (int32_t amount : endpoint.before)
		if (!coin_wire_append_u32_owned(&command->payload, static_cast<uint32_t>(amount),
						budget))
			return false;
	for (int32_t amount : endpoint.after)
		if (!coin_wire_append_u32_owned(&command->payload, static_cast<uint32_t>(amount),
						budget))
			return false;
	if (!coin_wire_append_u32_owned(&command->payload, static_cast<uint32_t>(encoded.size()),
					budget))
		return false;
	if (!budget.growth(command->payload, encoded.size()))
		return false;
	command->payload.insert(command->payload.end(), encoded.begin(), encoded.end());
	if (!budget.growth(command->keys, change.keys.size()))
		return false;
	command->keys.insert(command->keys.end(), change.keys.begin(), change.keys.end());
	budget.change = nullptr;
	budget.encoded = nullptr;
	return command->payload.size() <= CRITICAL_COMMAND_MAX_PAYLOAD_BYTES;
}

bool coin_wire_read_endpoint_owned(const critical_command &command, size_t *offset,
				   coin_transfer_endpoint *endpoint, coin_wire_budget &budget)
{
	size_t admission_prefix = 0;
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
	    (!budget.prefix(admission_prefix, coin_wire_critical_codec_frames) ?
		     critical_command_codec_result::overflow :
		     critical_command_decode_bounded(command.payload.data() + *offset, size,
						     &endpoint->change, budget.reserve,
						     budget.context, admission_prefix, nullptr)) !=
		    critical_command_codec_result::ok)
		return false;
	*offset += size;
	return true;
}

bool coin_wire_coin_transfer_command_build_owned(critical_command *command,
						 const critical_operation_id &operation_id,
						 const coin_transfer_payload &payload,
						 critical_source_site source_site,
						 critical_deadline_class deadline_class,
						 const char **error, coin_wire_budget &budget)
{
	size_t admission_prefix = 0;
	if (error)
		*error = "invalid command identity";
	if (!command || critical_operation_id_is_zero(operation_id) ||
	    !(budget.prefix(admission_prefix) &&
	      coin_transfer_payload_valid_bounded(payload, error, budget.reserve, budget.context,
						  admission_prefix)))
		return false;
	if (error)
		*error = "coin command encoding failed";
	try
	{
		if (!budget.peak(sizeof(critical_command) + coin_wire_command_default_frames))
			return false;
		critical_command built = {};
		budget.built = &built;
		built.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		built.operation_id = operation_id;
		built.type = critical_command_type::coin_transfer;
		built.payload_version = COIN_TRANSFER_PAYLOAD_VERSION;
		built.source_site = source_site;
		built.deadline_class = deadline_class;
		built.accepted_at_usec = 1;
		if (!coin_wire_append_endpoint_owned(&built, payload.source, 0, budget) ||
		    !coin_wire_append_endpoint_owned(&built, payload.destination, 1, budget))
			return false;
		if (!budget.sort_frame<critical_entity_key, decltype(&critical_entity_key_less)>(
			    built.keys.size()))
			return false;
		std::sort(built.keys.begin(), built.keys.end(), critical_entity_key_less);
		if (!budget.peak(coin_wire_unique_erase_frames))
			return false;
		built.keys.erase(std::unique(built.keys.begin(), built.keys.end(),
					     critical_entity_key_equal),
				 built.keys.end());
		// Wallet and custody revisions can share a player key but describe
		// different domains. Keep their exact fences in the endpoint commands.
		if (!budget.peak(critical_command_valid_frame_bytes() +
				 critical_command_copy_frame_bytes()))
			return false;
		if (!critical_command_valid(built))
			return false;
		built.accepted_at_usec = 0;
		*command = std::move(built);
		if (error)
			*error = nullptr;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool coin_wire_coin_transfer_command_decode_payload_owned(const critical_command &command,
							  coin_transfer_payload *payload,
							  coin_wire_budget &budget,
							  size_t *retained_heap)
{
	if (!payload || command.type != critical_command_type::coin_transfer ||
	    command.payload_version != COIN_TRANSFER_PAYLOAD_VERSION ||
	    command.payload.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES ||
	    !command.expected_revisions.empty())
		return false;
	try
	{
		if (!budget.peak(sizeof(coin_transfer_payload) +
				 2 * coin_wire_command_default_frames + 3 * sizeof(void *) +
				 sizeof(void *)))
			return false;
		coin_transfer_payload decoded;
		budget.decoded = &decoded;
		size_t offset = 0;
		size_t admission_prefix = 0, admission_retained = 0;
		if (!coin_wire_read_endpoint_owned(command, &offset, &decoded.source, budget) ||
		    !coin_wire_read_endpoint_owned(command, &offset, &decoded.destination,
						   budget) ||
		    offset != command.payload.size())
			return false;
		if (!budget.peak(sizeof(critical_command) + coin_wire_command_default_frames))
			return false;
		critical_command expected = {};
		budget.expected = &expected;
		if (!(budget.prefix(admission_prefix) &&
		      coin_transfer_command_build_bounded(
			      &expected, command.operation_id, decoded, command.source_site,
			      command.deadline_class, nullptr, budget.reserve, budget.context,
			      admission_prefix)) ||
		    !budget.peak(coin_wire_equal_frames) || expected.payload != command.payload ||
		    expected.keys.size() != command.keys.size() ||
		    !std::equal(expected.keys.begin(), expected.keys.end(), command.keys.begin(),
				critical_entity_key_equal))
			return false;
		if (!coin_transfer_payload_current_heap_bytes(decoded, &admission_retained) ||
		    !budget.peak(2 * critical_command_copy_frame_bytes() + 3 * sizeof(void *)))
			return false;
		*payload = std::move(decoded);
		if (retained_heap)
			*retained_heap = admission_retained;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
} // namespace
#endif // genuine GCC13 private owning profile and consumers
bool coin_transfer_payload_current_heap_bytes(const coin_transfer_payload &payload,
					      size_t *bytes) noexcept
{
	if (!bytes)
		return false;
	size_t total = 0, heap = 0;
	if (!critical_command_current_heap_bytes(payload.source.change, &heap) ||
	    !coin_value_add(total, heap) ||
	    !critical_command_current_heap_bytes(payload.destination.change, &heap) ||
	    !coin_value_add(total, heap))
		return false;
	*bytes = total;
	return true;
}
bool coin_transfer_command_build_bounded(critical_command *command,
					 const critical_operation_id &operation_id,
					 const coin_transfer_payload &payload,
					 critical_source_site source_site,
					 critical_deadline_class deadline_class, const char **error,
					 bool (*reserve)(size_t, void *) noexcept, void *context,
					 size_t outer_live) noexcept
{
	if (!reserve)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	constexpr size_t frames =
		// Public/owned build and full append endpoint arguments/results,
		// original new prefix/request, range loops amount/begin/end and
		// true append_u32 owned parameters/byte/return/catch-reference.
		12 * sizeof(void *) + 2 * sizeof(critical_source_site) +
		2 * sizeof(critical_deadline_class) + 4 * sizeof(size_t) + 4 * sizeof(bool) +
		4 * sizeof(void *) + sizeof(uint64_t) + 4 * sizeof(size_t) + 2 * sizeof(int32_t) +
		4 * sizeof(void *) + sizeof(uint32_t) + sizeof(unsigned int) + sizeof(bool) +
		3 * sizeof(void *) + sizeof(void *) + sizeof(bool);
	coin_wire_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	static_assert(std::is_nothrow_move_assignable_v<critical_command>);
	return coin_wire_coin_transfer_command_build_owned(
		command, operation_id, payload, source_site, deadline_class, error, budget);
#else
	(void)command;
	(void)operation_id;
	(void)payload;
	(void)source_site;
	(void)deadline_class;
	(void)error;
	(void)context;
	(void)outer_live;
	return false;
#endif
}
bool coin_transfer_command_decode_payload_bounded(const critical_command &command,
						  coin_transfer_payload *payload,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer_live,
						  size_t *retained_heap) noexcept
{
	if (!reserve)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	constexpr size_t frames =
		// Public/owned decoder and endpoint reader arguments/result plus
		// original offset/data/index/before/after/size, real read_u32 byte,
		// three admission scalars and original bad_alloc catch reference.
		9 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool) + 4 * sizeof(void *) +
		sizeof(bool) + sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(uint32_t) +
		sizeof(void *) + sizeof(uint32_t) + sizeof(unsigned int) + 3 * sizeof(size_t) +
		sizeof(void *);
	coin_wire_budget budget{ reserve, context, outer_live, frames };
	if (!budget.peak())
		return false;
	static_assert(std::is_nothrow_move_assignable_v<coin_transfer_payload>);
	return coin_wire_coin_transfer_command_decode_payload_owned(command, payload, budget,
								    retained_heap);
#else
	(void)command;
	(void)payload;
	(void)context;
	(void)outer_live;
	(void)retained_heap;
	return false;
#endif
}
