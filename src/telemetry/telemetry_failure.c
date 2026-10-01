#include "telemetry/telemetry_failure.h"

telemetry_failure_class telemetry_classify_sql_failure(std::uint32_t error_code,
						       telemetry_sql_phase phase) noexcept
{
	if (phase == telemetry_sql_phase::commit)
		return telemetry_failure_class::commit_ambiguous;

	switch (error_code)
	{
	/* Deadlock and lock-wait timeout are safe whole-transaction retries. */
	case 1205U:
	case 1213U:
		return telemetry_failure_class::transient_transaction;

	/* Client/server disconnects and bounded server-capacity failures. */
	case 1040U:
	case 1042U:
	case 1053U:
	case 1158U:
	case 1159U:
	case 1160U:
	case 1161U:
	case 1203U:
	case 1317U:
	case 2002U:
	case 2003U:
	case 2006U:
	case 2013U:
	case 2055U:
	case 3024U:
		return telemetry_failure_class::transient_connection;

	/* Schema/query-shape defects cannot heal through immediate replay. */
	case 1049U:
	case 1054U:
	case 1064U:
	case 1146U:
	case 1149U:
	case 1193U:
	case 1305U:
		return telemetry_failure_class::permanent_schema;

	/* Authentication and object/operation grants need operator action. */
	case 1044U:
	case 1045U:
	case 1142U:
	case 1143U:
	case 1227U:
	case 1370U:
		return telemetry_failure_class::permanent_permission;

	/* A single represented value violates the deployed storage contract. */
	case 1048U:
	case 1264U:
	case 1265U:
	case 1292U:
	case 1366U:
	case 1406U:
	case 1452U:
	case 3819U:
		return telemetry_failure_class::invalid_record;

	default:
		/* A connect failure may not have a server error code yet. */
		return phase == telemetry_sql_phase::connect ?
			       telemetry_failure_class::transient_connection :
			       telemetry_failure_class::permanent_repository;
	}
}
