#ifndef DURIS_CURRENCY_FLATFILE_MUTATION_WRITER_H
#define DURIS_CURRENCY_FLATFILE_MUTATION_WRITER_H
#include "economy/currency_command.h"
#include "flatfile/flatfile_authority_transaction.h"

// Only a typed accounting owner may stage the prepared currency effect. No
// commit is performed here and no caller-supplied after-image is accepted.
class currency_flatfile_mutation_writer
{
	friend class flatfile_accounting_bank_transaction;
#ifdef DURIS_FLATFILE_ACCOUNTING_TEST
	friend class flatfile_accounting_test_access;
#endif
	static unsigned int stage(const std::string &, const flatfile_authority_lock &,
				  const critical_command &, const currency_prepared_mutation &,
				  std::vector<flatfile_authority_operation> *, std::string *);
};
#endif
