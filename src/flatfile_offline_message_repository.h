#ifndef DURIS_FLATFILE_OFFLINE_MESSAGE_REPOSITORY_H
#define DURIS_FLATFILE_OFFLINE_MESSAGE_REPOSITORY_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

constexpr size_t FLATFILE_OFFLINE_MESSAGE_ID_BYTES = 16;
constexpr size_t FLATFILE_OFFLINE_MESSAGE_MAX_BYTES = 4096;

using flatfile_offline_message_id = std::array<uint8_t, FLATFILE_OFFLINE_MESSAGE_ID_BYTES>;

struct flatfile_offline_message_record
{
	flatfile_offline_message_id id = {};
	uint64_t created_at = 0;
	std::string text;
};

enum class flatfile_offline_message_result
{
	ok,
	not_found,
	invalid,
	io_error,
	full,
	conflict,
};

flatfile_offline_message_result
flatfile_offline_message_enqueue(const std::string &root, uint32_t pid,
				 const flatfile_offline_message_id &id, const std::string &text,
				 std::string *error);
flatfile_offline_message_result
flatfile_offline_message_list(const std::string &root, uint32_t pid,
			      std::vector<flatfile_offline_message_record> *messages,
			      std::string *error);
flatfile_offline_message_result
flatfile_offline_message_acknowledge(const std::string &root, uint32_t pid,
				     const flatfile_offline_message_id &id, std::string *error);

#endif
