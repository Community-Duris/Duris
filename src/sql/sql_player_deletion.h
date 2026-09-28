#ifndef DURIS_SQL_PLAYER_DELETION_H_INCLUDED
#define DURIS_SQL_PLAYER_DELETION_H_INCLUDED

// SQL character deletion guard. Must be called immediately after beginning the
// deletion transaction, before its first consistent read; the PID row lock is
// held through commit. It checks retained unresolved evidence without locking it.
bool sql_player_deletion_guard(int pid);

// Transaction owners defer revision eviction until their commit is confirmed.
bool sql_delete_player(int pid, bool forget_revision = true);

#endif // DURIS_SQL_PLAYER_DELETION_H_INCLUDED
