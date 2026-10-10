#include "world/native_mobile_birth_artifact.h"

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <memory>
#if defined(__linux__)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace
{
struct running_artifact
{
	std::array<uint8_t, 32> digest{};
	bool valid = false;
	int error = 0;
};

running_artifact capture_running_artifact() noexcept
{
	running_artifact result;
#if defined(__linux__)
	struct descriptor
	{
		int value = -1;
		~descriptor()
		{
			if (value >= 0)
				close(value);
		}
	} file;
	file.value = open("/proc/self/exe", O_RDONLY | O_CLOEXEC);
	struct stat before
	{
	}, after{};
	if (file.value < 0 || fstat(file.value, &before) || !S_ISREG(before.st_mode) ||
	    before.st_size < 4)
		return result;
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context(EVP_MD_CTX_new(),
									EVP_MD_CTX_free);
	if (!context || EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1)
		return result;
	std::array<uint8_t, 32768> bytes{};
	uint64_t remaining = static_cast<uint64_t>(before.st_size);
	bool first = true;
	while (remaining)
	{
		const size_t requested =
			static_cast<size_t>(std::min<uint64_t>(remaining, bytes.size()));
		ssize_t received;
		do
		{
			received = read(file.value, bytes.data(), requested);
		} while (received < 0 && errno == EINTR);
		if (received <= 0 || static_cast<uint64_t>(received) > remaining)
			return result;
		if (first && (received < 4 || memcmp(bytes.data(), "\177ELF", 4)))
			return result;
		first = false;
		if (EVP_DigestUpdate(context.get(), bytes.data(), static_cast<size_t>(received)) !=
		    1)
			return result;
		remaining -= static_cast<uint64_t>(received);
	}
	ssize_t tail;
	do
	{
		tail = read(file.value, bytes.data(), 1);
	} while (tail < 0 && errno == EINTR);
	if (tail != 0 || fstat(file.value, &after) || before.st_dev != after.st_dev ||
	    before.st_ino != after.st_ino || before.st_size != after.st_size ||
	    before.st_mtim.tv_sec != after.st_mtim.tv_sec ||
	    before.st_mtim.tv_nsec != after.st_mtim.tv_nsec ||
	    before.st_ctim.tv_sec != after.st_ctim.tv_sec ||
	    before.st_ctim.tv_nsec != after.st_ctim.tv_nsec)
		return result;
	unsigned int size = 0;
	if (EVP_DigestFinal_ex(context.get(), result.digest.data(), &size) != 1 ||
	    size != result.digest.size())
		return running_artifact{};
	const int fd = file.value;
	file.value = -1;
	if (close(fd))
		return running_artifact{};
	result.valid = true;
#endif
	return result;
}
} // namespace

namespace
{
using artifact_reserve_fn = bool (*)(size_t, void *) noexcept;

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&      \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)

// Full pinned OpenSSL 3.0.13 SHA256 source scopes, not an EVP allowance.
// sha512-x86_64.pl: SHA256 SZ=4/rounds=64, largest AVX2 transient,
// four saved pointers, six pushes, worst 1024-byte alignment loss and
// caller/secondary frame-pointer transient. Other dispatch leaves are smaller.
constexpr size_t artifact_sha_assembly =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t artifact_sha_c_small = 16 * sizeof(unsigned int) + 12 * sizeof(unsigned int) +
					sizeof(unsigned int) + sizeof(int) + sizeof(void *);
constexpr size_t artifact_sha_c_normal = 16 * sizeof(unsigned int) + 11 * sizeof(unsigned int) +
					 2 * sizeof(int) + 2 * sizeof(void *);
constexpr size_t artifact_sha_init = sizeof(void *) + sizeof(int);
constexpr size_t artifact_sha_update =
	4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(unsigned int) + sizeof(int);
constexpr size_t artifact_sha_final = 3 * sizeof(void *) + sizeof(size_t) + sizeof(unsigned long) +
				      sizeof(unsigned int) + sizeof(int);
constexpr size_t artifact_sha_memory = 3 * sizeof(void *) + sizeof(size_t) + sizeof(int);
constexpr size_t artifact_sha_cleanse = 2 * sizeof(void *) + sizeof(size_t) + artifact_sha_memory;
constexpr size_t artifact_sha_block = std::max(
	artifact_sha_assembly, 2 * sizeof(void *) + sizeof(size_t) +
				       std::max(artifact_sha_c_small, artifact_sha_c_normal));
constexpr size_t artifact_sha_leaf =
	std::max(artifact_sha_init, std::max(artifact_sha_update, artifact_sha_final)) +
	std::max(artifact_sha_block, std::max(artifact_sha_memory, artifact_sha_cleanse));

struct artifact_capture_workspace
{
	running_artifact result;
	int descriptor = -1;
	struct stat before
	{
	}, after{};
	SHA256_CTX hash;
	std::array<uint8_t, 32768> bytes{};
	uint64_t remaining = 0;
	size_t requested = 0;
	ssize_t received = 0, tail = 0;
	int fd = -1;
	bool first = true;
	~artifact_capture_workspace()
	{
		if (descriptor >= 0)
			close(descriptor);
	}
};

running_artifact artifact_capture_failure(int error) noexcept
{
	running_artifact result;
	result.error = error;
	errno = error;
	return result;
}

running_artifact capture_running_artifact_fixed() noexcept
{
	artifact_capture_workspace work;
	work.descriptor = open("/proc/self/exe", O_RDONLY | O_CLOEXEC);
	if (work.descriptor < 0)
		return artifact_capture_failure(errno);
	if (fstat(work.descriptor, &work.before))
		return artifact_capture_failure(errno);
	if (!S_ISREG(work.before.st_mode) || work.before.st_size < 4)
		return artifact_capture_failure(EIO);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	const int initialized = SHA256_Init(&work.hash);
#pragma GCC diagnostic pop
	if (initialized != 1)
		return artifact_capture_failure(EIO);
	work.remaining = static_cast<uint64_t>(work.before.st_size);
	while (work.remaining)
	{
		work.requested =
			static_cast<size_t>(std::min<uint64_t>(work.remaining, work.bytes.size()));
		do
		{
			work.received = read(work.descriptor, work.bytes.data(), work.requested);
		} while (work.received < 0 && errno == EINTR);
		if (work.received < 0)
			return artifact_capture_failure(errno);
		if (!work.received || static_cast<uint64_t>(work.received) > work.remaining)
			return artifact_capture_failure(EIO);
		if (work.first && (work.received < 4 || memcmp(work.bytes.data(), "\177ELF", 4)))
			return artifact_capture_failure(EIO);
		work.first = false;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		const int updated = SHA256_Update(&work.hash, work.bytes.data(),
						  static_cast<size_t>(work.received));
#pragma GCC diagnostic pop
		if (updated != 1)
			return artifact_capture_failure(EIO);
		work.remaining -= static_cast<uint64_t>(work.received);
	}
	do
	{
		work.tail = read(work.descriptor, work.bytes.data(), 1);
	} while (work.tail < 0 && errno == EINTR);
	if (work.tail < 0)
		return artifact_capture_failure(errno);
	if (work.tail != 0)
		return artifact_capture_failure(EIO);
	if (fstat(work.descriptor, &work.after))
		return artifact_capture_failure(errno);
	if (work.before.st_dev != work.after.st_dev || work.before.st_ino != work.after.st_ino ||
	    work.before.st_size != work.after.st_size ||
	    work.before.st_mtim.tv_sec != work.after.st_mtim.tv_sec ||
	    work.before.st_mtim.tv_nsec != work.after.st_mtim.tv_nsec ||
	    work.before.st_ctim.tv_sec != work.after.st_ctim.tv_sec ||
	    work.before.st_ctim.tv_nsec != work.after.st_ctim.tv_nsec)
		return artifact_capture_failure(EIO);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	const int finalized = SHA256_Final(work.result.digest.data(), &work.hash);
#pragma GCC diagnostic pop
	if (finalized != 1)
		return artifact_capture_failure(EIO);
	work.fd = work.descriptor;
	work.descriptor = -1;
	if (close(work.fd))
		return artifact_capture_failure(errno);
	work.result.valid = true;
	return work.result;
}
#else
running_artifact capture_running_artifact_fixed() noexcept
{
	// The bounded entry refuses before entering the shared initializer here.
	return running_artifact{};
}
#endif

running_artifact initialize_running_artifact(bool bounded) noexcept
{
	running_artifact result = bounded ? capture_running_artifact_fixed() :
					    capture_running_artifact();
	if (!result.valid && !result.error)
		// The original capture has no typed error result. Do not turn a stale
		// errno from a successful/EINTR-retried syscall into a claimed cause.
		result.error = EIO;
	return result;
}

const running_artifact &running_artifact_cached(bool bounded) noexcept
{
	// One cache and one original once-only C++ static guard for both callers.
	// Bounded resource/profile admission has already succeeded before this call.
	static const running_artifact original = initialize_running_artifact(bounded);
	return original;
}
} // namespace

bool native_mobile_birth_running_artifact_digest_bounded(std::array<uint8_t, 32> *output,
							 artifact_reserve_fn reserve, void *context,
							 size_t outer) noexcept
{
	if (!output)
	{
		errno = EINVAL;
		return false;
	}
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&      \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
	{
		errno = ENOTSUP;
		return false;
	}
	// Public output/reserve/context/outer, retained request and cached ref;
	// getter's mode/ref, initializer's mode/result, capture's workspace/result;
	// actual three SHA result locals; failure's argument/result and destructor.
	constexpr size_t callers = 6 * sizeof(void *) + 3 * sizeof(size_t) + 4 * sizeof(bool) +
				   2 * sizeof(running_artifact) + 4 * sizeof(int);
	// Same real process cache and its original Itanium once-only guard, not a
	// second digest cache. Callback-private memory remains callback-owned.
	constexpr size_t retained_cache = sizeof(running_artifact) + sizeof(uint64_t);
	// Actual array data->_S_ptr pointer arguments/results, size's this/result,
	// std::min's two const-reference arguments/result and two size conversions.
	constexpr size_t array_leaf = 6 * sizeof(void *) + 3 * sizeof(size_t);
	// open path/flags/mode/result; read fd/buffer/count/result; fstat fd/stat/result;
	// close fd/result and ELF memcmp's two pointers/length/int result are sequential.
	constexpr size_t syscall_leaf =
		std::max(sizeof(void *) + 3 * sizeof(int),
			 std::max(sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(ssize_t),
				  std::max(sizeof(void *) + 2 * sizeof(int),
					   2 * sizeof(void *) + sizeof(size_t) + sizeof(int))));
	// Genuine GCC13 guard.cc's installed futex acquire/release branch: guard/gi,
	// expected/newv and three bit integers, returned int, __test_and_acquire's
	// g/p/char/return, __is_single_threaded result and syscall(long,gi,int,int,0).
	// The original static guard is not replaced with a new locking framework.
	constexpr size_t guard_leaf = 5 * sizeof(void *) + 8 * sizeof(int) + sizeof(char) +
				      2 * sizeof(bool) + 2 * sizeof(long);
	constexpr size_t request =
		callers + retained_cache + sizeof(artifact_capture_workspace) +
		std::max(artifact_sha_leaf,
			 std::max(array_leaf, std::max(syscall_leaf, guard_leaf)));
	if (outer > SIZE_MAX - request)
	{
		errno = EOVERFLOW;
		return false;
	}
	if (!reserve || !reserve(outer + request, context))
	{
		errno = ENOBUFS;
		return false;
	}
	const running_artifact &original = running_artifact_cached(true);
	if (!original.valid)
	{
		errno = original.error ? original.error : EIO;
		return false;
	}
	*output = original.digest;
	return true;
#else
	(void)reserve;
	(void)context;
	(void)outer;
	errno = ENOTSUP;
	return false;
#endif
}

bool native_mobile_birth_running_artifact_digest(std::array<uint8_t, 32> *output) noexcept
{
	if (!output)
		return false;
	const running_artifact &original = running_artifact_cached(false);
	if (!original.valid)
		return false;
	*output = original.digest;
	return true;
}
