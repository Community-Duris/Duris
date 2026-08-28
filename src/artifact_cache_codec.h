#ifndef ARTIFACT_CACHE_CODEC_H
#define ARTIFACT_CACHE_CODEC_H

#include <cjson/cJSON.h>

constexpr int ARTIFACT_CACHE_SCHEMA_VERSION = 1;

bool artifact_cache_payload_valid(const cJSON *root, int expected_type, bool expected_godlist);

#endif
