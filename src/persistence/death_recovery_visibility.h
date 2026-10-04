#ifndef DURIS_DEATH_RECOVERY_VISIBILITY_H
#define DURIS_DEATH_RECOVERY_VISIBILITY_H

#include "persistence/critical_command.h"
#include <array>
#include <cerrno>
#include <cstdint>
#include <openssl/sha.h>

// The generic reporter rejects string conversions to protect private values.
// These callers render only numeric, correlation and fixed category metadata.
// Reject every format directive/control character before passing that bounded
// metadata as a literal format; do not widen the generic reporter's policy.
inline const char *death_recovery_literal_detail(const char *text)
{
	if (!text)
		return nullptr;
	for (const unsigned char *p = reinterpret_cast<const unsigned char *>(text); *p; ++p)
		if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
		      (*p >= '0' && *p <= '9') || *p == '_' || *p == '=' || *p == '-' || *p == ' '))
			return nullptr;
	return text;
}

// Diagnostics only. The existing (pid, corpse save ID) custody relationship is
// the input; this digest never authorizes a mutation or replaces an operation ID.
inline void death_recovery_correlation(uint64_t corpse_owner, char output[33])
{
	constexpr char domain[] = "duris-death-recovery-v1";
	std::array<unsigned char, sizeof(domain) - 1 + 8> input = {};
	for (size_t i = 0; i < sizeof(domain) - 1; ++i)
		input[i] = domain[i];
	for (size_t i = 0; i < 8; ++i)
		input[sizeof(domain) - 1 + i] = static_cast<unsigned char>(corpse_owner >> (i * 8));
	std::array<unsigned char, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(input.data(), input.size(), digest.data());
	constexpr char hex[] = "0123456789abcdef";
	for (size_t i = 0; i < 16; ++i)
	{
		output[2 * i] = hex[digest[i] >> 4];
		output[2 * i + 1] = hex[digest[i] & 15];
	}
	output[32] = 0;
}

inline bool death_recovery_command_correlation(const critical_command &command, char output[33])
{
	for (const auto &key : command.keys)
		if (key.type == critical_entity_type::corpse)
		{
			death_recovery_correlation(key.id, output);
			return true;
		}
	return false;
}

// A distinct durable result disambiguates selected authority cardinality from
// payload/serialization size limits. Historical errno 90 remains readable.
constexpr unsigned int ITEM_TRANSFER_TOPOLOGY_CARDINALITY = 0x445201;
inline const char *death_recovery_refusal_name(unsigned int code)
{
	switch (code)
	{
	case 0:
		return "none";
	case ITEM_TRANSFER_TOPOLOGY_CARDINALITY:
		return "durable_topology_cardinality_mismatch";
	case EMSGSIZE:
		return "legacy_transfer_size_mismatch";
	case EINVAL:
		return "invalid_evidence";
	case ENOENT:
		return "authority_or_payload_missing";
	case ESTALE:
		return "stale_authority_revision";
	case EEXIST:
		return "identity_or_projection_conflict";
	case EBUSY:
		return "authority_busy";
	case ENOMEM:
		return "allocation_unavailable";
	case EIO:
		return "storage_io_failure";
	default:
		return "storage_or_integrity_failure";
	}
}

// Emit at 1, 2, 4, ... occurrences, at most once per 30 seconds after the first.
// Counters are retained even when the alert is suppressed. Terminal summaries
// bypass this gate, so resolution always reports total count and elapsed time.
inline bool death_recovery_alert_due(uint64_t count, uint64_t now, uint64_t *last)
{
	if (!last || !count || (count & (count - 1)))
		return false;
	if (count != 1 && (now < *last || now - *last < 30000000))
		return false;
	*last = now;
	return true;
}

#endif
