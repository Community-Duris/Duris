#ifndef DURIS_FLATFILE_RECIPE_REPOSITORY_H
#define DURIS_FLATFILE_RECIPE_REPOSITORY_H

#include "flatfile_authority_transaction.h"

#include <cstdint>
#include <string>
#include <vector>

struct flatfile_recipe_record
{
	uint32_t pid = 0;
	std::vector<int32_t> recipes;
	bool operator==(const flatfile_recipe_record &) const = default;
};

enum class flatfile_recipe_result
{
	ok,
	not_found,
	already_exists,
	invalid,
	io_error
};

flatfile_recipe_result flatfile_recipe_establish(const std::string &root,
						 const std::vector<flatfile_recipe_record> &records,
						 std::string *error);
flatfile_recipe_result flatfile_recipe_list(const std::string &root, uint32_t pid,
					    std::vector<int32_t> *recipes, std::string *error);
flatfile_recipe_result flatfile_recipe_contains(const std::string &root, uint32_t pid,
						int32_t recipe_vnum, bool *contains,
						std::string *error);
flatfile_recipe_result flatfile_recipe_add(const std::string &root, uint32_t pid,
					   int32_t recipe_vnum, std::string *error);
flatfile_recipe_result flatfile_recipe_clear(const std::string &root, uint32_t pid,
					     std::string *error);
flatfile_recipe_result flatfile_recipe_prepare_clear(const std::string &root,
						     const flatfile_authority_lock &lock,
						     uint32_t pid,
						     flatfile_authority_operation *operation,
						     std::string *error);

#endif
