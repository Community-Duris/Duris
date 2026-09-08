/*************************************************************
 * account.c
 *************************************************************/

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "world/db.h"
#include "cmd/interp.h"
#include "item/item_movement_transaction.h"
#include "item/item_ownership_runtime.h"
#include "core/utils.h"
#include "account/account.h"
#include "account/password_hash.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include "world/graph.h"
#include "core/mm.h"
#include "item/objmisc.h"
#include "magic/spells.h"
#include "sql/sql_player.h"
#include "player/player_name.h"
#include "player/player_load_materialize.h"
#include "player/player_load_pipeline.h"
#include "player/player_revision_state.h"
#include "player/player_save_pipeline.h"
#include "persistence/critical_command_coordinator.h"
#include "persistence/critical_outbox.h"
#include "persistence/locker_async.h"
#include "persistence/maintenance_scheduler.h"
#include "persistence/persistence_observability.h"
#include "flatfile/flatfile_account_adapter.h"
#include "flatfile/flatfile_account_delete.h"
#include "persistence/persistence_mode.h"
#include "ships/ships.h"
#include "guild/assocs.h"
#include "net/ws_handlers.h"
#include "redis/redis_ship_legacy.h"

#include <unordered_map>
#include <new>
#include <string>
#include <utility>
#include <vector>

// External Stuff
extern P_index obj_index;
extern P_obj object_list;
extern P_room world;
extern const int top_of_world;
extern struct time_info_data time_info;
extern const char *dirs[];
extern P_desc descriptor_list;
extern P_char character_list;
extern struct mm_ds *dead_mob_pool;
extern struct mm_ds *dead_pconly_pool;

struct acct_entry *account_list = NULL;

namespace
{
std::unordered_map<uint64_t, player_load_result> ready_player_loads;

struct account_deletion_identity
{
	int pid = 0;
	std::string name;
};

bool account_password_matches(P_acct account, char *password)
{
	if (!account || !account->acct_password || !password)
		return false;
	if (is_bcrypt_hash(account->acct_password))
		return bcrypt_verify_password(password, account->acct_password) != 0;
	return !strcmp(CRYPT2(password, account->acct_password), account->acct_password);
}

void display_account_deletion_confirmation(P_desc d, bool retry)
{
	if (!d || !d->account || !d->account->acct_name)
		return;
	char prompt[1024];
	checked_snprintf(
		prompt, sizeof(prompt),
		retry ? "\r\n&+RAccount deletion is already in progress.&n\r\n"
			"Type the exact account name &+W%s&n to retry completion: " :
			"\r\n&+R!!! PERMANENT ACCOUNT DELETION !!!&n\r\n"
			"Every character and all live account data will be permanently deleted.\r\n"
			"This cannot be cancelled after the deletion fence is written.\r\n\r\n"
			"Type the exact account name &+W%s&n to continue, or &+WCANCEL&n: ",
		d->account->acct_name);
	SEND_TO_Q(prompt, d);
	d->prompt_mode = TRUE;
}

bool capture_account_deletion_identities(P_desc d,
					 std::vector<account_deletion_identity> *identities)
{
	if (!d || !d->account || !d->account->acct_name || !identities)
		return false;
	identities->clear();
	auto append = [&](int pid, const char *name)
	{
		if (pid <= 0 || !name || !name[0])
			return false;
		for (const auto &identity : *identities)
			if (identity.pid == pid)
				return !strcasecmp(identity.name.c_str(), name);
		try
		{
			identities->push_back({ pid, name });
			return true;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	};
	for (struct acct_chars *character = d->account->acct_character_list; character;
	     character = character->next)
		if (!append(character->pid, character->charname))
			return false;
	for (P_desc other = descriptor_list; other; other = other->next)
		if (other->account && other->account->acct_name && other->character &&
		    !strcasecmp(other->account->acct_name, d->account->acct_name) &&
		    !append(GET_PID(other->character), GET_NAME(other->character)))
			return false;
	return true;
}

bool account_deletion_locker_runtime_active(
	const std::string &account_name, const std::vector<account_deletion_identity> &identities)
{
	std::string account_prefix = "account." + account_name + ".";
	for (P_char character = character_list; character; character = character->next)
	{
		const char *name = GET_NAME(character);
		if (!name)
			continue;
		if (!strncasecmp(name, account_prefix.c_str(), account_prefix.size()) &&
		    strstr(name + account_prefix.size(), ".locker"))
			return true;
		for (const auto &identity : identities)
		{
			std::string locker_name = identity.name + ".locker";
			if (!strcasecmp(name, locker_name.c_str()))
				return true;
		}
	}
	return false;
}

void close_other_account_sessions(P_desc d)
{
	if (!d || !d->account || !d->account->acct_name)
		return;
	for (P_desc other = descriptor_list, next = NULL; other; other = next)
	{
		next = other->next;
		if (other != d && other->account && other->account->acct_name &&
		    !strcasecmp(other->account->acct_name, d->account->acct_name))
		{
			SEND_TO_Q("\r\nYour account is being permanently deleted.\r\n", other);
			close_socket(other);
		}
	}
}

class account_deletion_drain_guard
{
    public:
	account_deletion_drain_guard()
	{
		maintenance_scheduler_quiesce();
		critical_command_coordinator_quiesce();
		critical_outbox_quiesce();
	}

	~account_deletion_drain_guard()
	{
		if (player_quiesced_)
			player_save_pipeline_resume();
		critical_outbox_resume();
		critical_command_coordinator_resume();
		maintenance_scheduler_resume();
	}

	bool drain()
	{
		if (!maintenance_scheduler_drain(3000) ||
		    !critical_command_coordinator_drain(3000) || !critical_outbox_drain(3000) ||
		    !persistence_flush_all_character_saves())
			return false;
		player_save_pipeline_quiesce();
		player_quiesced_ = true;
		return player_save_pipeline_drain(3000) && locker_async_drain(3000);
	}

    private:
	bool player_quiesced_ = false;
};

void remove_deleted_account_runtime(P_desc deleting_session,
				    const std::vector<account_deletion_identity> &identities)
{
	for (const auto &identity : identities)
	{
		item_ownership_runtime_forget_player_domain(identity.pid);
		player_revision_forget(identity.pid);
		redis_invalidate_ship_snapshot(identity.name.c_str());
		forget_deleted_guild_member(identity.name.c_str());
		delete_ship_runtime(identity.name.c_str());
	}
	for (P_char character = character_list, next = NULL; character; character = next)
	{
		next = character->next;
		if (IS_NPC(character))
			continue;
		for (const auto &identity : identities)
			if (GET_PID(character) == identity.pid)
			{
				if (character->desc)
				{
					P_desc character_desc = character->desc;
					character->desc = NULL;
					character_desc->character = NULL;
					if (character_desc != deleting_session)
						close_socket(character_desc);
				}
				extract_char_after_terminal_save(character);
				break;
			}
	}
}

void display_account_login_pages(P_desc d)
{
	SEND_TO_Q("\r\n&+YNews:&n Type 'news' in game to read the latest updates.\r\n", d);
	SEND_TO_Q(motd.c_str(), d);
	SEND_TO_Q("\r\n*** PRESS RETURN: ", d);
}
} // namespace

#define ACCT_SERIAL 1
#define ACCOUNT_EMAIL_DB "Accounts/email.db"

bool account_exists(const char *, char *);
void check_rested_bonus(P_desc);

// Email validation function with strict RFC compliance
// Returns 1 if valid, 0 if invalid
int is_valid_email(const char *email)
{
	const char *p;
	int at_count = 0, dot_after_at = 0;
	int local_len = 0, domain_len = 0;
	char prev_char = '\0';

	if (!email || !*email)
		return 0;

	// Find the @ symbol and validate local part
	for (p = email; *p && *p != '@'; p++)
	{
		local_len++;

		// Check for valid characters in local part
		if (!isalnum(*p) && *p != '.' && *p != '-' && *p != '_' && *p != '+')
			return 0;

		// Can't start with a dot
		if (local_len == 1 && *p == '.')
			return 0;

		// Can't have consecutive dots
		if (*p == '.' && prev_char == '.')
			return 0;

		prev_char = *p;
	}

	// Local part must exist and can't end with dot
	if (local_len == 0 || local_len > 64 || prev_char == '.')
		return 0;

	// Must have exactly one @
	if (*p != '@')
		return 0;

	p++; // Skip the @
	at_count = 0;
	prev_char = '\0';

	// Validate domain part
	for (; *p; p++)
	{
		domain_len++;

		// Check for valid characters in domain
		if (!isalnum(*p) && *p != '.' && *p != '-')
			return 0;

		// Can't start with dot or hyphen
		if (domain_len == 1 && (*p == '.' || *p == '-'))
			return 0;

		// Can't have consecutive dots
		if (*p == '.' && prev_char == '.')
			return 0;

		// Track if we have a dot after @
		if (*p == '.')
			dot_after_at = 1;

		prev_char = *p;
	}

	// Domain must exist, have at least one dot, and can't end with dot or hyphen
	if (domain_len == 0 || domain_len > 253 || !dot_after_at || prev_char == '.' ||
	    prev_char == '-')
		return 0;

	// Check for second @ symbol (invalid)
	for (p = email; *p; p++)
	{
		if (*p == '@')
		{
			at_count++;
			if (at_count > 1)
				return 0;
		}
	}

	return 1;
}

bool is_email_taken([[maybe_unused]] const char *email)
{
#ifdef REQUIRE_EMAIL_VERIFICATION
	FILE *f = NULL;
	char db_name[MAX_INPUT_LENGTH];

	f = fopen(ACCOUNT_EMAIL_DB, "r");
	if (!f)
	{
		statuslog(56, "Couldn't open Email DB!");
		return TRUE;
	}

	while (fscanf(f, "%s\n", &db_name) != 0)
	{
		if (!strcmp(email, db_name))
		{
			return TRUE;
		}
	}
#endif

	return FALSE;
}

void select_accountname(P_desc d, char *arg)
{
	char tmp_name[MAX_INPUT_LENGTH];

	for (; isspace(*arg); arg++)
		;
	if (!*arg)
	{
		close_socket(d);
		return;
	}

	if (_parse_name(arg, tmp_name))
	{
		SEND_TO_Q("Illegal account name, please try another.\r\n", d);
		SEND_TO_Q("Account Name: ", d);
		return;
	}

	*tmp_name = toupper(*tmp_name);

	if (!d->account)
	{
		d->account = allocate_account();
		if (!d->account)
		{
			SEND_TO_Q(
				"ERROR:  Could not allocate a new account, notify an immortal!\r\n",
				d);
			statuslog(56, "&+RALERT&n:  Could not allocate memory for a new account!");
			STATE(d) = CON_FLUSH;
			return;
		}
	}

	d->account->acct_name = str_dup(tmp_name);

	if (account_exists("Accounts", tmp_name))
	{
		if (read_account(d->account) == -1)
		{
			SEND_TO_Q(
				"There is an error with your account, please notify an immortal!\r\n",
				d);
			statuslog(56, "&+RALERT&n:  Account corrupt: %s", tmp_name);
			d->account = free_account(d->account);
			STATE(d) = CON_FLUSH;
			return;
		}

		SEND_TO_Q("Please enter your password: ", d);
		echo_off(d);
		STATE(d) = CON_GET_ACCT_PASSWD;
		return;
	}

	verify_account_name(d, NULL);
	STATE(d) = CON_VERIFY_NEW_ACCT_NAME;
	return;
}

void get_account_password(P_desc d, char *arg)
{
	if (!arg)
	{
		d->account = free_account(d->account);
		close_socket(d);
		return;
	}

	// skip whitespace
	for (; isspace(*arg); arg++)
		;

	if (*arg == -1)
	{
		if (arg[1] != '0' && arg[2] != '0')
		{
			if (arg[3] == '0')
			{ /* Password on next read  */
				return;
			}
			else
			{ /* Password available */
				arg = arg + 3;
			}
		}
		else
			d->account = free_account(d->account);
		close_socket(d);
		return;
	}

	// Check password - support both bcrypt (new) and MD5 (legacy)
	const int password_valid = account_password_matches(d->account, arg);
	const int needs_upgrade = password_valid && !is_bcrypt_hash(d->account->acct_password);

	if (!password_valid)
	{
		SEND_TO_Q("Invalid Password ... disconnecting\r\n", d);
		d->account = free_account(d->account);
		STATE(d) = CON_FLUSH;
		return;
	}

	if (d->account->acct_blocked == ACCOUNT_BLOCK_DELETION)
	{
		echo_on(d);
		STATE(d) = CON_ACCT_VERIFY_DELETE_ACCT;
		display_account_deletion_confirmation(d, true);
		return;
	}

	// Auto-upgrade MD5 passwords to bcrypt
	if (needs_upgrade)
	{
		char *new_hash = bcrypt_hash_password(arg);
		if (new_hash)
		{
			FREE(d->account->acct_password);
			d->account->acct_password = str_dup(new_hash);
			free(new_hash);
			if (-1 == write_account(d->account))
			{
				statuslog(56, "&+RALERT&n: account password upgrade save failed");
				persistence_alert(AVATAR, "account", "redacted", "none", "none",
						  "write_failed", "password upgrade save failed");
			}
		}
	}

	// Check for duplicate account login - kick old connection if found
	P_desc k;
	for (k = descriptor_list; k; k = k->next)
	{
		if ((k != d) && k->account && k->account->acct_name &&
		    !strcasecmp(k->account->acct_name, d->account->acct_name))
		{
			// Same account already logged in - disconnect the old connection
			SEND_TO_Q(
				"\r\n\r\nYour account has been logged in from another location.\r\n",
				k);
			SEND_TO_Q("Disconnecting...\r\n\r\n", k);
			close_socket(k);
			SEND_TO_Q("Overriding old connection...\r\n", d);
			break; // Only one duplicate should exist
		}
	}

	echo_on(d);
#ifdef REQUIRE_EMAIL_VERIFICATION
	if (is_account_confirmed(d))
	{
		// Display the news notice and MOTD before showing the account menu
		display_account_login_pages(d);
		update_account_iplist(d);
		STATE(d) = CON_ACCT_RMOTD;
		return;
	}
	else
	{
		confirm_account(d, NULL);
		STATE(d) = CON_CONFIRM_ACCT;
		return;
	}
#else
	// Email verification disabled - skip confirmation and show the login pages
	display_account_login_pages(d);
	update_account_iplist(d);
	STATE(d) = CON_ACCT_RMOTD;
	return;
#endif
}

void display_account_menu(P_desc d, char *arg)
{
	if (!arg)
	{
		char buf[256];

		SEND_TO_Q("\r\n", d);
		SEND_TO_Q("&+y/===========================================\\&n\r\n", d);
		snprintf(buf, 256, "&+y|&n         &+W%s&n's &+CACCOUNT MENU&n          &+y|&n\r\n",
			 d->account->acct_name);
		SEND_TO_Q(buf, d);
		SEND_TO_Q("&+y\\===========================================/&n\r\n", d);
		SEND_TO_Q("\r\n", d);

		SEND_TO_Q("&+G1) Select a character to play&n\r\n", d);
		SEND_TO_Q("&+G2) Create a new character&n\r\n", d);
		SEND_TO_Q("&+R3) Delete a character&n\r\n", d);
		SEND_TO_Q("\r\n", d);
		SEND_TO_Q("&+C4) Display account information&n\r\n", d);
		SEND_TO_Q("&+Y5) Change registered email address&n\r\n", d);
		SEND_TO_Q("&+Y6) Change account password&n\r\n", d);
		SEND_TO_Q("&+R7) Delete this account&n\r\n", d);
		SEND_TO_Q("&+C8) Check rested bonus&n\r\n", d);
		SEND_TO_Q("\r\n", d);
		SEND_TO_Q("&+L0) Disconnect from this account&n\r\n", d);
		SEND_TO_Q("&+y------------------------------------------&n\r\n", d);
		SEND_TO_Q("Please select an option: ", d);
		d->prompt_mode = TRUE;
		return;
	}

	char *end = NULL;
	while (isspace((unsigned char)*arg))
		arg++;
	long selection = strtol(arg, &end, 10);
	while (end && isspace((unsigned char)*end))
		end++;
	if (!*arg || end == arg || (end && *end) || selection < 0 || selection > 8)
	{
		SEND_TO_Q("Invalid Selection, please try again.\r\n", d);
		display_account_menu(d, NULL);
		return;
	}

	switch (selection)
	{
	case 0:
		STATE(d) = CON_FLUSH;
		SEND_TO_Q("\r\n\r\nThank you for playing!\r\n", d);
		if (-1 == write_account(d->account))
		{
			statuslog(56, "&+RALERT&n: account logout save failed");
			persistence_alert(AVATAR, "account", "redacted", "none", "none",
					  "write_failed", "logout save failed");
		}
		break;

	case 1:
		STATE(d) = CON_ACCT_SELECT_CHAR;
		account_select_char(d, NULL);
		break;

	case 2:
		SEND_TO_Q("Enter your new name:  ", d);
		STATE(d) = CON_ACCT_NEW_CHAR_NAME;
		break;

	case 3:
		STATE(d) = CON_ACCT_DELETE_CHAR;
		account_delete_char(d, NULL);
		break;

	case 4:
		STATE(d) = CON_ACCT_DISPLAY_INFO;
		account_display_info(d, NULL);
		break;

	case 5:
		STATE(d) = CON_ACCT_CHANGE_EMAIL;
		get_new_account_email(d, NULL);
		break;

	case 6:
		STATE(d) = CON_ACCT_CHANGE_PASSWD;
		get_new_account_password(d, NULL);
		break;

	case 7:
		STATE(d) = CON_ACCT_DELETE_ACCT;
		delete_account(d, NULL);
		break;

	case 8:
		check_rested_bonus(d);
		break;

	default:
		SEND_TO_Q("Invalid Selection, please try again.\r\n", d);
		display_account_menu(d, NULL);
		break;
	}
}

void confirm_account(P_desc d, char *arg)
{
	if (!arg)
	{
		SEND_TO_Q("Please enter the confirmation code shown above: ", d);
		return;
	}

	// Trim any whitespace from the input
	while (*arg && isspace(*arg))
		arg++;

	// Fix doubled $ characters (MUD color code escaping)
	// When user types $, it comes through as $$
	char fixed_input[512];
	char *src = arg;
	char *dst = fixed_input;
	while (*src && (dst - fixed_input) < 511)
	{
		*dst++ = *src++;
		if (*(src - 1) == '$' && *src == '$')
		{
			src++; // Skip the doubled $
		}
	}
	*dst = '\0';

	if (str_cmp(fixed_input, d->account->acct_confirmation))
	{
		SEND_TO_Q("\r\n&+RInvalid confirmation code.&n\r\n", d);
		SEND_TO_Q("Please try again or reconnect to get a new code.\r\n", d);
		SEND_TO_Q("Please enter the confirmation code: ", d);
		return; // Allow retry instead of disconnecting
	}
	else
	{
		SEND_TO_Q("\r\n&+GThank you for confirming your account!&n\r\n\r\n", d);
		d->account->acct_confirmed = 1;
		if (-1 == write_account(d->account))
		{
			SEND_TO_Q(
				"Oh no, I couldn't write your account information to disk, notify a god!\r\n",
				d);
			statuslog(56, "&+RALERT&n: account confirmation save failed");
			persistence_alert(AVATAR, "account", "redacted", "none", "none",
					  "write_failed", "confirmation save failed");
			d->account = free_account(d->account);
			STATE(d) = CON_FLUSH;
			return;
		}
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		if (-1 == write_account(d->account))
		{
			statuslog(56, "&+RALERT&n: confirmed account rewrite failed");
			persistence_alert(AVATAR, "account", "redacted", "none", "none",
					  "write_failed", "post-confirm menu save failed");
		}
	}
}

void verify_account_name(P_desc d, char *arg)
{
	char buf[1024];

	if (!arg)
	{
		*d->account->acct_name = toupper(*d->account->acct_name);
		snprintf(buf, 1024, "You chose the name %s, is this correct? (Y/N)  ",
			 d->account->acct_name);
		SEND_TO_Q(buf, d);
		return;
	}

	if ((arg[0] == 'y') || (arg[0] == 'Y'))
	{
		SEND_TO_Q("Confirmed...\r\n", d);
		STATE(d) = CON_GET_NEW_ACCT_EMAIL;
		get_new_account_email(d, NULL);
		return;
	}
	else if ((arg[0] == 'n') || (arg[0] == 'N'))
	{
		SEND_TO_Q("Ok, what then?\r\n", d);
		d->account = free_account(d->account);
		STATE(d) = CON_GET_ACCT_NAME;
		return;
	}
	else
	{
		SEND_TO_Q("Invalid choice...\r\n", d);
		verify_account_name(d, NULL);
	}
}

void get_new_account_email(P_desc d, char *arg)
{
	if (!arg)
	{
		SEND_TO_Q("\r\nPlease enter your email address:  ", d);
		return;
	}
	for (; isspace(*arg); arg++)
		;

	// Validate email format
	if (!is_valid_email(arg))
	{
		SEND_TO_Q("\r\n&+RInvalid email format.&n Please enter a valid email address.\r\n",
			  d);
		SEND_TO_Q("Email address:  ", d);
		return;
	}

	if (is_email_taken(arg))
	{
		SEND_TO_Q(
			"\r\n&+REmail already in use. &+WOnly one account per user please.&n If you are a new user please try a different email.\r\n",
			d);
		SEND_TO_Q("Email address:  ", d);
		return;
	}

	d->account->acct_email = str_dup(arg);
	STATE(d) = CON_VERIFY_NEW_ACCT_EMAIL;
	verify_new_account_email(d, NULL);
	return;
}

void verify_new_account_email(P_desc d, char *arg)
{
	char buf[1024];

	if (!arg)
	{
		snprintf(buf, 1024, "\r\nYou entered %s, is this correct?  (Y/N)",
			 d->account->acct_email);
		SEND_TO_Q(buf, d);
		return;
	}
	if ((arg[0] == 'y') || (arg[0] == 'Y'))
	{
		SEND_TO_Q("Confirmed...\r\n", d);
		if (d->account->acct_confirmed == 0)
		{
			STATE(d) = CON_GET_NEW_ACCT_PASSWD;
			get_new_account_password(d, NULL);
		}
		else
		{
			STATE(d) = CON_DISPLAY_ACCT_MENU;
			display_account_menu(d, NULL);
			if (-1 == write_account(d->account))
			{
				statuslog(56, "&+RALERT&n: existing confirmed account save failed");
				persistence_alert(AVATAR, "account", "redacted", "none", "none",
						  "write_failed", "post-login menu save failed");
			}
		}
		return;
	}
	else if ((arg[0] == 'n') || (arg[0] == 'N'))
	{
		SEND_TO_Q("Ok, what then?\r\n", d);
		FREE(d->account->acct_email);
		d->account->acct_email = NULL;
		STATE(d) = CON_GET_NEW_ACCT_EMAIL;
		return;
	}
	else
	{
		SEND_TO_Q("Invalid choice...\r\n", d);
		verify_new_account_email(d, NULL);
	}
}

void get_new_account_password(P_desc d, char *arg)
{
	if (!arg)
	{
		echo_on(d);
		SEND_TO_Q("Please enter your password:  ", d);
		echo_off(d);
		return;
	}
	echo_on(d);

	for (; isspace(*arg); arg++)
		;

	if (!*arg)
	{
		SEND_TO_Q("Invalid Password, try again.\r\n", d);
		get_new_account_password(d, NULL);
		return;
	}

	if (!valid_password(d, arg))
	{
		get_new_account_password(d, NULL);
		return;
	}

	// Use bcrypt for account passwords
	char *hash = bcrypt_hash_password(arg);
	if (!hash)
	{
		SEND_TO_Q("Error hashing password, please try again.\r\n", d);
		get_new_account_password(d, NULL);
		return;
	}

	d->account->acct_password = str_dup(hash);
	free(hash);
	STATE(d) = CON_VERIFY_NEW_ACCT_PASSWD;
	verify_new_account_password(d, NULL);
	return;
}

void verify_new_account_password(P_desc d, char *arg)
{
	if (!arg)
	{
		echo_on(d);
		SEND_TO_Q("Please verify your password:  ", d);
		echo_off(d);
		return;
	}
	echo_on(d);

	// Use bcrypt to verify the password
	if (!bcrypt_verify_password(arg, d->account->acct_password))
	{
		SEND_TO_Q("Passwords do not match!\r\n", d);
		get_new_account_password(d, NULL);
		FREE(d->account->acct_password);
		d->account->acct_password = NULL;
		STATE(d) = CON_GET_NEW_ACCT_PASSWD;
		return;
	}

	if (d->account->acct_confirmed == 0)
	{
		STATE(d) = CON_VERIFY_NEW_ACCT_INFO;
		verify_new_account_information(d, NULL);
	}
	else
	{
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		if (-1 == write_account(d->account))
		{
			statuslog(56, "&+RALERT&n: auto-confirmed account save failed");
			persistence_alert(AVATAR, "account", "redacted", "none", "none",
					  "write_failed", "auto-confirm save failed");
		}
	}
	return;
}

void verify_new_account_information(P_desc d, char *arg)
{
	if (!arg)
	{
		display_account_information(d);
		SEND_TO_Q("\r\nIs this information correct?  (Y/N) ", d);
		return;
	}
	if ((arg[0] == 'y') || (arg[0] == 'Y'))
	{
#ifdef REQUIRE_EMAIL_VERIFICATION
		SEND_TO_Q(
			"You will receive a confimation code in your email.\r\nYou must confirm your account before using it.\r\n\r\n",
			d);
		generate_account_confirmation_code(d, NULL);
		if (-1 == write_account(d->account))
		{
			statuslog(56, "&+RALERT&n: account confirmation-code save failed");
			persistence_alert(AVATAR, "account", "redacted", "none", "none",
					  "write_failed", "confirmation code save failed");
		}
		// Don't disconnect - go straight to confirmation prompt
		STATE(d) = CON_CONFIRM_ACCT;
		SEND_TO_Q("Please enter the confirmation code shown above: ", d);
#else
		// Email verification disabled - skip confirmation and go directly to menu
		SEND_TO_Q("&+GAccount created successfully!&n\r\n\r\n", d);
		generate_account_confirmation_code(
			d, NULL); // Still generates code (for display) but auto-confirms
		if (-1 == write_account(d->account))
		{
			statuslog(56, "&+RALERT&n: auto-confirmed account save failed");
			persistence_alert(AVATAR, "account", "redacted", "none", "none",
					  "write_failed", "auto-confirm save failed");
		}
		display_account_login_pages(d);
		update_account_iplist(d);
		STATE(d) = CON_ACCT_RMOTD;
#endif
		return;
	}
	else if ((arg[0] == 'n') || (arg[0] == 'N'))
	{
		SEND_TO_Q("Ok, starting over!\r\n", d);
		d->account = free_account(d->account);
		STATE(d) = CON_GET_ACCT_NAME;
		SEND_TO_Q("Please enter your account name: ", d);
		return;
	}
	else
	{
		SEND_TO_Q("Invalid choice...\r\n", d);
		verify_new_account_information(d, NULL);
	}
}

void update_account_iplist(P_desc d)
{
	P_acct acct = d->account;
	struct acct_ip *ip = NULL;

	ip = find_ip_entry(acct, d);

	if (!ip)
	{
		add_ip_entry(acct, d);
		return;
	}
	else
	{
		ip->count++;
		if (-1 == write_account(acct))
		{
			statuslog(56, "&+RALERT&n: account IP-list update failed");
			persistence_alert(AVATAR, "account", "redacted", "none", "none",
					  "write_failed", "ip list update failed");
		}
		return;
	}
}

struct acct_ip *find_ip_entry(P_acct acct, P_desc d)
{
	struct acct_ip *a = acct->acct_unique_ips;

	if (!a)
		return NULL;

	while (a)
	{
		if (!strcmp(a->hostname, d->host))
			return a;
		else
			a = a->next;
	}

	return NULL;
}

void add_ip_entry(P_acct acct, P_desc d)
{
	char host[512];
	struct acct_ip *a = NULL;

	snprintf(host, 512, "%s", d->host);

	CREATE(a, struct acct_ip, 1, MEM_TAG_OTHER);
	if (!a)
		return;

	a->hostname = str_dup(host);
	a->count = 1;
	a->ip_address = str_dup(host);
	acct->num_ips++;
	a->next = acct->acct_unique_ips;
	acct->acct_unique_ips = a;
	return;
}

void account_select_char(P_desc d, char *arg)
{
	struct acct_chars *c = NULL;
	struct acct_chars *sorted_chars[MAX_CHARS_PER_ACCOUNT];
	struct acct_chars *temp;
	long selection = -1;
	int count = 0, i, j;

	if (!arg)
	{
		display_character_list(d);
		return;
	}

	while (isspace((unsigned char)*arg))
		arg++;

	// Check if input is numeric
	if (isdigit((unsigned char)arg[0]) || arg[0] == '+' || arg[0] == '-')
	{
		char *end = NULL;
		selection = strtol(arg, &end, 10);
		while (end && isspace((unsigned char)*end))
			end++;
		if (end == arg || (end && *end))
		{
			SEND_TO_Q("&+RInvalid selection.&n\r\n\r\n", d);
			display_character_list(d);
			return;
		}

		// Option 0 = back to account menu
		if (selection == 0)
		{
			STATE(d) = CON_DISPLAY_ACCT_MENU;
			display_account_menu(d, NULL);
			return;
		}

		// Build sorted character list (same as display_character_list)
		temp = d->account->acct_character_list;
		while (temp && count < MAX_CHARS_PER_ACCOUNT)
		{
			sorted_chars[count++] = temp;
			temp = temp->next;
		}

		// Sort by last login time (most recent first)
		for (i = 0; i < count - 1; i++)
		{
			for (j = 0; j < count - i - 1; j++)
			{
				if (sorted_chars[j]->last < sorted_chars[j + 1]->last)
				{
					struct acct_chars *swap = sorted_chars[j];
					sorted_chars[j] = sorted_chars[j + 1];
					sorted_chars[j + 1] = swap;
				}
			}
		}

		// Validate selection
		if (selection < 1 || selection > count)
		{
			SEND_TO_Q("&+RInvalid selection.&n\r\n\r\n", d);
			display_character_list(d);
			return;
		}

		// Get the selected character (1-based index)
		c = sorted_chars[selection - 1];
	}
	else
	{
		// Text-based selection (for backward compatibility)
		c = find_char_in_list(d->account->acct_character_list, arg);
	}

	if (!c)
	{
		SEND_TO_Q("Sorry, I couldn't find that character!\r\n", d);
		display_character_list(d);
		return;
	}

	if (!can_connect(c, d))
	{
		char buf[512];
		int current_time = time(NULL);
		int time_remaining = 0;
		int minutes_remaining = 0;
		int racewarSwitchTimer = get_property("account.timer.racewarSwitch", 3600);

		// Calculate time remaining for racewar timer
		if (c->racewar == ACCT_GOOD &&
		    current_time < (d->account->acct_evil + racewarSwitchTimer))
		{
			time_remaining =
				(d->account->acct_evil + racewarSwitchTimer) - current_time;
			minutes_remaining = (time_remaining + 59) / 60; // Round up
			snprintf(
				buf, 512,
				"\r\n&+RSorry, you cannot play this Good-aligned character yet!&n\r\n"
				"You must wait &+Y%d&n more minute%s before playing a Good character.\r\n"
				"(You recently played an Evil character)\r\n\r\n",
				minutes_remaining, minutes_remaining == 1 ? "" : "s");
			SEND_TO_Q(buf, d);
		}
		else if (c->racewar == ACCT_EVIL &&
			 current_time < (d->account->acct_good + racewarSwitchTimer))
		{
			time_remaining =
				(d->account->acct_good + racewarSwitchTimer) - current_time;
			minutes_remaining = (time_remaining + 59) / 60; // Round up
			snprintf(
				buf, 512,
				"\r\n&+RSorry, you cannot play this Evil-aligned character yet!&n\r\n"
				"You must wait &+Y%d&n more minute%s before playing an Evil character.\r\n"
				"(You recently played a Good character)\r\n\r\n",
				minutes_remaining, minutes_remaining == 1 ? "" : "s");
			SEND_TO_Q(buf, d);
		}
		else if (c->blocked)
		{
			SEND_TO_Q(
				"\r\n&+RThis character has been blocked and cannot be played.&n\r\n\r\n",
				d);
		}
		else
		{
			SEND_TO_Q("Sorry, you can't play that character right now!\r\n", d);
		}

		display_character_list(d);
		return;
	}

	if (is_char_in_game(c, d))
	{
		return;
	}

	// Store the selected character name for confirmation
	if (d->selected_char_name)
		str_free(d->selected_char_name);
	d->selected_char_name = str_dup(c->charname);

	// Capitalize character name for display
	char name_cap[32];
	strlcpy(name_cap, c->charname, sizeof name_cap);
	if (name_cap[0])
		name_cap[0] = toupper((unsigned char)name_cap[0]);

	// Send confirmation prompt
	char confirm_buf[256];
	snprintf(confirm_buf, 256, "\r\nPlay as &+W%s&n? (Y/N) ", name_cap);
	SEND_TO_Q(confirm_buf, d);
	d->prompt_mode = TRUE;

	// Change state to confirmation
	STATE(d) = CON_ACCT_CONFIRM_CHAR;

	return;
}

void account_confirm_char(P_desc d, char *arg)
{
	struct acct_chars *c = NULL;
	P_char ch = NULL;

	if (!arg || !*arg)
	{
		SEND_TO_Q("Play this character? (Y/N) ", d);
		return;
	}

	// Handle 'No' or cancel
	if (arg[0] == 'n' || arg[0] == 'N')
	{
		if (d->selected_char_name)
		{
			str_free(d->selected_char_name);
			d->selected_char_name = NULL;
		}
		SEND_TO_Q("\r\n", d);
		display_character_list(d);
		STATE(d) = CON_ACCT_SELECT_CHAR;
		return;
	}

	// Handle 'Yes'
	if (arg[0] == 'y' || arg[0] == 'Y')
	{
		if (!d->selected_char_name)
		{
			SEND_TO_Q("&+RError: No character selected.&n\r\n", d);
			display_character_list(d);
			STATE(d) = CON_ACCT_SELECT_CHAR;
			return;
		}

		// Find the character
		c = find_char_in_list(d->account->acct_character_list, d->selected_char_name);

		if (!c)
		{
			SEND_TO_Q("&+RSorry, I couldn't find that character!&n\r\n", d);
			if (d->selected_char_name)
			{
				str_free(d->selected_char_name);
				d->selected_char_name = NULL;
			}
			display_character_list(d);
			STATE(d) = CON_ACCT_SELECT_CHAR;
			return;
		}

		// Verify can still connect (double-check racewar timer, etc.)
		if (!can_connect(c, d))
		{
			char buf[512];
			int current_time = time(NULL);
			int time_remaining = 0;
			int minutes_remaining = 0;
			int racewarSwitchTimer = get_property("account.timer.racewarSwitch", 3600);

			if (c->racewar == ACCT_GOOD &&
			    current_time < (d->account->acct_evil + racewarSwitchTimer))
			{
				time_remaining = (d->account->acct_evil + 3600) - current_time;
				minutes_remaining = (time_remaining + 59) / 60;
				snprintf(
					buf, 512,
					"\r\n&+RSorry, you cannot play this Good-aligned character yet!&n\r\n"
					"You must wait &+Y%d&n more minute%s before playing a Good character.\r\n",
					minutes_remaining, minutes_remaining == 1 ? "" : "s");
				SEND_TO_Q(buf, d);
			}
			else if (c->racewar == ACCT_EVIL &&
				 current_time < (d->account->acct_good + racewarSwitchTimer))
			{
				time_remaining = (d->account->acct_good + 3600) - current_time;
				minutes_remaining = (time_remaining + 59) / 60;
				snprintf(
					buf, 512,
					"\r\n&+RSorry, you cannot play this Evil-aligned character yet!&n\r\n"
					"You must wait &+Y%d&n more minute%s before playing an Evil character.\r\n",
					minutes_remaining, minutes_remaining == 1 ? "" : "s");
				SEND_TO_Q(buf, d);
			}
			else if (c->blocked)
			{
				SEND_TO_Q(
					"\r\n&+RThis character has been blocked and cannot be played.&n\r\n",
					d);
			}

			if (d->selected_char_name)
			{
				str_free(d->selected_char_name);
				d->selected_char_name = NULL;
			}
			display_character_list(d);
			STATE(d) = CON_ACCT_SELECT_CHAR;
			return;
		}

		// Check if character is already in game
		if (is_char_in_game(c, d))
		{
			if (d->selected_char_name)
			{
				str_free(d->selected_char_name);
				d->selected_char_name = NULL;
			}
			return;
		}

		// Load character into game
		ch = load_char_into_game(c, d);

		if (!ch)
		{
			if (STATE(d) == CON_PLAYER_LOAD)
				return;
			SEND_TO_Q("&+RSorry, I couldn't load that character!&n\r\n", d);
			if (d->selected_char_name)
			{
				str_free(d->selected_char_name);
				d->selected_char_name = NULL;
			}
			display_character_list(d);
			STATE(d) = CON_ACCT_SELECT_CHAR;
			return;
		}

		// Clean up temporary storage
		if (d->selected_char_name)
		{
			str_free(d->selected_char_name);
			d->selected_char_name = NULL;
		}

		// Show MOTD and enter game
		if (IS_TRUSTED(ch))
			SEND_TO_Q(wizmotd.c_str(), d);
		else
			SEND_TO_Q(motd.c_str(), d);

		echo_on(d);
		STATE(d) = CON_PLAYING;
		d->character = ch;
		c->count++;
		c->last = time(NULL);
		enter_game(d);
		const int projection_room = ch->in_room >= 0 && ch->in_room <= top_of_world ?
						    world[ch->in_room].number :
						    ch->specials.was_in_room;
		if (!sync_account_character_projection(ch, projection_room, TRUE))
		{
			statuslog(
				56,
				"&+RALERT&n: loaded flat-file account character projection save failed");
			persistence_alert(AVATAR, "account", "redacted", "none", "none",
					  "write_failed",
					  "loaded character projection save failed");
		}
		d->prompt_mode = !item_creation_grant_blocks_commands(ch);

		switch (GET_RACEWAR(ch))
		{
		case RACEWAR_GOOD:
			d->account->acct_good = time(NULL);
			break;
		case RACEWAR_EVIL:
			d->account->acct_evil = time(NULL);
			break;
		}

		return;
	}

	// Invalid input
	SEND_TO_Q("Please answer (Y/N): ", d);
	return;
}

// Helper structure for character display data
struct char_display_info
{
	char charname[32];
	int level;
	int race;
	unsigned int m_class;
	unsigned int secondary_class;
	char *rested_status; // "Well-Rested", "Rested", or "None"
	int hometown; // Last room character was in
	long last_login;
};

// cleanup temp char loaded via restoreCharOnly before freeing
// handles items, affects, events, strings
// NOTE: does NOT free the char struct itself or pc_only_data - caller must do that
void cleanup_temp_char(P_char ch)
{
	extern struct mm_ds *dead_affect_pool;

	if (!ch)
		return;

	// unequip and extract all equipment
	for (int i = 0; i < MAX_WEAR; i++)
	{
		if (ch->equipment[i])
		{
			P_obj obj = unequip_char(ch, i);
			extract_obj(obj, FALSE);
		}
	}

	// remove and extract all carried items
	while (ch->carrying)
	{
		P_obj obj = ch->carrying;
		obj_from_char(obj);
		extract_obj(obj, FALSE);
	}

	// release affects directly to pool (don't use affect_remove - it schedules events)
	while (ch->affected)
	{
		struct affected_type *af = ch->affected;
		ch->affected = af->next;
		if (dead_affect_pool)
			mm_release(dead_affect_pool, af);
	}

	// clear any scheduled events
	disarm_char_nevents(ch, NULL);

	// free strings allocated by sql_row_str/getString
	if (ch->player.name)
		str_free(ch->player.name);
	if (ch->player.title)
		str_free(ch->player.title);
	if (ch->player.short_descr)
		str_free(ch->player.short_descr);
	if (ch->player.long_descr)
		str_free(ch->player.long_descr);
	if (ch->player.description)
		str_free(ch->player.description);

	// free pc-only strings and data
	if (IS_PC(ch) && ch->only.pc)
	{
		if (ch->only.pc->poofIn)
			str_free(ch->only.pc->poofIn);
		if (ch->only.pc->poofOut)
			str_free(ch->only.pc->poofOut);
		if (ch->only.pc->gcmd_arr)
			FREE(ch->only.pc->gcmd_arr);
	}
}

// Helper function to load character display data
// Returns 1 on success, 0 on failure
int load_char_display_data(char *charname, struct char_display_info *info)
{
	P_char temp_ch;
	int result;

	// Create temporary character structure using malloc (like pfile.c does)
	temp_ch = (struct char_data *)malloc(sizeof(struct char_data));
	if (!temp_ch)
		return 0;

	memset(temp_ch, 0, sizeof(struct char_data));

	temp_ch->only.pc = (struct pc_only_data *)malloc(sizeof(struct pc_only_data));
	if (!temp_ch->only.pc)
	{
		free(temp_ch);
		return 0;
	}

	memset(temp_ch->only.pc, 0, sizeof(struct pc_only_data));

	// Load character data
	result = restoreCharOnly(temp_ch, charname);
	if (result < 0)
	{
		if (temp_ch->only.pc)
			free(temp_ch->only.pc);
		free(temp_ch);
		return 0;
	}

	// Extract display data
	strlcpy(info->charname, GET_NAME(temp_ch), sizeof info->charname);
	info->level = GET_LEVEL(temp_ch);
	info->race = GET_RACE(temp_ch);
	info->m_class = temp_ch->player.m_class;
	info->secondary_class = temp_ch->player.secondary_class;
	info->hometown = GET_HOME(temp_ch);

	// Calculate rested status based on offline time (same logic as nanny.c)
	time_t current_time = time(0);
	time_t offline_seconds = current_time - temp_ch->player.time.saved;
	int offline_hours = offline_seconds / 3600;
	char rested_buf[128];

	if (offline_hours >= 20)
	{
		// Well-rested bonus
		snprintf(rested_buf, 128, "&+Wwell-rested&n bonus (&+G%d&n hours offline)",
			 offline_hours);
		info->rested_status = str_dup(rested_buf);
	}
	else if (offline_hours >= 9)
	{
		// Rested bonus
		snprintf(rested_buf, 128, "&+Grested&n bonus (&+Y%d&n hours offline)",
			 offline_hours);
		info->rested_status = str_dup(rested_buf);
	}
	else
	{
		// No bonus yet - show how many more hours needed
		int hours_needed = 9 - offline_hours;
		snprintf(rested_buf, 128, "&+LNone&n (&+R%d&n more hour%s needed)", hours_needed,
			 hours_needed == 1 ? "" : "s");
		info->rested_status = str_dup(rested_buf);
	}

	cleanup_temp_char(temp_ch);

	// free the temp char struct
	if (temp_ch->only.pc)
		free(temp_ch->only.pc);
	free(temp_ch);

	return 1;
}

// Helper function to get race name from character display info
void get_race_name_from_info(struct char_display_info *info, char *race_str, int max_len)
{
	extern const struct race_names race_names_table[];
	if (info->race >= 0 && info->race < LAST_RACE)
		strlcpy(race_str, race_names_table[info->race].normal, max_len);
	else
		strlcpy(race_str, "Unknown", max_len);
}

void check_rested_bonus(P_desc d)
{
	struct acct_chars *c = d->account->acct_character_list;
	char buf[512];
	int count = 0;

	SEND_TO_Q("\r\n&+y===== &+WRESTED BONUS STATUS&+y =====&n\r\n\r\n", d);

	while (c)
	{
		struct char_display_info info;

		if (load_char_display_data(c->charname, &info))
		{
			// Capitalize character name
			char name_cap[32];
			strlcpy(name_cap, info.charname, sizeof name_cap);
			if (name_cap[0])
				name_cap[0] = toupper(name_cap[0]);

			snprintf(buf, 512, "&+C%-12s&n: %s\r\n", name_cap,
				 info.rested_status ? info.rested_status : "&+LNone&n");
			SEND_TO_Q(buf, d);

			if (info.rested_status)
				str_free(info.rested_status);

			count++;
		}
		c = c->next;
	}

	if (count == 0)
	{
		SEND_TO_Q("&+LNo characters found.&n\r\n", d);
	}

	SEND_TO_Q("\r\n", d);
	display_account_menu(d, NULL);
}

void display_delete_character_list(P_desc d)
{
	struct acct_chars *c = d->account->acct_character_list;
	struct acct_chars *sorted_chars[MAX_CHARS_PER_ACCOUNT];
	char buf[256];
	int count = 0, i, j;
	struct acct_chars *temp;

	// Enable ANSI terminal mode for color display
	d->term_type = TERM_ANSI;

	if (!c)
	{
		snprintf(buf, 256, "You currently don't have any characters to delete.\r\n");
		SEND_TO_Q(buf, d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}

	// Count characters and build array for sorting
	temp = c;
	while (temp && count < MAX_CHARS_PER_ACCOUNT)
	{
		sorted_chars[count++] = temp;
		temp = temp->next;
	}

	// Sort by last login time (most recent first) - simple bubble sort
	for (i = 0; i < count - 1; i++)
	{
		for (j = 0; j < count - i - 1; j++)
		{
			if (sorted_chars[j]->last < sorted_chars[j + 1]->last)
			{
				struct acct_chars *swap = sorted_chars[j];
				sorted_chars[j] = sorted_chars[j + 1];
				sorted_chars[j + 1] = swap;
			}
		}
	}

	// Display large warning banner in bright red
	SEND_TO_Q("\r\n", d);
	SEND_TO_Q(
		"&+R/===========================================================================\\&n\r\n",
		d);
	SEND_TO_Q(
		"&+R|                                                                           |&n\r\n",
		d);
	SEND_TO_Q(
		"&+R|                         !!!  W A R N I N G  !!!                          |&n\r\n",
		d);
	SEND_TO_Q(
		"&+R|                                                                           |&n\r\n",
		d);
	SEND_TO_Q(
		"&+R|                  CHARACTER DELETION IS PERMANENT!                        |&n\r\n",
		d);
	SEND_TO_Q(
		"&+R|                                                                           |&n\r\n",
		d);
	SEND_TO_Q(
		"&+R|       Once deleted, your character CANNOT be recovered or restored!      |&n\r\n",
		d);
	SEND_TO_Q(
		"&+R|                                                                           |&n\r\n",
		d);
	SEND_TO_Q(
		"&+R\\===========================================================================/&n\r\n",
		d);
	SEND_TO_Q("\r\n", d);

	// Display table header
	SEND_TO_Q("&+R/-------------------------------------------------------\\&n\r\n", d);

	char title_buf[256];
	snprintf(title_buf, 256, "DELETE CHARACTER (%d character%s)", count, count == 1 ? "" : "s");
	int title_len = strlen(title_buf);
	int left_pad = (57 - title_len) / 2;
	int right_pad = 57 - title_len - left_pad - 2;

	checked_snprintf(buf, 256, "&+R|%*s&+W%s%*s&+R|&n\r\n", left_pad, "", title_buf, right_pad,
			 "");
	SEND_TO_Q(buf, d);

	SEND_TO_Q("&+R|=======================================================|&n\r\n", d);
	SEND_TO_Q(
		"&+R|&n # &+R|&n &+RCharacter    &+R|&n &+RLevel &+R|&n &+RRace         &+R|&n &+RClass&n        &+R|&n\r\n",
		d);
	SEND_TO_Q("&+R|-------------------------------------------------------|&n\r\n", d);

	// Display sorted characters in red
	for (i = 0; i < count; i++)
	{
		struct char_display_info info;
		char name_capitalized[32];
		char race_str[32];
		char class_str[64];
		char level_str[16];
		char line_buf[512];

		// Load character display data
		if (!load_char_display_data(sorted_chars[i]->charname, &info))
		{
			snprintf(
				line_buf, 512,
				"&+R|&n &+R%d&n &+R|&n &+R%-12s&n &+R|&n &+R%-5s&n &+R|&n &+R%-12s&n &+R|&n &+R%-12s&n &+R|&n\r\n",
				i + 1, sorted_chars[i]->charname, "?", "?", "?");
			SEND_TO_Q(line_buf, d);
			continue;
		}

		// Capitalize character name
		strlcpy(name_capitalized, info.charname, sizeof name_capitalized);
		if (name_capitalized[0])
			name_capitalized[0] = toupper(name_capitalized[0]);

		// Get race name
		get_race_name_from_info(&info, race_str, 32);

		// Get class name(s)
		extern const struct class_names class_names_table[];
		int primary_idx = flag2idx(info.m_class);
		int secondary_idx = info.secondary_class ? flag2idx(info.secondary_class) : 0;

		snprintf(level_str, 16, "%d", info.level);
		if (info.secondary_class && secondary_idx > 0)
		{
			// Multiclass
			snprintf(class_str, sizeof class_str, "%s/%s",
				 class_names_table[primary_idx].normal,
				 class_names_table[secondary_idx].normal);
		}
		else
		{
			// Single class
			strlcpy(class_str, class_names_table[primary_idx].normal, sizeof class_str);
		}

		// Truncate strings if too long
		if (strlen(name_capitalized) > 12)
			name_capitalized[12] = '\0';
		if (strlen(race_str) > 12)
			race_str[12] = '\0';
		if (strlen(class_str) > 12)
			class_str[12] = '\0';

		// Display character row in bright red
		snprintf(
			line_buf, 512,
			"&+R|&n %d &+R|&n &+R%-12s&n &+R|&n &+R%-5s&n &+R|&n &+R%-12s&n &+R|&n &+R%-12s&n &+R|&n\r\n",
			i + 1, name_capitalized, level_str, race_str, class_str);
		SEND_TO_Q(line_buf, d);

		// Free rested status string
		if (info.rested_status)
			str_free(info.rested_status);
	}

	// Display table footer
	SEND_TO_Q("&+R\\-------------------------------------------------------/&n\r\n", d);
	SEND_TO_Q("\r\n&+W0&n) &+GCancel and return to Account Menu&n\r\n\r\n", d);
	SEND_TO_Q("Which character do you want to &+RDELETE&n? (Enter number or 0 to cancel): ", d);
}

void display_character_list_to_char(P_char ch, P_acct account)
{
	display_character_list(ch->desc, account);
}

void display_character_list(P_desc d, P_acct account)
{
	struct acct_chars *c = account ? account->acct_character_list :
					 d->account->acct_character_list;
	struct acct_chars *sorted_chars[MAX_CHARS_PER_ACCOUNT];
	char buf[256];
	int count = 0, i, j;
	struct acct_chars *temp;

	if (!c)
	{
		snprintf(buf, 256, "Account currently doesn't have any characters (0/%d).\r\n",
			 MAX_CHARS_PER_ACCOUNT);
		SEND_TO_Q(buf, d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}

	// Count characters and build array for sorting
	temp = c;
	while (temp && count < MAX_CHARS_PER_ACCOUNT)
	{
		sorted_chars[count++] = temp;
		temp = temp->next;
	}

	// Sort by last login time (most recent first) - simple bubble sort
	for (i = 0; i < count - 1; i++)
	{
		for (j = 0; j < count - i - 1; j++)
		{
			if (sorted_chars[j]->last < sorted_chars[j + 1]->last)
			{
				struct acct_chars *swap = sorted_chars[j];
				sorted_chars[j] = sorted_chars[j + 1];
				sorted_chars[j + 1] = swap;
			}
		}
	}

	// Display table header
	SEND_TO_Q("\r\n", d);
	SEND_TO_Q(
		"&+y/---------------------------------------------------------------------------\\&n\r\n",
		d);

	// Build CHARACTER SELECTION line with proper padding
	char title_buf[256];
	int title_len;

	snprintf(title_buf, 256, "CHARACTER SELECTION (%d/%d)", count, MAX_CHARS_PER_ACCOUNT);

	title_len = strlen(title_buf);
	int left_pad = (77 - title_len) / 2;
	int right_pad = 77 - title_len - left_pad - 2;

	checked_snprintf(buf, 256, "&+y|%*s&+W%s%*s&+y|&n\r\n", left_pad, "", title_buf, right_pad,
			 "");
	SEND_TO_Q(buf, d);

	SEND_TO_Q(
		"&+y|===========================================================================|&n\r\n",
		d);
	SEND_TO_Q(
		"&+y|&n # &+y|&n Character    &+y|&n Level &+y|&n Race         &+y|&n Class        &+y|&n Last Room        &+y|&n\r\n",
		d);
	SEND_TO_Q(
		"&+y|---------------------------------------------------------------------------|&n\r\n",
		d);

	// Display sorted characters
	for (i = 0; i < count; i++)
	{
		struct acct_chars *ch = sorted_chars[i];
		char name_capitalized[32];
		char race_str[32];
		char class_str[64];
		char level_str[16];
		char line_buf[512];

		// capitalize character name
		strlcpy(name_capitalized, ch->charname, sizeof name_capitalized);
		if (name_capitalized[0])
			name_capitalized[0] = toupper((unsigned char)name_capitalized[0]);

		// get race name
		extern const struct race_names race_names_table[];
		if (ch->race >= 0 && ch->race < LAST_RACE)
			strlcpy(race_str, race_names_table[ch->race].normal, sizeof race_str);
		else
			strlcpy(race_str, "Unknown", sizeof race_str);

		// get class name(s)
		extern const struct class_names class_names_table[];
		int primary_idx = flag2idx(ch->m_class);
		int secondary_idx = ch->secondary_class ? flag2idx(ch->secondary_class) : 0;
		const char *primary_class = primary_idx >= 1 && primary_idx <= CLASS_COUNT ?
						    class_names_table[primary_idx].normal :
						    "Unknown";
		const char *secondary_class = secondary_idx >= 1 && secondary_idx <= CLASS_COUNT ?
						      class_names_table[secondary_idx].normal :
						      NULL;

		if (ch->secondary_class && secondary_class)
		{
			snprintf(level_str, 16, "%d", ch->level);
			snprintf(class_str, sizeof class_str, "%s/%s", primary_class,
				 secondary_class);
		}
		else
		{
			snprintf(level_str, 16, "%d", ch->level);
			strlcpy(class_str, primary_class, sizeof class_str);
		}

		// truncate strings if too long for table
		if (strlen(name_capitalized) > 12)
			name_capitalized[12] = '\0';
		if (strlen(race_str) > 12)
			race_str[12] = '\0';
		if (strlen(class_str) > 12)
			class_str[12] = '\0';

		// get room name (last_room is vnum, need to convert to rnum)
		const char *room_name_src;
		char room_display[128];
		int room_rnum = real_room(ch->last_room);
		if (room_rnum >= 0 && room_rnum < top_of_world && world[room_rnum].name)
		{
			room_name_src = world[room_rnum].name;
		}
		else
		{
			room_name_src = "Unknown";
		}

		// truncate room name to fit column (16 visible chars) while preserving ansi codes
		size_t src_idx = 0, dst_idx = 0;
		int visible_count = 0;
		int max_visible = 16;
		while (room_name_src[src_idx] && dst_idx + 1 < sizeof room_display)
		{
			if (room_name_src[src_idx] == '&' && room_name_src[src_idx + 1])
			{
				size_t color_length = (room_name_src[src_idx + 1] == '+' ||
						       room_name_src[src_idx + 1] == '-') &&
								      room_name_src[src_idx + 2] ?
							      3 :
							      2;
				if (dst_idx + color_length >= sizeof room_display)
					break;
				memcpy(room_display + dst_idx, room_name_src + src_idx,
				       color_length);
				dst_idx += color_length;
				src_idx += color_length;
			}
			else
			{
				if (visible_count >= max_visible)
					break;
				room_display[dst_idx++] = room_name_src[src_idx++];
				visible_count++;
			}
		}
		room_display[dst_idx] = '\0';

		snprintf(
			line_buf, 512,
			"&+y|&n %d &+y|&n %-12s &+y|&n %-5s &+y|&n %-12s &+y|&n %-12s &+y|&n %s &+y|&n\r\n",
			i + 1, name_capitalized, level_str, race_str, class_str, room_display);
		SEND_TO_Q(line_buf, d);
	}

	// Display table footer
	SEND_TO_Q(
		"&+y\\---------------------------------------------------------------------------/&n\r\n",
		d);
	if (d->character == NULL)
	{
		SEND_TO_Q("\r\n&+W0&n) &+LBack to Account Menu&n\r\n\r\n", d);
		SEND_TO_Q("Which character would you like to play? ", d);
		d->prompt_mode = TRUE;
	}
}

int can_connect(struct acct_chars *c, P_desc d)
{
	int current_time = time(NULL);
	int racewarSwitchTimer = get_property("account.timer.racewarSwitch", 3600);

	if (c->blocked)
		return 0;

	if (c->racewar == ACCT_IMMORTAL)
		return 1;

	if ((c->racewar == ACCT_GOOD) &&
	    (current_time < (d->account->acct_evil + racewarSwitchTimer)))
		return 0;

	if ((c->racewar == ACCT_EVIL) &&
	    (current_time < (d->account->acct_good + racewarSwitchTimer)))
		return 0;

	return 1;
}

int is_char_in_game(struct acct_chars *c, P_desc d)
{
	P_desc k = descriptor_list;
	P_char ch = character_list;

	for (; k; k = k->next)
	{
		if ((k != d) && k->character && GET_NAME(k->character) &&
		    !strcasecmp(GET_NAME(k->character), c->charname))
		{
			// ok, same character, take over the descriptor
			d->character = k->character;
			d->character->desc = d;
			close_socket(k);
			SEND_TO_Q("Overriding old connection...\r\n", d);
		}
	}

	for (; ch; ch = ch->next)
	{
		if (IS_PC(ch) && !ch->desc && GET_NAME(ch) &&
		    !strcasecmp(GET_NAME(ch), c->charname))
		{
			echo_on(d);
			SEND_TO_Q("Reconnecting...\r\n", d);
			act("$n has reconnected.", TRUE, ch, 0, 0, TO_ROOM);
			d->character = ch;
			ch->desc = d;
			// sql_update_playerIP(ch);  // Deprecated function
			ch->specials.timer = 0;
			STATE(d) = CON_PLAYING;

			logit(LOG_COMM, "%s [%s] has reconnected.", GET_NAME(d->character),
			      d->host);
			loginlog(d->character->player.level, "%s [%s] has reconnected.",
				 GET_NAME(d->character), d->host);

			if (IS_SET(ch->specials.act, PLR_MORPH))
			{
				if (!ch->only.pc->switched || !IS_MORPH(ch->only.pc->switched) ||
				    /*              (ch != ((P_char)
				       ch->only.pc->switched->only.npc->memory))) */
				    (ch != ch->only.pc->switched->only.npc->orig_char))
				{
					logit(LOG_EXIT,
					      "Something fucked while trying to reconnect linkless morph");
					ch->desc = NULL;
					d->character = NULL;
					STATE(d) = CON_ACCT_SELECT_CHAR;
					display_character_list(d);
					return 1;
				}
				d->original = ch;
				d->character = ch->only.pc->switched;
				d->character->desc = d;
				ch->desc = NULL;
			}
			return 1;
		}
	}
	return 0;
}

struct acct_chars *find_char_in_list(struct acct_chars *list, char *arg)
{
	if (!list)
		return NULL;

	while (list)
	{
		if (!strcasecmp(list->charname, arg))
			return list;
		else
			list = list->next;
	}
	return NULL;
}

P_char load_char_into_game(struct acct_chars *c, P_desc d)
{
	P_char player = NULL;
	player_load_result loaded = {};
	if (!c || !d || c->pid <= 0 || !d->account || !d->account->acct_name)
		return NULL;
	if (!d->player_load_request_id)
	{
		player_load_request request = {};
		request.request_id = player_load_pipeline_next_request_id();
		request.pid = c->pid;
		request.account_name = d->account->acct_name;
		request.deadline_usec =
			persistence_observability_now_usec() + PLAYER_LOAD_TIMEOUT_USEC;
		if (STATE(d) == CON_ACCT_CONFIRM_CHAR)
		{
			if (player_load_pipeline_submit(request) !=
			    player_load_submit_outcome::accepted)
				return NULL;
			d->player_load_request_id = request.request_id;
			d->player_load_pid = c->pid;
			d->player_load_mode = PLAYER_LOAD_MODE_ACCOUNT;
			STATE(d) = CON_PLAYER_LOAD;
			SEND_TO_Q("Loading character...\r\n", d);
			return NULL;
		}
		player_load_result blocking = {};
		if (!player_load_pipeline_wait(request, &blocking, PLAYER_LOAD_TIMEOUT_USEC / 1000))
			return NULL;
		loaded = std::move(blocking);
		d->player_load_mode = PLAYER_LOAD_MODE_NONE;
	}
	else
	{
		auto ready = ready_player_loads.find(d->player_load_request_id);
		if (ready == ready_player_loads.end())
			return NULL;
		loaded = std::move(ready->second);
		ready_player_loads.erase(ready);
		d->player_load_request_id = 0;
		d->player_load_pid = 0;
	}

	player = (P_char)mm_get(dead_mob_pool);
	if (!player)
	{
		d->player_load_mode = PLAYER_LOAD_MODE_NONE;
		return NULL;
	}

	clear_char(player);

	if (!dead_pconly_pool)
		dead_pconly_pool =
			mm_create("PC_ONLY", sizeof(struct pc_only_data),
				  offsetof(struct pc_only_data, switched),
				  mm_find_best_chunk(sizeof(struct pc_only_data), 10, 25));

	player->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);
	if (!player->only.pc)
	{
		d->player_load_mode = PLAYER_LOAD_MODE_NONE;
		mm_release(dead_mob_pool, player);
		return NULL;
	}
	player->desc = d;

	if (!player_load_materialize(player, loaded))
	{
		d->player_load_mode = PLAYER_LOAD_MODE_NONE;
		free_char(player);
		return NULL;
	}
	d->player_load_mode = PLAYER_LOAD_MODE_ACCOUNT;
	// fixing racewar assignment on character list
	c->racewar = GET_RACEWAR(player) == RACEWAR_EVIL ? ACCT_EVIL : ACCT_GOOD;
	d->rtype = loaded.snapshot.save_intent;
	return player;
}

void account_player_load_complete(P_desc d, player_load_result result)
{
	if (!d || STATE(d) != CON_PLAYER_LOAD || d->player_load_mode != PLAYER_LOAD_MODE_ACCOUNT ||
	    !d->player_load_request_id || result.request_id != d->player_load_request_id)
	{
		player_load_pipeline_note_stale();
		return;
	}
	if (result.pid != d->player_load_pid)
	{
		player_load_pipeline_note_stale();
		result.pid = d->player_load_pid;
		result.outcome = player_load_outcome::stale;
	}
	const uint64_t completed_request_id = result.request_id;
	try
	{
		ready_player_loads.emplace(completed_request_id, std::move(result));
	}
	catch (const std::bad_alloc &)
	{
		d->player_load_request_id = 0;
		d->player_load_pid = 0;
		d->player_load_mode = PLAYER_LOAD_MODE_NONE;
		SEND_TO_Q("&+RSorry, I couldn't prepare that character!&n\r\n", d);
		if (d->selected_char_name)
		{
			str_free(d->selected_char_name);
			d->selected_char_name = NULL;
		}
		STATE(d) = CON_ACCT_SELECT_CHAR;
		display_character_list(d);
		return;
	}
	STATE(d) = CON_ACCT_CONFIRM_CHAR;
	account_confirm_char(d, writable_arg("Y"));
	if (ready_player_loads.erase(completed_request_id))
	{
		d->player_load_request_id = 0;
		d->player_load_pid = 0;
		d->player_load_mode = PLAYER_LOAD_MODE_NONE;
	}
}

void account_new_char(P_desc d, char * /*arg*/)
{
	SEND_TO_Q("Enter your new name:  ", d);
	STATE(d) = CON_ACCT_NEW_CHAR_NAME;
	return;
}

void account_new_char_name(P_desc d, char *arg)
{
	P_char player = NULL;
	char tmp_name[1024];

	if (!arg)
	{
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}

	// Check if account has reached character limit
	if (d->account->num_chars >= MAX_CHARS_PER_ACCOUNT)
	{
		SEND_TO_Q(
			"\r\n&+RYou already have the maximum number of characters allowed per account.&n\r\n",
			d);
		SEND_TO_Q("Please delete a character before creating a new one.\r\n\r\n", d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}

	for (; isspace(*arg); arg++)
		;

	if (_parse_name(arg, tmp_name))
	{
		SEND_TO_Q("Illegal account name, please try another.\r\n", d);
		SEND_TO_Q("Account Name: ", d);
		return;
	}

	arg = tmp_name;

	if (!sql_player_exists(arg) && pfile_exists(BADNAME_DIR, arg))
	{
		SEND_TO_Q("That name has been declined before, and would be now too!\r\nName:", d);
		return;
	}
	if (sql_player_exists(arg))
	{
		SEND_TO_Q("Name is in use already. Please enter new name.\r\nName:", d);
		return;
	}
	else if (pfile_exists(BADNAME_DIR, arg))
	{
		SEND_TO_Q("That name has been declined before, and would be now too!\r\nName:", d);
		return;
	}
	if (IS_SET(game_locked, LOCK_CREATION))
	{
		SEND_TO_Q("Game is currently not allowing creation of new characters.\r\n"
			  "Please use an existing character, or try again later.\r\n\r\n",
			  d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}
	else if (bannedsite(d->host, 1))
	{
		SEND_TO_Q(
			"New characters have been banned from your site. If you want the ban lifted\r\n"
			"mail duris@duris.org with a _LENGTHY_ explanation about\r\n"
			"why, or who could have forced us to ban the site in the first place.\r\n"
			"          - The Management \r\n\r\n",
			d);
		banlog(AVATAR, "&+yNew Character reject from %s, banned.", d->host);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}
	else if ((game_locked & LOCK_CONNECTIONS) ||
		 ((game_locked & LOCK_MAX_PLAYERS) &&
		  (number_of_players() >= MAX_PLAYERS_BEFORE_LOCK)))
	{
		SEND_TO_Q("Game is temporarily full.  Please try again later.\r\n", d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}

	// Ok, we got this far, so it's ok to make a character, yay!
	if (d->character)
	{
		player = d->character;
	}
	else
	{
		player = (P_char)mm_get(dead_mob_pool);

		clear_char(player);

		if (!dead_pconly_pool)
			dead_pconly_pool =
				mm_create("PC_ONLY", sizeof(struct pc_only_data),
					  offsetof(struct pc_only_data, switched),
					  mm_find_best_chunk(sizeof(struct pc_only_data), 10, 25));

		player->only.pc = (struct pc_only_data *)mm_get(dead_pconly_pool);
		player->desc = d;

		d->character = player;
	}

	strlcpy(d->character->only.pc->pwd, d->account->acct_password,
		sizeof(d->character->only.pc->pwd));
	d->character->player.name = str_dup(arg);
	normalize_player_name_case(d->character->player.name);
	SEND_TO_Q("You chose the name ", d);
	SEND_TO_Q(d->character->player.name, d);
	SEND_TO_Q("  Is this correct?  (Y/N)  ", d);
	STATE(d) = CON_NAME_CONF;
	return;
}

void add_char_to_account(P_desc d)
{
	P_char player = d->character;
	struct acct_chars *c = NULL;

	CREATE(c, struct acct_chars, 1, MEM_TAG_OTHER);
	if (!c)
		return;

	c->pid = GET_PID(player);
	c->charname = str_dup(player->player.name);
	c->count = 1;
	c->last = time(NULL);
	c->blocked = 0;
	if (GET_RACEWAR(player) == RACEWAR_EVIL)
		c->racewar = ACCT_EVIL;
	else
		c->racewar = ACCT_GOOD;
	c->level = GET_LEVEL(player);
	c->race = GET_RACE(player);
	c->m_class = player->player.m_class;
	c->secondary_class = player->player.secondary_class;
	c->next = d->account->acct_character_list;
	d->account->acct_character_list = c;

#ifdef __NO_MYSQL__
	if (-1 == write_account(d->account))
	{
		statuslog(56, "&+RALERT&n: account character-entry save failed");
		persistence_alert(AVATAR, "account", "redacted", "none", "none", "write_failed",
				  "add character save failed");
	}
#else
	/* The new pid has been allocated, but no player_data row exists yet. Writing the
	 * account here skips that unresolved mapping and then reloads the account, which
	 * discards this live entry until the next login. The first character save commits
	 * player_data plus account_characters and publishes the completed projection. */
#endif
}

int sync_account_character_projection(P_char player, int room, int persist)
{
	if (!player || !player->desc || !player->desc->account || !GET_NAME(player))
		return 1;

	struct acct_chars *character =
		find_char_in_list(player->desc->account->acct_character_list, GET_NAME(player));
	if (!character)
		return 0;

	character->pid = GET_PID(player);
	character->level = GET_LEVEL(player);
	character->race = GET_RACE(player);
	character->m_class = player->player.m_class;
	character->secondary_class = player->player.secondary_class;
	character->racewar = GET_RACEWAR(player) == RACEWAR_EVIL ? ACCT_EVIL : ACCT_GOOD;
	if (room != NOWHERE)
		character->last_room = room;
	character->last_save = time(NULL);

	return !persist || write_account(player->desc->account) == 1;
}

void account_delete_char(P_desc d, char *arg)
{
	P_char ch = NULL;
	struct acct_chars *c = NULL;
	struct acct_chars *sorted_chars[MAX_CHARS_PER_ACCOUNT];
	struct acct_chars *temp;
	char buf[256];
	int selection, count = 0, i, j;

	if (!arg)
	{
		// First call - display the character list
		display_delete_character_list(d);
		return;
	}

	// Check if confirming deletion (yes/no)
	if (!strcasecmp(arg, "y") || !strcasecmp(arg, "yes"))
	{
		if (!d->character)
		{
			SEND_TO_Q("\r\n&+ROdd, couldn't delete that char.&n\r\n", d);
			STATE(d) = CON_DISPLAY_ACCT_MENU;
			display_account_menu(d, NULL);
			return;
		}
		SEND_TO_Q("\r\n&+RDeleting character...&n\r\n\r\n", d);
		statuslog(d->character->player.level, "%s deleted %sself (%s).",
			  GET_NAME(d->character), GET_SEX(d->character) == SEX_MALE ? "him" : "her",
			  d->host);
		logit(LOG_PLAYER, "%s deleted %sself (%s).", GET_NAME(d->character),
		      GET_SEX(d->character) == SEX_MALE ? "him" : "her", d->host);
		deleteCharacter(d->character);
		d->character = NULL; // Clear dangling pointer
		d->term_type = TERM_ANSI; // Preserve ANSI terminal mode
		SEND_TO_Q("&+GCharacter deleted successfully.&n\r\n\r\n", d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}
	else if (!strcasecmp(arg, "n") || !strcasecmp(arg, "no"))
	{
		SEND_TO_Q("\r\n&+GDeletion cancelled.&n\r\n", d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}

	// Check if user wants to go back (0 or "back")
	if (!strcasecmp(arg, "0") || !strcasecmp(arg, "back"))
	{
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}

	// Parse numeric selection
	selection = atoi(arg);

	if (selection <= 0)
	{
		SEND_TO_Q("\r\n&+RInvalid selection.&n Please enter a number or 0 to cancel.\r\n",
			  d);
		display_delete_character_list(d);
		return;
	}

	// Build sorted character list (same sorting as display)
	temp = d->account->acct_character_list;
	while (temp && count < MAX_CHARS_PER_ACCOUNT)
	{
		sorted_chars[count++] = temp;
		temp = temp->next;
	}

	// Sort by last login time (most recent first)
	for (i = 0; i < count - 1; i++)
	{
		for (j = 0; j < count - i - 1; j++)
		{
			if (sorted_chars[j]->last < sorted_chars[j + 1]->last)
			{
				struct acct_chars *swap = sorted_chars[j];
				sorted_chars[j] = sorted_chars[j + 1];
				sorted_chars[j + 1] = swap;
			}
		}
	}

	// Validate selection range
	if (selection > count)
	{
		SEND_TO_Q("\r\n&+RInvalid selection.&n Please choose a number from the list.\r\n",
			  d);
		display_delete_character_list(d);
		return;
	}

	// Get the selected character (adjust for 0-based indexing)
	c = sorted_chars[selection - 1];
	ch = load_char_into_game(c, d);

	if (!ch)
	{
		SEND_TO_Q("\r\n&+RCouldn't load that character!&n\r\n", d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}

	// Capitalize character name for display
	char name_cap[128];
	strlcpy(name_cap, c->charname, sizeof name_cap);
	if (name_cap[0])
		name_cap[0] = toupper(name_cap[0]);

	// Confirm deletion
	checked_snprintf(
		buf, 256,
		"\r\n&+R!!! FINAL WARNING !!!&n\r\n"
		"Are you &+RABSOLUTELY SURE&n you want to &+RPERMANENTLY DELETE&n &+W%s&n?\r\n"
		"This action &+RCANNOT BE UNDONE!&n\r\n\r\n"
		"Type &+WYES&n to confirm deletion, or &+WNO&n to cancel: ",
		name_cap);
	SEND_TO_Q(buf, d);
	d->character = ch;
	return;
}

void remove_char_from_list(P_acct acct, char *ch)
{
	struct acct_chars *c = NULL;
	struct acct_chars *prev = NULL;

	if (!acct || !ch || !acct->acct_character_list)
		return;

	c = acct->acct_character_list;

	if (!strcasecmp(ch, c->charname))
	{
		acct->acct_character_list = c->next;
		FREE(c->charname);
		FREE(c);
		acct->num_chars--;
		if (-1 == write_account(acct))
		{
			statuslog(56, "&+RALERT&n: account character-removal save failed");
			persistence_alert(AVATAR, "account", "redacted", "none", "none",
					  "write_failed", "remove char save failed");
		}
		return;
	}

	prev = c;
	c = c->next;
	while (c)
	{
		if (!strcasecmp(ch, c->charname))
		{
			prev->next = c->next;
			FREE(c->charname);
			FREE(c);
			acct->num_chars--;
			if (-1 == write_account(acct))
			{
				statuslog(56, "&+RALERT&n: account character-removal save failed");
				persistence_alert(AVATAR, "account", "redacted", "none", "none",
						  "write_failed", "remove char save failed");
			}
			return;
		}
		prev = c;
		c = c->next;
	}
}

void account_display_info(P_desc d, char * /*arg*/)
{
	display_account_information(d);
	STATE(d) = CON_DISPLAY_ACCT_MENU;
	display_account_menu(d, NULL);
	return;
}

void delete_account(P_desc d, char *arg)
{
	if (!d || !d->account || !d->account->acct_name)
		return;
	if (!arg)
	{
		SEND_TO_Q("\r\nRe-enter your account password to begin permanent account deletion, "
			  "or type CANCEL: ",
			  d);
		d->prompt_mode = TRUE;
		echo_off(d);
		return;
	}
	while (isspace(static_cast<unsigned char>(*arg)))
		arg++;
	if (!strcasecmp(arg, "cancel"))
	{
		echo_on(d);
		SEND_TO_Q("\r\nAccount deletion cancelled.\r\n", d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}
	if (!account_password_matches(d->account, arg))
	{
		echo_on(d);
		SEND_TO_Q("\r\nInvalid password. Account deletion cancelled.\r\n", d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}
	echo_on(d);
	STATE(d) = CON_ACCT_VERIFY_DELETE_ACCT;
	display_account_deletion_confirmation(d, false);
}

void verify_delete_account(P_desc d, char *arg)
{
	if (!d || !d->account || !d->account->acct_name)
		return;
	const bool fenced = d->account->acct_blocked == ACCOUNT_BLOCK_DELETION;
	if (!arg)
	{
		display_account_deletion_confirmation(d, fenced);
		return;
	}
	while (isspace(static_cast<unsigned char>(*arg)))
		arg++;
	char *end = arg + strlen(arg);
	while (end > arg && isspace(static_cast<unsigned char>(end[-1])))
		*--end = '\0';
	if (!strcasecmp(arg, "cancel"))
	{
		if (fenced)
		{
			SEND_TO_Q("\r\nDeletion has already started and cannot be cancelled. "
				  "Retry the exact account name or contact an immortal.\r\n",
				  d);
			display_account_deletion_confirmation(d, true);
			return;
		}
		SEND_TO_Q("\r\nAccount deletion cancelled.\r\n", d);
		STATE(d) = CON_DISPLAY_ACCT_MENU;
		display_account_menu(d, NULL);
		return;
	}
	if (strcmp(arg, d->account->acct_name))
	{
		SEND_TO_Q("\r\nThe account name did not match exactly.\r\n", d);
		display_account_deletion_confirmation(d, fenced);
		return;
	}

	if (!fenced)
	{
		const char previous_block = d->account->acct_blocked;
		d->account->acct_blocked = ACCOUNT_BLOCK_DELETION;
		if (write_account(d->account) != 1)
		{
			d->account->acct_blocked = previous_block;
			SEND_TO_Q(
				"\r\nAccount deletion could not establish its durable fence; no data "
				"was deleted. Please contact an immortal.\r\n",
				d);
			STATE(d) = CON_DISPLAY_ACCT_MENU;
			display_account_menu(d, NULL);
			return;
		}
		statuslog(56, "account deletion fenced (account=redacted)");
	}

	std::vector<account_deletion_identity> identities;
	if (!capture_account_deletion_identities(d, &identities))
	{
		SEND_TO_Q("\r\nAccount deletion could not capture stable character identities.\r\n",
			  d);
		display_account_deletion_confirmation(d, true);
		return;
	}
	const std::string account_name = d->account->acct_name;
	if (account_deletion_locker_runtime_active(account_name, identities))
	{
		SEND_TO_Q("\r\nAccount deletion is waiting for an open locker to finish saving. "
			  "Please retry shortly.\r\n",
			  d);
		display_account_deletion_confirmation(d, true);
		return;
	}
	close_other_account_sessions(d);

	bool deleted = false;
	{
		account_deletion_drain_guard drain_guard;
		flush_pending_ship_saves();
		if (drain_pending_ship_saves() && drain_guard.drain())
		{
#ifndef __NO_MYSQL__
			deleted = sql_delete_account(account_name.c_str());
#else
			std::string error;
			const auto result = flatfile_account_delete(
				persistence_mode_flatfile_root(), account_name, &error);
			deleted = result == flatfile_account_delete_result::ok ||
				  result == flatfile_account_delete_result::already_deleted;
			if (!deleted)
				logit(LOG_FILE, "flat-file account deletion failed: %s",
				      error.empty() ? "unspecified authority failure" :
						      error.c_str());
#endif
			if (deleted)
				remove_deleted_account_runtime(d, identities);
		}
	}

	if (!deleted)
	{
		persistence_alert(AVATAR, "account_delete", "redacted", "fenced", "none",
				  "delete_failed", NULL);
		SEND_TO_Q("\r\nAccount deletion did not complete. The account remains fenced; "
			  "please retry or contact an immortal.\r\n",
			  d);
		display_account_deletion_confirmation(d, true);
		return;
	}

	statuslog(56, "account deletion completed (account=redacted characters=%zu)",
		  identities.size());
	SEND_TO_Q("\r\n&+GYour account and all of its characters were permanently deleted.&n\r\n",
		  d);
	d->account = free_account(d->account);
	STATE(d) = CON_FLUSH;
}

#ifndef __NO_MYSQL__
/* Release a DTO from sql_load_account() whose contents were never transferred to
 * a live account. Its strings and list nodes come from the live-account allocator,
 * the container itself from malloc(). */
static void free_acct_entry_shallow(struct acct_entry *loaded)
{
	if (!loaded)
		return;
	loaded->acct_name = check_and_clear(loaded->acct_name);
	loaded->acct_email = check_and_clear(loaded->acct_email);
	loaded->acct_password = check_and_clear(loaded->acct_password);
	loaded->acct_confirmation = check_and_clear(loaded->acct_confirmation);
	while (loaded->acct_unique_ips)
	{
		struct acct_ip *next = loaded->acct_unique_ips->next;
		loaded->acct_unique_ips->hostname =
			check_and_clear(loaded->acct_unique_ips->hostname);
		loaded->acct_unique_ips->ip_address =
			check_and_clear(loaded->acct_unique_ips->ip_address);
		FREE(loaded->acct_unique_ips);
		loaded->acct_unique_ips = next;
	}
	while (loaded->acct_character_list)
	{
		struct acct_chars *next = loaded->acct_character_list->next;
		loaded->acct_character_list->charname =
			check_and_clear(loaded->acct_character_list->charname);
		FREE(loaded->acct_character_list);
		loaded->acct_character_list = next;
	}
	free(loaded);
}
#endif

int read_account(P_acct acct) // returns -1 if error, 1 if no errors
{
	if (!acct || !acct->acct_name)
		return -1;

	char name_backup[256];
	strlcpy(name_backup, acct->acct_name, sizeof(name_backup));

#ifndef __NO_MYSQL__
	/* Repair from durable player ownership before loading the account. Doing this
	 * on every read also recovers one missing character from an otherwise healthy
	 * multi-character account; soft-deleted mappings remain tombstoned. */
	const int repaired = sql_repair_account_character_projection(name_backup);
	if (repaired < 0)
	{
		statuslog(56, "&+RALERT&n: account character projection repair failed");
		persistence_alert(AVATAR, "account", "redacted", "none", "none",
				  "character_projection_repair_failed", NULL);
		return -1;
	}
	struct acct_entry *loaded = sql_load_account(name_backup);
#else
	std::string flatfile_error;
	struct acct_entry *loaded = flatfile_account_state_load(name_backup, &flatfile_error);
#endif
	if (!loaded)
	{
		logit(LOG_FILE, "account load failed");
		return -1;
	}

	/* A positive repair must be visible to the immediately following load. */
#ifndef __NO_MYSQL__
	if (repaired > 0)
	{
		if (!loaded->acct_character_list)
		{
			free_acct_entry_shallow(loaded);
			statuslog(56, "&+RALERT&n: repaired account projection did not reload");
			persistence_alert(AVATAR, "account", "redacted", "none", "none",
					  "character_projection_repair_unreadable", NULL);
			return -1;
		}
		statuslog(56, "account character projection repaired (affected=%d)", repaired);
	}
#endif

	/* Flat-file mode can atomically republish still-live membership when a reload
	 * unexpectedly returns empty. */
#ifdef __NO_MYSQL__
	if (!loaded->acct_character_list)
	{
		if (acct->acct_character_list)
		{
			flatfile_account_state_release(loaded);
			loaded = NULL;
			flatfile_error.clear();
			if (!flatfile_account_state_save(acct, &flatfile_error))
			{
				statuslog(56,
					  "&+RALERT&n: account character projection repair failed");
				persistence_alert(AVATAR, "account", "redacted", "none", "none",
						  "character_projection_repair_failed", NULL);
				return -1;
			}
			loaded = flatfile_account_state_load(name_backup, &flatfile_error);
			if (!loaded || !loaded->acct_character_list)
			{
				if (loaded)
					flatfile_account_state_release(loaded);
				statuslog(56,
					  "&+RALERT&n: repaired account projection did not reload");
				persistence_alert(AVATAR, "account", "redacted", "none", "none",
						  "character_projection_repair_unreadable", NULL);
				return -1;
			}
		}
	}
#endif

	// free old data
	acct->acct_name = check_and_clear(acct->acct_name);
	acct->acct_email = check_and_clear(acct->acct_email);
	acct->acct_password = check_and_clear(acct->acct_password);
	acct->acct_confirmation = check_and_clear(acct->acct_confirmation);

	if (acct->acct_unique_ips)
	{
		struct acct_ip *curr_ip, *next_ip;
		for (curr_ip = acct->acct_unique_ips; curr_ip; curr_ip = next_ip)
		{
			curr_ip->hostname = check_and_clear(curr_ip->hostname);
			curr_ip->ip_address = check_and_clear(curr_ip->ip_address);
			next_ip = curr_ip->next;
			FREE(curr_ip);
		}
		acct->acct_unique_ips = NULL;
	}
	if (acct->acct_character_list)
	{
		struct acct_chars *curr_char, *next_char;
		for (curr_char = acct->acct_character_list; curr_char; curr_char = next_char)
		{
			curr_char->charname = check_and_clear(curr_char->charname);
			next_char = curr_char->next;
			FREE(curr_char);
		}
		acct->acct_character_list = NULL;
	}

	/*
	 * MariaDB materializes strings and list nodes through the live-account
	 * allocator, so its pointers can transfer directly.  The flat-file adapter
	 * returns an isolated standard-library DTO; copy that data before releasing
	 * the DTO so the live account always has one allocator contract.
	 */
#ifdef __NO_MYSQL__
	acct->acct_name = str_dup(loaded->acct_name ? loaded->acct_name : "");
	acct->acct_email = str_dup(loaded->acct_email ? loaded->acct_email : "");
	acct->acct_password = str_dup(loaded->acct_password ? loaded->acct_password : "");
	acct->acct_confirmation =
		str_dup(loaded->acct_confirmation ? loaded->acct_confirmation : "");

	struct acct_ip **ip_tail = &acct->acct_unique_ips;
	for (struct acct_ip *source = loaded->acct_unique_ips; source; source = source->next)
	{
		struct acct_ip *copy;
		CREATE(copy, struct acct_ip, 1, MEM_TAG_OTHER);
		memset(copy, 0, sizeof(*copy));
		copy->hostname = str_dup(source->hostname ? source->hostname : "");
		copy->ip_address = str_dup(source->ip_address ? source->ip_address : "");
		copy->count = source->count;
		*ip_tail = copy;
		ip_tail = &copy->next;
	}

	struct acct_chars **character_tail = &acct->acct_character_list;
	for (struct acct_chars *source = loaded->acct_character_list; source; source = source->next)
	{
		struct acct_chars *copy;
		CREATE(copy, struct acct_chars, 1, MEM_TAG_OTHER);
		memset(copy, 0, sizeof(*copy));
		copy->pid = source->pid;
		copy->charname = str_dup(source->charname ? source->charname : "");
		copy->count = source->count;
		copy->last = source->last;
		copy->blocked = source->blocked;
		copy->racewar = source->racewar;
		copy->level = source->level;
		copy->race = source->race;
		copy->m_class = source->m_class;
		copy->secondary_class = source->secondary_class;
		copy->last_room = source->last_room;
		copy->last_save = source->last_save;
		*character_tail = copy;
		character_tail = &copy->next;
	}
#else
	acct->acct_name = loaded->acct_name;
	acct->acct_email = loaded->acct_email;
	acct->acct_password = loaded->acct_password;
	acct->acct_confirmation = loaded->acct_confirmation;
	acct->acct_unique_ips = loaded->acct_unique_ips;
	acct->acct_character_list = loaded->acct_character_list;
#endif
	acct->num_ips = loaded->num_ips;
	acct->num_chars = loaded->num_chars;
	acct->acct_blocked = loaded->acct_blocked;
	acct->acct_confirmed = loaded->acct_confirmed;
	acct->acct_confirmation_sent = loaded->acct_confirmation_sent;
	acct->acct_last = loaded->acct_last;
	acct->acct_good = loaded->acct_good;
	acct->acct_evil = loaded->acct_evil;
	acct->acct_flags1 = loaded->acct_flags1;
	acct->acct_flags2 = loaded->acct_flags2;
	acct->acct_flags3 = loaded->acct_flags3;
	acct->acct_flags4 = loaded->acct_flags4;
	acct->persistence_revision = loaded->persistence_revision;

	/* Release the DTO container with the allocator that created it. */
#ifdef __NO_MYSQL__
	flatfile_account_state_release(loaded);
#else
	free(loaded);
#endif
	return 1;
}

int write_account(P_acct acct) // returns -1 if error, 1 if no errors
{
	P_desc d = NULL;

	if (!acct)
		return -1;

#ifndef __NO_MYSQL__
	if (!sql_save_account(acct))
	{
		logit(LOG_FILE, "sql_save_account failed");
		return -1;
	}
#else
	std::string flatfile_error;
	if (!flatfile_account_state_save(acct, &flatfile_error))
	{
		logit(LOG_FILE, "flat-file account save failed");
		return -1;
	}
#endif

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->account && acct->acct_name && d->account->acct_name &&
		    !strcasecmp(acct->acct_name, d->account->acct_name))
			read_account(d->account);
	}
	return 1;
}

void write_unique_ip(P_acct acct, FILE *f)
{
	int count = 0;
	struct acct_ip *c = NULL;

	if (acct->acct_name)
	{
		if (!sql_save_account_ips(acct->acct_name, acct->acct_unique_ips))
			logit(LOG_DEBUG, "write_unique_ip: account IP save failed");
	}

	c = acct->acct_unique_ips;
	if (!c)
	{
		fprintf(f, "0\n");
		return;
	}

	while (c)
	{
		count++;
		c = c->next;
	}

	fprintf(f, "%d\n", count);
	c = acct->acct_unique_ips;
	while (c)
	{
		fprintf(f, "%s\n%s\n%li\n", c->hostname, c->ip_address, c->count);
		c = c->next;
	}
}

void read_unique_ip(P_acct acct, FILE *f)
{
	int count = 0;
	int i;
	struct acct_ip *c = NULL;
	struct acct_ip *d = NULL;
	char buf[256];

	REQUIRED_FSCANF(f, "%d\n", &count);
	if (count == 0)
		return;

	for (i = 0; i < count; i++)
	{
		CREATE(c, struct acct_ip, 1, MEM_TAG_OTHER);
		if (!c)
			return;

		REQUIRED_FSCANF(f, "%s\n", buf);
		c->hostname = str_dup(buf);
		REQUIRED_FSCANF(f, "%s\n", buf);
		c->ip_address = str_dup(buf);
		REQUIRED_FSCANF(f, "%lu\n", &c->count);
		if (i == 0)
			acct->acct_unique_ips = c;
		if (d)
			d->next = c;
		d = c;
	}
}

void write_character_list(P_acct acct, FILE *f)
{
	int count = 0;
	struct acct_chars *c = NULL;

	c = acct->acct_character_list;
	if (!c)
	{
		fprintf(f, "0\n");
		return;
	}

	while (c)
	{
		count++;
		c = c->next;
	}

	fprintf(f, "%d\n", count);
	c = acct->acct_character_list;
	while (c)
	{
		fprintf(f, "%s\n%li %li %d %d\n", c->charname, c->count, c->last, c->blocked,
			c->racewar);
		c = c->next;
	}
}

void read_character_list(P_acct acct, FILE *f)
{
	int count = 0;
	int i;
	int blocked;
	int racewar_value;
	struct acct_chars *c = NULL;
	struct acct_chars *d = NULL;
	char buf[256];

	REQUIRED_FSCANF(f, "%d\n", &count);
	if (count == 0)
		return;

	for (i = 0; i < count; i++)
	{
		CREATE(c, struct acct_chars, 1, MEM_TAG_OTHER);
		if (!c)
			return;

		REQUIRED_FSCANF(f, "%s\n", buf);
		c->charname = str_dup(buf);
		REQUIRED_FSCANF(f, "%lu %ld %d %d\n", &c->count, &c->last, &blocked,
				&racewar_value);
		c->blocked = static_cast<char>(blocked);
		c->racewar = static_cast<char>(racewar_value);
		if (i == 0)
			acct->acct_character_list = c;
		if (d)
			d->next = c;
		d = c;
	}
}

void generate_account_confirmation_code(P_desc d, char * /*arg*/)
{
	char a[256], b[256];

	snprintf(a, 256, "%d%d", number(0, 32767), number(0, 2147483647));
	snprintf(b, 256, "%s", CRYPT2(a, d->account->acct_name));

	d->account->acct_confirmation = str_dup(b);
	if (-1 == write_account(d->account))
	{
		statuslog(56, "&+RALERT&n: account confirmation-token save failed");
		persistence_alert(AVATAR, "account", "redacted", "none", "none", "write_failed",
				  "confirmation token save failed");
	}

	// Display confirmation code on screen
	char display_buf[1024];
	snprintf(display_buf, 1024,
		 "\r\n&+Y========================================&n\r\n"
		 "&+W*** Account Confirmation Code ***&n\r\n"
		 "&+Y========================================&n\r\n\r\n"
		 "Your confirmation code is: &+C%s&n\r\n\r\n"
		 "&+YPLEASE WRITE THIS DOWN!&n\r\n"
		 "&+Y========================================&n\r\n\r\n",
		 d->account->acct_confirmation);
	SEND_TO_Q(display_buf, d);

#ifdef REQUIRE_EMAIL_VERIFICATION
	// Only send email if verification is enabled
	snprintf(a, 256, "/tmp/%s.confirmation", d->account->acct_name);
	f = fopen(a, "w");
	if (!f)
	{
		ereglog(AVATAR, "Couldn't open account confirmation temp file!");
		SEND_TO_Q(
			"&+YWarning: Could not send confirmation email, but you can still use the code displayed above.&n\r\n",
			d);
		return;
	}

	fprintf(f, "  *** Duris Account Confirmation Code ***\n\n\n");
	fprintf(f, "Your account confirmation code is:  %s\n", d->account->acct_confirmation);
	fclose(f);

	snprintf(b, 256, "mail -s \"%s\" %s < %s", "Duris Account Confirmation",
		 d->account->acct_email, a);
	system(b);
	unlink(a);

	f = fopen(ACCOUNT_EMAIL_DB, "a");
	if (!f)
	{
		statuslog(56, "Couldn't open Email DB!");
	}
	else
	{
		fprintf(f, "%s\n", d->account->acct_email);
		fclose(f);
	}

	SEND_TO_Q(
		"&+GAn email with your confirmation code has also been sent to your email address.&n\r\n",
		d);
#else
	// Email verification disabled - auto-confirm account
	SEND_TO_Q(
		"&+G(Email verification is disabled - your account is automatically confirmed)&n\r\n",
		d);
	d->account->acct_confirmed = 1;
	if (-1 == write_account(d->account))
	{
		statuslog(56, "&+RALERT&n: account auto-confirm persistence failed");
		persistence_alert(AVATAR, "account", "redacted", "none", "none", "write_failed",
				  "auto-confirm save failed");
	}
#endif

	return;
}

void display_account_information_to_char(P_char ch, P_acct account)
{
	display_account_information(ch->desc, account);
}

void display_account_information(P_desc d, P_acct account)
{
	char buffer[4096];
	if (!account)
		account = d->account;

	snprintf(buffer, 4096, "Account Name:              %s\r\n", account->acct_name);
	SEND_TO_Q(buffer, d);
	snprintf(buffer, 4096, "Email Address:             %s\r\n", account->acct_email);
	SEND_TO_Q(buffer, d);
}

char is_account_confirmed(P_desc d)
{
	if (d->account && d->account->acct_confirmed)
		return 1;
	else
		return 0;
}

void clear_account(P_acct acct)
{
	struct acct_ip *curr_ip = NULL;
	struct acct_ip *next_ip = NULL;
	struct acct_chars *curr_char = NULL;
	struct acct_chars *next_char = NULL;

	acct->acct_name = check_and_clear(acct->acct_name);
	acct->acct_email = check_and_clear(acct->acct_email);
	acct->acct_password = check_and_clear(acct->acct_password);
	acct->acct_confirmation = check_and_clear(acct->acct_confirmation);

	if (acct->acct_unique_ips)
	{
		for (curr_ip = acct->acct_unique_ips; curr_ip; curr_ip = next_ip)
		{
			curr_ip->hostname = check_and_clear(curr_ip->hostname);
			curr_ip->ip_address = check_and_clear(curr_ip->ip_address);
			next_ip = curr_ip->next;
			FREE(curr_ip);
		}
		acct->acct_unique_ips = NULL;
	}

	if (acct->acct_character_list)
	{
		for (curr_char = acct->acct_character_list; curr_char; curr_char = next_char)
		{
			curr_char->charname = check_and_clear(curr_char->charname);
			next_char = curr_char->next;
			FREE(curr_char);
		}
		acct->acct_character_list = NULL;
	}

	acct->acct_blocked = 0;
	acct->acct_confirmed = 0;
	acct->acct_confirmation_sent = 0;

	acct->acct_last = 0;
	acct->acct_good = 0;
	acct->acct_evil = 0;

	acct->acct_flags1 = 0;
	acct->acct_flags2 = 0;
	acct->acct_flags3 = 0;
	acct->acct_flags4 = 0;
	acct->persistence_revision = 0;

	acct->next = NULL;
}

char *check_and_clear(char *ptr)
{
	if (ptr)
		FREE(ptr);
	return NULL;
}

P_acct free_account(P_acct acct)
{
	if (acct)
	{
		remove_account_from_list(acct);
		clear_account(acct);
		FREE(acct);
	}
	return NULL;
}

P_acct allocate_account(void)
{
	P_acct acct = NULL;

	CREATE(acct, acct_entry, 1, MEM_TAG_OTHER);

	memset(acct, 0, sizeof(acct_entry));
	add_account_to_list(acct);

	return acct;
}

void add_account_to_list(P_acct acct)
{
	if (!acct)
		return;

	if (account_list == NULL)
	{
		account_list = acct;
		return;
	}
	else
	{
		acct->next = account_list;
		account_list = acct;
	}
}

void remove_account_from_list(P_acct acct)
{
	P_acct i = NULL;

	if (!acct)
		return;

	i = account_list;
	if (i == acct)
	{
		account_list = i->next;
		return;
	}
	while (i && (i->next != acct))
	{
		i = i->next;
	}

	if (i)
	{
		i->next = acct->next;
	}
	return;
}

bool account_exists(const char *dir, char *name)
{
#ifndef __NO_MYSQL__
	// check database first
	if (sql_account_exists(name))
		return TRUE;
#else
	bool exists = false;
	std::string flatfile_error;
	if (!flatfile_account_state_exists(name, &exists, &flatfile_error))
		return FALSE;
	return exists;
#endif

	// fallback to file check
	char buf[256], *buff;
	struct stat statbuf;
	char Gbuf1[MAX_STRING_LENGTH];

	strcpy(buf, name);
	buff = buf;
	for (; *buff; buff++)
		*buff = LOWER(*buff);
	snprintf(Gbuf1, MAX_STRING_LENGTH, "%s/%c/%s", dir, buf[0], buf);
	if (stat(Gbuf1, &statbuf) != 0)
	{
		snprintf(Gbuf1, MAX_STRING_LENGTH, "%s/%c/%s", dir, buf[0], name);
		if (stat(Gbuf1, &statbuf) != 0)
			return FALSE;
	}
	return TRUE;
}

/* Helper function to get account name safely */
const char *get_account_name_safe(P_char ch)
{
	// If character has descriptor with account, return account name
	if (ch->desc && ch->desc->account && ch->desc->account->acct_name)
		return ch->desc->account->acct_name;

	// Character not connected or no account - return "Unknown"
	// This can happen when updating offline characters or during loading
	return "Unknown";
}
