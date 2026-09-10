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

	/* Shared password worker. Handles belong to the game thread; no account/descriptor
	 * pointer crosses the worker boundary. A full queue fails closed. */
	struct password_login_job;
	struct password_login_job *password_login_submit(const char *password, const char *hash,
							 int upgrade_legacy);
	/* NULL hash selects hash-only; replacement hashes a new password only after
	 * verification succeeds. sha256 selects the private-chest legacy format. */
	struct password_login_job *password_work_submit(const char *password, const char *hash,
							const char *replacement, int upgrade_legacy,
							int sha256);
	/* Returns zero while pending. A changed account hash invalidates the result.
	 * On completion the caller owns *new_hash (free), then releases the handle. */
	int password_login_poll(struct password_login_job *job, const char *current_hash,
				int *valid, char **new_hash);
	void password_login_release(struct password_login_job *job);
	/* After descriptors are closed; also invoked automatically on process exit. */
	void password_login_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
