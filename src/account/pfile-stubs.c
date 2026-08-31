#include "core/structs.h"
#include <stdlib.h>
#include <string.h>

int mem_use[20];
struct mm_ds *dead_trophy_pool = NULL;

void __free(void *p, const char *file, int line);
#define FREE(i)                                \
	{                                      \
		__free(i, __FILE__, __LINE__); \
		(i) = NULL;                    \
	}

#define GET_BYTE(buf) (*(char *)((buf)++))
#define GET_SHORT(buf) getShort(&buf)
#define GET_INTE(buf) getInt(&buf)
#define GET_LONG(buf) getLong(&buf)
#define GET_STRING(buf) getString(&buf)
void logit(char const *, char const *, ...) {}

void wizlog(char *, char *, ...) {}

void obj_to_room(void *, int) {}

void send_to_char(const char *, P_char) {}

int wear(P_char, P_obj, int, bool)
{
	return 0;
}

void writeSavedItem(void *) {}

void all_affects(void *, int) {}

P_obj read_object(int number, int)
{
	P_obj obj;
	obj = (P_obj)malloc(sizeof(struct obj_data));
	memset(obj, 0, sizeof(struct obj_data));
	obj->R_num = number;
	return obj;
}

void obj_to_obj(void *, void *) {}

void obj_to_char(P_obj obj, P_char ch)
{
	obj->next_content = ch->carrying;
	ch->carrying = obj;
}

void str_free(const char *source)
{
	if (source)
	{
		char *owned = const_cast<char *>(source);
		FREE(owned);
	}
}

int GET_LEVEL(P_char ch)
{
	return ch->player.level;
}

int vitality_limit(P_char ch)
{
	return ch->points.base_vitality;
}

int calculate_mana(P_char)
{
	return 1;
}

void extract_obj(void *obj, int)
{
	FREE(obj);
}

int flag2idx(int flag)
{
	int i = 0;

	while (flag > 0)
	{
		i++;
		flag >>= 1;
	}
	return i;
}

void update_skills(P_char) {}

struct affected_type *get_spell_from_char(P_char, int, void *, int)
{
	return 0;
}

/*
 * The pfile scanner links only skills.c and files.c out of the server tree.
 * Those two translation units still reference the wider runtime; the offline
 * tool never reaches those paths, so stub them rather than pull the server in.
 */
#include "persistence/persistence_mode.h"
#include "flatfile/flatfile_character_delete.h"
#include "guild/assocs.h"

P_index obj_index = NULL;
P_room world = NULL;
unsigned long next_obj_uid = 0;

void debug(const char *, ...) {}
int checked_snprintf(char *, size_t, const char *, ...)
{
	return 0;
}
char *str_dup(const char *source)
{
	return source ? strdup(source) : NULL;
}

int real_room(const int room)
{
	return room;
}
void clear_title(P_char) {}
void create_epic_skills() {}
void delete_ship(char *) {}
void event_short_affect(P_char, P_char, P_obj, void *) {}
struct extra_descr_data *find_spell_description(P_obj)
{
	return NULL;
}
P_Guild get_guild_from_id(int)
{
	return NULL;
}
void Guild::kick(P_char) {}
void load_zone_trophy(P_char) {}
int ne_event_time(P_nevent)
{
	return 0;
}
void remove_all_artifacts_sql(P_char) {}
bool remove_all_locker_access(P_char)
{
	return true;
}
void remove_char_from_list(P_acct, char *) {}

enum persistence_mode persistence_mode_get(void)
{
	return PERSISTENCE_MODE_MARIADB_PRIMARY;
}
const char *persistence_mode_flatfile_root(void)
{
	return "";
}
flatfile_character_delete_result flatfile_character_delete(const std::string &, int32_t,
							   const std::string &, std::string *)
{
	return flatfile_character_delete_result::ok;
}

bool sql_delete_corpse(const char *, int)
{
	return true;
}
bool sql_delete_locker(int, int)
{
	return true;
}
bool sql_delete_player(int)
{
	return true;
}
int sql_get_player_pid(const char *)
{
	return 0;
}
bool sql_load_player_affects(P_char)
{
	return true;
}
bool sql_load_player_shapechanges(P_char)
{
	return true;
}
bool sql_load_player_skills(P_char)
{
	return true;
}
bool sql_load_player_status(P_char, int)
{
	return true;
}
bool sql_soft_delete_character(long)
{
	return true;
}
