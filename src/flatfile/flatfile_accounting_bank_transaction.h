#ifndef DURIS_FLATFILE_ACCOUNTING_BANK_TRANSACTION_H
#define DURIS_FLATFILE_ACCOUNTING_BANK_TRANSACTION_H
#include "persistence/critical_command_completion.h"
#include <string>

// Typed root ATM owner: locks/recovery, retained replay, native effect and
// evidence commit, followed by locked readback. Does not open gameplay admission;
// baseline/coverage lifecycle and global legacy receipt fencing are prerequisites.
class flatfile_accounting_bank_transaction
{
    public:
	static critical_apply_result apply(const std::string &root,
					   const critical_command &command);
};
#endif
