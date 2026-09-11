#include "account/password_hash.h"

#include <crypt.h>
#include <ctype.h>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <algorithm>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

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

struct password_login_job
{
	char password[4096] = {};
	char hash[128] = {};
	char replacement[4096] = {};
	bool hash_only = false;
	bool replace = false;
	bool sha256 = false;
	char *new_hash = nullptr;
	bool upgrade_legacy = false;
	bool started = false;
	bool done = false;
	bool cancelled = false;
	int valid = 0;

	~password_login_job()
	{
		OPENSSL_cleanse(password, sizeof(password));
		OPENSSL_cleanse(replacement, sizeof(replacement));
		OPENSSL_cleanse(hash, sizeof(hash));
		if (new_hash)
		{
			OPENSSL_cleanse(new_hash, strlen(new_hash));
			free(new_hash);
		}
	}
};

namespace
{
struct login_password_worker
{
	std::mutex mutex;
	std::condition_variable available;
	std::deque<std::unique_ptr<password_login_job>> jobs;
	std::thread thread;
	bool stopping = false;

	~login_password_worker() { shutdown(); }

	password_login_job *next_job()
	{
		for (const auto &job : jobs)
			if (!job->started)
				return job.get();
		return nullptr;
	}

	void erase(password_login_job *job)
	{
		const auto found = std::find_if(jobs.begin(), jobs.end(), [job](const auto &entry)
						{ return entry.get() == job; });
		if (found != jobs.end())
			jobs.erase(found);
	}

	void run()
	{
		std::unique_lock<std::mutex> lock(mutex);
		for (;;)
		{
			available.wait(lock, [this] { return stopping || next_job(); });
			if (stopping)
				return;
			password_login_job *job = next_job();
			job->started = true;
			lock.unlock();
			const bool bcrypt = is_bcrypt_hash(job->hash);
			if (job->hash_only)
				job->valid = 1;
			else if (job->sha256 && !bcrypt)
				job->valid =
					password_verify_legacy_sha256(job->password, job->hash);
			else if (bcrypt)
				job->valid = bcrypt_verify_password(job->password, job->hash);
			else
			{
				/* Match CRYPT2's legacy setting, using thread-local crypt state. */
				char setting[40];
				if (job->hash[0] == '$')
					snprintf(setting, sizeof(setting), "%.*s", 39, job->hash);
				else
					snprintf(setting, sizeof(setting), "$1$%.8s$", job->hash);
				crypt_data data = {};
				const char *result = crypt_r(job->password, setting, &data);
				job->valid = result && strlen(result) == strlen(job->hash) &&
					     CRYPTO_memcmp(result, job->hash, strlen(job->hash)) ==
						     0;
				OPENSSL_cleanse(&data, sizeof(data));
				OPENSSL_cleanse(setting, sizeof(setting));
			}
			if (job->valid &&
			    (job->hash_only || job->replace || (!bcrypt && job->upgrade_legacy)))
				job->new_hash = bcrypt_hash_password(
					job->replace ? job->replacement : job->password);
			OPENSSL_cleanse(job->replacement, sizeof(job->replacement));
			OPENSSL_cleanse(job->password, sizeof(job->password));
			lock.lock();
			job->done = true;
			if (job->cancelled)
				erase(job);
		}
	}

	void shutdown()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			stopping = true;
			available.notify_one();
		}
		if (thread.joinable())
			thread.join();
		std::lock_guard<std::mutex> lock(mutex);
		jobs.clear();
	}
};

login_password_worker login_worker;
} // namespace

password_login_job *password_login_submit(const char *password, const char *hash,
					  int upgrade_legacy)
{
	if (!hash || !*hash)
		return nullptr;
	return password_work_submit(password, hash, nullptr, upgrade_legacy, 0);
}

password_login_job *password_work_submit(const char *password, const char *hash,
					 const char *replacement, int upgrade_legacy, int sha256)
{
	if (!password || strnlen(password, 4096) >= 4096 ||
	    (hash && (!*hash || strnlen(hash, 128) >= 128)) ||
	    (replacement && strnlen(replacement, 4096) >= 4096))
		return nullptr;
	std::lock_guard<std::mutex> lock(login_worker.mutex);
	/* Includes queued, running, and unconsumed results, not just the queue. */
	if (login_worker.stopping || login_worker.jobs.size() >= 16)
		return nullptr;
	try
	{
		if (!login_worker.thread.joinable())
			login_worker.thread = std::thread([] { login_worker.run(); });
		auto job = std::make_unique<password_login_job>();
		memcpy(job->password, password, strlen(password) + 1);
		job->hash_only = !hash;
		job->sha256 = sha256 != 0;
		if (hash)
			memcpy(job->hash, hash, strlen(hash) + 1);
		job->replace = replacement != nullptr;
		if (replacement)
			memcpy(job->replacement, replacement, strlen(replacement) + 1);
		job->upgrade_legacy = upgrade_legacy != 0;
		password_login_job *handle = job.get();
		login_worker.jobs.push_back(std::move(job));
		login_worker.available.notify_one();
		return handle;
	}
	catch (...)
	{
		return nullptr;
	}
}

int password_login_poll(password_login_job *job, const char *current_hash, int *valid,
			char **new_hash)
{
	std::lock_guard<std::mutex> lock(login_worker.mutex);
	if (!job || !job->done)
		return 0;
	*valid = (job->hash_only || (current_hash && !strcmp(current_hash, job->hash))) &&
		 job->valid;
	*new_hash = nullptr;
	if (*valid)
	{
		*new_hash = job->new_hash;
		job->new_hash = nullptr;
	}
	return 1;
}

void password_login_release(password_login_job *job)
{
	if (!job)
		return;
	std::lock_guard<std::mutex> lock(login_worker.mutex);
	if (!job->started || job->done)
		login_worker.erase(job);
	else
		job->cancelled = true;
}

void password_login_shutdown(void)
{
	login_worker.shutdown();
}
