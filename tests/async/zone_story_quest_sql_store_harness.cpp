#include "sql/zone_story_quest_state_repository.h"
#include "sql/sql.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "zone_story_quest_sql_test_support.h"
int main(int argc, char **argv)
{
	assert(argc == 3 && std::string(argv[2]).starts_with("zone_daily_"));
	DB = mysql_init(nullptr);
	assert(mysql_real_connect(DB, "localhost", "root", "zone-daily-disposable", argv[2], 0,
				  argv[1], 0));
	assert(!mysql_query(DB, "DELETE FROM zone_story_quest_state"));
	assert(!mysql_query(DB, "INSERT INTO zone_story_quest_state "
				"(state_id,state_version,catalog_revision,state_blob) "
				"VALUES (1,1,1,'ZSQF|1\\nN|7|42|416c696365|1\\n')"));
	using namespace zone_story_quest_state;
	using status = sql_zone_story_quest_state_result;
	std::string state, error;
	bool legacy = false;
	assert(sql_zone_story_quest_state_load(2, &state, &error, &legacy) == status::ok && legacy);
	changes initial{ split_document("ZSQF|2\nK|0\nN|7|42|416c696365|1\n"), true };
	// Include a payload bigger than qry's buffer to exercise the dynamic writer.
	initial.values["E:bulk"] = "E|bulk|" + std::string(100000, 'a') + "\n";
	assert(sql_zone_story_quest_records_save(2, initial, &error) == status::ok);
	assert(sql_zone_story_quest_state_load(2, &state, &error, &legacy) == status::ok &&
	       !legacy);
	assert(state == document(initial.values));
	assert(!mysql_query(
		DB,
		("UPDATE zone_story_quest_state SET state_blob=CONCAT(state_blob,'00') WHERE state_id=" +
		 std::to_string(bucket("E:bulk")))
			.c_str()));
	assert(sql_zone_story_quest_state_load(2, &state, &error) == status::invalid);
	assert(sql_zone_story_quest_records_save(2, initial, &error) == status::ok);
	assert(sql_zone_story_quest_state_load(2, &state, &error) == status::ok);
	changes update{ { { "character:7:42:V:831", "V|7|42|831|83450|864001|arrival\n" },
			  { "character:7:43", "N|7|43|426f62|1\n" } },
			false };
	assert(bucket(update.values.begin()->first) != bucket(update.values.rbegin()->first));
	fail_insert = 2;
	assert(sql_zone_story_quest_records_save(2, update, &error) == status::io_error);
	assert(sql_zone_story_quest_state_load(2, &state, &error) == status::ok &&
	       state == document(initial.values));
	assert(sql_zone_story_quest_records_save(2, update, &error) == status::ok);
	auto expected = initial.values;
	zone_story_quest_state::apply(&expected, update);
	assert(sql_zone_story_quest_state_load(2, &state, &error) == status::ok &&
	       state == document(expected));
	assert(sql_zone_story_quest_records_save(2, update, &error) == status::ok);
	changes rename{ { { "character:7:43", "N|7|43|426f626279|1\n" } }, false };
	assert(sql_zone_story_quest_records_save(2, rename, &error) == status::ok);
	zone_story_quest_state::apply(&expected, rename);
	changes remove{ { { "character:7:42:V:831", "" } }, false };
	assert(sql_zone_story_quest_records_save(2, remove, &error) == status::ok);
	zone_story_quest_state::apply(&expected, remove);
	// Repeated large overwrites exercise compressed snapshot replacement.
	for (int index = 0; index < 12; ++index)
	{
		changes overwrite{
			{ { "E:bulk", "E|bulk|" + std::string(100000, 'b' + index) + "\n" } }, false
		};
		assert(sql_zone_story_quest_records_save(2, overwrite, &error) == status::ok);
		zone_story_quest_state::apply(&expected, overwrite);
	}
	assert(sql_zone_story_quest_state_load(2, &state, &error) == status::ok &&
	       state == document(expected));
	changes erased{ { { "meta", "ZSQF|2\nK|0\n" },
			  { "character:7:42", "X|7|42\n" },
			  { "character:7:43", "N|7|43|426f62|1\n" } },
			true };
	assert(sql_zone_story_quest_records_save(2, erased, &error) == status::ok);
	assert(sql_zone_story_quest_state_load(2, &state, &error) == status::ok &&
	       state == document(erased.values));
	// Erasure rewrites physical bucket journals, including obsolete identity.
	const std::string identity_query =
		"SELECT COUNT(*) FROM zone_story_quest_state WHERE LOCATE('" +
		hex("N|7|42|416c696365|1\n") + "',state_blob)>0";
	assert(!mysql_query(DB, identity_query.c_str()));
	MYSQL_RES *identities = mysql_store_result(DB);
	assert(identities && std::string(mysql_fetch_row(identities)[0]) == "0");
	mysql_free_result(identities);
	assert(mysql_query(
		DB,
		"INSERT INTO zone_story_quest_state "
		"(state_id,state_version,catalog_revision,state_blob) VALUES (255,1,1,'legacy')"));
	mysql_close(DB);
	std::cout
		<< "SQL legacy conversion, large records, atomic rollback, replay, and erasure passed\n";
}
