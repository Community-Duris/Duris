#ifndef DURIS_WS_AUTH_H
#define DURIS_WS_AUTH_H

#include <openssl/crypto.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int ws_auth_rate_limited(time_t *window_start, unsigned int *attempts, unsigned int maximum,
				time_t window)
{
	time_t now = time(NULL);

	if (!window_start || !attempts)
		return 1;
	if (!*window_start || now < *window_start || now - *window_start >= window)
	{
		*window_start = now;
		*attempts = 0;
	}
	return *attempts >= maximum;
}

static void ws_auth_record_failure(time_t *window_start, unsigned int *attempts,
				   unsigned int maximum, time_t window)
{
	(void)ws_auth_rate_limited(window_start, attempts, maximum, window);
	if (*attempts < maximum)
		(*attempts)++;
}

static void ws_auth_reset(time_t *window_start, unsigned int *attempts)
{
	*window_start = 0;
	*attempts = 0;
}

/* Shared DurisWeb HMAC authentication contract for WebSocket and GMCP. */
static int ws_issue_durisweb_challenge(char challenge[65], time_t *expires)
{
	unsigned char random[32];

	if (!challenge || !expires || RAND_bytes(random, sizeof(random)) != 1)
		return 0;
	for (size_t i = 0; i < sizeof(random); i++)
		snprintf(challenge + (i * 2), 3, "%02x", random[i]);
	challenge[64] = '\0';
	*expires = time(NULL) + 30;
	return 1;
}

static int ws_verify_durisweb_signature_with_secret(const char *sig, const char *challenge,
						    const char *secret, long minute)
{
	char message[128];
	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int digest_len = 0;
	char expected[65];
	int message_len;

	if (!secret || !*secret)
		return 0;
	message_len = snprintf(message, sizeof(message), "%ld:%s", minute, challenge);
	if (message_len < 0 || (size_t)message_len >= sizeof(message) ||
	    !HMAC(EVP_sha256(), secret, strlen(secret), (unsigned char *)message,
		  (size_t)message_len, digest, &digest_len) ||
	    digest_len != 32)
		return 0;
	for (unsigned int i = 0; i < digest_len; i++)
		snprintf(expected + (i * 2), 3, "%02x", digest[i]);
	expected[64] = '\0';
	return CRYPTO_memcmp(sig, expected, 64) == 0;
}

static int ws_verify_durisweb_signature(const char *sig, const char *challenge,
					time_t challenge_expires)
{
	const char *secret = getenv("DURISWEB_SECRET");
	const char *previous_secret = getenv("DURISWEB_SECRET_PREVIOUS");
	time_t now;
	long minute;

	if (!secret || !*secret || !sig || strlen(sig) != 64 || !challenge ||
	    strlen(challenge) != 64)
		return 0;
	for (size_t i = 0; i < 64; i++)
	{
		if (!((sig[i] >= '0' && sig[i] <= '9') || (sig[i] >= 'a' && sig[i] <= 'f') ||
		      (sig[i] >= 'A' && sig[i] <= 'F')))
			return 0;
	}

	now = time(NULL);
	if (challenge_expires < now)
		return 0;
	minute = now / 60;
	for (int offset = -1; offset <= 1; offset++)
	{
		if (ws_verify_durisweb_signature_with_secret(sig, challenge, secret,
							     minute + offset) ||
		    ws_verify_durisweb_signature_with_secret(sig, challenge, previous_secret,
							     minute + offset))
			return 1;
	}
	return 0;
}

#endif
