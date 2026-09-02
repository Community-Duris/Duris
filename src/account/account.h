// account.h
//

#ifndef DURIS_ACCOUNT_H
#define DURIS_ACCOUNT_H

#include <stdint.h>

#ifndef _DE_
#include "core/structs.h"
#endif

#define USE_ACCOUNT

#define ACCT_IMMORTAL 0
#define ACCT_GOOD 1
#define ACCT_EVIL 2

/* Persisted before account deletion crosses its cancellation boundary. */
#define ACCOUNT_BLOCK_DELETION 2

#define MAX_CHARS_PER_ACCOUNT 16

struct acct_ip
{ // Account IP Information
	char *hostname;
	char *ip_address;
	unsigned long int count;
	struct acct_ip *next;
};

struct acct_chars
{ // Account Character Entry
	int pid;
	char *charname;
	unsigned long int count;
	long int last;
	char blocked;
	char racewar;
	int level;
	int race;
	unsigned int m_class;
	unsigned int secondary_class;
	int last_room;
	long last_save;
	struct acct_chars *next;
};

struct acct_entry
{ // Main Account Structure
	char *acct_name;
	char *acct_email;
	char *acct_password;
	char *acct_confirmation;

	int num_ips;
	int num_chars;

	struct acct_ip *acct_unique_ips;
	struct acct_chars *acct_character_list;

	char acct_blocked;
	char acct_confirmed;
	char acct_confirmation_sent;

	long int acct_last;
	long int acct_good;
	long int acct_evil;

	unsigned long int acct_flags1;
	unsigned long int acct_flags2;
	unsigned long int acct_flags3;
	unsigned long int acct_flags4;
	uint64_t persistence_revision;

	struct acct_entry *next;
};

struct acct_list_entry
{ // List of loaded accounts
	struct acct_entry *account;
	struct acct_list_entry *next;
};

void cleanup_temp_char(struct char_data *ch);
bool account_exists(const char *dir, char *name);
int is_valid_email(const char *email);
bool is_email_taken(const char *email);

#endif // DURIS_ACCOUNT_H
