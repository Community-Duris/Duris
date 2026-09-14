#ifndef DURIS_COLLECTOR_CATALOG_CACHE_H
#define DURIS_COLLECTOR_CATALOG_CACHE_H

#include <string>

// Read I/O occurs on one refresh worker. Pulse and runtime publication are
// game-thread only. A failed refresh retains the last authoritative runtime.
bool collector_catalog_cache_refresh(void);
void collector_catalog_cache_pulse(void);
void collector_catalog_cache_shutdown(void);
bool collector_catalog_cache_ready(void);
bool collector_catalog_cache_busy(void);
std::string collector_catalog_cache_status(void);

#endif
