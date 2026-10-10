#ifndef DURIS_FLATFILE_INITIALIZED_WORLD_OWNER_H
#define DURIS_FLATFILE_INITIALIZED_WORLD_OWNER_H

#include "flatfile/flatfile_identity_repository.h"
#include "world/economic_initialized_world_snapshot.h"

class economic_initialized_world_owner;

// Actual late game-loop lifecycle owner. This class cannot select an accounting
// epoch, install a baseline, apply a transaction, or acknowledge a command.
// It is deliberately unselected until the owning game-loop bridge is supplied.
class flatfile_initialized_world_owner final
{
    private:
	friend void game_loop(int, int);
	friend class economic_initialized_world_owner;
	struct implementation;
	static implementation &state() noexcept;
	static bool held(const implementation &) noexcept;
	static bool retained_bytes(size_t *) noexcept;
	// Call only after successful hydration, on the genuine game thread. The
	// reserve context and caller's outer storage stay alive until cleanup. The
	// same actual save epoch must already exist; zero cannot grant exclusion.
	static unsigned int begin_initialized_cut(const std::string &, flatfile_scratch_reserve_fn,
						  void *, size_t outer_live_scratch) noexcept;
	// Complete synchronous borrow of the SAME captured world and held locks.
	// No callback may retain references, end exclusion, mutate the captured world,
	// or begin another owner. Callback success alone does not grant activation.
	using consumer = bool (*)(const economic_initialized_world_snapshot &, const std::string &,
				  const flatfile_identity_lock &, const flatfile_authority_lock &,
				  void *) noexcept;
	static bool with_boot_cut(consumer, void *) noexcept;
	// Revoke the borrow before any native/network callback or exclusion release.
	// Failed cleanup retains its actual reservation and acquired original locks.
	static bool before_world_callbacks() noexcept;
};

#endif
