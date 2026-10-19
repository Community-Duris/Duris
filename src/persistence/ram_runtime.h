#ifndef DURIS_RAM_RUNTIME_H
#define DURIS_RAM_RUNTIME_H

#include <mysql/mysql.h>
#include <string>

bool ram_runtime_recover(MYSQL *connection, const char *operation_directory,
			 const char *legacy_player_directory, std::string *error);
bool ram_runtime_hydrate(MYSQL *connection, const char *flatfile_root, std::string *error);
bool ram_runtime_start();
void ram_runtime_shutdown();

#endif
