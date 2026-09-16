#include "core/defines.h"
#include "core/prototypes.h"
#include "core/structs.h"
#include "item/item_ownership_runtime.h"
#include "persistence/persistence_observability.h"
#include "persistence/player_death_restitution_command.h"
#include "player/player_load_items.h"
#include "player/player_load_repository.h"

#include <mysql/mysql.h>
#include <openssl/sha.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

Skill skills[MAX_SKILLS] = {};

int real_object(const int vnum)
{
	return vnum > 0 ? 0 : -1;
}

P_obj read_object(int, int)
{
	P_obj object = new obj_data{};
	object->R_num = 0;
	object->type = ITEM_CONTAINER;
	return object;
}

bool obj_can_nest(P_obj, P_obj)
{
	return true;
}

void recalc_container_weight(P_obj) {}

void set_obj_affected(P_obj object, int, sh_int type, sh_int data)
{
	if (!object)
		return;
	obj_affect *affect = new obj_affect{};
	affect->type = type;
	affect->data = data;
	affect->next = object->affects;
	object->affects = affect;
}

void set_obj_affected_extra(P_obj object, int, sh_int type, sh_int data, ulong extra2)
{
	set_obj_affected(object, -1, type, data);
	if (object && object->affects)
		object->affects->extra2 = extra2;
}

void balance_affects(P_char) {}

char *str_dup(const char *text)
{
	if (!text)
		return nullptr;
	const size_t size = std::strlen(text) + 1;
	char *copy = static_cast<char *>(std::malloc(size));
	if (copy)
		std::memcpy(copy, text, size);
	return copy;
}

void *__malloc(size_t size, const char *, const char *, int)
{
	return std::calloc(1, size);
}

void *__realloc(void *pointer, size_t size, const char *, int)
{
	return std::realloc(pointer, size);
}

void __free(void *pointer, const char *, int)
{
	std::free(pointer);
}

[[noreturn]] int panic_corruption_int(const char *, const char *, ...)
{
	std::abort();
}

bool item_owner_identity_valid(const item_owner_identity &owner)
{
	return owner.type > item_owner_type::unknown && owner.type <= item_owner_type::shopkeeper &&
	       ((owner.type == item_owner_type::system || owner.type == item_owner_type::destruction) ?
			owner.id == 0 && owner.context_id == 0 : owner.id != 0);
}

bool item_owner_identity_equal(const item_owner_identity &left, const item_owner_identity &right)
{
	return left.type == right.type && left.id == right.id && left.context_id == right.context_id;
}

bool item_ownership_runtime_hydrate_batch(const item_ownership_runtime_entry *, size_t)
{
	return true;
}

bool item_ownership_runtime_hydrate_owner(const item_owner_identity &, uint64_t)
{
	return true;
}

void extract_obj(P_obj object, int)
{
	if (!object)
		return;
	P_obj child = object->contains;
	while (child)
	{
		P_obj next = child->next_content;
		child->next_content = nullptr;
		extract_obj(child, 0);
		child = next;
	}
	while (object->affects)
	{
		obj_affect *next = object->affects->next;
		delete object->affects;
		object->affects = next;
	}
	while (object->ex_description)
	{
		extra_descr_data *next = object->ex_description->next;
		std::free(object->ex_description->keyword);
		std::free(object->ex_description->description);
		std::free(object->ex_description);
		object->ex_description = next;
	}
	std::free(object->name);
	std::free(object->short_description);
	std::free(object->description);
	std::free(object->action_description);
	delete object;
}

namespace
{
constexpr int32_t PID = 4701;
constexpr uint64_t ITEM_UID = 770001;
constexpr uint32_t ITEM_ID = 770001;
constexpr int32_t ITEM_VNUM = 19408;
const std::string ACCOUNT = "spellbook-fidelity-account";
const std::string RECEIPT_ID = "00112233445566778899aabbccddeeff";
const std::vector<int32_t> CAPTURED_SPELLS = { 1, 7, 31 };

[[noreturn]] void fail(const std::string &message, MYSQL *connection = nullptr)
{
	if (connection && mysql_errno(connection))
	{
		std::cerr << message << ": " << mysql_error(connection) << '\n';
		std::abort();
	}
	std::cerr << message << '\n';
	std::abort();
}

void require(bool condition, const std::string &message)
{
	if (!condition)
		fail(message);
}

void execute_sql(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()) != 0)
		fail("fixture SQL failed", connection);
}

std::string scalar(MYSQL *connection, const std::string &sql)
{
	execute_sql(connection, sql);
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		fail("scalar query returned no result", connection);
	MYSQL_ROW row = mysql_fetch_row(rows);
	if (!row || !row[0])
	{
		mysql_free_result(rows);
		fail("scalar query returned no row");
	}
	const std::string value = row[0];
	mysql_free_result(rows);
	return value;
}

std::string hex_encode(const std::vector<uint8_t> &bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result;
	result.reserve(bytes.size() * 2);
	for (uint8_t byte : bytes)
	{
		result.push_back(digits[byte >> 4]);
		result.push_back(digits[byte & 0x0f]);
	}
	return result;
}

std::string sha256_hex(const std::vector<uint8_t> &bytes)
{
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	return hex_encode(std::vector<uint8_t>(digest.begin(), digest.end()));
}

std::vector<uint8_t> captured_spell_bitmap()
{
	constexpr size_t bitmap_bytes = (MAX_SKILLS + 1) / 8 + 1;
	std::vector<uint8_t> bitmap(bitmap_bytes, 0);
	for (int32_t spell : CAPTURED_SPELLS)
	{
		require(spell >= 0 && spell < MAX_SKILLS, "captured spell is outside the skill table");
		bitmap[static_cast<size_t>(spell) / 8] = static_cast<uint8_t>(
			bitmap[static_cast<size_t>(spell) / 8] | static_cast<uint8_t>(1U << (spell % 8)));
	}
	return bitmap;
}

player_death_restitution_item_state make_state(const std::vector<uint8_t> &keyword,
							       const std::vector<uint8_t> &description)
{
	player_death_restitution_item_state state = {};
	state.item_uid = ITEM_UID;
	state.vnum = ITEM_VNUM;
	state.equip_slot = 9;
	state.quantity = 1;
	state.weight = 23;
	state.cost = 4567;
	state.timer = 1234;
	state.extra_flags = 0x12345678U;
	state.wear_flags = 77;
	state.item_type = 20;
	state.values = { 11, 22, 33, 44, 55, 66, 77, 88 };
	state.string_present = { true, true, true, true };
	state.strings[0] = { 'c', 'a', 'p', 't', 'u', 'r', 'e', 'd' };
	state.strings[1] = { 's', 'p', 'e', 'l', 'l', 'b', 'o', 'o', 'k' };
	state.strings[2] = { 'a', 'u', 't', 'h', 'o', 'r', 'i', 't', 'a', 't', 'i', 'v', 'e' };
	state.strings[3] = { 'd', 'e', 'a', 't', 'h' };
	state.bitvector_present = { true, true, true, true, true };
	state.bitvectors = { 0x11, 0x22, 0x33, 0x44, 0x55 };
	state.material = 4;
	state.condition = 97;
	state.affects.push_back({ 7, -3 });
	state.extra_descriptions.push_back({ { 'l', 'o', 'r', 'e' }, { 'c', 'a', 'p', 't', 'u', 'r', 'e', 'd' } });
	state.extra_descriptions.push_back({ keyword, description });
	return state;
}

std::vector<uint8_t> encode_state(const std::vector<uint8_t> &keyword,
						  const std::vector<uint8_t> &description)
{
	std::vector<uint8_t> encoded;
	const player_death_restitution_item_state state = make_state(keyword, description);
	require(player_death_restitution_item_state_encode(state, &encoded),
		"authoritative IST1 state did not encode");
	return encoded;
}

void seed_fixture(MYSQL *connection, const std::vector<uint8_t> &payload)
{
	const std::string payload_hex = hex_encode(payload);
	const std::string digest_hex = sha256_hex(payload);
	execute_sql(connection,
		"INSERT INTO accounts(account_name,password) VALUES('" + ACCOUNT + "','')");
	execute_sql(connection,
		"INSERT INTO player_data(pid,name,account_name,save_revision,active) VALUES(" +
			std::to_string(PID) + ",'spellbook-fidelity','" + ACCOUNT + "',1,1)");
	execute_sql(connection,
		"INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) VALUES"
		"(1," + std::to_string(PID) + ",0,7)");
	execute_sql(connection,
		"INSERT INTO player_items(id,pid,vnum,equip_slot,container_id,quantity,weight,cost,timer,"
		"extra_flags,wear_flags,item_type,value0,value1,value2,value3,value4,value5,value6,value7,"
		"name,short_descr,description,action_descr,bitvector1,bitvector2,bitvector3,bitvector4,"
		"bitvector5,item_material,obj_uid,item_condition) VALUES(" +
			std::to_string(ITEM_ID) + "," + std::to_string(PID) + "," +
			std::to_string(ITEM_VNUM) + ",2,NULL,1,1,2,-1,0,0,20,0,0,0,0,0,0,0,0,"
		"'projection','projection','projection','projection',0,0,0,0,0,1," +
			std::to_string(ITEM_UID) + ",98)");
	execute_sql(connection,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,item_revision,vnum,state) VALUES(" +
			std::to_string(ITEM_UID) + "," + std::to_string(ITEM_UID) + ",NULL,1," +
			std::to_string(PID) + ",0,11," + std::to_string(ITEM_VNUM) + ",1)");
	execute_sql(connection,
		"INSERT INTO player_death_restitution_receipt(restitution_id,source_pid,death_revision,"
		"recipient_pid,death_operation_id,evidence_digest,plan_digest,status,actor,reason) VALUES"
		"(UNHEX('" + RECEIPT_ID + "')," + std::to_string(PID) + ",9," + std::to_string(PID) +
		",UNHEX('11111111111111111111111111111111'),UNHEX(REPEAT('22',32)),"
		"UNHEX(REPEAT('33',32)),2,'spellbook-test','captured spell fidelity')");
	execute_sql(connection,
		"INSERT INTO player_death_restitution_item(restitution_id,item_uid,vnum,disposition,"
		"classification,metadata_digest,metadata_payload) VALUES(UNHEX('" + RECEIPT_ID + "')," +
			std::to_string(ITEM_UID) + "," + std::to_string(ITEM_VNUM) + ",1,'spellbook',"
		"UNHEX('" + digest_hex + "'),UNHEX('" + payload_hex + "'))");
	execute_sql(connection,
		"INSERT INTO player_death_restitution_delivery(item_uid,restitution_id,source_pid,"
		"death_revision,recipient_pid,source_item_revision,delivered_item_revision,"
		"delivered_item_id,metadata_digest,original_payload) VALUES(" + std::to_string(ITEM_UID) +
		",UNHEX('" + RECEIPT_ID + "')," + std::to_string(PID) + ",9," + std::to_string(PID) +
		",11,11," + std::to_string(ITEM_ID) + ",UNHEX('" + digest_hex + "'),UNHEX('" +
			payload_hex + "'))");
	execute_sql(connection,
		"INSERT INTO player_death_restitution_runtime(item_uid,recipient_pid,state_payload,"
		"state_digest) VALUES(" + std::to_string(ITEM_UID) + "," + std::to_string(PID) +
		",UNHEX('" + payload_hex + "'),UNHEX('" + digest_hex + "'))");
	require(scalar(connection, "SELECT COUNT(*) FROM player_spellbooks WHERE pid=" +
				std::to_string(PID)) == "0",
		"fixture unexpectedly seeded player_spellbooks rows");
}

void replace_runtime_payload(MYSQL *connection, const std::vector<uint8_t> &payload)
{
	execute_sql(connection,
		"UPDATE player_death_restitution_runtime SET state_payload=UNHEX('" +
			hex_encode(payload) + "'),state_digest=UNHEX('" + sha256_hex(payload) +
			"') WHERE item_uid=" + std::to_string(ITEM_UID));
}

player_load_result load_fixture(MYSQL *connection, uint64_t request_id)
{
	player_load_request request = {};
	request.schema_version = PLAYER_LOAD_SCHEMA_VERSION;
	request.request_id = request_id;
	request.pid = PID;
	request.account_name = ACCOUNT;
	request.deadline_usec = persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
	request.include_items = true;
	request.include_pets = false;
	return player_load_repository_execute(connection, request);
}

void verify_loaded_spellbook(const player_load_result &result, const char *encoding)
{
	if (result.outcome != player_load_outcome::applied)
	{
		std::cerr << "loader outcome=" << static_cast<unsigned>(result.outcome)
			  << " error=" << result.error_code << " failed_component="
			  << (result.failed_component ? result.failed_component : "none")
			  << " queries=" << result.metrics.query_count << '\n';
		fail(std::string("loader rejected authoritative ") + encoding + " state");
	}
	require(result.snapshot.items.size() == 1, "loader returned the wrong item count");
	const player_item_snapshot &item = result.snapshot.items[0];
	require(item.object_uid == ITEM_UID && item.vnum == ITEM_VNUM,
		"loader changed the delivered item identity");
	require(item.equipment_slot == 2 && item.parent_index == PLAYER_SNAPSHOT_NO_PARENT,
		"loader did not preserve live placement from the ownership projection");
	require(item.string_mask == 15 && item.name == "captured" &&
			item.short_description == "spellbook" && item.description == "authoritative" &&
			item.action_description == "death",
		"loader did not preserve the captured item fields");
	require(item.values == std::array<int32_t, 8>{ 11, 22, 33, 44, 55, 66, 77, 88 } &&
			item.timers[0] == 1234 && item.extra_flags == 0x12345678U &&
			item.wear_flags == 77 && item.type == 20 && item.material == 4 &&
			item.condition == 97 && item.bitvectors ==
				std::array<uint64_t, 5>{ 0x11, 0x22, 0x33, 0x44, 0x55 } &&
			item.affects[0] == std::array<int16_t, 2>{ 7, -3 },
		"loader did not preserve the captured item state fields");
	require(item.extra_descriptions.size() == 2 &&
			item.extra_descriptions[0].keyword == "lore" &&
			item.extra_descriptions[0].description == "captured" &&
			!item.extra_descriptions[0].spellbook &&
			item.extra_descriptions[0].spell_ids.empty(),
		"loader changed the non-spell extra description");
	const player_item_extra_description_snapshot &book = item.extra_descriptions[1];
	require(book.keyword == "SPELLBOOK" && book.description.empty() && book.spellbook &&
			book.spell_ids == CAPTURED_SPELLS,
		std::string("loader did not restore all captured spell IDs from ") + encoding);
}

void verify_materialized_spellbook(const player_load_result &result)
{
	std::vector<player_item_snapshot> items = result.snapshot.items;
	for (player_item_snapshot &item : items)
		item.equipment_slot = -1;
	std::vector<P_obj> roots;
	player_load_item_materialize_metrics metrics = {};
	const item_owner_identity owner = { item_owner_type::player, PID, 0 };
	if (!player_load_item_graph_materialize_detached(items, result.item_identities, owner,
							 result.item_owner_revision, false, true, &roots, &metrics))
	{
		std::cerr << "materializer outcome=" << static_cast<unsigned>(metrics.outcome)
			  << " items=" << metrics.item_count << '\n';
		fail("materializer rejected the loader's spellbook snapshot");
	}
	require(roots.size() == 1, "materializer returned the wrong root count");
	const std::vector<uint8_t> expected_bitmap = captured_spell_bitmap();
	const char marker[] = { 3, 1, 3 };
	const extra_descr_data *book = nullptr;
	for (const extra_descr_data *description = roots[0]->ex_description; description;
	     description = description->next)
		if (description->keyword && std::strlen(description->keyword) == sizeof(marker) &&
		    std::memcmp(description->keyword, marker, sizeof(marker)) == 0)
		{
			book = description;
			break;
		}
	require(book && book->description &&
			std::memcmp(book->description, expected_bitmap.data(), expected_bitmap.size()) == 0,
		"materializer did not recreate the captured spell bitmap");
	extract_obj(roots[0], 0);
}

void verify_rejects_incomplete_bitmap(MYSQL *connection)
{
	const std::vector<uint8_t> malformed = encode_state({ 3, 1, 3 }, { 2 });
	replace_runtime_payload(connection, malformed);
	const player_load_result result = load_fixture(connection, 202);
	require(result.outcome == player_load_outcome::component_failure &&
			result.failed_component && std::string(result.failed_component) == "items",
		"incomplete raw spellbook evidence was not rejected at the item loader");
}

void verify_rejects_malformed_json(MYSQL *connection)
{
	const std::vector<uint8_t> malformed = encode_state(
		{ 'S', 'P', 'E', 'L', 'L', 'B', 'O', 'O', 'K' },
		{ '[', '1', ',', '1', ']' });
	replace_runtime_payload(connection, malformed);
	const player_load_result result = load_fixture(connection, 203);
	require(result.outcome == player_load_outcome::component_failure &&
			result.failed_component && std::string(result.failed_component) == "items",
		"malformed JSON spellbook evidence was not rejected at the item loader");
}
} // namespace

int main()
{
	MYSQL *connection = mysql_init(nullptr);
	if (!connection)
		return 2;
	const char *host = std::getenv("DB_HOST");
	const char *user = std::getenv("DB_USER");
	const char *password = std::getenv("DB_PASSWD");
	const char *database = std::getenv("DB_NAME");
	const char *port_text = std::getenv("DB_PORT");
	if (!host || !user || !password || !database || !port_text ||
		!mysql_real_connect(connection, host, user, password, database,
						static_cast<unsigned int>(std::strtoul(port_text, nullptr, 10)), nullptr, 0))
		fail("database connection failed", connection);

	const std::vector<uint8_t> raw_keyword = { 3, 1, 3 };
	const std::vector<uint8_t> raw_payload = encode_state(raw_keyword, captured_spell_bitmap());
	seed_fixture(connection, raw_payload);
	const player_load_result raw_loaded = load_fixture(connection, 200);
	verify_loaded_spellbook(raw_loaded, "raw captured bitmap");
	verify_materialized_spellbook(raw_loaded);
	verify_rejects_incomplete_bitmap(connection);

	const std::vector<uint8_t> json_payload = encode_state(
		{ 'S', 'P', 'E', 'L', 'L', 'B', 'O', 'O', 'K' },
		{ '[', '1', ',', '7', ',', '3', '1', ']' });
	replace_runtime_payload(connection, json_payload);
	const player_load_result json_loaded = load_fixture(connection, 201);
	verify_loaded_spellbook(json_loaded, "canonical captured JSON");
	verify_materialized_spellbook(json_loaded);
	verify_rejects_malformed_json(connection);
	replace_runtime_payload(connection, json_payload);
	require(scalar(connection, "SELECT COUNT(*) FROM player_spellbooks WHERE pid=" +
				std::to_string(PID)) == "0",
		"spellbook test gained a player_spellbooks row");
	mysql_close(connection);
	std::cout << "isolated spellbook IST1 loader fidelity passed: raw bitmap and canonical JSON restored "
			  << CAPTURED_SPELLS.size() << " captured spell IDs through materialization; malformed evidence failed closed\n";
	return 0;
}
