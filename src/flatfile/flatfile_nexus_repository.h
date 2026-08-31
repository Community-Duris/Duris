#ifndef DURIS_FLATFILE_NEXUS_REPOSITORY_H
#define DURIS_FLATFILE_NEXUS_REPOSITORY_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

constexpr size_t FLATFILE_NEXUS_MAX_RECORDS = 9;
constexpr size_t FLATFILE_NEXUS_NAME_MAX_BYTES = 255;

struct flatfile_nexus_record
{
	int32_t id = 0;
	std::string name;
	int32_t room_vnum = 0;
	int32_t align = 0;
	int32_t stat_affect = -1;
	int32_t affect_amount = 0;
	int64_t last_touched_at = 0;
	int32_t bonus = 0;
};

enum class flatfile_nexus_result
{
	ok,
	not_found,
	unchanged,
	invalid,
	io_error
};

flatfile_nexus_result flatfile_nexus_establish(const std::string &root,
					       const std::vector<flatfile_nexus_record> &records,
					       std::string *error);
flatfile_nexus_result flatfile_nexus_list(const std::string &root,
					  std::vector<flatfile_nexus_record> *records,
					  std::string *error);
flatfile_nexus_result flatfile_nexus_find(const std::string &root, int32_t id,
					  flatfile_nexus_record *record, std::string *error);
flatfile_nexus_result flatfile_nexus_update_state(const std::string &root, int32_t id,
						  int32_t align, int64_t last_touched_at,
						  std::string *error);

#endif
