
#include "prototypes.h"
#include "structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/events.h"
#include "cmd/interp.h"
#include "utility.h"
#include "utils.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "world/graph.h"
#include "item/objmisc.h"
#include "ships.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "sql/sql_player.h"
#include "world/timers.h"
#ifdef __NO_MYSQL__
#include "flatfile/flatfile_store.h"
#include "persistence/persistence_mode.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <string>
#include <vector>
#endif

float ship_cargo_market_mod[NUM_PORTS][NUM_PORTS];
float ship_cargo_market_mod_delayed[NUM_PORTS][NUM_PORTS];
float ship_contra_market_mod[NUM_PORTS][NUM_PORTS];
static int cargo_maintenance_last_update;
static int cargo_maintenance_last_delayed_update;
static uint64_t cargo_maintenance_work_id;
static size_t cargo_maintenance_value_count;
static int64_t cargo_maintenance_values[3 + NUM_PORTS * NUM_PORTS * 4];

#ifdef __NO_MYSQL__
namespace
{
constexpr std::array<uint8_t, 8> cargo_magic = { 'D', 'U', 'R', 'C', 'A', 'R', 'G', 'O' };
constexpr uint32_t cargo_version = 1;
constexpr size_t cargo_value_count = NUM_PORTS * NUM_PORTS;
constexpr size_t cargo_record_size = cargo_magic.size() + sizeof(uint32_t) + sizeof(uint64_t) +
				     cargo_value_count * sizeof(uint32_t) * 2 +
				     SHA256_DIGEST_LENGTH;
constexpr const char *cargo_filename = "cargo_market";
constexpr const char *cargo_lock_filename = "cargo_market.lock";

enum class flat_cargo_result
{
	ok,
	not_found,
	invalid,
	io_error
};

struct flat_cargo_record
{
	uint64_t revision = 0;
	std::array<float, cargo_value_count> cargo = {};
	std::array<float, cargo_value_count> contraband = {};
};

std::string flat_cargo_directory()
{
	const char *root = persistence_mode_flatfile_root();
	return root ? std::string(root) + "/metadata" : std::string();
}

bool valid_market_modifier(float value)
{
	return std::isfinite(value) && value >= 0.0f && value <= 1000.0f;
}

void append_u32(std::vector<uint8_t> *bytes, uint32_t value)
{
	for (size_t offset = 0; offset < sizeof(value); ++offset)
	{
		bytes->push_back(static_cast<uint8_t>(value & 0xff));
		value >>= 8;
	}
}

void append_u64(std::vector<uint8_t> *bytes, uint64_t value)
{
	for (size_t offset = 0; offset < sizeof(value); ++offset)
	{
		bytes->push_back(static_cast<uint8_t>(value & 0xff));
		value >>= 8;
	}
}

uint32_t read_u32(const uint8_t *bytes)
{
	uint32_t value = 0;
	for (size_t offset = 0; offset < sizeof(value); ++offset)
		value |= static_cast<uint32_t>(bytes[offset]) << (offset * 8);
	return value;
}

uint64_t read_u64(const uint8_t *bytes)
{
	uint64_t value = 0;
	for (size_t offset = 0; offset < sizeof(value); ++offset)
		value |= static_cast<uint64_t>(bytes[offset]) << (offset * 8);
	return value;
}

bool valid_flat_cargo_record(const flat_cargo_record &record)
{
	if (!record.revision)
		return false;
	for (float value : record.cargo)
		if (!valid_market_modifier(value))
			return false;
	for (float value : record.contraband)
		if (!valid_market_modifier(value))
			return false;
	return true;
}

bool encode_flat_cargo(const flat_cargo_record &record, std::vector<uint8_t> *bytes)
{
	static_assert(sizeof(float) == sizeof(uint32_t));
	if (!bytes || !valid_flat_cargo_record(record))
		return false;
	try
	{
		bytes->clear();
		bytes->reserve(cargo_record_size);
		bytes->insert(bytes->end(), cargo_magic.begin(), cargo_magic.end());
		append_u32(bytes, cargo_version);
		append_u64(bytes, record.revision);
		for (float value : record.cargo)
			append_u32(bytes, std::bit_cast<uint32_t>(value));
		for (float value : record.contraband)
			append_u32(bytes, std::bit_cast<uint32_t>(value));
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
		SHA256(bytes->data(), bytes->size(), digest.data());
		bytes->insert(bytes->end(), digest.begin(), digest.end());
	}
	catch (const std::bad_alloc &)
	{
		bytes->clear();
		return false;
	}
	return bytes->size() == cargo_record_size;
}

bool decode_flat_cargo(const std::vector<uint8_t> &bytes, flat_cargo_record *record)
{
	if (!record || bytes.size() != cargo_record_size ||
	    !std::equal(cargo_magic.begin(), cargo_magic.end(), bytes.begin()) ||
	    read_u32(bytes.data() + cargo_magic.size()) != cargo_version)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size() - digest.size(), digest.data());
	if (CRYPTO_memcmp(bytes.data() + bytes.size() - digest.size(), digest.data(),
			  digest.size()))
		return false;
	flat_cargo_record decoded;
	size_t offset = cargo_magic.size() + sizeof(uint32_t);
	decoded.revision = read_u64(bytes.data() + offset);
	offset += sizeof(uint64_t);
	for (float &value : decoded.cargo)
	{
		value = std::bit_cast<float>(read_u32(bytes.data() + offset));
		offset += sizeof(uint32_t);
	}
	for (float &value : decoded.contraband)
	{
		value = std::bit_cast<float>(read_u32(bytes.data() + offset));
		offset += sizeof(uint32_t);
	}
	if (offset + digest.size() != bytes.size() || !valid_flat_cargo_record(decoded))
		return false;
	*record = decoded;
	return true;
}

flat_cargo_result read_flat_cargo_unlocked(const std::string &directory, flat_cargo_record *record,
					   std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto result =
		flatfile_read(directory, cargo_filename, cargo_record_size, &bytes, error);
	if (result == flatfile_read_result::not_found)
		return flat_cargo_result::not_found;
	if (result == flatfile_read_result::io_error)
		return flat_cargo_result::io_error;
	if (result != flatfile_read_result::ok || !decode_flat_cargo(bytes, record))
	{
		if (error && error->empty())
			*error = "cargo market authority is corrupt";
		return flat_cargo_result::invalid;
	}
	return flat_cargo_result::ok;
}

flat_cargo_result load_flat_cargo(flat_cargo_record *record, std::string *error)
{
	const std::string directory = flat_cargo_directory();
	if (directory.empty() || !record)
		return flat_cargo_result::invalid;
	int lock_fd = -1;
	if (!flatfile_lock_acquire(directory, cargo_lock_filename, &lock_fd, error))
		return flat_cargo_result::io_error;
	const auto result = read_flat_cargo_unlocked(directory, record, error);
	flatfile_lock_release(lock_fd);
	return result;
}

flat_cargo_result save_flat_cargo(flat_cargo_record record, std::string *error)
{
	const std::string directory = flat_cargo_directory();
	if (directory.empty())
		return flat_cargo_result::invalid;
	int lock_fd = -1;
	if (!flatfile_lock_acquire(directory, cargo_lock_filename, &lock_fd, error))
		return flat_cargo_result::io_error;
	flat_cargo_record existing;
	const auto loaded = read_flat_cargo_unlocked(directory, &existing, error);
	if (loaded == flat_cargo_result::ok)
	{
		if (existing.revision == std::numeric_limits<uint64_t>::max())
		{
			flatfile_lock_release(lock_fd);
			return flat_cargo_result::invalid;
		}
		record.revision = existing.revision + 1;
	}
	else if (loaded == flat_cargo_result::not_found)
		record.revision = 1;
	else
	{
		flatfile_lock_release(lock_fd);
		return loaded;
	}
	std::vector<uint8_t> bytes;
	const bool encoded = encode_flat_cargo(record, &bytes);
	const bool stored = encoded &&
			    flatfile_atomic_write(directory, cargo_filename, bytes, error);
	flatfile_lock_release(lock_fd);
	return stored  ? flat_cargo_result::ok :
	       encoded ? flat_cargo_result::io_error :
			 flat_cargo_result::invalid;
}

flat_cargo_record capture_flat_cargo()
{
	flat_cargo_record record;
	for (int port = 0; port < NUM_PORTS; ++port)
		for (int type = 0; type < NUM_PORTS; ++type)
		{
			const size_t index = port * NUM_PORTS + type;
			record.cargo[index] = ship_cargo_market_mod[port][type];
			record.contraband[index] = ship_contra_market_mod[port][type];
		}
	return record;
}

void replace_live_cargo(const flat_cargo_record &record)
{
	for (int port = 0; port < NUM_PORTS; ++port)
		for (int type = 0; type < NUM_PORTS; ++type)
		{
			const size_t index = port * NUM_PORTS + type;
			ship_cargo_market_mod[port][type] = record.cargo[index];
			ship_cargo_market_mod_delayed[port][type] = record.cargo[index];
			ship_contra_market_mod[port][type] = record.contraband[index];
		}
}
} // namespace
#endif

// This matrix shows the base cost in platinum for each port's cargo/contraband,
// as well as the minimum number of ship frags required to be able to buy that
// type of contraband
const CargoData cargo_location_data[NUM_PORTS] = {
	//  Base cargo cost, Base contra cost, Required frags for contraband
	{ 42, 192, 150 }, { 46, 202, 150 }, { 40, 176, 100 }, { 56, 196, 150 }, { 36, 214, 200 },
	{ 44, 183, 100 }, { 38, 220, 200 }, { 52, 204, 150 }, { 48, 190, 150 }, { 69, 312, 250 },
};

// This is the matrix that shows each port's preference for the other ports' cargo. Number is percentage.
const int cargo_location_mod[NUM_PORTS][NUM_PORTS] = {
	//            Flann  Dalvik Menden Myrabo Torrha Sarmiz Storm  Venan' Thur'G             MIN     MAX
	/*  Flann */ { 0, 254, 190, 214, 225, 311, 211, 249, 272, 200 }, // Menden  Sarmiz
	/* Dalvik */ { 254, 0, 286, 178, 235, 257, 275, 296, 201, 208 }, // Myrabo  Venan
	/* Menden */ { 190, 286, 0, 246, 271, 290, 179, 185, 297, 252 }, // Storm   Thur'G
	/* Myrabo */ { 214, 178, 246, 0, 273, 287, 290, 247, 239, 289 }, // Dalvik  Storm
	/* Torrha */ { 225, 235, 271, 273, 0, 308, 224, 254, 203, 313 }, // Thur'G  Sarmiz
	/* Sarmiz */ { 311, 257, 290, 287, 308, 0, 243, 276, 314, 386 }, // Storm   Thur'G
	/* Storm  */ { 211, 275, 179, 290, 224, 243, 0, 231, 271, 184 }, // Menden  Myrabo
	/* Venan' */ { 249, 296, 185, 247, 254, 276, 231, 0, 252, 297 }, // Menden  Dalvik
	/* Thur'G */ { 272, 201, 297, 239, 203, 314, 271, 252, 0, 281 }, // Dalvik  Sarmiz
	/* Dera   */ { 252, 207, 283, 309, 351, 325, 406, 264, 317, 0 } //
	// Shortest route: Dalvik <-> Myrabolus (178)
	// Longest route: Sarmiz'Duul <-> Thur'Gurax (314)
	// Num routes: 36, Average distance: 250
};

const char *cargo_name[NUM_PORTS] = {
	"&+LCured &+rMeats&N",	 "&+GExotic &+yFoods&N",
	"&+gPine &+LPitch&N",	 "&+CElven &+RWines&N",
	"&+LBulk &+yLumber&N",	 "&+LBlack&+WSteel &+wIngots&N",
	"&+LBulk Coal&N",	 "&+MSi&+mlk &+BCl&+Co&+Yth&N",
	"&+YCopper &+yIngots&N", "&+yDw&+Ya&+yrv&+Ye&+yn &+mMi&+Mthr&+mil&N"
};

const char *contra_name[NUM_PORTS] = {
	"&+gAncient &+LBooks &+gand &+yScrolls&N",
	"&+GExotic &+WHerbs&N",
	"&+GExotic &+COils&N",
	"&+CElvish &+BAntiquities&N",
	"&+GRare &+YMagical &+yComponents&N",
	"&+RRare &+MDyes&N",
	"&+LR&+woug&+Lh &+WD&+wi&+Wa&+wm&+Wo&+wn&+Wd&+ws&N",
	"&+LR&+woug&+Lh &+RR&+ru&+Rb&+ri&+Re&+rs&N",
	"&+mUnderdark &+MMithril&N",
	"&+WWhite Dragon &+rEggs&N",
};

void reset_cargo()
{
	for (int i = 0; i < NUM_PORTS; i++)
	{
		for (int j = 0; j < NUM_PORTS; j++)
		{
			ship_cargo_market_mod[i][j] = 1.0;
			ship_contra_market_mod[i][j] = 1.0;
			ship_cargo_market_mod_delayed[i][j] = 1.0;
		}
	}
}

void initialize_ship_cargo()
{
	reset_cargo();

	if (!read_cargo())
	{
#ifdef __NO_MYSQL__
		fatal_boot_error("ship_cargo", "flat cargo market authority could not be loaded");
#else
		logit(LOG_SHIP, "Error reading market values from database!");
#endif
	}
	cargo_maintenance_last_update = get_timer("update_cargo");
	cargo_maintenance_last_delayed_update = get_timer("update_delayed_cargo_prices");
}

int read_cargo()
{
#ifdef __NO_MYSQL__
	std::string error;
	flat_cargo_record record;
	auto result = load_flat_cargo(&record, &error);
	if (result == flat_cargo_result::not_found)
	{
		record = capture_flat_cargo();
		result = save_flat_cargo(record, &error);
	}
	if (result != flat_cargo_result::ok)
	{
		logit(LOG_SHIP, "Error reading flat cargo market values: %s",
		      error.empty() ? "authority failure" : error.c_str());
		return FALSE;
	}
	if (record.revision)
		replace_live_cargo(record);
	return TRUE;
#else

	if (!qry("select type, port_id, cargo_type, modifier from ship_cargo_market_mods"))
	{
		logit(LOG_DEBUG, "read_cargo(): cargo query failed!");
		return FALSE;
	}

	MYSQL_RES *res = mysql_store_result(DB);
	if (!res)
		return FALSE;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
	{
		if (!row[0] || !row[1] || !row[2] || !row[3])
			continue;
		char *type = row[0];
		int port_id = atoi(row[1]);
		int cargo_type = atoi(row[2]);
		float modifier = atof(row[3]);

		if (port_id < 0 || port_id >= NUM_PORTS || cargo_type < 0 ||
		    cargo_type >= NUM_PORTS || !isfinite(modifier) || modifier < 0.0f ||
		    modifier > 1000.0f)
		{
			logit(LOG_DEBUG, "read_cargo(): invalid cargo record: (%s, %d, %d, %f)",
			      type, port_id, cargo_type, modifier);
			continue;
		}

		if (!strcmp(type, "CARGO"))
		{
			ship_cargo_market_mod[port_id][cargo_type] =
				modifier; // BOUNDEDF(get_property("ship.cargo.minPriceMod", 0.0), modifier, get_property("ship.cargo.maxPriceMod", 0.0));
			ship_cargo_market_mod_delayed[port_id][cargo_type] =
				ship_cargo_market_mod[port_id][cargo_type];
		}
		else if (!strcmp(type, "CONTRABAND"))
		{
			ship_contra_market_mod[port_id][cargo_type] =
				modifier; // BOUNDEDF(get_property("ship.contraband.minPriceMod", 0.0), modifier, get_property("ship.contraband.maxPriceMod", 0.0));
		}
	}

	mysql_free_result(res);

	return TRUE;

#endif
}

int write_cargo()
{
#ifdef __NO_MYSQL__
	std::string error;
	if (save_flat_cargo(capture_flat_cargo(), &error) == flat_cargo_result::ok)
		return TRUE;
	logit(LOG_SHIP, "Error writing flat cargo market values: %s",
	      error.empty() ? "authority failure" : error.c_str());
	return FALSE;
#else
	bool own_txn = false;
	if (!sql_in_transaction())
	{
		if (!sql_begin_transaction())
		{
			logit(LOG_DEBUG, "write_cargo(): failed to start transaction");
			return FALSE;
		}
		own_txn = true;
	}

	if (!qry("delete from ship_cargo_market_mods; delete from ship_cargo_prices;"))
	{
		logit(LOG_DEBUG, "write_cargo(): cargo query failed!");
		if (own_txn)
			sql_rollback();
		return FALSE;
	}

	char buffer[MAX_STRING_LENGTH] = { 0 };
	char cargoPrices[MAX_STRING_LENGTH / 4] = { 0 };
	char contrabandPrices[MAX_STRING_LENGTH / 4] = { 0 };
	char cargoMarketMods[MAX_STRING_LENGTH / 4] = { 0 };
	char contrabandMarketMods[MAX_STRING_LENGTH / 4] = { 0 };

	// create 4 separate statements
	snprintf(cargoPrices, ARRAY_SIZE(cargoPrices),
		 "insert into ship_cargo_prices (type, port_id, cargo_type, price) values ");
	snprintf(contrabandPrices, ARRAY_SIZE(contrabandPrices),
		 "insert into ship_cargo_prices (type, port_id, cargo_type, price) values ");
	snprintf(
		cargoMarketMods, ARRAY_SIZE(cargoMarketMods),
		"insert into ship_cargo_market_mods (type, port_id, cargo_type, modifier) values ");
	snprintf(
		contrabandMarketMods, ARRAY_SIZE(contrabandMarketMods),
		"insert into ship_cargo_market_mods (type, port_id, cargo_type, modifier) values ");

	bool isFirst = true;

	for (int port = 0; port < NUM_PORTS; port++)
	{
		char buf[1024];
		for (int type = 0; type < NUM_PORTS; type++)
		{
			int price = port == type ? cargo_sell_price(port) :
						   cargo_buy_price(port, type);

			// insert into prices table
			snprintf(buf, ARRAY_SIZE(buf), "%s('%s', %d, %d, %d)", isFirst ? "" : ",",
				 "CARGO", port, type, price);
			strncat(cargoPrices, buf, ARRAY_SIZE(buf));
			snprintf(buf, ARRAY_SIZE(buf), "%s('%s', %d, %d, %d)", isFirst ? "" : ",",
				 "CONTRABAND", port, type, price);
			strncat(contrabandPrices, buf, ARRAY_SIZE(buf));

			// insert into mods table
			snprintf(buf, ARRAY_SIZE(buf), "%s('%s', %d, %d, %f)", isFirst ? "" : ",",
				 "CARGO", port, type, ship_cargo_market_mod[port][type]);
			strncat(cargoMarketMods, buf, ARRAY_SIZE(buf));
			snprintf(buf, ARRAY_SIZE(buf), "%s('%s', %d, %d, %f)", isFirst ? "" : ",",
				 "CONTRABAND", port, type, ship_contra_market_mod[port][type]);
			strncat(contrabandMarketMods, buf, ARRAY_SIZE(buf));
			isFirst = false;
		}
	}

	// put all the statements into a single buffer
	checked_snprintf(buffer, ARRAY_SIZE(buffer), "%s;%s;%s;%s;", cargoPrices, contrabandPrices,
			 cargoMarketMods, contrabandMarketMods);

	if (!qry(buffer))
	{
		logit(LOG_DEBUG, "write_cargo(): insert query failed!");
		if (own_txn)
			sql_rollback();
		return FALSE;
	}
	sql_clear_results();
	if (own_txn && !sql_commit())
	{
		logit(LOG_DEBUG, "write_cargo(): commit failed");
		sql_rollback();
		return FALSE;
	}
	return TRUE;
#endif
}

/*
- ship.cargo.autoSellAdjustMod
- ship.cargo.autoBuyAdjustMod
- ship.cargo.minPriceMod
- ship.cargo.maxPriceMod
- ship.contraband.autoSellAdjustMod
- ship.contraband.autoBuyAdjustMod
- ship.contraband.minPriceMod
- ship.contraband.maxPriceMod

ship.cargo.autoSellAdjustRate=0.05
ship.cargo.autoBuyAdjustRate=0.05
ship.cargo.sellPriceMod=1.0
ship.cargo.buyPriceMod=1.0
ship.contraband.autoSellAdjustRate=0.05
ship.contraband.autoBuyAdjustRate=0.05
ship.contraband.sellPriceMod=1.0
ship.contraband.buyPriceMod=1.0
*/

static void adjust_cargo_market()
{
	int i, j;

	// all mods auto-balance to 1.0 with time, the farther it from 1.0, the faster it changes
	for (i = 0; i < NUM_PORTS; i++)
	{
		for (j = 0; j < NUM_PORTS; j++)
		{
			if (i == j)
			{
				float cargo_sell_mod = get_property("ship.cargo.sellPriceMod", 1.0);
				if (ship_cargo_market_mod[i][j] < cargo_sell_mod)
				{
					ship_cargo_market_mod[i][j] = MAX(
						cargo_sell_mod,
						ship_cargo_market_mod[i][j] +
							(cargo_sell_mod -
							 ship_cargo_market_mod[i][j] + 0.1) *
								get_property(
									"ship.cargo.autoSellAdjustRate",
									0.05));
				}
				if (ship_cargo_market_mod[i][j] > cargo_sell_mod)
				{
					ship_cargo_market_mod[i][j] = MIN(
						cargo_sell_mod,
						ship_cargo_market_mod[i][j] -
							(ship_cargo_market_mod[i][j] -
							 cargo_sell_mod + 0.1) *
								get_property(
									"ship.cargo.autoSellAdjustRate",
									0.05));
				}

				float contra_sell_mod =
					get_property("ship.contraband.sellPriceMod", 1.0);
				if (ship_contra_market_mod[i][j] < contra_sell_mod)
				{
					ship_contra_market_mod[i][j] = MAX(
						contra_sell_mod,
						ship_contra_market_mod[i][j] +
							(contra_sell_mod -
							 ship_contra_market_mod[i][j] + 0.1) *
								get_property(
									"ship.contraband.autoSellAdjustRate",
									0.05));
				}
				if (ship_contra_market_mod[i][j] > contra_sell_mod)
				{
					ship_contra_market_mod[i][j] = MIN(
						contra_sell_mod,
						ship_contra_market_mod[i][j] -
							(ship_contra_market_mod[i][j] -
							 contra_sell_mod + 0.1) *
								get_property(
									"ship.contraband.autoSellAdjustRate",
									0.05));
				}
			}
			else
			{
				float cargo_buy_mod = get_property("ship.cargo.buyPriceMod", 1.0);
				if (ship_cargo_market_mod[i][j] < cargo_buy_mod)
				{
					ship_cargo_market_mod[i][j] = MAX(
						cargo_buy_mod,
						ship_cargo_market_mod[i][j] +
							(cargo_buy_mod -
							 ship_cargo_market_mod[i][j] + 0.1) *
								get_property(
									"ship.cargo.autoBuyAdjustRate",
									0.05));
				}
				if (ship_cargo_market_mod[i][j] > cargo_buy_mod)
				{
					ship_cargo_market_mod[i][j] = MIN(
						cargo_buy_mod,
						ship_cargo_market_mod[i][j] -
							(ship_cargo_market_mod[i][j] -
							 cargo_buy_mod + 0.1) *
								get_property(
									"ship.cargo.autoBuyAdjustRate",
									0.05));
				}

				float contra_buy_mod =
					get_property("ship.contraband.buyPriceMod", 1.0);
				if (ship_contra_market_mod[i][j] < contra_buy_mod)
				{
					ship_contra_market_mod[i][j] = MAX(
						contra_buy_mod,
						ship_contra_market_mod[i][j] +
							(contra_buy_mod -
							 ship_contra_market_mod[i][j] + 0.1) *
								get_property(
									"ship.contraband.autoBuyAdjustRate",
									0.05));
				}
				if (ship_contra_market_mod[i][j] > contra_buy_mod)
				{
					ship_contra_market_mod[i][j] = MIN(
						contra_buy_mod,
						ship_contra_market_mod[i][j] -
							(ship_contra_market_mod[i][j] -
							 contra_buy_mod + 0.1) *
								get_property(
									"ship.contraband.autoBuyAdjustRate",
									0.05));
				}
			}
		}
	}
}

void update_cargo(bool force)
{
	if (!force && !has_elapsed("update_cargo", get_property("ship.cargo.update.secs", 1800)))
		return;
	debug("update_cargo()");
	adjust_cargo_market();
	if (!write_cargo())
		logit(LOG_SHIP, "Error writing market values!");
	set_timer("update_cargo");
	cargo_maintenance_last_update = time(nullptr);
	cargo_maintenance_work_id = 0;
	cargo_maintenance_value_count = 0;
}

void update_cargo()
{
	update_cargo(false);
}

void update_delayed_cargo_prices()
{
	if (!has_elapsed("update_delayed_cargo_prices",
			 get_property("ship.cargo.updateDelayedPrices.secs", 1800)))
		return;

	debug("update_delayed_cargo_prices()");

	int i, j;

	for (i = 0; i < NUM_PORTS; i++)
	{
		for (j = 0; j < NUM_PORTS; j++)
		{
			ship_cargo_market_mod_delayed[i][j] = ship_cargo_market_mod[i][j];
		}
	}

	set_timer("update_delayed_cargo_prices");
	cargo_maintenance_last_delayed_update = time(nullptr);
}

// this gets run once every minute
void cargo_activity()
{
	update_cargo();
	update_delayed_cargo_prices();
}

#ifdef __NO_MYSQL__
bool flatfile_cargo_maintenance_apply(const int64_t *values, size_t count)
{
	constexpr size_t expected = 3 + NUM_PORTS * NUM_PORTS * 4;
	if (!values || count != expected || values[0] <= 0 || values[0] > INT_MAX ||
	    (values[1] != 0 && values[1] != 1) || (values[2] != 0 && values[2] != 1) ||
	    (!values[1] && !values[2]))
		return false;
	flat_cargo_record record;
	for (int port = 0; port < NUM_PORTS; ++port)
		for (int type = 0; type < NUM_PORTS; ++type)
		{
			const size_t source = 3 + (port * NUM_PORTS + type) * 4;
			const size_t destination = port * NUM_PORTS + type;
			if (values[source] < 0 || values[source] > INT_MAX ||
			    values[source + 1] < 0 || values[source + 1] > INT_MAX ||
			    values[source + 2] < 0 || values[source + 2] > 1000000000LL ||
			    values[source + 3] < 0 || values[source + 3] > 1000000000LL)
				return false;
			record.cargo[destination] = values[source + 2] / 1000000.0f;
			record.contraband[destination] = values[source + 3] / 1000000.0f;
		}
	if (values[1])
	{
		std::string error;
		if (save_flat_cargo(record, &error) != flat_cargo_result::ok)
		{
			logit(LOG_SHIP, "Error writing scheduled flat cargo market values: %s",
			      error.empty() ? "authority failure" : error.c_str());
			return false;
		}
		set_timer("update_cargo", static_cast<int>(values[0]));
	}
	if (values[2])
		set_timer("update_delayed_cargo_prices", static_cast<int>(values[0]));
	return true;
}
#endif

size_t cargo_maintenance_snapshot(uint64_t work_id, int64_t *values, size_t capacity)
{
	constexpr size_t required = 3 + NUM_PORTS * NUM_PORTS * 4;
	if (!values || capacity < required)
		return 0;
	if (cargo_maintenance_work_id == work_id && cargo_maintenance_value_count == required)
	{
		memcpy(values, cargo_maintenance_values, sizeof(cargo_maintenance_values));
		return required;
	}
	const int now = time(nullptr);
	const bool cargo_due =
		now > cargo_maintenance_last_update + get_property("ship.cargo.update.secs", 1800);
	const bool delayed_due = now >
				 cargo_maintenance_last_delayed_update +
					 get_property("ship.cargo.updateDelayedPrices.secs", 1800);
	if (!cargo_due && !delayed_due)
		return 0;
	if (cargo_due)
		adjust_cargo_market();
	cargo_maintenance_values[0] = now;
	cargo_maintenance_values[1] = cargo_due ? 1 : 0;
	cargo_maintenance_values[2] = delayed_due ? 1 : 0;
	size_t offset = 3;
	for (int port = 0; port < NUM_PORTS; ++port)
		for (int type = 0; type < NUM_PORTS; ++type)
		{
			if (delayed_due)
				ship_cargo_market_mod_delayed[port][type] =
					ship_cargo_market_mod[port][type];
			cargo_maintenance_values[offset++] =
				port == type ? cargo_sell_price(port) : cargo_buy_price(port, type);
			cargo_maintenance_values[offset++] = port == type ?
								     contra_sell_price(port) :
								     contra_buy_price(port, type);
			cargo_maintenance_values[offset++] = static_cast<int64_t>(
				llround(ship_cargo_market_mod[port][type] * 1000000.0));
			cargo_maintenance_values[offset++] = static_cast<int64_t>(
				llround(ship_contra_market_mod[port][type] * 1000000.0));
		}
	cargo_maintenance_work_id = work_id;
	cargo_maintenance_value_count = offset;
	memcpy(values, cargo_maintenance_values, sizeof(cargo_maintenance_values));
	return cargo_maintenance_value_count;
}

void cargo_maintenance_complete(uint64_t work_id, bool success)
{
	if (work_id != cargo_maintenance_work_id)
		return;
	if (success)
	{
		if (cargo_maintenance_values[1])
			cargo_maintenance_last_update = cargo_maintenance_values[0];
		if (cargo_maintenance_values[2])
			cargo_maintenance_last_delayed_update = cargo_maintenance_values[0];
	}
	cargo_maintenance_work_id = 0;
	cargo_maintenance_value_count = 0;
}

void calculate_port_distances()
{
	logit(LOG_SHIP, "Calculating distances between ports...");

	char line[MAX_STRING_LENGTH];
	char buff[MAX_STRING_LENGTH];
	strcat(line, "       ");

	for (int i = 0; i < NUM_PORTS; i++)
	{
		sprintf(buff, "%6s ", string(ports[i].loc_name).substr(0, 6).c_str());
		strcat(line, buff);
	}

	strcat(line, "MIN    MAX");

	logit(LOG_SHIP, "%s", line);
	line[0] = '\0';

	int global_min_route[3] = { 10000, 0, 0 };

	int global_max_route[3] = { 0, 0, 0 };

	float avg_distance = 0;
	int count = 0;

	for (int i = 0; i < NUM_PORTS; i++)
	{
		int min_port = 0;
		int min_dist = 10000;
		int max_port = 0;
		int max_dist = -1;

		sprintf(buff, "%6s ", string(ports[i].loc_name).substr(0, 6).c_str());
		strcat(line, buff);

		for (int j = 0; j < NUM_PORTS; j++)
		{
			if (i == j)
			{
				strcat(line, "     0,");
				continue;
			}

			vector<int> route;

			bool found_path = dijkstra(real_room0(ports[i].ocean_map_room),
						   real_room0(ports[j].ocean_map_room),
						   valid_ship_edge, route);

			if (found_path)
			{
				int dist = (int)route.size();
				dist = (dist + 250) / 2;

				if (dist < min_dist)
				{
					min_dist = dist;
					min_port = j;
				}
				else if (dist > max_dist)
				{
					max_dist = dist;
					max_port = j;
				}

				sprintf(buff, "%6d,", dist);
				strcat(line, buff);

				avg_distance += (float)dist;
				count++;
			}
			else
			{
				strcat(line, "     ? ");
			}
		}

		sprintf(buff, "%6s %6s", string(ports[min_port].loc_name).substr(0, 6).c_str(),
			string(ports[max_port].loc_name).substr(0, 6).c_str());
		strcat(line, buff);

		if (min_dist < global_min_route[0])
		{
			global_min_route[0] = min_dist;
			global_min_route[1] = i;
			global_min_route[2] = min_port;
		}
		else if (max_dist > global_max_route[0])
		{
			global_max_route[0] = max_dist;
			global_max_route[1] = i;
			global_max_route[2] = max_port;
		}

		logit(LOG_SHIP, "%s", line);
		line[0] = '\0';
	}

	logit(LOG_SHIP, "Shortest route: %s <-> %s (%d)", ports[global_min_route[1]].loc_name,
	      ports[global_min_route[2]].loc_name, global_min_route[0]);
	logit(LOG_SHIP, "Longest route: %s <-> %s (%d)", ports[global_max_route[1]].loc_name,
	      ports[global_max_route[2]].loc_name, global_max_route[0]);
	logit(LOG_SHIP, "Num routes: %d, Average distance: %d", count / 2,
	      (int)avg_distance / count);
}

// i.e. the price the port charges to sell its cargo
int cargo_sell_price(int location, bool delayed)
{
	// the port sells its own cargo at just base price * market mod
	if (delayed)
	{
		return (int)(1000 * cargo_location_data[location].base_cost_cargo *
			     ship_cargo_market_mod_delayed[location][location]);
	}
	else
	{
		return (int)(1000 * cargo_location_data[location].base_cost_cargo *
			     ship_cargo_market_mod[location][location]);
	}
}

// i.e. the price the port will pay to buy cargo
int cargo_buy_price(int location, int type, bool delayed)
{
	if (location == type)
		return cargo_sell_price(location, delayed) * 0.5;
	// Adding a 1.5 multiplier for cargo being sold to account for gem mines. 01/04/2015
	if (delayed)
	{
		return (int)(1500 * cargo_location_data[type].base_cost_cargo *
			     (cargo_location_mod[location][type] / 100.0) *
			     ship_cargo_market_mod_delayed[location][type]);
	}
	else
	{
		return (int)(1500 * cargo_location_data[type].base_cost_cargo *
			     (cargo_location_mod[location][type] / 100.0) *
			     ship_cargo_market_mod[location][type]);
	}
}

// i.e. the price the port charges to sell its contraband
int contra_sell_price(int location)
{
	// the port sells its own contraband at just base price * market mod
	return (int)(1000 * cargo_location_data[location].base_cost_contra *
		     ship_contra_market_mod[location][location]);
}

// i.e. the price the port will pay to buy contraband
int contra_buy_price(int location, int type)
{
	if (location == type)
		return contra_sell_price(location) * 0.5;
	else
		// Adding a 1.5 multiplier for cargo being sold to account for gem mines. 01/04/2015
		return (int)(1500 * cargo_location_data[type].base_cost_contra *
			     (cargo_location_mod[location][type] / 100.0) *
			     ship_contra_market_mod[location][type]);
}

void adjust_ship_market(int transaction, int location, int type, int volume)
{
	if (transaction == SOLD_CARGO)
	{
		// player sold cargo, so adjust market price downwards slightly
		ship_cargo_market_mod[location][type] =
			ship_cargo_market_mod[location][type] *
			(1.0 - get_property("ship.cargo.sellAdjustMod", 0.005) * volume);
	}
	else if (transaction == BOUGHT_CARGO)
	{
		// player bought cargo, so adjust market price upwards slightly
		ship_cargo_market_mod[location][type] =
			ship_cargo_market_mod[location][type] *
			(1.0 + get_property("ship.cargo.buyAdjustMod", 0.003) * volume);
	}
	else if (transaction == SOLD_CONTRA)
	{
		// player sold contraband, so adjust market price downwards slightly
		ship_contra_market_mod[location][type] =
			ship_contra_market_mod[location][type] *
			(1.0 - get_property("ship.contraband.sellAdjustMod", 0.025) * volume);
	}
	else if (transaction == BOUGHT_CONTRA)
	{
		// player bought contraband, so adjust market price upwards slightly
		ship_contra_market_mod[location][type] =
			ship_contra_market_mod[location][type] *
			(1.0 + get_property("ship.contraband.buyAdjustMod", 0.015) * volume);
	}

	if (!write_cargo())
	{
		logit(LOG_SHIP, "Error writing market values!");
	}
}

int required_ship_frags_for_contraband(int type)
{
	return cargo_location_data[type].required_frags;
}

bool can_buy_contraband(P_ship ship, int type)
{
	int frags = required_ship_frags_for_contraband(type);
	if (ship->frags >= frags)
		return true;
	if (ship->crew.sail_skill >= frags * 4 && ship->crew.guns_skill >= frags * 1 &&
	    ship->crew.rpar_skill >= frags * 2)
	{
		return true;
	}
	return false;
}

const char *cargo_type_name(int type)
{
	if (type < 0 || type >= NUM_PORTS)
		return "";

	return cargo_name[type];
}

const char *contra_type_name(int type)
{
	if (type < 0 || type >= NUM_PORTS)
		return "";

	return contra_name[type];
}

void show_cargo_prices(P_char ch)
{
	char line[MAX_STRING_LENGTH];
	char buff[MAX_STRING_LENGTH];

	bool delayed = (bool)!IS_TRUSTED(ch);

	send_to_char(
		"&+y/--------------------------------------------------------------------------------------------\\\r\n",
		ch);

	line[0] = '\0';
	strcat(line, "&+y|&n                    ");

	// Port names
	for (int i = 0; i < NUM_PORTS; i++)
	{
		sprintf(buff, "%6s ", string(ports[i].loc_name).substr(0, 6).c_str());
		strcat(line, buff);
	}
	strcat(line, " &+y|\r\n");
	send_to_char(line, ch);

	for (int type = 0; type < NUM_PORTS; type++)
	{
		line[0] = '\0';

		sprintf(buff, "&+y| %s ", pad_ansi(cargo_type_name(type), 18).c_str());
		strcat(line, buff);

		for (int port = 0; port < NUM_PORTS; port++)
		{
			if (type == port)
			{
				// show sell price
				sprintf(buff, "&+W%6.2f",
					(float)cargo_sell_price(port, delayed) / 1000);
			}
			else if (cargo_buy_price(port, type, delayed) >
				 cargo_sell_price(type, delayed))
			{
				sprintf(buff, "&+g%6.2f",
					(float)cargo_buy_price(port, type, delayed) / 1000);
			}
			else if (cargo_buy_price(port, type, delayed) <
				 cargo_sell_price(type, delayed))
			{
				sprintf(buff, "&+r%6.2f",
					(float)cargo_buy_price(port, type, delayed) / 1000);
			}
			else
			{
				sprintf(buff, "%6.2f",
					(float)cargo_buy_price(port, type, delayed) / 1000);
			}

			strcat(line, pad_ansi(buff, 6).c_str());
			strcat(line, " ");
		}

		strcat(line, " &+y|\r\n");
		send_to_char(line, ch);
	}

	send_to_char(
		"&+y|                                                                                            &+y|\r\n",
		ch);
	send_to_char(
		"&+y| &nAll prices in platinum per crate, and are current within the last hour.                    &+y|\r\n",
		ch);
	send_to_char(
		"&+y\\--------------------------------------------------------------------------------------------/\r\n",
		ch);
}

void show_contra_prices(P_char ch)
{
	char line[MAX_STRING_LENGTH];
	char buff[MAX_STRING_LENGTH];

	send_to_char(
		"&+yCurrent contraband market:\r\n"
		"&+y---------------------------------------------------------------------------------------------------------\r\n",
		ch);

	line[0] = '\0';
	strcat(line, "                          ");

	// Port names
	for (int i = 0; i < NUM_PORTS; i++)
	{
		sprintf(buff, "%7s ", string(ports[i].loc_name).substr(0, 7).c_str());
		strcat(line, buff);
	}
	strcat(line, "\r\n");
	send_to_char(line, ch);

	for (int type = 0; type < NUM_PORTS; type++)
	{
		line[0] = '\0';

		sprintf(buff, "%s ", pad_ansi(contra_type_name(type), 25).c_str());
		strcat(line, buff);

		for (int port = 0; port < NUM_PORTS; port++)
		{
			if (type == port)
			{
				// show sell price
				sprintf(buff, "&+W%7.2f", (float)contra_sell_price(port) / 1000);
			}
			else if (contra_buy_price(port, type) > contra_sell_price(type))
			{
				sprintf(buff, "&+g%7.2f",
					(float)contra_buy_price(port, type) / 1000);
			}
			else if (contra_buy_price(port, type) < contra_sell_price(type))
			{
				sprintf(buff, "&+r%7.2f",
					(float)contra_buy_price(port, type) / 1000);
			}
			else
			{
				sprintf(buff, "%7.2f", (float)contra_buy_price(port, type) / 1000);
			}

			strcat(line, pad_ansi(buff, 6).c_str());
			strcat(line, " ");
		}

		strcat(line, "\r\n");
		send_to_char(line, ch);
	}

	send_to_char("\r\nAll prices in platinum per crate.\r\n", ch);
}

int ship_cargo_info_stick(P_obj obj, P_char ch, int cmd, char *arg)
{
	ShipVisitor svs;

	/*
	 check for periodic event calls
	 */
	if (cmd == CMD_SET_PERIODIC)
		return FALSE;

	if (!obj || !ch)
		return (FALSE);

	if (!OBJ_WORN_POS(obj, HOLD))
		return (FALSE);

	if (!(IS_TRUSTED(ch)))
		return (FALSE);

	if (arg && (cmd == CMD_LOOK))
	{
		if (isname(arg, "cargo"))
		{
			show_cargo_prices(ch);
			return TRUE;
		}
		if (isname(arg, "ships"))
		{
			act("&+yListing &+YALL ships &+yin game:&n", FALSE, ch, obj, obj, TO_CHAR);
			// LOOP through all ships
			for (bool fn = shipObjHash.get_first(svs); fn;
			     fn = shipObjHash.get_next(svs))
			{
				send_to_char_f(ch, "&+yShip:&+C %s&+y Owner: &+C%s ",
					       SHIP_NAME(svs), SHIP_OWNER(svs));
				send_to_char_f(ch, "&+yRoom: %s ", world[svs->location].name);
				if (SHIP_DOCKED(svs))
				{
					send_to_char_f(ch, "&+y | &+LDOCKED&+y");
				}
				if (SHIP_ANCHORED(svs))
				{
					send_to_char_f(ch, "&+y | &+YANCHORED&+y");
				}
				if (SHIP_IMMOBILE(svs))
				{
					send_to_char_f(ch, "&+y | &+rIMMOBILE&+y");
				}
				if (SHIP_SINKING(svs))
				{
					send_to_char_f(ch, "&+y | &+RSINKING&+y");
				}
				send_to_char_f(ch, "\n");
			}
			return TRUE;
		}
	}

	return FALSE;
}

void do_world_cargo(P_char ch, char *arg)
{
	if (!ch)
		return;

	if (!arg || !*arg)
	{
		show_cargo_prices(ch);
		send_to_char("\r\n", ch);
		show_contra_prices(ch);
	}
	else if (is_abbrev(arg, "reload"))
	{
		send_to_char("Reloading cargo mods from persistent storage...\r\n", ch);
		if (!read_cargo())
			send_to_char("FAILED!\r\n", ch);
	}
	else if (is_abbrev(arg, "reset"))
	{
		send_to_char("Resetting cargo mods...\r\n", ch);
		reset_cargo();
	}
	else if (is_abbrev(arg, "save"))
	{
		send_to_char("Writing cargo mods to persistent storage...\r\n", ch);
		if (!write_cargo())
			send_to_char("FAILED!\r\n", ch);
	}
	else if (is_abbrev(arg, "update"))
	{
		send_to_char("Updating cargo prices...\r\n", ch);
		update_cargo(true);
	}
}
