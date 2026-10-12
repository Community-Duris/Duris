#include "core/logging_internal.h"
#include "core/defines.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

uint debugcount = 0;

namespace core_logging_detail
{
char *format_variadic_message(const char *prefix, const char *suffix, const char *format,
			      va_list args)
{
	va_list copy;
	int body_len;
	size_t prefix_len;
	size_t suffix_len;
	char *buf;
	const char *safe_prefix = prefix ? prefix : "";
	const char *safe_suffix = suffix ? suffix : "";

	va_copy(copy, args);
	body_len = vsnprintf(NULL, 0, format, copy);
	va_end(copy);
	if (body_len < 0)
		return NULL;

	prefix_len = strlen(safe_prefix);
	suffix_len = strlen(safe_suffix);
	buf = (char *)malloc(prefix_len + (size_t)body_len + suffix_len + 1);
	if (!buf)
		return NULL;

	memcpy(buf, safe_prefix, prefix_len);
	va_copy(copy, args);
	vsnprintf(buf + prefix_len, (size_t)body_len + 1, format, copy);
	va_end(copy);
	memcpy(buf + prefix_len + (size_t)body_len, safe_suffix, suffix_len);
	buf[prefix_len + (size_t)body_len + suffix_len] = '\0';
	return buf;
}

bool create_log_parent_directories(const char *filename)
{
	char path[MAX_STRING_LENGTH];
	size_t length;

	if (!filename || !*filename)
		return false;

	length = strlen(filename);
	if (length >= sizeof(path))
		return false;
	memcpy(path, filename, length + 1);

	for (char *slash = strchr(path, '/'); slash; slash = strchr(slash + 1, '/'))
	{
		if (slash == path)
			continue;

		*slash = '\0';
		if (mkdir(path, 0755) != 0 && errno != EEXIST)
		{
			*slash = '/';
			return false;
		}
		*slash = '/';
	}
	return true;
}
} // namespace core_logging_detail

using core_logging_detail::create_log_parent_directories;
using core_logging_detail::format_variadic_message;

void logit(const char *filename, const char *format, ...)
{
	FILE *log_f;
	char tbuf[MAX_STRING_LENGTH];
	char *lbuf;
	time_t ct;

	// long ct;
	va_list args;

	va_start(args, format);
	ct = time(0);

	bzero(tbuf, MAX_STRING_LENGTH);

	if (str_cmp(filename, LOG_EVENT))
	{
		strcpy(tbuf, asctime(localtime(&ct)));
		tbuf[strlen(tbuf) - 1] = 0;
		strcat(tbuf, "::");
	}
	else
		*tbuf = '\0';

	if (str_cmp(filename, LOG_DEBUG))
		debugcount++;

	lbuf = format_variadic_message(tbuf, "\n", format, args);
	va_end(args);
	if (!lbuf)
		return;

	log_f = fopen(filename, "a");
	if (!log_f && errno == ENOENT && create_log_parent_directories(filename))
		log_f = fopen(filename, "a");
	if (!log_f)
	{
		free(lbuf);
		if (str_cmp(filename, LOG_FILE))
			logit(LOG_FILE, "failure opening logfile %s", filename);
		return;
	}
	if (!(debugcount % 500))
	{
		rewind(log_f);
	}
	fputs(lbuf, log_f);
	fclose(log_f);
	if (!str_cmp(filename, LOG_EXIT))
	{
		fputs(lbuf, stderr);
	}
	free(lbuf);
}
