#ifndef DURIS_FLATFILE_SHOP_TRADE_REPOSITORY_H
#define DURIS_FLATFILE_SHOP_TRADE_REPOSITORY_H

#include "persistence/critical_command_coordinator.h"

#include <string>

critical_apply_result flatfile_shop_trade_repository_apply(const std::string &root,
							   const critical_command &command);

// Read-only validation of the complete replay catalog, including lazy entries.
bool flatfile_shop_trade_repository_validate(const std::string &root, std::string *error);

// Private native receipt staging only; original schema-1 apply remains intact.
// Caller must hold the existing authority lock. No lock is acquired here.
#include "flatfile/flatfile_authority_transaction.h"
#include <span>
class flatfile_shop_trade_storage
{
	friend class flatfile_accounting_shop_transaction;
	static unsigned int lookup_locked(const std::string &, const flatfile_authority_lock &,
					  const critical_command &, critical_apply_result *,
					  std::string *) noexcept;
	static unsigned int stage_locked(const std::string &, const flatfile_authority_lock &,
					 const critical_command &, unsigned int result_code,
					 std::span<const uint8_t> result,
					 std::vector<flatfile_authority_operation> *,
					 std::string *) noexcept;
};
#endif
