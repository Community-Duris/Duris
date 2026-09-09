#pragma once

/*
 * Account password recovery by email.
 *
 * Value-taking core (src/account/account_recovery.c, MAIN THREAD ONLY) plus the
 * telnet adapter (src/account/account_recovery_nanny.c).  This header is reached
 * by every translation unit in the tree (core/structs.h -> account/account.h ->
 * here), so it stays include-light: C headers only and a forward declaration of
 * descriptor_data.  Nothing here touches libcurl.
 */

#include <stddef.h>
#include <stdint.h>
#include <time.h>

struct descriptor_data;

/* Canonical account name: ASCII tolower of the name read_account accepted, 1..64 bytes. */
#define ACCOUNT_RECOVERY_NAME_MAX 64
#define ACCOUNT_RECOVERY_NAME_BUF (ACCOUNT_RECOVERY_NAME_MAX + 1)
/* RAND_bytes(16) -> 32 lowercase hex digits (128 bits). */
#define ACCOUNT_RECOVERY_CODE_HEX_LEN 32
#define ACCOUNT_RECOVERY_CODE_BUF (ACCOUNT_RECOVERY_CODE_HEX_LEN + 1)
/* SHA256 over (password hash, email) captured when the code was issued. */
#define ACCOUNT_RECOVERY_FINGERPRINT_LEN 32
/* A code lives 15 minutes -- the default idle kick for a login-stage descriptor. */
#define ACCOUNT_RECOVERY_TTL_SEC 900
/* One mail per account per 10 minutes; the entry survives as a tombstone until then. */
#define ACCOUNT_RECOVERY_COOLDOWN_SEC 600
#define ACCOUNT_RECOVERY_MAX_TOKEN_ATTEMPTS 5
#define ACCOUNT_RECOVERY_MAX_DESCRIPTOR_ATTEMPTS 5
#define ACCOUNT_RECOVERY_MAX_ENTRIES 1024
#define ACCOUNT_RECOVERY_HOST_MAX_REQUESTS 5
#define ACCOUNT_RECOVERY_HOST_WINDOW_SEC 600
#define ACCOUNT_RECOVERY_HOST_SLOTS 1024

/* The one player-visible answer to a reset request, whatever actually happened. */
#define ACCOUNT_RECOVERY_UNIFORM_TEXT                                                                     \
	"\r\nIf that account has an email address on file, a reset code has been sent to it.\r\n"         \
	"The code is valid for 15 minutes and only for this server run. At most one code is sent per\r\n" \
	"account every 10 minutes. If nothing arrives, check your spam folder, then type CANCEL and\r\n"  \
	"press ? again once 10 minutes have passed. If the game restarts, request a new code the same "   \
	"way.\r\n"

enum class account_recovery_request_outcome : uint8_t
{
	disabled, /* feature off: adapters print the not-available line */
	invalid_name, /* canonical name rejected: uniform text */
	queued, /* mail handed to the sender: uniform text */
	suppressed, /* no/invalid email, blocked, or inside the cooldown: uniform text */
	host_limited, /* per-host window full: host-keyed text, store untouched */
	capacity /* sender queue full or unavailable: entry erased, uniform text */
};

enum class account_recovery_check_outcome : uint8_t
{
	accepted, /* code matched; verified flag set; nothing consumed */
	rejected, /* mismatch, malformed, no token, expired, consumed -- one bucket */
	capped /* this attempt exhausted the token (adapters render it as rejected) */
};

enum class account_recovery_complete_outcome : uint8_t
{
	ok,
	rejected, /* no live verified token or digest mismatch */
	fenced, /* account blocked at apply time */
	superseded, /* credentials changed since the code was issued */
	load_failed, /* fresh read_account failed; token kept */
	write_failed, /* write_account failed; token kept */
	bad_hash /* not a bcrypt hash */
};

enum class account_recovery_apply_outcome : uint8_t
{
	ok,
	fenced,
	superseded,
	load_failed,
	write_failed
};

struct account_recovery_health
{
	bool enabled = false;
	uint64_t entries = 0, live = 0, tombstones = 0;
	uint64_t requests = 0, queued = 0, suppressed_no_destination = 0, suppressed_blocked = 0,
		 suppressed_cooldown = 0, host_limited = 0, capacity = 0, invalid_name = 0;
	uint64_t checks = 0, accepted = 0, rejected = 0, capped = 0, completed = 0,
		 complete_rejected = 0, fenced = 0, superseded = 0, load_failed = 0,
		 write_failed = 0;
	uint64_t invalidated = 0, forgotten = 0, expired = 0, evicted_live = 0, evicted_dead = 0,
		 host_slots_recycled = 0;
	uint64_t mail_sent = 0, mail_retryable = 0, mail_terminal = 0, mail_stale = 0;
};

/* ---- core (src/account/account_recovery.c; main thread only) ---- */
bool account_recovery_canonical_name(const char *raw, char out[ACCOUNT_RECOVERY_NAME_BUF]);
void account_recovery_credential_fingerprint(const char *password_hash, const char *email,
					     unsigned char out[ACCOUNT_RECOVERY_FINGERPRINT_LEN]);
bool account_recovery_init(void);
void account_recovery_shutdown(void);
bool account_recovery_enabled(void);
account_recovery_request_outcome
account_recovery_request(const char *acct_name, const char *acct_email_or_null, char acct_blocked,
			 const unsigned char fingerprint[ACCOUNT_RECOVERY_FINGERPRINT_LEN],
			 const char *host, uint64_t *request_id_out);
account_recovery_check_outcome
account_recovery_check(const char *acct_name, const char *typed_code,
		       char normalized_out[ACCOUNT_RECOVERY_CODE_BUF]);
account_recovery_complete_outcome account_recovery_complete(const char *acct_name,
							    const char *normalized_code,
							    const char *new_bcrypt_hash,
							    struct descriptor_data *keep_session);
void account_recovery_invalidate(const char *acct_name);
void account_recovery_forget(const char *acct_name);
size_t account_recovery_pulse(void);
account_recovery_health account_recovery_health_copy(void);
void account_recovery_set_clock_for_tests(time_t (*clock_fn)(void));
void account_recovery_reset_for_tests(void);

/* ---- telnet adapter (src/account/account_recovery_nanny.c) ---- */
void account_recovery_begin_from_password_prompt(struct descriptor_data *d);
void account_recovery_enter_code(struct descriptor_data *d, char *arg);
void account_recovery_new_password(struct descriptor_data *d, char *arg);
void account_recovery_verify_new_password(struct descriptor_data *d, char *arg);
/* CANCEL / mismatch / exit paths: cleanse the code buffer and free the pending hash; attempts kept. */
void account_recovery_descriptor_cleanse(struct descriptor_data *d);
/* close_socket only: cleanse plus attempts = 0.  Cancels no mail. */
void account_recovery_descriptor_closed(struct descriptor_data *d);
