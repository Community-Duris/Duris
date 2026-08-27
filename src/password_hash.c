#include "password_hash.h"

#include <crypt.h>
#include <ctype.h>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>

static char *password_hash_copy(const char *value)
{
	if (!value)
		return NULL;
	size_t len = strlen(value) + 1;
	char *copy = (char *)malloc(len);
	if (copy)
		memcpy(copy, value, len);
	return copy;
}

char *bcrypt_hash_password(const char *password)
{
	if (!password)
		return NULL;

	char setting[CRYPT_GENSALT_OUTPUT_SIZE];
	if (!crypt_gensalt_rn("$2b$", 12, NULL, 0, setting, sizeof(setting)))
		return NULL;

	crypt_data data = {};
	char *result = crypt_r(password, setting, &data);
	char *copy = is_bcrypt_hash(result) ? password_hash_copy(result) : NULL;
	OPENSSL_cleanse(&data, sizeof(data));
	OPENSSL_cleanse(setting, sizeof(setting));
	return copy;
}

int is_bcrypt_hash(const char *hash)
{
	return hash && strlen(hash) == 60 && hash[0] == '$' && hash[1] == '2' &&
	       (hash[2] == 'a' || hash[2] == 'b' || hash[2] == 'y') && hash[3] == '$' &&
	       isdigit((unsigned char)hash[4]) && isdigit((unsigned char)hash[5]) && hash[6] == '$';
}

int bcrypt_verify_password(const char *password, const char *hash)
{
	if (!password || !is_bcrypt_hash(hash))
		return 0;

	crypt_data data = {};
	char *result = crypt_r(password, hash, &data);
	int valid = result && strlen(result) == strlen(hash) &&
		    CRYPTO_memcmp(result, hash, strlen(hash)) == 0;
	OPENSSL_cleanse(&data, sizeof(data));
	return valid;
}

int password_verify_legacy_sha256(const char *password, const char *hash)
{
	if (!password || !hash || strlen(hash) != SHA256_DIGEST_LENGTH * 2)
		return 0;
	for (size_t i = 0; hash[i]; ++i)
		if (!isxdigit((unsigned char)hash[i]))
			return 0;

	unsigned char digest[SHA256_DIGEST_LENGTH];
	if (!SHA256((const unsigned char *)password, strlen(password), digest))
		return 0;

	static const char hex[] = "0123456789abcdef";
	char encoded[SHA256_DIGEST_LENGTH * 2];
	for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
	{
		encoded[i * 2] = hex[digest[i] >> 4];
		encoded[i * 2 + 1] = hex[digest[i] & 0x0f];
	}

	char normalized[sizeof(encoded)];
	for (size_t i = 0; i < sizeof(normalized); ++i)
		normalized[i] = (char)tolower((unsigned char)hash[i]);
	int valid = CRYPTO_memcmp(encoded, normalized, sizeof(encoded)) == 0;
	OPENSSL_cleanse(digest, sizeof(digest));
	OPENSSL_cleanse(encoded, sizeof(encoded));
	OPENSSL_cleanse(normalized, sizeof(normalized));
	return valid;
}
