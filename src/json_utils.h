/*
 * json_utils.h - json helpers for websocket stuff
 *
 * builds and parses json messages for the web client.
 * uses cjson library (https://github.com/DaveGamble/cJSON)
 *
 * to install cjson:
 *   fedora/rhel: dnf install cjson-devel
 *   ubuntu/debian: apt install libcjson-dev
 */

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "structs.h"
#include <cjson/cJSON.h>

/* forward declarations */
struct ShipData;

/* message types */
#define MSG_TYPE_CMD "cmd"
#define MSG_TYPE_AUTH "auth"
#define MSG_TYPE_CHARGEN "chargen"
#define MSG_TYPE_GMCP "gmcp"
#define MSG_TYPE_TEXT "text"

/* websocket commands (prefixed to avoid collision with interp.h) */
#define WS_CMD_LOGIN "login"
#define WS_CMD_REGISTER "register"
#define WS_CMD_CHARGEN_OPTIONS "chargen_options"
#define WS_CMD_ROLL_STATS "roll_stats"
#define WS_CMD_CREATE_CHAR "create_character"
#define WS_CMD_ENTER "enter"
#define WS_CMD_GAME "game"

/* auth status */
#define AUTH_SUCCESS "success"
#define AUTH_REGISTERED "registered"
#define AUTH_FAILED "failed"

/* Unstructured game output currently has one category. Structured combat,
 * movement and channel state is carried by GMCP packages. */
#define TEXT_INFO "info"

/* gmcp package names (mudlet/ire compatible) */
#define GMCP_ROOM_INFO "Room.Info"
#define GMCP_CHAR_VITALS "Char.Vitals"
#define GMCP_CHAR_STATUS "Char.Status"
#define GMCP_CHAR_AFFECTS "Char.Affects"
#define GMCP_COMBAT_UPDATE "Combat.Update"
#define GMCP_COMM_CHANNEL "Comm.Channel"

/* parsing functions */

/* parse incoming json command, returns cjson object or null on error */
cJSON *json_parse_command(const char *json_str);

/* get command type from parsed json */
const char *json_get_cmd_type(cJSON *json);

/* get command name from parsed json */
const char *json_get_cmd_name(cJSON *json);

/* get data object from parsed json */
cJSON *json_get_data(cJSON *json);

/* get string from data object */
const char *json_get_string(cJSON *data, const char *key);

/* get integer from data object */
int json_get_int(cJSON *data, const char *key, int default_val);

/* response building - all return malloc'd strings, caller must free */

/* build auth success response with character list */
char *json_build_auth_success(const char *account, struct char_data *char_list);

/* build auth failure response */
char *json_build_auth_failed(const char *error);

/* build stat roll response */
char *json_build_stat_roll(int *rolls, int *racial_mods, int *final, int rerolls);

/* build chargen complete response */
char *json_build_chargen_complete(const char *name, const char *message);

/* gmcp building - all return malloc'd strings, caller must free */

/* build room.info gmcp message (ch is for visibility checks, can be null) */
char *json_build_room_info(struct room_data *room, struct char_data *ch);

/* build char.vitals gmcp message */
char *json_build_char_vitals(struct char_data *ch);

/* build char.status gmcp message */
char *json_build_char_status(struct char_data *ch);

/* build char.affects gmcp message */
char *json_build_char_affects(struct char_data *ch);

/* build combat.update gmcp message */
char *json_build_combat_update(struct char_data *ch, struct char_data *victim, int damage,
			       const char *damage_type, int critical);

/* build comm.channel gmcp message */
char *json_build_comm_channel(const char *channel, const char *sender, const char *text);

/* build comm.channel gmcp message with alignment (for nchat) */
char *json_build_comm_channel_ex(const char *channel, const char *sender, const char *text,
				 const char *alignment);

/* build quest.status gmcp message */
char *json_build_quest_status(struct char_data *ch);

/* build ship.info gmcp message */
char *json_build_ship_info(struct ShipData *ship, struct char_data *ch);

/* text message building - returns malloc'd string, caller must free */
char *json_build_text(const char *category, const char *text);

/* build complete gmcp json wrapper - returns malloc'd string, caller must free */
char *json_build_gmcp_message(const char *package, const char *data_json);

/* utility functions - all return malloc'd strings, caller must free */

/* escape string for json (handles quotes, newlines, utf-8, etc) */
char *json_escape_string(const char *str);

/* strip ansi codes and escape for json */
char *json_escape_ansi_string(const char *str);

/* free json string (convenience wrapper) */
void json_free_string(char *str);

#endif /* JSON_UTILS_H */
