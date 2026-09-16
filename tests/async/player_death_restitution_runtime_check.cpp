#include "core/prototypes.h"
#include "core/structs.h"
#include "item/item_ownership_runtime.h"
#include "player/player_load_items.h"
#include "player/player_load_repository.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_repository.h"
#include "persistence/persistence_observability.h"

#include <mysql/mysql.h>

MYSQL *DB = nullptr;

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

/*
 * The materializer is linked from the focused checker with the real production
 * player_load_items.c translation unit. These small test-only seams provide the
 * game-world/prototype hooks that are intentionally outside this repository test's
 * scope. No production loader or materializer code is replaced by these hooks.
 */
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

obj_affect *get_obj_affect(P_obj object, int type)
{
	for (obj_affect *affect = object ? object->affects : nullptr; affect; affect = affect->next)
		if (affect->type == type)
			return affect;
	return nullptr;
}

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

void act(const char *, int, P_char, P_obj, void *, int) {}

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
	       ((owner.type == item_owner_type::system ||
		 owner.type == item_owner_type::destruction) ?
			owner.id == 0 && owner.context_id == 0 :
			owner.id != 0);
}

bool item_owner_identity_equal(const item_owner_identity &left, const item_owner_identity &right)
{
	return left.type == right.type && left.id == right.id &&
	       left.context_id == right.context_id;
}

uint64_t item_transfer_result_root(const item_transfer_payload &payload)
{
	return payload.target_root_item_uid;
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
		extract_obj(child, FALSE);
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
struct runtime_row
{
	int recipient_pid = 0;
	std::string payload_hex;
	std::string digest_hex;
};

[[noreturn]] void fail(const std::string &message, MYSQL *connection = nullptr)
{
	if (connection && mysql_errno(connection))
		throw std::runtime_error(message + ": " + mysql_error(connection));
	throw std::runtime_error(message);
}

void exec_sql(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()) != 0)
		fail("SQL failed", connection);
}

MYSQL_RES *select_sql(MYSQL *connection, const std::string &sql)
{
	exec_sql(connection, sql);
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		fail("SQL result missing", connection);
	return rows;
}

uint64_t scalar_u64(MYSQL *connection, const std::string &sql)
{
	MYSQL_RES *rows = select_sql(connection, sql);
	MYSQL_ROW row = mysql_fetch_row(rows);
	if (!row || !row[0])
	{
		mysql_free_result(rows);
		fail("scalar query returned no value");
	}
	char *end = nullptr;
	const unsigned long long value = std::strtoull(row[0], &end, 10);
	const bool valid = end && end != row[0] && !*end;
	mysql_free_result(rows);
	if (!valid)
		fail("scalar query returned a non-numeric value");
	return value;
}

uint16_t port_from_environment()
{
	const char *value = std::getenv("DB_PORT");
	if (!value || !*value)
		return 3306;
	char *end = nullptr;
	const unsigned long parsed = std::strtoul(value, &end, 10);
	if (!end || end == value || *end || parsed > UINT16_MAX)
		fail("DB_PORT is invalid");
	return static_cast<uint16_t>(parsed);
}

void ensure_column(MYSQL *connection, const char *table, const char *column, const char *definition)
{
	const uint64_t present = scalar_u64(
		connection,
		std::string(
			"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() ") +
			"AND table_name='" + table + "' AND column_name='" + column + "'");
	if (!present)
		exec_sql(connection, std::string("ALTER TABLE ") + table + " ADD COLUMN " + column +
					     " " + definition);
}

void ensure_loader_tables(MYSQL *connection)
{
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS player_data (
    pid INT UNSIGNED NOT NULL,
    name VARCHAR(64) NOT NULL,
    account_name VARCHAR(50) NULL,
    short_descr VARCHAR(512) NULL,
    long_descr TEXT NULL,
    description TEXT NULL,
    title VARCHAR(512) NULL,
    poof_in VARCHAR(512) NULL, poof_out VARCHAR(512) NULL,
    m_class INT DEFAULT 0, secondary_class INT DEFAULT 0, spec INT DEFAULT 0,
    race INT DEFAULT 0, racewar INT DEFAULT 0, level INT DEFAULT 1, sex INT DEFAULT 0,
    weight INT DEFAULT 0, height INT DEFAULT 0, size INT DEFAULT 0,
    hometown INT DEFAULT 0, birthplace INT DEFAULT 0, orig_birthplace INT DEFAULT 0,
    birth_time TIMESTAMP NULL, played_time INT DEFAULT 0, last_room INT DEFAULT 0,
    last_save TIMESTAMP NULL, save_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    base_str INT DEFAULT 0, base_dex INT DEFAULT 0, base_agi INT DEFAULT 0,
    base_con INT DEFAULT 0, base_pow INT DEFAULT 0, base_int INT DEFAULT 0,
    base_wis INT DEFAULT 0, base_cha INT DEFAULT 0, base_kar INT DEFAULT 0,
    base_luk INT DEFAULT 0, mana INT DEFAULT 0, base_mana INT DEFAULT 0,
    hit_diff INT DEFAULT 0, base_hit INT DEFAULT 0, vitality INT DEFAULT 0,
    base_vitality INT DEFAULT 0, spells_memmed_extra INT DEFAULT 0,
    copper BIGINT DEFAULT 0, silver BIGINT DEFAULT 0, gold BIGINT DEFAULT 0,
    platinum BIGINT DEFAULT 0, wallet_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    exp BIGINT DEFAULT 0, epics BIGINT DEFAULT 0,
    epic_revision BIGINT UNSIGNED NOT NULL DEFAULT 0, epic_skill_points BIGINT DEFAULT 0,
    skillpoints INT DEFAULT 0, spell_bind_used BIGINT DEFAULT 0,
    act BIGINT UNSIGNED DEFAULT 0, act2 BIGINT UNSIGNED DEFAULT 0,
    act3 BIGINT UNSIGNED DEFAULT 0, vote BIGINT UNSIGNED DEFAULT 0,
    alignment INT DEFAULT 0, prestige INT DEFAULT 0, assoc_id INT DEFAULT 0,
    guild_status INT DEFAULT 0, time_left_guild TIMESTAMP NULL, nb_left_guild INT DEFAULT 0,
    time_unspecced TIMESTAMP NULL, frags BIGINT DEFAULT 0, oldfrags BIGINT DEFAULT 0,
    frag_revision BIGINT UNSIGNED NOT NULL DEFAULT 0, numb_deaths BIGINT UNSIGNED DEFAULT 0,
    echo_toggle INT DEFAULT 0, prompt INT DEFAULT 0, wiz_invis BIGINT DEFAULT 0,
    wimpy INT DEFAULT 0, aggressive INT DEFAULT -1, highest_level INT DEFAULT 0,
    screen_length INT DEFAULT 24, condition_0 INT DEFAULT 0, condition_1 INT DEFAULT 0,
    condition_2 INT DEFAULT 0, condition_3 INT DEFAULT 0, condition_4 INT DEFAULT 0,
    quest_active INT DEFAULT 0, quest_mob_vnum INT DEFAULT 0, quest_type INT DEFAULT 0,
    quest_accomplished INT DEFAULT 0, quest_started INT DEFAULT 0,
    quest_zone_number INT DEFAULT 0, quest_giver INT DEFAULT 0, quest_level INT DEFAULT 0,
    quest_receiver INT DEFAULT 0, quest_shares_left INT DEFAULT 0,
    quest_kill_how_many INT DEFAULT 0, quest_kill_original INT DEFAULT 0,
    quest_map_room INT DEFAULT 0, quest_map_bought INT DEFAULT 0,
    last_ip BIGINT UNSIGNED DEFAULT 0, output_preferences VARBINARY(512) NOT NULL DEFAULT '',
    PRIMARY KEY (pid), UNIQUE KEY player_data_name (name)
) ENGINE=InnoDB)SQL");
	ensure_column(connection, "player_data", "output_preferences",
		      "VARBINARY(512) NOT NULL DEFAULT ''");
	ensure_column(connection, "player_data", "poof_in", "VARCHAR(512) NULL");
	ensure_column(connection, "player_data", "poof_out", "VARCHAR(512) NULL");
	ensure_column(connection, "item_current_owner", "coin_payload", "MEDIUMBLOB NULL");

	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS account_characters (
    pid INT NOT NULL, account_name VARCHAR(50) NOT NULL, deleted_at TIMESTAMP NULL
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS account_banks (
    account_name VARCHAR(50) NOT NULL, racewar INT NOT NULL DEFAULT 0,
    bank_copper BIGINT DEFAULT 0, bank_silver BIGINT DEFAULT 0,
    bank_gold BIGINT DEFAULT 0, bank_platinum BIGINT DEFAULT 0,
    bank_revision BIGINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (account_name, racewar)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS player_languages (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid INT NOT NULL,
    tongue_id INT NOT NULL, proficiency INT DEFAULT 0, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS player_intros (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid INT NOT NULL,
    intro_index INT NOT NULL, intro_pid INT DEFAULT 0, intro_time TIMESTAMP NULL,
    PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS player_timers (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid INT NOT NULL,
    timer_id INT NOT NULL, timer_value TIMESTAMP NULL, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS player_undead_slots (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid INT NOT NULL,
    circle INT NOT NULL, slots INT DEFAULT 0, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS player_forged_items (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid INT NOT NULL,
    forge_index INT NOT NULL, item_vnum INT DEFAULT 0, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS player_granted_cmds (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid INT NOT NULL,
    cmd_num INT NOT NULL, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS player_skills (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid INT NOT NULL,
    skill_id INT NOT NULL, learned INT DEFAULT 0, taught INT DEFAULT 0, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS player_affects (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid INT NOT NULL,
    type INT NOT NULL, duration INT DEFAULT 0, flags INT DEFAULT 0, modifier INT DEFAULT 0,
    location INT DEFAULT 0, level INT DEFAULT 0, bitvector1 BIGINT DEFAULT 0,
    bitvector2 BIGINT DEFAULT 0, bitvector3 BIGINT DEFAULT 0, bitvector4 BIGINT DEFAULT 0,
    bitvector5 BIGINT DEFAULT 0, custom_msg_char TEXT NULL, custom_msg_room TEXT NULL,
    PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS player_shapechanges (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid INT NOT NULL,
    mob_vnum INT NOT NULL, times_researched INT DEFAULT 0,
    last_researched TIMESTAMP NULL, last_shapechanged TIMESTAMP NULL, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS zone_trophy (
    pid BIGINT NOT NULL, zone_number INT NOT NULL, exp INT NOT NULL DEFAULT 0,
    PRIMARY KEY (pid, zone_number)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS pkill_event (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, stamp TIMESTAMP NULL, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS pkill_info (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, event_id INT NOT NULL,
    pid INT NOT NULL, pk_type VARCHAR(32) NOT NULL, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS epic_ledger (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid BIGINT NOT NULL,
    reason_type INT NOT NULL, reason_id INT NOT NULL, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS epic_gain (
    id INT UNSIGNED NOT NULL AUTO_INCREMENT, pid BIGINT NOT NULL,
    time TIMESTAMP NULL, type INT NOT NULL, type_id INT NOT NULL, PRIMARY KEY (id)
) ENGINE=InnoDB)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS `player_pets` (
    restore_state MEDIUMTEXT NULL,
    hold_reason INT UNSIGNED NOT NULL DEFAULT 0,
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `owner_pid` int unsigned NOT NULL,
  `mob_vnum` int NOT NULL,
  `pet_order` tinyint DEFAULT '0',
  `hit` int DEFAULT '0',
  `max_hit` int DEFAULT '0',
  `mana` int DEFAULT '0',
  `max_mana` int DEFAULT '0',
  `vitality` int DEFAULT '0',
  `max_vitality` int DEFAULT '0',
  `charm_duration` int DEFAULT '-1',
  `room_vnum` int DEFAULT '0',
  `saved_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_owner_pid` (`owner_pid`),
  CONSTRAINT `fk_player_pets_owner` FOREIGN KEY (`owner_pid`) REFERENCES `player_data` (`pid`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci)SQL");
	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS `player_pet_items` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `pet_id` int unsigned NOT NULL,
  `vnum` int NOT NULL,
  `equip_slot` tinyint DEFAULT '0',
  `container_id` int unsigned DEFAULT NULL,
  `weight` int DEFAULT '0',
  `cost` int DEFAULT '0',
  `timer` int DEFAULT '-1',
  `extra_flags` bigint unsigned DEFAULT '0',
  `wear_flags` int DEFAULT NULL,
  `value0` int DEFAULT '0',
  `value1` int DEFAULT '0',
  `value2` int DEFAULT '0',
  `value3` int DEFAULT '0',
  `value4` int DEFAULT '0',
  `value5` int DEFAULT '0',
  `value6` int DEFAULT '0',
  `value7` int DEFAULT '0',
  `name` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `short_descr` varchar(512) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  `action_descr` text COLLATE utf8mb4_unicode_ci,
  `obj_uid` bigint unsigned DEFAULT NULL,
  `item_condition` smallint DEFAULT '100',
  `item_material` tinyint DEFAULT NULL,
  `item_type` tinyint DEFAULT NULL,
  `bitvector1` bigint unsigned DEFAULT NULL,
  `bitvector2` bigint unsigned DEFAULT NULL,
  `bitvector3` bigint unsigned DEFAULT NULL,
  `bitvector4` bigint unsigned DEFAULT NULL,
  `bitvector5` bigint unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `idx_pet_id` (`pet_id`),
  KEY `idx_container_id` (`container_id`),
  KEY `idx_obj_uid` (`obj_uid`),
  CONSTRAINT `fk_pet_items_container` FOREIGN KEY (`container_id`) REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE,
  CONSTRAINT `fk_pet_items_pet` FOREIGN KEY (`pet_id`) REFERENCES `player_pets` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci)SQL");

	exec_sql(connection,
		 "INSERT INTO player_data(pid,name,account_name,save_revision,racewar) "
		 "VALUES (43,'issue331-runtime-player','acct43',1,0) "
		 "ON DUPLICATE KEY UPDATE account_name='acct43',save_revision=1,racewar=0");
	exec_sql(connection,
		 "INSERT INTO account_banks(account_name,racewar,bank_copper,bank_silver,bank_gold,"
		 "bank_platinum,bank_revision) VALUES ('acct43',0,0,0,0,0,0) "
		 "ON DUPLICATE KEY UPDATE bank_revision=VALUES(bank_revision)");
}

const std::vector<uint64_t> restitution_uids = { 1000, 1001, 1002, 1003, 1004 };

void prepare_runtime_owner_fixture(MYSQL *connection)
{
	if (scalar_u64(connection,
		       "SELECT COUNT(*) FROM player_death_restitution_delivery WHERE item_uid IN "
		       "(1000,1001,1002,1003,1004)") != restitution_uids.size())
		fail("the parent restitution fixture did not deliver the five expected UIDs");
	exec_sql(connection,
		 "UPDATE player_items SET pid=43 WHERE obj_uid IN (1000,1001,1002,1003,1004,3000)");
	exec_sql(
		connection,
		"INSERT INTO player_items(pid,vnum,equip_slot,container_id,quantity,weight,cost,timer,"
		"extra_flags,wear_flags,item_type,value0,value1,value2,value3,value4,value5,value6,value7,"
		"name,short_descr,description,action_descr,bitvector1,bitvector2,bitvector3,bitvector4,"
		"bitvector5,item_material,obj_uid,item_condition) VALUES "
		"(43,300,0,NULL,1,1,5,123,0,0,1,0,0,0,0,0,0,0,0,"
		"'newer item','newer item','newer item','newer item',0,0,0,0,0,1,3000,99) "
		"ON DUPLICATE KEY UPDATE pid=43,vnum=300,name='newer item',short_descr='newer item',"
		"description='newer item',action_descr='newer item'");
	for (size_t index = 0; index < restitution_uids.size(); ++index)
	{
		const uint64_t uid = restitution_uids[index];
		const uint64_t parent = index == 0 ? 0 : 1000;
		const uint64_t root = 1000;
		const int32_t vnum = 100 + static_cast<int32_t>(index);
		exec_sql(connection,
			 "UPDATE item_current_owner SET root_item_uid=" + std::to_string(root) +
				 ",parent_item_uid=" + (parent ? std::to_string(parent) : "NULL") +
				 ",owner_type=1,owner_id=43,owner_context_id=0,vnum=" +
				 std::to_string(vnum) +
				 ",state=1 WHERE item_uid=" + std::to_string(uid));
	}
	exec_sql(
		connection,
		"INSERT INTO item_current_owner(item_uid,root_item_uid,parent_item_uid,owner_type,"
		"owner_id,owner_context_id,vnum,item_revision,state) VALUES "
		"(3000,3000,NULL,1,43,0,300,1,1) ON DUPLICATE KEY UPDATE root_item_uid=3000,"
		"parent_item_uid=NULL,owner_type=1,owner_id=43,owner_context_id=0,vnum=300,state=1");
	exec_sql(connection,
		 "INSERT INTO item_owner_revision(owner_type,owner_id,owner_context_id,revision) "
		 "VALUES (1,43,0,20) ON DUPLICATE KEY UPDATE revision=20");
	if (scalar_u64(connection,
		       "SELECT COUNT(*) FROM player_death_restitution_delivery WHERE item_uid IN "
		       "(1000,1001,1002,1003,1004) AND recipient_pid=42") !=
	    restitution_uids.size())
		fail("delivery recipient history was not retained");

	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS `player_pet_item_affects` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `location` tinyint unsigned DEFAULT '0',
  `modifier` int DEFAULT '0',
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_pet_item_affects` FOREIGN KEY (`item_id`) REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci)SQL");

	exec_sql(connection, R"SQL(
CREATE TABLE IF NOT EXISTS `player_pet_item_extra_descr` (
  `id` int unsigned NOT NULL AUTO_INCREMENT,
  `item_id` int unsigned NOT NULL,
  `keyword` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL,
  `description` text COLLATE utf8mb4_unicode_ci,
  PRIMARY KEY (`id`),
  KEY `idx_item_id` (`item_id`),
  CONSTRAINT `fk_pet_item_ed` FOREIGN KEY (`item_id`) REFERENCES `player_pet_items` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci)SQL");
}

player_load_result load_player(MYSQL *connection, uint64_t request_id)
{
	player_load_request request = {};
	request.schema_version = PLAYER_LOAD_SCHEMA_VERSION;
	request.request_id = request_id;
	request.pid = 43;
	request.account_name = "acct43";
	request.deadline_usec = persistence_observability_now_usec() + 2000000;
	request.include_items = true;
	// Exercise the real login request shape, including the empty-pet queries.
	request.include_pets = true;
	return player_load_repository_execute(connection, request);
}

const player_item_snapshot &item_by_uid(const player_snapshot &snapshot, uint64_t uid)
{
	for (const player_item_snapshot &item : snapshot.items)
		if (item.object_uid == uid)
			return item;
	fail("expected item UID is absent from the repository snapshot");
}

player_item_snapshot &mutable_item_by_uid(player_snapshot &snapshot, uint64_t uid)
{
	for (player_item_snapshot &item : snapshot.items)
		if (item.object_uid == uid)
			return item;
	fail("expected mutable item UID is absent from the repository snapshot");
}

void check_exact_item_payload(const player_item_snapshot &expected,
			      const player_item_snapshot &actual)
{
	if (expected.parent_index != actual.parent_index ||
	    expected.equipment_slot != actual.equipment_slot ||
	    expected.object_uid != actual.object_uid ||
	    expected.generated_key != actual.generated_key || expected.vnum != actual.vnum ||
	    expected.type != actual.type || expected.string_mask != actual.string_mask ||
	    expected.name != actual.name ||
	    expected.short_description != actual.short_description ||
	    expected.description != actual.description ||
	    expected.action_description != actual.action_description ||
	    expected.values != actual.values || expected.timers != actual.timers ||
	    expected.wear_flags != actual.wear_flags ||
	    expected.extra_flags != actual.extra_flags ||
	    expected.anti_flags != actual.anti_flags ||
	    expected.anti2_flags != actual.anti2_flags ||
	    expected.extra2_flags != actual.extra2_flags || expected.weight != actual.weight ||
	    expected.material != actual.material || expected.cost != actual.cost ||
	    expected.condition != actual.condition ||
	    expected.craftsmanship != actual.craftsmanship ||
	    expected.bitvectors != actual.bitvectors || expected.affects != actual.affects ||
	    expected.dynamic_affects.size() != actual.dynamic_affects.size() ||
	    expected.extra_descriptions.size() != actual.extra_descriptions.size())
		fail("repository item payload changed outside the intended mutation");
	for (size_t index = 0; index < expected.dynamic_affects.size(); ++index)
		if (expected.dynamic_affects[index].type != actual.dynamic_affects[index].type ||
		    expected.dynamic_affects[index].data != actual.dynamic_affects[index].data ||
		    expected.dynamic_affects[index].extra2 != actual.dynamic_affects[index].extra2)
			fail("repository dynamic affect changed outside the intended mutation");
	for (size_t index = 0; index < expected.extra_descriptions.size(); ++index)
		if (expected.extra_descriptions[index].keyword !=
			    actual.extra_descriptions[index].keyword ||
		    expected.extra_descriptions[index].description !=
			    actual.extra_descriptions[index].description ||
		    expected.extra_descriptions[index].spellbook !=
			    actual.extra_descriptions[index].spellbook ||
		    expected.extra_descriptions[index].spell_ids !=
			    actual.extra_descriptions[index].spell_ids)
			fail("repository extra description changed outside the intended mutation");
}

void check_exact_item_payloads(const player_snapshot &expected, const player_snapshot &actual)
{
	for (const player_item_snapshot &item : expected.items)
		check_exact_item_payload(item, item_by_uid(actual, item.object_uid));
}

void check_exact_loaded_state(MYSQL *connection, const player_load_result &loaded, bool mutated)
{
	if (loaded.outcome != player_load_outcome::applied || loaded.snapshot.items.size() != 6 ||
	    loaded.item_identities.size() != 6 || loaded.item_owner_revision != 20)
		fail("production loader did not apply the six-item authoritative snapshot: outcome=" +
		     std::to_string(static_cast<unsigned>(loaded.outcome)) + " failed_component=" +
		     (loaded.failed_component ? loaded.failed_component : "none") +
		     " error=" + std::to_string(loaded.error_code) +
		     " items=" + std::to_string(loaded.snapshot.items.size()) +
		     " identities=" + std::to_string(loaded.item_identities.size()) +
		     " owner_revision=" + std::to_string(loaded.item_owner_revision) +
		     " mysql_error=" +
		     (mysql_error(connection) ? mysql_error(connection) : "none"));
	const player_item_snapshot &child = item_by_uid(loaded.snapshot, 1002);
	const player_item_snapshot &unique = item_by_uid(loaded.snapshot, 1003);
	const player_item_snapshot &artifact = item_by_uid(loaded.snapshot, 1004);
	const player_item_snapshot &newer = item_by_uid(loaded.snapshot, 3000);
	const int64_t expected_key = mutated ? 777777 : 10002;
	const int64_t expected_timer = mutated ? 1900000005 : 1700000005;
	const int32_t expected_affect_data = mutated ? 73 : 39;
	const uint64_t expected_affect_extra2 = mutated ? 74 : 40;
	if (child.generated_key != expected_key || child.timers[5] != expected_timer ||
	    child.anti_flags != (mutated ? 0x1234U : 23U) ||
	    child.anti2_flags != (mutated ? 0x5678U : 24U) ||
	    child.extra2_flags != (mutated ? 0x9abcU : 25U) ||
	    child.craftsmanship != (mutated ? 47 : 29) || child.dynamic_affects.size() != 1 ||
	    child.dynamic_affects[0].data != expected_affect_data ||
	    child.dynamic_affects[0].extra2 != expected_affect_extra2 ||
	    unique.name != "unique gloves" || artifact.extra_flags != (UINT32_C(1) << 29) ||
	    artifact.timers[0] != 1700000100 || newer.name != "newer item")
		fail("exact runtime state did not survive the production loader: key=" +
		     std::to_string(child.generated_key) + " timer5=" +
		     std::to_string(child.timers[5]) + " anti=" + std::to_string(child.anti_flags) +
		     " anti2=" + std::to_string(child.anti2_flags) +
		     " extra2=" + std::to_string(child.extra2_flags) +
		     " craft=" + std::to_string(child.craftsmanship) +
		     " dyn=" + std::to_string(child.dynamic_affects.size()) +
		     (child.dynamic_affects.empty() ?
			      "" :
			      " dyn_data=" + std::to_string(child.dynamic_affects[0].data) +
				      " dyn_extra2=" +
				      std::to_string(child.dynamic_affects[0].extra2)) +
		     " unique=" + unique.name +
		     " artifact_extra=" + std::to_string(artifact.extra_flags) +
		     " artifact_timer=" + std::to_string(artifact.timers[0]) +
		     " newer=" + newer.name);
}

std::map<uint64_t, runtime_row> read_runtime_rows(MYSQL *connection)
{
	MYSQL_RES *rows = select_sql(
		connection,
		"SELECT item_uid,recipient_pid,HEX(state_payload),HEX(state_digest) "
		"FROM player_death_restitution_runtime WHERE item_uid IN (1000,1001,1002,1003,1004) "
		"ORDER BY item_uid");
	std::map<uint64_t, runtime_row> result;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(rows)) != nullptr)
	{
		if (!row[0] || !row[1] || !row[2] || !row[3])
		{
			mysql_free_result(rows);
			fail("runtime state row contains NULL data");
		}
		char *end = nullptr;
		const unsigned long long uid = std::strtoull(row[0], &end, 10);
		if (!end || end == row[0] || *end)
		{
			mysql_free_result(rows);
			fail("runtime state returned a non-numeric UID");
		}
		runtime_row value;
		value.recipient_pid = static_cast<int>(std::strtol(row[1], nullptr, 10));
		value.payload_hex = row[2];
		value.digest_hex = row[3];
		result.emplace(uid, std::move(value));
	}
	mysql_free_result(rows);
	if (result.size() != restitution_uids.size())
		fail("runtime state row count changed unexpectedly");
	return result;
}

void restore_runtime_row(MYSQL *connection, uint64_t uid, const runtime_row &row)
{
	exec_sql(
		connection,
		"INSERT INTO player_death_restitution_runtime(item_uid,recipient_pid,state_payload,"
		"state_digest) VALUES (" +
			std::to_string(uid) + "," + std::to_string(row.recipient_pid) + ",UNHEX('" +
			row.payload_hex + "'),UNHEX(SHA2(UNHEX('" + row.payload_hex +
			"'),256))) "
			"ON DUPLICATE KEY UPDATE recipient_pid=VALUES(recipient_pid),"
			"state_payload=VALUES(state_payload),state_digest=VALUES(state_digest)");
}

void recreate_runtime_table(MYSQL *connection, const std::map<uint64_t, runtime_row> &rows)
{
	exec_sql(connection, R"SQL(
CREATE TABLE player_death_restitution_runtime (
    item_uid BIGINT UNSIGNED NOT NULL,
    recipient_pid INT NOT NULL,
    state_payload MEDIUMBLOB NOT NULL,
    state_digest BINARY(32) NOT NULL,
    updated_at TIMESTAMP(6) NOT NULL DEFAULT CURRENT_TIMESTAMP(6)
        ON UPDATE CURRENT_TIMESTAMP(6),
    PRIMARY KEY (item_uid),
    KEY idx_restitution_runtime_recipient (recipient_pid,item_uid),
    CONSTRAINT restitution_runtime_delivery_fk FOREIGN KEY (item_uid)
        REFERENCES player_death_restitution_delivery(item_uid)
        ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB)SQL");
	for (const auto &entry : rows)
		restore_runtime_row(connection, entry.first, entry.second);
}

void require_load_rejected(MYSQL *connection, uint64_t request_id, const char *reason)
{
	const player_load_result rejected = load_player(connection, request_id);
	if (rejected.outcome == player_load_outcome::applied || !rejected.failed_component ||
	    std::strcmp(rejected.failed_component, "items") != 0)
		fail(std::string("corrupt runtime state was accepted: ") + reason);
}

player_save_apply_result apply_snapshot(MYSQL *connection, player_snapshot snapshot,
					uint64_t revision)
{
	snapshot.pid = 43;
	snapshot.revision = revision;
	snapshot.schema_version = PLAYER_SNAPSHOT_SCHEMA_VERSION;
	snapshot.components = PLAYER_COMPONENT_EQUIPMENT | PLAYER_COMPONENT_INVENTORY;
	snapshot.death.reset();
	return player_snapshot_repository_apply(connection, snapshot);
}

P_obj find_materialized(P_obj object, uint64_t uid)
{
	for (P_obj cursor = object; cursor; cursor = cursor->next_content)
	{
		if (cursor->obj_uid == uid)
			return cursor;
		if (P_obj child = find_materialized(cursor->contains, uid))
			return child;
	}
	return nullptr;
}

void check_full_materialization(const player_load_result &loaded)
{
	std::vector<player_item_snapshot> items = loaded.snapshot.items;
	for (player_item_snapshot &item : items)
		item.equipment_slot = -1;
	std::vector<P_obj> roots;
	player_load_item_materialize_metrics metrics = {};
	const item_owner_identity owner = { item_owner_type::player, 43, 0 };
	if (!player_load_item_graph_materialize_detached(items, loaded.item_identities, owner,
							 loaded.item_owner_revision, false, true,
							 &roots, &metrics))
		fail("full-snapshot detached materialization rejected the repository result");
	if (roots.size() != 2)
	{
		for (P_obj root : roots)
			extract_obj(root, FALSE);
		fail("full-snapshot materialization produced the wrong root count");
	}
	P_obj child = nullptr;
	for (P_obj root : roots)
		if ((child = find_materialized(root, 1002)) != nullptr)
			break;
	if (!child || child->g_key != 777777 || child->timer[5] != 1900000005 ||
	    child->anti_flags != 0x1234U || child->anti2_flags != 0x5678U ||
	    child->extra2_flags != 0x9abcU || child->craftsmanship != 47 || !child->affects ||
	    child->affects->data != 73 || child->affects->extra2 != 74)
	{
		for (P_obj root : roots)
			extract_obj(root, FALSE);
		fail("full-snapshot materialization lost generated or dynamic item state");
	}
	for (P_obj root : roots)
		extract_obj(root, FALSE);
}
}

int main()
{
	MYSQL *connection = mysql_init(nullptr);
	if (!connection)
		return 2;
	try
	{
		const char *host = std::getenv("DB_HOST");
		const char *user = std::getenv("DB_USER");
		const char *password = std::getenv("DB_PASSWD");
		const char *database = std::getenv("DB_NAME");
		if (!host || !user || !password || !database ||
		    !mysql_real_connect(connection, host, user, password, database,
					port_from_environment(), nullptr, 0))
			fail("database connection failed", connection);

		ensure_loader_tables(connection);
		prepare_runtime_owner_fixture(connection);
		player_load_result loaded = load_player(connection, 331);
		check_exact_loaded_state(connection, loaded, false);

		player_snapshot mutated = loaded.snapshot;
		player_item_snapshot &changed = mutable_item_by_uid(mutated, 1002);
		changed.generated_key = 777777;
		changed.timers[5] = 1900000005;
		changed.anti_flags = 0x1234;
		changed.anti2_flags = 0x5678;
		changed.extra2_flags = 0x9abc;
		changed.craftsmanship = 47;
		changed.dynamic_affects[0].data = 73;
		changed.dynamic_affects[0].extra2 = 74;
		const player_save_apply_result saved = apply_snapshot(connection, mutated, 2);
		if (saved.outcome != player_save_apply_outcome::applied ||
		    saved.durable_revision != 2)
			fail("normal repository save did not commit the mutated exact state");
		if (scalar_u64(connection, "SELECT COUNT(*) FROM player_items WHERE pid=43") != 6 ||
		    scalar_u64(connection,
			       "SELECT COUNT(*) FROM player_items WHERE pid=43 AND obj_uid=3000") !=
			    1)
			fail("normal save did not preserve the newer unrelated inventory item");
		const std::map<uint64_t, runtime_row> saved_runtime = read_runtime_rows(connection);
		for (const auto &entry : saved_runtime)
			if (entry.second.recipient_pid != 42)
				fail("normal save rewrote immutable delivery recipient history");

		loaded = load_player(connection, 332);
		check_exact_loaded_state(connection, loaded, true);
		check_exact_item_payloads(mutated, loaded.snapshot);
		check_full_materialization(loaded);

		const runtime_row &changed_runtime = saved_runtime.at(1002);
		exec_sql(connection,
			 "UPDATE player_death_restitution_item SET vnum=999 WHERE item_uid=1002");
		require_load_rejected(connection, 333, "custody vnum mismatch");
		exec_sql(connection,
			 "UPDATE player_death_restitution_item SET vnum=102 WHERE item_uid=1002");
		exec_sql(
			connection,
			"UPDATE player_death_restitution_runtime SET state_digest=REPEAT(0x00,32) WHERE item_uid=1002");
		require_load_rejected(connection, 334, "digest mismatch");
		restore_runtime_row(connection, 1002, changed_runtime);
		exec_sql(
			connection,
			"UPDATE player_death_restitution_runtime SET state_payload=UNHEX('00'),state_digest=UNHEX(SHA2(UNHEX('00'),256)) WHERE item_uid=1002");
		require_load_rejected(connection, 335, "malformed payload");
		restore_runtime_row(connection, 1002, changed_runtime);

		exec_sql(connection,
			 "DELETE FROM player_death_restitution_runtime WHERE item_uid=1002");
		require_load_rejected(connection, 335, "missing runtime row");
		const player_save_apply_result missing_row_save =
			apply_snapshot(connection, loaded.snapshot, 3);
		if (missing_row_save.outcome == player_save_apply_outcome::applied ||
		    scalar_u64(connection, "SELECT save_revision FROM player_data WHERE pid=43") !=
			    2 ||
		    scalar_u64(connection, "SELECT COUNT(*) FROM player_items WHERE pid=43") != 6)
			fail("save did not roll back when a scoped runtime row was missing");
		restore_runtime_row(connection, 1002, changed_runtime);

		exec_sql(connection, "DROP TABLE player_death_restitution_runtime");
		require_load_rejected(connection, 336, "missing runtime table");
		const player_save_apply_result missing_table_save =
			apply_snapshot(connection, loaded.snapshot, 3);
		if (missing_table_save.outcome == player_save_apply_outcome::applied ||
		    scalar_u64(connection, "SELECT save_revision FROM player_data WHERE pid=43") !=
			    2 ||
		    scalar_u64(connection, "SELECT COUNT(*) FROM player_items WHERE pid=43") != 6)
			fail("save did not roll back when the runtime table was missing");
		recreate_runtime_table(connection, saved_runtime);
		const player_save_apply_result restored_save =
			apply_snapshot(connection, loaded.snapshot, 3);
		if (restored_save.outcome != player_save_apply_outcome::applied ||
		    scalar_u64(connection, "SELECT save_revision FROM player_data WHERE pid=43") !=
			    3)
			fail("normal save did not recover after restoring the runtime table");

		exec_sql(connection, "DELETE FROM player_items WHERE pid=43 AND obj_uid=1002");
		const player_load_result consumed = load_player(connection, 337);
		if (consumed.outcome != player_load_outcome::applied ||
		    consumed.snapshot.items.size() != 5 || consumed.missing_payload_rows != 1 ||
		    std::any_of(consumed.snapshot.items.begin(), consumed.snapshot.items.end(),
				[](const player_item_snapshot &item)
				{ return item.object_uid == 1002; }) ||
		    scalar_u64(
			    connection,
			    "SELECT COUNT(*) FROM player_death_restitution_delivery WHERE item_uid=1002") !=
			    1 ||
		    scalar_u64(connection,
			       "SELECT COUNT(*) FROM player_items WHERE obj_uid=1002") != 0)
			fail("consumed delivery blocked login, disappeared from audit, or was resurrected");
		/* Restore the deliberately removed fixture from its retained test snapshot. */
		const player_save_apply_result final_restore =
			apply_snapshot(connection, mutated, 4);
		if (final_restore.outcome != player_save_apply_outcome::applied)
			fail("failed projection fixture could not be restored");
		loaded = load_player(connection, 339);
		check_exact_loaded_state(connection, loaded, true);
		check_exact_item_payloads(mutated, loaded.snapshot);
		check_full_materialization(loaded);

		mysql_close(connection);
		std::cout
			<< "player restitution runtime repository integration verified: load/save, rollback, "
			   "authority transfer, newer inventory, corrupt-state rejection, and full materialization\n";
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << "player restitution runtime integration failed: " << error.what()
			  << '\n';
		mysql_close(connection);
		return 1;
	}
}
