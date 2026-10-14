#ifndef CORPSE_RAISE_CUSTODY_H
#define CORPSE_RAISE_CUSTODY_H

#include <cstdint>
#include <vector>

struct obj_data;
struct corpse_lifecycle_payload;
struct corpse_lifecycle_result;

// Read-only native item selection for the original committed PC-corpse raise.
// UID-less loaded scalar cash is excluded only with its genuine loader origin,
// same current corpse/UID, a unique live leaf and no runtime custody history.
// Other objects require exact active corpse custody and native forest binding.
// Failure preserves output. This selects no new UID, owner, publication or ACK.
bool persistence_collect_corpse_raise_discarded(obj_data *corpse,
						const corpse_lifecycle_payload &payload,
						const corpse_lifecycle_result &result,
						std::vector<uint64_t> *output) noexcept;

#endif
