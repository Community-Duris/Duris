#ifndef DURIS_COLLECTOR_CATALOG_SOURCE_H
#define DURIS_COLLECTOR_CATALOG_SOURCE_H

#include "economy/collector_storage.h"

#include <string>

// Worker-thread selected-backend load. The caller owns publication.
bool collector_catalog_source_load(collector_bootstrap_snapshot &snapshot, std::string &error);

// Worker-thread selected-backend listing read. Not-found is not an I/O failure.
// error_code is zero on success and an errno/MySQL-style value on failure.
bool collector_listing_source_load(uint64_t listing, collector_listing_detail &detail, bool &found,
				   unsigned int &error_code, std::string &error);

#endif
