#include "guild/assocs.h"
#include "net/listen.h"
#include <fstream>
// Outside-world side effects are controlled; the real say/gcc recipient loops,
// language call sites, message builders, act gates and serializer are exercised.
Guild::Guild()
{
	strcpy(name, "Fixture Guild");
	racewar = 1;
}
Guild::~Guild() {}
P_Guild get_guild_from_id(int)
{
	return nullptr;
}
const racewar_struct racewar_color[MAX_RACEWAR + 2]{};
bool affected_by_spell_flagged(P_char, int, uint)
{
	return false;
}
void send_to_avatar(P_char, const char *) {}
void listen_broadcast(P_char, const char *, int) {}
void check_magic_doors(P_char, const char *) {}
void studioproc_speech(P_char, const char *) {}
