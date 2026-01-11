/*
 * poll.h - poll system for durismud
 *
 * allows immortals to create polls, mortals to vote
 * votes tracked per account (not per character)
 */

#ifndef DURIS_POLL_H
#define DURIS_POLL_H

#include "structs.h"
#include <vector>
#include <string>

#define MAX_POLL_OPTIONS    10
#define MAX_POLL_QUESTION   512
#define MAX_OPTION_TEXT     256
#define MINIMUM_POLL_LEVEL  30

/* wizard states */
#define POLL_WIZ_NONE       0
#define POLL_WIZ_QUESTION   1
#define POLL_WIZ_MULTI      2
#define POLL_WIZ_MAX_CHOICE 3
#define POLL_WIZ_DURATION   4
#define POLL_WIZ_OPTIONS    5
#define POLL_WIZ_CONFIRM    6

struct poll_option {
  int id;
  int option_num;
  std::string text;
  int vote_count;
};

struct poll_data {
  int id;
  std::string question;
  std::string created_by;
  time_t created_at;
  time_t expires_at;
  bool is_active;
  bool multi_select;
  int max_choices;
  std::vector<poll_option> options;
  int total_votes;
};

struct poll_wizard_data {
  int state;
  poll_data poll;
  int current_option;
};

/* command */
void do_poll(P_char ch, char *argument, int cmd);

/* sql */
bool poll_create(poll_data *poll);
bool poll_close(int poll_id, P_char ch);
bool poll_has_voted(const char *account_name, int poll_id);
int poll_cast_vote(P_char ch, int poll_id, std::vector<int> &choices);
int poll_record_votes(const char *acct_name, const char *char_name, int poll_id, poll_data &poll, std::vector<int> &choices);
std::vector<poll_data> poll_get_all(bool active_only);
poll_data poll_get_by_id(int poll_id);
void poll_check_expirations(void);

/* display */
void poll_display_list(P_char ch, bool show_all);
void poll_display_single(P_char ch, int poll_id);
void poll_display_results(P_char ch, int poll_id);

/* wizard */
void poll_wizard_start(P_char ch);
void poll_wizard_handle_input(P_char ch, char *input);
void poll_wizard_cancel(P_char ch);
bool poll_wizard_active(P_char ch);

/* broadcasts */
void poll_broadcast_new(int poll_id, const char *question, const char *creator);
void poll_broadcast_vote(int poll_id, int total_votes);
void poll_broadcast_close(int poll_id, const char *question);

#endif /* DURIS_POLL_H */
