// Linked only by the scoped gameplay qualifier. Native craft execution and
// recovery are unchanged. The wider #490 activation census is not certified by
// this fixture's synthetic epoch and admission projection.
#define DURIS_ECONOMIC_GAMEPLAY_AUTHORITY_TEST
#include "economy/economic_gameplay_authority.h"

#include <cstdlib>
#include <cstdio>
#include <unistd.h>

#ifndef __NO_MYSQL__
#include <mysql/mysql.h>
MYSQL *sql_open_configured_connection(unsigned long);
#endif

class economic_gameplay_authority_test_access
{
    public:
	static void install()
	{
		critical_operation_id lineage = {}, epoch = {}, receipt = {};
#ifdef __NO_MYSQL__
		lineage.bytes[0] = epoch.bytes[0] = receipt.bytes[0] = 0x51;
		lineage.bytes.back() = 71;
		epoch.bytes.back() = 72;
		receipt.bytes.back() = 74;
#else
		lineage.bytes[0] = 71;
		epoch.bytes[0] = 72;
		receipt.bytes[0] = 74;
#endif
		if (economic_gameplay_authority::install(lineage, epoch, receipt, {}, {}) !=
		    economic_accounting_error::ok)
			std::abort();
	}
};

static bool enabled()
{
	const char *marker = std::getenv("DURIS_551_ACTIVE_FIXTURE");
	return marker && access(marker, F_OK) == 0;
}

// Freeze a selected Encrust failure before command admission. Other random
// draws, including poison notch probabilities, use the production generator.
extern "C" int __real__Z6numberii(int, int);
extern "C" int __wrap__Z6numberii(int low, int high)
{
	const char *failure = std::getenv("DURIS_551_ENCRUST_FAILURE");
	if (enabled() && low == 1 && high == 110 && failure && access(failure, F_OK) == 0)
		return 110;
	return __real__Z6numberii(low, high);
}

extern "C" bool __real__ZN27economic_gameplay_authority6activeEv();
extern "C" bool
__real__Z52critical_command_coordinator_acknowledge_publicationRK21critical_operation_id(
	const critical_operation_id &);
extern "C" bool
__wrap__Z52critical_command_coordinator_acknowledge_publicationRK21critical_operation_id(
	const critical_operation_id &operation)
{
	const char *gate = std::getenv("DURIS_551_PUBLICATION_GATE");
	if (gate && access(gate, F_OK) == 0)
	{
		static bool reported = false;
		if (!reported)
		{
			std::fprintf(stderr, "ISSUE551_FIXTURE_PUBLICATION_REFUSED\n");
			reported = true;
		}
		return false;
	}
	return __real__Z52critical_command_coordinator_acknowledge_publicationRK21critical_operation_id(
		operation);
}
extern "C" bool __wrap__ZN27economic_gameplay_authority6activeEv()
{
#ifdef __NO_MYSQL__
	if (enabled() && !__real__ZN27economic_gameplay_authority6activeEv())
		economic_gameplay_authority_test_access::install();
#endif
	return __real__ZN27economic_gameplay_authority6activeEv();
}

#ifndef __NO_MYSQL__
extern "C" bool __real__Z26sql_economic_runtime_startv();
extern "C" bool __wrap__Z26sql_economic_runtime_startv()
{
	if (!enabled())
		return __real__Z26sql_economic_runtime_startv();
	MYSQL *connection = sql_open_configured_connection(0);
	if (!connection ||
	    mysql_query(
		    connection,
		    "UPDATE economic_lineage_state SET active_epoch=NULL WHERE lineage=UNHEX('47000000000000000000000000000000')"))
		std::abort();
	mysql_close(connection);
	if (!__real__Z26sql_economic_runtime_startv())
		return false;
	connection = sql_open_configured_connection(0);
	if (!connection ||
	    mysql_query(
		    connection,
		    "UPDATE economic_lineage_state SET active_epoch=UNHEX('48000000000000000000000000000000') WHERE lineage=UNHEX('47000000000000000000000000000000')"))
		std::abort();
	mysql_close(connection);
	economic_gameplay_authority_test_access::install();
	return true;
}
#endif
