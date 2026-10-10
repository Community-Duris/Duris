#ifndef NATIVE_MOBILE_BIRTH_ARTIFACT_H
#define NATIVE_MOBILE_BIRTH_ARTIFACT_H

#include <array>
#include <cstddef>
#include <cstdint>

// Immutable value witness only: SHA256 of the actual running Linux executable.
// First use reads its opened /proc/self/exe descriptor with fixed memory. The
// result (including refusal) is retained for this process; later births do not
// rehash the server. Different/stripped/rebuilt executables are not compatible.
// This does not authenticate shared libraries, data or admission/ACK authority.
// Caller must also prove the original constructor inputs and publication cut.
bool native_mobile_birth_running_artifact_digest(std::array<uint8_t, 32> *) noexcept;

// Preadmits the genuine fixed SHA/file workspace before touching the same cache.
// Profile/budget refusal cannot initialize or poison the shared process cache.
// Output stays unchanged on refusal. errno distinguishes actual resource/profile
// refusal from the cached genuine capture failure. First bounded use avoids EVP.
bool native_mobile_birth_running_artifact_digest_bounded(std::array<uint8_t, 32> *,
							 bool (*reserve)(size_t, void *) noexcept,
							 void *context, size_t outer) noexcept;

#endif
