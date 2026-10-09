/*
 *  utility.h
 *  Duris
 *
 *  Created by Torgal on 1/29/10.
 *
 */

#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "core/structs.h"

int GET_LVL_FOR_SKILL(P_char ch, int skill);
bool is_ansi_char(char collor_char);

void connect_rooms(int, int, int, int);
void connect_rooms(int, int, int);

void disconnect_exit(int v1, int dir);
void disconnect_rooms(int v1, int v2);

P_char get_char_online(char *name, bool include_linkdead = TRUE);

void logit(const char *, const char *, ...) __attribute__((format(printf, 2, 3)));

int cmd_from_dir(int dir);
int direction_tag(P_char ch);
bool opposite_racewar(P_char ch, P_char victim);

const char *condition_str(P_char ch);

string pad_ansi(const char *str, int length, bool trim_to_length = FALSE);
void trim_and_end_colorless(char *orig, char *good, int length);

P_char get_player_from_name(char *name);
int get_player_pid_from_name(char *name);
char *get_player_name_from_pid(int pid);

bool sub_string(const char *, const char *);
bool sub_string_cs(const char *, const char *);
bool sub_string_set(const char *, const char **);

char *coin_stringv(int amount, int padfront = 0);
char *coins_to_string(int platinum, int gold, int silver, int copper, const char *color_string);

int yes_no(const char *);

#include <stdarg.h>
// Owning exact original variadic formatting request, before malloc. Caller owns
// already-live prefixes/suffixes/format/va_list and prior output in outer; retains
// admitted peak through return and transferred request until free. Outputs strong
// on refusal; pinned GCC13/C++11 ABI; libc formatting internals use existing policy.
// No output delivery, log write, scheduler or accounting authority.
bool diagnostic_format_variadic_message_bounded(const char *, const char *, const char *, va_list &,
						char **, bool (*)(size_t, void *) noexcept, void *,
						size_t outer_live,
						size_t *retained_payload_bytes = nullptr) noexcept;
#endif // _UTILITY_H_
