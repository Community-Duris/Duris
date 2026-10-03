// Native component proof only. Every capture, queue, journal, repository and pool
// implementation is production code. Services and migrated schema are external.
#include "core/prototypes.h"
#include "core/files.h"
#include "item/item_ownership_runtime.h"
#include "economy/item_transfer_accounting.h"
#include "magic/spell_item_lifecycle.h"
#include "player/player_save_pipeline.h"
#include "player/player_save_journal.h"
#include "player/player_save_worker.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_repository.h"
#include "player/craft_progression_hooks.h"
#include "sql/sql_pool.h"
#include "persistence/sql_room_item_payload.h"
#include "world/quest_reward_recovery.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>

namespace
{
constexpr int fixture_pid = 60519;
constexpr uint64_t first_uid = 930100;
constexpr auto item_components = PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;
const std::thread::id game_thread = std::this_thread::get_id();
std::mutex barrier_mutex;
std::condition_variable barrier_changed;
unsigned barriers_requested = 0, barriers_entered = 0, barriers_released = 0;
std::atomic<bool> hide_next_commit{ false };
std::atomic<MYSQL *> hidden_connection{ nullptr };
std::atomic<unsigned> hidden_commits{ 0 }, pool_acquisitions{ 0 }, pool_releases{ 0 },
	replacements{ 0 };
MYSQL *escape_connection = nullptr;
std::atomic<bool> fail_next_game_allocation{ false }, forbid_game_allocation{ false };

void check(bool condition, const char *message)
{
	if (!condition)
	{
		std::cerr << "FAIL: " << message << '\n';
		std::abort();
	}
}

const char *required(const char *name)
{
	const char *value = std::getenv(name);
	check(value && *value, "missing required disposable fixture environment");
	return value;
}

void guard_target()
{
	check(!std::strcmp(required("TEST_DB_DISPOSABLE"), "1") &&
		      !std::strcmp(required("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA"), "1"),
	      "disposable markers");
	check(!std::strcmp(required("DB_HOST"), "127.0.0.1"), "loopback SQL only");
	check(!std::getenv("DB_SOCKET") && !std::getenv("TEST_DB_SOCKET"), "no inherited socket");
	const std::string schema = required("DB_NAME");
	const std::string prefix = "economic_schema_test_li_";
	check(schema.starts_with(prefix) && schema.size() == prefix.size() + 8 &&
		      schema.find_first_not_of("0123456789abcdef", prefix.size()) ==
			      std::string::npos,
	      "exact private schema namespace");
	check(std::strlen("duris.player.death.restitution.") + schema.size() <= 64,
	      "native MySQL advisory lock name bound");
	check(required("DB_ALLOWED_TARGETS") == "127.0.0.1/" + schema, "explicit schema allowlist");
}

MYSQL *connect_fixture(unsigned long flags)
{
	guard_target();
	char *end = nullptr;
	const auto port = std::strtoul(required("DB_PORT"), &end, 10);
	check(end && !*end && port > 0 && port <= 65535, "bounded explicit fixture port");
	MYSQL *connection = mysql_init(nullptr);
	check(connection, "mysql_init");
	unsigned int timeout = 5, protocol = MYSQL_PROTOCOL_TCP;
	const bool reconnect = false;
	check(!mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect) &&
		      !mysql_options(connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout) &&
		      !mysql_options(connection, MYSQL_OPT_READ_TIMEOUT, &timeout) &&
		      !mysql_options(connection, MYSQL_OPT_WRITE_TIMEOUT, &timeout) &&
		      !mysql_options(connection, MYSQL_OPT_PROTOCOL, &protocol),
	      "configured native SQL options");
	check(mysql_real_connect(connection, "127.0.0.1", required("DB_USER"),
				 required("DB_PASSWD"), required("DB_NAME"),
				 static_cast<unsigned int>(port), nullptr, flags),
	      "guarded fixture connection");
	check(!mysql_set_character_set(connection, "utf8mb4"), "fixture SQL charset");
	return connection;
}

std::vector<std::vector<std::string>> rows(MYSQL *connection, const std::string &query)
{
	check(!mysql_real_query(connection, query.data(), query.size()), "fixture SQL query");
	MYSQL_RES *result = mysql_store_result(connection);
	check(result, "fixture SQL row result");
	std::vector<std::vector<std::string>> values;
	while (MYSQL_ROW row = mysql_fetch_row(result))
	{
		const auto *lengths = mysql_fetch_lengths(result);
		std::vector<std::string> value;
		for (unsigned index = 0; index < mysql_num_fields(result); ++index)
			value.emplace_back(row[index] ? std::string(row[index], lengths[index]) :
							"<NULL>");
		values.push_back(std::move(value));
	}
	mysql_free_result(result);
	return values;
}

void execute(MYSQL *connection, const std::string &query)
{
	check(!mysql_real_query(connection, query.data(), query.size()), "fixture SQL write");
}

uint64_t scalar(MYSQL *connection, const std::string &query)
{
	const auto value = rows(connection, query);
	check(value.size() == 1 && value[0].size() == 1, "one native SQL scalar");
	return std::stoull(value[0][0]);
}

template <class Predicate> void await(Predicate predicate)
{
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
	while (!predicate())
	{
		check(std::chrono::steady_clock::now() < deadline, "bounded native pipeline wait");
		player_save_pipeline_pulse();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void await_barrier(unsigned index)
{
	await(
		[&]
		{
			std::lock_guard<std::mutex> lock(barrier_mutex);
			return barriers_entered >= index;
		});
}

void release_barrier(unsigned index)
{
	std::lock_guard<std::mutex> lock(barrier_mutex);
	barriers_released = index;
	barrier_changed.notify_all();
}

player_revision_snapshot revision()
{
	player_revision_snapshot value{};
	check(player_revision_snapshot_copy(fixture_pid, &value), "native revision state");
	return value;
}

bool same_revision(const player_revision_snapshot &before, const player_revision_snapshot &after)
{
	return before.pid == after.pid && before.current_revision == after.current_revision &&
	       before.acknowledged_revision == after.acknowledged_revision &&
	       before.queued_revision == after.queued_revision &&
	       before.inflight_revision == after.inflight_revision &&
	       before.dirty_components == after.dirty_components &&
	       before.unacknowledged_components == after.unacknowledged_components &&
	       before.queued_components == after.queued_components &&
	       before.inflight_components == after.inflight_components &&
	       before.overflowed == after.overflowed;
}

std::vector<uint8_t> encode(const std::vector<player_item_snapshot> &items)
{
	std::vector<uint8_t> value;
	check(player_item_snapshot_list_encode(items, &value) == player_snapshot_codec_result::ok,
	      "production canonical item encoding");
	return value;
}

std::string hex(const std::string &value)
{
	constexpr char digits[] = "0123456789ABCDEF";
	std::string result;
	for (unsigned char byte : value)
	{
		result += digits[byte >> 4];
		result += digits[byte & 15];
	}
	return result;
}

struct live_graph
{
	char_data actor{};
	pc_only_data pc{};
	std::array<obj_data, 6> objects{};
	std::array<std::array<std::string, 4>, 6> strings{};
	std::array<obj_affect, 6> dynamic{};
	std::array<extra_descr_data, 6> descriptions{};

	live_graph()
	{
		actor.only.pc = &pc;
		pc.pid = fixture_pid;
		actor.runtime_id = allocate_character_runtime_id();
		register_character_runtime_id(&actor);
		actor.in_room = 0;
		actor.specials.position = STAT_NORMAL | POS_STANDING;
		for (size_t index = 0; index < objects.size(); ++index)
		{
			auto &object = objects[index];
			object.obj_uid = first_uid + index;
			object.R_num = index % 2;
			object.g_key = INT64_C(9007199254740993) + index;
			object.type = index < 2 ? ITEM_CONTAINER : ITEM_NOTE;
			strings[index] = { "literal keys ' \\ " + std::to_string(index),
					   "literal short " + std::to_string(index),
					   "literal description\r\n" + std::to_string(index),
					   "literal action " + std::to_string(index) };
			assign_strings(index);
			object.str_mask = index == 0 ? 0 : index == 4 ? STRUNG_KEYS : STRUNG_DESC2;
			object.weight = 41 + index;
			object.cost = 71 + index;
			object.condition = 21 + index;
			object.craftsmanship = 31 + index;
			object.material = 2;
			object.wear_flags = 3;
			object.extra_flags = 0;
			object.anti_flags = 5;
			object.anti2_flags = 7;
			object.extra2_flags = 11;
			object.bitvector = UINT64_C(9007199254740993) + index;
			object.bitvector2 = 2;
			object.bitvector3 = 3;
			object.bitvector4 = 4;
			object.bitvector5 = 5;
			for (size_t number = 0; number < 8; ++number)
				object.value[number] = 101 + index * 10 + number;
			for (size_t number = 0; number < 6; ++number)
				object.timer[number] =
					INT64_C(9007199254740993) + index * 10 + number;
			// The maintained legacy projection stores timer zero in SQL INT;
			// other timers and the runtime blob exercise lossless 64-bit values.
			object.timer[0] = 61 + index;
			for (size_t number = 0; number < MAX_OBJ_AFFECT; ++number)
				object.affected[number] = { static_cast<signed char>(number + 1),
							    static_cast<sbyte>(41 + index * 10 +
									       number) };
			dynamic[index].type = 7;
			dynamic[index].data = 91 + index;
			dynamic[index].extra2 = 0x40;
			object.affects = &dynamic[index];
			descriptions[index].keyword = const_cast<char *>("detail ' \\");
			descriptions[index].description =
				const_cast<char *>("retained detail\nwith quote '");
			object.ex_description = &descriptions[index];
			object.loc_p = LOC_CARRIED;
			object.loc.carrying = &actor;
		}
		actor.carrying = &objects[0];
		objects[0].next_content = &objects[4];
		objects[0].contains = &objects[1];
		objects[1].next_content = &objects[3];
		objects[1].contains = &objects[2];
		for (size_t index : { 1U, 2U, 3U })
		{
			objects[index].loc_p = LOC_INSIDE;
			objects[index].loc.inside = &objects[index == 2 ? 1 : 0];
		}
		actor.equipment[0] = &objects[5];
		objects[5].loc_p = LOC_WORN;
		objects[5].loc.wearing = &actor;
	}

	~live_graph() { unregister_character_runtime_id(&actor); }

	void assign_strings(size_t index)
	{
		auto &object = objects[index];
		auto &value = strings[index];
		object.name = value[0].data();
		object.short_description = value[1].data();
		object.description = value[2].data();
		object.action_description = value[3].data();
	}

	// Independent expected values come from the synthetic source fields. Never
	// call the production capture adapter to construct its own expected result.
	std::vector<player_item_snapshot> expected() const
	{
		std::vector<player_item_snapshot> value;
		for (size_t index : { 5U, 0U, 1U, 2U, 3U, 4U })
		{
			const auto &object = objects[index];
			player_item_snapshot item{};
			item.object_uid = object.obj_uid;
			item.generated_key = object.g_key;
			item.vnum = index % 2 ? 5 : 48;
			item.type = object.type;
			item.parent_index = index == 1 || index == 3 ? 1 :
					    index == 2		     ? 2 :
								       PLAYER_SNAPSHOT_NO_PARENT;
			item.equipment_slot = index == 5 ? 1 : 0;
			item.string_mask = index < 4 ? 15 : object.str_mask;
			if (item.string_mask & STRUNG_KEYS)
				item.name = strings[index][0];
			if (item.string_mask & STRUNG_DESC2)
				item.short_description = strings[index][1];
			if (item.string_mask & STRUNG_DESC1)
				item.description = strings[index][2];
			if (item.string_mask & STRUNG_DESC3)
				item.action_description = strings[index][3];
			item.weight = object.weight;
			item.cost = object.cost;
			item.condition = object.condition;
			item.craftsmanship = object.craftsmanship;
			item.material = object.material;
			item.wear_flags = object.wear_flags;
			item.extra_flags = object.extra_flags;
			item.anti_flags = object.anti_flags;
			item.anti2_flags = object.anti2_flags;
			item.extra2_flags = object.extra2_flags;
			item.bitvectors = { object.bitvector, object.bitvector2, object.bitvector3,
					    object.bitvector4, object.bitvector5 };
			std::copy(std::begin(object.value), std::end(object.value),
				  item.values.begin());
			std::copy(std::begin(object.timer), std::end(object.timer),
				  item.timers.begin());
			for (size_t number = 0; number < item.affects.size(); ++number)
				item.affects[number] = { object.affected[number].location,
							 object.affected[number].modifier };
			item.dynamic_affects.push_back({ dynamic[index].type, dynamic[index].data,
							 dynamic[index].extra2 });
			player_item_extra_description_snapshot description{};
			description.keyword = descriptions[index].keyword;
			description.description = descriptions[index].description;
			item.extra_descriptions.push_back(std::move(description));
			value.push_back(std::move(item));
		}
		return value;
	}
};

critical_operation_id restored_operation(uint32_t serial)
{
	critical_operation_id operation{};
	operation.bytes[0] = 0x72;
	for (unsigned index = 0; index < 4; ++index)
		operation.bytes[index + 1] = static_cast<uint8_t>(serial >> (8 * index));
	return operation;
}

std::vector<player_item_snapshot> captured_drop_graph(live_graph &graph)
{
	std::vector<player_item_snapshot> items;
	size_t estimated = 0;
	check(player_item_snapshot_tree_capture_literal(&graph.objects[0], &items, &estimated) ==
			      player_snapshot_capture_result::ok &&
		      items.size() == 4 && estimated,
	      "actual literal runtime tree capture for restored command");
	return items;
}

item_transfer_payload restored_payload(const std::vector<player_item_snapshot> &items, int pid)
{
	item_transfer_payload payload{};
	payload.from_owner = { item_owner_type::player, static_cast<uint64_t>(pid), 0 };
	payload.to_owner = { item_owner_type::room, 22800, 0 };
	payload.reason = item_transfer_reason::player_drop;
	payload.reason_id = 22800;
	payload.expected_from_revision = payload.expected_to_revision = 1;
	payload.selected_item_uid = payload.target_root_item_uid = items.front().object_uid;
	payload.item_count = static_cast<uint16_t>(items.size());
	for (size_t index = 0; index < items.size(); ++index)
		payload.items[index] = { items[index].object_uid,
					 items.front().object_uid,
					 index ? items[items[index].parent_index].object_uid : 0,
					 1,
					 items[index].vnum,
					 item_custody_state::active };
	const auto blob = encode(items);
	check(blob.size() <= payload.item_blob.size(), "bounded canonical drop graph");
	payload.item_blob_size = blob.size();
	std::copy(blob.begin(), blob.end(), payload.item_blob.begin());
	return payload;
}

critical_command restored_command(const item_transfer_payload &payload, uint32_t serial)
{
	critical_command command{};
	check(item_transfer_command_build(&command, restored_operation(serial), payload,
					  critical_source_site::command,
					  critical_deadline_class::interactive),
	      "actual item-transfer command builder");
	command.accepted_at_usec = 1;
	std::vector<uint8_t> intent;
	check(item_transfer_accounting_intent(command, restored_operation(9001),
					      restored_operation(9002), payload.from_owner.id,
					      &intent) == economic_accounting_error::ok,
	      "actual typed EAI1 drop intent builder");
	command.schema_version = CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	command.accounting_intent = std::move(intent);
	// The generic builder creates an unretained schema-1 command. Retention
	// belongs to the final schema-2 envelope, after freezing its admission.
	command.publication_required = true;
	check(critical_command_envelope_valid(command), "valid retained accounting envelope");
	check(item_transfer_accounting_command_supported(command),
	      "structurally supported typed accounting command");
	return command;
}

size_t encoded_command_bytes(const critical_command &command)
{
	std::vector<uint8_t> encoded;
	check(critical_command_encode(command, &encoded) == critical_command_codec_result::ok,
	      "actual original command encoding");
	return encoded.size();
}

std::vector<player_item_snapshot> padded_drop_graph(std::vector<player_item_snapshot> items,
						    size_t target)
{
	check(encode(items).size() <= target && target <= ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES,
	      "bounded canonical padding target");
	while (encode(items).size() < target)
	{
		player_item_extra_description_snapshot description{};
		description.keyword = "capacity";
		items[0].extra_descriptions.push_back(std::move(description));
		const auto size = encode(items).size();
		if (size > target)
		{
			items[0].extra_descriptions.pop_back();
			const auto remaining = target - encode(items).size();
			check(items[0].name.size() + remaining <= PLAYER_SNAPSHOT_MAX_STRING_BYTES,
			      "short final padding remains a bounded literal string");
			items[0].name.append(remaining, 'x');
			break;
		}
		items[0].extra_descriptions.back().description.assign(
			std::min(target - size, PLAYER_SNAPSHOT_MAX_STRING_BYTES), 'x');
	}
	check(encode(items).size() == target, "exact canonical graph byte bound");
	return items;
}

void verify_projection(MYSQL *connection, const std::vector<player_item_snapshot> &expected,
		       uint64_t durable)
{
	check(scalar(connection, "SELECT save_revision FROM player_data WHERE pid=" +
					 std::to_string(fixture_pid)) == durable,
	      "exact physical save revision");
	const auto actual = rows(
		connection,
		"SELECT item.obj_uid,COALESCE(parent.obj_uid,0),item.equip_slot,HEX(item.name),HEX(item.short_descr),"
		"HEX(item.description),HEX(item.action_descr),runtime.payload FROM player_items item "
		"LEFT JOIN player_items parent ON parent.id=item.container_id "
		"JOIN player_item_runtime_state runtime ON runtime.item_id=item.id WHERE item.pid=" +
			std::to_string(fixture_pid) + " ORDER BY item.id");
	check(actual.size() == expected.size(), "all original UID runtime rows retained");
	for (size_t index = 0; index < expected.size(); ++index)
	{
		const auto &wanted = expected[index];
		const auto &found = actual[index];
		check(found.size() == 8 && std::stoull(found[0]) == wanted.object_uid &&
			      std::stoull(found[1]) ==
				      (wanted.parent_index < 0 ?
					       0 :
					       expected[wanted.parent_index].object_uid) &&
			      std::stoi(found[2]) == wanted.equipment_slot,
		      "original SQL UID/topology/equipment");
		const std::array<std::pair<uint8_t, std::string>, 4> strings = {
			{ { 1, wanted.name },
			  { 4, wanted.short_description },
			  { 2, wanted.description },
			  { 8, wanted.action_description } }
		};
		for (size_t number = 0; number < strings.size(); ++number)
			check(found[3 + number] == (wanted.string_mask & strings[number].first ?
							    hex(strings[number].second) :
							    "<NULL>"),
			      "physical SQL literal strings and ordinary sibling NULL masks");
		std::vector<player_item_snapshot> decoded;
		check(player_item_snapshot_list_decode(
			      reinterpret_cast<const uint8_t *>(found[7].data()), found[7].size(),
			      &decoded) == player_snapshot_codec_result::ok &&
			      decoded.size() == 1,
		      "production runtime payload decoder");
		auto standalone = wanted;
		standalone.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		check(encode(decoded) == encode({ standalone }),
		      "every original native item payload field");
		const std::string uid = std::to_string(wanted.object_uid);
		const auto affects = rows(
			connection,
			"SELECT affect.location,affect.modifier FROM player_item_affects affect "
			"JOIN player_items item ON item.id=affect.item_id WHERE item.obj_uid=" +
				uid + " ORDER BY affect.location,affect.modifier");
		check(affects.size() == wanted.affects.size(), "all physical static affects");
		for (size_t number = 0; number < affects.size(); ++number)
			check(affects[number] ==
				      std::vector<std::string>{
					      std::to_string(wanted.affects[number][0]),
					      std::to_string(wanted.affects[number][1]) },
			      "exact normalized physical affects");
		const auto descriptions =
			rows(connection,
			     "SELECT HEX(description.keyword),HEX(description.description) "
			     "FROM player_item_extra_descr description JOIN player_items item "
			     "ON item.id=description.item_id WHERE item.obj_uid=" +
				     uid);
		check(descriptions.size() == 1 &&
			      descriptions[0] ==
				      std::vector<std::string>{
					      hex(wanted.extra_descriptions[0].keyword),
					      hex(wanted.extra_descriptions[0].description) },
		      "exact normalized physical extra description");
		const uint64_t root = wanted.object_uid >= first_uid &&
						      wanted.object_uid <= first_uid + 3 ?
					      first_uid :
					      wanted.object_uid;
		const auto custody = rows(
			connection,
			"SELECT root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,"
			"item_revision,vnum,state,equipment_slot,HEX(coin_payload) FROM item_current_owner WHERE item_uid=" +
				uid);
		check(custody.size() == 1 &&
			      custody[0] ==
				      std::vector<std::string>{
					      std::to_string(root),
					      std::to_string(wanted.parent_index < 0 ?
								     0 :
								     expected[wanted.parent_index]
									     .object_uid),
					      "1", std::to_string(fixture_pid), "0", "1",
					      std::to_string(wanted.vnum), "1",
					      std::to_string(wanted.equipment_slot), "<NULL>" },
		      "exact original physical custody");
	}
	check(scalar(connection,
		     "SELECT COUNT(*) FROM item_current_owner WHERE owner_type=1 AND owner_id=" +
			     std::to_string(fixture_pid) + " AND state=1 AND item_revision=1") ==
		      expected.size(),
	      "checkpoint never mutates custody revisions");
}

std::vector<char> journal_bytes(const std::filesystem::path &directory)
{
	std::ifstream file(directory / "player-save.journal", std::ios::binary);
	check(file.good(), "native sealed journal exists");
	return { std::istreambuf_iterator<char>(file), {} };
}

void make_journal(const std::filesystem::path &directory, const std::vector<char> &bytes = {})
{
	check(std::filesystem::create_directory(directory), "fresh journal directory only");
	check(!chmod(directory.c_str(), 0700), "private journal permissions");
	if (!bytes.empty())
	{
		std::ofstream file(directory / "player-save.journal", std::ios::binary);
		file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
		check(file.good(), "preserve exact native unacknowledged journal frames");
		file.close();
		check(!chmod((directory / "player-save.journal").c_str(), 0600),
		      "private native journal file");
	}
}

// Direct registration/release API proof only: these synthetic commands are not
// submitted/applied/ACKed by a coordinator. Real observer/replay routing remains
// a separate qualification; no authority receipt is inferred from these calls.
void restored_capacity_checks(live_graph &graph, MYSQL *observer,
			      const std::vector<player_item_snapshot> &expected,
			      player_revision_t *final_revision)
{
	const auto items = captured_drop_graph(graph);
	const auto payload = restored_payload(items, fixture_pid + 1000);
	std::vector<critical_command> retained;
	for (size_t index = 0; index < PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS; ++index)
	{
		auto next = payload;
		next.from_owner.id += index;
		auto command = restored_command(next, 10000 + index);
		check(player_save_pipeline_restore_sql_drop_obligation(command),
		      "every bounded restored-scope slot is available");
		retained.push_back(std::move(command));
	}
	auto excess = payload;
	excess.from_owner.id += PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS;
	check(!player_save_pipeline_restore_sql_drop_obligation(restored_command(excess, 11000)) &&
		      player_save_pipeline_restore_sql_drop_obligation(retained.front()),
	      "slot capacity refuses new scope but admits exact idempotent duplicate");
	player_literal_inventory_token sentinel{ -1, 9, 9, 9 }, refused = sentinel;
	const auto before = revision();
	check(player_save_pipeline_literal_inventory_begin(&graph.actor, &graph.objects[0], 22800,
							   &refused) ==
			      player_literal_inventory_state::refused &&
		      refused == sentinel && same_revision(before, revision()),
	      "live scopes share restored slot capacity without marking or replacing output");
	player_save_pipeline_sql_drop_publication_acknowledged(retained.front().operation_id);
	player_literal_inventory_token token;
	check(player_save_pipeline_literal_inventory_begin(&graph.actor, &graph.objects[0], 22800,
							   &token) ==
		      player_literal_inventory_state::pending,
	      "released slot admits actual live checkpoint");
	await(
		[&]
		{
			return player_save_pipeline_literal_inventory_poll(token, &graph.actor) ==
			       player_literal_inventory_state::database_acknowledged;
		});
	*final_revision = revision().current_revision;
	verify_projection(observer, expected, *final_revision);
	check(player_save_pipeline_literal_inventory_cancel(token),
	      "cancel unheld live capacity scope");
	for (const auto &command : retained)
		player_save_pipeline_sql_drop_publication_acknowledged(command.operation_id);

	const auto padded = padded_drop_graph(items, ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES);
	const auto large = restored_payload(padded, fixture_pid + 2000);
	size_t used_bytes = 0;
	retained.clear();
	for (size_t index = 0; index < PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS; ++index)
	{
		auto next = large;
		next.from_owner.id += index;
		auto command = restored_command(next, 12000 + index);
		const auto bytes = encoded_command_bytes(command);
		if (!player_save_pipeline_restore_sql_drop_obligation(command))
		{
			check(used_bytes + bytes > PLAYER_SAVE_PIPELINE_MAX_BYTES &&
				      retained.size() < PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS - 1,
			      "encoded command byte capacity fails before exhausting slots");
			break;
		}
		used_bytes += bytes;
		retained.push_back(std::move(command));
	}
	check(!retained.empty() && retained.size() < PLAYER_SAVE_PIPELINE_MAX_SNAPSHOTS - 1,
	      "large commands establish byte rather than slot pressure");
	const auto small = restored_command(payload, 13000);
	const size_t available = PLAYER_SAVE_PIPELINE_MAX_BYTES - used_bytes;
	check(available > encoded_command_bytes(small), "remaining byte budget admits exact tail");
	const size_t tail_blob =
		encode(items).size() + available - 1 - encoded_command_bytes(small);
	const auto tail = restored_command(
		restored_payload(padded_drop_graph(items, tail_blob), fixture_pid + 3000), 13000);
	check(encoded_command_bytes(tail) == available - 1 &&
		      player_save_pipeline_restore_sql_drop_obligation(tail),
	      "fill exact byte budget leaving one byte and an unused slot");
	refused = sentinel;
	const auto byte_before = revision();
	check(player_save_pipeline_literal_inventory_begin(&graph.actor, &graph.objects[0], 22800,
							   &refused) ==
			      player_literal_inventory_state::refused &&
		      refused == sentinel && same_revision(byte_before, revision()),
	      "live literal capture shares restored byte budget without dirty marking");
	check(player_save_pipeline_restore_sql_drop_obligation(retained.front()),
	      "byte-saturated exact duplicate needs no extra storage");
	player_save_pipeline_sql_drop_publication_acknowledged(tail.operation_id);
	for (const auto &command : retained)
		player_save_pipeline_sql_drop_publication_acknowledged(command.operation_id);
	check(player_save_pipeline_save_admitted(fixture_pid + 2000) &&
		      player_save_pipeline_save_admitted(fixture_pid + 3000),
	      "all synthetic capacity obligations released independently");
}
} // namespace

// These leaves model the fixture's empty grant/receipt subsystems. Unexpected
// publication, pet, shape or custody-recapture calls fail; no core is replaced.
P_char character_list = nullptr;
index_data indexes[2]{};
P_index obj_index = indexes, mob_index = nullptr;
room_data rooms[1]{};
P_room world = rooms;
int top_of_objt = 1, top_of_mobt = -1;
extern const int top_of_world = 0;
Skill skills[MAX_SKILLS]{};
bool nevent_require_game_thread(const char *)
{
	check(std::this_thread::get_id() == game_thread, "game-thread identity access");
	return true;
}
bool training_dummy_capture_target_allowed(P_char)
{
	std::abort();
}
bool has_innate(P_char, int)
{
	std::abort();
}
P_char get_linked_char(P_char, ush_int)
{
	std::abort();
}
void logit(const char *, const char *, ...) {}
int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}
void panic_corruption(const char *, const char *, ...)
{
	std::abort();
}
bool item_movement_transaction_player_creation_busy(P_char actor)
{
	check(GET_PID(actor) == fixture_pid, "fixture grant owner");
	return false;
}
bool item_creation_grant_player_publication_pending(P_char actor)
{
	check(GET_PID(actor) == fixture_pid, "fixture grant publication owner");
	return false;
}
bool quest_reward_recovery_pending_save_receipts(
	int pid, std::vector<player_quest_xp_receipt_snapshot> *receipts,
	player_component_mask_t *components)
{
	check(pid == fixture_pid && receipts && receipts->empty() && components,
	      "empty quest subsystem");
	*components = 0;
	return true;
}
bool spell_component_retirement_pending_save_receipts(
	uint32_t pid, std::vector<player_spell_effect_receipt_snapshot> *receipts)
{
	check(pid == fixture_pid && receipts && receipts->empty(), "empty spell receipt subsystem");
	return true;
}
void quest_reward_recovery_save_acknowledged(int, uint64_t,
					     const player_quest_xp_receipt_snapshot *, size_t)
{
	std::abort();
}
void spell_component_retirement_save_completed(int32_t, bool,
					       const player_spell_effect_receipt_snapshot *, size_t)
{
	std::abort();
}
void persistence_schedule_character_save(P_char, int, int, const char *)
{
	std::abort();
}
void persistence_alert(int, const char *, const char *, const char *, const char *, const char *,
		       const char *, ...)
{
	std::abort();
}
MYSQL *sql_open_configured_connection(unsigned long flags)
{
	return connect_fixture(flags);
}
char *sql_escape_string(const char *value)
{
	check(escape_connection && std::this_thread::get_id() == game_thread,
	      "unexpected spellbook SQL escape");
	const size_t length = std::strlen(value);
	char *result = static_cast<char *>(std::malloc(length * 2 + 1));
	if (result)
		mysql_real_escape_string(escape_connection, result, value, length);
	return result;
}
#ifdef TEST_MUD
player_save_apply_result player_death_conflict_apply_from_pool(const player_snapshot &, void *)
{
	std::abort();
}
#endif

extern "C" int __real_mysql_real_query(MYSQL *, const char *, unsigned long);
extern "C" void *__real__Znwm(size_t);
extern "C" void *__wrap__Znwm(size_t bytes)
{
	if ((fail_next_game_allocation.load() || forbid_game_allocation.load()) &&
	    std::this_thread::get_id() == game_thread)
	{
		check(!forbid_game_allocation.load(), "publication release must not allocate");
		if (fail_next_game_allocation.exchange(false))
			throw std::bad_alloc();
	}
	return __real__Znwm(bytes);
}
extern "C" unsigned int __real_mysql_errno(MYSQL *);
extern "C" MYSQL *__real_sql_pool_acquire(void);
extern "C" void __real_sql_pool_release(MYSQL *);
extern "C" MYSQL *__real_sql_pool_replace_connection(MYSQL *);
extern "C" int __wrap_mysql_real_query(MYSQL *connection, const char *query, unsigned long length)
{
	const std::string_view text(query, length);
	if (std::this_thread::get_id() != game_thread && text == "START TRANSACTION")
	{
		std::unique_lock<std::mutex> lock(barrier_mutex);
		if (barriers_entered < barriers_requested)
		{
			const unsigned number = ++barriers_entered;
			barrier_changed.notify_all();
			check(barrier_changed.wait_for(lock, std::chrono::seconds(20),
						       [&] { return barriers_released >= number; }),
			      "bounded actual SQL apply barrier");
		}
	}
	const int result = __real_mysql_real_query(connection, query, length);
	if (!result && text == "COMMIT" && std::this_thread::get_id() != game_thread &&
	    hide_next_commit.exchange(false))
	{
		hidden_connection.store(connection);
		++hidden_commits;
		return 1; // The real server committed; only its reply is hidden.
	}
	return result;
}
extern "C" unsigned int __wrap_mysql_errno(MYSQL *connection)
{
	MYSQL *expected = connection;
	if (hidden_connection.compare_exchange_strong(expected, nullptr))
		return 2013;
	return __real_mysql_errno(connection);
}
extern "C" MYSQL *__wrap_sql_pool_acquire(void)
{
	auto *value = __real_sql_pool_acquire();
	if (value)
		++pool_acquisitions;
	return value;
}
extern "C" void __wrap_sql_pool_release(MYSQL *connection)
{
	if (connection)
		++pool_releases;
	__real_sql_pool_release(connection);
}
extern "C" MYSQL *__wrap_sql_pool_replace_connection(MYSQL *connection)
{
	const auto before = mysql_thread_id(connection);
	auto *replacement = __real_sql_pool_replace_connection(connection);
	check(replacement && mysql_thread_id(replacement) != before,
	      "real replacement has distinct server session");
	++replacements;
	return replacement;
}

int main(int argc, char **argv)
{
	guard_target();
	check(argc == 2 && std::filesystem::path(argv[1]).is_absolute(),
	      "private absolute journal root");
	check(mysql_library_init(0, nullptr, nullptr) == 0, "native client library init");
	MYSQL *observer = connect_fixture(0);
	escape_connection = observer;
	check(!scalar(observer,
		      "SELECT COUNT(*) FROM economic_lineage_state WHERE active_epoch IS NOT NULL") &&
		      !scalar(observer, "SELECT COUNT(*) FROM economic_sql_global_activation"),
	      "inactive fixture accounting");
	check(!scalar(observer,
		      "SELECT COUNT(*) FROM accounts WHERE account_name='LiteralCheckpointFixture'") &&
		      !scalar(observer, "SELECT COUNT(*) FROM player_data WHERE pid=" +
						std::to_string(fixture_pid)) &&
		      !scalar(observer,
			      "SELECT COUNT(*) FROM item_current_owner WHERE item_uid BETWEEN 930100 AND 930105"),
	      "fresh synthetic identity collision refusal");
	execute(observer, "INSERT INTO accounts(account_name) VALUES('LiteralCheckpointFixture')");
	execute(observer,
		"INSERT INTO player_data(pid,name,account_name,save_revision,copper,silver,gold,platinum) VALUES(" +
			std::to_string(fixture_pid) +
			",'LiteralProbe','LiteralCheckpointFixture',0,0,0,0,0)");
	indexes[0].virtual_number = 48;
	indexes[1].virtual_number = 5;
	rooms[0].number = 22800;
	{
		live_graph graph;
		character_list = &graph.actor;
		auto expected = graph.expected();
		const item_owner_identity owner{ item_owner_type::player, fixture_pid, 0 };
		execute(observer,
			"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES(1," +
				std::to_string(fixture_pid) + ",0,1)");
		for (const auto &item : expected)
		{
			const uint64_t parent =
				item.parent_index < 0 ? 0 : expected[item.parent_index].object_uid;
			const uint64_t root = item.object_uid >= first_uid &&
							      item.object_uid <= first_uid + 3 ?
						      first_uid :
						      item.object_uid;
			execute(observer,
				"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,owner_id,"
				"owner_context_id,item_revision,vnum,state,equipment_slot) VALUES(" +
					std::to_string(item.object_uid) + "," +
					std::to_string(root) + "," +
					(parent ? std::to_string(parent) : "NULL") + ",1," +
					std::to_string(fixture_pid) + ",0,1," +
					std::to_string(item.vnum) + ",1," +
					std::to_string(item.equipment_slot) + ")");
			check(item_ownership_runtime_hydrate({ item.object_uid, root, parent, owner,
							       1, 1, item.vnum,
							       item_custody_state::active }),
			      "actual custody runtime hydration");
		}
		check(player_revision_hydrate(fixture_pid, 0), "native baseline revision");
		check(sql_pool_init(2) == 0, "actual pool initialized");
		const std::filesystem::path directory(argv[1]);
		const auto active = directory / "active";
		make_journal(active);
		check(player_save_pipeline_init(active.c_str()), "actual pipeline init");
		await([] { return player_save_pipeline_loads_allowed(); });
		{
			std::lock_guard<std::mutex> lock(barrier_mutex);
			barriers_requested = 2;
		}
		player_literal_inventory_token token;
		check(player_save_pipeline_literal_inventory_begin(&graph.actor, &graph.objects[0],
								   22800, &token) ==
			      player_literal_inventory_state::pending,
		      "literal begin pending");
		await_barrier(1);
		const auto first_revision = revision().current_revision;
		check(player_save_journal_health_copy().records > 0 &&
			      player_save_pipeline_literal_inventory_poll(token, &graph.actor) ==
				      player_literal_inventory_state::pending &&
			      scalar(observer, "SELECT save_revision FROM player_data WHERE pid=" +
						       std::to_string(fixture_pid)) == 0,
		      "journal-only durability cannot acknowledge literal scope");
		graph.objects[4].value[1] += 25;
		check(player_save_pipeline_request(&graph.actor, PLAYER_COMPONENT_LANGUAGES,
						   RENT_CRASH,
						   22800) == player_save_pipeline_result::queued,
		      "new ordinary checkpoint keeps literal capture");
		await([] { return player_save_worker_health_copy().coalesced >= 1; });
		check(player_save_pipeline_request(&graph.actor, PLAYER_COMPONENT_TIMERS,
						   RENT_CRASH,
						   22800) == player_save_pipeline_result::queued,
		      "newest ordinary capture coalesces");
		await([] { return player_save_worker_health_copy().coalesced >= 2; });
		const auto newest_revision = revision().current_revision;
		check(newest_revision > first_revision, "newer native revision");
		expected = graph.expected();
		const auto retained_frames = journal_bytes(active);
		check(!retained_frames.empty(), "retain actual native frames for cold replay");
		release_barrier(1);
		await_barrier(2);
		await([&] { return revision().acknowledged_revision == first_revision; });
		check(player_save_pipeline_literal_inventory_poll(token, &graph.actor) ==
			      player_literal_inventory_state::pending,
		      "older exact ACK cannot acknowledge newer coalesced literal capture");
		const auto original_strings = graph.strings[0];
		graph.strings[0].fill(std::string(300, 'z'));
		graph.assign_strings(0);
		hide_next_commit.store(true);
		release_barrier(2);
		await([&] { return revision().acknowledged_revision == newest_revision; });
		verify_projection(observer, expected, newest_revision);
		// The worker owns immutable original bytes even when the live string
		// storage is freed/reallocated. Restore the intent before polling; a
		// changed-intent poll cancels an unheld lease and is tested below.
		graph.strings[0] = original_strings;
		graph.assign_strings(0);
		check(player_save_pipeline_literal_inventory_poll(token, &graph.actor) ==
			      player_literal_inventory_state::database_acknowledged,
		      "restored intent matches exact newest DB completion");
		check(hidden_commits == 1 && replacements == 1,
		      "real COMMIT reply loss reconciled via actual pool replacement");
		// A clean inventory ACK cannot let a subsequent ordinary capture discard
		// the selected literal policy or treat an unrelated ACK as its receipt.
		check(player_save_pipeline_request(&graph.actor, PLAYER_COMPONENT_LANGUAGES,
						   RENT_CRASH,
						   22800) == player_save_pipeline_result::queued,
		      "ordinary checkpoint after clean literal ACK");
		auto final_revision = revision().current_revision;
		check(final_revision > newest_revision &&
			      player_save_pipeline_literal_inventory_poll(token, &graph.actor) ==
				      player_literal_inventory_state::pending,
		      "clean ACK invalidated by new ordinary checkpoint");
		await(
			[&]
			{
				return player_save_pipeline_literal_inventory_poll(token,
										   &graph.actor) ==
				       player_literal_inventory_state::database_acknowledged;
			});
		verify_projection(observer, expected, final_revision);
		auto wrong = token;
		++wrong.generation;
		check(player_save_pipeline_literal_inventory_poll(wrong, &graph.actor) ==
			      player_literal_inventory_state::refused,
		      "wrong token generation");
		wrong = token;
		++wrong.actor_runtime_id;
		check(player_save_pipeline_literal_inventory_poll(wrong, &graph.actor) ==
			      player_literal_inventory_state::refused,
		      "wrong runtime actor identity");
		critical_operation_id operation{}, other{};
		operation.bytes[0] = 0x45;
		other.bytes[0] = 0x46;
		check(!player_save_pipeline_literal_inventory_hold(token, {}),
		      "empty operation cannot hold");
		check(player_save_pipeline_literal_inventory_hold(token, operation),
		      "exact clean token hold");
		check(!player_save_pipeline_literal_inventory_cancel(token) &&
			      !player_save_pipeline_literal_inventory_release(token, other),
		      "held scope original operation ownership");
		const auto held_revision = revision();
		const auto held_health = player_save_pipeline_health_copy();
		const auto held_journal = player_save_journal_health_copy();
		check(!player_save_pipeline_save_admitted(fixture_pid) &&
			      !player_save_pipeline_authoritative_hydration_admitted(fixture_pid) &&
			      player_save_pipeline_sealed_save_pending(fixture_pid) &&
			      player_save_pipeline_target_save_pending(fixture_pid) &&
			      !player_save_pipeline_acquire_target_save_login_fence(
				      fixture_pid, final_revision) &&
			      !player_save_pipeline_target_save_login_fenced(fixture_pid),
		      "held lease rejects save and login admission without allocating a fence");
		player_save_pipeline_diagnostic held_diagnostic;
		await(
			[&]
			{
				held_diagnostic = player_save_pipeline_diagnostic_copy(fixture_pid);
				return held_diagnostic.available;
			});
		check(!held_diagnostic.pid_admission_open && held_diagnostic.retained_save,
		      "held publication obligation remains visible in diagnostics");
		check(player_save_pipeline_terminal(&graph.actor, RENT_INN, 22800, 1, false) ==
				      player_save_terminal_result::unavailable &&
			      player_save_pipeline_terminal_death(
				      &graph.actor, &graph.objects[0], nullptr, other, 22800, 1,
				      false) == player_save_terminal_result::unavailable &&
			      player_save_pipeline_terminal_death_resume(&graph.actor, 0, 1) ==
				      player_save_terminal_result::not_pending,
		      "held lease refuses terminal and death before capture or pinning");
		const auto after_terminal = player_save_pipeline_health_copy();
		check(same_revision(held_revision, revision()) &&
			      after_terminal.captured == held_health.captured &&
			      after_terminal.marked == held_health.marked &&
			      after_terminal.pending_append == held_health.pending_append &&
			      after_terminal.durable_ready == held_health.durable_ready &&
			      after_terminal.terminal_fences == held_health.terminal_fences &&
			      after_terminal.terminal_death_requeues ==
				      held_health.terminal_death_requeues &&
			      player_save_journal_health_copy().appended == held_journal.appended &&
			      !player_save_worker_pid_pending(fixture_pid),
		      "rejected terminal/login intents do not mark, pin, append, or queue a save");
		constexpr int unrelated_pid = fixture_pid + 1;
		check(player_save_pipeline_save_admitted(unrelated_pid) &&
			      !player_save_pipeline_target_save_pending(unrelated_pid) &&
			      !player_save_pipeline_sealed_save_pending(unrelated_pid) &&
			      player_save_pipeline_acquire_target_save_login_fence(unrelated_pid,
										   0),
		      "held lease fences only its original PID");
		player_save_pipeline_release_target_save_login_fence(unrelated_pid, 0);
		check(!player_save_pipeline_target_save_login_fenced(unrelated_pid),
		      "unrelated PID login fence released independently");
		check(player_save_pipeline_mark(fixture_pid, PLAYER_COMPONENT_INVENTORY) &&
			      player_save_pipeline_checkpoint_dirty(&graph.actor, RENT_CRASH,
								    22800) ==
				      player_save_pipeline_result::unavailable,
		      "held inventory blocks capture while preserving dirty mark");
		check(player_save_pipeline_literal_inventory_release(token, operation),
		      "original operation releases hold");
		check(!player_save_pipeline_literal_inventory_cancel(token),
		      "released operation token cannot cancel a later scope");
		{
			std::lock_guard<std::mutex> lock(barrier_mutex);
			barriers_requested = 4;
		}
		player_literal_inventory_token old_lifetime;
		check(player_save_pipeline_literal_inventory_begin(&graph.actor, &graph.objects[0],
								   22800, &old_lifetime) ==
			      player_literal_inventory_state::pending,
		      "new unheld lease begins before runtime retirement");
		await_barrier(3);
		const auto retired_revision = revision().current_revision;
		const auto before_retirement = revision();
		const auto before_retirement_health = player_save_pipeline_health_copy();
		unregister_character_runtime_id(&graph.actor);
		check(!find_character_by_runtime_id(old_lifetime.actor_runtime_id),
		      "authoritative runtime identity retired");
		player_literal_inventory_token unregistered_token;
		check(graph.actor.runtime_id == old_lifetime.actor_runtime_id &&
			      player_save_pipeline_literal_inventory_begin(&graph.actor,
									   &graph.objects[0], 22800,
									   &unregistered_token) ==
				      player_literal_inventory_state::refused &&
			      !unregistered_token.pid &&
			      player_save_pipeline_literal_inventory_poll(old_lifetime,
									  &graph.actor) ==
				      player_literal_inventory_state::refused &&
			      !player_save_pipeline_literal_inventory_hold(old_lifetime,
									   operation) &&
			      same_revision(before_retirement, revision()) &&
			      player_save_pipeline_health_copy().captured ==
				      before_retirement_health.captured,
		      "field-identical unregistered actor refuses immediately before any pulse");
		graph.actor.runtime_id = allocate_character_runtime_id();
		register_character_runtime_id(&graph.actor);
		player_save_pipeline_pulse();
		player_literal_inventory_token new_lifetime;
		check(player_save_pipeline_literal_inventory_begin(&graph.actor, &graph.objects[0],
								   22800, &new_lifetime) ==
				      player_literal_inventory_state::pending &&
			      new_lifetime.generation != old_lifetime.generation &&
			      new_lifetime.actor_runtime_id != old_lifetime.actor_runtime_id,
		      "new runtime lifetime can replace only an unheld retired lease");
		final_revision = revision().current_revision;
		check(final_revision > retired_revision &&
			      player_save_pipeline_literal_inventory_poll(old_lifetime,
									  &graph.actor) ==
				      player_literal_inventory_state::refused &&
			      !player_save_pipeline_literal_inventory_hold(old_lifetime,
									   operation) &&
			      !player_save_pipeline_literal_inventory_cancel(old_lifetime),
		      "retired token cannot affect replacement lease");
		release_barrier(3);
		await_barrier(4);
		await([&] { return revision().acknowledged_revision == retired_revision; });
		check(player_save_pipeline_literal_inventory_poll(new_lifetime, &graph.actor) ==
			      player_literal_inventory_state::pending,
		      "late old-runtime completion cannot acknowledge replacement scope");
		release_barrier(4);
		await(
			[&]
			{
				return player_save_pipeline_literal_inventory_poll(new_lifetime,
										   &graph.actor) ==
				       player_literal_inventory_state::database_acknowledged;
			});
		verify_projection(observer, expected, final_revision);
		graph.strings[0].fill(std::string(300, 'x'));
		graph.assign_strings(0);
		check(player_save_pipeline_literal_inventory_poll(new_lifetime, &graph.actor) ==
			      player_literal_inventory_state::refused,
		      "changed live literal intent refuses and cancels an unheld scope");
		graph.strings[0] = original_strings;
		graph.assign_strings(0);
		check(player_save_pipeline_literal_inventory_poll(new_lifetime, &graph.actor) ==
				      player_literal_inventory_state::refused &&
			      !player_save_pipeline_literal_inventory_hold(new_lifetime,
									   operation) &&
			      !player_save_pipeline_literal_inventory_cancel(new_lifetime),
		      "restoring bytes cannot revive a cancelled scope token");
		await([] { return player_save_journal_health_copy().records == 0; });
		check(!player_save_worker_pid_pending(fixture_pid),
		      "all exact native completions finished before shutdown");
		player_save_pipeline_shutdown();
		check(!sql_pool_in_use(), "pipeline shutdown releases all actual pool leases");
		check(player_save_journal_health_copy().records == 0,
		      "exact native journal ACK retired frames");
		// Replay byte-for-byte ORIGINAL pending journal files into independently
		// reopened native pipelines. SQL is already durable, so stale/duplicate
		// records must retire without rewriting any payload or custody state.
		for (unsigned boot : { 1U, 2U })
		{
			mysql_close(observer);
			observer = connect_fixture(0);
			escape_connection = observer;
			verify_projection(observer, expected, final_revision);
			player_save_pipeline_reset_for_tests();
			const auto replay = directory / ("replay-" + std::to_string(boot));
			make_journal(replay, retained_frames);
			const auto drop_items = captured_drop_graph(graph);
			const auto drop = restored_payload(drop_items, fixture_pid);
			sql_room_item_payload_batch pure_capture;
			check(sql_room_item_payload_capture(drop, &pure_capture) &&
				      encode(pure_capture.items) == encode(drop_items),
			      "actual pure SQL room capture preserves canonical complete graph");
			const auto command = restored_command(drop, 20000);
			check(!player_save_pipeline_restore_sql_drop_obligation(command),
			      "uninitialized pipeline cannot restore a scope");
			unsigned replay_barrier;
			{
				std::lock_guard<std::mutex> lock(barrier_mutex);
				replay_barrier = barriers_requested = barriers_entered + 1;
			}
			check(player_save_pipeline_init(replay.c_str()),
			      "native journal cold replay init");
			await_barrier(replay_barrier);
			check(!player_save_pipeline_loads_allowed() &&
				      !player_save_pipeline_health_copy().replay_complete &&
				      player_save_pipeline_restore_sql_drop_obligation(command),
			      "restored obligation registers during real blocked save-journal replay");
			player_save_pipeline_quiesce();
			auto quiesced = drop;
			quiesced.from_owner.id = fixture_pid + 20;
			const auto quiesced_command = restored_command(quiesced, 20001);
			check(!player_save_pipeline_health_copy().accepting &&
				      player_save_pipeline_restore_sql_drop_obligation(command) &&
				      player_save_pipeline_restore_sql_drop_obligation(
					      quiesced_command),
			      "initialized quiesced pipeline can restore duplicate and new obligations");
			player_save_pipeline_sql_drop_publication_acknowledged(
				quiesced_command.operation_id);
			player_save_pipeline_resume();
			auto allocation_drop = drop;
			allocation_drop.from_owner.id = fixture_pid + 21;
			const auto allocation_command = restored_command(allocation_drop, 20008);
			fail_next_game_allocation = true;
			check(!player_save_pipeline_restore_sql_drop_obligation(
				      allocation_command) &&
				      !fail_next_game_allocation.load() &&
				      player_save_pipeline_save_admitted(fixture_pid + 21) &&
				      !player_save_pipeline_save_admitted(fixture_pid),
			      "actual allocation failure refuses without partial scope or prior-hold mutation");
			check(player_save_pipeline_restore_sql_drop_obligation(allocation_command),
			      "retry same typed command after allocation failure");
			player_save_pipeline_sql_drop_publication_acknowledged(
				allocation_command.operation_id);
			auto changed = command;
			++changed.accepted_at_usec;
			check(item_transfer_accounting_command_supported(changed) &&
				      !player_save_pipeline_restore_sql_drop_obligation(changed) &&
				      !player_save_pipeline_restore_sql_drop_obligation(
					      restored_command(drop, 20002)) &&
				      !player_save_pipeline_restore_sql_drop_obligation(
					      restored_command(quiesced, 20000)),
			      "same PID/operation refuses changed original bytes, changed ID, and changed PID");
			auto changed_items = drop_items;
			changed_items[0].description += "changed intent";
			check(!player_save_pipeline_restore_sql_drop_obligation(restored_command(
				      restored_payload(changed_items, fixture_pid), 20000)),
			      "same original ID refuses changed canonical command payload");
			auto unsupported = drop;
			unsupported.reason = item_transfer_reason::player_give;
			unsupported.to_owner = { item_owner_type::player, fixture_pid + 20, 0 };
			check(!player_save_pipeline_restore_sql_drop_obligation(
				      restored_command(unsupported, 20003)),
			      "supported typed player give is outside pure SQL room drop shape");
			changed_items = drop_items;
			changed_items[0].string_mask &= ~STRUNG_DESC3;
			changed_items[0].action_description.clear();
			check(!player_save_pipeline_restore_sql_drop_obligation(restored_command(
				      restored_payload(changed_items, fixture_pid), 20004)),
			      "prototype-dependent partial text cannot register a restored room scope");
			for (unsigned invalid = 0; invalid < 3; ++invalid)
			{
				changed = command;
				if (!invalid)
					changed.publication_required = false;
				else if (invalid == 1)
					changed.operation_id = {};
				else
				{
					changed.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
					changed.accounting_intent.clear();
				}
				check(!player_save_pipeline_restore_sql_drop_obligation(changed),
				      "publication/envelope/typed schema guards refuse invalid commands");
			}
			check(player_revision_hydrate(fixture_pid, final_revision),
			      "cold replay revision baseline");
			release_barrier(replay_barrier);
			await([] { return player_save_pipeline_loads_allowed(); });
			constexpr int fenced_pid = fixture_pid + 30;
			check(player_save_pipeline_acquire_target_save_login_fence(fenced_pid, 0),
			      "actual target-login fence acquisition");
			auto fenced_drop = drop;
			fenced_drop.from_owner.id = fenced_pid;
			const auto fenced_command = restored_command(fenced_drop, 20006);
			check(player_save_pipeline_restore_sql_drop_obligation(fenced_command) &&
				      !player_save_pipeline_authoritative_hydration_admitted(
					      fenced_pid),
			      "restored scope cannot bypass actual target-login fence");
			player_save_pipeline_release_target_save_login_fence(fenced_pid, 0);
			check(player_save_pipeline_authoritative_hydration_admitted(fenced_pid) &&
				      !player_save_pipeline_save_admitted(fenced_pid),
			      "target-fence release leaves restored save hold in place");
			player_save_pipeline_sql_drop_publication_acknowledged(
				fenced_command.operation_id);
			check(!player_save_pipeline_save_admitted(fixture_pid) &&
				      player_save_pipeline_authoritative_hydration_admitted(
					      fixture_pid) &&
				      player_save_pipeline_sealed_save_pending(fixture_pid) &&
				      player_save_pipeline_target_save_pending(fixture_pid) &&
				      !player_save_pipeline_acquire_target_save_login_fence(
					      fixture_pid, final_revision),
			      "restored held scope allows only authoritative hydration after replay");
			const auto restore_before = revision();
			check(player_save_pipeline_terminal(&graph.actor, RENT_INN, 22800, 1,
							    false) ==
					      player_save_terminal_result::unavailable &&
				      player_save_pipeline_terminal_death(
					      &graph.actor, &graph.objects[0], nullptr,
					      restored_operation(20005), 22800, 1,
					      false) == player_save_terminal_result::unavailable &&
				      same_revision(restore_before, revision()),
			      "restored hold refuses terminal/death before dirty mark or capture");
			check(player_save_pipeline_mark(fixture_pid, PLAYER_COMPONENT_INVENTORY) &&
				      player_save_pipeline_checkpoint_dirty(&graph.actor,
									    RENT_CRASH, 22800) ==
					      player_save_pipeline_result::unavailable,
			      "restored scope preserves dirty marks while rejecting capture");
			const player_literal_inventory_token restored_token{ fixture_pid, 0,
									     first_uid, 0 };
			check(!player_save_pipeline_literal_inventory_release(
				      restored_token, command.operation_id) &&
				      !player_save_pipeline_literal_inventory_cancel(
					      restored_token),
			      "live token APIs cannot remove restored held scope");
			unregister_character_runtime_id(&graph.actor);
			character_list = nullptr;
			player_save_pipeline_pulse();
			check(!player_save_pipeline_save_admitted(fixture_pid) &&
				      player_save_pipeline_authoritative_hydration_admitted(
					      fixture_pid),
			      "runtime-zero restored hold survives pulse without a registered runtime actor");
			forbid_game_allocation = true;
			player_save_pipeline_sql_drop_publication_acknowledged(
				restored_operation(20099));
			forbid_game_allocation = false;
			check(!player_save_pipeline_save_admitted(fixture_pid),
			      "wrong original ID cannot release scope");
			forbid_game_allocation = true;
			player_save_pipeline_sql_drop_publication_acknowledged(
				command.operation_id);
			forbid_game_allocation = false;
			check(player_save_pipeline_save_admitted(fixture_pid),
			      "exact original ID releases scope without actor");
			player_save_pipeline_sql_drop_publication_acknowledged(
				command.operation_id);
			register_character_runtime_id(&graph.actor);
			character_list = &graph.actor;
			player_literal_inventory_token released_token;
			check(player_save_pipeline_literal_inventory_begin(
				      &graph.actor, &graph.objects[0], 22800, &released_token) ==
				      player_literal_inventory_state::pending,
			      "actual save checkpoint resumes after original-ID release");
			await(
				[&]
				{
					return player_save_pipeline_literal_inventory_poll(
						       released_token, &graph.actor) ==
					       player_literal_inventory_state::database_acknowledged;
				});
			final_revision = revision().current_revision;
			check(player_save_pipeline_literal_inventory_cancel(released_token),
			      "released-scope save completes cleanly");
			const auto replay_health = player_save_journal_health_copy();
			check(replay_health.records == 0 &&
				      replay_health.replayed + replay_health.duplicates > 0 &&
				      !replay_health.corrupt_records &&
				      !replay_health.unsupported_records &&
				      !replay_health.quarantined_bytes &&
				      !player_save_journal_pid_quarantined(fixture_pid),
			      "real stale/duplicate frames consumed without corruption or quarantine");
			check(player_save_pipeline_literal_inventory_poll(token, &graph.actor) ==
				      player_literal_inventory_state::refused,
			      "shutdown token cannot acquire a new pipeline lifetime");
			verify_projection(observer, expected, final_revision);
			player_save_pipeline_shutdown();
		}
		player_save_pipeline_reset_for_tests();
		check(player_revision_hydrate(fixture_pid, final_revision),
		      "capacity fixture real durable revision");
		const auto capacity = directory / "restored-capacity";
		make_journal(capacity);
		check(player_save_pipeline_init(capacity.c_str()), "actual capacity pipeline init");
		await([] { return player_save_pipeline_loads_allowed(); });
		restored_capacity_checks(graph, observer, expected, &final_revision);
		constexpr int quarantined_pid = fixture_pid + 40;
		player_snapshot quarantined;
		graph.pc.pid = quarantined_pid;
		check(player_snapshot_capture_literal_inventory(
			      &graph.actor, 1, item_components, RENT_CRASH, 22800, first_uid,
			      &quarantined) == player_snapshot_capture_result::ok,
		      "actual capture for private journal fault");
		graph.pc.pid = fixture_pid;
		check(player_save_journal_append(quarantined) == player_save_journal_result::ok,
		      "real private pending journal frame");
		player_save_journal_worker_terminal(quarantined, nullptr);
		check(player_save_journal_pid_quarantined(quarantined_pid),
		      "actual terminal-failure journal hook establishes durable PID quarantine");
		const auto quarantined_command = restored_command(
			restored_payload(captured_drop_graph(graph), quarantined_pid), 20007);
		check(player_save_pipeline_restore_sql_drop_obligation(quarantined_command) &&
			      !player_save_pipeline_authoritative_hydration_admitted(
				      quarantined_pid),
		      "valid restored obligation cannot bypass actual journal quarantine");
		player_save_pipeline_sql_drop_publication_acknowledged(
			quarantined_command.operation_id);
		check(!player_save_pipeline_save_admitted(quarantined_pid) &&
			      !player_save_pipeline_authoritative_hydration_admitted(
				      quarantined_pid),
		      "original-ID release cannot erase independent durable quarantine");
		await([] { return player_save_journal_health_copy().records == 0; });
		player_save_pipeline_shutdown();
		check(!sql_pool_in_use(),
		      "zero native pool leases after two cold component replays");
		auto *clean = sql_pool_acquire();
		check(clean && scalar(clean, "SELECT @@autocommit") == 1,
		      "pool reborrow clean autocommit");
		sql_pool_release(clean);
		sql_pool_shutdown();
		check(sql_pool_total() == 0 && sql_pool_in_use() == 0,
		      "pool shutdown before library end");
		check(pool_acquisitions > 0 && pool_releases > 0,
		      "observed actual production pool work");
		character_list = nullptr;
		item_ownership_runtime_reset();
	}
	mysql_close(observer);
	escape_connection = nullptr;
	mysql_library_end();
	std::cout
		<< "PASS: actual native literal capture/pipeline/journal/worker/repository/pool; journal-only refusal; "
		   "newer coalescing and exact DB ACK; complete SQL payload/custody; real COMMIT reply loss; "
		   "operation hold ownership; two independent cold component reads and native journal replays; "
		   "manual typed restored-scope replay fences, conflicts, release and shared capacities; inactive fixture only\n";
}
