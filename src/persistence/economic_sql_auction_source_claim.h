#ifndef DURIS_ECONOMIC_SQL_AUCTION_SOURCE_CLAIM_H
#define DURIS_ECONOMIC_SQL_AUCTION_SOURCE_CLAIM_H

#include "economy/economic_accounting_intent.h"
#include "economy/auction_accounting.h"
#include "economy/auction_item_claim_accounting.h"
#include "economy/auction_listing_accounting.h"
#include "economy/auction_money_claim_accounting.h"
#include "economy/auction_settlement_accounting.h"
#include "persistence/economic_accounting_repository.h"
#include <cerrno>

#ifndef __NO_MYSQL__
#include <charconv>
#include <memory>
#include <new>
#include <string>
#include <type_traits>

namespace economic_sql_auction_source_claim_detail
{
inline unsigned int sql_error(MYSQL *db)
{
	const auto error = mysql_errno(db);
	return error ? error : static_cast<unsigned int>(EIO);
}

inline std::string hex(std::span<const uint8_t> bytes)
{
	static constexpr char digits[] = "0123456789abcdef";
	std::string result = "X'";
	result.reserve(bytes.size() * 2 + 3);
	for (const auto byte : bytes)
	{
		result += digits[byte >> 4];
		result += digits[byte & 15];
	}
	return result + "'";
}

struct session
{
	MYSQL *db;
	unsigned long id = 0;
	unsigned int check(bool initial = false)
	{
		if (!db)
			return EINVAL;
		if (!(db->server_status & SERVER_STATUS_IN_TRANS))
			return initial ? EINVAL : ENOTCONN;
		using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
		flag reconnect = false;
		if (mysql_get_option(db, MYSQL_OPT_RECONNECT, &reconnect))
			return sql_error(db);
		if (reconnect)
			return EPERM;
		if (initial)
			id = mysql_thread_id(db);
		return !id || mysql_thread_id(db) != id ? ENOTCONN : 0;
	}
};

inline unsigned int count(MYSQL *db, const std::string &table, const std::string &where,
			  uint64_t expected)
{
	const auto sql = "SELECT COUNT(*) FROM " + table + " WHERE " + where;
	if (mysql_real_query(db, sql.data(), sql.size()))
		return sql_error(db);
	std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> rows(mysql_store_result(db),
								      &mysql_free_result);
	if (!rows)
		return sql_error(db);
	if (mysql_num_fields(rows.get()) != 1 || mysql_num_rows(rows.get()) != 1)
		return EILSEQ;
	const auto row = mysql_fetch_row(rows.get());
	const auto lengths = row ? mysql_fetch_lengths(rows.get()) : nullptr;
	if (!row || !lengths || !row[0] || !lengths[0])
		return mysql_errno(db) ? sql_error(db) : static_cast<unsigned int>(EILSEQ);
	uint64_t value = 0;
	const auto parsed = std::from_chars(row[0], row[0] + lengths[0], value);
	return parsed.ec != std::errc{} || parsed.ptr != row[0] + lengths[0] || value != expected ?
		       EILSEQ :
		       0;
}

inline bool policy(const economic_operation_metadata &meta)
{
	if (!meta.source_event || meta.actor_kind != economic_actor_kind::domain ||
	    !meta.actor_id || critical_operation_id_is_zero(meta.operation_id) ||
	    critical_operation_id_is_zero(meta.lineage) ||
	    critical_operation_id_is_zero(meta.epoch))
		return false;
	const bool listing = meta.writer_id == ECONOMIC_WRITER_AUCTION_LISTING &&
			     meta.reason == economic_reason::auction_listing;
	if (critical_operation_id_is_zero(meta.original_operation_id) != listing)
		return false;
	if (meta.writer_id == ECONOMIC_WRITER_AUCTION_MONEY_CLAIM &&
	    meta.reason == economic_reason::auction_claim)
		return meta.source_event->kind == economic_source_kind::service;
	return meta.source_event->kind == economic_source_kind::auction &&
	       (listing ||
		(meta.writer_id == ECONOMIC_WRITER_AUCTION_BID &&
		 meta.reason == economic_reason::auction_bid) ||
		(meta.writer_id == ECONOMIC_WRITER_AUCTION_ITEM_CLAIM &&
		 meta.reason == economic_reason::auction_claim) ||
		(meta.writer_id == ECONOMIC_WRITER_AUCTION_SETTLEMENT &&
		 (meta.reason == economic_reason::auction_settle ||
		  meta.reason == economic_reason::auction_cancel)));
}

// The typed owner supplies already authenticated original EAI. This comparison
// never reconstructs an unobserved command header or consults current native state.
inline unsigned int apply(MYSQL *db, const economic_frozen_intent &intent, uint32_t code,
			  bool append)
{
	session guard{ db };
	auto error = guard.check(true);
	if (error)
		return error;
	const auto &meta = intent.admission.metadata;
	if (!policy(meta) || (append && code))
		return EILSEQ;
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
	std::vector<uint8_t> canonical;
	economic_digest digest{};
	for (const auto status : { economic_source_event_encode(*meta.source_event, &source),
				   economic_intent_encode(intent, &canonical),
				   economic_intent_digest(intent, &digest) })
		if (status != economic_accounting_error::ok)
			return status == economic_accounting_error::capacity ? ENOMEM : EILSEQ;
	const auto operation = "operation_id=" + hex(meta.operation_id.bytes);
	const auto identity = operation + " AND lineage=" + hex(meta.lineage.bytes) +
			      " AND source_event=" + hex(source) + " AND outcome=1";
	const auto root =
		operation + " AND lineage=" + hex(meta.lineage.bytes) +
		" AND epoch=" + hex(meta.epoch.bytes) +
		(critical_operation_id_is_zero(meta.original_operation_id) ?
			 " AND original_operation_id IS NULL" :
			 " AND original_operation_id=" + hex(meta.original_operation_id.bytes)) +
		" AND accounting_version=" + std::to_string(meta.version) +
		" AND writer_id=" + std::to_string(meta.writer_id) +
		" AND policy_version=" + std::to_string(meta.policy_version) +
		" AND compiler_version=" + std::to_string(meta.compiler_version) +
		" AND actor_kind=" + std::to_string(static_cast<unsigned>(meta.actor_kind)) +
		" AND actor_id=" + std::to_string(meta.actor_id) +
		" AND reason=" + std::to_string(static_cast<unsigned>(meta.reason)) +
		" AND source_event=" + hex(source) + " AND intent_digest=" + hex(digest) +
		" AND domain_digest=" + hex(intent.domain_digest) +
		" AND canonical_intent=" + hex(canonical) +
		" AND outcome=" + std::to_string(code ? 2 : 1) +
		" AND result_code=" + std::to_string(code);
	error = count(db, "economic_accounting_operation", root, 1);
	if (!error && append)
	{
		const auto sql =
			"INSERT INTO economic_accounting_source_claim(lineage,source_event,"
			"operation_id,outcome) SELECT lineage,source_event,operation_id,outcome "
			"FROM economic_accounting_operation WHERE " +
			root;
		if (mysql_real_query(db, sql.data(), sql.size()))
		{
			error = sql_error(db);
			if (error == 1062)
				error = EEXIST;
		}
		else if (mysql_affected_rows(db) != 1)
			error = EILSEQ;
	}
	if (!error && !code)
		error = count(db, "economic_accounting_source_claim", identity, 1);
	if (!error)
		error = count(db, "economic_accounting_source_claim", operation, code ? 0 : 1);
	const auto session_error = guard.check();
	return session_error ? session_error : error;
}
}
#endif

// Caller already authenticated the original canonical EAI/root/inbox. Only the
// observable source identity is compared; no unseen header or time is invented.
inline unsigned int economic_sql_auction_source_claim_verify_known_metadata(
	MYSQL *db, const economic_operation_metadata &meta, uint32_t original_result_code) noexcept
{
#ifdef __NO_MYSQL__
	(void)db;
	(void)meta;
	(void)original_result_code;
	return ENOTSUP;
#else
	using namespace economic_sql_auction_source_claim_detail;
	try
	{
		session guard{ db };
		auto error = guard.check(true);
		if (error)
			return error;
		if (!policy(meta))
			return EILSEQ;
		std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source{};
		const auto status = economic_source_event_encode(*meta.source_event, &source);
		if (status != economic_accounting_error::ok)
			return status == economic_accounting_error::capacity ? ENOMEM : EILSEQ;
		const auto operation = "operation_id=" + hex(meta.operation_id.bytes);
		if (!original_result_code)
			error = count(db, "economic_accounting_source_claim",
				      operation + " AND lineage=" + hex(meta.lineage.bytes) +
					      " AND source_event=" + hex(source) + " AND outcome=1",
				      1);
		if (!error)
			error = count(db, "economic_accounting_source_claim", operation,
				      original_result_code ? 0 : 1);
		const auto session_error = guard.check();
		return session_error ? session_error : error;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

// Fresh successful recording only. All five leaf owners call this after their
// root insert; every error requires rollback of the caller's whole transaction.
inline unsigned int
economic_sql_auction_source_claim_record(MYSQL *db, const critical_command &command,
					 const economic_frozen_intent &intent) noexcept
{
#ifdef __NO_MYSQL__
	(void)db;
	(void)command;
	(void)intent;
	return ENOTSUP;
#else
	try
	{
		if (command.type != critical_command_type::auction ||
		    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
			return EPROTONOSUPPORT;
		const auto status = economic_intent_verify_binding(command, intent);
		if (status != economic_accounting_error::ok)
			return status == economic_accounting_error::capacity ? ENOMEM : EILSEQ;
		return economic_sql_auction_source_claim_detail::apply(db, intent, 0, true);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

// Read-only original metadata proof, after the caller's canonical root/inbox
// verification. An unknown historical command header remains unobserved.
inline unsigned int economic_sql_auction_source_claim_verify(MYSQL *db,
							     const economic_frozen_intent &intent,
							     uint32_t original_result_code) noexcept
{
#ifdef __NO_MYSQL__
	(void)db;
	(void)intent;
	(void)original_result_code;
	return ENOTSUP;
#else
	try
	{
		return economic_sql_auction_source_claim_detail::apply(db, intent,
								       original_result_code, false);
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
#endif
}

#endif
