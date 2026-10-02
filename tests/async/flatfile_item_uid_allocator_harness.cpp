#include "flatfile/flatfile_item_uid_allocator.h"
#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

bool fail_initialization_publication = false;
bool flatfile_test_atomic_write(const std::string &, const std::string &,
				const std::vector<uint8_t> &, std::string *);
bool flatfile_atomic_write(const std::string &directory, const std::string &name,
			   const std::vector<uint8_t> &bytes, std::string *error)
{
	if (fail_initialization_publication && name == "item_uid_allocator.initialized")
	{
		if (error)
			*error = "injected initialization publication failure";
		return false;
	}
	return flatfile_test_atomic_write(directory, name, bytes, error);
}

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static bool write_all(int fd, const void *data, size_t size)
{
	const auto *bytes = static_cast<const unsigned char *>(data);
	while (size)
	{
		const ssize_t written = write(fd, bytes, size);
		if (written < 0 && errno == EINTR)
			continue;
		if (written <= 0)
			return false;
		bytes += written;
		size -= static_cast<size_t>(written);
	}
	return true;
}

static bool read_all(int fd, void *data, size_t size)
{
	auto *bytes = static_cast<unsigned char *>(data);
	while (size)
	{
		const ssize_t received = read(fd, bytes, size);
		if (received < 0 && errno == EINTR)
			continue;
		if (received <= 0)
			return false;
		bytes += received;
		size -= static_cast<size_t>(received);
	}
	return true;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const fs::path metadata = root / "metadata";
	fs::create_directories(metadata);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(metadata, fs::perms::owner_all, fs::perm_options::replace);

	std::string error;
	uint64_t first = 0;
	require(flatfile_item_uid_reserve(root.string(), 3, &first, &error) ==
				flatfile_item_uid_result::ok &&
			first == 1,
		"initial reservation failed: " + error);
	uint64_t next_uid = 0, revision = 0;
	require(flatfile_item_uid_current(root.string(), &next_uid, &revision, &error) ==
				flatfile_item_uid_result::ok &&
			next_uid == 4 && revision == 1,
		"initial allocator state was not published");
	require(flatfile_item_uid_reserve(root.string(), 0, &first, &error) ==
			flatfile_item_uid_result::invalid,
		"zero-sized reservation was accepted");

	std::vector<uint8_t> early_allocator;
	require(flatfile_read(metadata.string(), "item_uid_allocator", 4096, &early_allocator,
			      &error) == flatfile_read_result::ok,
		"could not preserve early allocator state");

	int descriptors[2];
	require(pipe(descriptors) == 0, "could not create allocator result pipe");
	constexpr size_t child_count = 4;
	for (size_t child = 0; child < child_count; ++child)
	{
		const pid_t pid = fork();
		require(pid >= 0, "allocator writer fork failed");
		if (!pid)
		{
			close(descriptors[0]);
			std::string child_error;
			uint64_t child_first = 0;
			const bool reserved = flatfile_item_uid_reserve(root.string(), 25,
									&child_first,
									&child_error) ==
					      flatfile_item_uid_result::ok;
			const bool reported = reserved && write_all(descriptors[1], &child_first,
								    sizeof(child_first));
			close(descriptors[1]);
			_exit(reported ? 0 : 2);
		}
	}
	close(descriptors[1]);
	std::array<uint64_t, child_count> starts = {};
	for (uint64_t &start : starts)
		require(read_all(descriptors[0], &start, sizeof(start)),
			"could not read allocator child result");
	close(descriptors[0]);
	for (size_t child = 0; child < child_count; ++child)
	{
		int status = 0;
		require(wait(&status) > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0,
			"concurrent allocator writer failed");
	}
	std::sort(starts.begin(), starts.end());
	require(starts == std::array<uint64_t, child_count>{ 4, 29, 54, 79 },
		"concurrent reservations overlapped or skipped unexpected ranges");
	require(flatfile_item_uid_current(root.string(), &next_uid, &revision, &error) ==
				flatfile_item_uid_result::ok &&
			next_uid == 104 && revision == 5,
		"concurrent allocator high-water mark was incorrect");

	std::vector<uint8_t> current_allocator;
	require(flatfile_read(metadata.string(), "item_uid_allocator", 4096, &current_allocator,
			      &error) == flatfile_read_result::ok,
		"could not preserve current allocator state");
	require(flatfile_atomic_write(metadata.string(), "item_uid_allocator", early_allocator,
				      &error),
		"could not restore older valid allocator state");
	first = 777;
	require(flatfile_item_uid_reserve(root.string(), 1, &first, &error) ==
				flatfile_item_uid_result::invalid &&
			first == 777,
		"older valid allocator reused previously issued item UIDs");
	next_uid = 888;
	revision = 999;
	require(flatfile_item_uid_current(root.string(), &next_uid, &revision, &error) ==
				flatfile_item_uid_result::invalid &&
			next_uid == 888 && revision == 999,
		"older valid allocator appeared current");
	std::vector<uint8_t> refused_allocator;
	require(flatfile_read(metadata.string(), "item_uid_allocator", 4096, &refused_allocator,
			      &error) == flatfile_read_result::ok &&
			refused_allocator == early_allocator,
		"rollback refusal rewrote the allocator");
	require(flatfile_atomic_write(metadata.string(), "item_uid_allocator", current_allocator,
				      &error),
		"could not restore exact current allocator state");

	const fs::path allocator = metadata / "item_uid_allocator";
	const fs::path preserved = root / "preserved_allocator";
	fs::rename(allocator, preserved);
	first = 777;
	require(flatfile_item_uid_reserve(root.string(), 1, &first, &error) ==
				flatfile_item_uid_result::invalid &&
			first == 777,
		"missing issued allocator restarted at UID 1");
	next_uid = 888;
	revision = 999;
	require(flatfile_item_uid_current(root.string(), &next_uid, &revision, &error) ==
				flatfile_item_uid_result::invalid &&
			next_uid == 888 && revision == 999,
		"missing issued allocator appeared to be an uninitialized authority");
	require(!fs::exists(allocator), "missing allocator was silently recreated");
	fs::rename(preserved, allocator);
	require(flatfile_item_uid_current(root.string(), &next_uid, &revision, &error) ==
				flatfile_item_uid_result::ok &&
			next_uid == 104 && revision == 5,
		"restored allocator high-water mark changed");

	const fs::path initialized = metadata / "item_uid_allocator.initialized";
	require(fs::exists(initialized),
		"successful reservation lacks durable initialization evidence");
	const fs::path preserved_marker = root / "preserved_marker";
	fs::rename(initialized, preserved_marker);
	require(flatfile_item_uid_reserve(root.string(), 1, &first, &error) ==
				flatfile_item_uid_result::ok &&
			first == 104 && fs::exists(initialized),
		"legacy allocator upgrade did not preserve its high-water mark");
	{
		std::fstream file(initialized, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open initialization marker for corruption test");
		file.put('X');
	}
	first = 777;
	require(flatfile_item_uid_reserve(root.string(), 1, &first, &error) ==
				flatfile_item_uid_result::invalid &&
			first == 777,
		"corrupt initialization marker allowed a reservation");
	fs::remove(initialized);
	fs::rename(preserved_marker, initialized);
	require(flatfile_item_uid_current(root.string(), &next_uid, &revision, &error) ==
				flatfile_item_uid_result::ok &&
			next_uid == 105 && revision == 6,
		"marker refusal modified allocator authority");

	// Upgrade the original eight-byte initialization marker without restarting
	// the allocator or discarding its high-water mark.
	const std::vector<uint8_t> legacy_marker{ 'D', 'U', 'R', 'U', 'I', 'D', 1, 0 };
	require(flatfile_atomic_write(metadata.string(), "item_uid_allocator.initialized",
				      legacy_marker, &error),
		"could not install legacy marker");
	require(flatfile_item_uid_reserve(root.string(), 1, &first, &error) ==
				flatfile_item_uid_result::ok &&
			first == 105,
		"legacy initialization marker upgrade reused item UIDs");
	std::vector<uint8_t> sealed_marker;
	require(flatfile_read(metadata.string(), "item_uid_allocator.initialized", 4096,
			      &sealed_marker, &error) == flatfile_read_result::ok &&
			sealed_marker.size() == 56 && sealed_marker[6] == 2,
		"legacy marker was not upgraded to sealed high-water evidence");
	for (size_t offset : { size_t{ 8 }, size_t{ 16 } })
	{
		auto damaged_marker = sealed_marker;
		damaged_marker[offset] ^= 0x33;
		require(flatfile_atomic_write(metadata.string(), "item_uid_allocator.initialized",
					      damaged_marker, &error),
			"could not inject marker damage");
		first = 777;
		next_uid = 888;
		revision = 999;
		require(flatfile_item_uid_reserve(root.string(), 1, &first, &error) ==
					flatfile_item_uid_result::invalid &&
				first == 777,
			"damaged high-water witness allowed a reservation");
		require(flatfile_item_uid_current(root.string(), &next_uid, &revision, &error) ==
					flatfile_item_uid_result::invalid &&
				next_uid == 888 && revision == 999,
			"damaged high-water witness appeared current");
		require(flatfile_atomic_write(metadata.string(), "item_uid_allocator.initialized",
					      sealed_marker, &error),
			"could not restore sealed marker");
	}

	// An older root may predate the marker. Surviving custody still proves that
	// a missing allocator cannot safely be bootstrapped from UID 1.
	const fs::path domains = root / "domains";
	fs::create_directories(domains);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);
	{
		std::ofstream file(domains / "item_ownership");
		file << "surviving custody evidence";
	}
	fs::rename(initialized, preserved_marker);
	fs::rename(allocator, preserved);
	require(flatfile_item_uid_reserve(root.string(), 1, &first, &error) ==
				flatfile_item_uid_result::invalid &&
			!fs::exists(allocator),
		"legacy custody with missing allocator was bootstrapped");
	fs::rename(preserved, allocator);
	fs::rename(preserved_marker, initialized);

	const fs::path interrupted = root / "interrupted";
	fs::create_directories(interrupted / "metadata");
	fs::permissions(interrupted, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(interrupted / "metadata", fs::perms::owner_all, fs::perm_options::replace);
	first = 777;
	fail_initialization_publication = true;
	require(flatfile_item_uid_reserve(interrupted.string(), 3, &first, &error) ==
				flatfile_item_uid_result::io_error &&
			first == 777,
		"marker publication failure released an item UID");
	fail_initialization_publication = false;
	require(flatfile_item_uid_current(interrupted.string(), &next_uid, &revision, &error) ==
				flatfile_item_uid_result::ok &&
			next_uid == 4 && revision == 1,
		"marker failure rolled back the consumed reservation");
	require(flatfile_item_uid_reserve(interrupted.string(), 1, &first, &error) ==
				flatfile_item_uid_result::ok &&
			first == 4 &&
			fs::exists(interrupted / "metadata" / "item_uid_allocator.initialized"),
		"marker publication retry reused a consumed range");
	// Publication failure must also burn a later range after initialization,
	// leaving the previous witness intact until a later successful reservation.
	const fs::path interrupted_metadata = interrupted / "metadata";
	std::vector<uint8_t> prior_marker;
	require(flatfile_read(interrupted_metadata.string(), "item_uid_allocator.initialized", 4096,
			      &prior_marker, &error) == flatfile_read_result::ok,
		"could not read initialized failure fixture witness");
	first = 777;
	fail_initialization_publication = true;
	require(flatfile_item_uid_reserve(interrupted.string(), 2, &first, &error) ==
				flatfile_item_uid_result::io_error &&
			first == 777,
		"later witness failure released item UIDs");
	fail_initialization_publication = false;
	std::vector<uint8_t> failed_marker;
	require(flatfile_read(interrupted_metadata.string(), "item_uid_allocator.initialized", 4096,
			      &failed_marker, &error) == flatfile_read_result::ok &&
			failed_marker == prior_marker,
		"failed later witness publication replaced prior evidence");
	require(flatfile_item_uid_current(interrupted.string(), &next_uid, &revision, &error) ==
				flatfile_item_uid_result::ok &&
			next_uid == 7 && revision == 3,
		"later witness failure rolled back allocator high-water");
	require(flatfile_item_uid_reserve(interrupted.string(), 1, &first, &error) ==
				flatfile_item_uid_result::ok &&
			first == 7,
		"later witness retry reused a burned reservation");
	{
		std::fstream file(allocator, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open allocator for corruption test");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x33;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_item_uid_current(root.string(), &next_uid, &revision, &error) ==
			flatfile_item_uid_result::invalid,
		"corrupt allocator checksum was accepted");
	require(flatfile_item_uid_reserve(root.string(), 1, &first, &error) ==
			flatfile_item_uid_result::invalid,
		"corrupt allocator was overwritten as a new authority");
	for (const fs::directory_entry &entry : fs::directory_iterator(metadata))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary allocator file was left behind");

	std::cout
		<< "flat-file item UID allocator: concurrency, missing/stale authority, legacy witness upgrade, sealed bounds and publication-failure burn passed\n";
	return 0;
}
