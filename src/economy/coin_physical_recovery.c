#include "economy/coin_physical_recovery.h"
#include "core/prototypes.h"
#include "core/utils.h"
#include "economy/coin_transfer_accounting.h"
#include "economy/coin_physical_publication.h"
#include "economy/currency_transaction.h"
#include "economy/economic_accounting_intent.h"
#include "item/item_ownership_runtime.h"
#include "player/inert_item_stage.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "world/handler.h"
#include "world/object_template.h"
#ifndef __NO_MYSQL__
#include "persistence/critical_command_repository.h"
#include "player/player_sql_transaction_cleanup.h"
#else
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_accounting_coin_transaction.h"
#include "flatfile/flatfile_accounting_pile_state.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_world_item_repository.h"
#include "persistence/persistence_mode.h"
#endif
#include <algorithm>
#include <array>
#include <charconv>
#include <climits>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <openssl/sha.h>

extern P_obj object_list;
extern P_char character_list;
extern P_desc descriptor_list;
extern P_room world;
extern const int top_of_world;
extern P_index obj_index;
extern int top_of_objt;

namespace
{
struct shape
{
	coin_transfer_payload transfer;
	currency_command_payload wallet = {};
	std::unique_ptr<item_transfer_payload> pile;
	player_item_snapshot literal;
	size_t wallet_index = 0, pile_index = 1;
	bool drop = false, consumed = false;
	uint64_t room = 0;
};
bool decode_shape(const critical_command &command, shape &out)
{
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    command.type != critical_command_type::coin_transfer || !command.publication_required ||
	    !critical_command_envelope_valid(command) ||
	    !coin_transfer_accounting_command_supported(command) ||
	    !coin_transfer_command_decode_payload(command, &out.transfer))
		return false;
	out.drop = out.transfer.source.change.type == critical_command_type::account_bank &&
		   out.transfer.destination.change.type == critical_command_type::item_transfer;
	const bool pickup =
		out.transfer.source.change.type == critical_command_type::item_transfer &&
		out.transfer.destination.change.type == critical_command_type::account_bank;
	if (!out.drop && !pickup)
		return false;
	out.wallet_index = out.drop ? 0 : 1;
	out.pile_index = out.drop ? 1 : 0;
	const auto &wallet = out.drop ? out.transfer.source : out.transfer.destination;
	const auto &endpoint = out.drop ? out.transfer.destination : out.transfer.source;
	out.pile = std::make_unique<item_transfer_payload>();
	if (!currency_command_decode_payload(wallet.change, &out.wallet) || out.wallet.pid == 0 ||
	    out.wallet.pid > INT_MAX || out.wallet.reason != currency_reason_type::coin_transfer ||
	    wallet.change.expected_revisions.size() != 2 ||
	    !item_transfer_command_decode_payload(endpoint.change, out.pile.get()))
		return false;
	const auto &pile = *out.pile;
	out.consumed = pile.to_owner.type == item_owner_type::destruction;
	if (pile.item_count != 1 || pile.multi_root || !pile.selected_item_uid ||
	    pile.selected_item_uid == UINT64_MAX || pile.target_parent_item_uid ||
	    pile.expected_target_parent_revision || pile.items[0].parent_item_uid ||
	    pile.items[0].item_uid != pile.selected_item_uid ||
	    pile.items[0].root_item_uid != pile.selected_item_uid ||
	    pile.target_root_item_uid != pile.selected_item_uid || pile.from_owner.context_id ||
	    pile.to_owner.context_id ||
	    pile.continuation.kind != item_transfer_continuation_kind::none ||
	    !pile.continuation.data.empty())
		return false;
	if (out.drop)
	{
		if (pile.from_owner.type != item_owner_type::system || pile.from_owner.id ||
		    pile.to_owner.type != item_owner_type::room || out.consumed ||
		    pile.reason != item_transfer_reason::creation ||
		    pile.items[0].expected_item_revision != ITEM_TRANSFER_ABSENT_REVISION ||
		    pile.items[0].expected_state != item_custody_state::absent)
			return false;
		out.room = pile.to_owner.id;
	}
	else
	{
		if (pile.from_owner.type != item_owner_type::room ||
		    (!out.consumed && !item_owner_identity_equal(pile.from_owner, pile.to_owner)) ||
		    (out.consumed && pile.to_owner.id) ||
		    pile.reason != (out.consumed ? item_transfer_reason::destruction :
						   item_transfer_reason::player_put) ||
		    !pile.items[0].expected_item_revision ||
		    pile.items[0].expected_item_revision == UINT64_MAX ||
		    pile.items[0].expected_state != item_custody_state::active)
			return false;
		out.room = pile.from_owner.id;
	}
	std::vector<player_item_snapshot> items;
	if (!out.room || out.room > INT_MAX ||
	    player_item_snapshot_list_decode(pile.item_blob.data(), pile.item_blob_size, &items) !=
		    player_snapshot_codec_result::ok ||
	    items.size() != 1 || items[0].object_uid != pile.selected_item_uid ||
	    items[0].vnum != pile.items[0].vnum || items[0].type != ITEM_MONEY ||
	    items[0].equipment_slot != -1 || items[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
	    items[0].string_mask != (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) ||
	    !items[0].dynamic_affects.empty() || items[0].extra_descriptions.size() != 1 ||
	    (items[0].extra_flags & (ITEM_LIT | ITEM_TRANSIENT | ITEM_ARTIFACT | ITEM_PROCLIB)))
		return false;
	out.literal = std::move(items[0]);
	return true;
}

constexpr size_t census_limit = 1000000;
using digest = std::array<unsigned char, SHA256_DIGEST_LENGTH>;
struct publication
{
	bool used = false, entered = false;
	critical_operation_id operation = {};
	uint64_t uid = 0;
	digest binding = {};
	bool native_started = false, native_returned = false;
};
std::array<publication, CURRENCY_PENDING_MAX> publications;
struct projection_cut
{
#ifndef __NO_MYSQL__
	MYSQL *connection;
	unsigned long session;
#else
	const std::string &root;
	const flatfile_identity_lock &identity;
	const flatfile_authority_lock &authority;
#endif
};
#ifndef __NO_MYSQL__
bool current_session(MYSQL *connection, unsigned long session)
{
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	return connection && session && mysql_thread_id(connection) == session &&
	       (connection->server_status & SERVER_STATUS_IN_TRANS) &&
	       (connection->server_status & SERVER_STATUS_AUTOCOMMIT) &&
	       !mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect;
}
#endif
bool current_cut(const projection_cut &cut)
{
#ifndef __NO_MYSQL__
	return current_session(cut.connection, cut.session);
#else
	return cut.identity.matches(cut.root) && cut.authority.matches(cut.root);
#endif
}
bool snapshot_equal(player_item_snapshot left, player_item_snapshot right)
{
	left.equipment_slot = right.equipment_slot = -1;
	left.parent_index = right.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	std::vector<uint8_t> a, b;
	return player_item_snapshot_list_encode({ left }, &a) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode({ right }, &b) ==
		       player_snapshot_codec_result::ok &&
	       a == b;
}
// Reuse the native renderer on unpublished dummy storage. Its allocation behavior
// remains the existing renderer's; this is not an inert constructor or callback.
bool opening_matches(player_item_snapshot actual, const shape &value)
{
	const auto &endpoint = value.transfer.source;
	if (!std::equal(endpoint.before.begin(), endpoint.before.end(), actual.values.begin()) ||
	    actual.extra_descriptions.size() != 1)
		return false;
	if (value.consumed)
		return snapshot_equal(actual, value.literal);
	struct rendering
	{
		obj_data object = {};
		extra_descr_data detail = {};
		~rendering()
		{
			if (object.description)
				str_free(object.description);
			if (object.short_description)
				str_free(object.short_description);
			if (detail.description)
				str_free(detail.description);
		}
	} rendered;
	rendered.object.type = ITEM_MONEY;
	rendered.object.ex_description = &rendered.detail;
	std::copy(endpoint.before.begin(), endpoint.before.end(), rendered.object.value);
	add_coins(&rendered.object, 0, 0, 0, 0);
	if (!rendered.object.description || !rendered.object.short_description ||
	    !rendered.detail.description || actual.weight != rendered.object.weight ||
	    actual.description != rendered.object.description ||
	    actual.short_description != rendered.object.short_description ||
	    actual.extra_descriptions[0].description != rendered.detail.description)
		return false;
	std::copy(endpoint.after.begin(), endpoint.after.end(), rendered.object.value);
	add_coins(&rendered.object, 0, 0, 0, 0);
	if (!rendered.object.description || !rendered.object.short_description ||
	    !rendered.detail.description)
		return false;
	actual.description = rendered.object.description;
	actual.short_description = rendered.object.short_description;
	actual.extra_descriptions[0].description = rendered.detail.description;
	actual.weight = rendered.object.weight;
	std::copy(endpoint.after.begin(), endpoint.after.end(), actual.values.begin());
	return snapshot_equal(actual, value.literal);
}

struct native_state
{
	currency_command_result balances = {};
	bool committed = false;
	bool item_exists = false;
	item_ownership_runtime_entry item = {};
	uint64_t from_revision = 0, to_revision = 0;
	std::optional<player_item_snapshot> literal;
};
#ifndef __NO_MYSQL__
using cell = std::optional<std::string>;
using row = std::vector<cell>;
std::vector<row> read(MYSQL *connection, const std::string &sql, size_t columns)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw std::runtime_error("coin recovery SQL query refused");
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(
		mysql_store_result(connection), mysql_free_result);
	if (!result || mysql_num_fields(result.get()) != columns ||
	    mysql_num_rows(result.get()) > 2)
		throw std::runtime_error("coin recovery SQL rows refused");
	std::vector<row> rows;
	while (MYSQL_ROW source = mysql_fetch_row(result.get()))
	{
		const auto *lengths = mysql_fetch_lengths(result.get());
		if (!lengths)
			throw std::runtime_error("coin recovery SQL lengths refused");
		row values;
		values.reserve(columns);
		for (size_t index = 0; index < columns; ++index)
			values.emplace_back(
				source[index] ? cell(std::string(source[index], lengths[index])) :
						std::nullopt);
		rows.push_back(std::move(values));
	}
	return rows;
}
template <typename Integer> Integer number(const cell &value)
{
	Integer output = 0;
	if (!value || value->empty())
		throw std::runtime_error("coin recovery SQL number absent");
	const auto parsed = std::from_chars(value->data(), value->data() + value->size(), output);
	if (parsed.ec != std::errc{} || parsed.ptr != value->data() + value->size())
		throw std::runtime_error("coin recovery SQL number malformed");
	return output;
}
std::string quoted(MYSQL *connection, const char *value)
{
	const size_t length = std::strlen(value);
	std::string escaped(2 * length + 1, '\0');
	escaped.resize(mysql_real_escape_string(connection, escaped.data(), value, length));
	return "'" + escaped + "'";
}
void read_wallet(MYSQL *connection, const shape &value, native_state &state)
{
	const auto rows = read(
		connection,
		"SELECT account_name,racewar,copper,silver,gold,platinum,wallet_revision FROM player_data WHERE pid=" +
			std::to_string(value.wallet.pid) + " LIMIT 2 FOR UPDATE",
		7);
	if (rows.size() != 1 || !rows[0][0] ||
	    rows[0][0]->size() > CURRENCY_ACCOUNT_NAME_MAX_BYTES ||
	    rows[0][0]->find('\0') != std::string::npos ||
	    strcasecmp(rows[0][0]->c_str(), value.wallet.account_name.data()) ||
	    number<uint64_t>(rows[0][1]) != value.wallet.racewar)
		throw std::runtime_error("coin recovery native wallet identity mismatch");
	for (size_t index = 0; index < 4; ++index)
	{
		const auto amount = number<int64_t>(rows[0][index + 2]);
		if (amount < 0 || amount > INT_MAX)
			throw std::runtime_error("coin recovery wallet range");
		state.balances.wallet.amount[index] = amount;
	}
	state.balances.wallet_revision = number<uint64_t>(rows[0][6]);
}
void read_bank(MYSQL *connection, const shape &value, native_state &state)
{
	const auto rows = read(
		connection,
		"SELECT bank_copper,bank_silver,bank_gold,bank_platinum,bank_revision FROM account_banks WHERE account_name=" +
			quoted(connection, value.wallet.account_name.data()) + " AND racewar=" +
			std::to_string(value.wallet.racewar) + " LIMIT 2 FOR UPDATE",
		5);
	if (rows.size() != 1)
		throw std::runtime_error("coin recovery bank absent or duplicate");
	for (size_t index = 0; index < 4; ++index)
	{
		const auto amount = number<int64_t>(rows[0][index]);
		if (amount < 0 || amount > INT_MAX)
			throw std::runtime_error("coin recovery bank range");
		state.balances.bank.amount[index] = amount;
	}
	state.balances.bank_revision = number<uint64_t>(rows[0][4]);
}
uint64_t read_owner(MYSQL *connection, const item_owner_identity &owner, bool absent_zero)
{
	const auto rows = read(connection,
			       "SELECT revision FROM item_owner_revision WHERE owner_type=" +
				       std::to_string(static_cast<unsigned>(owner.type)) +
				       " AND owner_id=" + std::to_string(owner.id) +
				       " AND owner_context_id=" + std::to_string(owner.context_id) +
				       " LIMIT 2 FOR UPDATE",
			       1);
	if (rows.empty() && absent_zero)
		return 0;
	if (rows.size() != 1)
		throw std::runtime_error("coin recovery owner counter absent");
	return number<uint64_t>(rows[0][0]);
}
void read_pile(MYSQL *connection, const shape &value, bool rejected, native_state &state)
{
	const auto &pile = *value.pile;
	const bool from_first = static_cast<unsigned>(pile.from_owner.type) <
					static_cast<unsigned>(pile.to_owner.type) ||
				(pile.from_owner.type == pile.to_owner.type &&
				 pile.from_owner.id <= pile.to_owner.id);
	const auto from = [&]
	{
		state.from_revision = read_owner(connection, pile.from_owner,
						 rejected && pile.expected_from_revision == 0);
	};
	const auto to = [&]
	{
		state.to_revision = read_owner(connection, pile.to_owner,
					       rejected && pile.expected_to_revision == 0);
	};
	if (item_owner_identity_equal(pile.from_owner, pile.to_owner))
	{
		from();
		state.to_revision = state.from_revision;
	}
	else if (from_first)
	{
		from();
		to();
	}
	else
	{
		to();
		from();
	}
	const auto rows = read(
		connection,
		"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot,OCTET_LENGTH(coin_payload),LEFT(coin_payload,131073) FROM item_current_owner WHERE item_uid=" +
			std::to_string(pile.selected_item_uid) +
			" OR root_item_uid=" + std::to_string(pile.selected_item_uid) +
			" OR parent_item_uid=" + std::to_string(pile.selected_item_uid) +
			" ORDER BY item_uid LIMIT 2 FOR UPDATE",
		12);
	if (rows.empty())
		return;
	if (rows.size() != 1)
		throw std::runtime_error("coin recovery native root is not single item");
	const auto &r = rows[0];
	state.item_exists = true;
	state.item = { number<uint64_t>(r[0]),
		       number<uint64_t>(r[1]),
		       number<uint64_t>(r[2]),
		       { static_cast<item_owner_type>(number<unsigned>(r[3])),
			 number<uint64_t>(r[4]), number<uint64_t>(r[5]) },
		       number<uint64_t>(r[6]),
		       0,
		       number<int32_t>(r[7]),
		       static_cast<item_custody_state>(number<unsigned>(r[8])) };
	if (state.item.item_uid != pile.selected_item_uid ||
	    state.item.root_item_uid != pile.selected_item_uid || state.item.parent_item_uid ||
	    state.item.vnum != pile.items[0].vnum || number<int32_t>(r[9]) != 0)
		throw std::runtime_error("coin recovery native custody graph mismatch");
	state.item.owner_revision = item_owner_identity_equal(state.item.owner, pile.from_owner) ?
					    state.from_revision :
					    state.to_revision;
	if (r[10] && r[11])
	{
		const auto length = number<size_t>(r[10]);
		std::vector<player_item_snapshot> items;
		if (length > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES || length != r[11]->size() ||
		    player_item_snapshot_list_decode(
			    reinterpret_cast<const uint8_t *>(r[11]->data()), length, &items) !=
			    player_snapshot_codec_result::ok ||
		    items.size() != 1 || items[0].object_uid != pile.selected_item_uid ||
		    items[0].vnum != pile.items[0].vnum || items[0].type != ITEM_MONEY ||
		    items[0].parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
		    std::any_of(items[0].values.begin(), items[0].values.begin() + 4,
				[](int32_t amount) { return amount < 0; }) ||
		    std::all_of(items[0].values.begin(), items[0].values.begin() + 4,
				[](int32_t amount) { return amount == 0; }))
			throw std::runtime_error("coin recovery native literal refused");
		state.literal = std::move(items[0]);
	}
	else if (r[10] || r[11])
		throw std::runtime_error("coin recovery partial null literal");
}
#endif
bool receipt_matches(const critical_apply_result &retained, const critical_completion &sealed)
{
	const bool successful = retained.outcome == critical_apply_outcome::applied ||
				retained.outcome == critical_apply_outcome::already_applied;
	const bool sealed_success = sealed.outcome == critical_apply_outcome::applied ||
				    sealed.outcome == critical_apply_outcome::already_applied;
	return (successful ? sealed_success :
			     retained.outcome == critical_apply_outcome::terminal_failure &&
				     sealed.outcome == retained.outcome) &&
	       retained.durable_revision == sealed.durable_revision &&
	       retained.error_code == sealed.error_code &&
	       retained.failure_stage == sealed.failure_stage &&
	       retained.result_size == sealed.result_size &&
	       retained.result_payload == sealed.result_payload;
}
bool current_bank_covers(const currency_command_result &current,
			 const currency_command_result &historical)
{
	return current.bank_revision >= historical.bank_revision &&
	       (current.bank_revision != historical.bank_revision ||
		current.bank.amount == historical.bank.amount);
}
bool native_matches(const shape &value, const native_state &state,
		    const critical_completion &receipt)
{
	const auto &pile = *value.pile;
	const bool rejected = receipt.outcome == critical_apply_outcome::terminal_failure;
	if (rejected)
	{
		if (coin_transfer_command_stale_result_expected(value.transfer,
								receipt.failure_stage))
		{
			coin_transfer_stale_result stale;
			if (!coin_transfer_command_decode_stale_result(
				    value.transfer, receipt.failure_stage,
				    receipt.result_payload.data(), receipt.result_size, &stale) ||
			    stale.endpoint_index != value.wallet_index ||
			    (stale.wallet_stale &&
			     (state.balances.wallet.amount != stale.current.wallet.amount ||
			      state.balances.wallet_revision != stale.current.wallet_revision)) ||
			    (stale.bank_stale &&
			     !current_bank_covers(state.balances, stale.current)))
				return false;
			// Compact stale receipts intentionally zero the unflagged half. Its
			// authority remains the current locked native row, never those zeros.
		}
		if (value.drop)
			return !state.item_exists;
		// A retained no-effect rejection does not freeze its source pile. Observe
		// the locked current single-room pile, never replay the rejected transfer
		// or infer its current literal from the obsolete command's opening text.
		return state.item_exists && state.item.item_revision != 0 &&
		       state.item.state == item_custody_state::active &&
		       item_owner_identity_equal(state.item.owner, pile.from_owner) &&
		       state.literal;
	}
	coin_transfer_result result;
	if (!coin_transfer_command_decode_result(value.transfer, receipt.result_payload.data(),
						 receipt.result_size, &result))
		return false;
	const auto &wallet = value.drop ? value.transfer.source : value.transfer.destination;
	const auto &balances = result.wallets[value.wallet_index];
	const auto &item = result.piles[value.pile_index];
	const uint64_t revision = value.drop ? 1 : pile.items[0].expected_item_revision + 1;
	if (wallet.change.expected_revisions[0].revision == UINT64_MAX ||
	    wallet.change.expected_revisions[1].revision == UINT64_MAX ||
	    pile.expected_from_revision == UINT64_MAX || pile.expected_to_revision == UINT64_MAX ||
	    state.balances.wallet.amount != balances.wallet.amount ||
	    state.balances.wallet_revision != balances.wallet_revision ||
	    !current_bank_covers(state.balances, balances) ||
	    balances.wallet_revision != wallet.change.expected_revisions[0].revision + 1 ||
	    balances.bank_revision != wallet.change.expected_revisions[1].revision + 1 ||
	    !std::equal(wallet.after.begin(), wallet.after.end(), balances.wallet.amount.begin()) ||
	    item.item_count != 1 || item.root_item_uid != pile.selected_item_uid ||
	    item.max_item_revision != revision ||
	    item.from_owner_revision != pile.expected_from_revision + 1 ||
	    item.to_owner_revision != pile.expected_to_revision + 1 || item.corpse_revision ||
	    item.collector_catalog_changed || state.from_revision < item.from_owner_revision ||
	    state.to_revision < item.to_owner_revision ||
	    receipt.durable_revision != std::max({ balances.wallet_revision, balances.bank_revision,
#ifndef __NO_MYSQL__
						   item.from_owner_revision, item.to_owner_revision,
#endif
						   item.max_item_revision }) ||
	    !state.item_exists || state.item.item_revision != revision ||
	    !item_owner_identity_equal(state.item.owner, pile.to_owner) ||
	    state.item.state !=
		    (value.consumed ? item_custody_state::destroyed : item_custody_state::active))
		return false;
	return value.consumed ? !state.literal :
				state.literal && snapshot_equal(*state.literal, value.literal);
}

struct physical
{
	P_obj object = nullptr;
	std::vector<P_char> characters;
};
bool census(const shape &value, physical &out)
{
	std::vector<P_obj> objects;
	size_t visits = 0;
	P_obj previous = nullptr;
	for (P_obj object = object_list; object; object = object->next)
	{
		if (++visits > census_limit || object->prev != previous)
			return false;
		previous = object;
		objects.push_back(object);
		if (object->obj_uid == value.pile->selected_item_uid)
		{
			if (out.object)
				return false;
			out.object = object;
		}
	}
	std::sort(objects.begin(), objects.end(), std::less<P_obj>{});
	std::vector<size_t> seen(objects.size(), 0);
	size_t generation = 0, room_links = 0;
	const auto visit = [&](P_obj object)
	{
		if (++visits > census_limit)
			return false;
		const auto found = std::lower_bound(objects.begin(), objects.end(), object,
						    std::less<P_obj>{});
		if (found == objects.end() || *found != object)
			return false;
		const size_t index = static_cast<size_t>(found - objects.begin());
		if (seen[index] == generation)
			return false;
		seen[index] = generation;
		return true;
	};
	if (!world || top_of_world < 0 || static_cast<size_t>(top_of_world) >= census_limit)
		return false;
	const int room = real_room(static_cast<int>(value.room));
	if (room < 0 || room > top_of_world || world[room].number != static_cast<int>(value.room))
		return false;
	for (int location = 0; location <= top_of_world; ++location)
	{
		if (++visits > census_limit)
			return false;
		++generation;
		for (P_obj object = world[location].contents; object; object = object->next_content)
		{
			if (!visit(object))
				return false;
			if (object->obj_uid == value.pile->selected_item_uid &&
			    (object != out.object || location != room || ++room_links != 1))
				return false;
		}
	}
	// Unknown intermediate pointers refuse: selected UIDs cannot hide below a
	// nonglobal object. Foreign containers and character lists cannot alias a pile.
	for (P_obj parent : objects)
	{
		if (++visits > census_limit)
			return false;
		++generation;
		for (P_obj child = parent->contains; child; child = child->next_content)
			if (!visit(child) || child->obj_uid == value.pile->selected_item_uid ||
			    parent == out.object)
				return false;
	}
	P_char slow = character_list, fast = character_list;
	for (P_char character = character_list; character; character = character->next)
	{
		if (++visits > census_limit)
			return false;
		if (fast && fast->next)
		{
			fast = fast->next->next;
			slow = slow->next;
			if (fast == slow)
				return false;
		}
		else
			fast = nullptr;
		out.characters.push_back(character);
		++generation;
		for (P_obj object = character->carrying; object; object = object->next_content)
			if (!visit(object) || object->obj_uid == value.pile->selected_item_uid)
				return false;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			P_obj object = character->equipment[slot];
			if (!object)
				continue;
			++generation;
			if (object &&
			    (!visit(object) || object->obj_uid == value.pile->selected_item_uid))
				return false;
		}
	}
	// Resolve the same owning PC as the live currency adapter, including a
	// switched/morphed original body. Its item links cannot alias the room pile.
	std::sort(out.characters.begin(), out.characters.end(), std::less<P_char>{});
	std::vector<P_char> descriptor_characters;
	P_desc desc_slow = descriptor_list, desc_fast = descriptor_list;
	for (P_desc descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
	{
		if (++visits > census_limit)
			return false;
		if (desc_fast && desc_fast->next)
		{
			desc_fast = desc_fast->next->next;
			desc_slow = desc_slow->next;
			if (desc_fast == desc_slow)
				return false;
		}
		else
			desc_fast = nullptr;
		if (STATE(descriptor) == CON_PLAYING && descriptor->character)
			descriptor_characters.push_back(descriptor->character);
	}
	std::sort(descriptor_characters.begin(), descriptor_characters.end(), std::less<P_char>{});
	descriptor_characters.erase(std::unique(descriptor_characters.begin(),
						descriptor_characters.end()),
				    descriptor_characters.end());
	for (P_char character : descriptor_characters)
	{
		if (std::binary_search(out.characters.begin(), out.characters.end(), character,
				       std::less<P_char>{}))
			continue;
		++generation;
		for (P_obj object = character->carrying; object; object = object->next_content)
			if (!visit(object) || object->obj_uid == value.pile->selected_item_uid)
				return false;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			if (!character->equipment[slot])
				continue;
			++generation;
			if (character->equipment[slot] &&
			    (!visit(character->equipment[slot]) ||
			     character->equipment[slot]->obj_uid == value.pile->selected_item_uid))
				return false;
		}
	}
	out.characters.insert(out.characters.end(), descriptor_characters.begin(),
			      descriptor_characters.end());
	std::sort(out.characters.begin(), out.characters.end(), std::less<P_char>{});
	out.characters.erase(std::unique(out.characters.begin(), out.characters.end()),
			     out.characters.end());
	std::vector<P_char> wallets;
	for (P_char character : out.characters)
	{
		P_char wallet = GET_PLYR(character);
		if (wallet && IS_PC(wallet) && wallet->only.pc)
			wallets.push_back(wallet);
	}
	std::sort(wallets.begin(), wallets.end(), std::less<P_char>{});
	wallets.erase(std::unique(wallets.begin(), wallets.end()), wallets.end());
	for (P_char wallet : wallets)
	{
		if (std::binary_search(out.characters.begin(), out.characters.end(), wallet,
				       std::less<P_char>{}))
			continue;
		++generation;
		for (P_obj object = wallet->carrying; object; object = object->next_content)
			if (!visit(object) || object->obj_uid == value.pile->selected_item_uid)
				return false;
		for (int slot = 0; slot < MAX_WEAR; ++slot)
		{
			if (!wallet->equipment[slot])
				continue;
			++generation;
			if (wallet->equipment[slot] &&
			    (!visit(wallet->equipment[slot]) ||
			     wallet->equipment[slot]->obj_uid == value.pile->selected_item_uid))
				return false;
		}
	}
	out.characters = std::move(wallets);
	return !out.object ||
	       (room_links == 1 && out.object->loc_p == LOC_ROOM && out.object->loc.room == room &&
		out.object->type == ITEM_MONEY && !out.object->contains && !out.object->affects &&
		!out.object->nevents && !out.object->hitched_to && !out.object->trap_eff &&
		!out.object->trap_dam && !out.object->trap_charge && !out.object->trap_level &&
		out.object->z_cord == 0 &&
		!(out.object->extra_flags &
		  (ITEM_LIT | ITEM_TRANSIENT | ITEM_ARTIFACT | ITEM_PROCLIB)));
}
bool capture_equal(P_obj object, const player_item_snapshot &literal)
{
	std::vector<player_item_snapshot> items;
	return player_item_snapshot_tree_capture_literal(object, &items, nullptr) ==
		       player_snapshot_capture_result::ok &&
	       items.size() == 1 && snapshot_equal(items[0], literal);
}
bool runtime_matches(const shape &value, const native_state &state, bool allow_before)
{
	std::vector<item_ownership_runtime_entry> entries;
	if (item_ownership_runtime_size() > census_limit ||
	    !item_ownership_runtime_snapshot_root(value.pile->selected_item_uid, 2, &entries) ||
	    entries.size() > 1)
		return false;
	for (const auto &owner : { value.pile->from_owner, value.pile->to_owner })
	{
		uint64_t cache = 0;
		const uint64_t revision = item_owner_identity_equal(owner, value.pile->from_owner) ?
						  state.from_revision :
						  state.to_revision;
		if (item_ownership_runtime_peek_owner_revision(owner, &cache) && cache > revision)
			return false;
	}
	item_ownership_runtime_entry current = {};
	const bool exists = item_ownership_runtime_lookup(value.pile->selected_item_uid, &current);
	if (!exists)
		return entries.empty();
	if (entries.size() != 1 || entries[0].item_uid != current.item_uid ||
	    current.root_item_uid != value.pile->selected_item_uid || current.parent_item_uid ||
	    current.vnum != value.pile->items[0].vnum)
		return false;
	const bool exact = state.item_exists && current.item_revision == state.item.item_revision &&
			   current.state == state.item.state &&
			   item_owner_identity_equal(current.owner, state.item.owner);
	const bool before = allow_before && !value.drop &&
			    current.item_revision == value.pile->items[0].expected_item_revision &&
			    current.state == item_custody_state::active &&
			    item_owner_identity_equal(current.owner, value.pile->from_owner);
	if (!exact && !before)
		return false;
	const bool target_revision = value.drop ||
				     state.item.item_revision !=
					     value.pile->items[0].expected_item_revision;
	if (exact && state.committed && target_revision &&
	    current.owner_revision < value.pile->expected_to_revision + 1)
		return false;
	uint64_t cache = 0;
	const uint64_t revision = item_owner_identity_equal(current.owner, value.pile->from_owner) ?
					  state.from_revision :
					  state.to_revision;
	return current.owner_revision <= revision &&
	       item_ownership_runtime_peek_owner_revision(current.owner, &cache) &&
	       cache >= current.owner_revision && cache <= revision;
}
bool body_matches(P_char character, const shape &value, const native_state &state, bool exact)
{
	if (!IS_PC(character) || !character->only.pc)
		return true;
	const bool wallet = GET_PID(character) == static_cast<int>(value.wallet.pid);
	const bool bank =
		GET_RACEWAR(character) == value.wallet.racewar &&
		!strcasecmp(get_account_name_safe(character), value.wallet.account_name.data());
	if (wallet && !bank)
		return false;
	if (wallet &&
	    (character->only.pc->wallet_revision > state.balances.wallet_revision ||
	     (exact && (character->only.pc->wallet_revision != state.balances.wallet_revision ||
			std::array<int64_t, 4>{ GET_COPPER(character), GET_SILVER(character),
						GET_GOLD(character), GET_PLATINUM(character) } !=
				state.balances.wallet.amount))))
		return false;
	return !bank || (character->only.pc->bank_revision <= state.balances.bank_revision &&
			 (!exact ||
			  (character->only.pc->bank_revision == state.balances.bank_revision &&
			   std::array<int64_t, 4>{
				   GET_BALANCE_COPPER(character), GET_BALANCE_SILVER(character),
				   GET_BALANCE_GOLD(character), GET_BALANCE_PLATINUM(character) } ==
				   state.balances.bank.amount)));
}
void project_bodies(const physical &world_state, const shape &value,
		    const native_state &state) noexcept
{
	for (P_char character : world_state.characters)
	{
		if (!IS_PC(character) || !character->only.pc)
			continue;
		if (GET_PID(character) == static_cast<int>(value.wallet.pid))
		{
			GET_COPPER(character) = static_cast<int>(state.balances.wallet.amount[0]);
			GET_SILVER(character) = static_cast<int>(state.balances.wallet.amount[1]);
			GET_GOLD(character) = static_cast<int>(state.balances.wallet.amount[2]);
			GET_PLATINUM(character) = static_cast<int>(state.balances.wallet.amount[3]);
			character->only.pc->wallet_revision = state.balances.wallet_revision;
		}
		if (GET_RACEWAR(character) == value.wallet.racewar &&
		    !strcasecmp(get_account_name_safe(character), value.wallet.account_name.data()))
		{
			GET_BALANCE_COPPER(character) =
				static_cast<int>(state.balances.bank.amount[0]);
			GET_BALANCE_SILVER(character) =
				static_cast<int>(state.balances.bank.amount[1]);
			GET_BALANCE_GOLD(character) =
				static_cast<int>(state.balances.bank.amount[2]);
			GET_BALANCE_PLATINUM(character) =
				static_cast<int>(state.balances.bank.amount[3]);
			character->only.pc->bank_revision = state.balances.bank_revision;
		}
	}
}
}

bool coin_physical_recovery_identity(const critical_command &command, int *wallet_pid,
				     uint64_t *pile_uid) noexcept
{
	if (!wallet_pid || !pile_uid)
		return false;
	try
	{
		shape value;
		if (!decode_shape(command, value))
			return false;
		*wallet_pid = static_cast<int>(value.wallet.pid);
		*pile_uid = value.pile->selected_item_uid;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

// Only this synchronous integrated owner can release an inert money stage.
class coin_physical_recovery_owner final
{
    public:
	static bool enroll(const shape &value, const native_state &native, inert_item_stage &stage)
	{
		const auto *prototype = find_recovery_object_template(native.item.vnum);
		const int room = real_room(static_cast<int>(value.room));
		if (!stage.get() || !prototype || !obj_index || room < 0 || room > top_of_world ||
		    prototype->R_num < 0 || prototype->R_num > top_of_objt ||
		    prototype->R_num != stage.object_->R_num ||
		    obj_index[prototype->R_num].virtual_number != native.item.vnum ||
		    obj_index[prototype->R_num].func.obj ||
		    obj_index[prototype->R_num].number < 0 ||
		    obj_index[prototype->R_num].number == INT_MAX ||
		    !coin_physical_publication_room_safe(room, stage.object_))
			return false;
		physical fresh;
		if (!census(value, fresh) || fresh.object || !runtime_matches(value, native, false))
			return false;
		// Last fallible enrollment step. No parser/handler/callback follows.
		if (!item_ownership_runtime_hydrate_many_atomic(&native.item, 1))
			return false;
		P_obj object = stage.object_;
		object->next = object_list;
		if (object_list)
			object_list->prev = object;
		object_list = object;
		object->loc_p = LOC_ROOM;
		object->loc.room = room;
		object->next_content = world[room].contents;
		world[room].contents = object;
		++obj_index[prototype->R_num].number;
		stage.object_ = nullptr;
		stage.pool_ = nullptr;
		return true;
	}
};

namespace
{
bool exact_runtime_identity(const item_ownership_runtime_entry &a,
			    const item_ownership_runtime_entry &b)
{
	return a.item_uid == b.item_uid && a.root_item_uid == b.root_item_uid &&
	       a.parent_item_uid == b.parent_item_uid &&
	       item_owner_identity_equal(a.owner, b.owner) && a.item_revision == b.item_revision &&
	       a.vnum == b.vnum && a.state == b.state;
}
bool project(const projection_cut &cut, const shape &value, const native_state &native,
	     const critical_completion &completion, publication &stage)
{
	const bool rejected = completion.outcome == critical_apply_outcome::terminal_failure;
	if (stage.native_started && !stage.native_returned)
		return false;
	physical actual;
	if (!census(value, actual) || !runtime_matches(value, native, !rejected))
		return false;
	for (P_char character : actual.characters)
		if (!body_matches(character, value, native, false))
			return false;
	if (rejected && value.drop)
	{
		if (actual.object || native.item_exists || !current_cut(cut))
			return false;
		project_bodies(actual, value, native);
		for (P_char character : actual.characters)
			if (!body_matches(character, value, native, true))
				return false;
		return current_cut(cut) && runtime_matches(value, native, false);
	}
	const bool active = native.item.state == item_custody_state::active;
	if (!actual.object && active)
	{
		if (!native.literal)
			return false;
		inert_item_stage prepared;
		std::array<int32_t, 4> denominations;
		std::copy_n(native.literal->values.begin(), 4, denominations.begin());
		if (prepare_inert_money_stage(*native.literal, native.item.item_uid, denominations,
					      prepared) != inert_item_stage_result::ok ||
		    !current_cut(cut) ||
		    !coin_physical_recovery_owner::enroll(value, native, prepared))
			return false;
	}
	else if (actual.object)
	{
		const bool target = active && native.literal &&
				    capture_equal(actual.object, *native.literal);
		if (!target)
		{
			if (rejected || value.drop || stage.native_returned)
				return false;
			std::vector<player_item_snapshot> captured;
			const int room = real_room(static_cast<int>(value.room));
			const int number = actual.object->R_num;
			if (!obj_index || number < 0 || number > top_of_objt ||
			    obj_index[number].virtual_number != value.literal.vnum ||
			    obj_index[number].func.obj ||
			    !coin_physical_publication_room_safe(room, actual.object) ||
			    player_item_snapshot_tree_capture_literal(actual.object, &captured,
								      nullptr) !=
				    player_snapshot_capture_result::ok ||
			    captured.size() != 1 || !opening_matches(captured[0], value) ||
			    !current_cut(cut))
				return false;
			stage.native_started = true;
			if (value.consumed)
				extract_obj(actual.object, FALSE);
			else
			{
				std::copy(value.transfer.source.after.begin(),
					  value.transfer.source.after.end(), actual.object->value);
				add_coins(actual.object, 0, 0, 0, 0);
			}
			stage.native_returned = true;
		}
	}
	if (!current_cut(cut))
		return false;
	physical final;
	if (!census(value, final) ||
	    (active ? !final.object || !native.literal ||
			      !capture_equal(final.object, *native.literal) :
		      final.object != nullptr) ||
	    !runtime_matches(value, native, !rejected))
		return false;
	for (P_char character : final.characters)
		if (!body_matches(character, value, native, false))
			return false;
	item_ownership_runtime_entry current = {};
	if (!item_ownership_runtime_lookup(native.item.item_uid, &current) ||
	    !exact_runtime_identity(current, native.item))
	{
		if (!item_ownership_runtime_hydrate_many_atomic(&native.item, 1))
			return false;
	}
	// These are the locked current rows, including a bank advanced by another
	// same-bank player. A rejected transfer also repairs stale loaded projection;
	// it never substitutes its compact/zero/historical result for native balances.
	project_bodies(final, value, native);
	for (P_char character : final.characters)
		if (!body_matches(character, value, native, true))
			return false;
	return current_cut(cut) && runtime_matches(value, native, false) &&
	       item_ownership_runtime_lookup(native.item.item_uid, &current) &&
	       exact_runtime_identity(current, native.item);
}
#ifndef __NO_MYSQL__
bool observe_and_project(MYSQL *connection, unsigned long session, const critical_command &command,
			 const critical_completion &completion, const shape &value,
			 publication &stage)
{
	economic_frozen_intent intent;
	if (economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, intent) != economic_accounting_error::ok ||
	    intent.admission.facts.size() != ECONOMIC_COIN_TRANSFER_FACT_BYTES)
		return false;
	uint64_t lifetime = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		lifetime |=
			static_cast<uint64_t>(intent.admission.facts[8 * value.wallet_index + byte])
			<< (8 * byte);
	economic_sql_mapping_request request{ { intent.admission.metadata.lineage,
						economic_account_kind::wallet, lifetime, 0 },
					      1,
					      value.wallet.pid };
	economic_sql_authority_snapshot authority;
	if (economic_sql_lock_authority(
		    connection, intent.admission.metadata.lineage, intent.admission.metadata.epoch,
		    std::span<const economic_sql_mapping_request>(&request, 1), &authority))
		return false;
	native_state native;
	const bool rejected = completion.outcome == critical_apply_outcome::terminal_failure;
	native.committed = !rejected;
	read_wallet(connection, value, native);
	// Native admission locks player first; endpoint order decides which native
	// bank/pile locks follow. Mutation preconditions cannot serve recovery proof.
	if (value.drop)
	{
		read_bank(connection, value, native);
		read_pile(connection, value, rejected, native);
	}
	else
	{
		read_pile(connection, value, rejected, native);
		read_bank(connection, value, native);
	}
	if (!current_session(connection, session))
		return false;
	const auto retained =
		critical_command_repository_verify_coin_in_transaction(connection, command);
	if (!receipt_matches(retained, completion) || !current_session(connection, session) ||
	    !native_matches(value, native, completion))
		return false;
	return project(projection_cut{ connection, session }, value, native, completion, stage);
}
#else
bool observe_and_project_flat(const projection_cut &cut, const critical_command &command,
			      const critical_completion &completion, const shape &value,
			      publication &stage)
{
	const auto &root = cut.root;
	const auto &lock = cut.authority;
	std::string error;
	if (!current_cut(cut) || flatfile_player_domain_recover_locked(root, lock, &error) !=
					 flatfile_player_domain_result::ok)
		return false;
	flatfile_identity_record identity;
	if (flatfile_identity_lookup_pid_locked(root, cut.identity, lock,
						static_cast<int32_t>(value.wallet.pid), &identity,
						&error) != flatfile_identity_result::ok ||
	    !identity.active ||
	    strcasecmp(identity.account.c_str(), value.wallet.account_name.data()) ||
	    identity.racewar != value.wallet.racewar)
		return false;
	economic_frozen_intent intent;
	if (economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, intent) != economic_accounting_error::ok ||
	    intent.admission.facts.size() != ECONOMIC_COIN_TRANSFER_FACT_BYTES)
		return false;
	uint64_t lifetime = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		lifetime |= uint64_t(intent.admission.facts[8 * value.wallet_index + byte])
			    << (8 * byte);
	flatfile_economic_mapping_request request{ { intent.admission.metadata.lineage,
						     economic_account_kind::wallet, lifetime, 0 },
						   { 1, value.wallet.pid, {} } };
	flatfile_economic_authority_snapshot authority;
	if (economic_flatfile_lock_authority(
		    root, lock, intent.admission.metadata.lineage, intent.admission.metadata.epoch,
		    std::span<const flatfile_economic_mapping_request>(&request, 1), &authority,
		    &error))
		return false;
	// Prove the retained historical root without replaying native mutation.
	const auto retained =
		flatfile_accounting_coin_transaction::verify_retained_locked(root, lock, command);
	if (!receipt_matches(retained, completion) || !current_cut(cut))
		return false;
	native_state native;
	const bool rejected = completion.outcome == critical_apply_outcome::terminal_failure;
	native.committed = !rejected;
	flatfile_player_domain_record domain;
	if (flatfile_player_domain_load_locked(root, lock, static_cast<int32_t>(value.wallet.pid),
					       value.wallet.account_name.data(),
					       static_cast<int8_t>(value.wallet.racewar), &domain,
					       &error) != flatfile_player_domain_result::ok)
		return false;
	for (size_t index = 0; index < 4; ++index)
	{
		if (domain.domains.wallet[index] > INT_MAX || domain.domains.bank[index] > INT_MAX)
			return false;
		native.balances.wallet.amount[index] =
			static_cast<int64_t>(domain.domains.wallet[index]);
		native.balances.bank.amount[index] =
			static_cast<int64_t>(domain.domains.bank[index]);
	}
	native.balances.wallet_revision = domain.domains.wallet_revision;
	native.balances.bank_revision = domain.domains.bank_revision;
	const auto &pile = *value.pile;
	const auto read_owner =
		[&](const item_owner_identity &owner, uint64_t expected, uint64_t &revision)
	{
		std::vector<flatfile_item_ownership_record> items;
		const auto result = flatfile_item_repository_load_owner_locked(
			root, lock, owner, &revision, &items, &error);
		if (result == flatfile_item_repository_result::not_found && rejected && !expected)
		{
			revision = 0;
			return true;
		}
		return result == flatfile_item_repository_result::ok;
	};
	if (!read_owner(pile.from_owner, pile.expected_from_revision, native.from_revision))
		return false;
	if (item_owner_identity_equal(pile.from_owner, pile.to_owner))
		native.to_revision = native.from_revision;
	else if (!read_owner(pile.to_owner, pile.expected_to_revision, native.to_revision))
		return false;
	std::vector<flatfile_item_ownership_record> catalog;
	if (flatfile_item_repository_recovery_catalog_locked(root, lock, &catalog, &error) !=
		    flatfile_item_repository_result::ok ||
	    catalog.size() > census_limit)
		return false;
	const flatfile_item_ownership_record *selected = nullptr;
	for (const auto &entry : catalog)
		if (entry.item_uid == pile.selected_item_uid ||
		    entry.root_item_uid == pile.selected_item_uid ||
		    entry.parent_item_uid == pile.selected_item_uid)
		{
			if (selected || entry.item_uid != pile.selected_item_uid ||
			    entry.root_item_uid != pile.selected_item_uid ||
			    entry.parent_item_uid || entry.equipment_slot ||
			    entry.vnum != pile.items[0].vnum)
				return false;
			selected = &entry;
		}
	if (selected)
	{
		native.item_exists = true;
		native.item = { selected->item_uid,
				selected->root_item_uid,
				selected->parent_item_uid,
				selected->owner,
				selected->item_revision,
				item_owner_identity_equal(selected->owner, pile.from_owner) ?
					native.from_revision :
					native.to_revision,
				selected->vnum,
				selected->state };
		if (selected->state == item_custody_state::active)
		{
			flatfile_coin_pile_source source;
			if (flatfile_item_repository_read_coin_pile_locked(
				    root, lock, selected->item_uid, &source, &error) !=
				    flatfile_item_repository_result::ok ||
			    source.item.type != ITEM_MONEY ||
			    source.item.object_uid != selected->item_uid ||
			    source.item.vnum != selected->vnum ||
			    source.item.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
			    std::any_of(source.item.values.begin(), source.item.values.begin() + 4,
					[](int32_t amount) { return amount < 0; }) ||
			    std::all_of(source.item.values.begin(), source.item.values.begin() + 4,
					[](int32_t amount) { return amount == 0; }))
				return false;
			native.literal = std::move(source.item);
		}
		else if (!selected->coin_payload.empty())
			return false;
		flatfile_accounting_pile_state head;
		if (flatfile_accounting_pile_state_read(root, lock, selected->item_uid, &head,
							&error) != flatfile_accounting_status::ok ||
		    !economic_account_key_equal(head.account, { intent.admission.metadata.lineage,
								economic_account_kind::pile,
								selected->item_uid, 0 }) ||
		    head.item_revision != selected->item_revision ||
		    head.retired != (selected->state == item_custody_state::destroyed))
			return false;
		for (size_t index = 0; index < 4; ++index)
			if (head.balance[index] !=
			    (native.literal ? native.literal->values[index] : 0))
				return false;
	}
	else
	{
		flatfile_accounting_pile_state head;
		if (flatfile_accounting_pile_state_read(root, lock, pile.selected_item_uid, &head,
							&error) !=
		    flatfile_accounting_status::not_found)
			return false;
	}
	// Cold native boot materializes room.items directly. A matching custody
	// coin_payload is not proof of the durable world literal that boot will use.
	// Read the same current authority cut before any live repair or ACK.
	std::vector<flatfile_corpse_record> corpses;
	std::vector<flatfile_room_item_record> rooms;
	if (flatfile_world_item_recovery_list_locked(root, lock, &corpses, &rooms, &error) !=
	    flatfile_world_item_result::ok)
		return false;
	for (const auto &corpse : corpses)
		for (const auto &item : corpse.items)
			if (item.object_uid == pile.selected_item_uid)
				return false;
	const bool active = native.item_exists && native.item.state == item_custody_state::active;
	size_t room_matches = 0;
	for (const auto &room : rooms)
	{
		const bool owning_room = room.room_vnum > 0 &&
					 static_cast<uint64_t>(room.room_vnum) == value.room;
		if (owning_room &&
		    room.revision != (value.drop ? native.to_revision : native.from_revision))
			return false;
		for (size_t index = 0; index < room.items.size(); ++index)
		{
			const auto &item = room.items[index];
			if (item.object_uid != pile.selected_item_uid)
				continue;
			if (!active || !owning_room || ++room_matches != 1 || !native.literal ||
			    item.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
			    item.equipment_slot != -1 || !snapshot_equal(item, *native.literal))
				return false;
			for (const auto &child : room.items)
				if (child.parent_index == index)
					return false;
		}
	}
	// The validated catalog also rejects duplicate UIDs across room, corpse,
	// and saved-item records. Active catalog-only/saved-item-only placements are
	// not proven loadable after journal removal; retain their publication hold.
	// A consumed pile must be absent from every durable cold room/corpse graph.
	if (active && room_matches != 1)
		return false;
	return current_cut(cut) && native_matches(value, native, completion) &&
	       project(cut, value, native, completion, stage) && current_cut(cut);
}
#endif
}

bool coin_physical_recovery_publish(const critical_command &command,
				    const critical_completion &completion) noexcept
{
	if (!nevent_is_game_thread())
		return false;
	try
	{
		shape value;
		const bool rejected = completion.outcome ==
				      critical_apply_outcome::terminal_failure;
		if (!decode_shape(command, value) ||
		    !critical_operation_id_equal(command.operation_id, completion.operation_id) ||
		    completion.disposition != critical_completion_disposition::execution ||
		    (rejected ? !completion.error_code :
				completion.error_code ||
					(completion.outcome != critical_apply_outcome::applied &&
					 completion.outcome !=
						 critical_apply_outcome::already_applied)) ||
		    completion.result_size > completion.result_payload.size())
			return false;
		std::vector<uint8_t> binding;
		if (critical_command_encode(command, &binding) != critical_command_codec_result::ok)
			return false;
		const auto append = [&](uint64_t number)
		{
			for (size_t byte = 0; byte < 8; ++byte)
				binding.push_back(static_cast<uint8_t>(number >> (byte * 8)));
		};
		append(rejected);
		append(completion.durable_revision);
		append(completion.error_code);
		append(static_cast<unsigned>(completion.failure_stage));
		append(completion.result_size);
		binding.insert(binding.end(), completion.result_payload.begin(),
			       completion.result_payload.end());
		digest hash;
		SHA256(binding.data(), binding.size(), hash.data());
		publication *stage = nullptr, *empty = nullptr;
		for (auto &candidate : publications)
		{
			if (!candidate.used)
			{
				if (!empty)
					empty = &candidate;
				continue;
			}
			if (candidate.uid == value.pile->selected_item_uid &&
			    !critical_operation_id_equal(candidate.operation, command.operation_id))
				return false;
			if (critical_operation_id_equal(candidate.operation, command.operation_id))
				stage = &candidate;
		}
		if (!stage)
		{
			if (!empty)
				return false;
			stage = empty;
			stage->used = true;
			stage->operation = command.operation_id;
			stage->uid = value.pile->selected_item_uid;
			stage->binding = hash;
		}
		if (stage->binding != hash || stage->entered)
			return false;
		struct entered_guard
		{
			publication &stage;
			~entered_guard() { stage.entered = false; }
		} entered{ *stage };
		stage->entered = true;
#ifndef __NO_MYSQL__
		player_sql_pool_lease lease(sql_pool_acquire());
		MYSQL *connection = lease.get();
		if (!connection || player_sql_idle_error(connection))
			return false;
		player_sql_cleanup proof;
		player_sql_transaction_cleanup transaction(connection, proof);
		transaction.starting();
		bool complete = false;
		try
		{
			if (!mysql_real_query(connection, "START TRANSACTION", 17) &&
			    current_session(connection, proof.original_session))
				complete = observe_and_project(connection, proof.original_session,
							       command, completion, value, *stage);
		}
		catch (...)
		{
			complete = false;
		}
		transaction.finish();
		if (!proof.rollback_confirmed || proof.cleanup_error ||
		    proof.disposition != player_sql_cleanup_disposition::idle_verified)
			return false;
		lease.reuse(proof);
		if (!complete)
			return false;
#else
		const char *configured = persistence_mode_flatfile_root();
		if (!configured || !*configured)
			return false;
		const std::string root(configured);
		std::string error;
		// Match native flat transactions: identity lock always precedes authority.
		flatfile_identity_lock identity;
		flatfile_authority_lock authority;
		if (!identity.acquire(root, &error) || !authority.acquire(root, &error) ||
		    !observe_and_project_flat(projection_cut{ root, identity, authority }, command,
					      completion, value, *stage))
			return false;
#endif
		// After confirmed success, a later guarded ACK retry reacquires exact
		// proof and sees target bytes/absence; it never repeats the old effect.
		stage->used = false;
		stage->native_started = stage->native_returned = false;
		return true;
	}
	catch (...)
	{
		return false;
	}
}
