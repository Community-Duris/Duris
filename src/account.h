// account.h
//

#ifndef DURIS_ACCOUNT_H
#define DURIS_ACCOUNT_H

#ifndef _DE_
# include "structs.h"
#endif

#define USE_ACCOUNT

#define ACCT_IMMORTAL 0
#define ACCT_GOOD     1
#define ACCT_EVIL     2

#define MAX_CHARS_PER_ACCOUNT 16

struct acct_ip
{ // Account IP Information
	char             *hostname;
	char             *ip_address;
	unsigned long int count;
	struct acct_ip   *next;
};

struct acct_chars
{ // Account Character Entry
	char              *charname;
	unsigned long int  count;
	long int           last;
	char               blocked;
	char               racewar;
	int                level;
	int                race;
	unsigned int       m_class;
	unsigned int       secondary_class;
	int                last_room;
	long               last_save;
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

	struct acct_ip    *acct_unique_ips;
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

	struct acct_entry *next;
};

struct acct_list_entry
{ // List of loaded accounts
	struct acct_entry      *account;
	struct acct_list_entry *next;
};

void cleanup_temp_char(struct char_data *ch);

#endif // DURIS_ACCOUNT_H
