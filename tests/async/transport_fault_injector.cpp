// Test-only faults at the real IPC boundary; never linked into the server.
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

extern "C" int setsockopt(int fd, int level, int option, const void *value,
			  socklen_t length) noexcept
{
	using setter = int (*)(int, int, int, const void *, socklen_t);
	static const auto original = reinterpret_cast<setter>(dlsym(RTLD_NEXT, "setsockopt"));
	const char *mode = getenv("DURIS_TEST_TRANSPORT_FAULT");
	if (level == SOL_SOCKET && option == SO_SNDBUF && mode && strstr(mode, "slow-output"))
	{
		const int capacity = 4096;
		return original(fd, level, option, &capacity, sizeof(capacity));
	}
	return original(fd, level, option, value, length);
}

extern "C" ssize_t send(int fd, const void *data, size_t length, int flags)
{
	using sender = ssize_t (*)(int, const void *, size_t, int);
	static const auto original = reinterpret_cast<sender>(dlsym(RTLD_NEXT, "send"));
	const auto *bytes = static_cast<const unsigned char *>(data);
	const char *mode = getenv("DURIS_TEST_TRANSPORT_FAULT");
	if (!mode || length < 32 || memcmp(bytes, "DTP1", 4))
		return original(fd, data, length, flags);
	const unsigned type = (bytes[6] << 8) | bytes[7];
	if (type == 8 && !getenv("DURIS_TRANSPORT_FD") && strstr(mode, "trace-close"))
	{
		constexpr char marker[] = "transport-test: frontend close sent\n";
		const ssize_t marked = write(STDERR_FILENO, marker, sizeof(marker) - 1);
		(void)marked;
	}
	if (type == 9 && strstr(mode, "delay-pause"))
	{
		constexpr char marker[] = "transport-test: pause barrier delayed\n";
		const ssize_t marked = write(STDERR_FILENO, marker, sizeof(marker) - 1);
		(void)marked;
		usleep(500000);
	}
	if (type == 1 && bytes[31] == 1 && strstr(mode, "corrupt-handoff"))
	{
		const char *path = getenv("COPYOVER_STATE_FILE");
		const int file = path ? open(path, O_RDWR | O_CLOEXEC | O_NOFOLLOW) : -1;
		unsigned char value = 0;
		if (file >= 0 && pread(file, &value, 1, 0) == 1)
		{
			value ^= 1;
			const ssize_t changed = pwrite(file, &value, 1, 0);
			(void)changed;
		}
		if (file >= 0)
			close(file);
	}
	if (type == 1 && bytes[31] == 1 && strstr(mode, "delay"))
		usleep(1000000); // Only the replacement world's HELLO (detail=1).
	const ssize_t result = original(fd, data, length, flags);
	if (type == 1 && strstr(mode, "lifecycle-fail"))
		_exit(1); // Fail before readiness to exercise the bounded cold-start budget.
	if (type == 14 && strstr(mode, "lifecycle-exit"))
		_exit(55); // Exercise supervisor status propagation; no wipe routine is called.
	const size_t payload_length = (static_cast<size_t>(bytes[24]) << 24) |
				      (static_cast<size_t>(bytes[25]) << 16) |
				      (static_cast<size_t>(bytes[26]) << 8) | bytes[27];
	if (result == static_cast<ssize_t>(length) && length == payload_length + 32 &&
	    (type == 4 || type == 7) && strstr(mode, "duplicate"))
	{
		const ssize_t repeated = original(fd, data, length, flags);
		(void)repeated;
	}
	return result;
}
