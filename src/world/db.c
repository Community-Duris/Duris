#include "economy/native_mobile_birth_recovery.h"

/*
 * ***************************************************************************
 *   File: db.c                                               Part of Duris *
 *   Usage: Loading/Saving chars, booting world, resetting etc.
 *   Copyright  1990, 1991 - see 'license.doc' for complete information.
 *   Copyright 1994 - 2008 - Duris Systems Ltd.
 *
 * ***************************************************************************
 */

#include "core/prototypes.h"
#include "world/character_maintenance.h"
#include "world/world_singletons.h"
#include "world/difficulty.h"
#include "combat/chaos_config.h"
#include <bit>
#include <cmath>
#include <openssl/evp.h>
#include "core/structs.h"
#include "player/pet_restore_runtime.h"
#include "net/comm.h"
#include "world/db.h"
#include "world/handler.h"
#include "world/quest_mobile_native_birth.h"
#include "world/native_mobile_birth_procedure.h"
#include "world/native_mobile_birth_reset_tail.h"
#include "world/events.h"
#include "world/world_activity.h"
#include "cmd/interp.h"
#include "core/utils.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "account/account.h"
#include "guild/assocs.h"
#include "persistence/copyover.h"
#include "world/epic.h"
#include "item/enhance.h"
#include "flatfile/flatfile_artifact_repository.h"
#include "combat/justice.h"
#include "combat/training_dummy.h"
#include "core/mm.h"
#include "item/objmisc.h"
#include "persistence/persistence_mode.h"
#include "redis/redis_world_runtime.h"
#include "ships/ships.h"
#include "world/specs.prototypes.h"
#include "magic/spells.h"
#include "sql/sql.h"
#include "mob/studioproc.h"
#include "item/trophy.h"
#include "world/weather.h"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <climits>
#include <cstdlib>
#include <type_traits>
#include <algorithm>
#include <new>
#include "world/object_template.h"
#include "player/player_snapshot.h"
#include "player/player_snapshot_capture.h"
#include "player/player_snapshot_codec.h"
#include "player/inert_item_stage.h"
#include "economy/native_mobile_birth_recipe.h"
#include <map>
#include <array>
#include <memory>
#include "specs/specs.venthix.h"
#include <utility>
#include "classes/npc_alchemist.h"
#include "account/newbie_kit_plan.h"
#include "economy/economic_gameplay_authority.h"
#include "world/zone_story_quest_runtime.h"

/*
 * external variables
 */

extern P_desc descriptor_list;
extern bool game_booted;
extern struct shop_data *shop_index;
extern int number_of_shops;
extern const char *equipment_types[];
extern const char *town_name_list[];
extern const int min_stats_for_class[][8];
extern const struct race_names race_names_table[];
extern struct stat_data stat_factor[];
extern int hometown[];
extern int no_specials;
extern int pulse;
extern int shutdownflag;
extern int spl_table[TOTALLVLS][MAX_CIRCLE];
extern long boot_time;
extern struct str_app_type str_app[];
extern struct time_info_data time_info;
extern struct mm_ds *dead_pconly_pool;
extern struct mm_ds *dead_trophy_pool;
extern int portal_id;
extern float exp_mods[EXPMOD_MAX + 1];
extern P_nevent current_nevent;
extern void obj_affect_remove(P_obj, struct obj_affect *);
void delete_knownShapes(P_char ch);
void proclib_obj_event(P_char, P_char, P_obj obj, void *);
int proclibObj_add(P_obj obj, char *procName, char *args);
int proclib_obj_proc(P_obj obj, P_char ch, int cmd, char *argument);
extern void event_mob_mundane(P_char, P_char, P_obj, void *);
extern void event_mob_proc(P_char, P_char, P_obj, void *);
extern void event_random_exit(P_char, P_char, P_obj, void *);
extern int teacher(P_char ch, P_char pl, int cmd, char *arg);
extern float hp_mob_con_factor, hp_mob_npc_pc_ratio, pulse_all;
extern float combat_by_race[LAST_RACE + 1][3], class_hitpoints[CLASS_COUNT + 1];
extern const struct racial_data_type racial_data[];
extern char *specdata[][MAX_SPEC];
extern char racial_innates[LAST_INNATE + 1][LAST_RACE + 1];
extern char class_innates[LAST_INNATE + 2][CLASS_COUNT][5];
extern unsigned int class_innates_at_all[LAST_INNATE + 2];
extern Skill skills[];
extern struct quest_data quest_index[MAX_QUESTS];
extern int number_of_quests;
extern void event_mob_skin_spell(P_char, P_char, P_obj, void *);
extern struct social_messg *soc_mess_list;
void recalc_zone_numbers();
void ne_init_events();
void ne_init_event_pool();
extern void event_reset_zone(P_char, P_char, P_obj, void *);

static void boot_zone_story_quest_state()
{
	// Retained character state needs its catalog even when mobile procedures
	// are disabled. Normal boots already load quests in assign_mobiles().
	if (no_specials)
		boot_the_quests();
	std::string error;
	if (!zone_story_quest_runtime::bootstrap(&error))
		logit(LOG_DEBUG, "Zone-story quest catalog disabled at boot: %s", error.c_str());
	else
		logit(LOG_STATUS, "Zone-story quest catalog ready: %zu definitions, revision %u",
		      zone_story_quest_runtime::service()->catalog().definitions.size(),
		      zone_story_quest_runtime::content_revision());
}

/**************************************************************************
 *  declarations of most of the 'global' variables                         *
 ************************************************************************ */

struct reset_q_type reset_q;

P_room world; /* dyn alloc'ed array of rooms     */
int top_of_world = 0; /* ref to the top element of world - LAST VALID ROOM INDEX
                                            * world[top_of_world] is valid world[top_of_world+1] is out
                                            * of bounds.
                                            */
P_obj object_list = NULL; /* the global linked list of obj's */
P_char character_list = NULL; /* global l-list of chars          */
struct ban_t *ban_list = NULL;
struct wizban_t *wizconnect = NULL;
struct zone_data *zone_table; /* table of reset data             */
struct sector_data *sector_table; /* mostly weather info             */
int top_of_zone_table = 0; /* The highest valid zone rnum     */
static bool mobile_probe_mode = false;
struct message_list fight_messages[MAX_MESSAGES]; /* fighting messages  */

char *guild_frags = NULL;
// char    *news = NULL;           /* * the news                        */
string news;
char *projects = NULL; /* * Project information             */
// char    *motd = NULL;           /* * ansi motd                       */
string motd;
// char    *wizmotd = NULL;        /* * ansi wizmotd * */
string wizmotd;
char *help = NULL; /* * the main help page              */
char *rules = NULL;
char *wizlista = NULL; /* * wizlist for ansi listeners * */
char *greetinga = NULL; /* * greeting for our ansi viewers * */
char *greetinga1 = NULL;
char *greetinga2 = NULL;
char *greetinga3 = NULL;
char *greetinga4 = NULL;
char *greetings = NULL; /* * greeting for ascii viewers * */
char *disclaimer = NULL; /* * disclaimer message * */
char *bugfile = NULL;
char *generaltable = NULL; /* * race/class comparison charts * */
char *racewars = NULL; /* * good/evil race explanation * */
char *classtable = NULL; /* * class selection tables * */
char *racetable = NULL; /* * race selection tables * */
// char    *attribmod = NULL;      /* * attribute modification for wipe 2011 * */
char *namechart = NULL;
char *reroll = NULL;
char *bonus = NULL;
char *keepchar = NULL;
char *hometown_table = NULL;
char *alignment_table = NULL;
char *shutdown_message = NULL;
char *artilist_mortal_main = NULL;
char *artilist_mortal_ioun = NULL;
char *artilist_mortal_unique = NULL;

FILE *mob_f, /* * file containing mob prototypes  */
	*obj_f; /* * obj prototypes                  */
//      *help_fl;               /* * file for help texts (HELP <kwd>) */  This commented out by weebler

P_index mob_index; /* * index table for mobile file     */
P_index obj_index; /* * index table for object file     */

P_table obj_tables; /* for random obj tables */
P_table mob_tables; /* for random mob tables */

int num_mob_tables, num_obj_tables = 0;

struct info_index_element *info_index = 0;

int top_of_mobt = 0; /* * top of mobile index table * */
int top_of_objt = 0; /* * top of object index table * */
unsigned long next_obj_uid = 1; /* global counter for unique object ids */
int top_of_helpt; /* * top of help index table         */
int top_of_infot; /* * top of info index table         */

int no_mail = 0; /* Is mail system working this boot? */

struct time_info_data time_info; /* * the information about the time * */

struct mm_ds *dead_mob_pool = NULL;
struct mm_ds *dead_obj_pool = NULL;

P_index generate_indices(FILE *, int *);
namespace
{
void invalidate_recovery_object_templates() noexcept;
unsigned int prepare_recovery_object_templates() noexcept;
}

void assign_continents();

void init_rand_tables(int mini_mode);
void init_email_reg_db(void);
void dump_email_reg_db(void);
int email_in_use(char *, char *);

void release_obj_mem(P_obj obj);
void release_acct_mem(P_obj obj);

void apply_zone_modifier(P_char ch);

void release_mob_mem(P_char ch, P_char /*victim*/, P_obj /*obj*/, void * /*data*/)
{
	if (ch->in_room != NOWHERE && is_char_in_room(ch, ch->in_room))
	{
		debug("Freeing memory from char in room %d!", world[ch->in_room].number);
	}
	mm_release(dead_mob_pool, ch);
}

void release_obj_mem(P_obj obj)
{
	mm_release(dead_obj_pool, obj);
}

void release_acct_mem(P_obj obj)
{
	mm_release(dead_obj_pool, obj);
}

const char *MENU = "\
   &+W      Welcome to\r\n\
\r\n\
   &+RDuris: Land of Bloodlust\r\n\
\r\n\
&+L=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n\
 &+L[&+W0&+L]&n&+y Leave Duris for a Time.\r\n\
 &+L[&+W1&+L]&n&+y Enter the realms of Duris.\r\n\
 &+L[&+W2&+L]&n&+y Read the background story.\r\n\
 &+L[&+W3&+L]&n&+y Change your password.\r\n\
 &+L[&+W4&+L]&n&+y Enter your character description.\r\n\
 &+L[&+W5&+L]&n&+y Delete this character.\r\n\
&+L=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n\
\r\n\
&+WChoose an option:&n";

const char *BACKGR_STORY = "\r\nThe History of Time:\r\n\r\n \
&+R=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&N\r\n \
&+R=                                                                     =&N\r\n \
&+R=                    DURIS - The Land of Bloodlust                    =&N\r\n \
&+R=                                                                     =&N\r\n \
&+R=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=&N\r\n \
\r\n\r\n\
Peering out into the hazy gloom of the early dawn, you gaze upon the\r\n \
land of Duris for what seems like the first time. From the east, the\r\n \
first glimmers of the Black Sun, Dakhira, cast their evil light upon\r\n \
the planet. Perpetual twilight hugs the land, even in the breaking\r\n \
hours of the sunrise. Moving your eyes from the glowing horizon, you\r\n \
gaze out upon the vast windswept plains, worn smooth by the tread of\r\n \
the marching armies. Thus comes the time of the Bloodlust, you think,\r\n \
as idle thought ravages your thoughts. Here is where I shall die.\r\n \
\r\n\r\n\
As Dakhira slowly rises higher in the sky, chills and shakes wreck your\r\n \
frame and dark, evil energy seeps into your body. For now the urges\r\n \
can be fought, but soon you know the desire for blood and death will\r\n \
become too strong. Soon the hordes of the Darklords will be driven into\r\n \
a frenzy by the light of the sun and the great battle will be fought.\r\n \
Many souls shall depart this land today, and many rivulets of blood\r\n \
will feed the earth.\r\n \
\r\n\r\n\
Finally the whole sphere of the Black Sun passes over the horizon's\r\n \
edge and the rays of corrupted light filter through the early morning\r\n \
mist with a renewed intensity. Suddenly waves of weariness and fear\r\n \
overcome you as dark shapes are outlined against the lighter sky. The\r\n \
mounts of the Darklords, the blackest of Durian dragons fly! Trembling\r\n \
in fear you manage to look away from the sky towards your earthly\r\n \
place and then the cries attract you. There in the distance across the\r\n \
plains, where the hills begin, the dark hordes have begun to emerge\r\n \
onto the level ground.\r\n \
\r\n\r\n\
You want to run but your legs do not respond, for the energy of Dakhira\r\n \
now compels you to battle, compels you to the Bloodlust and your death.\r\n \
With a quick glance to your few comrades, you advance slowly upon the\r\n \
battlefield. A sudden determination and resolve overcomes you as the\r\n \
dark mass aproaches. Closer grow the hordes..closer...then sudden blasts\r\n \
of acid straif your back and you scream in agony. With a rush of wind,\r\n \
the Durian dragon ascends higher into the shadowy sky for another run,\r\n \
and the massive dark army closes.\r\n \
\r\n\r\n\
Stumbling to your feet, you are met with the onslaught of the front\r\n \
rank of the orcish horde. 'Comrades die with honor!' you scream as the\r\n \
ugly brute stabs at your breast. The ringing of steel and the crunch\r\n \
of wood sound from all around as the melee begins in ernest. Finally\r\n \
felling the orc sergeant, you plow into the ranks of the enemy with\r\n \
a flurry of slashes and faints. PAIN strikes you hard as an orc out-\r\n \
flanks you, driving his spear deep into your ribcage. Cracking bone\r\n \
and hot spewing blood send you reeling into collapse. Hanging on too\r\n \
life, you feel your blood flow from the wound but are vaguely aware of\r\n \
the battle passing you by as the swarms of goblins and orcs sweep past.\r\n \
\r\n\r\n \
As you fade in and out of unconsciousness, a white light pierces the\r\n \
darkness and manages to arouse you. Shimmering and blurring, you feel\r\n \
a comfort and peace wave over you, unknown for many many years. With\r\n \
a sigh of pleasure, you fade from life..your spirit freed from its\r\n \
mortal trappings. Riding slowly on the ethereal winds, higher into the\r\n \
sky a pull unlike any ever described or felt by you appears. Stronger\r\n \
and stronger the pull becomes, your soul only vaguely aware as it is\r\n \
sucked into the heart of the rift in the sky..as Dakhira grows and\r\n \
strengthens.\r\n \
\r\n\r\n \
Credits: Cython and Zarland\r\n \
See Also: HELP THEME on the mud itself\r\n\r\n\r\n";

const char *GREETINGS = "\r\n\r\n\
                                              __----~~~~~~~~~~~------__\r\n\
      Welcome to Duris DikuMUD     _//~    ~//====......          __--~ ~~\r\n\
                   -_       _..--+~o`\\     ||_   ~~~~~~::::... /~\r\n\
                ___-==_     `--=;_`_  \\   ||  -_             _/~~-\r\n\
        __---~~~.==~|-_=_     ~-~ _/~ |-   _||   -_        _/~\r\n\
   __--~     .=~    |  -_-_       /  /-   / ||     -_     / \r\n\
  =        .~       |    -_-_    /  /-   /   ||      -_  / \r\n\
 /  ____  /         |      - ~-_/  /|- _/   .||        -/ \r\n\
 |~~    ~~|--~~~~--_|_     ~==-/   | \\--===~~        ./ \r\n\
          '         ~|       /|    |-~\\~       __--~~\r\n\
                     |~~~~-_/ |   |   ~\\   _-~              /\\ \r\n\
 The land of the Bloodlust /  -_    -__  <--~                \\ \r\n\
                       _--~ _/ | .-~~____--~-/                ~~~===. \r\n\
                      ((->/~   '.|||' -_|    ~~-/ ,              . _|| \r\n\
                                 -_     -_      ~~---l__i__i__i--~~_/ \r\n\
                                 _-~-__   ~)  _=--_____________--~~\r\n\
                               //.-~~~-~_--~- |-------~~~~~~~~\r\n\
                                      //.-~~~--\\ \r\n\
                    Original Code: Hans Henrik, Katja Nyboe, \r\n\
              Tom Madsen, Michael Seifert, and Sebastian Hammer.\r\n\
\r\n\
                   ** Modified for Duris dikuMUD by **\r\n\
                     Lots and Lots of people!!!!!!!!!!\r\n\
                                          \r\n\r\n";

int fread_string_to_buffer(FILE *fl, char *buf)
{
	char tmp[MAX_STRING_LENGTH];
	char *point = NULL;
	int length = 0, t_length = 0, done = FALSE;

	buf[0] = '\0';

	do
	{
		if (!fgets(tmp, MAX_STRING_LENGTH - 5, fl))
		{
			fatal_boot_error(
				"db",
				"fread_string_to_buffer: unexpected EOF while reading string");
		}
		t_length = strlen(tmp);

		/* find the last non-whitespace char in tmp */
		for (point = tmp + t_length - 1; point > tmp && isspace(*point); point--)
			;

		if (*point == '~')
		{
			*point = '\0';
			done = TRUE;
		}
		else
		{
			point = tmp + t_length - 1;
			*point++ = '\r';
			*point++ = '\n';
			*point = '\0';
		}
		t_length = point - tmp;

		if (length + t_length >= MAX_STRING_LENGTH)
		{
			fatal_boot_error("db", "fread_string: string too large (db.c)");
		}
		else
		{
			strcat(buf + length, tmp);
			length += t_length;
		}
	} while (!done);

	for (point = buf + length - 1; point > buf && *point != '&'; point--)
		;
	if (point > buf && *point == '&' && toupper(*(point + 1)) != 'N')
	{
		strcat(buf, "&n");
		length += 2;
	}

	return length;
}

/*************************************************************************
 *  routines for booting the system                                       *
 *********************************************************************** */
/*
void loadGodProcs()
{
  FILE    *f;
  char     vnum_str[15];
  int      rn;

  f = fopen("Players/deathobjs", "r");
  if (!f)
  {
    logit(LOG_STATUS, "Error loading death procs.");
    return;
  }
  while (fgets(vnum_str, 15, f))
  {
    if (!(rn = real_object0(atoi(vnum_str))))
    {
      logit(LOG_STATUS, "Error loading death procs: no such item.");
    }
    else
    {
      obj_index[rn].god_func = death_proc;
    }
  }
  fclose(f);
}
*/

void boot_material_rarity_objects(int mini_mode)
{
	invalidate_recovery_object_templates();
	if (!mini_mode)
	{
		if (!(obj_f = fopen(OBJ_FILE, "r")))
			fatal_boot_error("db", "Trouble opening object file world.obj: %s",
					 strerror(errno));
	}
	else if (!(obj_f = fopen("areas_mini/mini.obj", "r")))
	{
		fatal_boot_error("db", "Trouble opening mini object file areas_mini/mini.obj: %s",
				 strerror(errno));
	}

	obj_index = generate_indices(obj_f, &top_of_objt);
	dead_obj_pool = mm_create("OBJS", sizeof(struct obj_data), offsetof(struct obj_data, next),
				  mm_find_best_chunk(sizeof(struct obj_data), (top_of_objt / 3),
						     (top_of_objt >> 1)));
	ne_init_event_pool();
}

/** Load world data and persistent authorities before entering the game loop. */

void boot_db(int mini_mode)
{
	invalidate_recovery_object_templates();
	logit(LOG_STATUS, "Boot db -- BEGIN.");
	fprintf(stderr, "\nBoot db -- BEGIN.\r\n");
	boot_time = time(0);
	if (!persistence_mode_requires_mysql())
	{
		std::string error;
		const auto ensured =
			flatfile_artifact_ensure(persistence_mode_flatfile_root(), &error);
		if (ensured != flatfile_artifact_result::ok &&
		    ensured != flatfile_artifact_result::already_exists)
			fatal_boot_error("db", "Could not establish flat artifact catalog: %s",
					 error.empty() ? "invalid artifact authority" :
							 error.c_str());
	}

	logit(LOG_STATUS, "Resetting the game time:");
	fprintf(stderr, "Resetting the game time:\r\n");
	reset_time();

	fprintf(stderr, "Reading files from lib directory. (motd, wizlist, etc)\r\n");
	logit(LOG_STATUS, "Reading newsfile.");
	//  news = file_to_string(NEWS_FILE);
	news = get_mud_info("news");

	logit(LOG_STATUS, "Reading projectsfile.");
	projects = file_to_string(PROJECTS_FILE);

	logit(LOG_STATUS, "Reading Ansi motd.");
	//  motd = file_to_string(MOTD_FILE);
	motd = get_mud_info("motd");

	logit(LOG_STATUS, "Reading Ansi wizmotd.");
	//  wizmotd = file_to_string(WIZMOTD_FILE);
	wizmotd = get_mud_info("wizmotd");

	logit(LOG_STATUS, "Reading help.");
	help = file_to_string(HELP_PAGE_FILE);
	logit(LOG_STATUS, "Reading rules.");
	rules = file_to_string(RULES_FILE);
	logit(LOG_STATUS, "Reading Ansi 1 login screen.");
	greetinga = file_to_string(GREETINGA_FILE);
	logit(LOG_STATUS, "Reading Ansi 2 login screen.");
	greetinga1 = file_to_string(GREETINGA1_FILE);
	logit(LOG_STATUS, "Reading ANSI 3 login screen.");
	greetinga2 = file_to_string(GREETINGA2_FILE);
	logit(LOG_STATUS, "Reading ANSI 4 login screen.");
	greetinga3 = file_to_string(GREETINGA3_FILE);
	logit(LOG_STATUS, "Reading ANSI 5 login screen.");
	greetinga4 = file_to_string(GREETINGA4_FILE);
	logit(LOG_STATUS, "Reading ASCII login screen.");
	greetings = file_to_string("lib/information/greeting");
	logit(LOG_STATUS, "Reading Ansi wizlist.");
	wizlista = file_to_string(WIZLISTA_FILE);
	logit(LOG_STATUS, "Reading disclaimer.");
	disclaimer = file_to_string(DISCLAIMER_FILE);
	logit(LOG_STATUS, "Reading bug file.");
	bugfile = file_to_string(BUG_FILE);
	logit(LOG_STATUS, "Reading race/class comparison table.");
	generaltable = file_to_string(GENERALTABLE_FILE);
	logit(LOG_STATUS, "Reading Race table.");
	racetable = file_to_string(RACETABLE_FILE);
	//  logit(LOG_STATUS, "Reading Attribute Mod Message(wipe 2011)");
	//  attribmod = file_to_string(ATTRIBMOD_FILE);
	logit(LOG_STATUS, "Reading Class table.");
	classtable = file_to_string(CLASSTABLE_FILE);
	logit(LOG_STATUS, "Reading Racewars explanation.");
	racewars = file_to_string(RACEWARS_FILE);
	logit(LOG_STATUS, "Reading Namechart message.");
	namechart = file_to_string(NAMECHART_FILE);
	logit(LOG_STATUS, "Reading Reroll message.");
	reroll = file_to_string(REROLL_FILE);
	logit(LOG_STATUS, "Reading Bonus message.");
	bonus = file_to_string(BONUS_FILE);
	logit(LOG_STATUS, "Reading Keepchar message.");
	keepchar = file_to_string(KEEPCHAR_FILE);
	logit(LOG_STATUS, "Reading Hometown_table message.");
	hometown_table = file_to_string(HOMETOWN_FILE);
	logit(LOG_STATUS, "Reading Alignment_table message.");
	alignment_table = file_to_string(ALIGNMENT_FILE);
	logit(LOG_STATUS, "Reading Shutdown Ansi.");
	shutdown_message = file_to_string(SHUTDOWN_FILE);
	logit(LOG_STATUS, "Getting PC id numb info.");
	setNewPCidNumbfromFile();
	portal_id = 0; // if someone knows a better place to put this, feel free to move it
	logit(LOG_STATUS, "Reading in short desc tables.");
	boot_desc_data();
	fprintf(stderr, "Opening mobile, object, help and info files.\r\n");
	logit(LOG_STATUS, "Opening mobile, object, help and info files.");
	// mob_f and obj_f stay open for the life of the process on purpose: read_mobile()
	// and read_object() fseek into them every time a prototype is instantiated, so
	// these are not descriptors to close after boot. Valgrind reports them as open at
	// exit, which is expected rather than a leak.
	if (!mini_mode)
	{
		if (!(mob_f = fopen(MOB_FILE, "r")))
		{
			fatal_boot_error("db", "Trouble opening mobile file world.mob: %s",
					 strerror(errno));
		}
		if (!(obj_f = fopen(OBJ_FILE, "r")))
		{
			fatal_boot_error("db", "Trouble opening object file world.obj: %s",
					 strerror(errno));
		}
	}
	else
	{
		if (!(mob_f = fopen("areas_mini/mini.mob", "r")))
		{
			fatal_boot_error("db",
					 "Trouble opening mini mobile file areas_mini/mini.mob: %s",
					 strerror(errno));
		}
		if (!(obj_f = fopen("areas_mini/mini.obj", "r")))
		{
			fatal_boot_error("db",
					 "Trouble opening mini object file areas_mini/mini.obj: %s",
					 strerror(errno));
		}
	}

	fprintf(stderr, "Loading zone table.\r\n");
	logit(LOG_STATUS, "Loading zone table.");
	boot_zones(mini_mode);

	fprintf(stderr, "Loading rooms.\r\n");
	logit(LOG_STATUS, "Loading rooms.");
	boot_world(mini_mode);

	fprintf(stderr, "Renumbering rooms.\r\n");
	logit(LOG_STATUS, "Renumbering rooms.");
	renum_world();

	fprintf(stderr, "Generating index table for mobiles.\r\n");
	logit(LOG_STATUS, "Generating index table for mobiles.");
	mob_index = generate_indices(mob_f, &top_of_mobt);

	fprintf(stderr, "Generating index table for objects.\r\n");
	logit(LOG_STATUS, "Generating index table for objects.");
	obj_index = generate_indices(obj_f, &top_of_objt);

	/*
	 * load_obj_limits();
	 */

	fprintf(stderr, "Renumbering zone table.\r\n");
	logit(LOG_STATUS, "Renumbering zone table.");
	renum_zone_table();

	fprintf(stderr, "Initializing Random Load Tables.\r\n");
	logit(LOG_STATUS, "Initializing Random Load Tables.");
	init_rand_tables(mini_mode);

	if (0)
	{ /* EMAIL registration  */
		fprintf(stderr, "Initializing EMAIL registration table.\n\r");
		init_email_reg_db();
	}

	fprintf(stderr, "Loading social messages.\r\n");
	logit(LOG_STATUS, "Loading social messages.");
	boot_social_messages();

	if (!mini_mode)
	{
		fprintf(stderr, "Initializing boards.\r\n");
		logit(LOG_STATUS, "Initializing boards..");
		initialize_boards();
	}

	fprintf(stderr, "Loading pose messages.\r\n");
	logit(LOG_STATUS, "Loading pose messages.");
	boot_pose_messages();

	/*
	 * before loading any mobs, initialize the memory management for
	 * structs that will be used for mobiles (and objects)
	 */

	dead_mob_pool = mm_create("CHARS", sizeof(struct char_data),
				  offsetof(struct char_data, next),
				  mm_find_best_chunk(sizeof(struct char_data), (top_of_mobt >> 3),
						     (top_of_mobt >> 1)));

	dead_obj_pool = mm_create("OBJS", sizeof(struct obj_data), offsetof(struct obj_data, next),
				  mm_find_best_chunk(sizeof(struct obj_data), (top_of_objt / 3),
						     (top_of_objt >> 1)));

	if (!no_specials)
	{
		fprintf(stderr, "Assigning function pointers (spec procs):\r\n");
		logit(LOG_STATUS, "Assigning function pointers:");

		logit(LOG_STATUS, "   Mobiles.");
		fprintf(stderr, "-- Mobile special procedures.\r\n");
		assign_mobiles();

		logit(LOG_STATUS, "   Objects.");
		fprintf(stderr, "-- Object special procedures.\r\n");
		assign_objects();

		logit(LOG_STATUS, "   Room.");
		fprintf(stderr, "-- Room special procedures.\r\n");
		assign_rooms();
	}
	boot_zone_story_quest_state();

	fprintf(stderr, "Assigning command pointers from interpreter.\r\n");

	fprintf(stderr, "-- Commands.\n");
	logit(LOG_STATUS, "   Commands.");
	assign_command_pointers();

	fprintf(stderr, "-- Spells.\n");
	logit(LOG_STATUS, "   Spells.");
	assign_spell_pointers();

	npc_alchemist_cache_templates();

	// Parse starter prototypes before any descriptors can request a kit.
	for (int vnum : newbie_kit_template_vnums())
		if (!cache_object_template(vnum))
			logit(LOG_STATUS, "Starter template VNUM %d is unavailable", vnum);

	/* Load areas/world.trg and bind the generic zone procs.  Must run
	   after assign_spell_pointers() -- the .trg parser resolves spell
	   names through spells[] -- and before ne_init_events(), which asks
	   every bound room proc whether it wants a periodic tick. */
	studioproc_boot();

	fprintf(stderr, "Initializing...\n");

	fprintf(stderr, "-- Innates\n");
	assign_innates();

	fprintf(stderr, "-- Links\n");
	initialize_links();

	/*
	 * logit(LOG_STATUS, "Init Grants"); assign_grant_commands();
	 */
	fprintf(stderr, "-- Weather\n");
	logit(LOG_STATUS, "Setting up weather.");
	weather_setup(mini_mode);

	fprintf(stderr, "-- Banned sites\n");
	logit(LOG_STATUS, "Reading ban sites.");
	read_ban_file();

	logit(LOG_STATUS, "Reading wizconnect sites.");
	read_wizconnect_file();

	fprintf(stderr, "-- Events\n");
	logit(LOG_STATUS, "Initializing event driver.");
	ne_init_events();

	/*
	 * can't do the dynamic proc lib loading until AFTER the event driver
	 * is started.  Some of the proc libs might try to start events in the
	 * _init() function.  (ie: bloodstone gate)
	 */

#ifdef SHLIB
	if (!no_specials)
	{
		fprintf(stderr, "Loading dynamic proc libs (and assigning pointers):\r\n");
		logit(LOG_STATUS, "Loading dynamic proc libs");
		load_all_proc_libs();
	}
#endif

	if (!mini_mode)
	{
		fprintf(stderr, "-- Ships\n");
		logit(LOG_STATUS, "Initializing ships.");
		initialize_ships();

		logit(LOG_STATUS, "Initializing Arena.");
		initialize_arena();

		logit(LOG_STATUS, "Setting up Carriages and wagons.");
		init_wagons();
	}

	// Parse complete SQL recovery values after existing boot binders, before
	// populated native keeper sidecars need them during persistent restoration.
	// Failure only closes that recovery prerequisite; existing boot/loading stays
	// available and no partially prepared catalog becomes visible.
	if (persistence_mode_requires_mysql())
	{
		const auto error = prepare_recovery_object_templates();
		if (error)
			logit(LOG_STATUS, "SQL recovery object-template catalog unavailable (%u)",
			      error);
	}

	fprintf(stderr, "-- Mail\n");
	logit(LOG_STATUS, "Booting mail system.");
	if (!scan_mail_file())
	{
		logit(LOG_DEBUG, "Mail system error -- mail system disabled!");
		no_mail = 1;
	}

	if (!mini_mode)
	{
		/* Copyover carries the complete live ground-object graph, including player
		 * corpses.  Loading SQL corpses first materializes their stable child UIDs
		 * under a newly allocated root and makes recovery reject the same children
		 * as duplicates.  A cold boot still restores the durable SQL image. */
		if (!copyover_boot)
		{
			fprintf(stderr, "-- Player corpses\n");
			logit(LOG_STATUS, "Reloading Player corpses.");
			restoreCorpses();
		}

		/* Saved ground/storage objects are in the same copyover world graph. */
		if (!copyover_boot)
		{
			logit(LOG_STATUS, "Reloading SavedItems.");
			restoreSavedItems();
		}

		fprintf(stderr, "-- Shopkeepers\n");
		logit(LOG_STATUS, "Reloading Shopkeepers.");
		// Current copyovers commit full shop stock before handoff. Legacy files
		// only have their live NPC inventory; Redis never stores shop stock.
		if (!copyover_boot || copyover_has_durable_shopkeepers() ||
		    persistence_mode_get() == PERSISTENCE_MODE_FLATFILE_PRIMARY)
			restore_shopkeepers();
		remember_boot_shopkeepers();

		fprintf(stderr, "-- Associations\n");
		logit(LOG_STATUS, "Updating associations table.");
		sql_update_assoc_table();

		logit(LOG_STATUS, "Loading patrol Justice area.");
		load_justice_area();

		logit(LOG_STATUS, "Setting up player-side artifact list.");
		setupMortArtiList_sql();
		// skip loading artifacts from db during copyover - they're restored from copyover.dat
		if (!copyover_boot)
		{
			/* Redis recovery owns floor materialization when its validated generation
			 * is active. Loading the legacy vnum-only artifact row first would create
			 * a fresh-UID duplicate of the authoritative recovered object. */
			if (!redis_world_recovery_boot_active())
				addOnGroundArtis_sql();
			addOnMobArtis_sql();
		}
	}
	else
	{
		fprintf(stderr, "--  Skipping full-world state restoration in mini mode.\r\n");
	}

	fprintf(stderr, "-- Continents\n");
	assign_continents();
	training_dummy_bootstrap();

	//  logit(LOG_STATUS, "Setting up god object procedures.");
	//  loadGodProcs();

	logit(LOG_STATUS, "Boot db -- DONE.");
}

void update_stat_data()
{
	char buf[128];
	int i;

	for (i = 1; i <= LAST_RACE; i++)
	{
		snprintf(buf, 128, "stats.str.%s", race_names_table[i].no_spaces);
		stat_factor[i].Str = (sh_int)get_property(buf, 100.);
		snprintf(buf, 128, "stats.dex.%s", race_names_table[i].no_spaces);
		stat_factor[i].Dex = (sh_int)get_property(buf, 100.);
		snprintf(buf, 128, "stats.agi.%s", race_names_table[i].no_spaces);
		stat_factor[i].Agi = (sh_int)get_property(buf, 100.);
		snprintf(buf, 128, "stats.con.%s", race_names_table[i].no_spaces);
		stat_factor[i].Con = (sh_int)get_property(buf, 100.);
		snprintf(buf, 128, "stats.pow.%s", race_names_table[i].no_spaces);
		stat_factor[i].Pow = (sh_int)get_property(buf, 100.);
		snprintf(buf, 128, "stats.int.%s", race_names_table[i].no_spaces);
		stat_factor[i].Int = (sh_int)get_property(buf, 100.);
		snprintf(buf, 128, "stats.wis.%s", race_names_table[i].no_spaces);
		stat_factor[i].Wis = (sh_int)get_property(buf, 100.);
		snprintf(buf, 128, "stats.cha.%s", race_names_table[i].no_spaces);
		stat_factor[i].Cha = (sh_int)get_property(buf, 100.);
		snprintf(buf, 128, "stats.kar.%s", race_names_table[i].no_spaces);
		stat_factor[i].Kar = (sh_int)get_property(buf, 100.);
		snprintf(buf, 128, "stats.luc.%s", race_names_table[i].no_spaces);
		stat_factor[i].Luk = (sh_int)get_property(buf, 100.);
	}
}

/* reset the time in the game from file */
void reset_time(void)
{
	long beginning_of_time = 650336715;

	time_info = mud_time_passed(time(0), beginning_of_time);

	logit(LOG_STATUS, "   Current Gametime:  %d/%d/%d  %d%s", time_info.month, time_info.day,
	      time_info.year, (time_info.hour % 12) ? time_info.hour % 12 : 12,
	      (time_info.hour == 12) ? " noon." :
	      (time_info.hour == 0)  ? " midnight." :
	      (time_info.hour > 11)  ? "pm." :
				       "am.");
}

void weather_setup(int mini_mode)
{
	int zon, s, i;
	int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9, tmp10, tmp11, tmp12;
	FILE *fl;

	/* default conditions for season values */
	const signed char winds[6] = { 2, 12, 30, 40, 50, 80 };
	const signed char precip[9] = { 0, 1, 5, 10, 15, 25, 35, 45, 60 };
	const signed char humid[9] = { 4, 10, 20, 30, 40, 50, 60, 75, 100 };
	const signed char temps[11] = { -15, -8, 0, 10, 17, 27, 33, 40, 50, 75, 100 };

	const char *weather_file = mini_mode ? "areas_mini/world.weather" : "areas/world.weather";
	if (!(fl = fopen(weather_file, "r")))
	{
		fatal_boot_error("db", "weather_setup: could not open %s: %s", weather_file,
				 strerror(errno));
	}
	for (zon = 0; zon <= 99; zon++)
	{
		for (i = 0; i < 4; i++)
		{
			sector_table[zon].climate.season_wind_dir[i] = number(0, 3);
			sector_table[zon].climate.season_wind_variance[i] = number(0, 1);
		}
		sector_table[zon].climate.flags = 0;
		sector_table[zon].climate.energy_add = number(0, 1000);
		REQUIRED_FSCANF(fl, " %d %d %d %d %d %d %d %d %d %d %d %d \n", &tmp1, &tmp2, &tmp3,
				&tmp4, &tmp5, &tmp6, &tmp7, &tmp8, &tmp9, &tmp10, &tmp11, &tmp12);
		sector_table[zon].climate.season_wind[0] = tmp1;
		sector_table[zon].climate.season_precip[0] = tmp2;
		sector_table[zon].climate.season_temp[0] = tmp3;
		sector_table[zon].climate.season_wind[1] = tmp4;
		sector_table[zon].climate.season_precip[1] = tmp5;
		sector_table[zon].climate.season_temp[1] = tmp6;
		sector_table[zon].climate.season_wind[2] = tmp7;
		sector_table[zon].climate.season_precip[2] = tmp8;
		sector_table[zon].climate.season_temp[2] = tmp9;
		sector_table[zon].climate.season_wind[3] = tmp10;
		sector_table[zon].climate.season_precip[3] = tmp11;
		sector_table[zon].climate.season_temp[3] = tmp12;

		/* get the season */
		s = get_season(zon);

		/* These are pretty standard start values */
		sector_table[zon].conditions.pressure = 980;
		sector_table[zon].conditions.free_energy = 10000;
		sector_table[zon].conditions.precip_depth = 0;
		sector_table[zon].conditions.flags = 0;

		/* These use the default conditions above */
		sector_table[zon].conditions.windspeed =
			ARR_GET(winds, sector_table[zon].climate.season_wind[s]);

		sector_table[zon].conditions.wind_dir =
			sector_table[zon].climate.season_wind_dir[s];

		sector_table[zon].conditions.precip_rate =
			ARR_GET(precip, sector_table[zon].climate.season_precip[s]);

		sector_table[zon].conditions.temp =
			ARR_GET(temps, sector_table[zon].climate.season_temp[s]);

		sector_table[zon].conditions.humidity =
			ARR_GET(humid, sector_table[zon].climate.season_precip[s]);

		/* Set ambient light */
		calc_light_zone(zon);
	}
	fclose(fl);
}

/* generate random mob and obj tables */
void init_rand_tables(int mini_mode)
{
	uint mtables, otables; /* counts for tables */
	int v, w;
	FILE *tfile; /* table file */
	char buf[MAX_STRING_LENGTH];
	unsigned int tmp;
	int pos;

	mob_tables = 0;
	obj_tables = 0;
	mtables = 0;
	otables = 0;

	const char *table_file = mini_mode ? "areas_mini/world.tab" : "areas/world.tab";
	if (!(tfile = fopen(table_file, "r")))
	{
		fatal_boot_error("db", "boot_tables: could not open %s: %s", table_file,
				 strerror(errno));
	}

	/* First, count the number of each table. */
	for (;;)
	{
		REQUIRED_FGETS(buf, 81, tfile);
		if (buf[0] == '$')
			break; /*eof */
		if (buf[0] == 'M')
			mtables++;
		if (buf[0] == 'O')
			otables++;
	}
	rewind(tfile);
	CREATE(mob_tables, table_data, (unsigned)mtables, MEM_TAG_TBLDATA);
	CREATE(obj_tables, table_data, (unsigned)otables, MEM_TAG_TBLDATA);
	//  mob_tables =
	//    (struct table_data *) calloc(mtables, sizeof(struct table_data));
	//  obj_tables =
	//    (struct table_data *) calloc(otables, sizeof(struct table_data));
	num_mob_tables = mtables;
	num_obj_tables = otables; /* globals for db.c */

	/* now, go through and create each table */
	mtables = otables = 0;
	for (;;)
	{
		REQUIRED_FGETS(buf, 81, tfile);
		if (buf[0] == '$')
			break; /* EOF */
		switch (buf[0])
		{
		case 'M': /* new mob table */
			sscanf(buf, "M %d %d\n", &v, &w);
			mob_tables[mtables].virtual_number = v;
			mob_tables[mtables].empty_weight = w;
			pos = ftell(tfile); /* remember loc */
			/* count # of entries */
			tmp = 0;
			for (;;)
			{
				REQUIRED_FGETS(buf, 81, tfile);
				if (buf[0] == 'S')
					break; /*end of table */
				tmp++;
			}
			fseek(tfile, pos, 0);
			CREATE(mob_tables[mtables].table, table_element, tmp, MEM_TAG_TBLELEM);

			mob_tables[mtables].entries = tmp;
			mob_tables[mtables].weight = mob_tables[mtables].empty_weight;
			tmp = 0;
			for (;;)
			{
				REQUIRED_FGETS(buf, 81, tfile);
				if (buf[0] == 'S')
					break;
				sscanf(buf, "%d %d", &v, &w);
				mob_tables[mtables].table[tmp].virtual_number = v;
				mob_tables[mtables].table[tmp].weight = w;
				mob_tables[mtables].weight += w; /* total weight */
				tmp++;
			}
			mtables++;
			break;
		case 'O': /* new obj table */
			sscanf(buf, "O %d %d", &v, &w);
			obj_tables[otables].virtual_number = v;
			obj_tables[otables].empty_weight = w;
			pos = ftell(tfile);
			tmp = 0;
			for (;;)
			{
				REQUIRED_FGETS(buf, 81, tfile);
				if (buf[0] == 'S')
					break;
				tmp++;
			}
			fseek(tfile, pos, 0);
			CREATE(obj_tables[otables].table, table_element, tmp, MEM_TAG_TBLELEM);

			obj_tables[otables].entries = tmp;
			obj_tables[otables].weight = obj_tables[otables].empty_weight;
			tmp = 0;
			for (;;)
			{
				REQUIRED_FGETS(buf, 81, tfile);
				if (buf[0] == 'S')
					break;
				sscanf(buf, "%d %d", &v, &w);
				obj_tables[otables].table[tmp].virtual_number = v;
				obj_tables[otables].table[tmp].weight = w;
				obj_tables[otables].weight += w;
				tmp++;
			}
			otables++;
			break;
		default:
			break;
		}
	}
	fclose(tfile);
};

/* generate index table for object or monster file */
P_index generate_indices(FILE *fl, int *top)
{
	int i = 0, num;
	P_index t_idx;
	char buf[512];

	rewind(fl);

	/* first time just count */
	rewind(fl);
	num = 0;
	for (;;)
	{
		if (!fgets(buf, 511, fl))
			break; /* tolerate EOF-terminated legacy files */
		if (*buf == '$')
			break;
		if (*buf == '#')
			num++;
	}

	logit(LOG_STATUS, "\t\t%d entries allocated", num);

	/* allocate array of index_data */
	CREATE(t_idx, index_data, (unsigned)num, MEM_TAG_IDXDATA);

	rewind(fl);

	for (;;)
	{
		if (fgets(buf, 511, fl))
		{
			if (*buf == '#')
			{
				sscanf(buf, "#%d", &t_idx[i].virtual_number);
				t_idx[i].pos = ftell(fl);
				t_idx[i].number = 0;
				t_idx[i].func.mob = NULL;
				t_idx[i].qst_func = NULL;
				t_idx[i].keys = NULL;
				t_idx[i].desc1 = NULL;
				t_idx[i].desc2 = NULL;
				t_idx[i].desc3 = NULL;
				if (i && (t_idx[i - 1].virtual_number >= t_idx[i].virtual_number))
					logit(LOG_DEBUG, "Warning: index (%d, %d) out of order.",
					      t_idx[i - 1].virtual_number, t_idx[i].virtual_number);
				i++;
			}
			else if (*buf == '$') /* EOF  */
				break;
		}
		else
		{
			break; /* EOF-terminated legacy files */
		}
	}
	const bool has_sentinel = i > 0 && t_idx[i - 1].virtual_number == 9999999;
	*top = i - (has_sentinel ? 2 : 1);
	return (t_idx);
}

/* load the rooms */
void boot_world(int mini_mode)
{
	FILE *fl;
	int num_rooms, room_nr = 0, zone = 0, virtual_nr;
	int tmp = 0, tmp1 = 0, tmp2 = 0, tmp3 = 0, i, name_length, desc_length;
	char chk[MAX_STRING_LENGTH], tmp_buf[MAX_STRING_LENGTH];
	char buf[MAX_INPUT_LENGTH];
	char name_buf[MAX_STRING_LENGTH] = { 0 }, desc_buf[MAX_STRING_LENGTH] = { 0 };
	struct extra_descr_data *new_descr;
	bool found_name, found_desc;

	world = 0;
	character_list = 0;
	object_list = 0;

	if (mini_mode != 1)
	{
		if (!(fl = fopen(WORLD_FILE, "r")))
		{
			perror("fopen");
			fatal_boot_error("db", "boot_world: could not open world file");
		}
	}
	else if (mini_mode == 1)
	{
		if (!(fl = fopen("areas_mini/mini.wld", "r")))
		{
			fatal_boot_error("db", "boot_world: fopen failed: %s", strerror(errno));
		}
	}

	fseek(fl, 0, SEEK_END);
	size_t fsize = ftell(fl);

	char *memBuf = (char *)malloc(fsize + 1);
	if (!memBuf)
	{
		fatal_boot_error("db", "boot_world: could not allocate memory for world file");
	}

	fseek(fl, 0, SEEK_SET);
	size_t bytesRead = fread(memBuf, sizeof(char), fsize, fl);
	if (bytesRead != fsize)
	{
		free(memBuf);
		fatal_boot_error("db", "boot_world: short read while loading world file");
	}
	memBuf[fsize] = '\0';
	fclose(fl);

	fl = fmemopen(memBuf, fsize, "r");
	if (!fl)
	{
		fatal_boot_error("db", "boot_world: could not open memory stream for world file");
	}

	/* Count the number of rooms, to make allocation more efficient!! */
	num_rooms = 0;
	for (;;)
	{
		if (!fgets(tmp_buf, MAX_STRING_LENGTH, fl))
			break;
		if (tmp_buf[0] == '$')
			break;
		if (tmp_buf[0] == '#')
			num_rooms++;
	}

	logit(LOG_STATUS, "\t\t%d rooms allocated", num_rooms);
	rewind(fl);

	/* Allocate array of room structures */
	CREATE(world, room_data, (unsigned)num_rooms, MEM_TAG_ROOMDAT);

	logit(LOG_STATUS, "\t\tMemory allocation complete");

	/*
	 * allocate array of pointers to pointers for list of unique room
	 * descs, this list will speed searching.  It is freed after all rooms
	 * are read into memory.  The point?  Duplicate room descs are only
	 * allocated once. All rooms with identical descs will all use the
	 * same desc (mainly oceans, but there are other duplications as
	 * well). JAB
	 */

	for (;;)
	{
		if (fscanf(fl, " #%d\n", &virtual_nr) != 1)
			break;
		if (mini_mode == 2)
			fprintf(stderr, "#%d  ", virtual_nr);

		name_length = fread_string_to_buffer(fl, name_buf);
		if (*name_buf == '$')
			break;
		/* a new record to be read */
		world[room_nr].number = virtual_nr;
		desc_length = fread_string_to_buffer(fl, desc_buf);
		found_name = FALSE;
		found_desc = (desc_length == 0);
		// code looking up duplicate room names and descriptions to save memory
		for (i = room_nr - 1; i > (zone ? zone_table[zone - 1].real_top + 1 : 0) &&
				      (!found_name || !found_desc) && room_nr - i < 102;
		     i--)
		{
			if (!found_name && !strcmp(name_buf, world[i].name))
			{
				world[room_nr].name = world[i].name;
				found_name = TRUE;
			}
			if (!found_desc && world[i].description &&
			    !strcmp(desc_buf, world[i].description))
			{
				world[room_nr].description = world[i].description;
				found_desc = TRUE;
			}
		}
		// end of memory preserving code

		if (!found_name)
		{
			CREATE(world[room_nr].name, char, (unsigned)name_length + 1,
			       MEM_TAG_STRING);
			//        world[room_nr].name = (char *) calloc(name_length + 1, sizeof(char));
			strcpy(world[room_nr].name, name_buf);
		}
		if (!found_desc)
		{
			CREATE(world[room_nr].description, char, (unsigned)desc_length + 1,
			       MEM_TAG_STRING);
			//        world[room_nr].description =
			//          (char *) calloc(desc_length + 1, sizeof(char));
			strcpy(world[room_nr].description, desc_buf);
		}

		/* A few presets, may get changed further down */

		world[room_nr].continent = 0;
		world[room_nr].funct = 0;
		world[room_nr].contents = 0;
		world[room_nr].people = 0;
		world[room_nr].light = 0;
		world[room_nr].justice_area = 0;
		for (tmp = 0; tmp <= (NUM_EXITS - 1); tmp++)
			world[room_nr].dir_option[tmp] = 0;
		world[room_nr].ex_description = 0;
		world[room_nr].chance_fall = 0;
		world[room_nr].current_speed = 0;
		world[room_nr].current_direction = -1;
		if (top_of_zone_table >= 0)
		{
			if (world[room_nr].number <= (zone ? zone_table[zone - 1].top : -1))
			{
				logit(LOG_DEBUG, "Room nr %d (%d) is below zone %d.\n", room_nr,
				      world[room_nr].number, zone);
				fatal_boot_error("db", "boot_world: room %d (%d) is below zone %d",
						 room_nr, world[room_nr].number, zone);
			}
			while (world[room_nr].number > zone_table[zone].top)
				if (++zone > top_of_zone_table)
				{
					logit(LOG_DEBUG, "Room %d is outside of any zone.\n",
					      virtual_nr);
					fatal_boot_error(
						"db", "boot_world: room %d is outside of any zone",
						virtual_nr);
				}
			world[room_nr].zone = zone;
			if (zone_table[zone].real_bottom == -1)
				zone_table[zone].real_bottom = room_nr;
			zone_table[zone].real_top = room_nr;
		}

		/* tmp is the zone. Never used, and don't ask why :P */

		REQUIRED_FGETS(buf, sizeof(buf) - 1, fl);
		if (sscanf(buf, " %d %d %d %d\n", &tmp, &tmp1, &tmp2, &tmp3) == 4)
		{
			world[room_nr].room_flags = tmp1;
			world[room_nr].sector_type = tmp2;
			//        world[room_nr].resources = tmp3;
		}
		else if (sscanf(buf, " %d %d %d\n", &tmp, &tmp1, &tmp2) == 3)
		{
			world[room_nr].room_flags = tmp1;
			world[room_nr].sector_type = tmp2;
		}
		/* fix a few things */

		if (IS_ROOM(room_nr, ROOM_NO_MAGIC))
			if (!IS_ROOM(room_nr, ROOM_NO_SUMMON))
				SET_BIT(world[room_nr].room_flags, ROOM_NO_SUMMON);
		if (IS_ROOM(room_nr, ROOM_JAIL))
			if (!IS_ROOM(room_nr, ROOM_SAFE))
				SET_BIT(world[room_nr].room_flags, ROOM_SAFE);
		if ((zone_table[zone].flags & ZONE_MAP) &&
		    (SECT_CITY == world[room_nr].sector_type))
			world[room_nr].sector_type = SECT_ROAD;

		// Make roads no gate..
		if (world[room_nr].sector_type == SECT_ROAD)
		{
			SET_BIT(world[room_nr].room_flags, ROOM_NO_GATE);
			SET_BIT(world[room_nr].room_flags, ROOM_NO_TELEPORT);
		}
		// ADD NO PORT

		for (;;)
		{
			if (fscanf(fl, " %65535s \n", chk) != 1)
				break;

			if (*chk == 'D') /* direction field  */
				setup_dir(fl, room_nr, atoi(chk + 1));
			else if (*chk == 'E')
			{ /* extra description field */
				CREATE(new_descr, struct extra_descr_data, 1, MEM_TAG_EXDESCD);
				new_descr->keyword = fread_string(fl);
				new_descr->description = fread_string(fl);
				new_descr->next = world[room_nr].ex_description;
				world[room_nr].ex_description = new_descr;
			}
			else if (*chk == 'F')
			{
				REQUIRED_FSCANF(fl, "%d ", &tmp);
				world[room_nr].chance_fall = tmp;
			}
			else if (*chk == 'C')
			{
				REQUIRED_FSCANF(fl, "%d %d ", &tmp, &tmp2);
				world[room_nr].current_speed = tmp;
				world[room_nr].current_direction = tmp2;
			}
			else if (*chk == 'S')
				break;
		}
		if (world[room_nr].sector_type == SECT_INSIDE)
		{
			SET_BIT(world[room_nr].room_flags, ROOM_INDOORS);
			SET_BIT(world[room_nr].room_flags, ROOM_NO_PRECIP);
		}
		if ((world[room_nr].sector_type == SECT_NO_GROUND) &&
		    (!world[room_nr].dir_option[5] ||
		     (world[room_nr].dir_option[5]->to_room == room_nr)))
		{
			world[room_nr].sector_type = SECT_INSIDE;
		}
		if ((world[room_nr].chance_fall > 0) &&
		    (!world[room_nr].dir_option[5] ||
		     (world[room_nr].dir_option[5]->to_room == room_nr)))
		{
			world[room_nr].chance_fall = 0;
		}
		if (world[room_nr].room_flags & ROOM_INN)
			world[room_nr].funct = inn;
		if ((room_nr >= 65201) && (room_nr <= 65300))
		{
			world[room_nr].room_flags |= ROOM_LOCKER;
			world[room_nr].funct = storage_locker;
		}

		room_light(room_nr, REAL);
		room_nr++;
	}

	fclose(fl);
	free(memBuf);
	top_of_world = --room_nr;

	recalc_zone_numbers();
}

void free_world()
{
	invalidate_recovery_object_templates();
	std::unordered_set<char *> freed_room_strings;
	for (int room = 0; room <= top_of_world; room++)
	{
		if (world[room].name && freed_room_strings.insert(world[room].name).second)
			FREE(world[room].name);
		if (world[room].description &&
		    freed_room_strings.insert(world[room].description).second)
			FREE(world[room].description);

		while (world[room].ex_description)
		{
			struct extra_descr_data *description = world[room].ex_description;
			world[room].ex_description = description->next;
			if (description->keyword)
				FREE(description->keyword);
			if (description->description)
				FREE(description->description);

			FREE(description);
		}

		for (int dir = 0; dir < NUM_EXITS; dir++)
		{
			if (world[room].dir_option[dir])
			{
				if (world[room].dir_option[dir]->general_description)
					FREE(world[room].dir_option[dir]->general_description);
				if (world[room].dir_option[dir]->keyword)
					FREE(world[room].dir_option[dir]->keyword);

				FREE(world[room].dir_option[dir]);
			}
		}
	}

	FREE(world);
	freed_room_strings.clear();

	for (int mob = 0; mob <= top_of_mobt; mob++)
	{
		if (mob_index[mob].keys)
			FREE(mob_index[mob].keys);

		if (mob_index[mob].desc1)
			FREE(mob_index[mob].desc1);

		if (mob_index[mob].desc2)
			FREE(mob_index[mob].desc2);

		if (mob_index[mob].desc3)
			FREE(mob_index[mob].desc3);
	}
	FREE(mob_index);

	for (int obj = 0; obj <= top_of_objt; obj++)
	{
		if (obj_index[obj].keys)
			FREE(obj_index[obj].keys);

		if (obj_index[obj].desc1)
			FREE(obj_index[obj].desc1);

		if (obj_index[obj].desc2)
			FREE(obj_index[obj].desc2);

		if (obj_index[obj].desc3)
			FREE(obj_index[obj].desc3);
	}
	FREE(obj_index);

	free_social_messages();
	free_shops();

	for (int zone = 0; zone <= top_of_zone_table; zone++)
	{
		if (zone_table[zone].name)
			FREE(zone_table[zone].name);

		if (zone_table[zone].filename)
			FREE(zone_table[zone].filename);

		FREE(zone_table[zone].cmd);
	}
	FREE(zone_table);

	FREE(sector_table);

	if (mob_f)
	{
		fclose(mob_f);
		mob_f = NULL;
	}
	if (obj_f)
	{
		fclose(obj_f);
		obj_f = NULL;
	}
}

/* read direction data */
void setup_dir(FILE *fl, int room, int dir)
{
	int state, key, to_room;
	char *general_description, *keyword;

	general_description = fread_string(fl);
	keyword = fread_string(fl);
	if (fscanf(fl, " %d %d %d ", &state, &key, &to_room) != 3 || to_room < 0 || dir < 0 ||
	    dir >= NUM_EXITS)
	{
		if (general_description)
			FREE(general_description);
		if (keyword)
			FREE(keyword);
		return;
	}

	CREATE(world[room].dir_option[dir], room_direction_data, 1, MEM_TAG_DIRDATA);

	world[room].dir_option[dir]->general_description = general_description;
	world[room].dir_option[dir]->keyword = keyword;

	state &=
		3; // only grab first two bits, state gets set by zone reset (closed, blocked, secret)
	if (state)
	{
		world[room].dir_option[dir]->exit_info = EX_ISDOOR;
		if (state == 2)
			world[room].dir_option[dir]->exit_info |= EX_PICKABLE;
		if (state == 3)
			world[room].dir_option[dir]->exit_info |= EX_PICKPROOF;
	}
	else
		world[room].dir_option[dir]->exit_info = 0;

	world[room].dir_option[dir]->key = key;
	world[room].dir_option[dir]->to_room = to_room;

	if (to_room == 0)
		logit(LOG_DEBUG, "Room %d has exit to the void [Room 0].", world[room].number);
}

void renum_world(void)
{
	int room, door, to_room;

	for (room = 0; room <= top_of_world; room++)
		for (door = 0; door <= (NUM_EXITS - 1); door++)
			if (world[room].dir_option[door])
			{
				to_room = real_room0(world[room].dir_option[door]->to_room);
				if (to_room)
					world[room].dir_option[door]->to_room = to_room;
				else
				{
					struct room_direction_data *invalid_exit =
						world[room].dir_option[door];
					if (invalid_exit->general_description)
						FREE(invalid_exit->general_description);
					if (invalid_exit->keyword)
						FREE(invalid_exit->keyword);
					FREE(invalid_exit);
					world[room].dir_option[door] = NULL;
				}
			}
}

void renum_zone_table(void)
{
	int zone, comm;

	for (zone = 0; zone <= top_of_zone_table; zone++)
		for (comm = 0; zone_table[zone].cmd[comm].command != 'S'; comm++)
		{
			switch (zone_table[zone].cmd[comm].command)
			{
			case 'A':
				zone_table[zone].cmd[comm].arg3 =
					real_mobile(zone_table[zone].cmd[comm].arg1);
				break;
			case 'B':
				zone_table[zone].cmd[comm].arg3 =
					real_object(zone_table[zone].cmd[comm].arg3);
				break;
			case 'C':
			case 'Y':
				zone_table[zone].cmd[comm].arg3 =
					real_room(zone_table[zone].cmd[comm].arg3);
				break;
			case 'M':
			case 'R':
			case 'F':
				zone_table[zone].cmd[comm].arg1 =
					real_mobile(zone_table[zone].cmd[comm].arg1);
				zone_table[zone].cmd[comm].arg3 =
					real_room(zone_table[zone].cmd[comm].arg3);
				break;
			case 'O':
				zone_table[zone].cmd[comm].arg1 =
					real_object(zone_table[zone].cmd[comm].arg1);
				if (zone_table[zone].cmd[comm].arg3 != NOWHERE)
					zone_table[zone].cmd[comm].arg3 =
						real_room(zone_table[zone].cmd[comm].arg3);
				break;
			case 'G':
				zone_table[zone].cmd[comm].arg1 =
					real_object(zone_table[zone].cmd[comm].arg1);
				break;
			case 'E':
				zone_table[zone].cmd[comm].arg1 =
					real_object(zone_table[zone].cmd[comm].arg1);
				break;
			case 'P':
				zone_table[zone].cmd[comm].arg1 =
					real_object(zone_table[zone].cmd[comm].arg1);
				zone_table[zone].cmd[comm].arg3 =
					real_object(zone_table[zone].cmd[comm].arg3);
				break;
			case 'D':
				zone_table[zone].cmd[comm].arg1 =
					real_room(zone_table[zone].cmd[comm].arg1);
				break;
			} /* if the real_xxxx() function returned -1, disable this command */
			if (zone_table[zone].cmd[comm].arg1 == -1)
				zone_table[zone].cmd[comm].command = '!';
			/* Also check arg3 for room-based commands */
			if (zone_table[zone].cmd[comm].arg3 == -1)
			{
				switch (zone_table[zone].cmd[comm].command)
				{
				case 'M':
				case 'R':
				case 'F':
				case 'C':
				case 'Y':
				case 'O':
					logit(LOG_DEBUG,
					      "renum_zone: zone %d cmd %d (%c) has invalid room rnum -1",
					      zone, comm, zone_table[zone].cmd[comm].command);
					zone_table[zone].cmd[comm].command = '!';
					break;
				}
			}
		}
}

void recalc_zone_numbers()
{
	fprintf(stderr, "Recalculating zone numbers...\n");
	for (int z = 1; z <= top_of_zone_table; z++)
	{
		int zone_real_bottom = zone_table[z].real_bottom;
		if (zone_real_bottom < 0)
			continue;
		int bottom_vnum = world[zone_real_bottom].number;

		if (zone_table[z].number != (int)(bottom_vnum / 100))
		{
			fprintf(stderr, "  -- %s has invalid number: %d (should be %d)\n",
				strip_ansi(zone_table[z].name).c_str(), zone_table[z].number,
				(int)(bottom_vnum / 100));
			zone_table[z].number = (int)(bottom_vnum / 100);
		}
	}
}

void update_zone_difficulties()
{
	MYSQL_RES *res = db_query("SELECT number, difficulty FROM zones WHERE difficulty <> 0");

	if (!res)
		return;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
	{
		int number = atoi(row[0]);
		int difficulty = atoi(row[1]);

		if (!difficulty)
			continue;

		for (int z = 0; z <= top_of_zone_table; z++)
		{
			if (zone_table[z].number == number)
			{
				zone_table[z].difficulty = difficulty;
				break;
			}
		}
	}

	mysql_free_result(res);
}

#define IS_ZONE_COMMAND(ch)                                                            \
	(ch == 'M' || ch == 'O' || ch == 'E' || ch == 'P' || ch == 'D' || ch == 'G' || \
	 ch == 'R' || ch == 'F' || ch == 'A' || ch == 'B' || ch == 'C' || ch == 'Y' || ch == 'S')

/* load the zone table and command tables */
void boot_zones(int mini_mode)
{
	FILE *fl;
	int num_zones, num_commands, zon = 0, cmd_no = 0, tmp, i, t_idx;
	int tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
	int nu1, nu2; /* not used variables */
	int *command_array;
	char *check, buf[MAX_STRING_LENGTH], tmp_buf[MAX_STRING_LENGTH], c;
	char temp_buf[MAX_STRING_LENGTH];

	if (!mini_mode)
	{
		if (!(fl = fopen(ZONE_FILE, "r")))
		{
			fatal_boot_error("db", "boot_zones: could not open %s: %s", ZONE_FILE,
					 strerror(errno));
		}
	}
	else
	{
		if (!(fl = fopen("areas_mini/mini.zon", "r")))
		{
			fatal_boot_error("db", "boot_zones: could not open areas_mini/mini.zon: %s",
					 strerror(errno));
		}
	}
	logit(LOG_STATUS, "Counting zones...");
	num_zones = 0;
	for (;;)
	{
		if (!fgets(tmp_buf, MAX_STRING_LENGTH, fl))
			break;
		if (tmp_buf[0] == '$')
			break;
		if (tmp_buf[0] == '#')
		{
			num_zones++;
		}
	}
	logit(LOG_STATUS, "\t\t%d zones allocated", num_zones);
	rewind(fl);

	/* Count the number of commands in each zone */
	logit(LOG_STATUS, "Counting commands for each zone...");
	CREATE(command_array, int, (unsigned)num_zones, MEM_TAG_ARRAY);
	//  command_array = (int *) calloc(num_zones, sizeof(int));

	t_idx = 0;
	num_commands = 0;
	for (;;)
	{
		if (!fgets(tmp_buf, MAX_STRING_LENGTH, fl))
			break;
		if (tmp_buf[0] == '$')
			break;
		if (tmp_buf[0] == '#')
		{
			/* skip over zone name */
			if (!fgets(tmp_buf, MAX_STRING_LENGTH, fl))
				break;
		}
		else if (tmp_buf[0] == 'S')
		{
			command_array[t_idx] = num_commands + 1;
			t_idx++;
			num_commands = 0;
		}
		else if (IS_ZONE_COMMAND(tmp_buf[0]))
		{
			/* Commands A B C and Y are new random talbe zone comands */
			num_commands++;
		}
	}
	rewind(fl);

	/* Allocate array of zone structures */

	CREATE(zone_table, zone_data, num_zones, MEM_TAG_ZONEDAT);
	CREATE(sector_table, sector_data, 100, MEM_TAG_SECTDAT);
	//  zone_table =
	//    (struct zone_data *) calloc(num_zones, sizeof(struct zone_data));
	//  sector_table =
	//    (struct sector_data *) calloc(100, sizeof(struct sector_data));

	for (;;)
	{
		if (fscanf(fl, " #%d\n", &tmp) != 1)
			break; /* accept EOF or a legacy $~ terminator */
		check = fread_string(fl); /* zone name as specified by builder */
		if (*check == '$')
			break; /* * end of file */

		zone_table[zon].number = tmp; /* virtual zone number */
		zone_table[zon].name = check;
		zone_table[zon].avg_mob_level = -2;

		zone_table[zon].hometown = 0; /* * default hometown is none */

		check = fread_string(fl); /* zone filename */

		zone_table[zon].filename = check;

		REQUIRED_FSCANF(fl, "%d %d %d %d %d %d\n", &tmp1, &tmp2, &tmp3, &tmp4, &tmp5,
				&tmp6);
		/* * new with variable length lifespan */
		zone_table[zon].top = tmp1;
		zone_table[zon].reset_mode = tmp2;
		zone_table[zon].flags = tmp3;
		zone_table[zon].lifespan_min = tmp4;
		zone_table[zon].lifespan_max = tmp5;
		zone_table[zon].difficulty = tmp6;

		zone_table[zon].fullreset_lifespan_min = 20 * 60;
		zone_table[zon].fullreset_lifespan_max = 28 * 60;

		/* gotta preset this here */

		zone_table[zon].fullreset_lifespan = number(zone_table[zon].fullreset_lifespan_min,
							    zone_table[zon].fullreset_lifespan_max);

		if (zone_table[zon].flags & ZONE_TOWN)
			for (i = 0; town_name_list[i][0] != '\n'; i++)
			{
				stripansi_2(zone_table[zon].name, temp_buf);
				if (isname(town_name_list[i], temp_buf))
				{
					zone_table[zon].hometown = i;
					break;
				}
			}
		/* Set beginning values for zone real range */
		zone_table[zon].real_bottom = zone_table[zon].real_top = -1;

		/* if zone is flagged as map, read map info (x,y size) */
		if (zone_table[zon].flags & ZONE_MAP)
		{
			REQUIRED_FSCANF(fl, "%d %d\n", &tmp1, &tmp2);
			zone_table[zon].mapx = tmp1;
			zone_table[zon].mapy = tmp2;
		}

		/* allocate the command table */
		if (command_array[zon] != 0)
		{
			CREATE(zone_table[zon].cmd, reset_com, (unsigned)command_array[zon],
			       MEM_TAG_RESET);
			/*
			      zone_table[zon].cmd =
			        (struct reset_com *) calloc(command_array[zon],
			                                    sizeof(struct reset_com));
			 */
		}
		else
		{
			fatal_boot_error("db", "boot_zones: zone %d has no commands", zon);
		}

		/* read the command table */
		cmd_no = 0;

		for (;;)
		{
			REQUIRED_FSCANF_NO_FIELDS(fl, " "); /* skip blanks */
			REQUIRED_FSCANF(fl, "%c", &c);

			if (c == '*')
			{
				REQUIRED_FGETS(buf, MAX_STRING_LENGTH, fl); /* skip command */
				/* OLC KLUDGE! */
				if (!strn_cmp(buf, "owner:", 6) && !zone_table[zon].owner)
				{
					char *o, *p;

					o = buf + 6;
					while (*o == ' ') /* skip whitespace  */
						o++;
					p = str_dup(o);
					o = p;
					while (*o)
						if ((*o == ' ') || (*o == '\n'))
							*o = '\0';
						else
							o++;
					zone_table[zon].owner = p;
				}
				continue;
			}

			if (!IS_ZONE_COMMAND(c))
			{
				REQUIRED_FGETS(buf, MAX_STRING_LENGTH, fl); /* skip command */
				continue;
			}

			zone_table[zon].cmd[cmd_no].command = c;

			if (c == 'S')
				break;

			REQUIRED_FSCANF(fl, " %d %d %d %d %d %d %d", &tmp,
					&zone_table[zon].cmd[cmd_no].arg1,
					&zone_table[zon].cmd[cmd_no].arg2,
					&zone_table[zon].cmd[cmd_no].arg3,
					&zone_table[zon].cmd[cmd_no].arg4, &nu1, &nu2);

			zone_table[zon].cmd[cmd_no].if_flag = tmp;

			REQUIRED_FGETS(buf, sizeof(buf) - 1, fl); /* read comment */

			cmd_no++;
			if (mini_mode == 2)
				fprintf(stderr, "cmd_no == %c%d", c, cmd_no);
		}

		zon++;
		if (mini_mode == 2)
			fprintf(stderr, "\r\nzon == %d\r\n", zon);
	}
	top_of_zone_table = --zon;
	//  str_free(check);  // i don't think so..  the last time check is used, it reads a string that is put directly into the zone data
	FREE(command_array);
	fclose(fl);

	update_zone_difficulties();
}

#undef IS_ZONE_COMMAND

/*************************************************************************
 *  procedures for resetting, both play-time and boot-time         *
 *********************************************************************** */

/* get a mobile NUM from random table */
int get_mob_table(int tnum)
{
	int temp;
	int temp2;
	int w, w2;

	for (temp = 0; temp < num_mob_tables; temp++)
	{
		if (mob_tables[temp].virtual_number == tnum)
			break;
	}
	if (mob_tables[temp].virtual_number != tnum)
		return 0; /* no table */
	w = number(0, mob_tables[temp].weight); /* generate # between 0  and total wt */
	if (w < mob_tables[temp].empty_weight)
		return 0; /* empty chance */
	w2 = mob_tables[temp].empty_weight;
	for (temp2 = 0; temp2 < mob_tables[temp].entries; temp2++)
	{
		w2 += mob_tables[temp].table[temp2].weight;
		if (w < w2) /* got it */
			return mob_tables[temp].table[temp2].virtual_number;
	}
	return 0; /* shouldn't get here */
}

/* get a object NUM from random table */
int get_obj_table(int tnum)
{
	int temp, temp2;
	int w, w2;

	for (temp = 0; temp < num_obj_tables; temp++)
	{
		if (obj_tables[temp].virtual_number == tnum)
			break;
	}
	if (obj_tables[temp].virtual_number != tnum)
		return 0; /* no table */
	w = number(0, obj_tables[temp].weight);
	if (w < obj_tables[temp].empty_weight)
		return 0;
	w2 = obj_tables[temp].empty_weight;
	for (temp2 = 0; temp2 < obj_tables[temp].entries; temp2++)
	{
		w2 += obj_tables[temp].table[temp2].weight;
		if (w < w2)
			return obj_tables[temp].table[temp2].virtual_number;
	}
	return 0;
}

// Raw disposal is restricted to the detached loader's empty preparation.
// extract_char/free_char assume world membership and schedule release events.
static bool discard_unpublished_mobile(P_char mob) noexcept
{
	if (!mob || mob->in_room != NOWHERE || mob->next || mob->next_in_room || mob->desc ||
	    mob->carrying || mob->affected || mob->nevents || mob->nevents_tail || mob->followers ||
	    mob->following || mob->group || mob->lobj || mob->linked || mob->linking ||
	    mob->obj_linked || GET_OPPONENT(mob) || mob->character_maintenance_in_world ||
	    mob->player.title)
		return false;
	for (int slot = 0; slot < MAX_WEAR; ++slot)
		if (mob->equipment[slot])
			return false;
	if (mob->only.npc && (mob->only.npc->str_mask || mob->only.npc->memory))
		return false;
	if (find_character_by_runtime_id(mob->runtime_id))
		return false;
	for (P_char live = character_list; live; live = live->next)
		if (live == mob)
			return false;
	if (mob->only.npc)
		FREE(mob->only.npc);
	mm_release(dead_mob_pool, mob);
	return true;
}

struct unpublished_mobile_cleanup
{
	P_char character = nullptr;
	~unpublished_mobile_cleanup()
	{
		if (character && !discard_unpublished_mobile(character))
			panic_corruption("native-mobile",
					 "detached loader escaped empty preparation");
	}
};

// Legacy callers retain their original hook-before-conversion ordering.
// Detached preparation defers this entire callback/event boundary to publication.
static bool schedule_mobile_periodic(P_char mob, bool guard_runtime)
{
	const uint64_t runtime_id = mob->runtime_id;
	world_activity_schedule_mundane(mob, false, true);
	if (IS_SET(mob->specials.act, ACT_SPEC))
	{
		const bool wants_periodic = (mob_index[mob->only.npc->R_num].func.mob)(
			mob, NULL, CMD_SET_PERIODIC, NULL);
		if (guard_runtime && find_character_by_runtime_id(runtime_id) != mob)
			return false;
		if (wants_periodic)
			add_event(event_mob_proc, PULSE_MOBILE + number(-4, 4), mob, 0, 0, 0, 0, 0);
	}
	if (IS_ACT(mob, ACT_PATROL))
		add_event(event_patrol_move, WAIT_SEC, mob, 0, 0, 0, 0, 0);
	return true;
}

// One real loader body preserves file parsing, stats, randomization and conversion.
namespace
{
using constructor_digest = quest_mobile_native_constructor_digest;
using constructor_binding = quest_mobile_native_constructor_binding;

class constructor_hash
{
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context_{ EVP_MD_CTX_new(),
									  EVP_MD_CTX_free };
	bool valid_ = context_ && EVP_DigestInit_ex(context_.get(), EVP_sha256(), nullptr) == 1;

    public:
	bool bytes(const void *data, size_t count) noexcept
	{
		valid_ = valid_ && (!count || EVP_DigestUpdate(context_.get(), data, count) == 1);
		return valid_;
	}
	void integer(uint64_t value) noexcept
	{
		std::array<uint8_t, 8> encoded{};
		for (size_t i = 0; i < encoded.size(); ++i)
			encoded[i] = static_cast<uint8_t>(value >> (8 * i));
		bytes(encoded.data(), encoded.size());
	}
	void real(float value) noexcept
	{
		valid_ = valid_ && std::isfinite(value);
		integer(std::bit_cast<uint32_t>(value));
	}
	void real(double value) noexcept
	{
		valid_ = valid_ && std::isfinite(value);
		integer(std::bit_cast<uint64_t>(value));
	}
	void text(const char *value) noexcept
	{
		integer(value != nullptr);
		if (!value)
			return;
		const size_t length = strnlen(value, MAX_STRING_LENGTH);
		if (length == MAX_STRING_LENGTH)
		{
			valid_ = false;
			return;
		}
		integer(length);
		bytes(value, length);
	}
	bool finish(constructor_digest *output) noexcept
	{
		constructor_digest value{};
		unsigned int length = 0;
		if (!output || !valid_ ||
		    EVP_DigestFinal_ex(context_.get(), value.data(), &length) != 1 ||
		    length != value.size())
			return false;
		*output = value;
		return true;
	}
};

bool constructor_nonzero(const constructor_digest &value) noexcept
{
	return std::any_of(value.begin(), value.end(), [](uint8_t byte) { return byte != 0; });
}

bool constructor_binding_tag(qst_func_type function, constructor_binding *output) noexcept
{
	if (!output)
		return false;
	if (!function)
		*output = constructor_binding::none;
	else if (function == thief)
		*output = constructor_binding::thief;
	else if (function == teacher)
		*output = constructor_binding::teacher;
	else if (function == shop_keeper)
		*output = constructor_binding::shop_keeper;
	else if (function == quester)
		*output = constructor_binding::quester;
	else
		return false; // An arbitrary sealed NPC binding identity is still required.
	return true;
}

bool constructor_binding_transition(constructor_binding before, constructor_binding after) noexcept
{
	return before == after ||
	       (before == constructor_binding::none &&
		(after == constructor_binding::thief || after == constructor_binding::teacher));
}

// NBC2 names are classifications only; actual incumbent identity is always
// established by the main-ELF/current-dispatcher witness before using this tag.
bool constructor_binding_tag_v2(qst_func_type function, constructor_binding *output) noexcept
{
	if (!output)
		return false;
	if (!constructor_binding_tag(function, output))
		*output = constructor_binding::arbitrary;
	return true;
}

bool constructor_string_digest(const char *value, constructor_digest *output) noexcept
{
	constructor_hash hash;
	hash.text(value);
	return hash.finish(output);
}

// Hash the actual descriptor used by the parser, never a later path reopen.
// All file movement is serialized on the game thread and restores its cursor.
bool constructor_template_digest(long position, uint64_t length,
				 constructor_digest *output) noexcept
{
	if (!mob_f || !output || position < 0 || !length || ferror(mob_f) ||
	    length > static_cast<uint64_t>(LONG_MAX - position))
		return false;
	fpos_t saved;
	if (fgetpos(mob_f, &saved))
		return false;
	bool valid = fseek(mob_f, 0, SEEK_END) == 0;
	const long end = valid ? ftell(mob_f) : -1;
	valid = valid && end >= position && length <= static_cast<uint64_t>(end - position) &&
		fseek(mob_f, position, SEEK_SET) == 0;
	constructor_hash hash;
	hash.integer(length);
	std::array<uint8_t, 4096> buffer{};
	uint64_t remaining = length;
	while (valid && remaining)
	{
		const size_t count =
			static_cast<size_t>(std::min<uint64_t>(remaining, buffer.size()));
		valid = fread(buffer.data(), 1, count, mob_f) == count &&
			hash.bytes(buffer.data(), count);
		remaining -= count;
	}
	constructor_digest digest{};
	valid = valid && hash.finish(&digest);
	const bool restored = fsetpos(mob_f, &saved) == 0;
	clearerr(mob_f);
	if (!valid || !restored)
		return false;
	*output = digest;
	return true;
}

struct native_mobile_constructor_session
{
	int rnum = -1;
	bool apply_gold = false, replay = false;
	quest_mobile_native_constructor_recipe *recipe = nullptr;
	P_char prepared = nullptr;
	unpublished_mobile_cleanup cleanup;
	size_t clocks = 0;
	constructor_hash inputs;
	constructor_digest effective{};
	bool inputs_complete = false;
	qst_func_type original_binding = nullptr, installed_fallback = nullptr;
	bool keep_binding = false;

	void note_fallback(qst_func_type function) noexcept
	{
		// Only original synchronous detached none->thief/teacher writes.
		// No procedure callback/event runs in this constructor capsule.
		if (!original_binding && rnum >= 0 && !mob_index[rnum].func.mob &&
		    (function == thief || function == teacher))
			installed_fallback = function;
	}
	~native_mobile_constructor_session()
	{
		// Prove/dispose the exact empty preparation before any failure rollback.
		// A native escape is corruption, never permission to clear a binding.
		if (cleanup.character)
		{
			if (!discard_unpublished_mobile(cleanup.character))
				panic_corruption("native-mobile",
						 "constructor capsule escaped preparation");
			cleanup.character = nullptr;
		}
		if (!keep_binding && installed_fallback && !original_binding &&
		    mob_index[rnum].func.mob == installed_fallback)
			mob_index[rnum].func.mob = original_binding;
	}
	bool conversion_ignored = false;
	unsigned int conversion_class = 0;

	int integer_input(const char *tag, int row, int column, int value) noexcept
	{
		inputs.text(tag);
		inputs.integer(row);
		inputs.integer(column);
		inputs.integer(value);
		return value;
	}
	float real_input(const char *tag, float value) noexcept
	{
		inputs.text(tag);
		inputs.real(value);
		return value;
	}
	const char *race_code(int row, const char *value) noexcept
	{
		inputs.text("race-code");
		inputs.integer(row);
		inputs.text(value);
		return value;
	}
	bool before_conversion(P_char mobile) noexcept;
	bool after_conversion(P_char mobile) noexcept;

	time_t clock() noexcept
	{
		if (clocks >= 2 || !recipe)
			return static_cast<time_t>(-1);
		const size_t index = clocks++;
		if (replay)
			return static_cast<time_t>(recipe->clock_values[index]);
		const time_t value = time(nullptr);
		recipe->clock_values[index] = static_cast<int64_t>(value);
		return value;
	}
};

bool native_mobile_constructor_session::before_conversion(P_char mobile) noexcept
{
	if (!mobile || !recipe || mobile->player.spec > MAX_SPEC)
		return false;
	inputs.text("selected-conversion-inputs");
	conversion_ignored = IS_SET(mobile->specials.act, ACT_IGNORE) ||
			     strstr(mobile->player.name, "_ignore_");
	conversion_class = mobile->player.m_class;
	// convertMob selects these names before defaulting a class or converting level.
	const bool multiple = conversion_class && (conversion_class & (conversion_class - 1));
	if (!multiple && mobile->player.spec == 0)
	{
		const int cls = flag2idx(conversion_class);
		if (cls < 0 || cls > CLASS_COUNT)
			return false;
		for (int spec = 0; spec < MAX_SPEC; ++spec)
		{
			const std::array<const char *, 4> names{ "_spec1_", "_spec2_", "_spec3_",
								 "_spec4_" };
			if (!isname(names[spec], GET_NAME(mobile)))
				continue;
			const bool available = specdata[cls][spec] && *specdata[cls][spec];
			integer_input("specialization-available", cls, spec, available);
			if (available)
				break;
		}
	}
	inputs.integer(conversion_ignored);
	if (!conversion_ignored)
	{
		if (!conversion_class && GET_LEVEL(mobile) >= 15)
			conversion_class = CLASS_WARRIOR;
		const int cls = flag2idx(conversion_class);
		if (cls < 0 || cls > CLASS_COUNT)
			return false;
		inputs.integer(cls);
		inputs.real(class_hitpoints[cls]);
		inputs.real(hp_mob_con_factor);
		inputs.real(hp_mob_npc_pc_ratio);
		inputs.integer(chaos_mud_enabled());
		if (IS_ELITE(mobile))
		{
			inputs.real(get_property("hitpoints.mob.eliteBonus", 2.5));
			inputs.real(get_property("damage.eliteBonus", 1.2));
		}
	}
	return true;
}

bool native_mobile_constructor_session::after_conversion(P_char mobile) noexcept
{
	if (!mobile || !recipe || !world || !zone_table || mobile->player.spec > MAX_SPEC)
		return false;
	const int race = BOUNDED(0, (int)GET_RACE(mobile), LAST_RACE);
	const int spec = mobile->player.spec;
	const int room = real_room0(GET_BIRTHPLACE(mobile));
	if (room < 0 || room > top_of_world || world[room].zone > top_of_zone_table)
		return false;
	inputs.text("selected-affect-inputs");
	inputs.integer(race);
	// Fresh detached construction has no equipment, dynamic affects or group.
	// The actual converted race supplies all ten racial stat factors.
	for (const auto value :
	     { stat_factor[race].Str, stat_factor[race].Dex, stat_factor[race].Agi,
	       stat_factor[race].Con, stat_factor[race].Pow, stat_factor[race].Int,
	       stat_factor[race].Wis, stat_factor[race].Cha, stat_factor[race].Kar,
	       stat_factor[race].Luk })
		inputs.integer(value);
	inputs.real(combat_by_race[race][0]);
	inputs.real(combat_by_race[race][1]);
	inputs.real(pulse_all);
	inputs.integer(GET_BIRTHPLACE(mobile));
	inputs.integer(world[room].number);
	inputs.integer(zone_table[world[room].zone].number);
	const int difficulty = BOUNDED(1, zone_table[world[room].zone].difficulty, 10);
	inputs.integer(difficulty);
	if (difficulty > 1)
		inputs.real(get_property("damage.zoneDifficulty.mod.factor", .200));
	if (IS_AFFECTED2(mobile, AFF2_FLURRY))
		inputs.real(get_property("innate.flurry.pulse", .70));
	if (!mobile->points.base_vitality)
		inputs.integer(racial_data[GET_RACE(mobile)].base_vitality);
	if (!conversion_ignored)
	{
		const int level = BOUNDED(0, (int)GET_LEVEL(mobile), TOTALLVLS - 1);
		for (int circle = 0; circle < MAX_CIRCLE; ++circle)
			integer_input("converted-spell-slots", level, circle,
				      spl_table[level][circle]);
		const int quest = find_quester_id(rnum);
		if (quest >= number_of_quests)
			return false;
		inputs.integer(quest >= 0 && (quest_index[quest].quest_complete ||
					      quest_index[quest].quest_message));
		// Only the original reached scaling decision reads the live gold dial.
		if (apply_gold && difficulty_world_npc(mobile) && GET_MONEY(mobile))
			inputs.real(difficulty_multiplier(DIFFICULTY_MOB_GOLD));
	}
	// These are exactly the innate lookups reached by apply_affs and the
	// equipment-free innate_two_daggers check, not every innate/race/class row.
	for (const int innate : { INNATE_INFERNAL_FURY,
				  INNATE_SNEAK,
				  INNATE_FARSEE,
				  INNATE_PROT_LIGHTNING,
				  INNATE_PROT_FIRE,
				  INNATE_WATERBREATH,
				  INNATE_INFRAVISION,
				  INNATE_FLY,
				  INNATE_NATURAL_MOVEMENT,
				  INNATE_HASTE,
				  INNATE_REGENERATION,
				  INNATE_BLOOD_SCENT,
				  INNATE_ULTRAVISION,
				  INNATE_ANTI_GOOD,
				  INNATE_PROT_ACID,
				  INNATE_PROT_COLD,
				  INNATE_FIRE_AURA,
				  INNATE_ICE_AURA,
				  INNATE_ANTI_EVIL,
				  INNATE_VAMPIRIC_TOUCH,
				  INNATE_HOLY_LIGHT,
				  INNATE_DAUNTLESS,
				  INNATE_EYELESS,
				  INNATE_INVISIBILITY,
				  INNATE_DUAL_WIELDING_MASTER,
				  INNATE_HAMMER_MASTER,
				  INNATE_AXE_MASTER,
				  INNATE_LONGSWORD_MASTER,
				  INNATE_GAMBLERS_LUCK,
				  INNATE_WARDING_FAITH,
				  INNATE_TWO_DAGGERS })
	{
		inputs.integer(innate);
		const unsigned int classes = class_innates_at_all[innate] & mobile->player.m_class;
		inputs.integer(classes);
		for (int cls = 0; cls < CLASS_COUNT; ++cls)
			if (classes & (1U << cls))
			{
				integer_input("class-innate", cls, 0,
					      class_innates[innate][cls][0]);
				if (spec)
					integer_input("class-innate", cls, spec,
						      class_innates[innate][cls][spec]);
			}
		integer_input("racial-innate", innate, race, racial_innates[innate][race]);
	}
	const bool ward = has_innate(mobile, INNATE_WARDING_FAITH);
	if (ward)
	{
		inputs.real(get_property("innate.wardingFaith.multiplier", 3.0));
		inputs.real(get_property("innate.wardingFaith.rechargeTime", 120.0));
	}
	for (const int skill : { SKILL_EPIC_STRENGTH, SKILL_EPIC_POWER, SKILL_EPIC_AGILITY,
				 SKILL_EPIC_INTELLIGENCE, SKILL_EPIC_DEXTERITY, SKILL_EPIC_WISDOM,
				 SKILL_EPIC_CONSTITUTION, SKILL_EPIC_CHARISMA, SKILL_EPIC_LUCK,
				 SKILL_LISTEN, SKILL_EPIC_WARDING_FAITH, SKILL_IMPROVED_ENDURANCE })
	{
		if (skill == SKILL_EPIC_WARDING_FAITH && !ward)
			continue;
		inputs.integer(skill);
		// GET_CHAR_SKILL_P returns zero without accessing a class row
		// when this original low-level/ignored NPC has no class.
		if (!mobile->player.m_class)
			continue;
		// GET_LVL_FOR_SKILL's selected NPC class row plus actual cap rows.
		int selected = flag2idx(mobile->player.m_class) - 1;
		if (selected < 0 || selected >= CLASS_COUNT)
			return false;
		if (IS_MULTICLASS_NPC(mobile))
		{
			selected = 0;
			int highest = 0;
			for (int cls = 0; cls < CLASS_COUNT; ++cls)
				if (GET_CLASS(mobile, 1U << cls))
				{
					const int cap = skills[skill].m_class[cls].maxlearn[0];
					integer_input("skill-class-selection", cls, 0, cap);
					if (cap > highest)
					{
						highest = cap;
						selected = cls;
					}
				}
		}
		const int required = skills[skill].m_class[selected].rlevel[spec];
		integer_input("skill-required-level", selected, spec, required);
		if (required && GET_LEVEL(mobile) >= required)
			for (int cls = 0; cls < CLASS_COUNT; ++cls)
				if (GET_CLASS(mobile, 1U << cls))
				{
					integer_input("skill-cap", cls, 0,
						      skills[skill].m_class[cls].maxlearn[0]);
					integer_input("skill-cap", cls, spec,
						      skills[skill].m_class[cls].maxlearn[spec]);
					if (!IS_MULTICLASS_NPC(mobile))
						break;
				}
#ifdef STANCES_ALLOWED
		inputs.integer(skills[skill].category);
#endif
	}
	inputs_complete = inputs.finish(&effective);
	return inputs_complete && (!replay || effective == recipe->effective_inputs_digest);
}

}

static P_char read_mobile_body(int nr, int type, bool apply_mob_gold, bool detached,
			       native_mobile_constructor_session *capsule = nullptr)
{
	P_char mob = NULL;
	unpublished_mobile_cleanup cleanup;
	char Gbuf1[MAX_STRING_LENGTH], buf[MAX_INPUT_LENGTH], letter = 0;
	int foo, bar, i, j;
	long tmp, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8;
	unsigned utmp1, utmp2, utmp3, utmp4, utmp5, utmp6, utmp7, utmp8, utmp9;
	int stmp, stmp3, stmp4, level;
	static int idnum = 0;

	i = nr;
	if (type == VIRTUAL)
		if ((nr = real_mobile(nr)) < 0)
		{
#if defined(DB_NOTIFY) && DB_NOTIFY
			logit(LOG_DEBUG, "read_mobile: Mob %d not in database", i);
#endif
			return 0;
		}
	if (nr < 0)
	{
		logit(LOG_DEBUG, "read_mobile: negative rnum (%d). args %d, %s", nr, i,
		      type ? "VIRTUAL" : "REAL");
		return 0;
	}
	fseek(mob_f, mob_index[nr].pos, 0);
	if (capsule && (ferror(mob_f) || ftell(mob_f) != mob_index[nr].pos))
		return nullptr;

	mob = (P_char)mm_get(dead_mob_pool);

	clear_char(mob);
	if (detached)
		cleanup.character = mob;
	CREATE(mob->only.npc, npc_only_data, 1, MEM_TAG_NPCONLY);

	if (!mob->only.npc)
	{
		wizlog(56, "mob has no only.npc struct!");
		logit(LOG_DEBUG, "mob %s has no only.npc struct!", GET_NAME(mob));
		if (!detached)
			mm_release(dead_mob_pool, mob);
		return NULL;
	}

	bzero(mob->only.npc, sizeof(npc_only_data));
	mob->only.npc->shopkeeper_shop_id = -1;

	if (!detached)
	{
		/* insert in list */
		mob->next = character_list;
		character_list = mob;
	}
	mob->only.npc->R_num = nr;
	mob->desc = NULL;
	if (!detached)
		mob_index[nr].number++;
	idnum++;
	mob->only.npc->idnum = idnum;
	mob->only.npc->default_pos = POS_STANDING + STAT_NORMAL;

	for (int value_index = 0; value_index < NUMB_CHAR_VALS; value_index++)
	{
		mob->only.npc->value[value_index] = 0;
	}

	/***** String data *** */

	/*
	 * added pointers to the index struct, so that all mobs of the same
	 * type will now share all text.  This should save us a huge amount of
	 * RAM. -JAB
	 */

	if (!mob_index[nr].keys)
	{
		mob->player.name = fread_string(mob_f);
		if (!mob->player.name)
		{
			wizlog(56, "Error with mob:  No name");
			static char partial_mobile_name[] = "partial_mobile";
			mob->player.name = partial_mobile_name;
			SET_BIT(mob->specials.act, ACT_ISNPC);
			if (!detached)
				extract_char(mob);
			return NULL;
		}
		for (j = 0; *(mob->player.name + j); j++) /* make sure all keywords
		                                             are lowercased  */
			*(mob->player.name + j) = LOWER(*(mob->player.name + j));
		mob_index[nr].keys = mob->player.name;
	}
	else
	{
		skip_fread(mob_f);
		mob->player.name = mob_index[nr].keys;
	}

	if (!mob_index[nr].desc2)
	{
		mob->player.short_descr = fread_string(mob_f);
		mob_index[nr].desc2 = mob->player.short_descr;
	}
	else
	{
		skip_fread(mob_f);
		mob->player.short_descr = mob_index[nr].desc2;
	}

	if (!mob_index[nr].desc1)
	{
		mob->player.long_descr = fread_string(mob_f);
		mob_index[nr].desc1 = mob->player.long_descr;
	}
	else
	{
		skip_fread(mob_f);
		mob->player.long_descr = mob_index[nr].desc1;
	}

	if (!mob_index[nr].desc3)
	{
		mob->player.description = fread_string(mob_f);
		mob_index[nr].desc3 = mob->player.description;
	}
	else
	{
		skip_fread(mob_f);
		mob->player.description = mob_index[nr].desc3;
	}

	/**** Numeric data ****/

	/*
	 * get next line of info.  It can be one of three formats: %d%d%d%dS,
	 * %d%d%dS, or %d%d%d - DCL
	 */

	REQUIRED_FGETS(buf, sizeof(buf) - 1, mob_f);
	if (sscanf(buf, " %u %u %u %u %u %u %u %u %u %c \n", &utmp1, &utmp7, &utmp8, &utmp9, &utmp2,
		   &utmp3, &utmp4, &utmp5, &utmp6, &letter) == 10)
	{
		mob->specials.act = utmp1;
		mob->specials.affected_by = utmp2;
		mob->specials.affected_by2 = utmp3;
		mob->specials.affected_by3 = utmp4;
		mob->specials.affected_by4 = utmp5;
		mob->specials.affected_by5 = 0;
		mob->specials.alignment = utmp6;
		mob->only.npc->aggro_flags = utmp7;
		mob->only.npc->aggro2_flags = utmp8;
		mob->only.npc->aggro3_flags = utmp9;
	}
	else if (sscanf(buf, " %ld %ld %ld %ld %ld %ld %ld %ld %c \n", &tmp1, &tmp7, &tmp8, &tmp2,
			&tmp3, &tmp4, &tmp5, &tmp6, &letter) == 9)
	{
		mob->specials.act = tmp1;
		mob->only.npc->aggro_flags = tmp7;
		mob->only.npc->aggro2_flags = tmp8;
		mob->only.npc->aggro3_flags = 0;
		mob->specials.affected_by = tmp2;
		mob->specials.affected_by2 = tmp3;
		mob->specials.affected_by3 = tmp4;
		mob->specials.affected_by4 = tmp5;
		mob->specials.affected_by5 = 0;
		mob->specials.alignment = tmp6;
	}
	else if (sscanf(buf, " %ld %ld %ld %ld %c \n", &tmp1, &tmp2, &tmp3, &tmp4, &letter) == 5)
	{
		mob->specials.act = tmp1;
		mob->specials.affected_by = tmp2;
		mob->specials.affected_by2 = tmp3;
		mob->specials.alignment = tmp4;
	}
	else
	{
		if (sscanf(buf, " %ld %ld %ld %c \n", &tmp1, &tmp2, &tmp3, &letter) < 3)
		{
			logit(LOG_DEBUG, "Mob %d has messed up format.",
			      mob_index[nr].virtual_number);
			SET_BIT(mob->specials.act, ACT_ISNPC);
			if (!detached)
				extract_char(mob);
			return NULL;
		}
		mob->specials.act = tmp1;
		mob->specials.affected_by = tmp2;
		mob->specials.affected_by2 = 0;
		mob->specials.alignment = tmp3;
	}

	/* hack hack  */
	if (IS_SET(mob->specials.act, ACT_CANFLY))
		SET_BIT(mob->specials.affected_by, AFF_FLY);
	if (IS_AFFECTED2(mob, AFF2_CASTING))
		REMOVE_BIT(mob->specials.affected_by2, AFF2_CASTING);
	if (IS_AFFECTED(mob, AFF_FEAR))
		REMOVE_BIT(mob->specials.affected_by, AFF_FEAR);
	if (IS_AFFECTED(mob, AFF_CAMPING))
		REMOVE_BIT(mob->specials.affected_by, AFF_CAMPING);
	if (IS_AFFECTED2(mob, AFF2_MAJOR_PARALYSIS))
		REMOVE_BIT(mob->specials.affected_by2, AFF2_MAJOR_PARALYSIS);
	if (IS_AFFECTED2(mob, AFF2_SCRIBING))
		REMOVE_BIT(mob->specials.affected_by2, AFF2_SCRIBING);

	SET_BIT(mob->specials.act, ACT_ISNPC);

	// This should always be true as of 10/22/2015.
	if (letter == 'S')
	{
		REQUIRED_FGETS(buf, sizeof(buf) - 1, mob_f);
		if (sscanf(buf, " %s %i %u %i %i \n", Gbuf1, &stmp, &utmp2, &stmp3, &stmp4) == 5)
		{
			mob->player.race = RACE_NONE;

			// Start with the first race and end with the last.
			for (i = 1; i <= LAST_RACE; i++)
			{
				if (!str_cmp((capsule ? capsule->race_code(
								i, race_names_table[i].code) :
							race_names_table[i].code),
					     Gbuf1))
				{
					mob->player.race = i;
					break;
				}
			}
			GET_HOME(mob) = stmp;
			mob->player.m_class = utmp2;
			mob->player.spec = stmp3;
			mob->player.size = stmp4;
		}
		else
		{
			sscanf(buf, " %s %i %u %i \n", Gbuf1, &stmp, &utmp2, &stmp3);

			mob->player.race = RACE_NONE;

			for (i = 1; i <= LAST_RACE; i++)
			{
				if (!str_cmp((capsule ? capsule->race_code(
								i, race_names_table[i].code) :
							race_names_table[i].code),
					     Gbuf1))
				{
					mob->player.race = i;
					break;
				}
			}

			GET_HOME(mob) = stmp;
			mob->player.m_class = utmp2;
			mob->player.size = stmp3;
		}

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		if (tmp > MAXLVL || tmp < 1)
		{
			logit(LOG_DEBUG, "Bad level %ld for mob '%s' %d.", tmp, J_NAME(mob),
			      GET_VNUM(mob));
			debug("Bad level %ld for mob '%s' %d.", tmp, J_NAME(mob), GET_VNUM(mob));
			mob->player.level = level = (tmp > MAXLVL) ? MAXLVL : 1;
		}
		else
		{
			mob->player.level = level = tmp;
		}
#if defined(CTF_MUD) && (CTF_MUD == 1)
		if (!IS_SET(mob->specials.act, ACT_ELITE))
			mob->player.level = MAX(1, (int)(mob->player.level / 2));

		if (IS_SET(mob->specials.act, ACT_ELITE))
			mob->player.level -= number(10, 20);

		if (IS_SET(mob->specials.act, ACT_TEACHER) ||
		    IS_SET(mob->specials.act, ACT_SPEC_TEACHER) && mob->player.level < 56)
			mob->player.level = 56;

		level = mob->player.level;
#endif

		/*
		 * The following initialises the # of spells useable for NPCs in a given
		 * spell circle based on the spl_table[level][spell_circle] in memorize.c.
		 * Element 0 of this tracking array serves as an accumulator used in
		 * replenishing used slots. - SKB 31 Mar 1995
		 */
		mob->specials.undead_spell_slots[0] = 0;
		for (j = 1; j <= MAX_CIRCLE; j++)
		{
			mob->specials.undead_spell_slots[j] =
				(capsule ? capsule->integer_input("parsed-spell-slots", level,
								  j - 1, spl_table[level][j - 1]) :
					   spl_table[level][j - 1]);
		}

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		/* was warping things.  Tempy fix til everything changes.  JAB */
		if (IS_WARRIOR(mob) || IS_GREATER_RACE(mob) || IS_ELITE(mob) || IS_GIANT(mob))
		{
			mob->points.base_hitroll = BOUNDED(2, (level >> 1), 25);
		}
		else
		{
			mob->points.base_hitroll = BOUNDED(0, (level / 3), 25);
		}

		mob->points.hitroll = mob->points.base_hitroll;

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		mob->points.base_armor = BOUNDED(-250, tmp, 250);

		tmp = 0;
		tmp2 = 0;
		tmp3 = 0;
		REQUIRED_FSCANF(mob_f, " %ldd%ld+%ld ", &tmp, &tmp2, &tmp3);

		// Added some extra hps for mobs for the Sept 12th 2014 wipe.
		//  lvl 1:0, lvl 2:2, lvl 3:4, lvl4:8 .. lvl 50:1250, lvl 62:1922.
		if (tmp2 <= 0 || tmp <= 0)
		{
			// mob->points.base_hit = tmp3;
			mob->points.base_hit = tmp3 + level * level / 2;
		}
		else
		{
			// mob->points.base_hit = dice(tmp, tmp2) + tmp3;
			mob->points.base_hit = dice(tmp, tmp2) + tmp3 + level * level / 2;
		}
		mob->points.hit = mob->points.max_hit = mob->points.base_hit;
		if (mob->points.hit <= 0)
			logit(LOG_MOB, "Warning: MOB #%d has negative (%d) hp.\n",
			      mob_index[nr].virtual_number, mob->points.hit);

		REQUIRED_FSCANF(mob_f, " %ldd%ld+%ld \n", &tmp, &tmp2, &tmp3);
		mob->points.base_damroll = mob->points.damroll = tmp3 + level;
		mob->points.damnodice = tmp;
		mob->points.damsizedice = tmp2;

		REQUIRED_FGETS(buf, sizeof(buf) - 1, mob_f);
		if (sscanf(buf, " %ld.%ld.%ld.%ld %ld", &tmp1, &tmp2, &tmp3, &tmp4, &tmp) == 5)
		{
			// The legacy 20-platinum bonus is decided on the file's value; the final
			// converted wallet is scaled once in convertMob().
			const bool platinum_bonus = tmp4 > 20;
			GET_PLATINUM(mob) = tmp4; /* * (number(50, 200) / 100); */
			GET_GOLD(mob) = tmp3; /* * (number(50, 200) / 100); */
			GET_SILVER(mob) = tmp2; /* * (number(50, 200) / 100); */
			GET_COPPER(mob) = tmp1; /* * (number(50, 200) / 100); */
			if (tmp > 10000000)
			{
				logit(LOG_MOB, "Mob '%s' %d has extreme exp %s.", mob->player.name,
				      mob_index[nr].virtual_number, comma_string(tmp));
			}
			GET_EXP(mob) = tmp * (capsule ?
						      capsule->real_input("parsed-exp",
									  exp_mods[EXPMOD_GLOBAL]) :
						      exp_mods[EXPMOD_GLOBAL]);
			if (platinum_bonus)
			{
				tmp = ((GET_PLATINUM(mob) * 1000) + (GET_GOLD(mob) * 100) +
				       (GET_SILVER(mob) * 10) + GET_COPPER(mob));
				ADD_MONEY(mob, tmp);
			}
		}
		else
		{
			tmp1 = 0;
			tmp = 0;
			if (sscanf(buf, " %ld %ld", &tmp1, &tmp) == 2)
			{
				ADD_MONEY(mob, tmp1);
				GET_EXP(mob) =
					tmp *
					(capsule ? capsule->real_input("parsed-exp",
								       exp_mods[EXPMOD_GLOBAL]) :
						   exp_mods[EXPMOD_GLOBAL]);
			}
			else
			{
				fatal_boot_error("db",
						 "boot_mobiles: bogus cash and/or exp for mob %d",
						 mob_index[nr].virtual_number);
			}
		}
	}
	else
	{
		mob->player.level = level = 1;
		mob->player.race = RACE_NONE;
		mob->player.m_class = CLASS_NONE;
		mob->player.spec = SPEC_NONE;
		mob->player.size = SIZE_NONE;

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		mob->base_stats.Str = (sh_int)(tmp * 4.5);

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		mob->base_stats.Int = (sh_int)(tmp * 4.5);

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		mob->base_stats.Wis = (sh_int)(tmp * 4.5);

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		mob->base_stats.Dex = (sh_int)(tmp * 4.5);

		REQUIRED_FSCANF(mob_f, " %ld \n", &tmp);
		mob->base_stats.Con = (sh_int)(tmp * 4.5);

		mob->base_stats.Pow = mob->base_stats.Int;
		mob->base_stats.Agi = mob->base_stats.Dex;
		mob->base_stats.Cha = dice(3, 20) + 40;
		mob->base_stats.Kar = dice(3, 20) + 40;
		mob->base_stats.Luk = dice(3, 20) + 40;

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		REQUIRED_FSCANF(mob_f, " %ld ", &tmp2);

		mob->points.base_hit = number(tmp, tmp2);
		mob->points.hit = mob->points.max_hit = mob->points.base_hit;
		if (mob->points.hit < 0)
		{
			logit(LOG_DEBUG, "MOB #%d has negative (%d) hp.",
			      mob_index[nr].virtual_number, mob->points.hit);
		}

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);

		mob->points.base_armor = 250;

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		mob->points.mana = mob->points.base_mana = mob->points.max_mana = tmp;

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		mob->points.vitality = mob->points.base_vitality = mob->points.max_vitality = tmp;

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		GET_EXP(mob) = tmp * (capsule ? capsule->real_input("parsed-exp",
								    exp_mods[EXPMOD_GLOBAL]) :
						exp_mods[EXPMOD_GLOBAL]);

		/* Get hometown */
		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		GET_HOME(mob) = tmp;
		GET_BIRTHPLACE(mob) = tmp;

		/* Get alignment */
		REQUIRED_FSCANF(mob_f, " %ld \n", &tmp);
		GET_ALIGNMENT(mob) = tmp;
	}
	mob->points.base_ward = 0;
	mob->points.ward_reg = 0;

	REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
	/*
	 * ok, this has to be changed, until all mob files are changed.  We
	 * have to interpret old 'position' number into new one. JAB
	 */
	switch (tmp)
	{
	case 0: /* * was POSITION_DEAD */
		logit(LOG_DEBUG, "Mob %d tried to load dead", mob_index[nr].virtual_number);
		SET_POS(mob, POS_PRONE + STAT_DYING);
		break;
	case 1: /* * was POSITION_MORTALLYW */
		SET_POS(mob, POS_PRONE + STAT_DYING);
		break;
	case 2: /* * was POSITION_INCAP */
		SET_POS(mob, POS_PRONE + STAT_INCAP);
		break;
	case 3: /* * was POSITION_STUNNED */
		SET_POS(mob, POS_SITTING + STAT_RESTING);
		break;
	case 4: /* * was POSITION_SLEEPING */
		SET_POS(mob, POS_PRONE + STAT_SLEEPING);
		break;
	case 5: /* * was POSITION_RESTING */
		SET_POS(mob, POS_SITTING + STAT_RESTING);
		break;
	case 6: /* * was POSITION_SITTING */
		SET_POS(mob, POS_SITTING + STAT_NORMAL);
		break;
	case 7: /* * was POSITION_FIGHTING */
		logit(LOG_DEBUG, "Mob %d loaded fighting.", mob_index[nr].virtual_number);
		[[fallthrough]];
	case 8: /* * was POSITION_STANDING */
		SET_POS(mob, POS_STANDING + STAT_NORMAL);
		break;
	case 9: /* * was POSITION_SWIMMING? */
		SET_POS(mob, POS_PRONE + STAT_NORMAL);
		break;
	case 10: /* * was POSITION_FLYING */
		SET_POS(mob, POS_STANDING + STAT_NORMAL);
		SET_BIT(mob->specials.affected_by, AFF_FLY);
		break;
	case 11: /* * was POSITION_LEVITATING */
		SET_POS(mob, POS_STANDING + STAT_NORMAL);
		SET_BIT(mob->specials.affected_by, AFF_LEVITATE);
		break;
	}

	mob->only.npc->default_pos = mob->specials.position;

	REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
	/*
	 * ok, this has to be changed, until all mob files are changed.  We
	 * have to interpret old 'position' number into new one. JAB
	 */
	switch (tmp)
	{
	case 0: /* * was POSITION_DEAD */
		logit(LOG_DEBUG, "Mob %d tried to load dead", mob_index[nr].virtual_number);
		SET_POS(mob, POS_PRONE + STAT_DYING);
		break;
	case 1: /* * was POSITION_MORTALLYW */
		SET_POS(mob, POS_PRONE + STAT_DYING);
		break;
	case 2: /* * was POSITION_INCAP */
		SET_POS(mob, POS_PRONE + STAT_INCAP);
		break;
	case 3: /* * was POSITION_STUNNED */
		SET_POS(mob, POS_SITTING + STAT_RESTING);
		break;
	case 4: /* * was POSITION_SLEEPING */
		SET_POS(mob, POS_PRONE + STAT_SLEEPING);
		break;
	case 5: /* * was POSITION_RESTING */
		SET_POS(mob, POS_SITTING + STAT_RESTING);
		break;
	case 6: /* * was POSITION_SITTING */
		SET_POS(mob, POS_SITTING + STAT_NORMAL);
		break;
	case 7: /* * was POSITION_FIGHTING */
		logit(LOG_DEBUG, "Mob %d loaded fighting.", mob_index[nr].virtual_number);
		[[fallthrough]];
	case 8: /* * was POSITION_STANDING */
		SET_POS(mob, POS_STANDING + STAT_NORMAL);
		break;
	case 9: /* * was POSITION_SWIMMING? */
		SET_POS(mob, POS_PRONE + STAT_NORMAL);
		break;
	case 10: /* * was POSITION_FLYING */
		SET_POS(mob, POS_STANDING + STAT_NORMAL);
		SET_BIT(mob->specials.affected_by, AFF_FLY);
		break;
	case 11: /* * was POSITION_LEVITATING */
		SET_POS(mob, POS_STANDING + STAT_NORMAL);
		SET_BIT(mob->specials.affected_by, AFF_LEVITATE);
		break;
	}

	tmp = mob->only.npc->default_pos;
	mob->only.npc->default_pos = mob->specials.position;
	mob->specials.position = tmp;

	REQUIRED_FSCANF(mob_f, " %ld \n", &tmp);
	mob->player.sex = tmp;

	if (letter == 'S')
	{
		mob->player.time.birth = capsule ? capsule->clock() : time(0);
		mob->player.time.played = 0;
		mob->player.time.logon = capsule ? capsule->clock() : time(0);

		for (i = 0; i < 3; i++)
			GET_COND(mob, i) = -1;

		for (i = 0; i < 5; i++)
			mob->specials.apply_saving_throw[i] = 0;

		if ((GET_LEVEL(mob) > 50) || IS_GREATER_RACE(mob) || IS_ELITE(mob))
		{
			roll_basic_attributes(mob, ROLL_MOB_ELITE);
		}
		else if (GET_LEVEL(mob) > 5)
		{
			roll_basic_attributes(mob, ROLL_MOB_GOOD);
		}
		else
		{
			roll_basic_attributes(mob, ROLL_MOB_NORMAL);
		}

		if (strstr(mob->player.name, "guard") || strstr(mob->player.name, "elite") ||
		    strstr(mob->player.name, "militia") || strstr(mob->player.name, "fighter") ||
		    strstr(mob->player.name, "warrior"))
		{
			while (mob->base_stats.Str <
			       (capsule ? capsule->integer_input("class-minimum", 1, 0,
								 min_stats_for_class[1][0]) :
					  min_stats_for_class[1][0]))
				mob->base_stats.Str += number(10, 20);
			while (mob->base_stats.Dex <
			       (capsule ? capsule->integer_input("class-minimum", 1, 1,
								 min_stats_for_class[1][1]) :
					  min_stats_for_class[1][1]))
				mob->base_stats.Dex += number(10, 20);
			while (mob->base_stats.Agi <
			       (capsule ? capsule->integer_input("class-minimum", 1, 2,
								 min_stats_for_class[1][2]) :
					  min_stats_for_class[1][2]))
				mob->base_stats.Agi += number(10, 20);
			while (mob->base_stats.Con <
			       (capsule ? capsule->integer_input("class-minimum", 1, 3,
								 min_stats_for_class[1][3]) :
					  min_stats_for_class[1][3]))
				mob->base_stats.Con += number(10, 20);
		}
		if (strstr(mob->player.name, "thief") || strstr(mob->player.name, "rogue") ||
		    strstr(mob->player.name, "bandit") || strstr(mob->player.name, "assassin"))
		{
			while (mob->base_stats.Dex <
			       (capsule ? capsule->integer_input("class-minimum", 13, 1,
								 min_stats_for_class[13][1]) :
					  min_stats_for_class[13][1]))
				mob->base_stats.Dex += number(10, 20);
			while (mob->base_stats.Agi <
			       (capsule ? capsule->integer_input("class-minimum", 13, 2,
								 min_stats_for_class[13][2]) :
					  min_stats_for_class[13][2]))
				mob->base_stats.Agi += number(10, 20);
			while (mob->base_stats.Int <
			       (capsule ? capsule->integer_input("class-minimum", 13, 5,
								 min_stats_for_class[13][5]) :
					  min_stats_for_class[13][5]))
				mob->base_stats.Int += number(10, 20);
			while (mob->base_stats.Cha <
			       (capsule ? capsule->integer_input("class-minimum", 13, 7,
								 min_stats_for_class[13][7]) :
					  min_stats_for_class[13][7]))
				mob->base_stats.Cha += number(10, 20);
			if (GET_CLASS(mob, CLASS_ROGUE) && (!mob_index[GET_RNUM(mob)].func.mob))
			{
				if (capsule)
					capsule->note_fallback(thief);
				mob_index[GET_RNUM(mob)].func.mob = thief;
			}
		}

		// Start at first class, run through CLASS_COUNT and make sure they meet minimum requirements.
		for (int cls = 0; cls < CLASS_COUNT; cls++)
		{
			// If they have the class, make sure the stats fit.
			if (GET_CLASS(mob, 1 << cls))
			{
				// Str = 0, Dex = 1, Agi = 2, Con = 3, Pow = 4, Int = 5, Wis = 6, Cha = 7
				while (mob->base_stats.Str <
				       (capsule ? capsule->integer_input(
							  "class-minimum", cls + 1, 0,
							  min_stats_for_class[cls + 1][0]) :
						  min_stats_for_class[cls + 1][0]))
					mob->base_stats.Str += number(10, 20);
				while (mob->base_stats.Dex <
				       (capsule ? capsule->integer_input(
							  "class-minimum", cls + 1, 1,
							  min_stats_for_class[cls + 1][1]) :
						  min_stats_for_class[cls + 1][1]))
					mob->base_stats.Dex += number(10, 20);
				while (mob->base_stats.Agi <
				       (capsule ? capsule->integer_input(
							  "class-minimum", cls + 1, 2,
							  min_stats_for_class[cls + 1][2]) :
						  min_stats_for_class[cls + 1][2]))
					mob->base_stats.Agi += number(10, 20);
				while (mob->base_stats.Con <
				       (capsule ? capsule->integer_input(
							  "class-minimum", cls + 1, 3,
							  min_stats_for_class[cls + 1][3]) :
						  min_stats_for_class[cls + 1][3]))
					mob->base_stats.Con += number(10, 20);
				while (mob->base_stats.Pow <
				       (capsule ? capsule->integer_input(
							  "class-minimum", cls + 1, 4,
							  min_stats_for_class[cls + 1][4]) :
						  min_stats_for_class[cls + 1][4]))
					mob->base_stats.Pow += number(10, 20);
				while (mob->base_stats.Int <
				       (capsule ? capsule->integer_input(
							  "class-minimum", cls + 1, 5,
							  min_stats_for_class[cls + 1][5]) :
						  min_stats_for_class[cls + 1][5]))
					mob->base_stats.Int += number(10, 20);
				while (mob->base_stats.Wis <
				       (capsule ? capsule->integer_input(
							  "class-minimum", cls + 1, 6,
							  min_stats_for_class[cls + 1][6]) :
						  min_stats_for_class[cls + 1][6]))
					mob->base_stats.Wis += number(10, 20);
				while (mob->base_stats.Cha <
				       (capsule ? capsule->integer_input(
							  "class-minimum", cls + 1, 7,
							  min_stats_for_class[cls + 1][7]) :
						  min_stats_for_class[cls + 1][7]))
					mob->base_stats.Cha += number(10, 20);
			}
		}

		mob->base_stats.Str = BOUNDED(25, mob->base_stats.Str, 100);
		mob->base_stats.Dex = BOUNDED(25, mob->base_stats.Dex, 100);
		mob->base_stats.Agi = BOUNDED(25, mob->base_stats.Agi, 100);
		mob->base_stats.Con = BOUNDED(25, mob->base_stats.Con, 100);
		mob->base_stats.Pow = BOUNDED(25, mob->base_stats.Pow, 100);
		mob->base_stats.Int = BOUNDED(25, mob->base_stats.Int, 100);
		mob->base_stats.Wis = BOUNDED(25, mob->base_stats.Wis, 100);
		mob->base_stats.Cha = BOUNDED(25, mob->base_stats.Cha, 100);

		/* * variable mana */
		i = 80 + dice(MAX(1, GET_LEVEL(mob)), IS_ANIMAL(mob) ? 1 : 4) + GET_LEVEL(mob) * 2;

		/* a few special cases to up things a bit */
		if (IS_ELITE(mob) || IS_GREATER_RACE(mob))
		{
			i += GET_LEVEL(mob) * 5;
		}

		/* at this point, i ranges from 83 to 696, there are a few other cases */
		if (GET_LEVEL(mob) >= 56)
			i += 1000;
		else if (GET_LEVEL(mob) > 53)
			i += 500;
		else if (GET_LEVEL(mob) > 50)
			i += 150;

		mob->points.mana = mob->points.base_mana = mob->points.max_mana = i;

		mob->points.max_vitality =
			mob->base_stats.Agi +
			(mob->base_stats.Str + mob->base_stats.Con) / ((IS_ANIMAL(mob)) ? 1 : 2);
		if (mob->points.max_vitality < 50)
			mob->points.max_vitality = 50;
		mob->points.vitality = mob->points.base_vitality = mob->points.max_vitality;
	}
	else
	{ /* The old monsters are down below here */
		REQUIRED_FSCANF(mob_f, " %s ", Gbuf1);
		mob->player.race = 0;

		/* defaults to RACE_NONE */
		for (i = 0; (i <= LAST_RACE) && !mob->player.race; i++)
			if (!str_cmp((capsule ? capsule->race_code(i, race_names_table[i].code) :
						race_names_table[i].code),
				     Gbuf1))
				mob->player.race = i;

		logit(LOG_MOB, "Old style mob: %d Race: %s(%d)", mob_index[nr].virtual_number,
		      Gbuf1, mob->player.race);

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		//    GET_LEVEL(mob) = tmp;
		mob->player.level = tmp;

#if defined(CTF_MUD) && (CTF_MUD == 1)
		if (!IS_SET(mob->specials.act, ACT_ELITE))
			mob->player.level = (int)(mob->player.level / 2);
		if (IS_SET(mob->specials.act, ACT_ELITE))
			mob->player.level -= number(5, 15);
		if (IS_SET(mob->specials.act, ACT_TEACHER) ||
		    IS_SET(mob->specials.act, ACT_SPEC_TEACHER))
			mob->player.level = 56;
#endif

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
		mob->player.time.birth = capsule ? capsule->clock() : time(0);
		mob->player.time.played = 0;
		mob->player.time.logon = capsule ? capsule->clock() : time(0);

		REQUIRED_FSCANF(mob_f, " %ld ", &tmp); /* weight */

		REQUIRED_FSCANF(mob_f, " %ld \n", &tmp); /* height */

		for (i = 0; i < 3; i++)
		{
			REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
			GET_COND(mob, i) = tmp;
		}
		REQUIRED_FSCANF_NO_FIELDS(mob_f, " \n ");

		for (i = 0; i < 5; i++)
		{
			REQUIRED_FSCANF(mob_f, " %ld ", &tmp);
			mob->specials.apply_saving_throw[i] = tmp;
		}

		REQUIRED_FSCANF_NO_FIELDS(mob_f, " \n ");

		/* Set the damage as some standard 1d6 */
		REQUIRED_FSCANF(mob_f, " %ldd%ld+%ld %ld\n", &tmp, &tmp2, &tmp3, &tmp4);
		mob->points.base_damroll = mob->points.damroll = tmp3 + level / 2;
		mob->points.damnodice = tmp;
		mob->points.damsizedice = tmp2;
		/* was warping things.  Tempy fix til everything changes.  JAB */
		if (IS_WARRIOR(mob) || IS_GREATER_RACE(mob) || IS_GIANT(mob) || IS_ELITE(mob))
		{
			mob->points.base_hitroll = BOUNDED(2, (GET_LEVEL(mob) >> 1), 25);
		}
		else
		{
			mob->points.base_hitroll = BOUNDED(0, (GET_LEVEL(mob) / 3), 25);
		}
		mob->points.hitroll = mob->points.base_hitroll;

		/* read in amount of money the mob is carrying */
		REQUIRED_FGETS(buf, sizeof(buf) - 1, mob_f);
		if (sscanf(buf, " %ld.%ld.%ld.%ld %ld", &tmp1, &tmp2, &tmp3, &tmp4, &tmp) == 5)
		{
			GET_COPPER(mob) = tmp1;
			GET_SILVER(mob) = tmp2;
			GET_GOLD(mob) = tmp3;
			GET_PLATINUM(mob) = tmp4;
			GET_EXP(mob) = tmp * (capsule ?
						      capsule->real_input("parsed-exp",
									  exp_mods[EXPMOD_GLOBAL]) :
						      exp_mods[EXPMOD_GLOBAL]);
		}
		else
		{
			tmp1 = 0;
			tmp = 0;
			if (sscanf(buf, " %ld %ld", &tmp1, &tmp) == 2)
			{
				ADD_MONEY(mob, tmp1);
				GET_EXP(mob) =
					tmp *
					(capsule ? capsule->real_input("parsed-exp",
								       exp_mods[EXPMOD_GLOBAL]) :
						   exp_mods[EXPMOD_GLOBAL]);
			}
			else
			{
				fatal_boot_error("db",
						 "boot_mobiles: bogus cash and/or exp for mob %d",
						 mob_index[nr].virtual_number);
			}
		}
	}

	foo = GET_DAMROLL(mob);
	foo += mob->points.damnodice * ((1 + mob->points.damsizedice) >> 1);
	if (IS_GREATER_RACE(mob) || IS_ELITE(mob))
	{
		bar = MIN(foo, 400);
	}
	else
	{
		bar = MIN(foo, 200);
	}

	if (foo > bar)
	{
		foo = bar;
		logit(LOG_MOB,
		      "FYI - no changes made to MOB: %d has _RIDICULOUS_ damage. %dd%d + %d (%d to %d) check mob code, stats and racial stats.",
		      mob_index[nr].virtual_number, mob->points.damnodice, mob->points.damsizedice,
		      GET_DAMROLL(mob), GET_DAMROLL(mob) + mob->points.damnodice,
		      GET_DAMROLL(mob) + (mob->points.damnodice * mob->points.damsizedice));
	}

	mob->curr_stats = mob->base_stats;

	/* Set up an attack type */
	mob->only.npc->attack_type = GetFormType(mob);

	clearMemory(mob);

	if (!mobile_probe_mode && IS_SET(mob->specials.act, ACT_SPEC) &&
	    (mob_index[nr].func.mob == 0))
	{
		REMOVE_BIT(mob->specials.act, ACT_SPEC);
		if (mob_index[nr].number == (detached ? 0 : 1)) /*
		                                * only first, not every
		                                */
			logit(LOG_MOB, "ACT_SPEC, but no function: %d %s",
			      mob_index[nr].virtual_number, GET_NAME(mob));
	}
	/* if they have a func but no spec bit, add one -- DTS 2/12/95 */
	if (mob_index[nr].func.mob && !IS_SET(mob->specials.act, ACT_SPEC))
		SET_BIT(mob->specials.act, ACT_SPEC);
	if (IS_SHOPKEEPER(mob))
	{
		SET_BIT(mob->specials.act, ACT_BREAK_CHARM);
		SET_BIT(mob->specials.act, ACT_SPEC_DIE);
	}

	if (IS_ACT(mob, ACT_TEACHER) && !mob_index[nr].func.mob)
	{
		if (capsule)
			capsule->note_fallback(teacher);
		mob_index[nr].func.mob = teacher;
		SET_BIT(mob->specials.act, ACT_SPEC);
	}

	if (mob->nevents)
	{
		disarm_char_nevents(mob, NULL);
	}

	/* init a periodic event for each mob */
	if (!detached && !mobile_probe_mode)
		schedule_mobile_periodic(mob, false);

	if (capsule && !capsule->before_conversion(mob))
		return nullptr;
	convertMob(mob, apply_mob_gold);
	if (capsule && !capsule->after_conversion(mob))
		return nullptr;

	if (!detached && !mobile_probe_mode && IS_AFFECTED(mob, AFF_STONE_SKIN | AFF_BIOFEEDBACK))
		add_event(event_mob_skin_spell, number(1, 5), mob, 0, 0, 0, 0, 0);

	// The legacy list is linked early; publish identity only after initialization.
	if (!detached)
	{
		register_character_runtime_id(mob);
		if (!mobile_probe_mode)
			character_maintenance_enter(mob);
	}
	cleanup.character = nullptr;
	return (mob);
}

// read a mobile from MOB_FILE. `apply_mob_gold` is false for callers that will
// link the new NPC as a player's pet after loading it.
P_char read_mobile(int nr, int type, bool apply_mob_gold)
{
	return read_mobile_body(nr, type, apply_mob_gold, false);
}

namespace
{
bool construct_native_mobile_capsule(void *opaque)
{
	auto &session = *static_cast<native_mobile_constructor_session *>(opaque);
	session.prepared = read_mobile_body(session.rnum, REAL, session.apply_gold, true, &session);
	session.cleanup.character = session.prepared;
	return session.prepared && session.clocks == 2;
}

bool constructor_cache_matches(int rnum,
			       const quest_mobile_native_constructor_recipe &recipe) noexcept
{
	const auto &entry = mob_index[rnum];
	const std::array<const char *, 4> strings{ entry.keys, entry.desc2, entry.desc1,
						   entry.desc3 };
	for (size_t i = 0; i < strings.size(); ++i)
		if (strings[i])
		{
			constructor_digest digest{};
			if (!constructor_string_digest(strings[i], &digest) ||
			    digest != recipe.string_digests[i])
				return false;
		}
	return true;
}

bool constructor_finish_capsule(native_mobile_constructor_session &session,
				quest_mobile_native_constructor_recipe *recipe) noexcept
{
	if (!session.prepared || !recipe || session.clocks != 2)
		return false;
	const long end = ftell(mob_f);
	const long position = mob_index[session.rnum].pos;
	if (end <= position)
		return false;
	const uint64_t length = static_cast<uint64_t>(end - position);
	constructor_digest raw{};
	if (!constructor_template_digest(position, length, &raw) || !session.inputs_complete)
		return false;
	const std::array<const char *, 4> strings{ session.prepared->player.name,
						   session.prepared->player.short_descr,
						   session.prepared->player.long_descr,
						   session.prepared->player.description };
	std::array<constructor_digest, 4> digests{};
	for (size_t i = 0; i < strings.size(); ++i)
		if (!constructor_string_digest(strings[i], &digests[i]))
			return false;
	constructor_hash cached;
	for (const auto &value : digests)
		cached.bytes(value.data(), value.size());
	constructor_digest cache_digest{};
	constructor_binding after, quest;
	if (!cached.finish(&cache_digest) ||
	    !constructor_binding_tag(mob_index[session.rnum].func.mob, &after) ||
	    !constructor_binding_tag(mob_index[session.rnum].qst_func, &quest) ||
	    quest != recipe->quest_binding ||
	    !constructor_binding_transition(recipe->binding_before, after))
		return false;
	if (session.replay)
		return recipe->template_bytes == length && recipe->template_digest == raw &&
		       recipe->effective_inputs_digest == session.effective &&
		       recipe->constructor_birthplace_vnum == GET_BIRTHPLACE(session.prepared) &&
		       recipe->string_digests == digests &&
		       recipe->cached_strings_digest == cache_digest &&
		       recipe->binding_after == after;
	recipe->template_bytes = length;
	recipe->template_digest = raw;
	recipe->effective_inputs_digest = session.effective;
	recipe->constructor_birthplace_vnum = GET_BIRTHPLACE(session.prepared);
	recipe->string_digests = digests;
	recipe->cached_strings_digest = cache_digest;
	recipe->binding_after = after;
	return true;
}

bool constructor_finish_capsule_v2(native_mobile_constructor_session &session,
				   quest_mobile_native_constructor_recipe *recipe,
				   qst_func_type original_quest_binding) noexcept
{
	if (!session.prepared || !recipe || session.clocks != 2 ||
	    (recipe->wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION &&
	     recipe->wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) ||
	    mob_index[session.rnum].qst_func != original_quest_binding)
		return false;
	const auto actual_after = mob_index[session.rnum].func.mob;
	const bool unchanged = actual_after == session.original_binding;
	const bool original_fallback = !session.original_binding &&
				       actual_after == session.installed_fallback &&
				       (actual_after == thief || actual_after == teacher);
	if (!unchanged && !original_fallback)
		return false;
	constructor_binding after, quest;
	constructor_digest procedure{}, tail{};
	if (!native_mobile_birth_procedure_capture(recipe->mobile_vnum, recipe->build_digest,
						   &procedure) ||
	    !native_mobile_birth_reset_tail_capture(recipe->mobile_vnum, recipe->reset_room_vnum,
						    recipe->reset_shop_index, &tail) ||
	    tail != recipe->reset_tail || !constructor_binding_tag_v2(actual_after, &after) ||
	    !constructor_binding_tag_v2(original_quest_binding, &quest) ||
	    quest != recipe->quest_binding ||
	    !constructor_binding_transition(recipe->binding_before, after))
		return false;
	if (!session.replay && unchanged && procedure != recipe->procedure_before)
		return false;
	if (session.replay &&
	    (procedure != recipe->procedure_after || after != recipe->binding_after))
		return false;
	const long end = ftell(mob_f);
	const long position = mob_index[session.rnum].pos;
	if (end <= position)
		return false;
	const uint64_t length = static_cast<uint64_t>(end - position);
	constructor_digest raw{};
	if (!constructor_template_digest(position, length, &raw) || !session.inputs_complete)
		return false;
	const std::array<const char *, 4> strings{ session.prepared->player.name,
						   session.prepared->player.short_descr,
						   session.prepared->player.long_descr,
						   session.prepared->player.description };
	std::array<constructor_digest, 4> digests{};
	for (size_t i = 0; i < strings.size(); ++i)
		if (!constructor_string_digest(strings[i], &digests[i]))
			return false;
	constructor_hash cached;
	for (const auto &value : digests)
		cached.bytes(value.data(), value.size());
	constructor_digest cache_digest{};
	if (!cached.finish(&cache_digest))
		return false;
	if (session.replay)
		return recipe->template_bytes == length && recipe->template_digest == raw &&
		       recipe->effective_inputs_digest == session.effective &&
		       recipe->constructor_birthplace_vnum == GET_BIRTHPLACE(session.prepared) &&
		       recipe->string_digests == digests &&
		       recipe->cached_strings_digest == cache_digest;
	recipe->template_bytes = length;
	recipe->template_digest = raw;
	recipe->effective_inputs_digest = session.effective;
	recipe->constructor_birthplace_vnum = GET_BIRTHPLACE(session.prepared);
	recipe->string_digests = digests;
	recipe->cached_strings_digest = cache_digest;
	recipe->binding_after = after;
	recipe->procedure_after = procedure;
	return true;
}
}

bool quest_mobile_native_stage::prepare_captured(
	int nr, int type, bool apply_mob_gold, const quest_mobile_native_constructor_digest &build,
	quest_mobile_native_constructor_recipe *output) noexcept
{
	if (!output || !constructor_nonzero(build) ||
	    !nevent_require_game_thread("native_mobile_constructor_capture") || character_ ||
	    publication_next_step_ || publication_step_started_ || publication_consumed_ ||
	    publication_runtime_id_ || mobile_probe_mode || (type != REAL && type != VIRTUAL) ||
	    !mob_f || !mob_index || ferror(mob_f))
		return false;
	const int rnum = type == VIRTUAL ? real_mobile(nr) : nr;
	if (rnum < 0 || rnum > top_of_mobt)
		return false;
	quest_mobile_native_constructor_recipe recipe;
	recipe.mobile_vnum = mob_index[rnum].virtual_number;
	recipe.apply_mob_gold = apply_mob_gold;
	recipe.build_digest = build;
	if (!constructor_binding_tag(mob_index[rnum].func.mob, &recipe.binding_before) ||
	    !constructor_binding_tag(mob_index[rnum].qst_func, &recipe.quest_binding))
		return false;
	native_mobile_constructor_session session;
	session.rnum = rnum;
	session.original_binding = mob_index[rnum].func.mob;
	session.apply_gold = apply_mob_gold;
	session.recipe = &recipe;
	if (!native_mobile_birth_random_owner::capture(construct_native_mobile_capsule, &session,
						       &recipe.random) ||
	    !constructor_finish_capsule(session, &recipe))
		return false;
	*output = recipe;
	character_ = session.prepared;
	session.cleanup.character = nullptr;
	session.keep_binding = true;
	return true;
}

bool quest_mobile_native_stage::prepare_captured(
	int nr, int type, bool apply_mob_gold, const quest_mobile_native_constructor_digest &build,
	int32_t reset_room_vnum, int configured_shop,
	quest_mobile_native_constructor_recipe *output) noexcept
{
	if (!output || !constructor_nonzero(build) ||
	    !nevent_require_game_thread("native_mobile_constructor_capture_v2") || character_ ||
	    publication_next_step_ || publication_step_started_ || publication_consumed_ ||
	    publication_runtime_id_ || mobile_probe_mode || (type != REAL && type != VIRTUAL) ||
	    !mob_f || !mob_index || ferror(mob_f))
		return false;
	const int rnum = type == VIRTUAL ? real_mobile(nr) : nr;
	if (rnum < 0 || rnum > top_of_mobt)
		return false;
	quest_mobile_native_constructor_recipe recipe;
	recipe.wire_version = NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION;
	recipe.mobile_vnum = mob_index[rnum].virtual_number;
	recipe.apply_mob_gold = apply_mob_gold;
	recipe.build_digest = build;
	recipe.reset_room_vnum = reset_room_vnum;
	recipe.reset_shop_index = configured_shop;
	if (!native_mobile_birth_procedure_capture(recipe.mobile_vnum, build,
						   &recipe.procedure_before) ||
	    !native_mobile_birth_reset_tail_capture(recipe.mobile_vnum, reset_room_vnum,
						    configured_shop, &recipe.reset_tail) ||
	    !constructor_binding_tag_v2(mob_index[rnum].func.mob, &recipe.binding_before) ||
	    !constructor_binding_tag_v2(mob_index[rnum].qst_func, &recipe.quest_binding))
		return false;
	const qst_func_type original_quest_binding = mob_index[rnum].qst_func;
	native_mobile_constructor_session session;
	session.rnum = rnum;
	session.original_binding = mob_index[rnum].func.mob;
	session.apply_gold = apply_mob_gold;
	session.recipe = &recipe;
	if (!native_mobile_birth_random_owner::capture(construct_native_mobile_capsule, &session,
						       &recipe.random) ||
	    !constructor_finish_capsule_v2(session, &recipe, original_quest_binding) ||
	    !native_mobile_birth_constructor_recipe_valid(recipe))
		return false;
	*output = recipe;
	character_ = session.prepared;
	session.cleanup.character = nullptr;
	session.keep_binding = true;
	return true;
}

bool quest_mobile_native_stage::restore_constructor(
	const quest_mobile_native_constructor_recipe &original,
	const quest_mobile_native_constructor_digest &build) noexcept
{
	if (original.wire_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION ||
	    original.wire_version == NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION)
		return restore_constructor_v2(original, build);
	if (original.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_VERSION)
		return false;
	if (!constructor_nonzero(build) || build != original.build_digest ||
	    !nevent_require_game_thread("native_mobile_constructor_restore") || character_ ||
	    publication_next_step_ || publication_step_started_ || publication_consumed_ ||
	    publication_runtime_id_ || mobile_probe_mode || !mob_f || !mob_index || ferror(mob_f) ||
	    !constructor_binding_transition(original.binding_before, original.binding_after))
		return false;
	for (const int64_t clock : original.clock_values)
		if (static_cast<int64_t>(static_cast<time_t>(clock)) != clock)
			return false;
	const int rnum = real_mobile(original.mobile_vnum);
	if (rnum < 0 || rnum > top_of_mobt)
		return false;
	constructor_binding actual, quest;
	constructor_digest raw{};
	if (!constructor_binding_tag(mob_index[rnum].func.mob, &actual) ||
	    (actual != original.binding_before && actual != original.binding_after) ||
	    !constructor_binding_tag(mob_index[rnum].qst_func, &quest) ||
	    quest != original.quest_binding || !constructor_cache_matches(rnum, original) ||
	    !constructor_template_digest(mob_index[rnum].pos, original.template_bytes, &raw) ||
	    raw != original.template_digest)
		return false;
	// Only this private detached path uses retained times and the local RNG replay.
	// Ordinary loading and original publication callbacks are untouched.
	quest_mobile_native_constructor_recipe recipe = original;
	native_mobile_constructor_session session;
	session.rnum = rnum;
	session.original_binding = mob_index[rnum].func.mob;
	session.apply_gold = recipe.apply_mob_gold;
	session.replay = true;
	session.recipe = &recipe;
	if (!native_mobile_birth_random_owner::replay(recipe.random,
						      construct_native_mobile_capsule, &session) ||
	    !constructor_finish_capsule(session, &recipe))
		return false;
	character_ = session.prepared;
	session.cleanup.character = nullptr;
	session.keep_binding = true;
	return true;
}

bool quest_mobile_native_stage::restore_constructor_v2(
	const quest_mobile_native_constructor_recipe &original,
	const quest_mobile_native_constructor_digest &build) noexcept
{
	if ((original.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_SUCCESSOR_VERSION &&
	     original.wire_version != NATIVE_MOBILE_BIRTH_CONSTRUCTOR_RECIPE_ALCHEMIST_VERSION) ||
	    !native_mobile_birth_constructor_recipe_valid(original) ||
	    !constructor_nonzero(build) || build != original.build_digest ||
	    !nevent_require_game_thread("native_mobile_constructor_restore_v2") || character_ ||
	    publication_next_step_ || publication_step_started_ || publication_consumed_ ||
	    publication_runtime_id_ || mobile_probe_mode || !mob_f || !mob_index || ferror(mob_f) ||
	    !constructor_binding_transition(original.binding_before, original.binding_after) ||
	    (original.binding_before == original.binding_after &&
	     original.procedure_before != original.procedure_after))
		return false;
	for (const int64_t clock : original.clock_values)
		if (static_cast<int64_t>(static_cast<time_t>(clock)) != clock)
			return false;
	const int rnum = real_mobile(original.mobile_vnum);
	if (rnum < 0 || rnum > top_of_mobt)
		return false;
	constructor_binding actual, quest;
	constructor_digest procedure{}, tail{}, raw{};
	if (!native_mobile_birth_procedure_capture(original.mobile_vnum, build, &procedure) ||
	    !native_mobile_birth_reset_tail_capture(original.mobile_vnum, original.reset_room_vnum,
						    original.reset_shop_index, &tail) ||
	    tail != original.reset_tail ||
	    !constructor_binding_tag_v2(mob_index[rnum].func.mob, &actual) ||
	    !constructor_binding_tag_v2(mob_index[rnum].qst_func, &quest) ||
	    quest != original.quest_binding ||
	    !((actual == original.binding_before && procedure == original.procedure_before) ||
	      (actual == original.binding_after && procedure == original.procedure_after)) ||
	    !constructor_cache_matches(rnum, original) ||
	    !constructor_template_digest(mob_index[rnum].pos, original.template_bytes, &raw) ||
	    raw != original.template_digest)
		return false;
	const qst_func_type original_quest_binding = mob_index[rnum].qst_func;
	quest_mobile_native_constructor_recipe recipe = original;
	native_mobile_constructor_session session;
	session.rnum = rnum;
	session.original_binding = mob_index[rnum].func.mob;
	session.apply_gold = recipe.apply_mob_gold;
	session.replay = true;
	session.recipe = &recipe;
	if (!native_mobile_birth_random_owner::replay(recipe.random,
						      construct_native_mobile_capsule, &session) ||
	    !constructor_finish_capsule_v2(session, &recipe, original_quest_binding))
		return false;
	character_ = session.prepared;
	session.cleanup.character = nullptr;
	session.keep_binding = true;
	return true;
}

bool quest_mobile_native_stage::prepare(int nr, int type, bool apply_mob_gold)
{
	if (!nevent_require_game_thread("native_mobile_prepare") || character_ ||
	    mobile_probe_mode || (type != REAL && type != VIRTUAL) || !mob_f || !mob_index)
		return false;
	const int rnum = type == VIRTUAL ? real_mobile(nr) : nr;
	if (rnum < 0 || rnum > top_of_mobt)
		return false;
	P_char prepared = read_mobile_body(rnum, REAL, apply_mob_gold, true);
	if (!prepared)
		return false;
	character_ = prepared;
	return true;
}

bool quest_mobile_native_stage::discard_empty() noexcept
{
	if (!nevent_require_game_thread("native_mobile_discard") || !character_ ||
	    !discard_unpublished_mobile(character_))
		return false;
	character_ = nullptr;
	return true;
}

bool quest_mobile_native_stage::publish(int room_rnum, P_char *live_after_hooks)
{
	if (!nevent_require_game_thread("native_mobile_publish") || !character_ ||
	    !live_after_hooks || mobile_probe_mode || !world || room_rnum < 0 ||
	    room_rnum > top_of_world)
		return false;
	P_char mob = character_;
	if (!IS_NPC(mob) || !mob->only.npc || mob->in_room != NOWHERE || mob->next ||
	    mob->next_in_room || mob->desc || mob->nevents || mob->nevents_tail ||
	    mob->character_maintenance_in_world || !mob->runtime_id || !IS_ALIVE(mob) ||
	    find_character_by_runtime_id(mob->runtime_id))
		return false;
	for (P_char live = character_list; live; live = live->next)
		if (live == mob)
			return false;
	const int nr = mob->only.npc->R_num;
	if (nr < 0 || nr > top_of_mobt || mob_index[nr].number == INT_MAX)
		return false;
	const uint64_t runtime_id = mob->runtime_id;
	// Registration allocation failure leaves the stage unlinked and retained.
	register_character_runtime_id(mob);
	mob->next = character_list;
	character_list = mob;
	++mob_index[nr].number;
	character_ = nullptr;
	*live_after_hooks = nullptr;
	// Consumption precedes all room/special callbacks. No native commit is inferred.
	if (!char_to_room(mob, room_rnum, -2) || find_character_by_runtime_id(runtime_id) != mob)
	{
		*live_after_hooks = find_character_by_runtime_id(runtime_id);
		return true;
	}
	if (!schedule_mobile_periodic(mob, true))
		return true;
	if (IS_AFFECTED(mob, AFF_STONE_SKIN | AFF_BIOFEEDBACK))
		add_event(event_mob_skin_spell, number(1, 5), mob, 0, 0, 0, 0, 0);
	character_maintenance_enter(mob);
	*live_after_hooks = find_character_by_runtime_id(runtime_id);
	return true;
}

bool quest_mobile_native_stage::adopt_published(
	P_char actual, uint64_t actual_runtime_id, int room_rnum,
	const quest_mobile_native_image &original,
	const native_mobile_birth_recovery_context &context) noexcept
{
	if (!nevent_is_game_thread() || character_ || publication_next_step_ ||
	    publication_step_started_ || publication_consumed_ || publication_runtime_id_ ||
	    !actual || !actual_runtime_id || actual->runtime_id != actual_runtime_id ||
	    find_character_by_runtime_id(actual_runtime_id) != actual || !IS_NPC(actual) ||
	    !actual->only.npc || !IS_ALIVE(actual) || actual->desc || !mob_index || !world ||
	    room_rnum < 0 || room_rnum > top_of_world ||
	    original.state != quest_mobile_lifetime_state::live || !original.cash ||
	    original.cash->revision != 1 ||
	    original.last_transition_operation.bytes != original.reference.birth_operation.bytes ||
	    world[room_rnum].number != original.reference.birthplace_vnum)
		return false;
	try
	{
		size_t prefix = 0;
		bool gap = false;
		for (size_t step = 0; step < context.mobile_effects.size(); ++step)
		{
			const auto &effect = context.mobile_effects[step];
			if (effect.periodic && (step != 3 || !effect.returned || !effect.succeeded))
				return false;
			if (!effect.started)
			{
				if (effect.returned || effect.succeeded || effect.periodic)
					return false;
				gap = true;
			}
			else
			{
				if (gap || !effect.returned || !effect.succeeded)
					return false;
				++prefix;
			}
		}
		if (!prefix || !context.mobile_publication.started ||
		    !context.mobile_publication.consumed ||
		    context.mobile_publication.returned != (prefix == 8) ||
		    !context.whole_binding.started || !context.whole_binding.returned ||
		    !context.whole_binding.succeeded || !context.reference_install.started ||
		    !context.reference_install.returned || !context.reference_install.succeeded)
			return false;
		constexpr std::array<size_t, 4> schedule_steps{ 2, 4, 5, 6 };
		for (size_t index = 0; index < schedule_steps.size(); ++index)
		{
			const auto &choice = context.mobile_choices[index];
			const auto step = schedule_steps[index];
			if ((!choice.chosen && (choice.requested || choice.delay)) ||
			    (choice.chosen &&
			     (prefix < step ||
			      (choice.requested ? choice.delay <= 0 : choice.delay != 0))) ||
			    (prefix > step && !choice.chosen) ||
			    (index == 0 && choice.chosen && !choice.requested) ||
			    (index == 1 && choice.chosen &&
			     choice.requested != context.mobile_effects[3].periodic))
				return false;
		}
		if (actual->only.npc->R_num < 0 || actual->only.npc->R_num > top_of_mobt ||
		    mob_index[actual->only.npc->R_num].virtual_number !=
			    original.reference.mobile_vnum ||
		    mob_index[actual->only.npc->R_num].number <= 0)
			return false;
		size_t matching = 0;
		std::unordered_set<P_char> global_seen;
		for (P_char ch = character_list; ch; ch = ch->next)
		{
			if (!global_seen.insert(ch).second)
				return false;
			if (ch == actual || ch->runtime_id == actual_runtime_id)
			{
				if (ch != actual)
					return false;
				++matching;
			}
			quest_mobile_native_reference bound;
			if (ch != actual &&
			    quest_mobile_native_reference_copy(ch, ch->runtime_id, &bound) &&
			    bound.mobile_instance_id == original.reference.mobile_instance_id)
				return false;
		}
		if (matching != 1)
			return false;
		quest_mobile_native_reference bound;
		std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> observed_reference{},
			original_reference{};
		if (!quest_mobile_native_reference_copy(actual, actual_runtime_id, &bound) ||
		    quest_mobile_native_reference_encode(bound, &observed_reference) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_reference_encode(original.reference, &original_reference) !=
			    player_snapshot_codec_result::ok ||
		    observed_reference != original_reference)
			return false;
		const bool room_done = prefix > 1;
		if (actual->in_room != (room_done ? room_rnum : NOWHERE) ||
		    (!room_done && actual->next_in_room))
			return false;
		size_t room_matches = 0;
		std::unordered_set<P_char> room_seen;
		for (int room = 0; room <= top_of_world; ++room)
			for (P_char ch = world[room].people; ch; ch = ch->next_in_room)
			{
				if (!room_seen.insert(ch).second)
					return false;
				if (ch == actual)
				{
					if (!room_done || room != room_rnum)
						return false;
					++room_matches;
				}
			}
		if (room_matches != (room_done ? 1U : 0U))
			return false;
		const std::array<event_func_type, 4> callbacks{ event_mob_mundane, event_mob_proc,
								event_patrol_move,
								event_mob_skin_spell };
		std::array<size_t, 4> active{};
		P_nevent mundane = nullptr;
		std::unordered_set<P_nevent> seen;
		P_nevent previous = nullptr;
		for (P_nevent event = actual->nevents; event; event = event->next_char_nev)
		{
			if (!seen.insert(event).second || event->prev_char_nev != previous)
				return false;
			previous = event;
			for (size_t index = 0; index < callbacks.size(); ++index)
				if (event->func == callbacks[index] && event->ch == actual)
				{
					if (event->owner_runtime_id != actual_runtime_id ||
					    event->obj || event->victim ||
					    (event->data && index != 2) ||
					    !nevent_handle_is_active(
						    nevent_handle_from_event(event)))
						return false;
					++active[index];
					if (index == 0)
						mundane = event;
				}
		}
		for (size_t index = 0; index < schedule_steps.size(); ++index)
		{
			const bool required = prefix > schedule_steps[index] &&
					      context.mobile_choices[index].requested;
			if (active[index] != (required ? 1U : 0U))
				return false;
		}
		if ((mundane &&
		     (actual->world_activity_mundane_event != mundane ||
		      actual->world_activity_mundane_event_sequence != mundane->sequence)) ||
		    (!mundane && (actual->world_activity_mundane_event ||
				  actual->world_activity_mundane_event_sequence)) ||
		    actual->character_maintenance_in_world != room_done)
			return false;
		quest_mobile_native_image observed;
		std::vector<uint8_t> observed_bytes, original_bytes;
		if (quest_mobile_native_capture(
			    actual, original.reference, quest_mobile_lifetime_state::live,
			    original.reference.birth_operation, original.cash->revision,
			    &observed) != player_snapshot_capture_result::ok ||
		    quest_mobile_native_image_encode(observed, &observed_bytes) !=
			    player_snapshot_codec_result::ok ||
		    quest_mobile_native_image_encode(original, &original_bytes) !=
			    player_snapshot_codec_result::ok ||
		    observed_bytes != original_bytes)
			return false;
		// Actual world/event/body proof precedes fixed progress hydration.
		// No callback/event/count/UID or caller-visible body changes occur.
		publication_next_step_ = prefix;
		publication_runtime_id_ = actual_runtime_id;
		publication_consumed_ = true;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_native_stage::restore_published(
	int room, std::span<const native_mobile_birth_recovery_effect> effects,
	std::span<const native_mobile_birth_recovery_choice> choices, P_char *live_after) noexcept
{
	if (!live_after || !nevent_is_game_thread() || effects.size() != 8 || choices.size() != 4 ||
	    !world || !mob_index || room < 0 || room > top_of_world || mobile_probe_mode ||
	    publication_step_started_)
		return false;
	size_t prefix = 0;
	std::array<uint8_t, 8> encoded_effects{};
	std::array<uint8_t, 4> encoded_choices{};
	std::array<int32_t, 4> delays{};
	bool gap = false;
	for (size_t step = 0; step < effects.size(); ++step)
	{
		const auto &effect = effects[step];
		if (effect.periodic && (step != 3 || !effect.returned || !effect.succeeded))
			return false;
		if (!effect.started)
		{
			if (effect.returned || effect.succeeded || effect.periodic)
				return false;
			gap = true;
		}
		else
		{
			if (gap || !effect.returned || !effect.succeeded)
				return false;
			++prefix;
		}
		encoded_effects[step] = effect.started | (effect.returned << 1) |
					(effect.succeeded << 2) | (effect.periodic << 3);
	}
	if (!prefix)
		return false;
	constexpr std::array<size_t, 4> scheduled_steps{ 2, 4, 5, 6 };
	for (size_t i = 0; i < choices.size(); ++i)
	{
		const auto &choice = choices[i];
		if ((!choice.chosen && (choice.requested || choice.delay)) ||
		    (choice.chosen &&
		     (prefix < scheduled_steps[i] ||
		      (choice.requested ? choice.delay <= 0 : choice.delay != 0))) ||
		    (prefix > scheduled_steps[i] && !choice.chosen) ||
		    (i == 0 && choice.chosen && !choice.requested) ||
		    (i == 1 && choice.chosen && choice.requested != effects[3].periodic))
			return false;
		encoded_choices[i] = choice.chosen | (choice.requested << 1);
		delays[i] = choice.delay;
	}
	if (restoration_active_)
	{
		if (restoration_room_ != room || restoration_prefix_ != prefix ||
		    restoration_effects_ != encoded_effects ||
		    restoration_choices_ != encoded_choices || restoration_delays_ != delays)
			return false;
	}
	else
	{
		if (!character_ || publication_consumed_ || publication_runtime_id_ ||
		    publication_next_step_)
			return false;
		restoration_room_ = room;
		restoration_prefix_ = prefix;
		restoration_effects_ = encoded_effects;
		restoration_choices_ = encoded_choices;
		restoration_delays_ = delays;
		restoration_active_ = true;
	}
	P_char mob = publication_consumed_ ? find_character_by_runtime_id(publication_runtime_id_) :
					     character_;
	if (!mob || !IS_NPC(mob) || !mob->only.npc || !IS_ALIVE(mob) || !mob->runtime_id)
		return false;
	const int nr = mob->only.npc->R_num;
	if (nr < 0 || nr > top_of_mobt)
		return false;
	*live_after = publication_consumed_ ? mob : nullptr;
	try
	{
		if (!publication_consumed_)
		{
			if (mob->in_room != NOWHERE || mob->next || mob->next_in_room ||
			    mob->desc || mob->nevents || mob->nevents_tail ||
			    mob->character_maintenance_in_world ||
			    find_character_by_runtime_id(mob->runtime_id) ||
			    mob_index[nr].number == INT_MAX)
				return false;
			for (P_char current = character_list; current; current = current->next)
				if (current == mob || current->runtime_id == mob->runtime_id)
					return false;
			// The registry emplace has a strong allocation guarantee. All fixed
			// world/count ownership is consumed once immediately after it succeeds.
			register_character_runtime_id(mob);
			if (find_character_by_runtime_id(mob->runtime_id) != mob)
				return false;
			publication_runtime_id_ = mob->runtime_id;
			mob->next = character_list;
			character_list = mob;
			++mob_index[nr].number;
			character_ = nullptr;
			publication_consumed_ = true;
			*live_after = mob;
		}
		if (prefix > 1 && !quest_mobile_native_room_restore_owner::restore(
					  mob, room, &restoration_room_step_))
			return false;
		if (prefix == 1 && (mob->in_room != NOWHERE || restoration_room_step_))
			return false;
		const std::array<event_func_type, 4> callbacks{ event_mob_mundane, event_mob_proc,
								event_patrol_move,
								event_mob_skin_spell };
		for (size_t i = 0; i < callbacks.size(); ++i)
		{
			const bool required = prefix > scheduled_steps[i] && choices[i].requested;
			if (!required)
				continue;
			P_nevent found = nullptr;
			std::unordered_set<P_nevent> seen;
			for (P_nevent event = mob->nevents; event; event = event->next_char_nev)
			{
				if (!seen.insert(event).second)
					return false;
				if (event->func != callbacks[i])
					continue;
				if (found || event->ch != mob ||
				    event->owner_runtime_id != mob->runtime_id || event->obj ||
				    event->victim || (event->data && i != 2) ||
				    !nevent_handle_is_active(nevent_handle_from_event(event)))
					return false;
				found = event;
			}
			if (!found)
			{
				if (restoration_events_[i])
					return false; // Never replay a consumed rearm.
				const auto scheduled = add_event(callbacks[i], choices[i].delay,
								 mob, nullptr, nullptr, 0, nullptr,
								 0);
				if (!scheduled.was_scheduled())
					return false;
				found = scheduled.handle.event;
				if (i == 0)
					world_activity_record_mundane_event(mob, scheduled.handle);
			}
			if (i == 0 &&
			    (mob->world_activity_mundane_event != found ||
			     mob->world_activity_mundane_event_sequence != found->sequence))
				return false;
			restoration_events_[i] = true;
		}
		if (prefix > 7 && !mob->character_maintenance_in_world)
			return false;
		publication_next_step_ = prefix;
		return true;
	}
	catch (...)
	{
		// The body and every actual completed substep remain in this stage.
		// Historical effect/choice observations are never modified here.
		*live_after = publication_consumed_ ?
				      find_character_by_runtime_id(publication_runtime_id_) :
				      nullptr;
		return false;
	}
}

bool quest_mobile_native_stage::choose_publication_step(
	size_t step, P_char expected, const native_mobile_birth_recovery_effect &probe,
	native_mobile_birth_recovery_choice *output) noexcept
{
	if (!output || !expected || !nevent_is_game_thread() || !publication_consumed_ ||
	    publication_step_started_ || step != publication_next_step_ ||
	    find_character_by_runtime_id(publication_runtime_id_) != expected ||
	    !IS_NPC(expected) || !expected->only.npc)
		return false;
	try
	{
		native_mobile_birth_recovery_choice choice;
		choice.chosen = true;
		switch (step)
		{
		case 2:
			choice.requested = true;
			// Original draw remains after room insertion and before the special probe.
			choice.delay = world_activity_mundane_delay(expected, false, true);
			break;
		case 4:
			if (!probe.started || !probe.returned || !probe.succeeded)
				return false;
			choice.requested = probe.periodic;
			if (choice.requested)
				choice.delay = PULSE_MOBILE + number(-4, 4);
			break;
		case 5:
			choice.requested = IS_ACT(expected, ACT_PATROL);
			if (choice.requested)
				choice.delay = WAIT_SEC;
			break;
		case 6:
			choice.requested = IS_AFFECTED(expected, AFF_STONE_SKIN | AFF_BIOFEEDBACK);
			if (choice.requested)
				choice.delay = number(1, 5);
			break;
		default:
			return false;
		}
		*output = choice;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_native_stage::publication_step(size_t step, int room_rnum, P_char expected,
						 const native_mobile_birth_recovery_choice &choice,
						 native_mobile_birth_recovery_effect &effect,
						 P_char *live_after) noexcept
{
	if (!nevent_is_game_thread() || !live_after || !expected || step >= 8 ||
	    step != publication_next_step_ || publication_step_started_ || effect.started ||
	    mobile_probe_mode || !world || room_rnum < 0 || room_rnum > top_of_world)
		return false;
	P_char mob = step ? find_character_by_runtime_id(publication_runtime_id_) : character_;
	if (mob != expected || !IS_NPC(mob) || !mob->only.npc || !IS_ALIVE(mob))
		return false;
	if (step && !publication_consumed_)
		return false;
	const int nr = mob->only.npc->R_num;
	if (nr < 0 || nr > top_of_mobt)
		return false;
	if (step == 0)
	{
		if (mob->in_room != NOWHERE || mob->next || mob->next_in_room || mob->desc ||
		    mob->nevents || mob->nevents_tail || mob->character_maintenance_in_world ||
		    !mob->runtime_id || find_character_by_runtime_id(mob->runtime_id) ||
		    mob_index[nr].number == INT_MAX)
			return false;
		for (P_char current = character_list; current; current = current->next)
			if (current == mob)
				return false;
	}
	const bool scheduled_step = step == 2 || step == 4 || step == 5 || step == 6;
	if (scheduled_step &&
	    (!choice.chosen || (choice.requested ? choice.delay <= 0 : choice.delay != 0)))
		return false;
	if ((step == 2 && !choice.requested) ||
	    (step == 3 && IS_SET(mob->specials.act, ACT_SPEC) && !mob_index[nr].func.mob))
		return false;
	// This stage owns the once-only latch independently of caller observations.
	publication_step_started_ = true;
	effect.started = true;
	*live_after = nullptr;
	try
	{
		switch (step)
		{
		case 0:
			// All allocation precedes consumption. Failure is retained started uncertainty.
			register_character_runtime_id(mob);
			publication_runtime_id_ = mob->runtime_id;
			mob->next = character_list;
			character_list = mob;
			++mob_index[nr].number;
			character_ = nullptr;
			publication_consumed_ = true;
			effect.succeeded = true;
			break;
		case 1:
			effect.succeeded = char_to_room(mob, room_rnum, -2);
			break;
		case 2:
			world_activity_schedule_mundane_after(mob, choice.delay);
			effect.succeeded = world_activity_mundane_event(mob).event != nullptr;
			break;
		case 3:
			if (IS_SET(mob->specials.act, ACT_SPEC))
			{
				effect.periodic = mob_index[nr].func.mob(mob, nullptr,
									 CMD_SET_PERIODIC, nullptr);
			}
			effect.succeeded = true;
			break;
		case 4:
			effect.succeeded =
				!choice.requested ||
				add_event(event_mob_proc, choice.delay, mob, 0, 0, 0, 0, 0)
					.was_scheduled();
			break;
		case 5:
			effect.succeeded =
				!choice.requested ||
				add_event(event_patrol_move, choice.delay, mob, 0, 0, 0, 0, 0)
					.was_scheduled();
			break;
		case 6:
			effect.succeeded =
				!choice.requested ||
				add_event(event_mob_skin_spell, choice.delay, mob, 0, 0, 0, 0, 0)
					.was_scheduled();
			break;
		case 7:
			character_maintenance_enter(mob);
			effect.succeeded = mob->character_maintenance_in_world;
			break;
		}
		// Returned is actual function return, independently of success or extraction.
		effect.returned = true;
		P_char found = find_character_by_runtime_id(publication_runtime_id_);
		if (found != mob)
			effect.succeeded = false;
		*live_after = found;
		if (effect.returned && effect.succeeded)
		{
			++publication_next_step_;
			publication_step_started_ = false;
		}
		return effect.returned && effect.succeeded;
	}
	catch (...)
	{
		return false;
	}
}

P_char read_mobile(int nr, int type)
{
	return read_mobile(nr, type, true);
}

P_char read_mobile_probe(int nr, int type)
{
	const bool previous_mode = mobile_probe_mode;
	mobile_probe_mode = true;
	try
	{
		P_char mob = read_mobile(nr, type);
		mobile_probe_mode = previous_mode;
		return mob;
	}
	catch (...)
	{
		mobile_probe_mode = previous_mode;
		throw;
	}
}

void event_object_proc(P_char /*ch*/, P_char /*victim*/, P_obj obj, void * /*data*/)
{
	if (obj_index[obj->R_num].func.obj)
		invoke_object_special(obj, 0, CMD_PERIODIC, 0);

	/* Object procs may extract their owner, which detaches this event before freeing it. */
	if (!current_nevent || current_nevent->obj != obj)
		return;

	if (obj_index[obj->R_num].number == 55434)
	{
		add_event(event_object_proc, WAIT_SEC, 0, 0, obj, 0, 0, 0);
	}
	else
	{
		add_event(event_object_proc, PULSE_MOBILE + number(-4, 4), 0, 0, obj, 0, 0, 0);
	}
}

namespace
{
std::unordered_map<int, object_template> starter_object_templates;

struct recovery_template_entry
{
	int vnum;
	long position;
	obj_proc_type special;
	object_template prototype;
};
std::vector<recovery_template_entry> recovery_object_templates;
P_index recovery_template_index = nullptr;
FILE *recovery_template_file = nullptr;
int recovery_template_top = -1;
bool recovery_template_sealed = false;

void invalidate_recovery_object_templates() noexcept
{
	recovery_template_sealed = false;
	recovery_template_index = nullptr;
	recovery_template_file = nullptr;
	recovery_template_top = -1;
	recovery_object_templates.clear();
}

struct template_read_failure
{
	unsigned int error;
};

// Recoverable text uses the same valid tilde/newline/color decisions as
// fread_string, but neither its fatal allocator nor its diagnostic callbacks.
// Bounded malformed/overlong input refuses instead of overflowing a buffer.
std::string read_recovery_template_string(FILE *file)
{
	char buffer[MAX_STRING_LENGTH] = {}, line[MAX_STRING_LENGTH] = {};
	size_t length = 0;
	for (;;)
	{
		if (!fgets(line, MAX_STRING_LENGTH - 5, file))
			throw template_read_failure{ static_cast<unsigned int>(
				ferror(file) ? EIO : EILSEQ) };
		const size_t input_length = strlen(line);
		if (!input_length)
			throw template_read_failure{ EILSEQ };
		char *last = line + input_length - 1;
		while (last > line && isspace(static_cast<unsigned char>(*last)))
			--last;
		const bool done = *last == '~';
		if (done)
			*last = '\0';
		else
		{
			last = line + input_length - 1;
			*last++ = '\r';
			*last++ = '\n';
			*last = '\0';
		}
		const size_t count = strlen(line);
		if (count >= sizeof(buffer) - length)
			throw template_read_failure{ E2BIG };
		memcpy(buffer + length, line, count + 1);
		length += count;
		if (done)
			break;
	}
	if (strstr(buffer, "&+") &&
	    !(length >= 2 && buffer[length - 2] == '&' &&
	      toupper(static_cast<unsigned char>(buffer[length - 1])) == 'N'))
	{
		if (sizeof(buffer) - length <= 2)
			throw template_read_failure{ E2BIG };
		memcpy(buffer + length, "&n", 3);
		length += 2;
	}
	return { buffer, length };
}

std::string read_template_string(FILE *file, const char *shared = nullptr)
{
	if (shared)
	{
		skip_fread(file);
		return shared;
	}
	char *text = fread_string(file);
	std::string result = text ? text : "";
	if (text)
		FREE(text);
	return result;
}

struct object_template_reader
{
	FILE *file;
	bool recoverable;

	std::string string(const char *shared = nullptr)
	{
		// Recovery provenance is the boot file, not a mutable shared text pointer.
		return recoverable ? read_recovery_template_string(file) :
				     read_template_string(file, shared);
	}

	bool word(std::string &value)
	{
		int character;
		do
			character = fgetc(file);
		while (character != EOF && isspace(static_cast<unsigned char>(character)));
		if (character == EOF)
		{
			if (ferror(file))
				throw template_read_failure{ EIO };
			return false;
		}
		value.clear();
		do
		{
			if (character == 0 || value.size() >= MAX_STRING_LENGTH - 1)
				throw template_read_failure{ E2BIG };
			value.push_back(static_cast<char>(character));
			character = fgetc(file);
		} while (character != EOF && !isspace(static_cast<unsigned char>(character)));
		// Every parser numeric/token format consumes trailing whitespace.
		while (character != EOF && isspace(static_cast<unsigned char>(character)))
			character = fgetc(file);
		if (character != EOF && ungetc(character, file) == EOF)
			throw template_read_failure{ EIO };
		if (ferror(file))
			throw template_read_failure{ EIO };
		return true;
	}

	template <typename T> int optional(T *output)
	{
		if (!recoverable)
		{
			if constexpr (std::is_same_v<T, int>)
				return fscanf(file, " %d ", output);
			else
			{
				static_assert(std::is_same_v<T, unsigned long>);
				return fscanf(file, " %lu \n", output);
			}
		}
		fpos_t before;
		if (fgetpos(file, &before))
			throw template_read_failure{ EIO };
		std::string text;
		if (!word(text))
			return EOF;
		char *end = nullptr;
		errno = 0;
		if constexpr (std::is_same_v<T, int>)
		{
			const long number = strtol(text.c_str(), &end, 10);
			if (errno != ERANGE && end != text.c_str() && !*end && number >= INT_MIN &&
			    number <= INT_MAX)
			{
				*output = static_cast<int>(number);
				return 1;
			}
		}
		else
		{
			static_assert(std::is_same_v<T, unsigned long>);
			const unsigned long number = strtoul(text.c_str(), &end, 10);
			if (errno != ERANGE && end != text.c_str() && !*end)
			{
				*output = number;
				return 1;
			}
		}
		if (fsetpos(file, &before))
			throw template_read_failure{ EIO };
		return 0;
	}

	int token(char *output)
	{
		if (!recoverable)
			return fscanf(file, " %s \n", output);
		std::string text;
		if (!word(text))
			return EOF;
		memcpy(output, text.c_str(), text.size() + 1);
		return 1;
	}

	template <typename T> void required(T *output)
	{
		if (!recoverable)
		{
			if constexpr (std::is_same_v<T, int>)
				REQUIRED_FSCANF(file, " %d ", output);
			else if constexpr (std::is_same_v<T, unsigned long>)
				REQUIRED_FSCANF(file, " %lu ", output);
			else
			{
				static_assert(std::is_same_v<T, char>);
				REQUIRED_FSCANF(file, " %s \n", output);
			}
		}
		else
		{
			int result;
			if constexpr (std::is_same_v<T, char>)
				result = token(output);
			else
				result = optional(output);
			if (result != 1)
				throw template_read_failure{ EILSEQ };
		}
	}
};

object_template parse_object_template_with_reader(int nr, object_template_reader &input)
{
	object_template result;
	auto *obj = &result;
	int tmp, i;
	unsigned long utmp;
	char chk[MAX_STRING_LENGTH];
	obj->R_num = nr;
	if (input.recoverable)
	{
		if (fseek(input.file, obj_index[nr].pos, SEEK_SET))
			throw template_read_failure{ EIO };
	}
	else
		fseek(obj_f, obj_index[nr].pos, 0);
	obj->name = input.string(obj_index[nr].keys);
	for (char &letter : obj->name)
		letter = LOWER(letter);
	obj->short_description = input.string(obj_index[nr].desc2);
	obj->description = input.string(obj_index[nr].desc1);
	obj->action_description = input.string(obj_index[nr].desc3);
	/* *** numeric data *** */

	input.required(&tmp);
	obj->type = tmp;
	input.required(&tmp);
	obj->material = tmp;
	input.required(&tmp);
	//  obj->size = tmp;
	input.required(&tmp);
	//  obj->space = tmp;
	input.required(&tmp);
	obj->craftsmanship = tmp;
	input.required(&tmp);
	//  obj->damres_bonus = tmp;
	input.required(&utmp);
	obj->extra_flags = utmp;
	input.required(&utmp);
	obj->wear_flags = utmp;
	input.required(&utmp);
	obj->extra2_flags = utmp;
	input.required(&utmp);
	obj->anti_flags = utmp;
	input.required(&utmp);
	// Hack until we make as script to edit files directly.
	if (IS_SET(obj->anti_flags, CLASS_NECROMANCER))
		SET_BIT(obj->anti_flags, CLASS_THEURGIST);
	obj->anti2_flags = utmp;
	input.required(&tmp);
	obj->value[0] = tmp;
	input.required(&tmp);
	obj->value[1] = tmp;
	input.required(&tmp);
	obj->value[2] = tmp;
	input.required(&tmp);
	obj->value[3] = tmp;
	input.required(&tmp);
	obj->value[4] = tmp;
	input.required(&tmp);
	obj->value[5] = tmp;
	input.required(&tmp);
	obj->value[6] = tmp;
	input.required(&tmp);
	obj->value[7] = tmp;
	input.required(&tmp);
	obj->weight = tmp;
	input.required(&tmp);
	obj->cost = tmp;
	input.required(&tmp);
	obj->condition = tmp;
	//  fscanf(obj_f, " %d \n", &tmp);
	//  obj->max_condition = tmp;  wipe2011
	//  if(obj->max_condition < 100)
	//    obj->max_condition = 100;

	if (input.optional(&utmp) == 1)
	{
		obj->bitvector = utmp;
		if (input.optional(&utmp) == 1)
		{
			obj->bitvector2 = utmp;
			if (input.optional(&utmp) == 1)
			{
				obj->bitvector3 = utmp;
				if (input.optional(&utmp) == 1)
					obj->bitvector4 = utmp;
			}
		}
	}
	if (input.token(chk) != 1)
		*chk = '\0';
	if (!strcmp(chk, "B5"))
	{
		if (input.optional(&utmp) == 1)
			obj->bitvector5 = utmp;
		else
		{
			if (input.recoverable)
				throw template_read_failure{ EILSEQ };
			logit(LOG_STATUS, "Object %d has an invalid B5 affect mask.",
			      obj_index[nr].virtual_number);
		}
		if (input.token(chk) != 1)
			*chk = '\0';
	}

	//  if(obj->craftsmanship > ((OBJCRAFT_HIGHEST - 1) / 2))
	//  {
	//    obj->max_condition = (int) BOUNDED(100, (50 * 1.4285 * (obj->craftsmanship - 6)), 500);
	// 1.4285 = 10 / 7(max condition is 1000, there are 7 values after average craftsmanship)
	//  }
	//  obj->condition = obj->max_condition;  wipe2011

	// nuke the proclib flag - it'll be put back if needed
	REMOVE_BIT(obj->extra_flags, ITEM_PROCLIB);

	/* *** extra descriptions *** */
	// Proc-library descriptions stay inert until main-thread publication.
	while (*chk == 'E')
	{
		object_template_description description;
		description.keyword = input.string();
		description.description = input.string();
		obj->descriptions.push_back(std::move(description));
		if (input.token(chk) != 1)
			*chk = '\0';
	}
	for (i = 0; (i < MAX_OBJ_AFFECT) && (*chk == 'A'); i++)
	{
		input.required(&tmp);
		obj->affected[i].location = tmp;
		input.required(&tmp);
		obj->affected[i].modifier = tmp;
		input.required(chk);
	}

	/* Trapped item data */
	obj->trap_eff = obj->trap_dam = obj->trap_charge = 0;
	if (*chk == 'T')
	{
		input.required(&tmp);
		obj->trap_eff = tmp;
		input.required(&tmp);
		obj->trap_dam = tmp;
		input.required(&tmp);
		obj->trap_charge = tmp;
		input.required(&tmp);
		obj->trap_level = tmp;
	}
	/* ensure builders dont mess things up */
	if (IS_SET(obj->wear_flags, ITEM_TAKE) && !IS_SET(obj->wear_flags, ITEM_HOLD))
		SET_BIT(obj->wear_flags, ITEM_HOLD);
	if (IS_SET(obj->wear_flags, ITEM_HOLD) && !IS_SET(obj->wear_flags, ITEM_TAKE))
		SET_BIT(obj->wear_flags, ITEM_TAKE);
	if (obj->type == ITEM_ARMOR && !obj->value[0])
		obj->type = ITEM_WORN;
#if 0
  if (obj->type == ITEM_ARMOR && !IS_SET(obj->wear_flags, ITEM_WEAR_BODY)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_LEGS)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_ARMS)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_HEAD)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_ABOUT)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_FEET)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_SHIELD)
      && !IS_SET(obj->wear_flags, ITEM_WEAR_HANDS))
  {
    obj->type = ITEM_WORN;
    obj->affected[2].location = APPLY_ARMOR;
    obj->affected[2].modifier = -(obj->value[0]);
  }
#endif
	/* set up a few items that are belt attachable */
	if (((GET_ITEM_TYPE(obj) == ITEM_DRINKCON) /*&& !isname("barrel", obj->name.c_str()) */
	     && (isname("canteen", obj->name.c_str()) || isname("skin", obj->name.c_str()) ||
		 isname("horn", obj->name.c_str()))) ||
	    ((GET_ITEM_TYPE(obj) == ITEM_CONTAINER) &&
	     (isname("bag", obj->name.c_str()) || isname("sack", obj->name.c_str()) ||
	      isname("tube", obj->name.c_str()) || isname("case", obj->name.c_str()) ||
	      isname("scabbard", obj->name.c_str()) || isname("pouch", obj->name.c_str())) &&
	     (obj->value[0] < 25)) ||
	    (GET_ITEM_TYPE(obj) == ITEM_QUIVER))
		SET_BIT(obj->wear_flags, ITEM_ATTACH_BELT);

	/* and some that are back */
	if ((GET_ITEM_TYPE(obj) == ITEM_CONTAINER && isname("backpack", obj->name.c_str())) ||
	    GET_ITEM_TYPE(obj) == ITEM_QUIVER)
		SET_BIT(obj->wear_flags, ITEM_WEAR_BACK);

	/* set throw flag to obj */
	if (obj->type == ITEM_WEAPON)
	{
		if (strstr(obj->name.c_str(), "axe") || strstr(obj->name.c_str(), "hammer") ||
		    strstr(obj->name.c_str(), "trident") || strstr(obj->name.c_str(), "club") ||
		    strstr(obj->name.c_str(), "dart"))
			SET_BIT(obj->extra_flags, ITEM_CAN_THROW1);
		else if (strstr(obj->name.c_str(), "dagger") ||
			 strstr(obj->name.c_str(), "spear") || strstr(obj->name.c_str(), "javelin"))
			SET_BIT(obj->extra_flags, ITEM_CAN_THROW2);
		else if (strstr(obj->name.c_str(), "boomerang"))
		{
			SET_BIT(obj->extra_flags, ITEM_CAN_THROW1);
			SET_BIT(obj->extra_flags, ITEM_CAN_THROW2);
			SET_BIT(obj->extra_flags, ITEM_RETURNING);
		}
		if (obj->value[0] == WEAPON_2HANDSWORD)
		{
			SET_BIT(obj->extra_flags, ITEM_TWOHANDS);
		}
	}

	return result;
}

object_template parse_object_template(int nr)
{
	object_template_reader input{ obj_f, false };
	return parse_object_template_with_reader(nr, input);
}

unsigned int prepare_recovery_object_templates() noexcept
{
	// This is reachable only from serialized boot, never a runtime miss path.
	invalidate_recovery_object_templates();
	const auto index = obj_index;
	FILE *const file = obj_f;
	const int top = top_of_objt;
	if (!index || !file || top < 0 || ferror(file))
		return EINVAL;
	fpos_t initial;
	if (fgetpos(file, &initial))
		return EIO;
	auto restore = [&]() noexcept
	{
		const bool success = fsetpos(file, &initial) == 0;
		clearerr(file);
		return success;
	};
	unsigned int error = 0;
	try
	{
		if (fseek(file, 0, SEEK_END))
			throw template_read_failure{ EIO };
		const long file_size = ftell(file);
		if (file_size <= 0)
			throw template_read_failure{ EIO };
		std::vector<recovery_template_entry> candidate;
		const size_t count = static_cast<size_t>(top) + 1;
		if (count > candidate.max_size())
			throw template_read_failure{ E2BIG };
		candidate.reserve(count);
		object_template_reader input{ file, true };
		for (size_t number = 0; number < count; ++number)
		{
			const auto &entry = index[number];
			if (entry.pos < 0 || entry.pos >= file_size)
				throw template_read_failure{ EILSEQ };
			candidate.push_back({ entry.virtual_number, entry.pos, entry.func.obj,
					      parse_object_template_with_reader(
						      static_cast<int>(number), input) });
		}
		// Only the private lookup ordering changes. Native index order/R_num and
		// special procedure pointers are never rewritten or assigned here.
		std::sort(candidate.begin(), candidate.end(),
			  [](const auto &a, const auto &b) { return a.vnum < b.vnum; });
		for (size_t position = 0; position < candidate.size(); ++position)
		{
			const auto &entry = candidate[position];
			const int number = entry.prototype.R_num;
			if (number < 0 || number > top ||
			    (position && candidate[position - 1].vnum == entry.vnum) ||
			    index[number].virtual_number != entry.vnum ||
			    index[number].pos != entry.position ||
			    index[number].func.obj != entry.special)
				throw template_read_failure{ EILSEQ };
		}
		if (!restore())
			return EIO;
		if (obj_index != index || obj_f != file || top_of_objt != top)
			return ESTALE;
		recovery_object_templates.swap(candidate);
		recovery_template_index = index;
		recovery_template_file = file;
		recovery_template_top = top;
		recovery_template_sealed = true;
		return 0;
	}
	catch (const template_read_failure &failure)
	{
		error = failure.error;
	}
	catch (const std::bad_alloc &)
	{
		error = ENOMEM;
	}
	catch (...)
	{
		error = EIO;
	}
	return restore() ? error : EIO;
}
} // namespace

bool recovery_object_templates_ready() noexcept
{
	return recovery_template_sealed && persistence_mode_requires_mysql() && obj_index &&
	       obj_index == recovery_template_index && obj_f == recovery_template_file &&
	       top_of_objt == recovery_template_top && top_of_objt >= 0 &&
	       recovery_object_templates.size() == static_cast<size_t>(top_of_objt) + 1;
}

bool finalize_recovery_object_template_bindings() noexcept
{
	// This serialized pre-worker boot step snapshots existing bindings only.
	// Out-of-phase callers may not mutate a live or foreign-thread catalog.
	if (!nevent_is_game_thread() || game_booted || !persistence_mode_requires_mysql())
		return false;
	if (!recovery_object_templates_ready())
	{
		invalidate_recovery_object_templates();
		return false;
	}
	// Prove every parsed entry first. No partial function snapshot is exposed
	// if any native identity changed; no parser/allocation/callback is used.
	for (size_t position = 0; position < recovery_object_templates.size(); ++position)
	{
		const auto &entry = recovery_object_templates[position];
		const int number = entry.prototype.R_num;
		if (number < 0 || number > top_of_objt || entry.position < 0 ||
		    (position && recovery_object_templates[position - 1].vnum >= entry.vnum) ||
		    obj_index[number].virtual_number != entry.vnum ||
		    obj_index[number].pos != entry.position)
		{
			invalidate_recovery_object_templates();
			return false;
		}
	}
	// All existing startup assignments are now complete. Only the catalog's
	// binding snapshot changes; parsed values/addresses and native index stay.
	for (auto &entry : recovery_object_templates)
		entry.special = obj_index[entry.prototype.R_num].func.obj;
	return true;
}

const object_template *find_recovery_object_template(int vnum) noexcept
{
	if (!recovery_object_templates_ready())
		return nullptr;
	const auto found = std::lower_bound(recovery_object_templates.begin(),
					    recovery_object_templates.end(), vnum,
					    [](const auto &entry, int value)
					    { return entry.vnum < value; });
	if (found == recovery_object_templates.end() || found->vnum != vnum)
		return nullptr;
	const int number = found->prototype.R_num;
	if (number < 0 || number > top_of_objt || obj_index[number].virtual_number != vnum ||
	    obj_index[number].pos != found->position ||
	    obj_index[number].func.obj != found->special)
		return nullptr;
	return &found->prototype;
}

bool shop_trade_original_procedure_binding_stage::prepare(
	std::span<const P_obj> objects, std::span<const player_item_snapshot> literals,
	shop_trade_original_procedure_binding_stage &output) noexcept
{
	if (!nevent_is_game_thread() || !persistence_mode_requires_mysql() ||
	    objects.size() != literals.size() || objects.size() > PLAYER_SNAPSHOT_MAX_ROWS ||
	    !recovery_object_templates_ready())
		return false;
	try
	{
		shop_trade_original_procedure_binding_stage candidate;
		std::map<int, size_t> by_number;
		for (size_t i = 0; i < objects.size(); ++i)
		{
			const P_obj object = objects[i];
			const auto &literal = literals[i];
			const auto *prototype = find_recovery_object_template(literal.vnum);
			if (!object || !prototype || object->R_num != prototype->R_num ||
			    object->obj_uid != literal.object_uid || object->type != literal.type ||
			    object->extra_flags != literal.extra_flags)
				return false;
			const int number = prototype->R_num;
			const auto found = std::lower_bound(recovery_object_templates.begin(),
							    recovery_object_templates.end(),
							    literal.vnum,
							    [](const auto &entry, int value)
							    { return entry.vnum < value; });
			if (found == recovery_object_templates.end() ||
			    found->vnum != literal.vnum || &found->prototype != prototype)
				return false;
			const auto position =
				static_cast<size_t>(found - recovery_object_templates.begin());
			auto [located, added] =
				by_number.emplace(number, candidate.bindings_.size());
			if (added)
				candidate.bindings_.push_back({ position, found->special,
								found->special, nullptr, false });
			auto &binding = candidate.bindings_[located->second];
			// Simulate normal per-instance order in the original forest: parsed
			// proclib first, then ITEM_SWITCH only while no proc is installed.
			if (IS_SET(object->extra_flags, ITEM_PROCLIB))
			{
				bool eligible = false;
				if (!proclib_saved_binding_eligible(object, &eligible) || !eligible)
					return false;
				if (binding.after != proclib_obj_cmd_bridge)
				{
					binding.predecessor = binding.after;
					binding.chain_needed = true;
					binding.after = proclib_obj_cmd_bridge;
				}
			}
			if (object->type == ITEM_SWITCH && !binding.after)
				binding.after = item_switch;
		}
		std::vector<proclib_recovery_chain_stage::request> requests;
		for (const auto &binding : candidate.bindings_)
			if (binding.chain_needed)
				requests.push_back(
					{ recovery_object_templates[binding.catalog_index]
						  .prototype.R_num,
					  binding.predecessor });
		if (!proclib_recovery_chain_stage::prepare(requests, candidate.chain_))
			return false;
		candidate.prepared_ = true;
		output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
bool shop_trade_original_procedure_binding_stage::prepare_native_birth(
	std::span<const quest_mobile_native_item_binding> originals,
	shop_trade_original_procedure_binding_stage &output) noexcept
{
	if (!nevent_is_game_thread() || !persistence_mode_requires_mysql() ||
	    originals.size() > PLAYER_SNAPSHOT_MAX_OBJECTS || !recovery_object_templates_ready())
		return false;
	try
	{
		shop_trade_original_procedure_binding_stage candidate;
		std::map<int, size_t> by_number;
		std::unordered_set<uint64_t> uids;
		for (const auto &original : originals)
		{
			const auto *prototype = find_recovery_object_template(original.vnum_);
			P_obj object = original.object_;
			if (!object || !prototype || !original.uid_ ||
			    !uids.insert(original.uid_).second ||
			    object->obj_uid != original.uid_ || object->R_num != original.rnum_ ||
			    prototype->R_num != original.rnum_ ||
			    obj_index[original.rnum_].pos != original.position_ ||
			    obj_index[original.rnum_].func.obj != original.before_ ||
			    (original.parsed_proclib_ &&
			     !IS_SET(object->extra_flags, ITEM_PROCLIB)))
				return false;
			auto found = std::lower_bound(recovery_object_templates.begin(),
						      recovery_object_templates.end(),
						      original.vnum_,
						      [](const auto &entry, int value)
						      { return entry.vnum < value; });
			if (found == recovery_object_templates.end() ||
			    found->vnum != original.vnum_ || &found->prototype != prototype ||
			    found->special != original.before_)
				return false;
			const size_t position =
				static_cast<size_t>(found - recovery_object_templates.begin());
			auto [located, added] =
				by_number.emplace(original.rnum_, candidate.bindings_.size());
			if (added)
				candidate.bindings_.push_back({ position, found->special,
								found->special, nullptr, false });
			auto &binding = candidate.bindings_[located->second];
			// Original actual parse succeeded; unknown/invalid raw descriptors remain
			// plain descriptions. No saved-parameter parser/eligibility reinterpretation.
			if ((original.parsed_proclib_ || original.restored_bridge_request_) &&
			    binding.after != proclib_obj_cmd_bridge)
			{
				binding.predecessor = binding.after;
				binding.chain_needed = true;
				binding.after = proclib_obj_cmd_bridge;
			}
			if (object->type == ITEM_SWITCH && !binding.after)
				binding.after = item_switch;
		}
		std::vector<proclib_recovery_chain_stage::request> requests;
		for (const auto &binding : candidate.bindings_)
			if (binding.chain_needed)
				requests.push_back(
					{ recovery_object_templates[binding.catalog_index]
						  .prototype.R_num,
					  binding.predecessor });
		if (!proclib_recovery_chain_stage::prepare(requests, candidate.chain_))
			return false;
		candidate.prepared_ = true;
		output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}
size_t shop_trade_original_procedure_binding_stage::retained_bytes() const noexcept
{
	const size_t chain = chain_.retained_bytes();
	if (!chain || chain < sizeof(chain_) ||
	    bindings_.capacity() > (SIZE_MAX - sizeof(*this)) / sizeof(binding))
		return 0;
	const size_t bytes = sizeof(*this) + bindings_.capacity() * sizeof(binding);
	return chain - sizeof(chain_) > SIZE_MAX - bytes ? 0 : bytes + chain - sizeof(chain_);
}
bool shop_trade_original_procedure_binding_stage::valid() const noexcept
{
	if (!prepared_ || !nevent_is_game_thread() || !persistence_mode_requires_mysql() ||
	    !recovery_object_templates_ready() || !chain_.valid())
		return false;
	for (const auto &binding : bindings_)
	{
		if (binding.catalog_index >= recovery_object_templates.size())
			return false;
		const auto &entry = recovery_object_templates[binding.catalog_index];
		const int number = entry.prototype.R_num;
		if (number < 0 || number > top_of_objt ||
		    obj_index[number].virtual_number != entry.vnum ||
		    obj_index[number].pos != entry.position || entry.special != binding.before ||
		    obj_index[number].func.obj != binding.before)
			return false;
	}
	return true;
}
void shop_trade_original_procedure_binding_stage::commit_unchecked() noexcept
{
	chain_.commit_unchecked();
	for (const auto &binding : bindings_)
	{
		auto &entry = recovery_object_templates[binding.catalog_index];
		obj_index[entry.prototype.R_num].func.obj = binding.after;
		entry.special = binding.after;
	}
	prepared_ = false;
}
void shop_trade_original_procedure_binding_stage::observe_normal_binding(
	int number, obj_proc_type before, obj_proc_type after) noexcept
{
	// Private ordinary-constructor notification only. Never repair an arbitrary
	// drift or reseal a runtime catalog; native behavior already ran unchanged.
	if (!nevent_is_game_thread() || !persistence_mode_requires_mysql() ||
	    !recovery_object_templates_ready() || number < 0 || number > top_of_objt ||
	    before == after || obj_index[number].func.obj != after ||
	    (after != proclib_obj_cmd_bridge && !(after == item_switch && !before)) ||
	    (after == proclib_obj_cmd_bridge &&
	     !proclib_recovery_chain_stage::predecessor_matches(number, before)))
		return;
	const int vnum = obj_index[number].virtual_number;
	auto found = std::lower_bound(recovery_object_templates.begin(),
				      recovery_object_templates.end(), vnum,
				      [](const auto &entry, int value)
				      { return entry.vnum < value; });
	if (found == recovery_object_templates.end() || found->vnum != vnum ||
	    found->prototype.R_num != number || found->position != obj_index[number].pos ||
	    found->special != before)
		return;
	found->special = after;
}

bool cache_object_template(int vnum)
{
	if (starter_object_templates.count(vnum))
		return true;
	const int nr = real_object(vnum);
	if (nr < 0)
		return false;
	starter_object_templates.emplace(vnum, parse_object_template(nr));
	return true;
}

const object_template *find_object_template(int vnum)
{
	const auto found = starter_object_templates.find(vnum);
	return found == starter_object_templates.end() ? nullptr : &found->second;
}

P_obj instantiate_object_template(const object_template &prototype)
{
	// Only this main-thread adapter touches the pool, index, list or events.
	const int nr = prototype.R_num;
	P_obj obj = (P_obj)mm_get(dead_obj_pool);
	memset(obj, 0, sizeof(*obj));
	obj->R_num = prototype.R_num;
	obj->type = prototype.type;
	obj->material = prototype.material;
	obj->craftsmanship = prototype.craftsmanship;
	obj->extra_flags = prototype.extra_flags;
	obj->wear_flags = prototype.wear_flags;
	obj->extra2_flags = prototype.extra2_flags;
	obj->anti_flags = prototype.anti_flags;
	obj->anti2_flags = prototype.anti2_flags;
	memcpy(&obj->value, &prototype.value, sizeof(obj->value));
	obj->weight = prototype.weight;
	obj->cost = prototype.cost;
	obj->condition = prototype.condition;
	obj->bitvector = prototype.bitvector;
	obj->bitvector2 = prototype.bitvector2;
	obj->bitvector3 = prototype.bitvector3;
	obj->bitvector4 = prototype.bitvector4;
	obj->bitvector5 = prototype.bitvector5;
	memcpy(&obj->affected, &prototype.affected, sizeof(obj->affected));
	obj->trap_eff = prototype.trap_eff;
	obj->trap_dam = prototype.trap_dam;
	obj->trap_charge = prototype.trap_charge;
	obj->trap_level = prototype.trap_level;
	obj->obj_uid = static_cast<unsigned long>(persistence_next_item_uid());
	SET_BIT(obj->runtime_flags, OBJ_RFLAG_CREATION_CANDIDATE);
	obj->loc_p = LOC_NOWHERE;
	obj->loc.room = NOWHERE;
	obj_index[nr].number++;
	if (object_list)
		object_list->prev = obj;
	obj->next = object_list;
	object_list = obj;
	if (!obj_index[nr].keys)
		obj_index[nr].keys = prototype.name.empty() ? nullptr :
							      str_dup(prototype.name.c_str());
	obj->name = obj_index[nr].keys;
	if (!obj_index[nr].desc2)
		obj_index[nr].desc2 = prototype.short_description.empty() ?
					      nullptr :
					      str_dup(prototype.short_description.c_str());
	obj->short_description = obj_index[nr].desc2;
	if (!obj_index[nr].desc1)
		obj_index[nr].desc1 = prototype.description.empty() ?
					      nullptr :
					      str_dup(prototype.description.c_str());
	obj->description = obj_index[nr].desc1;
	if (!obj_index[nr].desc3)
		obj_index[nr].desc3 = prototype.action_description.empty() ?
					      nullptr :
					      str_dup(prototype.action_description.c_str());
	obj->action_description = obj_index[nr].desc3;
	for (const auto &description : prototype.descriptions)
	{
		extra_descr_data *new_descr;
		CREATE(new_descr, extra_descr_data, 1, MEM_TAG_EXDESCD);
		new_descr->keyword = description.keyword.empty() ?
					     nullptr :
					     str_dup(description.keyword.c_str());
		new_descr->description = description.description.empty() ?
						 nullptr :
						 str_dup(description.description.c_str());
		// Preserve nullable text while allowing procedures that take no arguments.
		char empty_args[] = "";
		if (new_descr->keyword && !strn_cmp("_proclib_", new_descr->keyword, 9) &&
		    !proclibObj_add(obj, new_descr->keyword + 9,
				    new_descr->description ? new_descr->description : empty_args))
		{
			FREE(new_descr->keyword);
			if (new_descr->description)
				FREE(new_descr->description);
			FREE(new_descr);
			continue;
		}
		new_descr->next = obj->ex_description;
		obj->ex_description = new_descr;
	}
	if (obj->type == ITEM_SWITCH && !obj_index[nr].func.obj)
	{
		obj_index[nr].func.obj = item_switch;
		shop_trade_original_procedure_binding_stage::observe_normal_binding(nr, nullptr,
										    item_switch);
	}
	obj->nevents = NULL;
	obj->nevents_tail = NULL;

	if (obj_index[nr].func.obj)
	{
		if (invoke_object_special(obj, 0, CMD_SET_PERIODIC, 0))
			add_event(event_object_proc, PULSE_MOBILE + number(-4, 4), 0, 0, obj, 0, 0,
				  0);
	}

	if (isname("random_exit", obj->name))
		add_event(event_random_exit, 3, 0, 0, obj, 0, 0, 0);

	/* This is no longer needed as poofing artis is handled in the DB via DB-located timers.
	if (IS_ARTIFACT(obj))
	{
	  add_event(event_artifact_poof, 2 * WAIT_SEC, 0, 0, obj, 0, 0, 0);
	  // Set current timer?  Not necessary as event_artifact_poof will fix it?
	  //   Not really sure atm.. will test this and see what happens...
	  obj->timer[3] = time(NULL);
	}
	*/

	/* The master spellbook carries no scribed spells in the object file -- the
	 * spell list is a raw bitmap -- so it is filled here on every load. */
	if (obj->type == ITEM_SPELLBOOK && obj_index[nr].virtual_number == MASTER_SPELLBOOK_VNUM)
		FillMasterSpellBook(obj);

	convertObj(obj);

	return (obj);
}

namespace
{
// Same actual pointer/UID membership check as the original private handler
// helper. It confers no custody or publication authority on a caller.
P_obj find_birth_live_object(P_obj expected, uint64_t uid) noexcept
{
	for (P_obj object = object_list; object; object = object->next)
		if (object == expected && object->obj_uid == uid)
			return object;
	return nullptr;
}
}

struct quest_mobile_native_item_stage::implementation
{
	P_obj object = nullptr;
	P_index index = nullptr;
	mm_ds *pool = nullptr;
	int rnum = -1, vnum = 0;
	long position = 0;
	uint64_t uid = 0;
	obj_proc_type original_proc = nullptr;
	obj_proc_type effective_proc = nullptr;
	std::array<char *, 4> shared{};
	std::vector<size_t> libraries;
	std::vector<extra_descr_data *> parsed_descriptions;
	mm_ds *affect_pool = nullptr;
	std::vector<bool> requested;
	std::vector<int> library_delays;
	int general_delay = 0;
	bool library_event_requested = false;
	extra_descr_data *allocated_spell_description = nullptr;
	quest_mobile_native_zombie_stage zombie;
	bool admitted = false, published = false, general_initialized = false;
	bool general_periodic = false, parsed_proclib = false;
	size_t next_step = 0;
	bool current_step_started = false;
	bool random_exit_requested = false;
	bool metadata_borrowed_world = false;
	bool restored_bridge_request = false;
	bool rebuilding_enrollment = false, enrollment_rebuilt = false;
	uint32_t rebuilding_prefix = 0;
	P_obj rebuilding_object = nullptr;
	std::array<bool, 3> rebuilding_events{};
	bool rebuilding_zombie = false;
	// One original shell probe may be outstanding on this actual staged body.
	// Successful cleanup clears it; nonreturning construction/cleanup stays owned.
	bool shell_probe_started = false;
	P_obj shell_probe_object = nullptr;
	uint64_t shell_probe_uid = 0;
};
P_obj quest_mobile_native_item_stage::object() const noexcept
{
	return state_ ? state_->object : nullptr;
}
quest_mobile_native_item_binding quest_mobile_native_item_stage::binding_input() const noexcept
{
	if (!state_)
		return quest_mobile_native_item_binding(nullptr, 0, -1, 0, 0, nullptr, false);
	quest_mobile_native_item_binding result(state_->object, state_->uid, state_->rnum,
						state_->vnum, state_->position,
						state_->original_proc, state_->parsed_proclib);
	result.restored_bridge_request_ = state_->restored_bridge_request;
	return result;
}
size_t quest_mobile_native_item_stage::retained_bytes() const noexcept
{
	if (!state_ || !nevent_is_game_thread())
		return 0;
	const auto &s = *state_;
	size_t bytes = sizeof(*this) + sizeof(implementation);
	const auto add = [&bytes](size_t amount)
	{
		if (amount > SIZE_MAX - bytes)
			return false;
		bytes += amount;
		return true;
	};
	const auto multiply = [&add](size_t count, size_t size)
	{ return (!size || count <= SIZE_MAX / size) && add(count * size); };
	if (!multiply(s.parsed_descriptions.capacity(), sizeof(extra_descr_data *)) ||
	    !multiply(s.libraries.capacity(), sizeof(size_t)) ||
	    !multiply(s.library_delays.capacity(), sizeof(int)) ||
	    !add(s.requested.capacity() / CHAR_BIT + (s.requested.capacity() % CHAR_BIT != 0)))
		return 0;
	if (s.zombie.game_ && (!add(sizeof(ZombieGame) + sizeof(ZombieGame *)) ||
			       !multiply(s.zombie.game_->zombies.capacity(), sizeof(P_char))))
		return 0;
	if (!s.object)
		return bytes; // Published native memory now belongs to the actual world owner.
	for (const auto *affect = s.object->affects; affect; affect = affect->next)
		if (!add(sizeof(obj_affect)))
			return 0;
	if (!add(sizeof(obj_data)))
		return 0;
	const std::array<char *, 4> strings = { s.object->name, s.object->short_description,
						s.object->description,
						s.object->action_description };
	for (size_t i = 0; i < strings.size(); ++i)
		if (strings[i] && strings[i] != s.shared[i] && !add(strlen(strings[i]) + 1))
			return 0;
	for (const auto *description = s.object->ex_description; description;
	     description = description->next)
	{
		if (!add(sizeof(extra_descr_data)) ||
		    (description->keyword && !add(strlen(description->keyword) + 1)))
			return 0;
		if (description->description && !add(description == s.allocated_spell_description ?
							     (MAX_SKILLS + 1) / 8 + 1 :
							     strlen(description->description) + 1))
			return 0;
	}
	return bytes;
}
void quest_mobile_native_item_stage::retain_admitted() noexcept
{
	if (state_ && state_->object && nevent_is_game_thread())
		state_->admitted = true;
}
bool quest_mobile_native_item_stage::discard_unadmitted() noexcept
{
	if (!state_)
		return true;
	auto &s = *state_;
	P_obj obj = s.object;
	if (!nevent_is_game_thread() || s.admitted || s.published || s.shell_probe_started ||
	    !obj || obj->loc_p != LOC_NOWHERE || obj->loc.room != NOWHERE || obj->contains ||
	    obj->next_content || obj->next || obj->prev || obj->nevents || obj->nevents_tail ||
	    find_birth_live_object(obj, s.uid) || !s.zombie.discard())
		return false;
	if (obj->affects && !s.affect_pool)
		return false;
	for (auto *affect = obj->affects; affect;)
	{
		auto *next = affect->next;
		mm_release(s.affect_pool, affect);
		affect = next;
	}
	obj->affects = nullptr;
	const std::array<char *, 4> strings = { obj->name, obj->short_description, obj->description,
						obj->action_description };
	for (size_t i = 0; i < strings.size(); ++i)
		if (strings[i] && strings[i] != s.shared[i])
			str_free(strings[i]);
	for (auto *description = obj->ex_description; description;)
	{
		auto *next = description->next;
		if (description->keyword)
			str_free(description->keyword);
		if (description->description)
			str_free(description->description);
		FREE(description);
		description = next;
	}
	// Never free_obj/extract_obj: unpublished instances have no index/list/events,
	// barb removal, Redis, artifact or other gameplay/extraction side effects.
	mm_release(s.pool, obj);
	delete state_;
	state_ = nullptr;
	return true;
}
bool quest_mobile_native_item_stage::capture_container_shell(
	quest_mobile_native_container_shell *output) noexcept
{
	if (!state_ || !output || output->valid_ || !nevent_is_game_thread())
		return false;
	auto &s = *state_;
	if (s.admitted || s.published || s.metadata_borrowed_world || s.shell_probe_started ||
	    !s.object || s.object->type != ITEM_CONTAINER || s.index != obj_index || s.rnum < 0 ||
	    s.rnum > top_of_objt || s.object->R_num != s.rnum || s.object->obj_uid != s.uid ||
	    find_birth_live_object(s.object, s.uid))
		return false;
	// This is the real original read/weight/extract path, not a template weight
	// or a detached substitute. It retains original UID, RNG and callback work.
	// The birth owner calls it at the original P probe cut; these values alone
	// confer no admission, source, SQL, publication or recovery permission.
	s.shell_probe_started = true;
	try
	{
		P_obj probe = read_object(s.rnum, REAL);
		s.shell_probe_object = probe;
		int32_t weight = 0;
		if (probe)
		{
			s.shell_probe_uid = probe->obj_uid;
			weight = probe->weight;
			extract_obj(probe, TRUE);
			if (find_birth_live_object(probe, s.shell_probe_uid))
				return false;
		}
		// Original obj_prototype_weight returns zero on a null constructor.
		// That returned outcome is not an uncertain or synthetic success.
		s.shell_probe_object = nullptr;
		s.shell_probe_uid = 0;
		s.shell_probe_started = false;
		output->target_ = s.object;
		output->target_rnum_ = s.rnum;
		output->shell_weight_ = weight;
		output->valid_ = true;
		return true;
	}
	catch (...)
	{
		// Retain the real outstanding attempt; never repeat it or dispose of
		// its owner because a probe or cleanup did not return a known result.
		return false;
	}
}
bool quest_mobile_native_item_stage::prepare(int nr, int type, uint64_t supplied_reserved_uid,
					     quest_mobile_native_item_stage *output) noexcept
{
	if (!nevent_is_game_thread() || !output || output->state_ || !supplied_reserved_uid ||
	    supplied_reserved_uid == UINT64_MAX || supplied_reserved_uid > ULONG_MAX ||
	    !obj_index || !dead_obj_pool || dead_obj_pool->size != sizeof(obj_data) ||
	    dead_obj_pool->next_off != offsetof(obj_data, next))
		return false;
	if (type == VIRTUAL)
		nr = real_object(nr);
	else if (type != REAL)
		return false;
	if (nr < 0 || nr > top_of_objt)
		return false;
	quest_mobile_native_item_stage candidate;
	try
	{
		const object_template prototype = parse_object_template(nr);
		if (prototype.R_num != nr ||
		    prototype.descriptions.size() > PLAYER_SNAPSHOT_MAX_ROWS)
			return false;
		auto state = std::make_unique<implementation>();
		state->index = obj_index;
		state->pool = dead_obj_pool;
		state->rnum = nr;
		state->vnum = obj_index[nr].virtual_number;
		state->position = obj_index[nr].pos;
		state->original_proc = obj_index[nr].func.obj;
		state->uid = supplied_reserved_uid;
		const auto original_proc = state->original_proc;
		const auto effective = [original_proc, nr](obj_proc_type function)
		{
			return original_proc == function ||
			       (original_proc == proclib_obj_cmd_bridge &&
				proclib_recovery_chain_stage::predecessor_matches(nr, function));
		};
		// REPOP can mutate literals and the world. It has no detached original
		// trigger/source participant yet; refuse before any birth or publication.
		if (effective(studioproc_obj))
			return false;
		// Only these actual original CMD_SET_PERIODIC branches are detached-safe.
		// An unrecognized incumbent/predecessor is not a predicted probe result.
		if (!effective(nullptr) && !effective(spell_pool) && !effective(super_cannon) &&
		    !effective(vecna_deathportal) && !effective(blood_stains) &&
		    !effective(zombies_game) && !effective(item_switch) &&
		    !effective(proclib_obj_proc))
			return false;
		for (const auto function : { static_cast<obj_proc_type>(nullptr), spell_pool,
					     super_cannon, vecna_deathportal, blood_stains,
					     zombies_game, item_switch, proclib_obj_proc })
			if (effective(function))
			{
				state->effective_proc = function;
				break;
			}
		state->libraries.reserve(prototype.descriptions.size());
		state->parsed_descriptions.reserve(prototype.descriptions.size());
		state->requested.reserve(prototype.descriptions.size());
		state->library_delays.reserve(prototype.descriptions.size());
		P_obj obj = static_cast<P_obj>(mm_get(dead_obj_pool));
		memset(obj, 0, sizeof(*obj));
		state->object = obj;
		candidate.state_ = state.release();
		auto &s = *candidate.state_;
		obj->R_num = prototype.R_num;
		obj->type = prototype.type;
		obj->material = prototype.material;
		obj->craftsmanship = prototype.craftsmanship;
		obj->extra_flags = prototype.extra_flags;
		obj->wear_flags = prototype.wear_flags;
		obj->extra2_flags = prototype.extra2_flags;
		obj->anti_flags = prototype.anti_flags;
		obj->anti2_flags = prototype.anti2_flags;
		memcpy(&obj->value, &prototype.value, sizeof(obj->value));
		obj->weight = prototype.weight;
		obj->cost = prototype.cost;
		obj->condition = prototype.condition;
		obj->bitvector = prototype.bitvector;
		obj->bitvector2 = prototype.bitvector2;
		obj->bitvector3 = prototype.bitvector3;
		obj->bitvector4 = prototype.bitvector4;
		obj->bitvector5 = prototype.bitvector5;
		memcpy(&obj->affected, &prototype.affected, sizeof(obj->affected));
		obj->trap_eff = prototype.trap_eff;
		obj->trap_dam = prototype.trap_dam;
		obj->trap_charge = prototype.trap_charge;
		obj->trap_level = prototype.trap_level;
		obj->obj_uid = static_cast<unsigned long>(supplied_reserved_uid);
		SET_BIT(obj->runtime_flags, OBJ_RFLAG_CREATION_CANDIDATE);
		obj->loc_p = LOC_NOWHERE;
		obj->loc.room = NOWHERE;
		// Same immutable prototype string sharing; no index count/list enrollment.
		if (!obj_index[nr].keys)
			obj_index[nr].keys =
				prototype.name.empty() ? nullptr : str_dup(prototype.name.c_str());
		obj->name = obj_index[nr].keys;
		s.shared[0] = obj->name;
		if (!obj_index[nr].desc2)
			obj_index[nr].desc2 = prototype.short_description.empty() ?
						      nullptr :
						      str_dup(prototype.short_description.c_str());
		obj->short_description = obj_index[nr].desc2;
		s.shared[1] = obj->short_description;
		if (!obj_index[nr].desc1)
			obj_index[nr].desc1 = prototype.description.empty() ?
						      nullptr :
						      str_dup(prototype.description.c_str());
		obj->description = obj_index[nr].desc1;
		s.shared[2] = obj->description;
		if (!obj_index[nr].desc3)
			obj_index[nr].desc3 = prototype.action_description.empty() ?
						      nullptr :
						      str_dup(prototype.action_description.c_str());
		obj->action_description = obj_index[nr].desc3;
		s.shared[3] = obj->action_description;
		for (const auto &description : prototype.descriptions)
		{
			extra_descr_data *new_descr;
			CREATE(new_descr, extra_descr_data, 1, MEM_TAG_EXDESCD);
			new_descr->keyword = description.keyword.empty() ?
						     nullptr :
						     str_dup(description.keyword.c_str());
			new_descr->description = description.description.empty() ?
							 nullptr :
							 str_dup(description.description.c_str());
			char empty_args[] = "";
			size_t library = 0;
			if (new_descr->keyword && !strn_cmp("_proclib_", new_descr->keyword, 9) &&
			    !quest_mobile_native_original_proclib::prepare(
				    obj, new_descr->keyword + 9,
				    new_descr->description ? new_descr->description : empty_args,
				    &library))
			{
				FREE(new_descr->keyword);
				if (new_descr->description)
					FREE(new_descr->description);
				FREE(new_descr);
				s.libraries.push_back(library);
				s.parsed_descriptions.push_back(obj->ex_description);
				// Original proclibObj_add probes only while its periodic event is absent.
				// The first requested event owns its original jitter; later adds skip
				// the probe just as after a successful original schedule. Enrollment
				// remains a checked publication step; scheduler refusal never rerolls.
				bool periodic = false;
				if (!s.library_event_requested &&
				    !quest_mobile_native_original_proclib::probe(obj, library,
										 &periodic))
				{
					candidate.discard_unadmitted();
					return false;
				}
				s.requested.push_back(periodic);
				s.library_delays.push_back(periodic ? PULSE_MOBILE + number(-4, 4) :
								      0);
				s.library_event_requested = s.library_event_requested || periodic;
				s.parsed_proclib = true;
				continue;
			}
			new_descr->next = obj->ex_description;
			obj->ex_description = new_descr;
		}
		// Actual local initializer decisions precede spellbook filling/conversion.
		// Library parsing is complete, and a bridge preserves the original incumbent.
		if (effective(spell_pool))
		{
			s.general_periodic = native_birth_spell_pool_initialize(obj, number(0, 8),
										time(nullptr));
			s.general_initialized = true;
		}
		else if (effective(super_cannon))
		{
			s.general_periodic = native_birth_super_cannon_initialize(obj);
			s.general_initialized = true;
		}
		else if (effective(vecna_deathportal))
		{
			s.general_periodic = native_birth_vecna_deathportal_initialize(obj);
			s.general_initialized = true;
		}
		else if (effective(blood_stains))
		{
			s.general_periodic = blood_stains(obj, nullptr, CMD_SET_PERIODIC, nullptr);
			s.general_initialized = true;
		}
		else if (effective(zombies_game))
		{
			if (!quest_mobile_native_zombie_stage::prepare(obj, s.zombie))
			{
				candidate.discard_unadmitted();
				return false;
			}
			s.general_periodic = true;
			s.general_initialized = true;
		}
		// The remaining known dispatches have no literal/global initialization.
		// Newly installed bridge with no incumbent ignores CMD_SET_PERIODIC;
		// ITEM_SWITCH fallback is installed only if no successful library bound it.
		else if (effective(item_switch) ||
			 (effective(nullptr) && !s.parsed_proclib && obj->type == ITEM_SWITCH))
		{
			s.general_periodic = item_switch(obj, nullptr, CMD_SET_PERIODIC, nullptr);
			s.general_initialized = true;
		}
		else if (effective(proclib_obj_proc))
		{
			s.general_periodic =
				proclib_obj_proc(obj, nullptr, CMD_SET_PERIODIC, nullptr);
			s.general_initialized = true;
		}
		else
			s.general_initialized = true;
		if (s.general_periodic)
			s.general_delay = PULSE_MOBILE + number(-4, 4);
		s.random_exit_requested = isname("random_exit", obj->name);
		if (obj->type == ITEM_SPELLBOOK &&
		    obj_index[nr].virtual_number == MASTER_SPELLBOOK_VNUM)
		{
			const bool had_spell_description = find_spell_description(obj) != nullptr;
			FillMasterSpellBook(obj);
			if (!had_spell_description)
				s.allocated_spell_description = find_spell_description(obj);
		}
		convertObj(obj);
		if (obj_index != s.index || obj_index[nr].virtual_number != s.vnum ||
		    obj_index[nr].pos != s.position || obj_index[nr].func.obj != s.original_proc)
		{
			candidate.discard_unadmitted();
			return false;
		}
		output->state_ = candidate.state_;
		candidate.state_ = nullptr;
		return true;
	}
	catch (...)
	{
		candidate.discard_unadmitted();
		return false;
	}
}

namespace
{
bool original_birth_procedure_tag(obj_proc_type function,
				  native_mobile_birth_procedure *output) noexcept
{
	if (!output)
		return false;
	native_mobile_birth_procedure tag;
	if (!function)
		tag = native_mobile_birth_procedure::none;
	else if (function == spell_pool)
		tag = native_mobile_birth_procedure::spell_pool;
	else if (function == super_cannon)
		tag = native_mobile_birth_procedure::super_cannon;
	else if (function == vecna_deathportal)
		tag = native_mobile_birth_procedure::vecna_deathportal;
	else if (function == blood_stains)
		tag = native_mobile_birth_procedure::blood_stains;
	else if (function == zombies_game)
		tag = native_mobile_birth_procedure::zombies_game;
	else if (function == item_switch)
		tag = native_mobile_birth_procedure::item_switch;
	else if (function == proclib_obj_proc)
		tag = native_mobile_birth_procedure::proclib_obj_proc;
	else
		return false;
	*output = tag;
	return true;
}
bool original_birth_procedure(native_mobile_birth_procedure tag, obj_proc_type *output) noexcept
{
	if (!output)
		return false;
	obj_proc_type function;
	switch (tag)
	{
	case native_mobile_birth_procedure::none:
		function = nullptr;
		break;
	case native_mobile_birth_procedure::spell_pool:
		function = spell_pool;
		break;
	case native_mobile_birth_procedure::super_cannon:
		function = super_cannon;
		break;
	case native_mobile_birth_procedure::vecna_deathportal:
		function = vecna_deathportal;
		break;
	case native_mobile_birth_procedure::blood_stains:
		function = blood_stains;
		break;
	case native_mobile_birth_procedure::zombies_game:
		function = zombies_game;
		break;
	case native_mobile_birth_procedure::item_switch:
		function = item_switch;
		break;
	case native_mobile_birth_procedure::proclib_obj_proc:
		function = proclib_obj_proc;
		break;
	default:
		return false;
	}
	*output = function;
	return true;
}
bool original_birth_literal_matches(P_obj object, const player_item_snapshot &literal)
{
	std::vector<player_item_snapshot> actual;
	if (player_item_snapshot_tree_capture_literal(object, &actual, nullptr) !=
		    player_snapshot_capture_result::ok ||
	    actual.empty())
		return false;
	// Topology belongs to the complete original forest owner. This stage proves
	// the entire local persisted row, without changing original parent/equipment.
	player_item_snapshot expected = literal;
	expected.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	expected.equipment_slot = 0;
	actual.resize(1);
	std::vector<uint8_t> a, b;
	return player_item_snapshot_list_encode(actual, &a) == player_snapshot_codec_result::ok &&
	       player_item_snapshot_list_encode({ expected }, &b) ==
		       player_snapshot_codec_result::ok &&
	       a == b;
}
}

bool quest_mobile_native_item_stage::capture_recipe(
	const player_item_snapshot &literal, native_mobile_birth_item_recipe *output) const noexcept
{
	if (!state_ || !output || !nevent_is_game_thread())
		return false;
	const auto &s = *state_;
	if (!s.object || s.admitted || s.published || !s.general_initialized ||
	    s.current_step_started || s.next_step || literal.object_uid != s.uid ||
	    literal.vnum != s.vnum || s.libraries.size() != s.requested.size() ||
	    s.libraries.size() != s.library_delays.size() ||
	    s.libraries.size() != s.parsed_descriptions.size() ||
	    !std::in_range<int16_t>(s.object->trap_eff) ||
	    !std::in_range<int16_t>(s.object->trap_dam) ||
	    !std::in_range<int16_t>(s.object->trap_charge) ||
	    !std::in_range<int16_t>(s.object->trap_level))
		return false;
	try
	{
		if (!original_birth_literal_matches(s.object, literal))
			return false;
		native_mobile_birth_item_recipe candidate;
		candidate.object_uid = s.uid;
		candidate.binding_form = s.original_proc == proclib_obj_cmd_bridge ?
						 native_mobile_birth_binding_form::bridge :
						 native_mobile_birth_binding_form::direct;
		if (!original_birth_procedure_tag(s.effective_proc, &candidate.procedure))
			return false;
		candidate.general_periodic = s.general_periodic;
		candidate.general_delay = s.general_delay;
		candidate.random_exit_requested = s.random_exit_requested;
		candidate.trap_eff = static_cast<int16_t>(s.object->trap_eff);
		candidate.trap_dam = static_cast<int16_t>(s.object->trap_dam);
		candidate.trap_charge = static_cast<int16_t>(s.object->trap_charge);
		candidate.trap_level = static_cast<int16_t>(s.object->trap_level);
		candidate.libraries.reserve(s.libraries.size());
		for (size_t i = 0; i < s.libraries.size(); ++i)
		{
			native_mobile_birth_library_recipe library;
			if (!quest_mobile_native_original_proclib::retained_library(
				    s.libraries[i], &library.library))
				return false;
			uint32_t index = 0;
			const auto *description = s.object->ex_description;
			while (description && description != s.parsed_descriptions[i])
			{
				if (index >= literal.extra_descriptions.size())
					return false;
				description = description->next;
				++index;
			}
			if (!description || index >= literal.extra_descriptions.size())
				return false;
			library.extra_description_index = index;
			library.periodic_requested = s.requested[i];
			library.delay = s.library_delays[i];
			candidate.libraries.push_back(library);
		}
		if (!native_mobile_birth_recipe_valid({ &literal, 1 }, { &candidate, 1 }))
			return false;
		*output = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

// Original sealed definition plus its actually committed binding, not a
// generic caller assertion or replacement of a saved function pointer.
const object_template *quest_mobile_native_item_stage::find_bound_recovery_template(
	const player_item_snapshot &literal, const native_mobile_birth_item_recipe &recipe) noexcept
{
	if (!recovery_object_templates_ready())
		return nullptr;
	const auto found = std::lower_bound(recovery_object_templates.begin(),
					    recovery_object_templates.end(), literal.vnum,
					    [](const auto &entry, int vnum)
					    { return entry.vnum < vnum; });
	if (found == recovery_object_templates.end() || found->vnum != literal.vnum)
		return nullptr;
	const int nr = found->prototype.R_num;
	obj_proc_type effective;
	if (nr < 0 || nr > top_of_objt || !original_birth_procedure(recipe.procedure, &effective) ||
	    obj_index[nr].virtual_number != literal.vnum || obj_index[nr].pos != found->position)
		return nullptr;
	const auto expected_before = recipe.binding_form ==
						     native_mobile_birth_binding_form::bridge ?
					     proclib_obj_cmd_bridge :
					     effective;
	const auto current = obj_index[nr].func.obj;
	// The authentic original binding batch also snapshots its committed result.
	if (found->special != expected_before && found->special != current)
		return nullptr;
	if (recipe.binding_form == native_mobile_birth_binding_form::bridge ||
	    !recipe.libraries.empty())
	{
		if (current != proclib_obj_cmd_bridge ||
		    !proclib_recovery_chain_stage::predecessor_matches(nr, effective))
			return nullptr;
	}
	else if (literal.type == ITEM_SWITCH && !effective)
	{
		if (current != item_switch)
			return nullptr;
	}
	else if (current != effective)
		return nullptr;
	return &found->prototype;
}

bool quest_mobile_native_item_stage::restore(const player_item_snapshot &literal,
					     const native_mobile_birth_item_recipe &recipe,
					     quest_mobile_native_item_stage *output) noexcept
{
	if (!output || output->state_ || !nevent_is_game_thread() ||
	    !native_mobile_birth_recipe_valid({ &literal, 1 }, { &recipe, 1 }) || !obj_index)
		return false;
	quest_mobile_native_item_stage candidate;
	try
	{
		const auto *prototype = find_recovery_object_template(literal.vnum);
		if (!prototype || prototype->R_num < 0 || prototype->R_num > top_of_objt ||
		    obj_index[prototype->R_num].virtual_number != literal.vnum)
			return false;
		const int nr = prototype->R_num;
		obj_proc_type effective;
		if (!original_birth_procedure(recipe.procedure, &effective))
			return false;
		const auto original = obj_index[nr].func.obj;
		if (recipe.binding_form == native_mobile_birth_binding_form::bridge)
		{
			if (original != proclib_obj_cmd_bridge ||
			    !proclib_recovery_chain_stage::predecessor_matches(nr, effective))
				return false;
		}
		else if (original != effective)
			return false;
		auto state = std::make_unique<implementation>();
		state->index = obj_index;
		state->rnum = nr;
		state->vnum = literal.vnum;
		state->position = obj_index[nr].pos;
		state->uid = literal.object_uid;
		state->original_proc = original;
		state->effective_proc = effective;
		state->general_periodic = recipe.general_periodic;
		state->general_delay = recipe.general_delay;
		state->random_exit_requested = recipe.random_exit_requested;
		state->general_initialized = true;
		state->libraries.reserve(recipe.libraries.size());
		state->parsed_descriptions.reserve(recipe.libraries.size());
		state->requested.reserve(recipe.libraries.size());
		state->library_delays.reserve(recipe.libraries.size());
		native_mobile_birth_literal_stage literal_stage;
		if (!native_mobile_birth_literal_stage::prepare(*prototype, literal, literal_stage))
			return false;
		state->object = std::exchange(literal_stage.object_, nullptr);
		state->pool = std::exchange(literal_stage.pool_, nullptr);
		state->affect_pool = std::exchange(literal_stage.affect_pool_, nullptr);
		candidate.state_ = state.release();
		auto &s = *candidate.state_;
		P_obj object = s.object;
		object->trap_eff = recipe.trap_eff;
		object->trap_dam = recipe.trap_dam;
		object->trap_charge = recipe.trap_charge;
		object->trap_level = recipe.trap_level;
		SET_BIT(object->runtime_flags, OBJ_RFLAG_CREATION_CANDIDATE);
		size_t description_index = 0;
		for (auto *description = object->ex_description; description;
		     description = description->next)
		{
			if (description_index >= literal.extra_descriptions.size())
			{
				candidate.discard_unadmitted();
				return false;
			}
			if (literal.extra_descriptions[description_index].spellbook)
				s.allocated_spell_description = description;
			++description_index;
		}
		if (description_index != literal.extra_descriptions.size())
		{
			candidate.discard_unadmitted();
			return false;
		}
		for (const auto &saved : recipe.libraries)
		{
			size_t index;
			if (!quest_mobile_native_original_proclib::retained_index(saved.library,
										  &index))
			{
				candidate.discard_unadmitted();
				return false;
			}
			auto *description = object->ex_description;
			for (uint32_t i = 0; description && i < saved.extra_description_index; ++i)
				description = description->next;
			if (!description)
			{
				candidate.discard_unadmitted();
				return false;
			}
			s.libraries.push_back(index);
			s.parsed_descriptions.push_back(description);
			s.requested.push_back(saved.periodic_requested);
			s.library_delays.push_back(saved.delay);
			s.library_event_requested = s.library_event_requested ||
						    saved.periodic_requested;
		}
		s.parsed_proclib = !s.libraries.empty();
		if (!original_birth_literal_matches(object, literal) || obj_index != s.index ||
		    obj_index[nr].virtual_number != s.vnum || obj_index[nr].pos != s.position ||
		    obj_index[nr].func.obj != s.original_proc)
		{
			candidate.discard_unadmitted();
			return false;
		}
		if (effective == zombies_game &&
		    !quest_mobile_native_zombie_stage::restore(object, s.zombie))
		{
			candidate.discard_unadmitted();
			return false;
		}
		// No allocating/callback work follows the last original retained owner.
		output->state_ = candidate.state_;
		candidate.state_ = nullptr;
		return true;
	}
	catch (...)
	{
		candidate.discard_unadmitted();
		return false;
	}
}

bool quest_mobile_native_item_stage::restore_bound(const player_item_snapshot &literal,
						   const native_mobile_birth_item_recipe &recipe,
						   quest_mobile_native_item_stage *output) noexcept
{
	if (!output || output->state_ || !nevent_is_game_thread() ||
	    !native_mobile_birth_recipe_valid({ &literal, 1 }, { &recipe, 1 }) || !obj_index)
		return false;
	// Cold process may still have the authentic original prototype binding.
	// The original literal restore has the same strong output guarantee.
	if (restore(literal, recipe, output))
		return true;
	quest_mobile_native_item_stage candidate;
	try
	{
		const auto *prototype = find_bound_recovery_template(literal, recipe);
		if (!prototype || prototype->R_num < 0 || prototype->R_num > top_of_objt ||
		    obj_index[prototype->R_num].virtual_number != literal.vnum)
			return false;
		const int nr = prototype->R_num;
		obj_proc_type effective;
		if (!original_birth_procedure(recipe.procedure, &effective))
			return false;
		const auto original = obj_index[nr].func.obj;
		// The sealed catalog helper already proved the actual committed binding.
		auto state = std::make_unique<implementation>();
		state->index = obj_index;
		state->rnum = nr;
		state->vnum = literal.vnum;
		state->position = obj_index[nr].pos;
		state->uid = literal.object_uid;
		state->original_proc = original;
		state->effective_proc = effective;
		state->general_periodic = recipe.general_periodic;
		state->general_delay = recipe.general_delay;
		state->random_exit_requested = recipe.random_exit_requested;
		state->general_initialized = true;
		state->libraries.reserve(recipe.libraries.size());
		state->parsed_descriptions.reserve(recipe.libraries.size());
		state->requested.reserve(recipe.libraries.size());
		state->library_delays.reserve(recipe.libraries.size());
		native_mobile_birth_literal_stage literal_stage;
		if (!native_mobile_birth_literal_stage::prepare(*prototype, literal, literal_stage))
			return false;
		state->object = std::exchange(literal_stage.object_, nullptr);
		state->pool = std::exchange(literal_stage.pool_, nullptr);
		state->affect_pool = std::exchange(literal_stage.affect_pool_, nullptr);
		candidate.state_ = state.release();
		auto &s = *candidate.state_;
		P_obj object = s.object;
		object->trap_eff = recipe.trap_eff;
		object->trap_dam = recipe.trap_dam;
		object->trap_charge = recipe.trap_charge;
		object->trap_level = recipe.trap_level;
		SET_BIT(object->runtime_flags, OBJ_RFLAG_CREATION_CANDIDATE);
		size_t description_index = 0;
		for (auto *description = object->ex_description; description;
		     description = description->next)
		{
			if (description_index >= literal.extra_descriptions.size())
			{
				candidate.discard_unadmitted();
				return false;
			}
			if (literal.extra_descriptions[description_index].spellbook)
				s.allocated_spell_description = description;
			++description_index;
		}
		if (description_index != literal.extra_descriptions.size())
		{
			candidate.discard_unadmitted();
			return false;
		}
		for (const auto &saved : recipe.libraries)
		{
			size_t index;
			if (!quest_mobile_native_original_proclib::retained_index(saved.library,
										  &index))
			{
				candidate.discard_unadmitted();
				return false;
			}
			auto *description = object->ex_description;
			for (uint32_t i = 0; description && i < saved.extra_description_index; ++i)
				description = description->next;
			if (!description)
			{
				candidate.discard_unadmitted();
				return false;
			}
			s.libraries.push_back(index);
			s.parsed_descriptions.push_back(description);
			s.requested.push_back(saved.periodic_requested);
			s.library_delays.push_back(saved.delay);
			s.library_event_requested = s.library_event_requested ||
						    saved.periodic_requested;
		}
		s.parsed_proclib = !s.libraries.empty();
		if (!original_birth_literal_matches(object, literal) || obj_index != s.index ||
		    obj_index[nr].virtual_number != s.vnum || obj_index[nr].pos != s.position ||
		    obj_index[nr].func.obj != s.original_proc)
		{
			candidate.discard_unadmitted();
			return false;
		}
		if (effective == zombies_game &&
		    !quest_mobile_native_zombie_stage::restore(object, s.zombie))
		{
			candidate.discard_unadmitted();
			return false;
		}
		// No allocating/callback work follows the last original retained owner.
		output->state_ = candidate.state_;
		candidate.state_ = nullptr;
		return true;
	}
	catch (...)
	{
		candidate.discard_unadmitted();
		return false;
	}
}

bool quest_mobile_native_item_stage::restore_rebind(const player_item_snapshot &literal,
						    const native_mobile_birth_item_recipe &recipe,
						    quest_mobile_native_item_stage *output) noexcept
{
	if (recipe.binding_form != native_mobile_birth_binding_form::bridge || !output ||
	    output->state_ || !nevent_is_game_thread() ||
	    !native_mobile_birth_recipe_valid({ &literal, 1 }, { &recipe, 1 }) || !obj_index)
		return false;
	quest_mobile_native_item_stage candidate;
	try
	{
		const auto *prototype = find_recovery_object_template(literal.vnum);
		if (!prototype || prototype->R_num < 0 || prototype->R_num > top_of_objt ||
		    obj_index[prototype->R_num].virtual_number != literal.vnum)
			return false;
		const int nr = prototype->R_num;
		obj_proc_type effective;
		if (!original_birth_procedure(recipe.procedure, &effective))
			return false;
		const auto original = obj_index[nr].func.obj;
		// Strict catalog lookup already proved sealed/current equality. The
		// retained explicit bridge must name its exact actual bare predecessor.
		if (original == proclib_obj_cmd_bridge || original != effective)
			return false;
		auto state = std::make_unique<implementation>();
		state->index = obj_index;
		state->rnum = nr;
		state->vnum = literal.vnum;
		state->position = obj_index[nr].pos;
		state->uid = literal.object_uid;
		state->original_proc = original;
		state->effective_proc = effective;
		state->restored_bridge_request = true;
		state->general_periodic = recipe.general_periodic;
		state->general_delay = recipe.general_delay;
		state->random_exit_requested = recipe.random_exit_requested;
		state->general_initialized = true;
		state->libraries.reserve(recipe.libraries.size());
		state->parsed_descriptions.reserve(recipe.libraries.size());
		state->requested.reserve(recipe.libraries.size());
		state->library_delays.reserve(recipe.libraries.size());
		native_mobile_birth_literal_stage literal_stage;
		if (!native_mobile_birth_literal_stage::prepare(*prototype, literal, literal_stage))
			return false;
		state->object = std::exchange(literal_stage.object_, nullptr);
		state->pool = std::exchange(literal_stage.pool_, nullptr);
		state->affect_pool = std::exchange(literal_stage.affect_pool_, nullptr);
		candidate.state_ = state.release();
		auto &s = *candidate.state_;
		P_obj object = s.object;
		object->trap_eff = recipe.trap_eff;
		object->trap_dam = recipe.trap_dam;
		object->trap_charge = recipe.trap_charge;
		object->trap_level = recipe.trap_level;
		SET_BIT(object->runtime_flags, OBJ_RFLAG_CREATION_CANDIDATE);
		size_t description_index = 0;
		for (auto *description = object->ex_description; description;
		     description = description->next)
		{
			if (description_index >= literal.extra_descriptions.size())
			{
				candidate.discard_unadmitted();
				return false;
			}
			if (literal.extra_descriptions[description_index].spellbook)
				s.allocated_spell_description = description;
			++description_index;
		}
		if (description_index != literal.extra_descriptions.size())
		{
			candidate.discard_unadmitted();
			return false;
		}
		for (const auto &saved : recipe.libraries)
		{
			size_t index;
			if (!quest_mobile_native_original_proclib::retained_index(saved.library,
										  &index))
			{
				candidate.discard_unadmitted();
				return false;
			}
			auto *description = object->ex_description;
			for (uint32_t i = 0; description && i < saved.extra_description_index; ++i)
				description = description->next;
			if (!description)
			{
				candidate.discard_unadmitted();
				return false;
			}
			s.libraries.push_back(index);
			s.parsed_descriptions.push_back(description);
			s.requested.push_back(saved.periodic_requested);
			s.library_delays.push_back(saved.delay);
			s.library_event_requested = s.library_event_requested ||
						    saved.periodic_requested;
		}
		s.parsed_proclib = !s.libraries.empty();
		if (!original_birth_literal_matches(object, literal) || obj_index != s.index ||
		    obj_index[nr].virtual_number != s.vnum || obj_index[nr].pos != s.position ||
		    obj_index[nr].func.obj != s.original_proc)
		{
			candidate.discard_unadmitted();
			return false;
		}
		if (effective == zombies_game &&
		    !quest_mobile_native_zombie_stage::restore(object, s.zombie))
		{
			candidate.discard_unadmitted();
			return false;
		}
		// No allocating/callback work follows the last original retained owner.
		output->state_ = candidate.state_;
		candidate.state_ = nullptr;
		return true;
	}
	catch (...)
	{
		candidate.discard_unadmitted();
		return false;
	}
}

P_obj quest_mobile_native_item_stage::publish() noexcept
{
	if (!state_ || !nevent_is_game_thread())
		return nullptr;
	auto &s = *state_;
	if (!s.admitted || s.published || !s.object || s.object->obj_uid != s.uid ||
	    s.object->R_num != s.rnum || obj_index != s.index ||
	    obj_index[s.rnum].virtual_number != s.vnum || obj_index[s.rnum].pos != s.position ||
	    obj_index[s.rnum].number == INT_MAX || find_birth_live_object(s.object, s.uid))
		return nullptr;
	const auto current = obj_index[s.rnum].func.obj;
	if (current != s.original_proc &&
	    !((s.parsed_proclib || s.restored_bridge_request) &&
	      current == proclib_obj_cmd_bridge) &&
	    !(s.object->type == ITEM_SWITCH && !s.original_proc && current == item_switch))
		return nullptr;
	// Root committed and consumed the complete original binding batch first.
	if (((s.parsed_proclib || s.restored_bridge_request) &&
	     current != proclib_obj_cmd_bridge) ||
	    (s.object->type == ITEM_SWITCH && !current) ||
	    (current == proclib_obj_cmd_bridge &&
	     !proclib_recovery_chain_stage::predecessor_matches(
		     s.rnum, s.original_proc == proclib_obj_cmd_bridge ? s.effective_proc :
									 s.original_proc)))
		return nullptr;
	P_obj object = s.object;
	s.object = nullptr;
	s.published = true; // Consume before any original caller room/mobile hook.
	++obj_index[s.rnum].number;
	if (object_list)
		object_list->prev = object;
	object->next = object_list;
	object_list = object;
	return object;
}
size_t quest_mobile_native_item_stage::publication_step_count() const noexcept
{
	return state_ ? state_->libraries.size() * 2 + 3 : 0;
}
bool quest_mobile_native_item_stage::publication_step(
	size_t step, P_obj expected, quest_mobile_native_item_effect &effect) noexcept
{
	if (!state_ || !nevent_is_game_thread() || !state_->published ||
	    step >= publication_step_count() || step != state_->next_step ||
	    state_->current_step_started || effect.started)
		return false;
	auto &s = *state_;
	P_obj object = find_birth_live_object(expected, s.uid);
	if (!object || object->R_num != s.rnum || obj_index != s.index ||
	    obj_index[s.rnum].virtual_number != s.vnum || obj_index[s.rnum].pos != s.position)
		return false;
	const auto current = obj_index[s.rnum].func.obj;
	if (((s.parsed_proclib || s.restored_bridge_request) &&
	     current != proclib_obj_cmd_bridge) ||
	    (!(s.parsed_proclib || s.restored_bridge_request) && current != s.original_proc &&
	     !(object->type == ITEM_SWITCH && !s.original_proc && current == item_switch)) ||
	    (current == proclib_obj_cmd_bridge &&
	     !proclib_recovery_chain_stage::predecessor_matches(
		     s.rnum, s.original_proc == proclib_obj_cmd_bridge ? s.effective_proc :
									 s.original_proc)))
		return false;
	const size_t general = s.libraries.size() * 2;
	try
	{
		s.current_step_started = true;
		effect.started = true;
		if (step < general)
		{
			const size_t index = step / 2;
			if (!(step & 1))
			{
				// This original safe probe already actually returned during preparation.
				// Confirm retained facts only; never invoke it again or choose new RNG.
				effect.returned = true;
				effect.periodic = s.requested[index];
				effect.succeeded = true;
			}
			else
			{
				effect.succeeded = !s.requested[index] ||
						   get_scheduled(object, proclib_obj_event) ||
						   add_event(proclib_obj_event,
							     s.library_delays[index], nullptr,
							     nullptr, object, 0, nullptr, 0)
							   .was_scheduled();
				effect.returned = true;
				effect.succeeded = effect.succeeded &&
						   find_birth_live_object(expected, s.uid);
			}
		}
		else if (step == general)
		{
			// Original object-local initialization already returned before literal
			// capture. Only the retained ZombieGame global tail remains here.
			effect.succeeded = !s.zombie.game_ || s.zombie.publish(object);
			effect.periodic = s.general_periodic;
			effect.returned = true;
			s.general_periodic = effect.periodic;
			effect.succeeded = effect.succeeded &&
					   find_birth_live_object(expected, s.uid);
		}
		else if (step == general + 1)
		{
			effect.succeeded = !s.general_periodic ||
					   get_scheduled(object, event_object_proc) ||
					   add_event(event_object_proc, s.general_delay, nullptr,
						     nullptr, object, 0, nullptr, 0)
						   .was_scheduled();
			effect.returned = true;
			effect.succeeded = effect.succeeded &&
					   find_birth_live_object(expected, s.uid);
		}
		else
		{
			effect.succeeded = !s.random_exit_requested ||
					   get_scheduled(object, event_random_exit) ||
					   add_event(event_random_exit, 3, nullptr, nullptr, object,
						     0, nullptr, 0)
						   .was_scheduled();
			effect.returned = true;
			effect.succeeded = effect.succeeded &&
					   find_birth_live_object(expected, s.uid);
		}
		if (effect.returned && effect.succeeded)
		{
			++s.next_step;
			s.current_step_started = false;
		}
		return effect.returned && effect.succeeded;
	}
	catch (...)
	{
		return false;
	}
}

bool quest_mobile_native_item_stage::read_progress(
	quest_mobile_native_item_progress *output) const noexcept
{
	if (!state_ || !output || !nevent_is_game_thread() || state_->next_step > UINT32_MAX)
		return false;
	*output = { static_cast<uint32_t>(state_->next_step), state_->current_step_started,
		    state_->admitted, state_->published };
	return true;
}

bool quest_mobile_native_item_stage::adopt_published(
	const player_item_snapshot &literal, const native_mobile_birth_item_recipe &recipe,
	P_obj actual, const quest_mobile_native_item_progress &progress,
	std::span<const quest_mobile_native_item_effect> effects,
	quest_mobile_native_item_stage *output) noexcept
{
	// Private original owner must authenticate its command, carrier, receipt,
	// native lifetime and complete SQL/world cut. These values grant no authority.
	if (!output || output->state_ || !actual || !nevent_is_game_thread() ||
	    !progress.admitted || !progress.published ||
	    !native_mobile_birth_recipe_valid({ &literal, 1 }, { &recipe, 1 }))
		return false;
	const size_t count = recipe.libraries.size() * 2 + 3;
	if (effects.size() != count || progress.next_step > count ||
	    (progress.current_step_started && progress.next_step == count))
		return false;
	for (size_t i = 0; i < count; ++i)
	{
		const auto &effect = effects[i];
		const size_t general = recipe.libraries.size() * 2;
		const bool expected_periodic = i < general && !(i & 1) ?
						       recipe.libraries[i / 2].periodic_requested :
						       (i == general && recipe.general_periodic);
		if (effect.periodic != (effect.returned && expected_periodic))
			return false;
		if (i < progress.next_step)
		{
			if (!effect.started || !effect.returned || !effect.succeeded)
				return false;
		}
		else if (i == progress.next_step && progress.current_step_started)
		{
			if (!effect.started || effect.succeeded)
				return false;
		}
		else if (effect.started || effect.returned || effect.succeeded || effect.periodic)
			return false;
	}
	quest_mobile_native_item_stage candidate;
	try
	{
		const auto *prototype = find_bound_recovery_template(literal, recipe);
		if (!prototype || prototype->R_num < 0 || prototype->R_num > top_of_objt ||
		    !obj_index || actual->R_num != prototype->R_num ||
		    find_birth_live_object(actual, literal.object_uid) != actual ||
		    !original_birth_literal_matches(actual, literal) ||
		    actual->trap_eff != recipe.trap_eff || actual->trap_dam != recipe.trap_dam ||
		    actual->trap_charge != recipe.trap_charge ||
		    actual->trap_level != recipe.trap_level)
			return false;
		size_t matching = 0;
		for (P_obj object = object_list; object; object = object->next)
			if (object->obj_uid == literal.object_uid)
			{
				if (object != actual)
					return false;
				++matching;
			}
		if (matching != 1)
			return false;
		// Retained returned-success never recreates an event or proves its current
		// scheduler presence. Adopt only the actual requested completed schedule.
		for (size_t i = 0; i < recipe.libraries.size(); ++i)
			if (effects[i * 2 + 1].succeeded &&
			    recipe.libraries[i].periodic_requested &&
			    !get_scheduled(actual, proclib_obj_event))
				return false;
		const size_t general = recipe.libraries.size() * 2;
		if ((effects[general + 1].succeeded && recipe.general_periodic &&
		     !get_scheduled(actual, event_object_proc)) ||
		    (effects[general + 2].succeeded && recipe.random_exit_requested &&
		     !get_scheduled(actual, event_random_exit)))
			return false;
		obj_proc_type effective;
		if (!original_birth_procedure(recipe.procedure, &effective))
			return false;
		const int nr = prototype->R_num;
		const auto current = obj_index[nr].func.obj;
		obj_proc_type original = recipe.binding_form ==
							 native_mobile_birth_binding_form::bridge ?
						 proclib_obj_cmd_bridge :
						 effective;
		const bool bridge = original == proclib_obj_cmd_bridge || !recipe.libraries.empty();
		if (bridge)
		{
			if (current != proclib_obj_cmd_bridge ||
			    !proclib_recovery_chain_stage::predecessor_matches(nr, effective))
				return false;
		}
		else if (current != original &&
			 !(original == nullptr && actual->type == ITEM_SWITCH &&
			   current == item_switch))
			return false;
		auto state = std::make_unique<implementation>();
		state->index = obj_index;
		state->rnum = nr;
		state->vnum = literal.vnum;
		state->position = obj_index[nr].pos;
		state->uid = literal.object_uid;
		state->original_proc = original;
		state->effective_proc = effective;
		state->admitted = true;
		state->published = true;
		state->general_initialized = true;
		state->general_periodic = recipe.general_periodic;
		state->general_delay = recipe.general_delay;
		state->random_exit_requested = recipe.random_exit_requested;
		state->next_step = progress.next_step;
		state->current_step_started = progress.current_step_started;
		state->libraries.reserve(recipe.libraries.size());
		state->parsed_descriptions.reserve(recipe.libraries.size());
		state->requested.reserve(recipe.libraries.size());
		state->library_delays.reserve(recipe.libraries.size());
		for (const auto &saved : recipe.libraries)
		{
			size_t index;
			if (!quest_mobile_native_original_proclib::retained_index(saved.library,
										  &index))
				return false;
			auto *description = actual->ex_description;
			for (uint32_t i = 0; description && i < saved.extra_description_index; ++i)
				description = description->next;
			if (!description)
				return false;
			state->libraries.push_back(index);
			state->parsed_descriptions.push_back(description);
			state->requested.push_back(saved.periodic_requested);
			state->library_delays.push_back(saved.delay);
			state->library_event_requested = state->library_event_requested ||
							 saved.periodic_requested;
		}
		state->parsed_proclib = !state->libraries.empty();
		candidate.state_ = state.release();
		if (effective == zombies_game)
		{
			if (progress.next_step > general)
			{
				if (!quest_mobile_native_zombie_stage::observe_published(actual))
				{
					delete candidate.state_;
					candidate.state_ = nullptr;
					return false;
				}
			}
			else if (progress.current_step_started && progress.next_step == general &&
				 quest_mobile_native_zombie_stage::observe_published(actual))
			{
				// Actual global tail may be present, but the retained started latch
				// is still uncertain. Do not infer returned or repeat the tail.
			}
			else if (!quest_mobile_native_zombie_stage::restore(
					 actual, candidate.state_->zombie))
			{
				delete candidate.state_;
				candidate.state_ = nullptr;
				return false;
			}
		}
		// Metadata only. Never adds a native object/count, proc, event or UID.
		candidate.state_->metadata_borrowed_world = true;
		output->state_ = candidate.state_;
		candidate.state_ = nullptr;
		return true;
	}
	catch (...)
	{
		if (candidate.state_)
		{
			candidate.state_->zombie.discard();
			delete candidate.state_;
			candidate.state_ = nullptr;
		}
		return false;
	}
}

bool quest_mobile_native_item_stage::abandon_adoption() noexcept
{
	if (!state_ || !nevent_is_game_thread() || !state_->metadata_borrowed_world ||
	    !state_->admitted || !state_->published || state_->pool || state_->affect_pool)
		return false;
	// discard owns only an unpublished private Zombie reservation, when present;
	// it refuses any globally enrolled game. Published generators stay untouched.
	if (!state_->zombie.discard())
		return false;
	delete state_;
	state_ = nullptr;
	return true;
}

bool quest_mobile_native_item_stage::rebuild_enrollment(
	P_obj expected, const native_mobile_birth_item_recipe &recipe,
	const quest_mobile_native_item_progress &progress,
	std::span<const quest_mobile_native_item_effect> effects) noexcept
{
	if (!state_ || !nevent_is_game_thread() || !state_->admitted || !state_->published ||
	    state_->metadata_borrowed_world || !expected || !progress.admitted ||
	    !progress.published || progress.current_step_started || state_->current_step_started ||
	    expected->obj_uid != state_->uid ||
	    find_birth_live_object(expected, state_->uid) != expected ||
	    effects.size() != publication_step_count() || progress.next_step > effects.size())
		return false;
	auto &s = *state_;
	const size_t general = s.libraries.size() * 2;
	for (size_t step = 0; step < effects.size(); ++step)
	{
		const auto &effect = effects[step];
		const bool expected_periodic = step < general && !(step & 1) ?
						       s.requested[step / 2] :
						       (step == general && s.general_periodic);
		if (effect.periodic != (effect.returned && expected_periodic))
			return false;
		if (step < progress.next_step)
		{
			if (!effect.started || !effect.returned || !effect.succeeded)
				return false;
		}
		else if (effect.started || effect.returned || effect.succeeded || effect.periodic)
			return false;
	}
	if (recipe.object_uid != s.uid || recipe.libraries.size() != s.libraries.size() ||
	    recipe.general_periodic != s.general_periodic ||
	    recipe.general_delay != s.general_delay ||
	    recipe.random_exit_requested != s.random_exit_requested)
		return false;
	for (size_t i = 0; i < s.libraries.size(); ++i)
		if (recipe.libraries[i].periodic_requested != s.requested[i] ||
		    recipe.libraries[i].delay != s.library_delays[i])
			return false;
	try
	{
		if (s.rebuilding_enrollment)
		{
			// Exact prefix plus internal original requested-periodic values
			// determine every validated effect bit without another heap copy.
			if (s.rebuilding_object != expected ||
			    s.rebuilding_prefix != progress.next_step)
				return false;
		}
		else
		{
			if (s.next_step)
				return false;
			s.rebuilding_object = expected;
			s.rebuilding_prefix = progress.next_step;
			s.rebuilding_enrollment = true;
		}
		if (obj_index != s.index || expected->R_num != s.rnum ||
		    obj_index[s.rnum].virtual_number != s.vnum ||
		    obj_index[s.rnum].pos != s.position)
			return false;
		const auto current = obj_index[s.rnum].func.obj;
		if (((s.parsed_proclib || s.restored_bridge_request) &&
		     current != proclib_obj_cmd_bridge) ||
		    (!(s.parsed_proclib || s.restored_bridge_request) &&
		     current != s.original_proc &&
		     !(expected->type == ITEM_SWITCH && !s.original_proc &&
		       current == item_switch)) ||
		    (current == proclib_obj_cmd_bridge &&
		     !proclib_recovery_chain_stage::predecessor_matches(
			     s.rnum, s.original_proc == proclib_obj_cmd_bridge ? s.effective_proc :
										 s.original_proc)))
			return false;
		if (s.effective_proc == zombies_game && progress.next_step > general)
		{
			if (!quest_mobile_native_zombie_stage::observe_published(expected))
			{
				if (s.rebuilding_zombie || !s.zombie.game_ ||
				    !s.zombie.publish(expected))
					return false;
			}
			if (!quest_mobile_native_zombie_stage::observe_published(expected))
				return false;
			s.rebuilding_zombie = true;
		}
		bool library_requested = false;
		int library_delay = 0;
		for (size_t i = 0; i < s.libraries.size(); ++i)
			if (progress.next_step > i * 2 + 1 && s.requested[i])
			{
				// Original sequence schedules at most one shared library event.
				// The first actual requested successful step chooses its delay.
				if (!library_requested)
					library_delay = s.library_delays[i];
				library_requested = true;
			}
		const std::array<event_func_type, 3> callbacks{ proclib_obj_event,
								event_object_proc,
								event_random_exit };
		const std::array<bool, 3> requested{
			library_requested, progress.next_step > general + 1 && s.general_periodic,
			progress.next_step > general + 2 && s.random_exit_requested
		};
		const std::array<int, 3> delays{ library_delay, s.general_delay, 3 };
		for (size_t i = 0; i < callbacks.size(); ++i)
		{
			if (!requested[i])
				continue;
			P_nevent found = nullptr;
			std::unordered_set<P_nevent> seen;
			for (P_nevent event = expected->nevents; event; event = event->next_obj_nev)
			{
				if (!seen.insert(event).second)
					return false;
				if (event->func != callbacks[i])
					continue;
				if (found || event->obj != expected || event->ch || event->victim ||
				    event->data ||
				    !nevent_handle_is_active(nevent_handle_from_event(event)))
					return false;
				found = event;
			}
			if (!found)
			{
				if (s.rebuilding_events[i] || delays[i] <= 0)
					return false;
				if (!add_event(callbacks[i], delays[i], nullptr, nullptr, expected,
					       0, nullptr, 0)
					     .was_scheduled())
					return false;
			}
			s.rebuilding_events[i] = true;
		}
		s.next_step = progress.next_step;
		s.enrollment_rebuilt = true;
		return true;
	}
	catch (...)
	{
		return false; // Never discard or rewind actual published ownership.
	}
}

bool quest_mobile_native_item_stage::release_published() noexcept
{
	if (!state_ || !nevent_is_game_thread() || !state_->published ||
	    state_->next_step != publication_step_count() || state_->zombie.game_)
		return false;
	delete state_;
	state_ = nullptr;
	return true;
}

/* Non-starter callers retain cold loading; both paths share one parser. */
P_obj read_object(int nr, int type)
{
	if (type == VIRTUAL)
		nr = real_object(nr);
	if (nr < 0 || nr > top_of_objt)
		return nullptr;
	return instantiate_object_template(parse_object_template(nr));
}

/*  Function to reset no_reset zones...
 *  This function will be called at a time during boot
 *     A) After the original stone has been touched
 *     B) Random event timer expires(event is applied after touch)
 *     C) Incremental chance of reset occurring above random roll
 *
 *  Why have this?  It moves us closer to a game-world that is persistent
 *  and auto-refreshes.  Incremental time will be stored in DB and will
 *  be modified by frequency of overall zone resets occurring through
 * this method.
 */
void no_reset_zone_reset(int zone_number)
{
	if (!qry("SELECT reset_perc FROM zones WHERE id = '%d'", zone_number))
	{
		logit(LOG_DEBUG,
		      "no_reset_zone_reset: could not find zone information for zone id %d",
		      zone_number);
		return;
	}

	MYSQL_RES *res = mysql_store_result(DB);

	if (mysql_num_rows(res) < 1)
	{
		logit(LOG_DEBUG, "No data retrieved from DB for no_reset_zone_reset...");
		mysql_free_result(res);
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(res);

	if (epic_zone_done_now(zone_table[zone_number].number) && atoi(row[0]) > number(0, 99))
	{
		// zone_purge(zone_number);
		reset_zone(zone_number, 0);
		db_query("UPDATE zones SET reset_perc = '%d' WHERE id = '%d'", 0, zone_number);
		// epic_zone_erase_touch(zone_table[zone_number].number);
	}
	else
	{
		add_event(event_reset_zone, WAIT_MIN * 60, 0, 0, 0, 0, &zone_number,
			  sizeof(zone_number));
		db_query("UPDATE zones SET reset_perc = '%d' WHERE id = '%d'", atoi(row[0]) + 1,
			 zone_number);
	}
	mysql_free_result(res);
}

#define ZCMD zone_table[zone].cmd[cmd_no]

static bool room_has_shopkeeper(int mobile_rnum, int room_rnum)
{
	if (room_rnum < 0 || room_rnum > top_of_world)
		return false;
	for (P_char keeper = world[room_rnum].people; keeper; keeper = keeper->next_in_room)
		if (IS_SHOPKEEPER(keeper) && GET_RNUM(keeper) == mobile_rnum)
			return true;
	return false;
}

static int configured_shopkeeper_for_room(int mobile_rnum, int room_rnum)
{
	if (!shop_index || number_of_shops <= 0 || room_rnum < 0 || room_rnum > top_of_world)
		return -1;
	const int room = world[room_rnum].number;
	int match = -1;
	for (int shop = 0; shop < number_of_shops; ++shop)
		if (shop_index[shop].keeper == mobile_rnum && shop_index[shop].in_room == room)
		{
			if (match >= 0)
				return -1;
			match = shop;
		}
	return match;
}

static bool live_shopkeeper_for_identity(int shop)
{
	if (!shop_index || shop < 0 || shop >= number_of_shops)
		return false;
	for (P_char keeper = character_list; keeper; keeper = keeper->next)
		if (singleton_shop_id(keeper) == shop)
			return true;
	return false;
}

static bool reset_command_issues_item(char command)
{
	switch (command)
	{
	case 'B':
	case 'C':
	case 'A':
	case 'O':
	case 'P':
	case 'G':
	case 'E':
		return true;
	default:
		return false;
	}
}

/* execute the reset command table of a given zone */
/* force_item_repop : 2 means this is a boot-time initial reset of zone. */
void reset_zone(int zone, int force_item_repop)
{
	const bool native_sql_reset = economic_gameplay_authority::active_regular_sql();
	if (economic_gameplay_authority::active() &&
	    !quest_mobile_native_birth_owner::begin_reset(zone, force_item_repop))
		return;
	const int respawn = get_property("artifact.respawn", 0);
	int cmd_no, last_cmd = 1, last_mob_load = 0;
	int temp, ival, configured_shop, replicated_shop;
	P_char mob = NULL, last_mob = NULL, tmp_mob = NULL, last_mob_followable = NULL;
	P_obj obj, obj_to;
	arti_data artidata;
	char buf[MAX_STRING_LENGTH];

	logit(LOG_STATUS, "reset_zone: reseting zone '%s', force_item_repop: %d",
	      zone_table[zone].filename, force_item_repop);
	for (cmd_no = 0;; cmd_no++)
	{
		if (ZCMD.command == 'S')
			break;
		// Zone item commands lack a durable reset-generation identity. Refuse
		// before read_object or any live placement during an accounting epoch.
		if (economic_gameplay_authority::active() &&
		    reset_command_issues_item(ZCMD.command) &&
		    !(native_sql_reset &&
		      (ZCMD.command == 'G' || ZCMD.command == 'E' || ZCMD.command == 'P') &&
		      quest_mobile_native_birth_owner::owns(mob)))
		{
			last_cmd = 0;
			continue;
		}

		if (economic_gameplay_authority::active() &&
		    (ZCMD.command == 'Y' || ZCMD.command == 'F' || ZCMD.command == 'R'))
		{
			// These original callbacks need their own complete retained owner.
			// Do not pass unpublished native pointers into follower/mount hooks.
			if (native_sql_reset)
				quest_mobile_native_birth_owner::block_mobile();
			mob = last_mob = tmp_mob = last_mob_followable = NULL;
			last_cmd = last_mob_load = 0;
			continue;
		}

		/* last_mob_load added in case an equipment load fails due to a random
		   roll..  we want the rest of the stuff on the mob to happen (followers,
		   other equip, items, riders), and if the original mob loaded, let's
		   let it all happen */

		if (last_cmd || !ZCMD.if_flag ||
		    (last_mob_load &&
		     ((ZCMD.command == 'G') || (ZCMD.command == 'E') || (ZCMD.command == 'R'))) ||
		    (last_mob_followable && (ZCMD.command == 'F')))
			switch (ZCMD.command)
			{
			case 'Y':
				last_cmd = 0;
				temp = get_mob_table(ZCMD.arg1);
				if (!temp)
				{
					last_cmd = 0;
					break;
				}
				if (real_mobile(temp) == -1)
				{
					last_cmd = 0;
					break;
				}
				// set the mob limit from zone file
				mob_index[real_mobile(temp)].limit = ZCMD.arg2;

				if (mob_index[real_mobile(temp)].number < ZCMD.arg2)
				{
					mob = read_mobile(temp, VIRTUAL);
					if (!mob)
					{
						last_cmd = 0;
						break;
					}
					tmp_mob = NULL;
					last_mob = mob;
					GET_BIRTHPLACE(mob) = world[ZCMD.arg3].number;
					apply_zone_modifier(mob);
					char_to_room(mob, ZCMD.arg3, -2);
					npc_alchemist_world_spawn(mob);
					last_cmd = 1;
				}
				else
					last_cmd = 0;

				break;

			case 'B':
				last_cmd = 0;
				temp = get_obj_table(ZCMD.arg1);
				if (!temp)
					break;
				temp = real_object(temp);
				if (temp == -1)
					break;

				obj_index[temp].limit =
					ZCMD.arg2; // set the mob limit from zone file

				if ((ZCMD.arg3 >= 0) && (obj_index[temp].number < ZCMD.arg2))
				{
					if (!(obj = read_object(temp, REAL)))
					{
						break;
					}
					if (get_artifact_data_sql(obj_index[temp].virtual_number,
								  &artidata))
					{
						// If the artifact is owned, then it's timer is ticking somwhere, so we don't need to load another.
						if (artidata.owned)
						{
							extract_obj(obj);
							break;
						}
					}
					// Remove the artifact unless artifact.respawn == 0
					//   or artifact.respawn == 1 and we are not booting.
					if (IS_ARTIFACT(obj) &&
					    (respawn == 0 ||
					     (respawn == 1 && force_item_repop != 2)))
					{
						extract_obj(obj);
						break;
					}

					ival = itemvalue(obj);
					if (!ITEM_LOAD_CHECK(obj, ival, ZCMD.arg4))
					{
						extract_obj(obj);
						last_cmd = 1;
						break;
					}

					obj_to = get_obj_num(ZCMD.arg3);
					if (!obj_to)
					{
						extract_obj(obj);
						break;
					}
					obj_to_obj(obj, obj_to);
					// Artifact poof timer to BLOOD_DAYS * secs in a day.
					obj->timer[3] = time(NULL);
					last_cmd = 1;
					break;
				}

				break;

			case 'C': /* As of 11/8/2015, no zones have a case 'C' .. hrm.
				           *   Checked all files in gmud/areas/zon/ - only zone names starting with C show up.
				           */
				last_cmd = 0;
				temp = get_obj_table(ZCMD.arg1);
				if (!temp)
					break;
				temp = real_object(temp);
				if (temp == -1)
					break;

				obj_index[temp].limit = ZCMD.arg2; // set the limit from zone file

				if (obj_index[temp].number > ZCMD.arg2)
					break; /* enough in game */
				if (ZCMD.arg3 == -1)
				{
					/* bad command, disable */
					ZCMD.command = '!';
					break;
				}
				if (!(obj = read_object(temp, REAL)))
				{
					break;
				}
				if (get_artifact_data_sql(obj_index[temp].virtual_number,
							  &artidata))
				{
					// If the artifact is owned, then it's timer is ticking somwhere, so we don't need to load another.
					if (artidata.owned)
					{
						extract_obj(obj);
						break;
					}
				}
				// Remove the artifact unless artifact.respawn == 0
				//   or artifact.respawn == 1 and we are not booting.
				if (IS_ARTIFACT(obj) &&
				    (respawn == 0 || (respawn == 1 && force_item_repop != 2)))
				{
					extract_obj(obj);
					break;
				}

				ival = itemvalue(obj);
				if (!ITEM_LOAD_CHECK(obj, ival, ZCMD.arg4))
				{
					extract_obj(obj);
					last_cmd = 1;
					break;
				}

				obj_to_room(obj, ZCMD.arg3);
				// Artifact poof timer to BLOOD_DAYS * secs in a day.
				obj->timer[3] = time(NULL);
				last_cmd = 1;

				break;

			case 'A': /* As of 11/8/2015, no zones have a case 'A' .. hrm.
				           *   Checked all files in gmud/areas/zon/ - only zone names starting with A show up.
				           */
				last_cmd = 0;
				temp = get_obj_table(ZCMD.arg1);
				if (!temp)
					break;
				temp = real_object(temp);
				if (temp == -1)
					break;

				obj_index[ZCMD.arg1].limit =
					ZCMD.arg2; // set the limit from zone file

				if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
				{
					if (!(obj = read_object(temp, REAL)))
					{
						break;
					}
					if (get_artifact_data_sql(obj_index[temp].virtual_number,
								  &artidata))
					{
						// If the artifact is owned, then it's timer is ticking somwhere, so we don't need to load another.
						if (artidata.owned)
						{
							extract_obj(obj);
							break;
						}
					}
					// Remove the artifact unless artifact.respawn == 0
					//   or artifact.respawn == 1 and we are not booting.
					if (IS_ARTIFACT(obj) &&
					    (respawn == 0 ||
					     (respawn == 1 && force_item_repop != 2)))
					{
						extract_obj(obj);
						break;
					}
					ival = itemvalue(obj);
					// Load all shopkeeper eq.
					if (!ITEM_LOAD_CHECK(obj, ival, ZCMD.arg4) &&
					    (!mob || !IS_SHOPKEEPER(mob)))
					{
						extract_obj(obj);
						last_cmd = 1;
						break;
					}
					if (mob) /* last mob */
					{
						obj_to_char(obj, mob);
						last_cmd = 1;
						break;
					}
					else
					{
						logit(LOG_DEBUG,
						      "reset_zone: object '%s' %d could not be placed on bad mob - zone %s.",
						      OBJ_SHORT(obj), OBJ_VNUM(obj),
						      zone_table[zone].filename);
						extract_obj(obj);
					}
				}
				break;

			case 'M': /* read a mobile */
				if (native_sql_reset)
					quest_mobile_native_birth_owner::seal_mobile();
				mob_index[ZCMD.arg1].limit =
					ZCMD.arg2; // set the limit from zone file
				configured_shop =
					configured_shopkeeper_for_room(ZCMD.arg1, ZCMD.arg3);
				replicated_shop = -1;
				if (configured_shop >= 0 && is_replicated_shop(configured_shop))
					replicated_shop = configured_shop;

				// Replicated shop identities are room-scoped: the same mobile prototype
				// may legitimately have one keeper in each configured shop room. A fixed
				// keeper that has walked away from home still owns its global identity.
				if ((native_sql_reset &&
				     quest_mobile_native_birth_owner::pending_shop(
					     ZCMD.arg1, ZCMD.arg3, configured_shop)) ||
				    room_has_shopkeeper(ZCMD.arg1, ZCMD.arg3) ||
				    (configured_shop >= 0 && replicated_shop < 0 &&
				     live_shopkeeper_for_identity(configured_shop)) ||
				    !((replicated_shop >= 0 && ZCMD.arg2 > 0 && ZCMD.arg4 == 100) ||
				      (static_cast<int64_t>(mob_index[ZCMD.arg1].number) +
						       (native_sql_reset ?
								static_cast<int64_t>(
									quest_mobile_native_birth_owner::
										pending_mobiles(
											ZCMD.arg1)) :
								0) <
					       ZCMD.arg2 &&
				       ZCMD.arg4 == 100) ||
				      force_item_repop))
				{
					mob = last_mob = tmp_mob = last_mob_followable = NULL;
					last_cmd = last_mob_load = 0;
					break;
				}
				if (ZCMD.arg4 > number(0, 99))
				{
					if (!(mob = native_sql_reset ?
							    quest_mobile_native_birth_owner::
								    prepare_mobile(
									    ZCMD.arg1, ZCMD.arg3,
									    cmd_no,
									    configured_shop) :
							    read_mobile(ZCMD.arg1, REAL)))
					{
						if (!native_sql_reset)
							ZCMD.command = '!';
						logit(LOG_DEBUG,
						      "reset_zone(): (zone %d) mob %d [%d] not loadable",
						      zone, ZCMD.arg1,
						      mob_index[ZCMD.arg1].virtual_number);
					}
				}
				else
				{
					mob = 0;
					last_mob = 0;
					logit(LOG_MOB, "M cmd not executed %d %d %d %d", ZCMD.arg1,
					      ZCMD.arg2, ZCMD.arg3, ZCMD.arg4);
				}
				if (!mob)
				{
					last_cmd = last_mob_load = 0;
					last_mob_followable = 0;
					break;
				}
				tmp_mob = NULL;
				last_mob = last_mob_followable = mob;
				/* Safety check: ensure room rnum is valid before accessing world array */
				if (ZCMD.arg3 < 0 || ZCMD.arg3 > top_of_world)
				{
					logit(LOG_DEBUG,
					      "reset_zone: M cmd zone %d has invalid room rnum %d",
					      zone, ZCMD.arg3);
					if (!native_sql_reset)
						extract_char(mob);
					ZCMD.command = '!';
					last_cmd = last_mob_load = 0;
					break;
				}
				GET_BIRTHPLACE(mob) = world[ZCMD.arg3].number;
				apply_zone_modifier(mob);
				if (configured_shop >= 0)
					bind_shopkeeper(mob, configured_shop);
				if (!native_sql_reset)
				{
					char_to_room(mob, ZCMD.arg3, -2);
					npc_alchemist_world_spawn(mob);
				}
				else if (!quest_mobile_native_birth_owner::capture_alchemist_spawn(
						 mob, ZCMD.arg3))
				{
					quest_mobile_native_birth_owner::block_mobile();
					mob = last_mob = tmp_mob = last_mob_followable = nullptr;
					last_cmd = last_mob_load = 0;
					break;
				}
				last_cmd = last_mob_load = 1;
				break;

			case 'O': /* load an object to room */
				obj_index[ZCMD.arg1].limit =
					ZCMD.arg2; // set the limit from zone file

				if ((ZCMD.arg1 >= 0) && (ZCMD.arg3 >= 0) &&
				    ((obj_index[ZCMD.arg1].number < ZCMD.arg2 && ZCMD.arg4 == 100) ||
				     force_item_repop))
				{
					if (!(obj = get_obj_in_list_num(
						      ZCMD.arg1, world[ZCMD.arg3].contents)) ||
					    IS_SET(obj->wear_flags, ITEM_TAKE))
					{
						obj = NULL;
						if (!(obj = read_object(ZCMD.arg1, REAL)))
						{
							ZCMD.command = '!';
							logit(LOG_DEBUG,
							      "reset_zone(): (zone %d) obj %d [%d] not loadable",
							      zone, ZCMD.arg1,
							      obj_index[ZCMD.arg1].virtual_number);
						}
						if (obj)
						{
							if (IS_ARTIFACT(obj) &&
							    get_artifact_data_sql(
								    obj_index[ZCMD.arg1]
									    .virtual_number,
								    &artidata))
							{
								// If the artifact is owned, then it's timer is ticking somwhere, so we don't need to load another.
								if (artidata.owned)
								{
									extract_obj(obj);
									break;
								}
							}
							if (IS_ARTIFACT(obj) &&
							    (respawn == 0 ||
							     (respawn == 1 &&
							      force_item_repop != 2)))
							{
								extract_obj(obj);
								break;
							}
							ival = itemvalue(obj);
							if (!ITEM_LOAD_CHECK(obj, ival, ZCMD.arg4))
							{
								extract_obj(obj);
								last_cmd = 1;
								break;
							}
							obj_to_room(obj, ZCMD.arg3);
							last_cmd = 1;
							break;
						}
					}
					else
						last_cmd = 0;
				}
				else if (obj_index[ZCMD.arg1].number < ZCMD.arg2)
				{
					logit(LOG_OBJ,
					      "O cmd: obj: %d to_room: %d, chance: %d, limit %d(%d)",
					      obj_index[ZCMD.arg1].virtual_number,
					      (ZCMD.arg3 >= 0) ? world[ZCMD.arg3].number : -2,
					      ZCMD.arg4, ZCMD.arg2, obj_index[ZCMD.arg1].number);
					ZCMD.command = '!'; /* disable */
				}
				last_cmd = 0;
				break;

			case 'P': /* object to object */
				last_cmd = 0;
				obj_index[ZCMD.arg1].limit =
					ZCMD.arg2; // set the limit from zone file

				if ((ZCMD.arg1 >= 0) && (ZCMD.arg3 >= 0) &&
				    (((static_cast<int64_t>(obj_index[ZCMD.arg1].number) +
				       (native_sql_reset ?
						static_cast<int64_t>(
							quest_mobile_native_birth_owner::
								pending_items(ZCMD.arg1)) :
						0)) < ZCMD.arg2) ||
				     force_item_repop))
				{
					if (!(obj = (native_sql_reset ?
							     quest_mobile_native_birth_owner::
								     prepare_item(ZCMD.arg1) :
							     read_object(ZCMD.arg1, REAL))))
					{
						if (!native_sql_reset)
							ZCMD.command = '!';
						logit(LOG_DEBUG,
						      "reset_zone(): (zone %d) obj %d [%d] not loadable",
						      zone, ZCMD.arg1,
						      obj_index[ZCMD.arg1].virtual_number);
					}
					if (obj)
					{
						if (IS_ARTIFACT(obj) &&
						    get_artifact_data_sql(
							    obj_index[ZCMD.arg1].virtual_number,
							    &artidata))
						{
							// If the artifact is owned, then it's timer is ticking somwhere, so we don't need to load another.
							if (artidata.owned)
							{
								if (native_sql_reset)
									quest_mobile_native_birth_owner::
										discard_item(obj);
								else
									extract_obj(obj);
								break;
							}
						}

						obj_to =
							native_sql_reset ?
								quest_mobile_native_birth_owner::
									original_object(ZCMD.arg3) :
								get_obj_num(ZCMD.arg3);
						if (obj_to)
						{
							if (IS_ARTIFACT(obj) &&
							    (respawn == 0 ||
							     (respawn == 1 &&
							      force_item_repop != 2)))
							{
								if (native_sql_reset)
									quest_mobile_native_birth_owner::
										discard_item(obj);
								else
									extract_obj(obj);
								break;
							}
							ival = itemvalue(obj);
							if (!ITEM_LOAD_CHECK(obj, ival, ZCMD.arg4))
							{
								if (native_sql_reset)
									quest_mobile_native_birth_owner::
										discard_item(obj);
								else
									extract_obj(obj);
								last_cmd = 1;
								break;
							}
							if (native_sql_reset)
								quest_mobile_native_birth_owner::nest(
									obj, obj_to, mob);
							else
								obj_to_obj(obj, obj_to);
							last_cmd = 1;
							break;
						}
					}
				}
				else if ((static_cast<int64_t>(obj_index[ZCMD.arg1].number) +
					  (native_sql_reset ?
						   static_cast<int64_t>(
							   quest_mobile_native_birth_owner::
								   pending_items(ZCMD.arg1)) :
						   0)) < ZCMD.arg2)
				{
					logit(LOG_OBJ,
					      "P cmd: obj: %d to_obj: %d, chance: %d, limit %d(%d)",
					      obj_index[ZCMD.arg1].virtual_number,
					      (ZCMD.arg3 >= 0) ?
						      obj_index[ZCMD.arg3].virtual_number :
						      -2,
					      ZCMD.arg4, ZCMD.arg2, obj_index[ZCMD.arg1].number);
					if (!native_sql_reset)
						ZCMD.command = '!'; /* disable */
				}
				break;

			case 'G': /* obj_to_char */
				last_cmd = 0;
				obj_index[ZCMD.arg1].limit =
					ZCMD.arg2; // set the limit from zone file

				if ((ZCMD.arg1 >= 0) &&
				    (((static_cast<int64_t>(obj_index[ZCMD.arg1].number) +
				       (native_sql_reset ?
						static_cast<int64_t>(
							quest_mobile_native_birth_owner::
								pending_items(ZCMD.arg1)) :
						0)) < ZCMD.arg2) ||
				     force_item_repop))
				{
					if (!(obj = (native_sql_reset ?
							     quest_mobile_native_birth_owner::
								     prepare_item(ZCMD.arg1) :
							     read_object(ZCMD.arg1, REAL))))
					{
						if (!native_sql_reset)
							ZCMD.command = '!';
						logit(LOG_DEBUG,
						      "reset_zone(): (zone %d) obj %d [%d] not loadable",
						      zone, ZCMD.arg1,
						      obj_index[ZCMD.arg1].virtual_number);
					}
					if (obj)
					{
						if (IS_ARTIFACT(obj) &&
						    get_artifact_data_sql(
							    obj_index[ZCMD.arg1].virtual_number,
							    &artidata))
						{
							// If the artifact is owned, then it's timer is ticking somwhere, so we don't need to load another.
							if (artidata.owned)
							{
								if (native_sql_reset)
									quest_mobile_native_birth_owner::
										discard_item(obj);
								else
									extract_obj(obj);
								break;
							}
						}
						if (IS_ARTIFACT(obj) &&
						    (respawn == 0 ||
						     (respawn == 1 && force_item_repop != 2)))
						{
							if (native_sql_reset)
								quest_mobile_native_birth_owner::
									discard_item(obj);
							else
								extract_obj(obj);
							break;
						}
						ival = itemvalue(obj);
						// Load all shopkeeper eq.
						if (!ITEM_LOAD_CHECK(obj, ival, ZCMD.arg4) &&
						    (!mob || !IS_SHOPKEEPER(mob)))
						{
							if (native_sql_reset)
								quest_mobile_native_birth_owner::
									skipped_item(mob, obj);
							else
								enhance_on_npc_item_reset_skipped(
									mob, obj);
							if (native_sql_reset)
								quest_mobile_native_birth_owner::
									discard_item(obj);
							else
								extract_obj(obj);
							last_cmd = 1;
							break;
						}
						if (mob)
						{
							if (native_sql_reset)
								quest_mobile_native_birth_owner::
									carry(obj, mob);
							else
								obj_to_char(obj, mob);
							last_cmd = 1;
							break;
						}
						else
						{
							logit(LOG_MOB,
							      "G cmd: obj: %d  chance: %d, limit %d(%d) (no char)",
							      obj_index[ZCMD.arg1].virtual_number,
							      ZCMD.arg4, ZCMD.arg2,
							      obj_index[ZCMD.arg1].number);
							if (native_sql_reset)
								quest_mobile_native_birth_owner::
									discard_item(obj);
							else
								extract_obj(obj);
							break;
						}
					}
				}
				else if (ZCMD.arg1 < 0)
				{
					logit(LOG_OBJ, "G cmd, bad arg1.  disabling!");
					if (!native_sql_reset)
						ZCMD.command = '!';
				}
				else if ((static_cast<int64_t>(obj_index[ZCMD.arg1].number) +
					  (native_sql_reset ?
						   static_cast<int64_t>(
							   quest_mobile_native_birth_owner::
								   pending_items(ZCMD.arg1)) :
						   0)) < ZCMD.arg2)
				{
					logit(LOG_OBJ,
					      "G cmd: obj: %d to_char: %d, chance: %d, limit %d(%d)",
					      obj_index[ZCMD.arg1].virtual_number,
					      (ZCMD.arg3 >= 0) ?
						      mob_index[ZCMD.arg3].virtual_number :
						      -2,
					      ZCMD.arg4, ZCMD.arg2, obj_index[ZCMD.arg1].number);
					if (!native_sql_reset)
						ZCMD.command = '!'; /* disable */
				}
				break;

			case 'E': /* object to equipment list */
				last_cmd = 0;
				obj_index[ZCMD.arg1].limit =
					ZCMD.arg2; // set the limit from zone file

				if ((ZCMD.arg1 >= 0) &&
				    (((static_cast<int64_t>(obj_index[ZCMD.arg1].number) +
				       (native_sql_reset ?
						static_cast<int64_t>(
							quest_mobile_native_birth_owner::
								pending_items(ZCMD.arg1)) :
						0)) < ZCMD.arg2) ||
				     force_item_repop))
				{
					if (!(obj = (native_sql_reset ?
							     quest_mobile_native_birth_owner::
								     prepare_item(ZCMD.arg1) :
							     read_object(ZCMD.arg1, REAL))))
					{
						if (!native_sql_reset)
							ZCMD.command = '!';
						logit(LOG_DEBUG,
						      "reset_zone(): (zone %d) obj %d [%d] not loadable",
						      zone, ZCMD.arg1,
						      obj_index[ZCMD.arg1].virtual_number);
					}
					if (obj)
					{
						if (IS_ARTIFACT(obj) &&
						    get_artifact_data_sql(
							    obj_index[ZCMD.arg1].virtual_number,
							    &artidata))
						{
							// If the artifact is owned, then it's timer is ticking somwhere, so we don't need to load another.
							if (artidata.owned)
							{
								if (native_sql_reset)
									quest_mobile_native_birth_owner::
										discard_item(obj);
								else
									extract_obj(obj);
								break;
							}
						}
						if (IS_ARTIFACT(obj) &&
						    (respawn == 0 ||
						     (respawn == 1 && force_item_repop != 2)))
						{
							if (native_sql_reset)
								quest_mobile_native_birth_owner::
									discard_item(obj);
							else
								extract_obj(obj);
							break;
						}
						ival = itemvalue(obj);
						if (!ITEM_LOAD_CHECK(obj, ival, ZCMD.arg4))
						{
							if (native_sql_reset)
								quest_mobile_native_birth_owner::
									skipped_item(mob, obj);
							else
								enhance_on_npc_item_reset_skipped(
									mob, obj);
							if (native_sql_reset)
								quest_mobile_native_birth_owner::
									discard_item(obj);
							else
								extract_obj(obj);
							last_cmd = 1;
							break;
						}
						if (mob && (ZCMD.arg3 > 0) &&
						    (ZCMD.arg3 <= CUR_MAX_WEAR))
						{
							if (native_sql_reset)
							{
								quest_mobile_native_birth_owner::
									equip(obj, mob, ZCMD.arg3);
							}
							else
							{
								if (mob->equipment[ZCMD.arg3])
									obj_to_char(
										unequip_char(
											mob,
											ZCMD.arg3),
										mob);
								equip_char(mob, obj, ZCMD.arg3, 1);
							}
							last_cmd = 1;
							break;
						}
						else
						{
							logit(LOG_OBJ,
							      "E cmd: obj: %d pos: %d(%s) chance: %d, limit %d(%d)",
							      obj_index[ZCMD.arg1].virtual_number,
							      ZCMD.arg3,
							      ((ZCMD.arg3 > 0) &&
							       (ZCMD.arg3 <= CUR_MAX_WEAR)) ?
								      equipment_types[ZCMD.arg3] :
								      "ERR",
							      ZCMD.arg4, ZCMD.arg2,
							      obj_index[ZCMD.arg1].number);
							break;
						}
					}
				}
				else if ((static_cast<int64_t>(obj_index[ZCMD.arg1].number) +
					  (native_sql_reset ?
						   static_cast<int64_t>(
							   quest_mobile_native_birth_owner::
								   pending_items(ZCMD.arg1)) :
						   0)) < ZCMD.arg2)
				{
					logit(LOG_OBJ,
					      "E cmd: obj: %d pos: %d(%s) chance: %d, limit %d(%d)",
					      obj_index[ZCMD.arg1].virtual_number, ZCMD.arg3,
					      ((ZCMD.arg3 > 0) && (ZCMD.arg3 <= CUR_MAX_WEAR)) ?
						      equipment_types[ZCMD.arg3] :
						      "ERR",
					      ZCMD.arg4, ZCMD.arg2, obj_index[ZCMD.arg1].number);
					if (!native_sql_reset)
						ZCMD.command = '!'; /* disable */
				}
				break;

			case 'F': /* follow last mob M loaded */
				mob_index[ZCMD.arg1].limit =
					ZCMD.arg2; // set the limit from zone file

				if (mob_index[ZCMD.arg1].number < ZCMD.arg2)
				{
					if (ZCMD.arg4 > number(0, 99))
					{
						if (!(mob = read_mobile(ZCMD.arg1, REAL)))
						{
							ZCMD.command = '!';
							logit(LOG_DEBUG,
							      "reset_zone(): (zone %d) mob %d [%d] not loadable",
							      zone, ZCMD.arg1,
							      mob_index[ZCMD.arg1].virtual_number);
							last_cmd = last_mob_load = 0;
							break;
						}
					}
					else
					{
						last_mob_load = 0;
						mob = last_mob = 0;
						logit(LOG_MOB, "F cmd not executed %d %d %d %d",
						      ZCMD.arg1, ZCMD.arg2, ZCMD.arg3, ZCMD.arg4);
					}
					if (!last_mob)
					{
						last_cmd = last_mob_load = 0;
						last_mob_followable = 0;
						break;
					}
					tmp_mob = mob;
					GET_BIRTHPLACE(mob) = world[ZCMD.arg3].number;
					apply_zone_modifier(mob);
					char_to_room(mob, ZCMD.arg3, -2);
					npc_alchemist_world_spawn(mob);
					add_follower(mob, last_mob_followable);
					strcpy(buf, "group all");
					command_interpreter(last_mob, buf);
					if (!IS_SET(mob->specials.act, ACT_SENTINEL))
					{
						SET_BIT(mob->specials.act, ACT_SENTINEL);
					}
					last_cmd = last_mob_load = 1;
				}
				else
				{
					last_cmd = last_mob_load = 0;
				}
				break;

			case 'R': /* last mob loaded with M/F command will mount this */
				mob_index[ZCMD.arg1].limit =
					ZCMD.arg2; // set the limit from zone file
				if (mob_index[ZCMD.arg1].number < ZCMD.arg2)
				{
					if (ZCMD.arg4 > number(0, 99))
					{
						if (!(mob = read_mobile(ZCMD.arg1, REAL)))
						{
							ZCMD.command = '!';
							logit(LOG_DEBUG,
							      "reset_zone(): (zone %d) mob %d [%d] not loadable",
							      zone, ZCMD.arg1,
							      mob_index[ZCMD.arg1].virtual_number);
							last_cmd = last_mob_load = 0;
							break;
						}
					}
					else
					{
						mob = 0;
						last_mob_load = 0;
						logit(LOG_MOB, "R cmd not executed %d %d %d %d",
						      ZCMD.arg1, ZCMD.arg2, ZCMD.arg3, ZCMD.arg4);
					}
					if (!last_mob)
					{
						last_cmd = last_mob_load = 0;
						break;
					}
					GET_BIRTHPLACE(mob) = world[ZCMD.arg3].number;
					apply_zone_modifier(mob);
					char_to_room(mob, ZCMD.arg3, -2);
					npc_alchemist_world_spawn(mob);
					snprintf(buf, MAX_STRING_LENGTH, "%s",
						 FirstWord(GET_NAME(mob)));
					if (!IS_SET(mob->specials.act, ACT_SENTINEL))
						SET_BIT(mob->specials.act, ACT_SENTINEL);
					if (!IS_SET(mob->specials.act, ACT_MOUNT))
						SET_BIT(mob->specials.act, ACT_MOUNT);
					if (!IS_SET(mob->specials.act, ACT_ISNPC))
						SET_BIT(mob->specials.act, ACT_ISNPC);
					if (tmp_mob)
					{
						do_mount(tmp_mob, buf, 0);
						add_follower(mob, tmp_mob);
					}
					else
					{
						do_mount(last_mob, buf, 0);
						add_follower(mob, last_mob);
					}
					last_cmd = last_mob_load = 1;
				}
				else
					last_cmd = last_mob_load = 0;
				break;

			case 'D': /* set state of door */
				last_cmd = 0;
				if ((ZCMD.arg1 < 0) || !world[ZCMD.arg1].dir_option[ZCMD.arg2])
				{
					logit(LOG_DEBUG,
					      "D cmd: room: %d dir: %d state: %d' has error.",
					      (ZCMD.arg1 > 0) ? world[ZCMD.arg1].number : ZCMD.arg1,
					      ZCMD.arg2, ZCMD.arg3);
					ZCMD.command = '!'; /* disable */
					break;
				}
				switch (ZCMD.arg3 & 0x03)
				{
				case 0:
					REMOVE_BIT(
						world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
						EX_LOCKED);
					REMOVE_BIT(
						world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
						EX_CLOSED);
					break;
				case 1:
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
						EX_CLOSED);
					REMOVE_BIT(
						world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
						EX_LOCKED);
					break;
				case 2:
				case 3:
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
						EX_LOCKED);
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
						EX_CLOSED);
					break;
				}
				if (ZCMD.arg3 & 0x04)
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
						EX_SECRET);
				if (ZCMD.arg3 & 0x08)
					SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info,
						EX_BLOCKED);
				last_cmd = 1;
				break;

			case '!':
				/* command previously disabled because of error */
				break;
			default:
				logit(LOG_FILE,
				      "Undefd cmd in reset table; zone %d cmd #%d command %c.",
				      zone, cmd_no, ZCMD.command);
				logit(LOG_DEBUG,
				      "Undefd cmd in reset table; zone %d cmd #%d command %c.",
				      zone, cmd_no, ZCMD.command);
				ZCMD.command = '!';
				last_cmd = 0;
				break;
			}
		else
			last_cmd = 0;
	}

	if (native_sql_reset)
		quest_mobile_native_birth_owner::finish_reset();

	if (zone_table[zone].lifespan_min != zone_table[zone].lifespan_max)
		zone_table[zone].lifespan =
			number(zone_table[zone].lifespan_min, zone_table[zone].lifespan_max);
	else
		zone_table[zone].lifespan = zone_table[zone].lifespan_min;

	// Server-wide repop dial: a harder setting shortens every zone's lifespan.
	const double repop_dial = difficulty_multiplier(DIFFICULTY_ZONE_REPOP);
	if (repop_dial != 1.0)
		zone_table[zone].lifespan =
			MAX(1, difficulty_scale_int(zone_table[zone].lifespan, 1.0 / repop_dial));

	zone_table[zone].age = 0;
}

#undef ZCMD

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(int zone_nr)
{
	P_desc i;

	for (i = descriptor_list; i; i = i->next)
		if (!i->connected && (i->character->in_room != NOWHERE))
			if (world[i->character->in_room].zone == zone_nr)
				return (0);

	return (1);
}

/************************************************************************
 *  procs of a (more or less) general utility nature              *
 ********************************************************************** */
/* read and allocate space for a '~'-terminated string from a given file.
   Added &n to end of strings with ansi, to prevent 'bleeding'  JAB
*/
char *fread_string(FILE *fl)
{
	char buf[MAX_STRING_LENGTH], tmp[MAX_STRING_LENGTH], *rslt;
	char *point;
	int done = 0, length = 0, templength = 0;

	buf[0] = '\0';

	if (!fl)
	{
		fprintf(stderr, "fread_str: null file pointer!\n");
		return NULL;
	}
	do
	{
		if (!fgets(tmp, MAX_STRING_LENGTH - 5, fl))
		{
			perror("fread_string");
			logit(LOG_DEBUG, "%s", tmp);
			return NULL;
		}
		/* If there is a '~', END the string stop; else put an "\r\n" over
		   the '\n'. */

		templength = strlen(tmp);

		/* find the last non-whitespace char in tmp */
		for (point = tmp + templength - 1; (point > tmp) && isspace(*point); point--)
			;

		/* if its a tilde, we're done :) */
		if (*point == '~')
		{
			*point = '\0';
			templength = strlen(tmp);
			done = 1;
		}
		else
		{
			point = tmp + templength - 1;
			*(point++) = '\r';
			*(point++) = '\n';
			*point = '\0';
		}

		if (length + templength >= MAX_STRING_LENGTH)
		{
			logit(LOG_EXIT, "fread_string: string too large (db.c)");
			return NULL;
		}
		else
		{
			strcat(buf + length, tmp);
			length += strlen(tmp) /*templength */;
		}
	} while (!done);

	/* allocate space for the new string and copy it */
	if (strlen(buf) > 0)
	{
		/* make sure there is a &n at the end if ANSI codes are imbedded! */
		if (strstr(buf, "&+"))
			if (!((buf[strlen(buf) - 2] == '&') &&
			      (toupper(buf[strlen(buf) - 1]) == 'N')))
			{
				strcat(buf, "&n");
				length += 2;
			}
		CREATE(rslt, char, (unsigned)(length + 1), MEM_TAG_STRING);

		strcpy(rslt, buf);
	}
	else
		rslt = NULL;

	return rslt;
}

/*
 * advance file pointer past next '~' terminated string, this saves us an
 * alloc and free when we already HAVE that string in common storage.
 * read_object() and read_mobile() use this after the first call for each
 * obj/mob.  JAB
 */

void skip_fread(FILE *fl)
{
	char tmp[MAX_STRING_LENGTH];
	char *point;

	if (!fl)
	{
		fprintf(stderr, "skip_fread: null file pointer!\n");
		return;
	}
	for (;;)
	{
		if (!fgets(tmp, MAX_STRING_LENGTH - 1, fl))
		{
			perror("skip_fread");
			logit(LOG_DEBUG, "%s", tmp);
			return;
		}
		for (point = tmp + strlen(tmp) - 1; (point >= tmp) && isspace(*point); point--)
			;
		if (point >= tmp && *point == '~')
			return;
	}
}

/* release memory allocated for a char struct */
void free_char(P_char ch)
{
	struct affected_type *af, *tmp;
	//  struct trophy_data *tr1, *tr2;

	if (!ch)
	{
		logit(LOG_DEBUG, "free_char called with no char!");
		return;
	}
	unregister_character_runtime_id(ch);
	++character_removal_generation;
	character_maintenance_leave(ch);
	if ((GET_OPPONENT(ch)))
	{
		logit(LOG_EXIT, "free_char: called with a non-extracted char");
		// tmp = (struct affected_type *) (0 / 0);
		tmp = NULL;
	}

	// debug: check if free_char called on char with items (bug - should use extract_char)
	for (int i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
		{
			logit(LOG_DEBUG,
			      "[db.c:free_char] BUG: char '%s' has equipment[%d] vnum=%d still attached!",
			      GET_NAME(ch), i, OBJ_VNUM(ch->equipment[i]));
		}
	}
	if (ch->carrying)
	{
		logit(LOG_DEBUG, "[db.c:free_char] BUG: char '%s' still has carrying items!",
		      GET_NAME(ch));
	}

	if (ch->player.title)
		str_free(ch->player.title);

	for (af = ch->affected; af; af = tmp)
	{
		tmp = af->next;
		affect_remove(ch, af);
	}

	disarm_char_nevents(ch, NULL);

	if (IS_PC(ch) && ch->only.pc)
	{
		delete_knownShapes(ch);
		delete ch->only.pc->held_pets;
		ch->only.pc->held_pets = nullptr;
		delete ch->only.pc->zone_trophy;
		ch->only.pc->zone_trophy = nullptr;
	}

	//  if (IS_PC(ch))                /* clear trophy */
	//    for (tr1 = ch->only.pc->trophy; tr1; tr1 = tr2)
	//    {
	//      tr2 = tr1->next;
	//      mm_release(dead_trophy_pool, tr1);
	//    }
	if (IS_NPC(ch))
	{
		/* MOST mob strings should not be freed, as they are shared among
		   all mobs with the same Vnum.  Only if a string has been altered
		   inside the game, should it be freed here.  */

		if ((ch->only.npc->str_mask & STRUNG_KEYS) && ch->player.name)
			str_free(ch->player.name);

		if ((ch->only.npc->str_mask & STRUNG_DESC1) && ch->player.long_descr)
			str_free(ch->player.long_descr);

		if ((ch->only.npc->str_mask & STRUNG_DESC2) && ch->player.short_descr)
			str_free(ch->player.short_descr);

		if ((ch->only.npc->str_mask & STRUNG_DESC3) && ch->player.description)
			str_free(ch->player.description);
		if (ch->only.npc)
			FREE(ch->only.npc);
		ch->only.npc = NULL;
	}
	else
	{
		/* unlike for mobs, all player strings are unique, so they get
		   freed always (if they exist of course) */

		if (ch->only.pc->poofIn)
		{
			str_free(ch->only.pc->poofIn);
		}
		if (ch->only.pc->poofOut)
		{
			str_free(ch->only.pc->poofOut);
		}

		/* title freed earlier */

		if (ch->player.description)
		{
			str_free(ch->player.description);
		}

		if (ch->player.short_descr)
		{
			str_free(ch->player.short_descr);
		}

		// long_descr was the one player string this branch never released, so every
		// character load that set one leaked it - definitely lost, once per login.
		if (ch->player.long_descr)
		{
			str_free(ch->player.long_descr);
		}

		// must remove from room first or char_from_room logs with freed name
		if (ch->in_room != NOWHERE)
		{
			char_from_room(ch);
		}

		if (ch->player.name)
		{
			str_free(ch->player.name);
		}
		else
		{
			logit(LOG_DEBUG, "free_char called with no name. room: (%d)", ch->in_room);
		}

		if (ch->only.pc->log)
		{
			delete ch->only.pc->log;
			ch->only.pc->log = NULL;
		}

		mm_release(dead_pconly_pool, ch->only.pc);
		ch->only.pc = NULL;
	}
	SET_POS(ch, GET_POS(ch) + STAT_DEAD);
	add_event(release_mob_mem, 10 * WAIT_SEC, ch, 0, 0, 0, 0, 0);
	// release_mob_mem(ch);
	return;
}

/* release memory allocated for an obj struct */
void free_obj(P_obj obj)
{
	struct extra_descr_data *th, *next_one;
	struct obj_affect *af;

	if (!obj)
	{
		logit(LOG_DEBUG, "free_obj called with no obj!");
		return;
	}
	disarm_obj_nevents(obj, NULL);

	while ((af = obj->affects))
		obj_affect_remove(obj, af);

	/* MOST obj strings should not be freed, as they are shared among all
	   objects with the same Vnum.  Only if a string has been altered
	   inside the game, should it be freed here.  */

	if ((obj->str_mask & STRUNG_KEYS) && obj->name)
		str_free(obj->name);

	if ((obj->str_mask & STRUNG_DESC1) && obj->description)
		str_free(obj->description);

	if ((obj->str_mask & STRUNG_DESC2) && obj->short_description)
		str_free(obj->short_description);

	if ((obj->str_mask & STRUNG_DESC3) && obj->action_description)
		str_free(obj->action_description);

	// If the special function is barb (the mystical warhammer arti).
	if (obj->R_num >= 0 && obj_index[obj->R_num].func.obj == barb)
	{
		// Call the proc with the reset command for static variables.
		barb(obj, NULL, CMD_BARB_REMOVE, NULL);
	}

	obj->str_mask = 0;

	for (th = obj->ex_description; th; th = next_one)
	{
		next_one = th->next;
		if (th->keyword)
		{
			str_free(th->keyword);
			th->keyword = NULL;
		}
		else
			debug("extra description with null keyword for %s", obj->short_description);

		if (th->description)
		{
			str_free(th->description);
			th->description = NULL;
		}
		FREE(th);
	}

	obj->ex_description = NULL;
	release_obj_mem(obj);

	obj = NULL;
}

/*
 * read contents of a text file, and place in buf
 * Removed global array, this function now mallocs what it needs SAM 7-94
 */

char *file_to_string(const char *name)
{
	FILE *fl;
	char tmp[256], *ptr;
	char Gbuf1[MAX_STRING_LENGTH * 20];

	bzero(tmp, 256);
	bzero(Gbuf1, MAX_STRING_LENGTH * 20);

	if (!(fl = fopen(name, "r")))
	{
		if (!(fl = fopen(name, "w")))
		{
			snprintf(tmp, 256, "file-to-string (%s)", name);
			perror(tmp);
			return (NULL);
		}

		fclose(fl);

		if (!(fl = fopen(name, "r")))
		{
			snprintf(tmp, 256, "file-to-string (%s)", name);
			perror(tmp);
			return (NULL);
		}
	}

	/* End of file is this loop's exit condition, so a NULL return from fgets()
	   is expected and must not be treated as a missing required line. */
	while (fgets(tmp, 255, fl))
	{
		if (strlen(Gbuf1) + strlen(tmp) + 2 > MAX_STRING_LENGTH * 20)
		{
			logit(LOG_FILE, "file_to_string(): file (%s) too long.", name);
			fclose(fl);
			return (NULL);
		}
		strcat(Gbuf1, tmp);
		*(Gbuf1 + strlen(Gbuf1) + 1) = '\0';
		*(Gbuf1 + strlen(Gbuf1)) = '\r';
	}

	fclose(fl);
	CREATE(ptr, char, strlen(Gbuf1) + 1, MEM_TAG_STRING);

	strcpy(ptr, Gbuf1);

	return (ptr);
}

/* clear some of the the working variables of a char */

void reset_char(P_char ch)
{
	int i;

	ch->runtime_flags = 0;

	for (i = 0; i < MAX_WEAR; i++) /* Initialisering  */
		ch->equipment[i] = 0;

	ch->followers = 0;
	ch->following = 0;
	ch->carrying = 0;
#ifdef REALTIME_COMBAT
	ch->specials.combat = 0;
#else
	ch->specials.next_fighting = 0;
#endif
	GET_OPPONENT(ch) = 0;
	ch->specials.carry_weight = 0;
	ch->specials.carry_items = 0;
	if (IS_PC(ch))
		ch->only.pc->wiz_invis = 0;
	REMOVE_BIT(ch->specials.act2, PLR2_WAIT);

	// we store diff now, so leave at 0 (Lom)
	if (GET_HIT(ch) < 0)
	{
		GET_HIT(ch) = 0;
	}
	if (GET_VITALITY(ch) <= 0)
	{
		GET_VITALITY(ch) = 1;
	}
	if (GET_MANA(ch) <= 0)
	{
		GET_MANA(ch) = 1;
	}
}

/* clear ALL the working variables of a char & NOT free any space alloc'ed */
void clear_char(P_char ch)
{
	bzero(ch, sizeof(struct char_data));
	ch->runtime_id = allocate_character_runtime_id();

	ch->in_room = NOWHERE;
	ch->specials.was_in_room = NOWHERE;
	SET_POS(ch, POS_STANDING + STAT_NORMAL);
	ch->points.base_armor = 0; /* Basic Armor */

	/*
	 * Zero out our other flags -- this should have been done when
	 * we did 'bzero', but just in case
	 */

	ch->affected = NULL;

	ch->lobj = NULL;
}

/*
 * the reason to have real_room0(), real_mobile0(), and real_object0()
 * that mirrors the original functions is very simple.  These new
 * functions returns 0 instead of -1 for items not found in database. So
 * in spec_ass.c when it calls these functions it won't be assigning to a
 * -1 array index entry.  This causes some problem when zones are removed
 * or ids are reassigned. -DCL
 */

/* returns the real number of the zone with given virtual number */
int real_zone0(const int virt)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_zone_table;

	if (virt == -1)
		return 0;

	/*
	 * perform binary search on world-table
	 */
	for (;;)
	{
		mid = (bot + top) >> 1;

		if ((zone_table + mid)->number == virt)
			return (mid);
		if (bot >= top)
		{
#if defined(DB_NOTIFY) && DB_NOTIFY
			logit(LOG_DEBUG, "real_zone0: Zone %d not in database", virt);
#endif
			debug("real_zone0: Zone %d not in database", virt);
			return (0);
		}
		if ((zone_table + mid)->number > virt)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

/*
 * returns the real number of the zone with given virtual number
 */
int real_zone(const int virt)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_zone_table;
	if (virt == -1)
		return -1;

	/*
	 * perform binary search on world-table
	 */
	for (;;)
	{
		mid = (bot + top) >> 1;

		if ((zone_table + mid)->number == virt)
			return (mid);
		if (bot >= top)
		{
#if defined(DB_NOTIFY) && DB_NOTIFY
			logit(LOG_DEBUG, "real_zone: Zone %d not in database", virt);
#endif
			return (-1);
		}
		if ((zone_table + mid)->number > virt)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

/* returns the real number of the room with given virtual number */
int real_room0(const int virt)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_world;

	if (virt < 0)
		return 0;

	/*
	 * perform binary search on world-table
	 */
	for (;;)
	{
		mid = (bot + top) >> 1;

		if ((world + mid)->number == virt)
			return (mid);
		if (bot >= top)
		{
#if defined(DB_NOTIFY) && DB_NOTIFY
			logit(LOG_DEBUG, "real_room0: Room %d not in database", virt);
#endif
			return (0);
		}
		if ((world + mid)->number > virt)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

/*
 * returns the real number of the room with given virtual number
 */

int real_room(const int virt)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_world;
	if (virt < 0)
		return NOWHERE;

	/*
	 * perform binary search on world-table
	 */
	for (;;)
	{
		mid = (bot + top) >> 1;

		if ((world + mid)->number == virt)
			return (mid);
		if (bot >= top)
		{
#if defined(DB_NOTIFY) && DB_NOTIFY
			logit(LOG_DEBUG, "real_room: Room %d not in database", virt);
#endif
			return NOWHERE;
		}
		if ((world + mid)->number > virt)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

/*
 * returns the real number of the monster with given virtual number
 */

int real_mobile0(const int virt)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_mobt;

	/*
	 * perform binary search on mob-table
	 */
	for (;;)
	{
		mid = (bot + top) >> 1;

		if ((mob_index + mid)->virtual_number == virt)
			return (mid);
		if (bot >= top)
		{
#if defined(DB_NOTIFY) && DB_NOTIFY
			logit(LOG_DEBUG, "real_mobile0: Mob %d not in database", virt);
#endif
			return (0);
		}
		if ((mob_index + mid)->virtual_number > virt)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

/*
 * returns the real number of the monster with given virtual number
 */

int real_mobile(const int virt)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_mobt;

	/*
	 * perform binary search on mob-table
	 */
	for (;;)
	{
		mid = (bot + top) >> 1;

		if ((mob_index + mid)->virtual_number == virt)
			return (mid);
		if (bot >= top)
		{
#if defined(DB_NOTIFY) && DB_NOTIFY
			logit(LOG_DEBUG, "real_mobile: Mob %d not in database", virt);
#endif
			return (-1);
		}
		if ((mob_index + mid)->virtual_number > virt)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

/*
 * returns the real number of the object with given virtual number
 */

int real_object0(const int virt)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_objt;

	/*
	 * perform binary search on obj-table
	 */
	for (;;)
	{
		mid = (bot + top) >> 1;

		if ((obj_index + mid)->virtual_number == virt)
			return (mid);
		if (bot >= top)
		{
#if defined(DB_NOTIFY) && DB_NOTIFY
			logit(LOG_DEBUG, "real_object0: Obj %d not in database", virt);
#endif
			return (0);
		}
		if ((obj_index + mid)->virtual_number > virt)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}

/*
 * returns the real number of the object with given virtual number
 */

int real_object(const int virt)
{
	int bot, top, mid;

	bot = 0;
	top = top_of_objt;

	/*
	 * perform binary search on obj-table
	 */
	for (;;)
	{
		mid = (bot + top) >> 1;

		if ((obj_index + mid)->virtual_number == virt)
			return (mid);
		if (bot >= top)
		{
#if defined(DB_NOTIFY) && DB_NOTIFY
			logit(LOG_DEBUG, "real_object: Obj %d not in database", virt);
#endif
			return (-1);
		}
		if ((obj_index + mid)->virtual_number > virt)
			top = mid - 1;
		else
			bot = mid + 1;
	}
}
void worldcheck(P_char ch)
{
	int i;
	char tmp_buf[MAX_STRING_LENGTH];

	for (i = 1; i < top_of_world; i++)
	{
		if (world[i].number <= world[i - 1].number)
		{
			snprintf(
				tmp_buf, MAX_STRING_LENGTH,
				"Real: %d Virtual: %d is out of order with Real: %d Virtual: %d\r\n",
				i, world[i].number, i - 1, world[i - 1].number);
			send_to_char(tmp_buf, ch);
		}
	}
}

int InsertIntoFile(const char *filename, const char *text)
{
	FILE *fin = 0;
	unsigned char *buffer = 0;
	long sizeOfFile = 0;
	unsigned int sizeToRead = 0;
	FILE *fout = 0;

	// open the existing bug file for read
	fin = fopen(filename, "rb");
	if (fin != NULL)
	{
		// create the buffer to hold the file's current contents
		CREATE(buffer, unsigned char, MAX_STRING_LENGTH, MEM_TAG_BUFFER);
		if (buffer == NULL)
		{
			fclose(fin);
			return 1;
		}

		// determine the size of the file
		fseek(fin, 0, SEEK_END);
		sizeOfFile = ftell(fin);
		fseek(fin, 0, SEEK_SET);

		// determine if the file is larger than the max size
		sizeToRead = 0;
		if (sizeOfFile <= MAX_STRING_LENGTH)
			sizeToRead = (int)sizeOfFile;
		else
			sizeToRead = MAX_STRING_LENGTH;

		// fread reports zero elements for a zero-sized element, which the required-read
		// wrapper treats as fatal.  An empty report file has nothing to preserve.
		if (sizeToRead)
			REQUIRED_FREAD(buffer, sizeToRead, 1, fin);

		// close the input file
		fclose(fin);
	}

	// open the file again to be rewritten
	fout = fopen(filename, "wb");
	if (fout == NULL)
	{
		FREE(buffer);
		return 2;
	}

	// write the formatted text out to the file
	fputs(text, fout);

	// write out the existing file's contents
	if (buffer != NULL)
	{
		fwrite(buffer, sizeToRead, 1, fout);
		FREE(buffer);
	}

	fclose(fout);

	return 0;
}

#if 0
void load_obj_limits()
{
  FILE    *f;
  int      vnum, max, rnum, rnum;

  f = fopen("obj_limits", "r");
  if (!f)
  {
    save_obj_limits();
    return;
  }
  while (!feof(f))
  {
    REQUIRED_FGETS(buf, sizeof(buf) - 1, f);
    if (sscanf(buf, "%d %d %d", &vnum, &max, &rented) == 3)
    {
      rnum = real_object(vnum);
      if (rnum != -1)
      {
        obj_index[rnum].max = max;
        obj_index[rnum].rnumber = rented;
      }
    }
  }
  fclose(f);
}

void save_obj_limits()
{
  FILE    *f;
  int      i;

  f = fopen("obj_limits", "w");
  for (i = 0; i <= top_of_objt; i++)
  {
    fprintf(f, "%d %d %d\n", obj_index[i].virtual_number,
            obj_index[i].max, obj_index[i].rnumber);
  }
  fclose(f);
}

#endif
