#!/usr/bin/env python3
"""Exercise the Redis application/environment/deployment namespace validator."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

HARNESS = r'''
#include "redis_namespace.h"

#include <assert.h>
#include <string.h>

int main()
{
    char output[64] = {};
    assert(redis_namespace_validate("duris:local:default", "local", output, sizeof output));
    assert(strcmp(output, "duris:local:default") == 0);
    assert(redis_namespace_validate("duris:production:blue_2", "production", output,
                                    sizeof output));
    assert(!redis_namespace_validate(nullptr, "local", output, sizeof output));
    assert(!redis_namespace_validate("duris:local:test", nullptr, output, sizeof output));
    assert(!redis_namespace_validate("mud", "local", output, sizeof output));
    assert(!redis_namespace_validate("duris:production:test", "local", output,
                                     sizeof output));
    assert(!redis_namespace_validate("duris:local:", "local", output, sizeof output));
    assert(!redis_namespace_validate("duris:local:-test", "local", output, sizeof output));
    assert(!redis_namespace_validate("duris:local:test_", "local", output, sizeof output));
    assert(!redis_namespace_validate("duris:local:Test", "local", output, sizeof output));
    assert(!redis_namespace_validate("duris:local:test:other", "local", output,
                                     sizeof output));
    assert(!redis_namespace_validate("duris:staging:test", "staging", output, sizeof output));
    char small[8] = {};
    assert(!redis_namespace_validate("duris:local:test", "local", small, sizeof small));
    assert(redis_namespace_season_key("duris:local:test", 42, "cache:named", output,
                                      sizeof output));
    assert(strcmp(output, "duris:local:test:season:42:cache:named") == 0);
    assert(!redis_namespace_season_key("duris:local:test", 42, "cache:named", small,
                                       sizeof small));
    assert(!redis_namespace_season_key("", 42, "cache:named", output, sizeof output));
    assert(!redis_namespace_season_key("duris:local:test", 0, "cache:named", output,
                                       sizeof output));
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="redis-namespace-") as directory:
    root = Path(directory)
    harness = root / "harness.cpp"
    binary = root / "harness"
    harness.write_text(HARNESS, encoding="ascii")
    subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-I", str(SRC), str(harness), str(SRC / "redis_namespace.c"),
            "-o", str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("Redis namespace validation passed under ASan/UBSan")
