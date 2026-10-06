#include "world/native_mobile_birth_artifact.h"

#include <openssl/evp.h>
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

bool native_mobile_birth_running_artifact_digest(std::array<uint8_t, 32> *output) noexcept
{
	if (!output)
		return false;
	static const running_artifact original = capture_running_artifact();
	if (!original.valid)
		return false;
	*output = original.digest;
	return true;
}
