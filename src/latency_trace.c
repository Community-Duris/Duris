#include "latency_trace.h"

latency_entry _latency_buf[LATENCY_TRACE_MAX_SAMPLES];
int _latency_head = 0;
int _latency_count = 0;
pthread_mutex_t _latency_mutex = PTHREAD_MUTEX_INITIALIZER;
_latency_section _latency_sections[_LATENCY_MAX_SECTIONS];
int _latency_nsections = 0;
