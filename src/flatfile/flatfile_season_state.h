#ifndef DURIS_FLATFILE_SEASON_STATE_H
#define DURIS_FLATFILE_SEASON_STATE_H

#include "flatfile/flatfile_authority_transaction.h"
#include "persistence/persistence_mode.h"

#include <cstdint>
#include <string>

enum class flatfile_season_status : uint32_t
{
	active = 1,
	resetting = 2,
};
struct flatfile_season_state
{
	uint64_t epoch = 0;
	flatfile_season_status status = flatfile_season_status::resetting;
};
enum class flatfile_season_state_result
{
	ok,
	not_found,
	invalid,
	io_error,
	publication_uncertain,
};

// Passive native season observation under the caller's SAME genuine recovered
// exclusive configured-root lock. Both absent returns not_found; partial,
// foreign format, corrupt or resetting records refuse with strong output.
// Marker binds only format/genesis, never path/lineage/accounting/source proof.
// Caller separately authenticates configured root, epoch/source and memory budget.
flatfile_season_state_result
flatfile_season_state_read_locked(const std::string &root, const flatfile_authority_lock &lock,
				  flatfile_season_state *output) noexcept;

class flatfile_season_bootstrap final
{
    private:
	friend bool persistence_mode_configure(char *, size_t);
	// ONLY the real bootstrap can call this after its actual successful mkdir
	// and directory durability proof, before IP/UID/runtime/world initialization.
	// No public bool witness, legacy enrollment, reseed, reset or writer permit.
	// SAME recovered lock, two-record atomic bundle and exact readback.
	// Unknown publication/readback remains closed; no erase/reissue on refusal.
	static flatfile_season_state_result
	enroll_fresh_locked(const std::string &root, const flatfile_authority_lock &lock,
			    flatfile_season_state *output) noexcept;
};

#endif
