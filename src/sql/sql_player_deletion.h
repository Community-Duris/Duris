#ifndef DURIS_SQL_PLAYER_DELETION_H_INCLUDED
#define DURIS_SQL_PLAYER_DELETION_H_INCLUDED

class economic_sql_currency_writer_guard;

// SQL character deletion guard. Must be called immediately after beginning the
// deletion transaction, before its first consistent read; the PID row lock is
// held through commit. It checks retained unresolved evidence without locking it.
// Acquire the supplied opaque writer lease on the same session before BEGIN.
bool sql_player_deletion_guard(int pid, const economic_sql_currency_writer_guard &writer);

// Transaction owners defer revision eviction until their commit is confirmed.
// Own transactions acquire a lease; outer owners must supply their held lease.
bool sql_delete_player(int pid, bool forget_revision = true,
		       const economic_sql_currency_writer_guard *writer = nullptr);

#endif // DURIS_SQL_PLAYER_DELETION_H_INCLUDED
