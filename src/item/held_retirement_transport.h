#ifndef DURIS_HELD_RETIREMENT_TRANSPORT_H
#define DURIS_HELD_RETIREMENT_TRANSPORT_H
#include "persistence/critical_command.h"

// Allocation-free wire classification only. Typed source, complete original
// bodies, current receipt and publication reservations grant domain authority.
// Ordinary continuation versions9/10 have identical framing; quest versions
// and generic item commands never gain held-retirement admission from this test.
inline bool held_retirement_transport_command(const critical_command &command) noexcept
{
	if (command.type != critical_command_type::item_transfer ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !command.publication_required ||
	    (command.payload_version != 9 && command.payload_version != 10))
		return false;
	const auto &bytes = command.payload;
	if (bytes.size() < 184 || bytes.size() > CRITICAL_COMMAND_MAX_PAYLOAD_BYTES)
		return false;
	const auto integer = [&](size_t offset, size_t count) noexcept
	{
		uint64_t result = 0;
		for (size_t i = 0; i < count; ++i)
			result |= static_cast<uint64_t>(bytes[offset + i]) << (8 * i);
		return result;
	};
	// One original item entry, including the reserved count bytes.
	if (integer(36, 4) != 1)
		return false;
	constexpr size_t item_section = 96 + 40;
	const auto blob_size = integer(item_section, 4);
	if (!blob_size || blob_size > bytes.size() - item_section - 4)
		return false;
	const size_t tail = item_section + 4 + static_cast<size_t>(blob_size);
	// Empty corpse/collector, no logical-source sidecar, exact kind8 terms.
	if (bytes.size() - tail != 44 || integer(tail, 4) || integer(tail + 4, 4) ||
	    integer(tail + 8, 8) || integer(tail + 16, 4) != 8 || integer(tail + 20, 4) != 20)
		return false;
	const size_t terms = tail + 24;
	return bytes[terms] == 1 && bytes[terms + 1] >= 1 && bytes[terms + 1] <= 3 &&
	       !bytes[terms + 2] && !bytes[terms + 3] && integer(terms + 4, 8) &&
	       integer(terms + 4, 8) != UINT64_MAX && integer(terms + 12, 4) &&
	       integer(terms + 12, 4) <= INT32_MAX && integer(terms + 16, 4) > 0 &&
	       integer(terms + 16, 4) <= INT32_MAX;
}
#endif
