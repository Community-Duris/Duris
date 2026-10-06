#ifndef NATIVE_MOBILE_BIRTH_ARTIFACT_H
#define NATIVE_MOBILE_BIRTH_ARTIFACT_H

#include <array>
#include <cstdint>

// Immutable value witness only: SHA256 of the actual running Linux executable.
// First use reads its opened /proc/self/exe descriptor with fixed memory. The
// result (including refusal) is retained for this process; later births do not
// rehash the server. Different/stripped/rebuilt executables are not compatible.
// This does not authenticate shared libraries, data or admission/ACK authority.
// Caller must also prove the original constructor inputs and publication cut.
bool native_mobile_birth_running_artifact_digest(std::array<uint8_t, 32> *) noexcept;

#endif
