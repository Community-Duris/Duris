#include "flatfile_item_uid_allocator.h"

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

	const fs::path allocator = metadata / "item_uid_allocator";
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

	std::cout << "flat-file item UID allocator passed\n";
	return 0;
}
