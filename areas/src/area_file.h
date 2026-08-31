#ifndef DURIS_AREA_FILE_H
#define DURIS_AREA_FILE_H

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/*
 * Preserve exact-name lookup first, then accommodate legacy area files whose
 * spelling differs from the AREA entry only by ASCII letter case.
 */
static FILE *fopen_area_file(const char *path)
{
	FILE *file = fopen(path, "r");
	if (file != NULL || errno != ENOENT)
		return file;

	const char *filename = strrchr(path, '/');
	if (filename == NULL)
		return NULL;
	filename++;

	size_t directory_length = (size_t)(filename - path - 1);
	char *directory = malloc(directory_length + 1);
	if (directory == NULL)
		return NULL;
	memcpy(directory, path, directory_length);
	directory[directory_length] = '\0';

	DIR *entries = opendir(directory);
	if (entries == NULL)
	{
		free(directory);
		return NULL;
	}

	char *match = NULL;
	errno = 0;
	for (;;)
	{
		struct dirent *entry = readdir(entries);
		if (entry == NULL)
			break;
		if (strcasecmp(entry->d_name, filename) != 0)
			continue;

		if (match != NULL)
		{
			fprintf(stderr, "error: %s matches both %s/%s and %s/%s\n", path, directory,
				match, directory, entry->d_name);
			free(match);
			closedir(entries);
			free(directory);
			errno = EEXIST;
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

	int scan_error = errno;
	closedir(entries);
	if (match == NULL || scan_error != 0)
	{
		free(match);
		free(directory);
		errno = scan_error != 0 ? scan_error : ENOENT;
		return NULL;
	}

	size_t match_length = strlen(match);
	char *resolved_path = malloc(directory_length + match_length + 2);
	if (resolved_path == NULL)
	{
		free(match);
		free(directory);
		return NULL;
	}
	memcpy(resolved_path, directory, directory_length);
	resolved_path[directory_length] = '/';
	memcpy(resolved_path + directory_length + 1, match, match_length + 1);

	file = fopen(resolved_path, "r");
	free(resolved_path);
	free(match);
	free(directory);
	return file;
}

#endif
