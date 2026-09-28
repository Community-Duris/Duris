#ifndef DURIS_SQL_SPELLBOOK_H_INCLUDED
#define DURIS_SQL_SPELLBOOK_H_INCLUDED

// Conjuration spellbook data stores learned mob vnums, outside player snapshots.
bool sql_add_spellbook_mob(int pid, int mob_vnum);
bool sql_remove_spellbook_mob(int pid, int mob_vnum);
bool sql_has_spellbook_mob(int pid, int mob_vnum);
int *sql_get_spellbook_mobs(int pid, int *count);
bool sql_delete_spellbook_mobs(int pid);

#endif // DURIS_SQL_SPELLBOOK_H_INCLUDED
