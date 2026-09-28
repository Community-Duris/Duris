#ifndef DURIS_SQL_TRANSACTION_H_INCLUDED
#define DURIS_SQL_TRANSACTION_H_INCLUDED

// Connection-scoped transaction helpers shared by SQL domain owners.
bool sql_begin_transaction(void);
bool sql_commit(void);
bool sql_rollback(void);
bool sql_in_transaction(void);

#endif // DURIS_SQL_TRANSACTION_H_INCLUDED
