#include "flatfile/flatfile_accounting_dispatch.h"
#include "flatfile/flatfile_accounting_bank_transaction.h"
#include "flatfile/flatfile_item_repository.h"
#include "persistence/persistence_mode.h"
#include <cassert>
#include <cerrno>
#include <iostream>

const char *configured_root = nullptr;
unsigned legacy_calls = 0, bank_calls = 0;
critical_command expected;
std::string expected_root;
void *expected_context = nullptr;
critical_apply_result forwarded = {};
const char *persistence_mode_flatfile_root()
{
	return configured_root;
}
critical_apply_result
flatfile_critical_command_repository_apply_selected(const critical_command &command, void *context)
{
	assert(&command == &expected && context == expected_context);
	++legacy_calls;
	return forwarded;
}
critical_apply_result flatfile_accounting_bank_transaction::apply(const std::string &root,
								  const critical_command &command)
{
	assert(&command == &expected && root == expected_root);
	++bank_calls;
	return forwarded;
}
void check_result(critical_apply_result actual)
{
	assert(actual.outcome == forwarded.outcome &&
	       actual.durable_revision == forwarded.durable_revision &&
	       actual.error_code == forwarded.error_code &&
	       actual.failure_stage == forwarded.failure_stage &&
	       actual.result_size == forwarded.result_size &&
	       actual.result_payload == forwarded.result_payload);
}
int main()
{
	forwarded = { critical_apply_outcome::ambiguous_commit, 123, EIO };
	forwarded.failure_stage = critical_failure_stage::coin_source_wallet_revision;
	forwarded.result_size = forwarded.result_payload.size();
	for (size_t i = 0; i < forwarded.result_payload.size(); ++i)
		forwarded.result_payload[i] = static_cast<uint8_t>(i);
	char explicit_root[] = "synthetic-explicit-root";
	expected.type = critical_command_type::coin_transfer;
	for (uint16_t version : { 0, 1, 3 })
	{
		expected.schema_version = version;
		expected_context = nullptr;
		check_result(flatfile_accounting_apply_selected(expected, nullptr));
		expected_context = explicit_root;
		check_result(flatfile_accounting_apply_selected(expected, explicit_root));
	}
	expected.schema_version = 2;
	expected.type = critical_command_type::account_bank;
	for (const char *root : { static_cast<const char *>(nullptr), "" })
	{
		configured_root = root;
		auto result = flatfile_accounting_apply_selected(expected, nullptr);
		assert(result.outcome == critical_apply_outcome::retryable_failure &&
		       result.error_code == ENOENT);
	}
	configured_root = "synthetic-configured-root";
	expected_root = configured_root;
	check_result(flatfile_accounting_apply_selected(expected, nullptr));
	expected_root = explicit_root;
	check_result(flatfile_accounting_apply_selected(expected, explicit_root));
	char empty_root[] = "";
	assert(flatfile_accounting_apply_selected(expected, empty_root).error_code == ENOENT);
	for (uint16_t type = static_cast<uint16_t>(critical_command_type::test);
	     type <= static_cast<uint16_t>(critical_command_type::player_death_restitution); ++type)
	{
		if (type == static_cast<uint16_t>(critical_command_type::account_bank))
			continue;
		expected.type = static_cast<critical_command_type>(type);
		for (void *context :
		     { static_cast<void *>(nullptr), static_cast<void *>(explicit_root) })
		{
			auto result = flatfile_accounting_apply_selected(expected, context);
			assert(result.outcome == critical_apply_outcome::retryable_failure &&
			       result.error_code == ENOTSUP);
		}
	}
	assert(legacy_calls == 6 && bank_calls == 2);
	std::cout
		<< "flatfile dispatcher: bank-only routing, legacy contexts, root precedence and full completion preservation passed\n";
}
