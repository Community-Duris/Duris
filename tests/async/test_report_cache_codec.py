#!/usr/bin/env python3
"""Exercise versioned report-cache countdown framing and freshness checks."""

from _paths import SRC, rel
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HARNESS = r'''
#include "report_cache_codec.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>

int main()
{
    constexpr uint64_t generated = 2000000000;
    constexpr uint64_t deadline = generated + 90061;
    char *payload = report_cache_countdown_encode("before ", " after", generated, deadline);
    assert(payload);
    assert(!strncmp(payload, "FRC1|", 5));

    char *first = report_cache_countdown_render(payload, generated, 900);
    assert(first && !strcmp(first, "before 01:01:01:01 after"));
    free(first);
    char *later = report_cache_countdown_render(payload, generated + 61, 900);
    assert(later && !strcmp(later, "before 01:01:00:00 after"));
    free(later);
    char *expired_timer = report_cache_countdown_render(payload, deadline + 1, 100000);
    assert(expired_timer && !strcmp(expired_timer, "before 00:00:00:00 after"));
    free(expired_timer);

    assert(!report_cache_countdown_render(payload, generated + 901, 900));
    assert(!report_cache_countdown_render(payload, generated - 61, 900));

    std::string corrupt(payload);
    corrupt.back() = 'x';
    assert(!report_cache_countdown_render(corrupt.c_str(), generated, 900));
    std::string trailing(payload);
    trailing += "x";
    assert(!report_cache_countdown_render(trailing.c_str(), generated, 900));
    std::string old_schema(payload);
    old_schema[3] = '0';
    assert(!report_cache_countdown_render(old_schema.c_str(), generated, 900));

    assert(!report_cache_countdown_encode(nullptr, "suffix", generated, deadline));
    assert(!report_cache_countdown_encode("prefix", nullptr, generated, deadline));
    assert(!report_cache_countdown_encode("prefix", "suffix", 0, deadline));
    assert(!report_cache_countdown_encode("prefix", "suffix", generated, 0));
    std::string oversized(65536, 'x');
    assert(!report_cache_countdown_encode(oversized.c_str(), "suffix", generated, deadline));
    free(payload);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="duris-report-cache-") as temp:
    source = Path(temp) / "report_cache_test.cpp"
    binary = Path(temp) / "report_cache_test"
    source.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            "-Isrc",
            str(source),
            rel("report_cache_codec.c"),
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run([str(binary)], check=True)

redis = (SRC / "redis_report_cache.c").read_text(encoding="ascii")
comm = (SRC / "comm.c").read_text(encoding="ascii")
assert "named_report_cache_ttl_seconds = 86400" in redis
assert "fraglist_cache_ttl_seconds = 900" in redis
assert "report_cache_countdown_encode" in redis
assert "report_cache_countdown_render" in redis
fraglist = redis[redis.index("char *generate_fraglist_cache_payload") :]
fraglist = fraglist[: fraglist.index("} // namespace")]
assert "sql_level_cap(" not in fraglist
cache_fraglist = redis[redis.index("void redis_cache_fraglist") :]
cache_fraglist = cache_fraglist[: cache_fraglist.index("char *redis_get_fraglist")]
assert "cache_set_ex(REDIS_CACHE_FRAGLIST" in cache_fraglist
assert "redis_invalidate_fraglist();" in comm

print("versioned report cache freshness and dynamic countdown checks passed")
