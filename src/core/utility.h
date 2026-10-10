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

// Full original diagnostic formatting/fanout/logging companions. Debug outer
// includes ALL-current registered queue/pager observer once; refresh its actual
// retained allowance on EVERY return/drain/disconnect. False after a prior send
// may retain actual output. Native caller MUST record returned effect first.
// Logit keeps original timestamp/counter/open/mkdir/fallback/rewind/stderr order.
// Original funcs unchanged; no general output policy, source or activation authority.
bool diagnostic_debug_bounded(bool (*)(size_t, void *) noexcept, void *, size_t, const char *,
			      ...) noexcept __attribute__((format(printf, 4, 5)));
bool diagnostic_logit_bounded(bool (*)(size_t, void *) noexcept, void *, size_t, const char *,
			      const char *, ...) noexcept __attribute__((format(printf, 5, 6)));
// Allocation-free owning profile for ORIGINAL logit, including original body
// malloc, timestamp/va_list, parent-directory path and one-hop freed-body fallback.
// Timestamp payload uses the ORIGINAL terminated tbuf fixed-buffer bound, valid
// across clock changes. No logging/cache mutation. Strong output/unsupported refusal.
// Caller preadmits preflight_object_bytes BEFORE this helper, owns input strings
// and outer storage, and retains admitted original working peak through logit.
size_t diagnostic_original_logit_preflight_object_bytes() noexcept;
bool diagnostic_original_logit_working_bytes(const char *, size_t *, const char *, ...) noexcept
	__attribute__((format(printf, 3, 4)));

// Full original level/WIZLOG selection, variadic formatting and fanout order.
// outer contains genuine G INCLUDING all current registered output once; child
// output census replaces its own allowance, relay replaces only foreign G.
// returned records original completion (including null formatter outcome).
// False before completion can retain actual earlier output; never retry it.
bool diagnostic_wizlog_bounded(int, bool *, bool (*)(size_t *, void *) noexcept,
			       bool (*)(size_t, void *) noexcept, void *, size_t, const char *,
			       ...) noexcept __attribute__((format(printf, 7, 8)));

#endif // _UTILITY_H_
