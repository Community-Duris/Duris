#include "persistence_mode.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static enum persistence_mode active_mode = PERSISTENCE_MODE_MARIADB_PRIMARY;
static const char *active_flatfile_root;

static const char *const flatfile_directories[] = {
	"metadata",	    "identities", "identities/accounts",
	"identities/names", "players",	  "operations",
	"operations/wal",   "domains",	  "manifests",
};

static const char unimplemented_domains[] =
	"player external domain sidecars, character rename/delete completion, "
	"item ownership/ledger, critical operations, lockers/private chests, "
	"corpses/saved items/shopkeepers/pets/shapes/recipes/spellbooks, "
	"guilds/alliances/halls/outposts/towns/siege/nexus, ships/cargo/markets, "
	"artifacts/auctions/economy/offline messages";

static bool fail(char *error, size_t error_size, const char *format, ...)
{
	va_list args;

	if (error && error_size)
	{
		va_start(args, format);
		vsnprintf(error, error_size, format, args);
		va_end(args);
	}
	return false;
}

static bool validate_private_directory(const char *path, char *error, size_t error_size)
{
	struct stat info;

	if (lstat(path, &info) < 0)
		return fail(error, error_size, "cannot inspect flat-file directory %s: %s", path,
			    strerror(errno));
	if (!S_ISDIR(info.st_mode))
		return fail(error, error_size, "flat-file path is not a directory: %s", path);
	if (info.st_uid != geteuid())
		return fail(error, error_size,
			    "flat-file directory is not owned by server user: %s", path);
	if (info.st_mode & 0077)
		return fail(error, error_size,
			    "flat-file directory permissions must be 0700 or stricter: %s", path);
	return true;
}

static bool ensure_private_directory(const char *path, char *error, size_t error_size)
{
	if (mkdir(path, 0700) < 0 && errno != EEXIST)
		return fail(error, error_size, "cannot create flat-file directory %s: %s", path,
			    strerror(errno));
	return validate_private_directory(path, error, error_size);
}

static bool provision_flatfile_directories(const char *root, char *error, size_t error_size)
{
	char path[4096];

	if (!root || !*root)
		return fail(error, error_size,
			    "FLATFILE_STATE_DIR is required for persistence mode %s",
			    persistence_mode_name());
	if (root[0] != '/')
		return fail(error, error_size, "FLATFILE_STATE_DIR must be an absolute path");
	if (!ensure_private_directory(root, error, error_size))
		return false;

	for (const char *directory : flatfile_directories)
	{
		int written = snprintf(path, sizeof(path), "%s/%s", root, directory);
		if (written < 0 || (size_t)written >= sizeof(path))
			return fail(error, error_size, "flat-file state path exceeds %zu bytes",
				    sizeof(path) - 1);
		if (!ensure_private_directory(path, error, error_size))
			return false;
	}
	return true;
}

bool persistence_mode_configure(char *error, size_t error_size)
{
	const char *configured = getenv("PERSISTENCE_MODE");

	active_flatfile_root = NULL;
	if (!configured || !*configured || !strcmp(configured, "mariadb-primary"))
		active_mode = PERSISTENCE_MODE_MARIADB_PRIMARY;
	else if (!strcmp(configured, "mariadb-primary-flatfile-fallback"))
		active_mode = PERSISTENCE_MODE_MARIADB_PRIMARY_FLATFILE_FALLBACK;
	else if (!strcmp(configured, "flatfile-primary"))
		active_mode = PERSISTENCE_MODE_FLATFILE_PRIMARY;
	else
		return fail(error, error_size,
			    "invalid PERSISTENCE_MODE '%s'; expected mariadb-primary, "
			    "mariadb-primary-flatfile-fallback, or flatfile-primary",
			    configured);

	if (active_mode == PERSISTENCE_MODE_MARIADB_PRIMARY)
		return true;

	active_flatfile_root = getenv("FLATFILE_STATE_DIR");
	if (!provision_flatfile_directories(active_flatfile_root, error, error_size))
		return false;

	return fail(error, error_size,
		    "persistence mode %s is not ready; unimplemented durable domains: %s",
		    persistence_mode_name(), unimplemented_domains);
}

enum persistence_mode persistence_mode_get(void)
{
	return active_mode;
}

const char *persistence_mode_name(void)
{
	switch (active_mode)
	{
	case PERSISTENCE_MODE_MARIADB_PRIMARY:
		return "mariadb-primary";
	case PERSISTENCE_MODE_MARIADB_PRIMARY_FLATFILE_FALLBACK:
		return "mariadb-primary-flatfile-fallback";
	case PERSISTENCE_MODE_FLATFILE_PRIMARY:
		return "flatfile-primary";
	}
	return "invalid";
}

bool persistence_mode_requires_mysql(void)
{
	return active_mode != PERSISTENCE_MODE_FLATFILE_PRIMARY;
}

const char *persistence_mode_flatfile_root(void)
{
	return active_flatfile_root;
}
