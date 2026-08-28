#include "flatfile_help_catalog.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <new>
#include <unordered_map>

namespace
{
constexpr size_t source_maximum_bytes = 8 * 1024 * 1024;
constexpr size_t entry_maximum = 4096;
constexpr size_t title_maximum = 255;
constexpr size_t text_maximum = 1024 * 1024;

struct help_source
{
	const char *path;
	const char *title;
};

constexpr help_source individual_sources[] = {
	{ "lib/information/help", "help" },
	{ "lib/information/help.1", "help commands" },
	{ "lib/information/help.2", "help advanced" },
	{ "lib/information/helpguild1", "guilds" },
	{ "lib/information/helpguild2", "guild commands" },
	{ "lib/information/helpships", "ships" },
	{ "lib/information/helpkingdoms", "kingdoms" },
	{ "lib/information/faq", "faq" },
	{ "lib/information/rules", "rules" },
	{ "lib/information/info", "info" },
	{ "lib/information/credits", "credits" },
	{ "lib/information/wizlist", "wizlist" },
	{ "docs/lib/information/hints.txt", "hints" },
};

std::string trim(const std::string &value)
{
	const size_t first = value.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return {};
	return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}

std::string canonical(const std::string &value)
{
	std::string result = value;
	for (char &character : result)
		if (character >= 'A' && character <= 'Z')
			character = static_cast<char>(character - 'A' + 'a');
	return result;
}

bool separator(const std::string &line)
{
	const std::string value = trim(line);
	return !value.empty() && std::all_of(value.begin(), value.end(),
					     [](char character) { return character == '='; });
}

bool valid_entry(const flatfile_help_entry &entry)
{
	if (entry.title.empty() || entry.title.size() > title_maximum || entry.text.empty() ||
	    entry.text.size() > text_maximum || entry.title.find('\0') != std::string::npos ||
	    entry.text.find('\0') != std::string::npos)
		return false;
	for (unsigned char character : entry.title)
		if (character < 0x20 || character == 0x7f)
			return false;
	return true;
}

enum class read_result
{
	ok,
	not_found,
	invalid,
};

read_result read_source(const std::string &path, std::string *contents)
{
	if (!contents)
		return read_result::invalid;
	std::ifstream file(path, std::ios::binary);
	if (!file)
		return read_result::not_found;
	file.seekg(0, std::ios::end);
	const std::streamoff size = file.tellg();
	if (size < 0 || size > static_cast<std::streamoff>(source_maximum_bytes))
		return read_result::invalid;
	file.seekg(0, std::ios::beg);
	try
	{
		contents->assign(std::istreambuf_iterator<char>(file),
				 std::istreambuf_iterator<char>());
	}
	catch (const std::bad_alloc &)
	{
		return read_result::invalid;
	}
	return file.bad() || contents->find('\0') != std::string::npos ? read_result::invalid :
									 read_result::ok;
}

std::vector<std::string> lines(const std::string &contents)
{
	std::vector<std::string> result;
	size_t begin = 0;
	while (begin <= contents.size())
	{
		const size_t end = contents.find('\n', begin);
		std::string line = contents.substr(
			begin, end == std::string::npos ? contents.size() - begin : end - begin);
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		result.push_back(std::move(line));
		if (end == std::string::npos)
			break;
		begin = end + 1;
	}
	return result;
}

std::string join_content(const std::vector<std::string> &source, size_t begin, size_t end,
			 bool strip_separators)
{
	while (begin < end && trim(source[begin]).empty())
		++begin;
	while (end > begin && trim(source[end - 1]).empty())
		--end;
	if (strip_separators && begin < end && separator(source[begin]))
		++begin;
	if (strip_separators && end > begin && separator(source[end - 1]))
		--end;
	std::string result;
	for (size_t index = begin; index < end; ++index)
	{
		if (!result.empty())
			result.push_back('\n');
		result += source[index];
	}
	return trim(result);
}

bool publish(flatfile_help_entry entry,
	     std::unordered_map<std::string, flatfile_help_entry> *entries)
{
	if (!entries || !valid_entry(entry))
		return false;
	try
	{
		(*entries)[canonical(entry.title)] = std::move(entry);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	return entries->size() <= entry_maximum;
}

bool parse_help_index(const std::string &contents,
		      std::unordered_map<std::string, flatfile_help_entry> *entries)
{
	const auto source = lines(contents);
	size_t begin = 0;
	while (begin < source.size())
	{
		size_t end = begin;
		while (end < source.size() && trim(source[end]) != "#")
			++end;
		size_t first = begin;
		while (first < end && trim(source[first]).empty())
			++first;
		if (first < end && canonical(trim(source[first])).rfind("last update:", 0) != 0)
		{
			const std::string title_line = trim(source[first]);
			std::string title;
			if (!title_line.empty() && title_line.front() == '"')
			{
				const size_t quote = title_line.find('"', 1);
				if (quote != std::string::npos)
					title = trim(title_line.substr(1, quote - 1));
			}
			else
			{
				const size_t parenthesis = title_line.find('(');
				title = trim(title_line.substr(0, parenthesis));
			}
			const std::string text = join_content(source, first + 1, end, true);
			if (!title.empty() && !text.empty() && !publish({ title, text }, entries))
				return false;
		}
		begin = end + 1;
	}
	return true;
}

bool parse_parsed_help(const std::string &contents,
		       std::unordered_map<std::string, flatfile_help_entry> *entries)
{
	const auto source = lines(contents);
	size_t begin = 0;
	while (begin < source.size())
	{
		size_t end = begin;
		while (end < source.size() && trim(source[end]) != "#0")
			++end;
		size_t first = begin;
		while (first < end && trim(source[first]).empty())
			++first;
		if (first + 1 < end)
		{
			const std::string heading = trim(source[first + 1]);
			if (heading.empty() || heading.rfind("==", 0) == 0 ||
			    heading.front() == '*')
			{
				begin = end + 1;
				continue;
			}
			const std::string marker = " - Last Edited:";
			const size_t marker_at = heading.find(marker);
			const std::string title = trim(heading.substr(
				0, marker_at == std::string::npos ? heading.size() : marker_at));
			const std::string text = join_content(source, first + 1, end, false);
			if (!title.empty() && text.size() > 10 &&
			    !publish({ title, text }, entries))
				return false;
		}
		begin = end + 1;
	}
	return true;
}
} // namespace

bool flatfile_help_catalog_load(const std::string &project_root, flatfile_help_catalog *catalog,
				std::string *error)
{
	if (project_root.empty() || !catalog)
		return false;
	std::unordered_map<std::string, flatfile_help_entry> entries;
	for (const auto &source : individual_sources)
	{
		std::string contents;
		const read_result read = read_source(project_root + "/" + source.path, &contents);
		if (read == read_result::invalid)
		{
			if (error)
				*error = std::string("invalid help source: ") + source.path;
			return false;
		}
		if (read == read_result::ok && !contents.empty() &&
		    !publish({ source.title, std::move(contents) }, &entries))
			return false;
	}

	bool loaded_catalog_source = false;
	std::string contents;
	read_result read = read_source(project_root + "/lib/information/help_index", &contents);
	if (read == read_result::invalid ||
	    (read == read_result::ok && !parse_help_index(contents, &entries)))
	{
		if (error)
			*error = "invalid help index";
		return false;
	}
	loaded_catalog_source |= read == read_result::ok;
	contents.clear();
	read = read_source(project_root + "/help/duris_help_parsed.hlp", &contents);
	if (read == read_result::invalid ||
	    (read == read_result::ok && !parse_parsed_help(contents, &entries)))
	{
		if (error)
			*error = "invalid parsed help catalog";
		return false;
	}
	loaded_catalog_source |= read == read_result::ok;
	if (!loaded_catalog_source || entries.empty())
	{
		if (error)
			*error = "no flat-file help catalog source found";
		return false;
	}
	try
	{
		catalog->entries.clear();
		catalog->entries.reserve(entries.size());
		for (auto &entry : entries)
			catalog->entries.push_back(std::move(entry.second));
		std::sort(catalog->entries.begin(), catalog->entries.end(),
			  [](const auto &left, const auto &right)
			  { return canonical(left.title) < canonical(right.title); });
	}
	catch (const std::bad_alloc &)
	{
		if (error)
			*error = "flat-file help catalog allocation failed";
		return false;
	}
	return true;
}

const flatfile_help_entry *flatfile_help_catalog_find(const flatfile_help_catalog &catalog,
						      const std::string &title)
{
	const std::string key = canonical(title);
	const auto found = std::lower_bound(catalog.entries.begin(), catalog.entries.end(), key,
					    [](const auto &entry, const auto &candidate)
					    { return canonical(entry.title) < candidate; });
	return found != catalog.entries.end() && canonical(found->title) == key ? &*found : nullptr;
}

std::vector<const flatfile_help_entry *>
flatfile_help_catalog_search(const flatfile_help_catalog &catalog, const std::string &query,
			     size_t limit)
{
	std::vector<const flatfile_help_entry *> matches;
	if (!limit)
		return matches;
	const std::string key = canonical(query);
	try
	{
		matches.reserve(std::min(limit, catalog.entries.size()));
		for (const auto &entry : catalog.entries)
			if (canonical(entry.title).find(key) != std::string::npos)
			{
				matches.push_back(&entry);
				if (matches.size() == limit)
					break;
			}
	}
	catch (const std::bad_alloc &)
	{
		matches.clear();
	}
	return matches;
}
