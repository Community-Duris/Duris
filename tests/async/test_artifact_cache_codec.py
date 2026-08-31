#!/usr/bin/env python3
"""Runtime validation for versioned artifact cache payloads."""

from _paths import SRC, rel
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "guild/artifact_cache_codec.h"

#include <cassert>
#include <cjson/cJSON.h>

static bool valid(const char *json, int type, bool godlist)
{
    cJSON *root = cJSON_Parse(json);
    bool result = artifact_cache_payload_valid(root, type, godlist);
    cJSON_Delete(root);
    return result;
}

int main()
{
    const char *mortal = R"({"schema_version":1,"type":1,"godlist":false,"artifacts":[{"vnum":900,"locType":3,"location":42,"owned":true,"shortDesc":"an artifact","ownerName":"Tester","racewar":1}]})";
    const char *god = R"({"schema_version":1,"type":2,"godlist":true,"artifacts":[{"vnum":901,"locType":5,"location":42,"owned":true,"shortDesc":"a unique","racewar":2,"timer":2000000000,"lastUpdate":"2026-08-28"}]})";
    assert(valid(mortal, 1, false));
    assert(valid(god, 2, true));
    assert(!valid(mortal, 2, false));
    assert(!valid(mortal, 1, true));
    assert(!valid(R"({"type":1,"godlist":false,"artifacts":[]})", 1, false));
    assert(!valid(R"({"schema_version":1,"type":1,"godlist":false,"artifacts":[{}]})", 1, false));
    assert(!valid(R"({"schema_version":1,"type":1,"godlist":false,"artifacts":[{"vnum":900,"locType":3,"location":42,"owned":true,"shortDesc":null,"racewar":1}]})", 1, false));
    assert(!valid(R"({"schema_version":1,"type":1,"godlist":false,"artifacts":[{"vnum":900,"locType":3,"location":42,"owned":true,"shortDesc":"x","racewar":99}]})", 1, false));
    assert(!valid("not json", 1, false));
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-artifact-cache-") as temp:
    source = Path(temp) / "artifact_cache_test.cpp"
    binary = Path(temp) / "artifact_cache_test"
    source.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Isrc",
            str(source),
            rel("artifact_cache_codec.c"),
            "-lcjson",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)

artifact = (SRC / "artifact.c").read_text(encoding="utf-8")
listing_start = artifact.index("void list_artifacts_sql", artifact.index("void do_artifact_sql"))
listing = artifact[
    listing_start : artifact.index("void arti_remove_sql", listing_start)
]
assert listing.count("redis_get_artifact_list") == 1
assert "redis_invalidate_artifact_list" in listing
assert "redis_cache_artifact_list(type, Godlist, json)" in listing
assert "Artifact data is temporarily unavailable." in listing
assert "Cache error." not in listing

redis = (SRC / "redis_report_cache.c").read_text(encoding="ascii")
cache_start = redis.index("void redis_cache_artifact_list")
cache_end = redis.index("char *redis_get_artifact_list", cache_start)
cache = redis[cache_start:cache_end]
assert "artifact_cache_ttl_seconds = 900" in redis
assert "cache_set_ex" in cache

print("artifact cache schema and SQL fallback checks passed")
