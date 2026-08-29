#include "flatfile_store.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{
std::atomic<uint64_t> temporary_sequence{ 0 };

void set_error(std::string *error, const char *operation)
{
	if (error)
		*error = std::string(operation) + ": " + strerror(errno);
}

bool valid_name(const std::string &name)
{
	return !name.empty() && name != "." && name != ".." && name.find('/') == std::string::npos;
}

bool private_directory(int fd)
{
	struct stat info;
	return fstat(fd, &info) == 0 && S_ISDIR(info.st_mode) && info.st_uid == geteuid() &&
	       !(info.st_mode & 0077);
}

bool write_all(int fd, const uint8_t *data, size_t size)
{
	while (size)
	{
		const size_t chunk = std::min(size, static_cast<size_t>(SSIZE_MAX));
		const ssize_t written = write(fd, data, chunk);
		if (written < 0)
		{
			if (errno == EINTR)
				continue;
			return false;
		}
		if (!written)
		{
			errno = EIO;
			return false;
		}
		data += written;
		size -= static_cast<size_t>(written);
	}
	return true;
}

bool read_all(int fd, uint8_t *data, size_t size)
{
	while (size)
	{
		const size_t chunk = std::min(size, static_cast<size_t>(SSIZE_MAX));
		const ssize_t received = read(fd, data, chunk);
		if (received < 0)
		{
			if (errno == EINTR)
				continue;
			return false;
		}
		if (!received)
		{
			errno = EIO;
			return false;
		}
		data += received;
		size -= static_cast<size_t>(received);
	}
	return true;
}
} // namespace

bool flatfile_atomic_write(const std::string &directory, const std::string &name,
			   const std::vector<uint8_t> &bytes, std::string *error)
{
	if (!valid_name(name))
	{
		if (error)
			*error = "invalid flat-file name";
		return false;
	}

	const int directory_fd =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (directory_fd < 0)
	{
		set_error(error, "open authority directory");
		return false;
	}
	if (!private_directory(directory_fd))
	{
		if (error)
			*error = "invalid authority directory metadata";
		close(directory_fd);
		return false;
	}

	char temporary[256];
	const uint64_t sequence = temporary_sequence.fetch_add(1, std::memory_order_relaxed);
	const int length = snprintf(temporary, sizeof(temporary), ".%s.tmp.%ld.%llu", name.c_str(),
				    static_cast<long>(getpid()),
				    static_cast<unsigned long long>(sequence));
	if (length < 0 || static_cast<size_t>(length) >= sizeof(temporary))
	{
		if (error)
			*error = "temporary flat-file name is too long";
		close(directory_fd);
		return false;
	}

	const int file_fd = openat(directory_fd, temporary,
				   O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (file_fd < 0)
	{
		set_error(error, "create temporary authority file");
		close(directory_fd);
		return false;
	}

	bool ok = write_all(file_fd, bytes.data(), bytes.size());
	if (ok && fdatasync(file_fd) < 0)
		ok = false;
	if (close(file_fd) < 0)
		ok = false;
	if (!ok)
	{
		set_error(error, "write authority file");
		unlinkat(directory_fd, temporary, 0);
		close(directory_fd);
		return false;
	}

	if (renameat(directory_fd, temporary, directory_fd, name.c_str()) < 0)
	{
		set_error(error, "publish authority file");
		unlinkat(directory_fd, temporary, 0);
		close(directory_fd);
		return false;
	}
	if (fsync(directory_fd) < 0)
	{
		set_error(error, "sync authority directory");
		close(directory_fd);
		return false;
	}
	close(directory_fd);
	return true;
}

bool flatfile_atomic_remove(const std::string &directory, const std::string &name, bool missing_ok,
			    std::string *error)
{
	if (!valid_name(name))
	{
		if (error)
			*error = "invalid flat-file name";
		return false;
	}
	const int directory_fd =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (directory_fd < 0)
	{
		set_error(error, "open authority directory");
		return false;
	}
	if (!private_directory(directory_fd))
	{
		if (error)
			*error = "invalid authority directory metadata";
		close(directory_fd);
		return false;
	}
	struct stat info;
	if (fstatat(directory_fd, name.c_str(), &info, AT_SYMLINK_NOFOLLOW) < 0)
	{
		const int saved_errno = errno;
		close(directory_fd);
		errno = saved_errno;
		if (missing_ok && errno == ENOENT)
			return true;
		set_error(error, "inspect authority file for removal");
		return false;
	}
	if (!S_ISREG(info.st_mode) || info.st_uid != geteuid() || (info.st_mode & 0077))
	{
		if (error)
			*error = "invalid authority file metadata for removal";
		close(directory_fd);
		return false;
	}
	if (unlinkat(directory_fd, name.c_str(), 0) < 0 || fsync(directory_fd) < 0)
	{
		set_error(error, "remove authority file");
		close(directory_fd);
		return false;
	}
	close(directory_fd);
	return true;
}

flatfile_read_result flatfile_read(const std::string &directory, const std::string &name,
				   size_t maximum_size, std::vector<uint8_t> *bytes,
				   std::string *error)
{
	if (!bytes || !valid_name(name))
	{
		if (error)
			*error = "invalid flat-file read request";
		return flatfile_read_result::invalid;
	}
	bytes->clear();
	const int directory_fd =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (directory_fd < 0)
	{
		set_error(error, "open authority directory");
		return flatfile_read_result::io_error;
	}
	if (!private_directory(directory_fd))
	{
		if (error)
			*error = "invalid authority directory metadata";
		close(directory_fd);
		return flatfile_read_result::invalid;
	}
	const int file_fd = openat(directory_fd, name.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
	if (file_fd < 0)
	{
		const int saved_errno = errno;
		close(directory_fd);
		errno = saved_errno;
		if (errno == ENOENT)
			return flatfile_read_result::not_found;
		set_error(error, "open authority file");
		return errno == ELOOP ? flatfile_read_result::invalid :
					flatfile_read_result::io_error;
	}
	close(directory_fd);

	struct stat info;
	if (fstat(file_fd, &info) < 0)
	{
		set_error(error, "inspect authority file");
		close(file_fd);
		return flatfile_read_result::io_error;
	}
	if (!S_ISREG(info.st_mode) || info.st_uid != geteuid() || (info.st_mode & 0077) ||
	    info.st_size < 0 || static_cast<uintmax_t>(info.st_size) > maximum_size)
	{
		if (error)
			*error = "invalid authority file metadata or size";
		close(file_fd);
		return flatfile_read_result::invalid;
	}
	bytes->resize(static_cast<size_t>(info.st_size));
	if (!read_all(file_fd, bytes->data(), bytes->size()))
	{
		set_error(error, "read authority file");
		bytes->clear();
		close(file_fd);
		return flatfile_read_result::io_error;
	}
	close(file_fd);
	return flatfile_read_result::ok;
}

bool flatfile_lock_acquire(const std::string &directory, const std::string &name, int *lock_fd,
			   std::string *error)
{
	if (!lock_fd || !valid_name(name))
	{
		if (error)
			*error = "invalid flat-file lock request";
		return false;
	}
	*lock_fd = -1;
	const int directory_fd =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (directory_fd < 0)
	{
		set_error(error, "open lock directory");
		return false;
	}
	if (!private_directory(directory_fd))
	{
		if (error)
			*error = "invalid lock directory metadata";
		close(directory_fd);
		return false;
	}
	const int file_fd =
		openat(directory_fd, name.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
	const int open_errno = errno;
	close(directory_fd);
	if (file_fd < 0)
	{
		errno = open_errno;
		set_error(error, "open authority lock");
		return false;
	}
	struct stat info;
	if (fstat(file_fd, &info) < 0 || !S_ISREG(info.st_mode) || info.st_uid != geteuid() ||
	    (info.st_mode & 0077))
	{
		if (error)
			*error = "invalid authority lock metadata";
		close(file_fd);
		return false;
	}
	while (flock(file_fd, LOCK_EX) < 0)
	{
		if (errno == EINTR)
			continue;
		set_error(error, "lock authority");
		close(file_fd);
		return false;
	}
	*lock_fd = file_fd;
	return true;
}

void flatfile_lock_release(int lock_fd)
{
	if (lock_fd < 0)
		return;
	while (flock(lock_fd, LOCK_UN) < 0 && errno == EINTR)
		;
	close(lock_fd);
}
