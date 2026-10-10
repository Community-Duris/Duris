#ifndef _SPECS_VENTHIX_H_
#define _SPECS_VENTHIX_H_

#include <vector>
using namespace std;

#include "core/structs.h"

struct ZombieGame
{
	ZombieGame();
	ZombieGame(P_obj _generator);
	~ZombieGame();

	int load();
	int unload();

	int id;

	static int next_id;

	P_obj generator;

	vector<P_char> zombies;
	int zombies_to_load;

	int zombies_alive()
	{
		if (zombies.size() < 1)
			return 0;

		return (int)zombies.size();
	}
};

class quest_mobile_native_item_stage;
// Original ZombieGame allocation/ID retained privately, not globally enrolled.
// No automatic cleanup: admitted uncertainty belongs to the birth owner.
class quest_mobile_native_zombie_stage
{
    private:
	friend class quest_mobile_native_item_stage;
	ZombieGame *game_ = nullptr;
	int mob_rnum_ = -1;
	P_index original_mob_index_ = nullptr;
	mob_proc_type original_mob_proc_ = nullptr;
	uint64_t item_uid_ = 0;
	static bool prepare(P_obj, quest_mobile_native_zombie_stage &) noexcept;
	// Complete original warm constructor; outer excludes genuine CURRENT G.
	// Recount actual registry/private stage on EVERY outcome, including failure.
	static bool prepare_bounded(P_obj, quest_mobile_native_zombie_stage &,
				    bool (*)(size_t *, void *) noexcept,
				    bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	static bool restore(P_obj, quest_mobile_native_zombie_stage &) noexcept;
	// Complete original frozen off-state restoration, without new ID issuance.
	// Outer retains actual CURRENT registry once; recount registry/private stage
	// on every return, including reserve failure or successful ownership transfer.
	static bool restore_bounded(P_obj, quest_mobile_native_zombie_stage &,
				    bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;
	bool publish(P_obj) noexcept;
	// Caller includes current registry observation and the private retained game
	// exactly once in outer, retains admitted peak through return, and recounts
	// registry + stage on EVERY return. No budget callback after enrollment.
	bool publish_bounded(P_obj, bool (*)(size_t, void *) noexcept, void *, size_t) noexcept;

	static bool observe_published(P_obj) noexcept;
	bool discard() noexcept;
};

// Game-thread-only allocation-free actual registry inline + vector capacity +
// owned game/zombie-vector requests under pinned GCC13 C++11 ABI. Strong output.
// The private unpublished game is excluded. Caller must retain/recount this
// global allowance after EVERY bounded publication return (including false).
// Observation grants no admission, source, publication or gameplay authority.
bool quest_mobile_native_zombie_registry_storage_bytes(size_t *) noexcept;
int zgame_load_zombie(P_obj obj);
void zgame_clear_zombies(P_obj obj);
int zg_count_zombies(P_obj obj);
int zgame_mob_proc(P_char ch, P_char pl, int cmd, char *arg);
ZombieGame *get_zgame_from_obj(P_obj obj);

#endif
