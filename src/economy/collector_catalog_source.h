#ifndef DURIS_COLLECTOR_CATALOG_SOURCE_H
#define DURIS_COLLECTOR_CATALOG_SOURCE_H

#include "economy/collector_codec.h"

#include <string>

// Worker-thread selected-backend load. The caller owns publication.
bool collector_catalog_source_load(collector::catalog &catalog, std::string &error);

#endif
