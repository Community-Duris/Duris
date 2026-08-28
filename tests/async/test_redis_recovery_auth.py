#!/usr/bin/env python3
"""Require independent authentication for Redis world-recovery generations."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
REDIS = (ROOT / "src" / "redis.c").read_text(encoding="ascii")
STORE = (ROOT / "src" / "redis_world_store.c").read_text(encoding="ascii")
HEADER = (ROOT / "src" / "redis_world_store.h").read_text(encoding="ascii")

assert "REDIS_WORLD_STATE_SECRET" in REDIS
assert "REDIS_WORLD_STATE_SECRET_PREVIOUS" in REDIS
assert "current_size < 32 || current_size > 256" in REDIS
assert "previous_size < 32 || previous_size > 256" in REDIS
assert "config.authentication_secret = redis_world_authentication_secret.c_str()" in REDIS
assert "redis_clear_world_authentication_secrets()" in REDIS
assert "REDIS_WORLD_GENERATION_MANIFEST_BYTES = 120" in HEADER

for token in (
    'memcpy(output, "WRG2", 4)',
    "SHA256(generation, generation_size, output + 56)",
    "HMAC(EVP_sha256()",
    "CRYPTO_memcmp(expected_tag, data + 88, SHA256_DIGEST_LENGTH)",
    "config->previous_authentication_secret",
    "config->key_namespace",
    "config->season_epoch",
):
    assert token in STORE, token

reader = STORE[STORE.index("bool redis_world_store_read_generation"):STORE.index(
    "bool redis_world_store_publish"
)]
assert reader.index("decode_manifest") < reader.index("generation->assign")
assert reader.index("CRYPTO_memcmp(actual_digest") < reader.index("return valid")

publisher = STORE[STORE.index("bool redis_world_store_publish"):]
assert "encode_manifest(config, sequence" in publisher
assert publisher.index("encode_manifest") < publisher.index("WORLD_PUBLISH_SCRIPT")

print("Redis recovery authentication contracts passed")
