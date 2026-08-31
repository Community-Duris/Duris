#include "economy/donation_event.h"

#include <cjson/cJSON.h>
#include <openssl/crypto.h>
#include <openssl/hmac.h>

#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
constexpr size_t MAX_ENVELOPE_BYTES = 4096;
constexpr int64_t MAX_CLOCK_SKEW_SECONDS = 300;
constexpr int64_t MAX_AMOUNT_CENTS = 100000000;

bool copy_safe_text(const cJSON *item, char *destination, size_t capacity, bool required)
{
	if (!item)
	{
		if (required)
			return false;
		destination[0] = '\0';
		return true;
	}
	if (!cJSON_IsString(item) || !item->valuestring)
		return false;

	const size_t length = strlen(item->valuestring);
	if ((required && length == 0) || length >= capacity)
		return false;
	for (size_t index = 0; index < length; ++index)
	{
		const unsigned char value = (unsigned char)item->valuestring[index];
		if (value < 0x20 || value > 0x7e || value == '&')
			return false;
	}
	memcpy(destination, item->valuestring, length + 1);
	return true;
}

bool exact_integer(const cJSON *item, int64_t minimum, int64_t maximum, int64_t *value)
{
	if (!cJSON_IsNumber(item) || !std::isfinite(item->valuedouble) ||
	    floor(item->valuedouble) != item->valuedouble || item->valuedouble < minimum ||
	    item->valuedouble > maximum)
		return false;
	*value = (int64_t)item->valuedouble;
	return true;
}

bool valid_event_id(const char *event_id)
{
	const size_t length = strlen(event_id);
	if (length < 16 || length > 64)
		return false;
	for (size_t index = 0; index < length; ++index)
	{
		const char value = event_id[index];
		if (!((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
		      (value >= '0' && value <= '9') || value == '-' || value == '_'))
			return false;
	}
	return true;
}

bool valid_currency(const char *currency)
{
	if (strlen(currency) != 3)
		return false;
	for (size_t index = 0; index < 3; ++index)
		if (currency[index] < 'A' || currency[index] > 'Z')
			return false;
	return true;
}

int hex_nibble(char value)
{
	if (value >= '0' && value <= '9')
		return value - '0';
	if (value >= 'a' && value <= 'f')
		return value - 'a' + 10;
	if (value >= 'A' && value <= 'F')
		return value - 'A' + 10;
	return -1;
}

bool decode_signature(const cJSON *item, unsigned char signature[32])
{
	if (!cJSON_IsString(item) || !item->valuestring || strlen(item->valuestring) != 64)
		return false;
	for (size_t index = 0; index < 32; ++index)
	{
		const int high = hex_nibble(item->valuestring[index * 2]);
		const int low = hex_nibble(item->valuestring[index * 2 + 1]);
		if (high < 0 || low < 0)
			return false;
		signature[index] = (unsigned char)((high << 4) | low);
	}
	return true;
}
} // namespace

bool donation_event_decode(const char *json, size_t length, const char *secret, time_t now,
			   struct donation_event *event)
{
	if (!json || length == 0 || length > MAX_ENVELOPE_BYTES || !secret || strlen(secret) < 32 ||
	    !event)
		return false;

	cJSON *root = cJSON_ParseWithLength(json, length);
	if (!root || !cJSON_IsObject(root))
	{
		cJSON_Delete(root);
		return false;
	}

	struct donation_event candidate = {};
	int64_t schema_version = 0;
	const cJSON *version = cJSON_GetObjectItemCaseSensitive(root, "schema_version");
	const cJSON *event_id = cJSON_GetObjectItemCaseSensitive(root, "event_id");
	const cJSON *issued_at = cJSON_GetObjectItemCaseSensitive(root, "issued_at");
	const cJSON *amount = cJSON_GetObjectItemCaseSensitive(root, "amount_cents");
	const cJSON *currency = cJSON_GetObjectItemCaseSensitive(root, "currency");
	const cJSON *is_public = cJSON_GetObjectItemCaseSensitive(root, "is_public");
	const cJSON *character = cJSON_GetObjectItemCaseSensitive(root, "character_name");
	const cJSON *message = cJSON_GetObjectItemCaseSensitive(root, "message");
	const cJSON *signature_item = cJSON_GetObjectItemCaseSensitive(root, "signature");
	unsigned char supplied_signature[32] = {};

	bool valid =
		exact_integer(version, 1, 1, &schema_version) &&
		copy_safe_text(event_id, candidate.event_id, sizeof(candidate.event_id), true) &&
		valid_event_id(candidate.event_id) &&
		exact_integer(issued_at, (int64_t)now - MAX_CLOCK_SKEW_SECONDS,
			      (int64_t)now + MAX_CLOCK_SKEW_SECONDS, &candidate.issued_at) &&
		exact_integer(amount, 1, MAX_AMOUNT_CENTS, &candidate.amount_cents) &&
		copy_safe_text(currency, candidate.currency, sizeof(candidate.currency), true) &&
		valid_currency(candidate.currency) && cJSON_IsBool(is_public) &&
		copy_safe_text(character, candidate.character_name,
			       sizeof(candidate.character_name), false) &&
		copy_safe_text(message, candidate.message, sizeof(candidate.message), false) &&
		decode_signature(signature_item, supplied_signature);
	if (valid)
	{
		candidate.is_public = cJSON_IsTrue(is_public);
		if (candidate.is_public && candidate.character_name[0] == '\0')
			valid = false;
	}

	char canonical[512];
	int canonical_length = -1;
	if (valid)
		canonical_length = snprintf(canonical, sizeof(canonical),
					    "v1\n%s\n%lld\n%lld\n%s\n%d\n%s\n%s",
					    candidate.event_id, (long long)candidate.issued_at,
					    (long long)candidate.amount_cents, candidate.currency,
					    candidate.is_public ? 1 : 0, candidate.character_name,
					    candidate.message);
	if (canonical_length < 0 || (size_t)canonical_length >= sizeof(canonical))
		valid = false;

	unsigned char expected_signature[EVP_MAX_MD_SIZE] = {};
	unsigned int expected_length = 0;
	if (valid && !HMAC(EVP_sha256(), secret, (int)strlen(secret), (unsigned char *)canonical,
			   (size_t)canonical_length, expected_signature, &expected_length))
		valid = false;
	if (valid && (expected_length != sizeof(supplied_signature) ||
		      CRYPTO_memcmp(expected_signature, supplied_signature,
				    sizeof(supplied_signature)) != 0))
		valid = false;

	cJSON_Delete(root);
	if (!valid)
		return false;
	*event = candidate;
	return true;
}
