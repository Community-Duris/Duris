#include "economy/auction_repository.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace
{
MYSQL *connection;

void execute(const std::string &sql)
{
	if (mysql_query(connection, sql.c_str()))
	{
		fprintf(stderr, "SQL failed: %s\n", mysql_error(connection));
		abort();
	}
}

uint64_t scalar(const std::string &sql)
{
	execute(sql);
	MYSQL_RES *query = mysql_store_result(connection);
	assert(query);
	MYSQL_ROW row = mysql_fetch_row(query);
	assert(row && row[0]);
	const auto value = strtoull(row[0], nullptr, 10);
	mysql_free_result(query);
	return value;
}

void expect_state(uint64_t money, uint64_t status, uint64_t revision, uint64_t claimant,
		  uint64_t ledgers)
{
	assert(scalar("SELECT money FROM auction_money_pickups WHERE pid=22") == money);
	assert(scalar("SELECT status FROM auctions WHERE id=1") == status);
	assert(scalar("SELECT auction_revision FROM auctions WHERE id=1") == revision);
	assert(scalar("SELECT COALESCE(claim_pid,0) FROM auction_item_custody WHERE auction_id=1") ==
	       claimant);
	assert(scalar("SELECT COUNT(*) FROM auction_ledger") == ledgers);
}
} // namespace

int main()
{
	// Deliberately separate commissioning: never read .env or create a database.
	const char *database = getenv("AUCTION_TEST_DB_NAME");
	assert(database && !strncmp(database, "duris_pr717_", 12));
	connection = mysql_init(nullptr);
	assert(connection);
	const char *port = getenv("DB_PORT");
	assert(mysql_real_connect(connection, getenv("DB_HOST"), getenv("DB_USER"),
				  getenv("DB_PASSWD"), database, port ? atoi(port) : 3306, nullptr,
				  0));
	execute("SET SESSION sql_mode='STRICT_ALL_TABLES'");
	// Temporary InnoDB tables isolate the fixture even from other test runs.
	execute("CREATE TEMPORARY TABLE auctions(id INT PRIMARY KEY,seller_pid INT,"
		"winning_bidder_pid INT,status INT,custody_state INT,cur_price BIGINT,"
		"buy_price BIGINT,auction_revision BIGINT UNSIGNED,end_time DATETIME,"
		"winning_bidder_name VARCHAR(32)) ENGINE=InnoDB");
	execute("CREATE TEMPORARY TABLE account_characters(pid INT PRIMARY KEY,"
		"account_name VARCHAR(32)) ENGINE=InnoDB");
	execute("CREATE TEMPORARY TABLE auction_item_custody(auction_id INT PRIMARY KEY,"
		"claim_pid INT NULL,claimed_at DATETIME NULL) ENGINE=InnoDB");
	execute("CREATE TEMPORARY TABLE auction_money_pickups(pid INT PRIMARY KEY,"
		"money INT UNSIGNED,claim_revision BIGINT UNSIGNED) ENGINE=InnoDB");
	execute("CREATE TEMPORARY TABLE auction_ledger(operation_id BINARY(16) PRIMARY KEY,"
		"event_type INT,auction_id INT,auction_revision BIGINT UNSIGNED,actor_pid INT,"
		"counterparty_pid INT,value_delta BIGINT,final_price BIGINT,item_count INT) ENGINE=InnoDB");
	execute("INSERT INTO auctions VALUES(1,11,22,1,1,400,0,7,'2030-01-01','bidder')");
	execute("INSERT INTO account_characters VALUES(11,'seller')");
	execute("INSERT INTO auction_item_custody VALUES(1,NULL,NULL)");
	execute("INSERT INTO auction_money_pickups VALUES(22,100,3)");

	auction_command_payload payload{};
	payload.action = auction_action::remove;
	payload.auction_id = 1;
	critical_operation_id operation{};
	assert(critical_operation_id_generate(&operation));
	critical_command command{};
	assert(auction_command_build(&command, operation, payload,
				     critical_source_site::operator_repair,
				     critical_deadline_class::interactive));
	command.accepted_at_usec = 1;
	auction_command_result result{};
	unsigned int error = 0;
	bool mutated = false;

	// Refund and item return share the enclosing critical-command transaction.
	execute("START TRANSACTION");
	assert(auction_repository_execute(connection, command, &result, &error, &mutated));
	assert(!error && mutated && result.winner_pid == 22 && result.final_price == 400);
	expect_state(500, 3, 8, 11, 1);
	execute("ROLLBACK");
	expect_state(100, 1, 7, 0, 0);

	// A refused refund must not leave the item returned or the auction removed.
	execute("UPDATE auction_money_pickups SET money=4294967295 WHERE pid=22");
	execute("START TRANSACTION");
	assert(!auction_repository_execute(connection, command, &result, &error, &mutated));
	execute("ROLLBACK");
	expect_state(4294967295ULL, 1, 7, 0, 0);
	execute("UPDATE auction_money_pickups SET money=100 WHERE pid=22");

	execute("START TRANSACTION");
	assert(auction_repository_execute(connection, command, &result, &error, &mutated));
	assert(!error && mutated);
	execute("COMMIT");
	expect_state(500, 3, 8, 11, 1);
	assert(scalar("SELECT claim_revision FROM auction_money_pickups WHERE pid=22") == 4);
	// A second removal, even with a fresh operation ID, cannot refund twice.
	assert(critical_operation_id_generate(&command.operation_id));
	execute("START TRANSACTION");
	assert(auction_repository_execute(connection, command, &result, &error, &mutated));
	assert(error == EALREADY && !mutated);
	execute("ROLLBACK");
	expect_state(500, 3, 8, 11, 1);
	mysql_close(connection);
	puts("auction removal refund, rollback, refund failure and duplicate removal passed");
}
