#ifndef DURIS_SQL_GUILD_H_INCLUDED
#define DURIS_SQL_GUILD_H_INCLUDED

class Guild;

bool sql_save_guild(Guild *guild);
Guild *sql_load_guild(unsigned int guild_id);
bool sql_load_all_guilds(void);
bool sql_delete_guild(unsigned int guild_id);

#endif // DURIS_SQL_GUILD_H_INCLUDED
