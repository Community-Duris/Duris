#ifndef REPORT_CACHE_CODEC_H
#define REPORT_CACHE_CODEC_H

#include <stdint.h>

inline constexpr uint64_t REPORT_CACHE_CLOCK_SKEW_SECONDS = 60;

char *report_cache_countdown_encode(const char *prefix, const char *suffix, uint64_t generated_at,
				    uint64_t deadline);
char *report_cache_countdown_render(const char *payload, uint64_t now,
				    uint64_t maximum_age_seconds);

#endif
