#ifndef PERSISTENCE_MODE_H
#define PERSISTENCE_MODE_H

#include <stdbool.h>
#include <stddef.h>

enum persistence_mode
{
	PERSISTENCE_MODE_MARIADB_PRIMARY = 0,
	PERSISTENCE_MODE_MARIADB_PRIMARY_FLATFILE_FALLBACK,
	PERSISTENCE_MODE_FLATFILE_PRIMARY
};

bool persistence_mode_configure(char *error, size_t error_size);
enum persistence_mode persistence_mode_get(void);
const char *persistence_mode_name(void);
bool persistence_mode_requires_mysql(void);
const char *persistence_mode_flatfile_root(void);

#endif
