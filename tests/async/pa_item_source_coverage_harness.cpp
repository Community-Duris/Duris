#include "persistence/economic_sql_source_snapshot.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace
{
const char *required(const char *key)
{
	const char *value = std::getenv(key);
	assert(value && *value);
	return value;
}
using connection = std::unique_ptr<MYSQL, decltype(&mysql_close)>;
connection connect_fixture()
{
	assert(!std::strcmp(required("ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA"), "1"));
	assert(!std::strcmp(required("DB_HOST"), "127.0.0.1"));
	assert(!std::getenv("DB_SOCKET") || !*std::getenv("DB_SOCKET"));
	const std::string name = required("DB_NAME");
	assert(name.starts_with("economic_schema_test_pa_"));
	assert(name.find_first_not_of(
		       "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") ==
	       std::string::npos);
	connection c(mysql_init(nullptr), mysql_close);
	assert(c);
	unsigned timeout = 10, protocol = MYSQL_PROTOCOL_TCP;
	assert(!mysql_options(c.get(), MYSQL_OPT_CONNECT_TIMEOUT, &timeout));
	assert(!mysql_options(c.get(), MYSQL_OPT_READ_TIMEOUT, &timeout));
	assert(!mysql_options(c.get(), MYSQL_OPT_WRITE_TIMEOUT, &timeout));
	assert(!mysql_options(c.get(), MYSQL_OPT_PROTOCOL, &protocol));
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	assert(!mysql_options(c.get(), MYSQL_OPT_RECONNECT, &reconnect));
	assert(mysql_real_connect(
		c.get(), "127.0.0.1", required("DB_USER"), required("DB_PASSWD"), name.c_str(),
		static_cast<unsigned>(std::strtoul(required("DB_PORT"), nullptr, 10)), nullptr, 0));
	return c;
}
const economic_sql_source_table &table(const economic_sql_source_snapshot &snapshot,
				       std::string_view name)
{
	auto found = std::find_if(snapshot.item_sources.begin(), snapshot.item_sources.end(),
				  [&](const auto &entry) { return entry.name == name; });
	assert(found != snapshot.item_sources.end());
	return *found;
}
void verify_sources(MYSQL *connection)
{
	economic_sql_source_snapshot snapshot;
	assert(!economic_sql_capture_sources(connection, {}, &snapshot));
	assert(!economic_sql_validate_sources(snapshot));
	assert(!(connection->server_status & SERVER_STATUS_IN_TRANS));
	assert(snapshot.tables.size() == 20 && snapshot.item_sources.size() == 3);

	const auto &pets = table(snapshot, "player_pet_items");
	assert((pets.columns ==
		std::vector<std::string>{ "id", "pet_id", "container_id", "obj_uid", "vnum" }));
	assert(pets.rows.size() == 6);
	assert(pets.rows[0].cells[0] == "1" && pets.rows[0].cells[1] == "101" &&
	       !pets.rows[0].cells[2] && pets.rows[0].cells[3] == "9200" &&
	       pets.rows[0].cells[4] == "200");
	assert(pets.rows[1].cells[0] == "2" && pets.rows[1].cells[1] == "101" &&
	       pets.rows[1].cells[2] == "1" && pets.rows[1].cells[3] == "9201" &&
	       pets.rows[1].cells[4] == "201");
	assert(pets.rows[2].cells[3] == "9301" && pets.rows[3].cells[3] == "9302");
	assert(pets.rows[4].cells[0] == "5" && pets.rows[4].cells[3] == "9220");
	assert(pets.rows[5].cells[0] == "6" && pets.rows[5].cells[2] == "5" &&
	       pets.rows[5].cells[3] == "9219");

	const auto &shops = table(snapshot, "shopkeeper_items");
	assert((shops.columns == std::vector<std::string>{ "id", "shopkeeper_id", "container_id",
							   "obj_uid", "vnum", "item_condition" }));
	assert(shops.rows.size() == 3);
	assert(shops.rows[0].cells[1] == "201" && shops.rows[0].cells[3] == "9400" &&
	       shops.rows[0].cells[5] == "37");
	assert(shops.rows[1].cells[3] == "9600" && shops.rows[2].cells[3] == "9602");

	const auto &siege = table(snapshot, "siege_items");
	assert((siege.columns ==
		std::vector<std::string>{ "id", "room_vnum", "container_id", "obj_uid", "vnum" }));
	assert(siege.rows.size() == 2 && siege.rows[0].cells[1] == "700" &&
	       siege.rows[0].cells[3] == "9500" && siege.rows[1].cells[1] == "701" &&
	       siege.rows[1].cells[3] == "9602");

	// EIM1 binds only raw selected item rows. Owner mapping changes are outside
	// that digest and do not prove that a resolved ownership baseline is unchanged.
	constexpr char change_pet_mapping[] = "UPDATE player_pets SET pet_uid=9199 WHERE id=101";
	assert(!mysql_real_query(connection, change_pet_mapping, sizeof(change_pet_mapping) - 1));
	economic_sql_source_snapshot pet_mapping_changed;
	assert(!economic_sql_capture_sources(connection, {}, &pet_mapping_changed));
	assert(pet_mapping_changed.digest == snapshot.digest);
	assert(pet_mapping_changed.item_sources_digest == snapshot.item_sources_digest);
	constexpr char restore_pet_mapping[] = "UPDATE player_pets SET pet_uid=9101 WHERE id=101";
	assert(!mysql_real_query(connection, restore_pet_mapping, sizeof(restore_pet_mapping) - 1));
	constexpr char change_shop_mapping[] = "UPDATE shopkeepers SET shop_id=42 WHERE id=201";
	assert(!mysql_real_query(connection, change_shop_mapping, sizeof(change_shop_mapping) - 1));
	economic_sql_source_snapshot shop_mapping_changed;
	assert(!economic_sql_capture_sources(connection, {}, &shop_mapping_changed));
	assert(shop_mapping_changed.digest != snapshot.digest);
	assert(shop_mapping_changed.item_sources_digest == snapshot.item_sources_digest);
	constexpr char restore_shop_mapping[] = "UPDATE shopkeepers SET shop_id=41 WHERE id=201";
	assert(!mysql_real_query(connection, restore_shop_mapping,
				 sizeof(restore_shop_mapping) - 1));
	constexpr char change_condition[] =
		"UPDATE shopkeeper_items SET item_condition=38 WHERE id=1";
	assert(!mysql_real_query(connection, change_condition, sizeof(change_condition) - 1));
	economic_sql_source_snapshot condition_changed;
	assert(!economic_sql_capture_sources(connection, {}, &condition_changed));
	assert(condition_changed.digest == snapshot.digest);
	assert(condition_changed.item_sources_digest != snapshot.item_sources_digest);
	constexpr char restore_condition[] =
		"UPDATE shopkeeper_items SET item_condition=37 WHERE id=1";
	assert(!mysql_real_query(connection, restore_condition, sizeof(restore_condition) - 1));

	// ESM1 remains the pre-existing phase-1/2 replay identity; new raw item
	// source rows get an independent digest without changing that identity.
	constexpr char change[] = "UPDATE siege_items SET obj_uid=9501 WHERE id=1";
	assert(!mysql_real_query(connection, change, sizeof(change) - 1));
	economic_sql_source_snapshot changed;
	assert(!economic_sql_capture_sources(connection, {}, &changed));
	assert(changed.digest == snapshot.digest);
	assert(changed.item_sources_digest != snapshot.item_sources_digest);
	constexpr char restore[] = "UPDATE siege_items SET obj_uid=9500 WHERE id=1";
	assert(!mysql_real_query(connection, restore, sizeof(restore) - 1));
	assert(snapshot ==
	       [&]()
	       {
		       economic_sql_source_snapshot restored;
		       assert(!economic_sql_capture_sources(connection, {}, &restored));
		       return restored;
	       }());
}
void verify_missing_source_refusal(MYSQL *connection)
{
	economic_sql_source_snapshot unchanged;
	unchanged.version = 91;
	unchanged.rows = 47;
	unchanged.cells = 13;
	unchanged.item_sources.push_back({ "sentinel", {}, {}, {}, {} });
	const auto before = unchanged;
	assert(economic_sql_capture_sources(connection, {}, &unchanged) != 0);
	assert(unchanged == before);
	assert(!(connection->server_status & SERVER_STATUS_IN_TRANS));
}
}

int main(int argc, char **argv)
{
	auto c = connect_fixture();
	if (argc == 2 && std::string_view(argv[1]) == "--missing-source")
		verify_missing_source_refusal(c.get());
	else
	{
		assert(argc == 1);
		verify_sources(c.get());
	}
	std::cout << "durable SQL item source snapshot coverage PASS\n";
}
