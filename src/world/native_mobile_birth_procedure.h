#include <cstddef>
#ifndef NATIVE_MOBILE_BIRTH_PROCEDURE_H
#define NATIVE_MOBILE_BIRTH_PROCEDURE_H

#include <array>
#include <cstdint>

struct char_data;
using native_mobile_birth_procedure_function = int (*)(char_data *, char_data *, int, char *);
using native_mobile_birth_procedure_digest = std::array<uint8_t, 32>;

// Read-only, serialized game-thread observation of the actual VNUM callbacks.
// The original owner supplies its authenticated CURRENT /proc/self/exe digest;
// this value helper cannot establish that build provenance itself.
// NMP1 hashes build/VNUM, null-or-main-ELF virtual entry addresses, selected
// parsed studioproc definitions and actual chained predecessors. No pointers
// are serialized, reconstructed, installed or invoked. DSO entries refuse.
// Capture separately before/after construction; this confers no permission
// for a transition, source claim, SQL mutation or physical publication.
// Strong output: failure leaves output unchanged. Default Linux x86_64 ELF
// ABI only; the existing constructor still proves template/config compatibility.
bool native_mobile_birth_procedure_capture(
	int32_t mobile_vnum, const native_mobile_birth_procedure_digest &actual_build_digest,
	native_mobile_birth_procedure_digest *output) noexcept;
bool native_mobile_birth_procedure_matches(
	int32_t mobile_vnum, const native_mobile_birth_procedure_digest &actual_build_digest,
	const native_mobile_birth_procedure_digest &expected) noexcept;

// Full actual original ELF/dispatcher/definition/predecessor witness with
// admitted fixed SHA/carriers and original shop order. No callback execution.
// Strong output; caller preserves actual source records through each relay.
bool native_mobile_birth_procedure_capture_bounded(int32_t,
						   const native_mobile_birth_procedure_digest &,
						   native_mobile_birth_procedure_digest *,
						   bool (*)(size_t, void *) noexcept, void *,
						   size_t) noexcept;

#endif
