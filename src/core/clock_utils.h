#ifndef __CLOCK_UTILS_H__
#define __CLOCK_UTILS_H__

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef int (*clock_read_fn)(clockid_t clock_id, struct timespec *result);

static inline bool clock_read_microseconds(clockid_t clock_id, uint64_t *result,
					   clock_read_fn read_clock)
{
	struct timespec now = {};

	if (!result || !read_clock || read_clock(clock_id, &now) != 0 || now.tv_sec < 0 ||
	    now.tv_nsec < 0 || now.tv_nsec >= 1000000000L)
		return false;
	const uint64_t seconds = (uint64_t)now.tv_sec;
	const uint64_t fractional_us = (uint64_t)now.tv_nsec / 1000ULL;
	if (seconds > (UINT64_MAX - fractional_us) / 1000000ULL)
		return false;
	*result = seconds * 1000000ULL + fractional_us;
	return true;
}

#endif /* __CLOCK_UTILS_H__ */
