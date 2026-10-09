#include "flatfile/flatfile_accounting_dispatch.h"
#include "flatfile/flatfile_accounting_bank_transaction.h"
#include "flatfile/flatfile_accounting_coin_transaction.h"
#include "flatfile/flatfile_item_repository.h"
#include "persistence/persistence_mode.h"
#ifdef __NO_MYSQL__
#include "flatfile/flatfile_accounting_shop_transaction.h"
#include "flatfile/flatfile_collector_repository.h"
#include "economy/economic_command_admission.h"
#endif

#include <cerrno>

critical_apply_result flatfile_accounting_apply_selected(const critical_command &command,
							 void *context)
{
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		return flatfile_critical_command_repository_apply_selected(command, context);
	const char *root = context ? static_cast<const char *>(context) :
				     persistence_mode_flatfile_root();
	if (!root || !*root)
		return { critical_apply_outcome::retryable_failure, 0, ENOENT };
	if (command.type == critical_command_type::item_transfer)
		return flatfile_item_repository_apply(root, command);
	if (command.type == critical_command_type::account_bank)
		return flatfile_accounting_bank_transaction::apply(root, command);
	if (command.type == critical_command_type::coin_transfer)
		return flatfile_accounting_coin_transaction::apply(root, command);
#ifdef __NO_MYSQL__
	if (command.type == critical_command_type::shop_trade &&
	    economic_flatfile_command_admission_supported(command))
		return flatfile_accounting_shop_transaction::apply(root, command);
	if (command.type == critical_command_type::collector &&
	    economic_flatfile_command_admission_supported(command))
		return flatfile_collector_repository_apply_accounted(root, command);
#endif
	// Never checkpoint an unsupported durable accounting envelope.
	return { critical_apply_outcome::retryable_failure, 0, ENOTSUP };
}
