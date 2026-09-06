/*************************************************************
 * account_recovery_nanny.c
 *
 * Telnet adapter for account password recovery by email: the '?' entry at the
 * account password prompt and the CON_ACCT_RESET_* handlers.  This file owns
 * every prompt, the per-descriptor attempt counter and the pending-hash field;
 * the core (account_recovery.c) owns tokens, cooldowns and mail.
 *
 * The reset code is only ever INPUT here.  No output, log line or copy carries
 * it beyond d->account_recovery_code, which is cleansed on every exit path.
 *************************************************************/

#include "core/prototypes.h"
#include "core/structs.h"
#include "net/comm.h"
#include "core/utils.h"
#include "account/account.h"
#include "account/account_recovery.h"
#include "account/password_hash.h"
#include <ctype.h>
#include <openssl/crypto.h>
#include <string.h>
#include <strings.h>

namespace
{
/* Trims both ends in place and returns the first non-blank byte. */
char *trim_both(char *arg)
{
	for (; isspace((unsigned char)*arg); arg++)
		;
	size_t len = strlen(arg);
	while (len > 0 && isspace((unsigned char)arg[len - 1]))
		arg[--len] = '\0';
	return arg;
}

bool is_cancel(const char *arg)
{
	return strcasecmp(arg, "cancel") == 0;
}

/* Drops the parked bcrypt hash.  FREE nulls the pointer, so repeated calls are safe. */
void release_pending_hash(P_desc d)
{
	if (!d->account_recovery_pending_hash)
		return;
	OPENSSL_cleanse(d->account_recovery_pending_hash, strlen(d->account_recovery_pending_hash));
	FREE(d->account_recovery_pending_hash);
}

void send_not_available(P_desc d)
{
	SEND_TO_Q("\r\nPassword reset by email is not available on this server. If you are locked "
		  "out, ask an immortal for help.\r\n",
		  d);
	send_account_password_prompt(d);
}

/* Leaves the reset flow for the ordinary password prompt; the attempt counter is kept
 * on purpose so CANCEL cannot be used to refill the per-connection guess budget. */
void return_to_password_prompt(P_desc d)
{
	account_recovery_descriptor_cleanse(d);
	send_account_password_prompt(d);
}

void disconnect_from_recovery(P_desc d)
{
	account_recovery_descriptor_cleanse(d);
	d->account = free_account(d->account);
	STATE(d) = CON_FLUSH;
}
} // namespace

void account_recovery_descriptor_cleanse(P_desc d)
{
	if (!d)
		return;
	OPENSSL_cleanse(d->account_recovery_code, sizeof(d->account_recovery_code));
	release_pending_hash(d);
}

void account_recovery_descriptor_closed(P_desc d)
{
	if (!d)
		return;
	account_recovery_descriptor_cleanse(d);
	d->account_recovery_attempts = 0;
}

/* S1 -> S2: a lone '?' at the password prompt. */
void account_recovery_begin_from_password_prompt(P_desc d)
{
	if (!d)
		return;
	account_recovery_descriptor_cleanse(d);
	if (!d->account || !d->account->acct_name)
	{
		disconnect_from_recovery(d);
		return;
	}
	if (!account_recovery_enabled())
	{
		send_not_available(d);
		return;
	}

	/* The server never prints the code, so the prompt can stay echoed. */
	echo_on(d);

	unsigned char fingerprint[ACCOUNT_RECOVERY_FINGERPRINT_LEN];
	account_recovery_credential_fingerprint(d->account->acct_password, d->account->acct_email,
						fingerprint);
	uint64_t request_id = 0;
	const account_recovery_request_outcome outcome = account_recovery_request(
		d->account->acct_name, d->account->acct_email, d->account->acct_blocked,
		fingerprint, d->host, &request_id);
	OPENSSL_cleanse(fingerprint, sizeof(fingerprint));

	switch (outcome)
	{
	case account_recovery_request_outcome::disabled:
		send_not_available(d);
		return;
	case account_recovery_request_outcome::host_limited:
		SEND_TO_Q(
			"\r\nToo many reset requests from your connection. Wait 10 minutes and try "
			"again.\r\n",
			d);
		send_account_password_prompt(d);
		return;
	case account_recovery_request_outcome::queued:
	case account_recovery_request_outcome::suppressed:
	case account_recovery_request_outcome::capacity:
	case account_recovery_request_outcome::invalid_name:
		break;
	}

	/* One text whatever happened: no email-on-file oracle. */
	SEND_TO_Q(ACCOUNT_RECOVERY_UNIFORM_TEXT, d);
	SEND_TO_Q("Enter the reset code (or CANCEL): ", d);
	STATE(d) = CON_ACCT_RESET_CODE;
}

/* S2 CON_ACCT_RESET_CODE. */
void account_recovery_enter_code(P_desc d, char *arg)
{
	if (!d)
		return;
	if (!d->account || !d->account->acct_name)
	{
		disconnect_from_recovery(d);
		return;
	}
	if (arg)
		arg = trim_both(arg);
	if (!arg || !*arg)
	{
		SEND_TO_Q("Enter the reset code (or CANCEL): ", d);
		return;
	}
	if (is_cancel(arg))
	{
		SEND_TO_Q(
			"\r\nReset cancelled. Any code already sent stays valid until it expires.\r\n",
			d);
		return_to_password_prompt(d);
		return;
	}

	/* Per-connection cap, checked before counting so it fires on the same input number
	 * whether or not a token exists. */
	if (d->account_recovery_attempts >= ACCOUNT_RECOVERY_MAX_DESCRIPTOR_ATTEMPTS)
	{
		SEND_TO_Q("\r\nToo many attempts. Disconnecting.\r\n", d);
		disconnect_from_recovery(d);
		return;
	}
	++d->account_recovery_attempts;

	const account_recovery_check_outcome outcome =
		account_recovery_check(d->account->acct_name, arg, d->account_recovery_code);
	if (outcome == account_recovery_check_outcome::accepted)
	{
		SEND_TO_Q("\r\nCode accepted. Choose a new password.\r\n", d);
		STATE(d) = CON_ACCT_RESET_NEWPW;
		account_recovery_new_password(d, NULL);
		return;
	}

	/* rejected and capped share one text: an exhausted token must look like a miss. */
	SEND_TO_Q("\r\nThat code was not accepted.\r\nEnter the reset code (or CANCEL): ", d);
}

/* S3 CON_ACCT_RESET_NEWPW. */
void account_recovery_new_password(P_desc d, char *arg)
{
	if (!d)
		return;
	if (!arg)
	{
		echo_on(d);
		SEND_TO_Q("New password (or CANCEL): ", d);
		echo_off(d);
		return;
	}
	echo_on(d);
	if (!d->account || !d->account->acct_name)
	{
		disconnect_from_recovery(d);
		return;
	}

	for (; isspace((unsigned char)*arg); arg++)
		;

	if (is_cancel(arg))
	{
		SEND_TO_Q("\r\nReset cancelled. Your code is still valid until it expires.\r\n", d);
		return_to_password_prompt(d);
		return;
	}
	if (!*arg)
	{
		SEND_TO_Q("Invalid Password, try again.\r\n", d);
		account_recovery_new_password(d, NULL);
		return;
	}
	if (!valid_password(d, arg))
	{
		account_recovery_new_password(d, NULL);
		return;
	}

	char *hash = bcrypt_hash_password(arg);
	if (!hash)
	{
		SEND_TO_Q("Error hashing password, please try again.\r\n", d);
		account_recovery_new_password(d, NULL);
		return;
	}

	/* Parked on the descriptor, never in acct_entry: write_account's re-read loop
	 * would clobber anything parked there. */
	release_pending_hash(d);
	d->account_recovery_pending_hash = str_dup(hash);
	free(hash);
	STATE(d) = CON_ACCT_RESET_NEWPW2;
	account_recovery_verify_new_password(d, NULL);
}

/* S4 CON_ACCT_RESET_NEWPW2. */
void account_recovery_verify_new_password(P_desc d, char *arg)
{
	if (!d)
		return;
	if (!arg)
	{
		echo_on(d);
		SEND_TO_Q("Verify new password: ", d);
		echo_off(d);
		return;
	}
	echo_on(d);
	if (!d->account || !d->account->acct_name)
	{
		disconnect_from_recovery(d);
		return;
	}

	for (; isspace((unsigned char)*arg); arg++)
		;

	if (is_cancel(arg))
	{
		SEND_TO_Q("\r\nReset cancelled. Your code is still valid until it expires.\r\n", d);
		return_to_password_prompt(d);
		return;
	}

	if (!d->account_recovery_pending_hash ||
	    !bcrypt_verify_password(arg, d->account_recovery_pending_hash))
	{
		SEND_TO_Q("\r\nPasswords do not match!\r\n", d);
		/* Only the hash goes: the flow returns to S3 and completion still needs the
		 * accepted code, so d->account_recovery_code is kept until an exit path. */
		release_pending_hash(d);
		STATE(d) = CON_ACCT_RESET_NEWPW;
		account_recovery_new_password(d, NULL);
		return;
	}

	const account_recovery_complete_outcome outcome =
		account_recovery_complete(d->account->acct_name, d->account_recovery_code,
					  d->account_recovery_pending_hash, d);

	/* On success write_account re-read d->account and reallocated its strings; the
	 * acct_entry itself is still ours.  Only descriptor-owned state is touched now. */
	account_recovery_descriptor_cleanse(d);

	switch (outcome)
	{
	case account_recovery_complete_outcome::ok:
		d->account_recovery_attempts = 0;
		SEND_TO_Q(
			"\r\n&+GYour password has been changed.&n Any other connection using this "
			"account has been disconnected.\r\n",
			d);
		send_account_password_prompt(d);
		return;
	case account_recovery_complete_outcome::rejected:
	case account_recovery_complete_outcome::fenced:
	case account_recovery_complete_outcome::superseded:
		/* fenced and superseded share the text on purpose: no disclosure to a code
		 * holder; the token is dead either way. */
		SEND_TO_Q("\r\nYour reset code stopped being valid while you were typing (it "
			  "expired, was cancelled, or a newer code was requested). Press ? at the "
			  "password prompt to start again.\r\n",
			  d);
		send_account_password_prompt(d);
		return;
	case account_recovery_complete_outcome::write_failed:
		/* Token kept by the core: '?' -> suppressed -> the same code still works. */
		SEND_TO_Q(
			"\r\nYour new password could not be saved. Nothing has changed and your "
			"code is still valid: press ? at the password prompt to try again, or ask "
			"an immortal.\r\n",
			d);
		send_account_password_prompt(d);
		return;
	case account_recovery_complete_outcome::load_failed:
	case account_recovery_complete_outcome::bad_hash:
		break;
	}

	SEND_TO_Q("\r\nThere is an error with your account, please notify an immortal!\r\n", d);
	d->account = free_account(d->account);
	STATE(d) = CON_FLUSH;
}
