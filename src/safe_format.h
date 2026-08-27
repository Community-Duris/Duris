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

/* For vetted, data-driven format templates whose conversion types cannot be
 * checked at compile time. Prefer checked_snprintf for literal formats. */
int checked_snprintf_runtime(char *destination, size_t destination_size, const char *format, ...);

/*
 * Expand data-driven templates containing only %s placeholders. Unlike
 * checked_snprintf_runtime, the template is never passed to a printf-family
 * function, so malformed or player-controlled percent sequences are copied
 * literally instead of being interpreted as conversions.
 */
int checked_substitute_strings(char *destination, size_t destination_size, const char *format,
			       const char *const *substitutions, size_t substitution_count);

/*
 * Append formatted text to a NUL-terminated buffer holding `capacity` bytes.
 *
 * Truncates rather than overflowing, and leaves an already-full buffer
 * untouched. Truncation is silent, as with snprintf: these are display buffers
 * whose callers expect snprintf semantics, and several append from inside the
 * game loop where writing to stderr would be worse than a short line.
 *
 * Prefer the APPENDF() macro below, which supplies the capacity for you.
 */
int checked_appendf(char *buffer, size_t capacity, const char *format, ...)
	__attribute__((format(printf, 3, 4)));

#ifdef __cplusplus
template <typename... Values> int checked_substitute(char *destination, size_t destination_size,
						     const char *format, Values... values)
{
	const char *substitutions[] = { values... };
	return checked_substitute_strings(destination, destination_size, format, substitutions,
					  sizeof...(values));
}

/*
 * A buffer's capacity, deduced from its type. A pointer carries no array bound,
 * so APPENDF() on one fails to compile instead of silently using sizeof(char *).
 */
template <size_t N> constexpr size_t duris_buffer_capacity(const char (&)[N])
{
	return N;
}

/*
 * Append to a fixed-size buffer:  APPENDF(buf, "hp: %d", hp);
 *
 * This replaces the legacy idiom
 *     snprintf(buf + strlen(buf), CAPACITY - strlen(buf), ...)
 * which repeatedly hard-coded CAPACITY as MAX_STRING_LENGTH (64KB) against
 * buffers as small as 50 bytes. glibc's __snprintf_chk aborts the process as
 * soon as the claimed size exceeds the destination size it can determine - it
 * does not wait for the output to actually be long - so each of those was a
 * dormant "*** buffer overflow detected ***" that a rebuild could wake.
 * Deducing the capacity from the array removes the chance to state it wrongly.
 */
#define APPENDF(buffer, ...) checked_appendf((buffer), duris_buffer_capacity(buffer), __VA_ARGS__)
#endif

#endif
