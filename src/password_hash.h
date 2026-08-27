#ifndef DURIS_PASSWORD_HASH_H
#define DURIS_PASSWORD_HASH_H

#include <stddef.h>

#define BCRYPT_PASSWORD_MAX_BYTES 72

#ifdef __cplusplus
extern "C"
{
#endif

	char *bcrypt_hash_password(const char *password);
	int bcrypt_verify_password(const char *password, const char *hash);
	int is_bcrypt_hash(const char *hash);
	int password_verify_legacy_sha256(const char *password, const char *hash);

#ifdef __cplusplus
}
#endif

#endif
