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
	static bool restore(P_obj, quest_mobile_native_zombie_stage &) noexcept;
	bool publish(P_obj) noexcept;
	static bool observe_published(P_obj) noexcept;
	bool discard() noexcept;
};

int zgame_load_zombie(P_obj obj);
void zgame_clear_zombies(P_obj obj);
int zg_count_zombies(P_obj obj);
int zgame_mob_proc(P_char ch, P_char pl, int cmd, char *arg);
ZombieGame *get_zgame_from_obj(P_obj obj);

#endif
