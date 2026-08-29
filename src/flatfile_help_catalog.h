#ifndef DURIS_FLATFILE_HELP_CATALOG_H
#define DURIS_FLATFILE_HELP_CATALOG_H

#include <cstddef>
#include <string>
#include <vector>

struct flatfile_help_entry
{
	std::string title;
	std::string text;
};

struct flatfile_help_catalog
{
	std::vector<flatfile_help_entry> entries;
};

bool flatfile_help_catalog_load(const std::string &project_root, flatfile_help_catalog *catalog,
				std::string *error);
bool flatfile_information_read(const std::string &project_root, const std::string &name,
			       std::string *contents, std::string *error);
const flatfile_help_entry *flatfile_help_catalog_find(const flatfile_help_catalog &catalog,
						      const std::string &title);
std::vector<const flatfile_help_entry *>
flatfile_help_catalog_search(const flatfile_help_catalog &catalog, const std::string &query,
			     size_t limit);

#endif
