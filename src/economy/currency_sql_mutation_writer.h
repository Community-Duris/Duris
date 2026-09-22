#ifndef DURIS_CURRENCY_SQL_MUTATION_WRITER_H
#define DURIS_CURRENCY_SQL_MUTATION_WRITER_H
#include "economy/currency_command.h"
#include <mysql/mysql.h>
class economic_sql_bank_transaction;
// Shared legacy currency writer. Its accounting entry is available only to the
// typed transaction after locked preparation. It never grants an audit receipt.
class currency_sql_mutation_writer
{
    private:
	static unsigned int write(MYSQL *, const critical_command &,
				  const currency_prepared_mutation &, uint32_t bank_id);
	friend class economic_sql_bank_transaction;
};
#endif
