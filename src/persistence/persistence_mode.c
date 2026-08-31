#include "persistence/persistence_mode.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __NO_MYSQL__
#include "flatfile/flatfile_ip_activity_repository.h"

#include <string>
#include <time.h>
#endif

static enum persistence_mode active_mode = PERSISTENCE_MODE_MARIADB_PRIMARY;
static const char *active_flatfile_root;

#ifdef __NO_MYSQL__
static const char *const flatfile_directories[] = {
	"metadata",	    "identities", "identities/accounts",
	"identities/names", "players",	  "operations",
	"operations/wal",   "domains",	  "manifests",
};
#endif

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

#ifdef __NO_MYSQL__
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
#endif

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
	{
#ifdef __NO_MYSQL__
		return fail(error, error_size,
			    "persistence mode mariadb-primary requires a MariaDB client build");
#else
		return true;
#endif
	}

	if (active_mode == PERSISTENCE_MODE_MARIADB_PRIMARY_FLATFILE_FALLBACK)
		return fail(error, error_size,
			    "persistence mode mariadb-primary-flatfile-fallback is not supported; "
			    "select mariadb-primary or flatfile-primary explicitly");

#ifndef __NO_MYSQL__
	return fail(error, error_size,
		    "persistence mode flatfile-primary requires a client-free flatfile build");
#else

	active_flatfile_root = getenv("FLATFILE_STATE_DIR");
	if (!provision_flatfile_directories(active_flatfile_root, error, error_size))
		return false;

	std::string activity_error;
	if (flatfile_ip_activity_reset_active(active_flatfile_root, (int64_t)time(NULL),
					      &activity_error) != flatfile_ip_activity_result::ok)
		return fail(error, error_size, "cannot reset flat-file IP activity: %s",
			    activity_error.c_str());
	return true;
#endif
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
