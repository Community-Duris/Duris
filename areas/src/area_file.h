#ifndef DURIS_AREA_FILE_H
#define DURIS_AREA_FILE_H

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

static FILE *fopen_regular_area_file(const char *path)
{
	struct stat status;
	if (stat(path, &status) != 0)
		return NULL;
	if (!S_ISREG(status.st_mode))
	{
		errno = EISDIR;
		return NULL;
	}

	return fopen(path, "r");
}

/*
 * Preserve exact-name lookup first, then accommodate legacy area files whose
 * spelling differs from the AREA entry only by ASCII letter case.
 */
static FILE *fopen_area_file(const char *path)
{
	FILE *file = fopen_regular_area_file(path);
	if (file != NULL || errno != ENOENT)
		return file;

	const char *separator = strrchr(path, '/');
	const char *filename = separator == NULL ? path : separator + 1;
	size_t directory_length =
		separator == NULL || separator == path ? 1 : (size_t)(separator - path);
	char *directory = malloc(directory_length + 1);
	if (directory == NULL)
		return NULL;
	if (separator == NULL)
		directory[0] = '.';
	else if (separator == path)
		directory[0] = '/';
	else
		memcpy(directory, path, directory_length);
	directory[directory_length] = '\0';

	DIR *entries = opendir(directory);
	if (entries == NULL)
	{
		free(directory);
		return NULL;
	}

	int directory_fd = dirfd(entries);
	if (directory_fd == -1)
	{
		int directory_error = errno;
		closedir(entries);
		free(directory);
		errno = directory_error;
		return NULL;
	}

	char *match = NULL;
	int scan_error = 0;
	int saw_non_regular_match = 0;
	for (;;)
	{
		errno = 0;
		struct dirent *entry = readdir(entries);
		if (entry == NULL)
		{
			scan_error = errno;
			break;
		}
		if (strcasecmp(entry->d_name, filename) != 0)
			continue;

		struct stat entry_status;
		if (fstatat(directory_fd, entry->d_name, &entry_status, 0) != 0)
		{
			scan_error = errno;
			break;
		}
		if (!S_ISREG(entry_status.st_mode))
		{
			saw_non_regular_match = 1;
			continue;
		}

		if (match != NULL)
		{
			fprintf(stderr, "error: %s matches both %s/%s and %s/%s\n", path, directory,
				match, directory, entry->d_name);
			free(match);
			closedir(entries);
			free(directory);
			errno = ENOTUNIQ;
			return NULL;
		}

		size_t match_length = strlen(entry->d_name) + 1;
		match = malloc(match_length);
		if (match == NULL)
		{
			closedir(entries);
			free(directory);
			return NULL;
		}
		memcpy(match, entry->d_name, match_length);
	}

	closedir(entries);
	if (match == NULL || scan_error != 0)
	{
		free(match);
		free(directory);
		errno = scan_error != 0 ? scan_error : saw_non_regular_match ? EISDIR : ENOENT;
		return NULL;
	}

	size_t match_length = strlen(match);
	size_t separator_length = directory[directory_length - 1] == '/' ? 0 : 1;
	char *resolved_path = malloc(directory_length + separator_length + match_length + 1);
	if (resolved_path == NULL)
	{
		free(match);
		free(directory);
		return NULL;
	}
	memcpy(resolved_path, directory, directory_length);
	if (separator_length != 0)
		resolved_path[directory_length] = '/';
	memcpy(resolved_path + directory_length + separator_length, match, match_length + 1);

	file = fopen_regular_area_file(resolved_path);
	free(resolved_path);
	free(match);
	free(directory);
	return file;
}

static void punt_area_file(const char *path)
{
	int open_error = errno;
	fprintf(stderr, "error: cannot open area file %s: %s\n", path, strerror(open_error));
	exit(EXIT_FAILURE);
}

static inline int area_file_is_optional_missing(const char *directory)
{
	if (errno != ENOENT)
		return 0;

	struct stat status;
	if (stat(directory, &status) != 0)
		return 0;
	if (!S_ISDIR(status.st_mode))
	{
		errno = ENOTDIR;
		return 0;
	}

	errno = ENOENT;
	return 1;
}

#endif /* DURIS_AREA_FILE_H */
