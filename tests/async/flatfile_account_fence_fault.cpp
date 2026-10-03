// Test-only preload fault for the real server's owned account-menu journey.
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" int fsync(int fd)
{
	static const auto native = reinterpret_cast<int (*)(int)>(dlsym(RTLD_NEXT, "fsync"));
	if (!native)
	{
		errno = ENOSYS;
		return -1;
	}
	const char *marker = getenv("DURIS_FLATFILE_FENCE_FAULT_MARKER");
	if (!marker || faccessat(fd, ".critical-authority-transaction", F_OK, 0))
		return native(fd);
	char link[64], directory[4096], hit[4096];
	snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
	const auto length = readlink(link, directory, sizeof(directory) - 1);
	if (length < 8 || static_cast<size_t>(length) >= sizeof(directory) - 1)
		return native(fd);
	directory[length] = '\0';
	if (strcmp(directory + length - 8, "/domains"))
		return native(fd);
	const int fault = open(marker, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	if (fault < 0)
		return native(fd);
	char mode = '\0';
	const auto count = read(fault, &mode, 1);
	close(fault);
	if (count != 1 || (mode != 'D' && mode != 'U'))
		return native(fd);
	const int written = snprintf(hit, sizeof(hit), "%s.hit", marker);
	if (written < 0 || static_cast<size_t>(written) >= sizeof(hit) || rename(marker, hit))
		return native(fd);
	if (mode == 'U')
	{
		errno = EIO;
		return -1;
	}
	const int result = native(fd);
	if (!result)
	{
		// The journal is durable; force its account after-image to remain pending.
		directory[length - 8] = '\0';
		const int size = snprintf(hit, sizeof(hit), "%s/identities/accounts", directory);
		if (size >= 0 && static_cast<size_t>(size) < sizeof(hit))
			chmod(hit, 0701);
	}
	return result;
}
