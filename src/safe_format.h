#ifndef DURIS_SAFE_FORMAT_H
#define DURIS_SAFE_FORMAT_H

#include <stddef.h>

/*
 * snprintf-compatible formatting for fixed legacy buffers. The formatted
 * value is rendered separately before it is copied, so a destination may also
 * appear among the string arguments without invoking overlapping-buffer UB.
 * Truncation remains NUL-terminated and is reported on stderr.
 */
int checked_snprintf(char *destination, size_t destination_size, const char *format, ...)
	__attribute__((format(printf, 3, 4)));

#endif
