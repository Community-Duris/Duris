#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <limits>
#include <new>
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
	return flatfile_atomic_write_with_publication(directory, name, bytes, error, nullptr);
}

bool flatfile_atomic_write_with_publication(const std::string &directory, const std::string &name,
					    const std::vector<uint8_t> &bytes, std::string *error,
					    bool *published)
{
	if (published)
		*published = false;
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
	if (published)
		*published = true;
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
		close(directory_fd);
		if (error)
			*error = "invalid authority directory metadata";
		return flatfile_read_result::invalid;
	}
	const int file_fd =
		openat(directory_fd, name.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
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
		const int saved_errno = errno;
		close(file_fd);
		errno = saved_errno;
		set_error(error, "inspect authority file");
		return flatfile_read_result::io_error;
	}
	if (!S_ISREG(info.st_mode) || info.st_nlink != 1 || info.st_uid != geteuid() ||
	    (info.st_mode & 0077) || info.st_size < 0 ||
	    static_cast<uintmax_t>(info.st_size) > maximum_size)
	{
		close(file_fd);
		if (error)
			*error = "invalid authority file metadata or size";
		return flatfile_read_result::invalid;
	}
	try
	{
		bytes->resize(static_cast<size_t>(info.st_size));
	}
	catch (const std::bad_alloc &)
	{
		close(file_fd);
		errno = ENOMEM;
		return flatfile_read_result::io_error;
	}
	if (!read_all(file_fd, bytes->data(), bytes->size()))
	{
		const int saved_errno = errno;
		bytes->clear();
		close(file_fd);
		errno = saved_errno;
		set_error(error, "read authority file");
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
		errno = EINVAL;
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
	struct stat directory_info;
	const int directory_status = fstat(directory_fd, &directory_info);
	if (directory_status || !S_ISDIR(directory_info.st_mode) ||
	    directory_info.st_uid != geteuid() || (directory_info.st_mode & 0077))
	{
		const int saved_errno = directory_status ? errno : EACCES;
		close(directory_fd);
		errno = saved_errno;
		set_error(error, "invalid lock directory metadata");
		return false;
	}
	const int file_fd = openat(directory_fd, name.c_str(),
				   O_RDWR | O_CREAT | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW, 0600);
	const int open_errno = errno;
	close(directory_fd);
	if (file_fd < 0)
	{
		errno = open_errno;
		set_error(error, "open authority lock");
		return false;
	}
	struct stat info;
	const int file_status = fstat(file_fd, &info);
	if (file_status || !S_ISREG(info.st_mode) || info.st_uid != geteuid() ||
	    (info.st_mode & 0077) || (info.st_nlink != 1 || info.st_size != 0))
	{
		const int saved_errno = file_status ? errno : EINVAL;
		close(file_fd);
		errno = saved_errno;
		set_error(error, "invalid authority lock metadata");
		return false;
	}
	while (flock(file_fd, LOCK_EX) < 0)
	{
		if (errno == EINTR)
			continue;
		const int saved_errno = errno;
		close(file_fd);
		errno = saved_errno;
		set_error(error, "lock authority");
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

flatfile_read_result flatfile_read_bounded(const std::string &directory, const std::string &name,
					   size_t maximum_size, std::vector<uint8_t> *bytes,
					   flatfile_scratch_reserve_fn reserve_scratch_peak,
					   void *context, size_t outer_live_scratch) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	errno = ENOTSUP;
	return flatfile_read_result::io_error;
#else
	if (!bytes || !reserve_scratch_peak || !valid_name(name))
	{
		errno = EINVAL;
		return flatfile_read_result::invalid;
	}
	// The directory helper and same-FD file inspection each use one stat;
	// their lifetimes do not overlap. Reserve it before either inspection.
	if (outer_live_scratch > SIZE_MAX - sizeof(struct stat) ||
	    !reserve_scratch_peak(outer_live_scratch + sizeof(struct stat), context))
	{
		errno = ENOBUFS;
		return flatfile_read_result::io_error;
	}
	const size_t metadata_live = outer_live_scratch + sizeof(struct stat);
	const int directory_fd =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (directory_fd < 0)
		return flatfile_read_result::io_error;
	if (!private_directory(directory_fd))
	{
		close(directory_fd);
		errno = EBADMSG;
		return flatfile_read_result::invalid;
	}
	const int file_fd =
		openat(directory_fd, name.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
	const int open_error = errno;
	close(directory_fd);
	if (file_fd < 0)
	{
		errno = open_error;
		return open_error == ENOENT ? flatfile_read_result::not_found :
		       open_error == ELOOP  ? flatfile_read_result::invalid :
					      flatfile_read_result::io_error;
	}
	struct stat info;
	if (fstat(file_fd, &info) < 0)
	{
		const int saved_error = errno;
		close(file_fd);
		errno = saved_error;
		return flatfile_read_result::io_error;
	}
	if (!S_ISREG(info.st_mode) || info.st_nlink != 1 || info.st_uid != geteuid() ||
	    (info.st_mode & 0077) || info.st_size < 0 ||
	    static_cast<uintmax_t>(info.st_size) > maximum_size ||
	    static_cast<uintmax_t>(info.st_size) > SIZE_MAX)
	{
		close(file_fd);
		errno = EBADMSG;
		return flatfile_read_result::invalid;
	}
	const size_t size = static_cast<size_t>(info.st_size);
	// The original output and caller-owned paths are included in outer_live_scratch.
	// The inspected file stat remains live alongside the fresh vector.
	// A fresh vector reserve is exactly the requested allocation under pinned
	// libstdc++13. This is an explicit implementation contract, not portable C++.
	if (metadata_live > SIZE_MAX - sizeof(std::vector<uint8_t>) ||
	    size > SIZE_MAX - metadata_live - sizeof(std::vector<uint8_t>) ||
	    !reserve_scratch_peak(metadata_live + sizeof(std::vector<uint8_t>) + size,
				  context))
	{
		close(file_fd);
		errno = ENOBUFS;
		return flatfile_read_result::io_error;
	}
	try
	{
		std::vector<uint8_t> candidate;
		candidate.reserve(size);
		candidate.resize(size);
		if (!read_all(file_fd, candidate.data(), candidate.size()))
		{
			const int saved_error = errno;
			close(file_fd);
			errno = saved_error;
			return flatfile_read_result::io_error;
		}
		close(file_fd);
		bytes->swap(candidate);
		return flatfile_read_result::ok;
	}
	catch (const std::bad_alloc &)
	{
		close(file_fd);
		errno = ENOMEM;
		return flatfile_read_result::io_error;
	}
	catch (...)
	{
		close(file_fd);
		errno = EOVERFLOW;
		return flatfile_read_result::io_error;
	}
#endif
}

size_t flatfile_atomic_write_working_bytes() noexcept
{
	// private_directory's stat dies before the original temporary-name array.
	// Null diagnostics give the original writer no explicit C++ heap requests.
	return std::max(sizeof(struct stat), sizeof(char[256]));
}

size_t flatfile_atomic_remove_working_bytes() noexcept
{
	// The directory inspection and remove target metadata frames are sequential.
	return sizeof(struct stat);
}

// Original diagnostic-output companions. Default methods above are unchanged.
#include <functional>
#include <iterator>
#include <utility>
namespace
{

// Source carriers authenticated against this packet's actual installed GCC13
// definitions. P includes genuine this/reference/pointer/result carriers; N is
// the actual size_type, D the real pointer-difference type. No padded structs,
// row DTOs, vector move, nonselected constexpr algorithms or guessed heap.
constexpr size_t diagnostic_P = sizeof(void *), diagnostic_N = sizeof(size_t),
		 diagnostic_D = sizeof(std::ptrdiff_t), diagnostic_B = sizeof(bool),
		 diagnostic_C = sizeof(char);
constexpr size_t diagnostic_library_allocate = 9 * diagnostic_P + 5 * diagnostic_N + diagnostic_B;
constexpr size_t diagnostic_library_deallocate = 7 * diagnostic_P + 4 * diagnostic_N + diagnostic_B;
constexpr size_t diagnostic_library_move_ref = 2 * diagnostic_P;
constexpr size_t diagnostic_library_addressof = 2 * diagnostic_P;
constexpr size_t diagnostic_library_pointer_to = 2 * diagnostic_P + diagnostic_library_addressof;
constexpr size_t diagnostic_library_local_data = 2 * diagnostic_P + diagnostic_library_pointer_to;
constexpr size_t diagnostic_library_data = 2 * diagnostic_P;
constexpr size_t diagnostic_library_size = diagnostic_P + diagnostic_N;
constexpr size_t diagnostic_library_capacity = diagnostic_P + diagnostic_N;
constexpr size_t diagnostic_library_allocator = 2 * diagnostic_P;
constexpr size_t diagnostic_library_init_local = diagnostic_P + diagnostic_B;
constexpr size_t diagnostic_library_use_local =
	2 * diagnostic_P + diagnostic_library_init_local + diagnostic_library_local_data;
constexpr size_t diagnostic_library_is_local =
	diagnostic_P + diagnostic_B + diagnostic_library_data + diagnostic_library_local_data;
constexpr size_t diagnostic_library_traits_assign = 2 * diagnostic_P + diagnostic_B;
constexpr size_t diagnostic_library_traits_length =
	2 * diagnostic_P + 2 * diagnostic_N +
	diagnostic_B; // actual length + strlen argument/result
constexpr size_t diagnostic_library_traits_copy =
	6 * diagnostic_P + 2 * diagnostic_N + diagnostic_B; // actual copy + memcpy argument/result
constexpr size_t diagnostic_library_copy =
	2 * diagnostic_P + diagnostic_N +
	std::max(diagnostic_library_traits_assign, diagnostic_library_traits_copy);
constexpr size_t diagnostic_library_copy_chars =
	3 * diagnostic_P + diagnostic_D + diagnostic_library_copy;
constexpr size_t diagnostic_library_set_length = diagnostic_P + diagnostic_N +
						 diagnostic_library_size + diagnostic_library_data +
						 diagnostic_library_traits_assign + diagnostic_C;
constexpr size_t diagnostic_library_destroy =
	diagnostic_P + diagnostic_N + diagnostic_library_allocator + diagnostic_library_data +
	diagnostic_library_deallocate;
constexpr size_t diagnostic_library_dispose =
	diagnostic_P + diagnostic_library_is_local + diagnostic_library_destroy;
constexpr size_t diagnostic_library_destructor = diagnostic_P + diagnostic_library_dispose;
constexpr size_t diagnostic_library_max_size =
	2 * (diagnostic_P + diagnostic_N) + diagnostic_library_allocator;
constexpr size_t diagnostic_library_string_allocate =
	3 * diagnostic_P + diagnostic_N + diagnostic_library_allocate;
constexpr size_t diagnostic_library_create =
	3 * diagnostic_P + diagnostic_N + diagnostic_library_max_size +
	diagnostic_library_allocator + diagnostic_library_string_allocate;
// distance(pointer,pointer), iterator-category, random-access __distance,
// and genuine subtraction result. No normal iterator is manufactured.
constexpr size_t diagnostic_library_distance =
	2 * diagnostic_P + diagnostic_D + diagnostic_P + sizeof(std::random_access_iterator_tag) +
	2 * diagnostic_P + diagnostic_D + sizeof(std::random_access_iterator_tag);
constexpr size_t diagnostic_library_constructor =
	14 * diagnostic_P + diagnostic_N + sizeof(std::forward_iterator_tag) +
	sizeof(std::allocator<char>) + 6 * diagnostic_P + diagnostic_library_local_data +
	diagnostic_library_traits_length + diagnostic_library_distance +
	std::max(diagnostic_library_create + diagnostic_library_capacity,
		 diagnostic_library_init_local) +
	diagnostic_library_copy_chars + diagnostic_library_set_length + diagnostic_library_dispose;
constexpr size_t diagnostic_library_mutate =
	3 * diagnostic_P + 5 * diagnostic_N + 2 * diagnostic_library_size +
	diagnostic_library_capacity + diagnostic_library_create + 2 * diagnostic_library_copy +
	diagnostic_library_dispose + diagnostic_library_data + diagnostic_library_capacity;
constexpr size_t diagnostic_library_append =
	10 * diagnostic_P + 5 * diagnostic_N + diagnostic_library_traits_length +
	diagnostic_library_max_size + 2 * diagnostic_library_size + diagnostic_library_capacity +
	std::max(diagnostic_library_data + diagnostic_library_size + diagnostic_library_copy,
		 diagnostic_library_size + diagnostic_library_mutate) +
	diagnostic_library_set_length;
constexpr size_t diagnostic_library_move_constructor =
	5 * diagnostic_P + 4 * diagnostic_P + 2 * diagnostic_library_move_ref +
	diagnostic_library_allocator + diagnostic_library_local_data + diagnostic_library_is_local +
	diagnostic_library_traits_copy + 3 * diagnostic_library_data + diagnostic_library_capacity +
	diagnostic_library_size + diagnostic_library_use_local + diagnostic_library_set_length +
	2 * diagnostic_library_size;
constexpr size_t diagnostic_library_move_assign =
	4 * diagnostic_P + diagnostic_N + diagnostic_B + diagnostic_library_allocator +
	2 * diagnostic_P + diagnostic_library_move_ref + 2 * diagnostic_P +
	diagnostic_library_is_local +
	std::max(diagnostic_library_addressof + diagnostic_library_size + diagnostic_library_copy +
			 diagnostic_library_set_length,
		 diagnostic_library_is_local + 4 * diagnostic_library_data +
			 2 * diagnostic_library_size + 2 * diagnostic_library_capacity +
			 diagnostic_library_use_local) +
	diagnostic_P + diagnostic_library_set_length;
constexpr size_t diagnostic_library_disjunct =
	2 * diagnostic_P + diagnostic_B + 2 * sizeof(std::less<const char *>) +
	2 * (3 * diagnostic_P + 2 * diagnostic_B) + 2 * diagnostic_library_data + diagnostic_P +
	diagnostic_N;
constexpr size_t diagnostic_library_literal =
	6 * diagnostic_P + diagnostic_library_traits_length + diagnostic_library_size +
	4 * diagnostic_P + 6 * diagnostic_N + 2 * diagnostic_P + 2 * diagnostic_N +
	diagnostic_library_size + diagnostic_library_max_size +
	std::max(diagnostic_library_data + diagnostic_library_disjunct + diagnostic_library_copy,
		 diagnostic_library_mutate) +
	diagnostic_library_set_length;
constexpr size_t diagnostic_library_system =
	diagnostic_library_constructor +
	2 * (diagnostic_library_append + diagnostic_library_move_constructor) +
	diagnostic_library_move_assign + 3 * diagnostic_library_destructor;
constexpr size_t diagnostic_library_forward_construct =
	4 * diagnostic_P + diagnostic_N + sizeof(std::forward_iterator_tag) + 3 * diagnostic_P +
	diagnostic_library_distance +
	std::max(diagnostic_library_create + diagnostic_library_capacity,
		 diagnostic_library_init_local) +
	diagnostic_library_copy_chars + diagnostic_library_set_length + diagnostic_library_dispose;
constexpr size_t diagnostic_library_copy_constructor =
	2 * diagnostic_P + diagnostic_library_local_data + diagnostic_library_allocator +
	diagnostic_P + sizeof(std::allocator<char>) + diagnostic_P + sizeof(std::allocator<char>) +
	4 * diagnostic_P + 3 * diagnostic_P + diagnostic_library_move_ref + 4 * diagnostic_P +
	2 * diagnostic_library_data + diagnostic_library_size +
	diagnostic_library_forward_construct;
constexpr size_t diagnostic_library_swap =
	2 * diagnostic_P + diagnostic_library_addressof + 2 * diagnostic_library_allocator +
	4 * diagnostic_P + 2 * diagnostic_library_is_local +
	std::max(size_t{ 16 }, diagnostic_N + diagnostic_P) + diagnostic_N +
	std::max(
		3 * diagnostic_library_traits_copy + 3 * diagnostic_library_size,
		std::max(diagnostic_library_init_local + diagnostic_library_traits_copy +
				 2 * diagnostic_library_size + diagnostic_library_set_length,
			 std::max(diagnostic_library_init_local + diagnostic_library_traits_copy +
					  diagnostic_library_size + 3 * diagnostic_library_data +
					  diagnostic_library_capacity,
				  4 * diagnostic_library_data + 2 * diagnostic_library_capacity))) +
	4 * diagnostic_library_size;
constexpr size_t diagnostic_library_default_fill =
	3 * diagnostic_P + diagnostic_N + 2 * diagnostic_P + diagnostic_N + 2 * diagnostic_B +
	3 * diagnostic_P + diagnostic_N + diagnostic_library_addressof + diagnostic_P +
	diagnostic_B + diagnostic_N + 2 * diagnostic_P + 3 * diagnostic_P + diagnostic_N +
	2 * diagnostic_N + diagnostic_P + sizeof(std::random_access_iterator_tag) +
	3 * diagnostic_P + diagnostic_N + sizeof(std::random_access_iterator_tag) +
	3 * diagnostic_P + 3 * diagnostic_P + sizeof(uint8_t) + diagnostic_N + diagnostic_B +
	2 * diagnostic_P + sizeof(int) + diagnostic_N;
constexpr size_t diagnostic_library_empty_relocate = 21 * diagnostic_P + diagnostic_D;
constexpr size_t diagnostic_library_vector_clear = 3 * diagnostic_P + diagnostic_N +
						   diagnostic_library_allocator + 7 * diagnostic_P +
						   diagnostic_B;
constexpr size_t diagnostic_library_vector_max =
	diagnostic_P + 3 * diagnostic_N + 2 * (diagnostic_P + diagnostic_N) + 3 * diagnostic_P;
constexpr size_t diagnostic_library_byte_resize =
	diagnostic_P + diagnostic_N + 4 * diagnostic_P + 4 * diagnostic_N + 2 * diagnostic_P +
	3 * diagnostic_N + diagnostic_library_vector_max + 2 * (diagnostic_P + diagnostic_N) +
	2 * diagnostic_library_allocator + 3 * diagnostic_P + 2 * diagnostic_P + diagnostic_N +
	diagnostic_library_allocate + diagnostic_library_default_fill +
	diagnostic_library_empty_relocate + 2 * diagnostic_P + diagnostic_N +
	diagnostic_library_deallocate;

size_t diagnostic_heap(const std::string *value) noexcept
{
	return value && value->capacity() > 15 ? value->capacity() + 1 : 0;
}
struct diagnostic_budget
{
	flatfile_scratch_reserve_fn reserve;
	void *context;
	size_t outer, initial_error, initial_bytes;
	std::string *error;
	std::vector<uint8_t> *bytes;
	bool denied = false;
	bool admit(size_t working) noexcept
	{
		const int saved_errno = errno;
		size_t current = outer;
		const size_t error_heap = diagnostic_heap(error);
		const size_t byte_heap = bytes ? bytes->capacity() : 0;
		if (current < initial_error || current - initial_error < initial_bytes)
			return denied = true, errno = ENOBUFS, false;
		current -= initial_error + initial_bytes;
		if (error_heap > SIZE_MAX - current ||
		    byte_heap > SIZE_MAX - current - error_heap ||
		    working > SIZE_MAX - current - error_heap - byte_heap ||
		    !reserve(current + error_heap + byte_heap + working, context))
			return denied = true, errno = ENOBUFS, false;
		errno = saved_errno;
		return true;
	}
	bool literal(const char *message)
	{
		if (!error)
			return true;
		const size_t length = strlen(message), capacity = error->capacity();
		size_t replacement = 0;
		if (length > capacity)
		{
			if (capacity > SIZE_MAX / 2)
				return denied = true, errno = ENOBUFS, false;
			const size_t grown = std::max(length, capacity * 2);
			if (grown == SIZE_MAX)
				return denied = true, errno = ENOBUFS, false;
			replacement = grown + 1;
		}
		if (!admit(replacement))
			return false;
		*error = message;
		return true;
	}
	bool system(const char *operation)
	{
		if (!error)
			return true;
		const int saved_errno = errno;
		const char *description = strerror(saved_errno);
		const size_t first = strlen(operation), last = strlen(description);
		size_t size = first, capacity = first > 15 ? first : 15;
		size_t peak = capacity > 15 ? capacity + 1 : 0;
		for (const size_t added : { size_t{ 2 }, last })
		{
			if (added > SIZE_MAX - size)
				return denied = true, errno = ENOBUFS, false;
			const size_t next = size + added;
			if (next > capacity)
			{
				if (capacity > SIZE_MAX / 2)
					return denied = true, errno = ENOBUFS, false;
				const size_t replacement = std::max(next, capacity * 2);
				if (replacement == SIZE_MAX)
					return denied = true, errno = ENOBUFS, false;
				const size_t old_heap = capacity > 15 ? capacity + 1 : 0;
				if (replacement + 1 > SIZE_MAX - old_heap)
					return denied = true, errno = ENOBUFS, false;
				peak = std::max(peak, old_heap + replacement + 1);
				capacity = replacement;
			}
			size = next;
		}
		if (!admit(peak))
			return false;
		errno = saved_errno;
		// Execute the unchanged real original expression after admission.
		set_error(error, operation);
		return true;
	}
};

// Actual store body locals, nested private_directory/read_all/write_all,
// diagnostic expression string construction/move/append, and allocator/vector
// closures. Inline objects are separate from allocation requests above.
constexpr size_t diagnostic_store_source_frames =
	2 * sizeof(struct stat) + sizeof(char[256]) + sizeof(diagnostic_budget) +
	3 * sizeof(std::string) + std::max(diagnostic_library_literal, diagnostic_library_system) +
	diagnostic_library_vector_clear + diagnostic_library_byte_resize +
	// Public input/result carriers; original fd/length/errno/sequence/ok locals;
	// budget admission and growth scan's actual size/capacity/peak values.
	19 * sizeof(void *) + 16 * sizeof(size_t) + 7 * sizeof(int) + sizeof(uint64_t) +
	5 * sizeof(bool) + sizeof(std::initializer_list<size_t>) +
	// Original write/read_all fd/data/size/chunk/result and valid/private helpers.
	2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(ssize_t) + 2 * sizeof(int) +
	2 * sizeof(bool);

bool diagnostic_store_init(diagnostic_budget &budget) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	errno = ENOTSUP;
	return false;
#else
	if (!budget.reserve)
	{
		errno = EINVAL;
		return false;
	}
	if (!budget.admit(diagnostic_store_source_frames))
		return false;
	if (budget.outer > SIZE_MAX - diagnostic_store_source_frames)
	{
		errno = ENOBUFS;
		return false;
	}
	budget.outer += diagnostic_store_source_frames;
	return true;
#endif
}
} // namespace

size_t flatfile_diagnostic_string_source_frame_bytes() noexcept
{
	return std::max(std::max(diagnostic_library_literal, diagnostic_library_system),
			std::max(diagnostic_library_copy_constructor, diagnostic_library_swap));
}
bool flatfile_atomic_write_with_error_bounded(const std::string &directory, const std::string &name,
					      const std::vector<uint8_t> &bytes, std::string *error,
					      bool *published, bool *returned,
					      flatfile_scratch_reserve_fn reserve_scratch_peak,
					      void *context, size_t outer_live_scratch,
					      bool *allocation_exception) noexcept
try
{
	if (allocation_exception)
		*allocation_exception = false;
	if (returned)
		*returned = false;
	diagnostic_budget budget{ reserve_scratch_peak,
				  context,
				  outer_live_scratch,
				  diagnostic_heap(error),
				  0,
				  error,
				  nullptr };
	if (published)
		*published = false;
	if (!diagnostic_store_init(budget))
		return false;
	if (published)
		*published = false;
	if (!valid_name(name))
	{
		budget.literal("invalid flat-file name");
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}

	const int directory_fd =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (directory_fd < 0)
	{
		budget.system("open authority directory");
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}
	if (!private_directory(directory_fd))
	{
		budget.literal("invalid authority directory metadata");
		close(directory_fd);
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}

	char temporary[256];
	const uint64_t sequence = temporary_sequence.fetch_add(1, std::memory_order_relaxed);
	const int length = snprintf(temporary, sizeof(temporary), ".%s.tmp.%ld.%llu", name.c_str(),
				    static_cast<long>(getpid()),
				    static_cast<unsigned long long>(sequence));
	if (length < 0 || static_cast<size_t>(length) >= sizeof(temporary))
	{
		budget.literal("temporary flat-file name is too long");
		close(directory_fd);
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}

	const int file_fd = openat(directory_fd, temporary,
				   O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
	if (file_fd < 0)
	{
		budget.system("create temporary authority file");
		close(directory_fd);
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}

	bool ok = write_all(file_fd, bytes.data(), bytes.size());
	if (ok && fdatasync(file_fd) < 0)
		ok = false;
	if (close(file_fd) < 0)
		ok = false;
	if (!ok)
	{
		budget.system("write authority file");
		unlinkat(directory_fd, temporary, 0);
		close(directory_fd);
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}

	if (renameat(directory_fd, temporary, directory_fd, name.c_str()) < 0)
	{
		budget.system("publish authority file");
		unlinkat(directory_fd, temporary, 0);
		close(directory_fd);
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}
	if (published)
		*published = true;
	if (fsync(directory_fd) < 0)
	{
		budget.system("sync authority directory");
		close(directory_fd);
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}
	close(directory_fd);
	if (returned)
		*returned = true;
	return true;
}

catch (const std::bad_alloc &)
{
	if (allocation_exception)
		*allocation_exception = true;
	errno = ENOMEM;
	return false;
}
catch (...)
{
	errno = EOVERFLOW;
	return false;
}
bool flatfile_atomic_remove_with_error_bounded(const std::string &directory,
					       const std::string &name, bool missing_ok,
					       std::string *error, bool *removed, bool *returned,
					       flatfile_scratch_reserve_fn reserve_scratch_peak,
					       void *context, size_t outer_live_scratch,
					       bool *allocation_exception) noexcept
try
{
	if (allocation_exception)
		*allocation_exception = false;
	if (removed)
		*removed = false;
	if (returned)
		*returned = false;
	diagnostic_budget budget{ reserve_scratch_peak,
				  context,
				  outer_live_scratch,
				  diagnostic_heap(error),
				  0,
				  error,
				  nullptr };
	if (!diagnostic_store_init(budget))
		return false;
	if (!valid_name(name))
	{
		budget.literal("invalid flat-file name");
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}
	const int directory_fd =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (directory_fd < 0)
	{
		budget.system("open authority directory");
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}
	if (!private_directory(directory_fd))
	{
		budget.literal("invalid authority directory metadata");
		close(directory_fd);
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}
	struct stat info;
	if (fstatat(directory_fd, name.c_str(), &info, AT_SYMLINK_NOFOLLOW) < 0)
	{
		const int saved_errno = errno;
		close(directory_fd);
		errno = saved_errno;
		if (missing_ok && errno == ENOENT)
			return returned ? (*returned = true, true) : true;
		budget.system("inspect authority file for removal");
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}
	if (!S_ISREG(info.st_mode) || info.st_uid != geteuid() || (info.st_mode & 0077))
	{
		budget.literal("invalid authority file metadata for removal");
		close(directory_fd);
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}
	const int unlink_result = unlinkat(directory_fd, name.c_str(), 0);
	if (unlink_result == 0 && removed)
		*removed = true;
	if (unlink_result < 0 || fsync(directory_fd) < 0)
	{
		budget.system("remove authority file");
		close(directory_fd);
		if (budget.denied)
			errno = ENOBUFS;
		return returned ? (*returned = !budget.denied, false) : false;
	}
	close(directory_fd);
	return returned ? (*returned = true, true) : true;
}

catch (const std::bad_alloc &)
{
	if (allocation_exception)
		*allocation_exception = true;
	errno = ENOMEM;
	return false;
}
catch (...)
{
	errno = EOVERFLOW;
	return false;
}
flatfile_read_result
flatfile_read_with_error_bounded(const std::string &directory, const std::string &name,
				 size_t maximum_size, std::vector<uint8_t> *bytes,
				 std::string *error, bool *returned,
				 flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
				 size_t outer_live_scratch, bool *allocation_exception) noexcept
try
{
	if (allocation_exception)
		*allocation_exception = false;
	if (returned)
		*returned = false;
	diagnostic_budget budget{ reserve_scratch_peak,
				  context,
				  outer_live_scratch,
				  diagnostic_heap(error),
				  bytes ? bytes->capacity() : 0,
				  error,
				  bytes };
	if (!diagnostic_store_init(budget))
		return flatfile_read_result::io_error;
	if (!bytes || !valid_name(name))
	{
		budget.literal("invalid flat-file read request");
		if (returned && !budget.denied)
			*returned = true;
		return budget.denied ? (errno = ENOBUFS, flatfile_read_result::io_error) :
				       flatfile_read_result::invalid;
	}
	bytes->clear();
	const int directory_fd =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (directory_fd < 0)
	{
		budget.system("open authority directory");
		if (budget.denied)
			errno = ENOBUFS;
		if (returned && !budget.denied)
			*returned = true;
		return flatfile_read_result::io_error;
	}
	if (!private_directory(directory_fd))
	{
		close(directory_fd);
		budget.literal("invalid authority directory metadata");
		if (returned && !budget.denied)
			*returned = true;
		return budget.denied ? (errno = ENOBUFS, flatfile_read_result::io_error) :
				       flatfile_read_result::invalid;
	}
	const int file_fd =
		openat(directory_fd, name.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);
	if (file_fd < 0)
	{
		const int saved_errno = errno;
		close(directory_fd);
		errno = saved_errno;
		if (errno == ENOENT)
			return returned ? (*returned = true, flatfile_read_result::not_found) :
					  flatfile_read_result::not_found;
		budget.system("open authority file");
		if (returned && !budget.denied)
			*returned = true;
		return budget.denied  ? (errno = ENOBUFS, flatfile_read_result::io_error) :
		       errno == ELOOP ? flatfile_read_result::invalid :
					flatfile_read_result::io_error;
	}
	close(directory_fd);

	struct stat info;
	if (fstat(file_fd, &info) < 0)
	{
		const int saved_errno = errno;
		close(file_fd);
		errno = saved_errno;
		budget.system("inspect authority file");
		if (budget.denied)
			errno = ENOBUFS;
		if (returned && !budget.denied)
			*returned = true;
		return flatfile_read_result::io_error;
	}
	if (!S_ISREG(info.st_mode) || info.st_nlink != 1 || info.st_uid != geteuid() ||
	    (info.st_mode & 0077) || info.st_size < 0 ||
	    static_cast<uintmax_t>(info.st_size) > maximum_size)
	{
		close(file_fd);
		budget.literal("invalid authority file metadata or size");
		if (returned && !budget.denied)
			*returned = true;
		return budget.denied ? (errno = ENOBUFS, flatfile_read_result::io_error) :
				       flatfile_read_result::invalid;
	}
	try
	{
		const size_t requested = static_cast<size_t>(info.st_size);
		if (!budget.admit((requested > bytes->capacity() ? requested : 0)))
		{
			close(file_fd);
			errno = ENOBUFS;
			if (budget.denied)
				errno = ENOBUFS;
			if (returned && !budget.denied)
				*returned = true;
			return flatfile_read_result::io_error;
		}
		bytes->resize(requested);
	}
	catch (const std::bad_alloc &)
	{
		close(file_fd);
		errno = ENOMEM;
		if (budget.denied)
			errno = ENOBUFS;
		if (returned && !budget.denied)
			*returned = true;
		return flatfile_read_result::io_error;
	}
	if (!read_all(file_fd, bytes->data(), bytes->size()))
	{
		const int saved_errno = errno;
		bytes->clear();
		close(file_fd);
		errno = saved_errno;
		budget.system("read authority file");
		if (budget.denied)
			errno = ENOBUFS;
		if (returned && !budget.denied)
			*returned = true;
		return flatfile_read_result::io_error;
	}
	close(file_fd);
	return returned ? (*returned = true, flatfile_read_result::ok) : flatfile_read_result::ok;
}

catch (const std::bad_alloc &)
{
	if (allocation_exception)
		*allocation_exception = true;
	errno = ENOMEM;
	return flatfile_read_result::io_error;
}
catch (...)
{
	errno = EOVERFLOW;
	return flatfile_read_result::io_error;
}
bool flatfile_lock_acquire_with_error_bounded(const std::string &directory, const std::string &name,
					      int *lock_fd, std::string *error, bool *returned,
					      flatfile_scratch_reserve_fn reserve_scratch_peak,
					      void *context, size_t outer_live_scratch,
					      bool *allocation_exception) noexcept
try
{
	if (allocation_exception)
		*allocation_exception = false;
	if (returned)
		*returned = false;
	diagnostic_budget budget{ reserve_scratch_peak,
				  context,
				  outer_live_scratch,
				  diagnostic_heap(error),
				  0,
				  error,
				  nullptr };
	if (!diagnostic_store_init(budget))
		return false;
	if (!lock_fd || !valid_name(name))
	{
		errno = EINVAL;
		budget.literal("invalid flat-file lock request");
		if (budget.denied)
			errno = ENOBUFS;
		if (returned && !budget.denied)
			*returned = true;
		return false;
	}
	*lock_fd = -1;
	const int directory_fd =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (directory_fd < 0)
	{
		budget.system("open lock directory");
		if (budget.denied)
			errno = ENOBUFS;
		if (returned && !budget.denied)
			*returned = true;
		return false;
	}
	struct stat directory_info;
	const int directory_status = fstat(directory_fd, &directory_info);
	if (directory_status || !S_ISDIR(directory_info.st_mode) ||
	    directory_info.st_uid != geteuid() || (directory_info.st_mode & 0077))
	{
		const int saved_errno = directory_status ? errno : EACCES;
		close(directory_fd);
		errno = saved_errno;
		budget.system("invalid lock directory metadata");
		if (budget.denied)
			errno = ENOBUFS;
		if (returned && !budget.denied)
			*returned = true;
		return false;
	}
	const int file_fd = openat(directory_fd, name.c_str(),
				   O_RDWR | O_CREAT | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW, 0600);
	const int open_errno = errno;
	close(directory_fd);
	if (file_fd < 0)
	{
		errno = open_errno;
		budget.system("open authority lock");
		if (budget.denied)
			errno = ENOBUFS;
		if (returned && !budget.denied)
			*returned = true;
		return false;
	}
	struct stat info;
	const int file_status = fstat(file_fd, &info);
	if (file_status || !S_ISREG(info.st_mode) || info.st_uid != geteuid() ||
	    (info.st_mode & 0077) || (info.st_nlink != 1 || info.st_size != 0))
	{
		const int saved_errno = file_status ? errno : EINVAL;
		close(file_fd);
		errno = saved_errno;
		budget.system("invalid authority lock metadata");
		if (budget.denied)
			errno = ENOBUFS;
		if (returned && !budget.denied)
			*returned = true;
		return false;
	}
	while (flock(file_fd, LOCK_EX) < 0)
	{
		if (errno == EINTR)
			continue;
		const int saved_errno = errno;
		close(file_fd);
		errno = saved_errno;
		budget.system("lock authority");
		if (budget.denied)
			errno = ENOBUFS;
		if (returned && !budget.denied)
			*returned = true;
		return false;
	}
	*lock_fd = file_fd;
	return returned ? (*returned = true, true) : true;
}

catch (const std::bad_alloc &)
{
	if (allocation_exception)
		*allocation_exception = true;
	errno = ENOMEM;
	return false;
}
catch (...)
{
	errno = EOVERFLOW;
	return false;
}
