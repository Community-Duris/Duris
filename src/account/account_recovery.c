/*
 * Account password recovery by email: the value-taking core.
 *
 * MAIN THREAD ONLY, no mutex.  A RAM token store keyed by canonical account
 * name, a per-host request window, and the boot/pulse/shutdown glue around the
 * mail_sender worker.  Nothing here loads an account or touches a descriptor;
 * the telnet and WebSocket adapters own all player I/O.  Only SHA256 digests of
 * codes persist in memory; every plaintext buffer is cleansed before it goes
 * out of scope, and no log line ever carries a code, an address, a host or an
 * account name.
 */

#include "core/prototypes.h"
#include "core/structs.h"
#include "account/account.h"
#include "account/account_recovery.h"
#include "account/password_hash.h"
#include "net/mail_sender.h"

#include <arpa/inet.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <cstring>
#include <string>
#include <unordered_map>

/* Boot dispositions: exactly one LOG_STATUS line each.  Macros, not arrays, so the format
 * checker still sees a literal and the journey test's grep finds the whole sentence. */
#define RECOVERY_LOG_NOT_CONFIGURED \
	"Account recovery: MAIL_ENABLED is not TRUE; password reset by email disabled."
#define RECOVERY_LOG_REJECTED \
	"Account recovery configuration rejected: %s; password reset by email disabled."
#define RECOVERY_LOG_SENDER_FAILED \
	"Account recovery: mail sender failed to start; password reset by email disabled."
#define RECOVERY_LOG_ENABLED "Account recovery enabled (smtp port=%ld tls=%d)."

namespace
{
static_assert(ACCOUNT_RECOVERY_FINGERPRINT_LEN == SHA256_DIGEST_LENGTH,
	      "the credential fingerprint is a raw SHA256 digest");

/* A host key is either the descriptor's literal host text or its IPv6 /64 prefix. */
constexpr size_t host_key_bytes = sizeof(descriptor_data::host);
static_assert(host_key_bytes >= INET6_ADDRSTRLEN, "an IPv6 key must fit a host slot");
static_assert(sizeof(descriptor_data::account_recovery_code) == ACCOUNT_RECOVERY_CODE_BUF,
	      "structs.h keeps the literal 33 for the descriptor code buffer");

constexpr char reset_subject[] = "Duris account password reset";

struct recovery_entry
{
	unsigned char digest[SHA256_DIGEST_LENGTH] = {}; /* SHA256(code); zeroed once !live */
	unsigned char fingerprint[ACCOUNT_RECOVERY_FINGERPRINT_LEN] = {};
	time_t issued_at = 0;
	time_t expires_at = 0;
	time_t cooldown_until = 0; /* the entry survives as a tombstone until then */
	unsigned attempts = 0;
	bool live = false;
	bool verified = false;
	uint64_t request_id = 0; /* mail job id; 0 when nothing was queued */
};

struct host_slot
{
	char host[host_key_bytes];
	time_t window_start;
	unsigned requests;
	time_t last_used;
};

/* Guarded plaintext: the code is cleansed however the scope is left. */
struct code_buffer
{
	char text[ACCOUNT_RECOVERY_CODE_BUF] = {};

	~code_buffer() { OPENSSL_cleanse(text, sizeof text); }
};

std::unordered_map<std::string, recovery_entry> entries;
host_slot host_slots[ACCOUNT_RECOVERY_HOST_SLOTS];
time_t (*clock_fn)(void) = nullptr;
time_t last_terminal_statuslog = 0;
time_t last_evict_statuslog = 0;
uint64_t noticed_evicted_live = 0;
uint64_t noticed_host_slots_recycled = 0;
unsigned pulse_calls = 0;
account_recovery_health health;

time_t now_time()
{
	return clock_fn ? clock_fn() : time(nullptr);
}

bool expired(const recovery_entry &entry, time_t now)
{
	return entry.expires_at < now;
}

/* Neither the token nor its cooldown has anything left to protect. */
bool dead(const recovery_entry &entry, time_t now)
{
	return entry.expires_at < now && entry.cooldown_until < now;
}

void kill_token(recovery_entry *entry)
{
	entry->live = false;
	entry->verified = false;
	OPENSSL_cleanse(entry->digest, sizeof entry->digest);
}

bool digest_matches(const unsigned char *stored, size_t stored_length,
		    const unsigned char *computed, size_t computed_length)
{
	if (stored_length != SHA256_DIGEST_LENGTH || computed_length != SHA256_DIGEST_LENGTH)
		return false;
	return CRYPTO_memcmp(stored, computed, SHA256_DIGEST_LENGTH) == 0;
}

bool entry_matches(const recovery_entry &entry, const unsigned char digest[SHA256_DIGEST_LENGTH])
{
	return digest_matches(entry.digest, sizeof entry.digest, digest, SHA256_DIGEST_LENGTH);
}

bool digest_of_code(const char *code, unsigned char digest[SHA256_DIGEST_LENGTH])
{
	return SHA256(reinterpret_cast<const unsigned char *>(code), ACCOUNT_RECOVERY_CODE_HEX_LEN,
		      digest) != nullptr;
}

/* RAND_bytes(16) -> 32 lowercase hex digits; only the digest outlives the caller. */
bool issue_code(char code[ACCOUNT_RECOVERY_CODE_BUF], unsigned char digest[SHA256_DIGEST_LENGTH])
{
	unsigned char random_bytes[ACCOUNT_RECOVERY_CODE_HEX_LEN / 2] = {};
	if (RAND_bytes(random_bytes, sizeof random_bytes) != 1)
		return false;
	static const char hex[] = "0123456789abcdef";
	for (size_t index = 0; index < sizeof random_bytes; ++index)
	{
		code[index * 2] = hex[random_bytes[index] >> 4];
		code[index * 2 + 1] = hex[random_bytes[index] & 0x0f];
	}
	code[ACCOUNT_RECOVERY_CODE_HEX_LEN] = '\0';
	OPENSSL_cleanse(random_bytes, sizeof random_bytes);
	return digest_of_code(code, digest);
}

/* Drop ' ' and '-' and lower-case; true only for exactly 32 hex digits. */
bool normalize_code(const char *typed, char out[ACCOUNT_RECOVERY_CODE_BUF])
{
	size_t length = 0;
	bool well_formed = typed != nullptr;
	for (const char *cursor = typed; well_formed && *cursor; ++cursor)
	{
		unsigned char c = static_cast<unsigned char>(*cursor);
		if (c == ' ' || c == '-')
			continue;
		if (c >= 'A' && c <= 'F')
			c = static_cast<unsigned char>(c + ('a' - 'A'));
		bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
		if (!hex || length >= ACCOUNT_RECOVERY_CODE_HEX_LEN)
			well_formed = false;
		else
			out[length++] = static_cast<char>(c);
	}
	if (length != ACCOUNT_RECOVERY_CODE_HEX_LEN)
		well_formed = false;
	if (!well_formed)
	{
		OPENSSL_cleanse(out, ACCOUNT_RECOVERY_CODE_BUF);
		return false;
	}
	out[length] = '\0';
	return true;
}

void host_key(const char *host, char out[host_key_bytes])
{
	/* An empty key would read as a free slot; give unknown peers one real bucket. */
	if (!host || !*host)
		host = "unknown";
	const socklen_t out_size = static_cast<socklen_t>(host_key_bytes);
	if (strchr(host, ':'))
	{
		struct in6_addr address;
		if (inet_pton(AF_INET6, host, &address) == 1)
		{
			/* comm.c strips the ::ffff: prefix, but a mapped IPv4 peer that slipped
			 * through must not collapse every IPv4 client into one "::" bucket. */
			bool mapped_v4 = address.s6_addr[10] == 0xff && address.s6_addr[11] == 0xff;
			for (size_t index = 0; mapped_v4 && index < 10; ++index)
				mapped_v4 = address.s6_addr[index] == 0;
			if (mapped_v4)
			{
				struct in_addr embedded;
				memcpy(&embedded, address.s6_addr + 12, sizeof embedded);
				if (inet_ntop(AF_INET, &embedded, out, out_size))
					return;
			}
			else
			{
				/* A subscriber usually holds a whole /64: key the window on it. */
				memset(address.s6_addr + 8, 0, 8);
				if (inet_ntop(AF_INET6, &address, out, out_size))
					return;
			}
		}
	}
	size_t length = strnlen(host, host_key_bytes - 1);
	memcpy(out, host, length);
	out[length] = '\0';
}

void claim_host_slot(host_slot *slot, const char *key, time_t now)
{
	memset(slot, 0, sizeof *slot);
	memcpy(slot->host, key, strnlen(key, sizeof slot->host - 1));
	slot->window_start = now;
	slot->last_used = now;
}

/* Find or claim the slot for a key: empty first, then an expired window, then LRU. */
host_slot *host_slot_for(const char *key, time_t now)
{
	host_slot *first_empty = nullptr;
	host_slot *expired_window = nullptr;
	host_slot *oldest = &host_slots[0];
	for (size_t index = 0; index < ACCOUNT_RECOVERY_HOST_SLOTS; ++index)
	{
		host_slot *slot = &host_slots[index];
		if (!slot->host[0])
		{
			first_empty = slot;
			break;
		}
		if (!strcmp(slot->host, key))
		{
			slot->last_used = now;
			return slot;
		}
		if (!expired_window && slot->window_start + ACCOUNT_RECOVERY_HOST_WINDOW_SEC < now)
			expired_window = slot;
		if (slot->last_used < oldest->last_used)
			oldest = slot;
	}
	host_slot *victim = first_empty ? first_empty : expired_window;
	if (!victim)
	{
		/* Displacing a live window loosens the brake; count it so it is visible. */
		victim = oldest;
		++health.host_slots_recycled;
	}
	claim_host_slot(victim, key, now);
	return victim;
}

/* Full store: drop everything dead first, then the single oldest entry. */
void make_room(time_t now)
{
	if (entries.size() < ACCOUNT_RECOVERY_MAX_ENTRIES)
		return;
	for (auto it = entries.begin(); it != entries.end();)
	{
		if (dead(it->second, now))
		{
			it = entries.erase(it);
			++health.evicted_dead;
		}
		else
			++it;
	}
	if (entries.size() < ACCOUNT_RECOVERY_MAX_ENTRIES)
		return;
	auto oldest = entries.begin();
	for (auto it = entries.begin(); it != entries.end(); ++it)
		if (it->second.issued_at < oldest->second.issued_at)
			oldest = it;
	if (oldest->second.live)
		++health.evicted_live;
	else
		++health.evicted_dead;
	entries.erase(oldest);
}

void sweep(time_t now)
{
	for (auto it = entries.begin(); it != entries.end();)
	{
		if (dead(it->second, now))
		{
			it = entries.erase(it);
			++health.expired;
		}
		else
			++it;
	}
}

std::unordered_map<std::string, recovery_entry>::iterator find_by_request_id(uint64_t request_id)
{
	if (request_id == 0)
		return entries.end();
	for (auto it = entries.begin(); it != entries.end(); ++it)
		if (it->second.request_id == request_id)
			return it;
	return entries.end();
}

/* ASCII by construction: the canonical name is printable ASCII, the code is hex. */
std::string render_body(const char *canonical_name, const char *code)
{
	std::string display(canonical_name);
	if (!display.empty() && display[0] >= 'a' && display[0] <= 'z')
		display[0] = static_cast<char>(display[0] - ('a' - 'A'));

	/* Reserve up front so no discarded reallocation keeps a copy of the code. */
	std::string body;
	body.reserve(512);
	body += "Someone asked to reset the password of the Duris account ";
	body += display;
	body += ".\r\n\r\nReset code: ";
	for (size_t index = 0; index < ACCOUNT_RECOVERY_CODE_HEX_LEN; ++index)
	{
		if (index && index % 8 == 0)
			body.push_back('-');
		body.push_back(code[index]);
	}
	body += "\r\n\r\nType it at the prompt within 15 minutes. It works once.\r\n"
		"If you did not request this, ignore this message; nothing changes.\r\n";
	return body;
}

const char *request_outcome_name(account_recovery_request_outcome outcome)
{
	switch (outcome)
	{
	case account_recovery_request_outcome::disabled:
		return "disabled";
	case account_recovery_request_outcome::invalid_name:
		return "invalid_name";
	case account_recovery_request_outcome::queued:
		return "queued";
	case account_recovery_request_outcome::suppressed:
		return "suppressed";
	case account_recovery_request_outcome::host_limited:
		return "host_limited";
	case account_recovery_request_outcome::capacity:
		return "capacity";
	}
	return "capacity";
}

const char *mail_outcome_name(mail_outcome outcome)
{
	switch (outcome)
	{
	case mail_outcome::sent:
		return "sent";
	case mail_outcome::retryable_failure:
		return "retryable";
	case mail_outcome::terminal_failure:
		return "terminal";
	}
	return "terminal";
}

/* One wizard notice per minute however many sends fail; integer codes only, never prose. */
void notice_terminal_failure(const mail_result &result, time_t now)
{
	if (last_terminal_statuslog != 0 && now - last_terminal_statuslog < 60)
		return;
	statuslog(56, "&+RALERT&n: account recovery mail failing (curl=%d smtp=%ld)",
		  result.curl_code, result.smtp_code);
	last_terminal_statuslog = now;
}

/* A store-fill or host-table flood is visible from these counters, once a minute. */
void notice_evictions(time_t now)
{
	const bool news = health.evicted_live != noticed_evicted_live ||
			  health.host_slots_recycled != noticed_host_slots_recycled;
	if (!news || (last_evict_statuslog != 0 && now - last_evict_statuslog < 60))
		return;
	statuslog(56, "account recovery store evicted live tokens=%llu host slots recycled=%llu",
		  static_cast<unsigned long long>(health.evicted_live),
		  static_cast<unsigned long long>(health.host_slots_recycled));
	last_evict_statuslog = now;
	noticed_evicted_live = health.evicted_live;
	noticed_host_slots_recycled = health.host_slots_recycled;
}

account_recovery_request_outcome
perform_request(const char *acct_name, const char *acct_email_or_null, char acct_blocked,
		const unsigned char fingerprint[ACCOUNT_RECOVERY_FINGERPRINT_LEN], const char *host,
		uint64_t *request_id_out, uint64_t *logged_id)
{
	if (!account_recovery_enabled())
		return account_recovery_request_outcome::disabled;

	char name[ACCOUNT_RECOVERY_NAME_BUF];
	if (!account_recovery_canonical_name(acct_name, name))
	{
		++health.invalid_name;
		return account_recovery_request_outcome::invalid_name;
	}

	const time_t now = now_time();

	/* The host window is spent before the store is touched, whatever happens next. */
	char key[host_key_bytes];
	host_key(host, key);
	host_slot *slot = host_slot_for(key, now);
	if (slot->window_start + ACCOUNT_RECOVERY_HOST_WINDOW_SEC < now)
	{
		slot->window_start = now;
		slot->requests = 0;
	}
	if (slot->requests >= ACCOUNT_RECOVERY_HOST_MAX_REQUESTS)
	{
		++health.host_limited;
		return account_recovery_request_outcome::host_limited;
	}
	++slot->requests;

	auto found = entries.find(name);
	if (found != entries.end() && now < found->second.cooldown_until)
	{
		++health.suppressed_cooldown;
		return account_recovery_request_outcome::suppressed;
	}
	if (found == entries.end())
	{
		make_room(now);
		found = entries.emplace(std::string(name), recovery_entry()).first;
	}
	recovery_entry &entry = found->second;
	entry = recovery_entry();

	code_buffer code;
	if (!issue_code(code.text, entry.digest))
	{
		entries.erase(found);
		++health.capacity;
		return account_recovery_request_outcome::capacity;
	}
	entry.issued_at = now;
	entry.expires_at = now + ACCOUNT_RECOVERY_TTL_SEC;
	entry.cooldown_until = now + ACCOUNT_RECOVERY_COOLDOWN_SEC;
	entry.live = true;
	if (fingerprint)
		memcpy(entry.fingerprint, fingerprint, ACCOUNT_RECOVERY_FINGERPRINT_LEN);

	/* A blocked or address-less account still leaves a tombstone: same clock, same counters. */
	if (acct_blocked != 0)
	{
		kill_token(&entry);
		++health.suppressed_blocked;
		return account_recovery_request_outcome::suppressed;
	}
	if (!acct_email_or_null || !*acct_email_or_null || !is_valid_email(acct_email_or_null))
	{
		kill_token(&entry);
		++health.suppressed_no_destination;
		return account_recovery_request_outcome::suppressed;
	}

	std::string body = render_body(name, code.text);
	const std::string to(acct_email_or_null);
	const uint64_t request_id = mail_sender_next_request_id();
	const mail_submit_outcome submitted =
		mail_sender_submit(request_id, to, reset_subject, body);
	OPENSSL_cleanse(body.data(), body.size());
	if (submitted != mail_submit_outcome::accepted)
	{
		/* Our outage must not become the player's lockout: no tombstone, retry at once. */
		entries.erase(found);
		++health.capacity;
		return account_recovery_request_outcome::capacity;
	}
	entry.request_id = request_id;
	++health.queued;
	*logged_id = request_id;
	if (request_id_out)
		*request_id_out = request_id;
	return account_recovery_request_outcome::queued;
}
} // namespace

bool account_recovery_canonical_name(const char *raw, char out[ACCOUNT_RECOVERY_NAME_BUF])
{
	if (!out)
		return false;
	out[0] = '\0';
	if (!raw || !*raw)
		return false;
	size_t length = 0;
	const unsigned char *cursor = reinterpret_cast<const unsigned char *>(raw);
	for (; *cursor; ++cursor)
	{
		unsigned char c = *cursor;
		if (length >= ACCOUNT_RECOVERY_NAME_MAX || c < 0x21 || c >= 0x7f)
		{
			out[0] = '\0';
			return false;
		}
		if (c >= 'A' && c <= 'Z')
			c = static_cast<unsigned char>(c + ('a' - 'A'));
		out[length++] = static_cast<char>(c);
	}
	out[length] = '\0';
	return true;
}

void account_recovery_credential_fingerprint(const char *password_hash, const char *email,
					     unsigned char out[ACCOUNT_RECOVERY_FINGERPRINT_LEN])
{
	if (!out)
		return;
	const char *hash_text = password_hash ? password_hash : "";
	const char *email_text = email ? email : "";
	std::string material;
	material.reserve(strlen(hash_text) + 1 + strlen(email_text));
	material.append(hash_text);
	material.push_back('\0');
	material.append(email_text);
	if (!SHA256(reinterpret_cast<const unsigned char *>(material.data()), material.size(), out))
		memset(out, 0, ACCOUNT_RECOVERY_FINGERPRINT_LEN);
	OPENSSL_cleanse(material.data(), material.size());
}

bool account_recovery_init(void)
{
	mail_sender_config config;
	const char *category = nullptr;
	bool started = false;
	if (!mail_sender_config_from_env(&config, &category))
	{
		const char *why = category ? category : "unspecified";
		if (!strcmp(why, "not configured"))
			logit(LOG_STATUS, RECOVERY_LOG_NOT_CONFIGURED);
		else
			logit(LOG_STATUS, RECOVERY_LOG_REJECTED, why);
	}
	else if (!is_valid_email(config.from.c_str()))
		logit(LOG_STATUS, RECOVERY_LOG_REJECTED, "MAIL_FROM invalid");
	else if (!mail_sender_init(config))
		logit(LOG_STATUS, RECOVERY_LOG_SENDER_FAILED);
	else
	{
		logit(LOG_STATUS, RECOVERY_LOG_ENABLED, config.port, config.tls ? 1 : 0);
		started = true;
	}
	/* The worker holds its own snapshot; this frame's copy of the secret dies here. */
	OPENSSL_cleanse(config.password.data(), config.password.size());
	return started;
}

void account_recovery_shutdown(void)
{
	mail_sender_shutdown();
	entries.clear();
	memset(host_slots, 0, sizeof host_slots);
}

/* The mailer is started only by account_recovery_init in the server; the harness starts it
 * directly with a capturing send function, so "running" is the one truth for both. */
bool account_recovery_enabled(void)
{
	return mail_sender_running();
}

account_recovery_request_outcome
account_recovery_request(const char *acct_name, const char *acct_email_or_null, char acct_blocked,
			 const unsigned char fingerprint[ACCOUNT_RECOVERY_FINGERPRINT_LEN],
			 const char *host, uint64_t *request_id_out)
{
	++health.requests;
	uint64_t logged_id = 0;
	const account_recovery_request_outcome outcome =
		perform_request(acct_name, acct_email_or_null, acct_blocked, fingerprint, host,
				request_id_out, &logged_id);
	logit(LOG_PLAYER, "account recovery request=%llu outcome=%s (account=redacted)",
	      static_cast<unsigned long long>(logged_id), request_outcome_name(outcome));
	return outcome;
}

account_recovery_check_outcome
account_recovery_check(const char *acct_name, const char *typed_code,
		       char normalized_out[ACCOUNT_RECOVERY_CODE_BUF])
{
	++health.checks;
	/* The caller's buffer holds plaintext only after an acceptance; every other exit
	 * leaves it cleansed, so a stale code never outlives the check that rejected it. */
	if (normalized_out)
		OPENSSL_cleanse(normalized_out, ACCOUNT_RECOVERY_CODE_BUF);
	char name[ACCOUNT_RECOVERY_NAME_BUF];
	if (!account_recovery_canonical_name(acct_name, name))
	{
		++health.rejected;
		return account_recovery_check_outcome::rejected;
	}
	const time_t now = now_time();
	auto found = entries.find(name);
	if (found == entries.end() || !found->second.live || expired(found->second, now))
	{
		/* Nothing to protect, so nothing to count; the adapter's one text hides which. */
		++health.rejected;
		return account_recovery_check_outcome::rejected;
	}
	recovery_entry &entry = found->second;

	code_buffer normalized;
	unsigned char digest[SHA256_DIGEST_LENGTH] = {};
	const bool matched = normalize_code(typed_code, normalized.text) &&
			     digest_of_code(normalized.text, digest) &&
			     entry_matches(entry, digest);
	OPENSSL_cleanse(digest, sizeof digest);
	if (matched)
	{
		entry.verified = true;
		if (normalized_out)
			memcpy(normalized_out, normalized.text, ACCOUNT_RECOVERY_CODE_BUF);
		++health.accepted;
		return account_recovery_check_outcome::accepted;
	}

	/* A malformed guess counts like any other: the two are indistinguishable outside. */
	if (++entry.attempts >= ACCOUNT_RECOVERY_MAX_TOKEN_ATTEMPTS)
	{
		kill_token(&entry);
		++health.capped;
		logit(LOG_PLAYER, "account recovery code exhausted (account=redacted)");
		statuslog(56, "account recovery code exhausted (account=redacted)");
		return account_recovery_check_outcome::capped;
	}
	++health.rejected;
	return account_recovery_check_outcome::rejected;
}

account_recovery_complete_outcome account_recovery_complete(const char *acct_name,
							    const char *normalized_code,
							    const char *new_bcrypt_hash,
							    struct descriptor_data *keep_session)
{
	if (!is_bcrypt_hash(new_bcrypt_hash))
		return account_recovery_complete_outcome::bad_hash;

	/* The apply helper's write_account re-reads and reallocates every same-name descriptor's
	 * account strings, and on telnet acct_name IS one of them: canonicalise first, and after
	 * the call touch only this buffer and the store. */
	char name[ACCOUNT_RECOVERY_NAME_BUF];
	if (!account_recovery_canonical_name(acct_name, name))
	{
		++health.complete_rejected;
		return account_recovery_complete_outcome::rejected;
	}
	const time_t now = now_time();
	auto found = entries.find(name);
	unsigned char digest[SHA256_DIGEST_LENGTH] = {};
	const bool valid = found != entries.end() && found->second.live && found->second.verified &&
			   !expired(found->second, now) && normalized_code &&
			   strlen(normalized_code) == ACCOUNT_RECOVERY_CODE_HEX_LEN &&
			   digest_of_code(normalized_code, digest) &&
			   entry_matches(found->second, digest);
	OPENSSL_cleanse(digest, sizeof digest);
	if (!valid)
	{
		++health.complete_rejected;
		return account_recovery_complete_outcome::rejected;
	}

	unsigned char fingerprint[ACCOUNT_RECOVERY_FINGERPRINT_LEN];
	memcpy(fingerprint, found->second.fingerprint, sizeof fingerprint);
	const account_recovery_apply_outcome applied = account_apply_recovered_password(
		acct_name, new_bcrypt_hash, fingerprint, keep_session);

	found = entries.find(name);
	recovery_entry *entry = found != entries.end() ? &found->second : nullptr;
	switch (applied)
	{
	case account_recovery_apply_outcome::ok:
		if (entry)
			kill_token(entry); /* single use; the tombstone keeps the cooldown */
		++health.completed;
		logit(LOG_PLAYER, "account recovery completed (account=redacted)");
		statuslog(56, "account password reset by email completed (account=redacted)");
		return account_recovery_complete_outcome::ok;
	case account_recovery_apply_outcome::fenced:
		if (entry)
			kill_token(entry);
		++health.fenced;
		return account_recovery_complete_outcome::fenced;
	case account_recovery_apply_outcome::superseded:
		if (entry)
			kill_token(entry);
		++health.superseded;
		return account_recovery_complete_outcome::superseded;
	case account_recovery_apply_outcome::load_failed:
		++health.load_failed; /* token kept: the player retries with the same code */
		return account_recovery_complete_outcome::load_failed;
	case account_recovery_apply_outcome::write_failed:
		++health.write_failed;
		return account_recovery_complete_outcome::write_failed;
	}
	++health.write_failed;
	return account_recovery_complete_outcome::write_failed;
}

void account_recovery_invalidate(const char *acct_name)
{
	char name[ACCOUNT_RECOVERY_NAME_BUF];
	if (!account_recovery_canonical_name(acct_name, name))
		return;
	auto found = entries.find(name);
	if (found == entries.end())
		return;
	kill_token(&found->second);
	++health.invalidated;
}

void account_recovery_forget(const char *acct_name)
{
	char name[ACCOUNT_RECOVERY_NAME_BUF];
	if (!account_recovery_canonical_name(acct_name, name))
		return;
	if (entries.erase(name))
		++health.forgotten;
}

/* Runs every 500 ms (inside comm.c's `if (!(pulse % 2))`); the expire sweep runs every
 * 4th call, about every 2 s. */
size_t account_recovery_pulse(void)
{
	mail_result results[MAIL_SENDER_MAX_COMPLETIONS];
	const size_t completed = mail_sender_pulse(results, MAIL_SENDER_MAX_COMPLETIONS);
	const time_t now = now_time();

	for (size_t index = 0; index < completed; ++index)
	{
		const mail_result &result = results[index];
		logit(LOG_PLAYER, "account recovery mail request=%llu outcome=%s curl=%d smtp=%ld",
		      static_cast<unsigned long long>(result.request_id),
		      mail_outcome_name(result.outcome), result.curl_code, result.smtp_code);
		auto found = find_by_request_id(result.request_id);
		if (found == entries.end())
			++health.mail_stale;
		switch (result.outcome)
		{
		case mail_outcome::sent:
			++health.mail_sent;
			break;
		case mail_outcome::retryable_failure:
			++health.mail_retryable;
			if (found != entries.end())
				entries.erase(found);
			break;
		case mail_outcome::terminal_failure:
			++health.mail_terminal;
			if (found != entries.end())
				entries.erase(found);
			notice_terminal_failure(result, now);
			break;
		}
	}

	if (++pulse_calls % 4 == 0)
		sweep(now);
	notice_evictions(now);
	return completed;
}

account_recovery_health account_recovery_health_copy(void)
{
	account_recovery_health copy = health;
	copy.enabled = account_recovery_enabled();
	copy.entries = entries.size();
	copy.live = 0;
	copy.tombstones = 0;
	for (const auto &item : entries)
	{
		if (item.second.live)
			++copy.live;
		else
			++copy.tombstones;
	}
	return copy;
}

void account_recovery_set_clock_for_tests(time_t (*clock_override)(void))
{
	clock_fn = clock_override;
}

void account_recovery_reset_for_tests(void)
{
	account_recovery_shutdown();
	mail_sender_reset_for_tests();
	health = account_recovery_health();
	clock_fn = nullptr;
	last_terminal_statuslog = 0;
	last_evict_statuslog = 0;
	noticed_evicted_live = 0;
	noticed_host_slots_recycled = 0;
	pulse_calls = 0;
}
