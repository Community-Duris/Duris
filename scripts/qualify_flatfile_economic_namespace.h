// Private bounded filename inventory and independent physical reverse checks.
#ifndef DURIS_QUALIFY_FLATFILE_ECONOMIC_NAMESPACE_H
#define DURIS_QUALIFY_FLATFILE_ECONOMIC_NAMESPACE_H
#include "qualify_flatfile_economic_records.h"
#include <charconv>
#include <iostream>

namespace restore_economic_namespace
{
using namespace restore_economic_authority;
constexpr size_t names_per_chunk = 128;
constexpr size_t entry_limit = 2 * buckets * bucket_capacity + 4096 * 18 +
			       buckets * (3 + restore_economic_records::segment_count_limit) +
			       8192 + 2;
constexpr size_t inventory_limit = 128 * 1024 * 1024;
struct chunk
{
	uint64_t offset;
	size_t size, count;
	digest checksum;
};
inline bytes name_bytes(const std::string &name)
{
	return { name.begin(), name.end() };
}
inline void private_output(int fd, const std::filesystem::path &root)
{
	struct stat info = {};
	need(fd > 2 && fstat(fd, &info) == 0 && S_ISREG(info.st_mode) && info.st_uid == geteuid() &&
	     info.st_nlink == 1 && !(info.st_mode & 0077) && info.st_size == 0 &&
	     lseek(fd, 0, SEEK_CUR) == 0);
	const auto flags = fcntl(fd, F_GETFL);
	need(flags >= 0 && (flags & O_ACCMODE) == O_RDWR && !(flags & O_APPEND));
	const auto path = std::filesystem::canonical("/proc/self/fd/" + std::to_string(fd));
	const auto source = std::filesystem::canonical(root);
	need(source != "/" && path != source && !path.string().starts_with(source.string() + "/"));
}
inline auto inventory(const std::filesystem::path &root, int output)
{
	private_output(output, root);
	const auto directory = root / "economic-evidence";
	restore_economic_records::checker reader(root);
	const auto initial = reader.namespace_context();
	const int descriptor =
		open(directory.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	need(descriptor >= 0);
	DIR *stream = fdopendir(descriptor);
	if (!stream)
		close(descriptor);
	need(stream);
	struct owner
	{
		DIR *value;
		~owner() { closedir(value); }
	} held{ stream };
	std::vector<chunk> chunks;
	bytes names;
	size_t count = 0, total = 0;
	uint64_t offset = 0;
	auto publish = [&]
	{
		if (!count)
			return;
		bytes block;
		put(block, count, 2);
		block.insert(block.end(), names.begin(), names.end());
		need(block.size() <= inventory_limit - offset);
		for (size_t written = 0; written < block.size();)
		{
			audit_checkpoint();
			const auto result =
				write(output, block.data() + written, block.size() - written);
			if (result < 0 && errno == EINTR)
				continue;
			need(result > 0);
			written += static_cast<size_t>(result);
		}
		chunks.push_back({ offset, block.size(), count, hash(block) });
		offset += block.size();
		names.clear();
		count = 0;
	};
	while (true)
	{
		audit_checkpoint();
		errno = 0;
		const auto entry = readdir(stream);
		if (!entry)
		{
			need(errno == 0);
			break;
		}
		audit_directory_entry();
		const std::string name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		need(!name.empty() && name.size() <= 255 && total < entry_limit);
		if (current_audit_budget)
		{
			need(name.size() <= current_audit_budget->remaining_bytes);
			current_audit_budget->remaining_bytes -= name.size();
		}
		put(names, name.size(), 2);
		const auto encoded = name_bytes(name);
		names.insert(names.end(), encoded.begin(), encoded.end());
		++total;
		if (++count == names_per_chunk)
			publish();
	}
	publish();
	need(reader.namespace_context().cut == initial.cut);
	return std::tuple{ initial, total, chunks };
}
template <typename Value> inline Value decimal(const std::string &text)
{
	Value value = {};
	const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
	need(!text.empty() && result.ec == std::errc{} && result.ptr == text.data() + text.size() &&
	     std::to_string(value) == text);
	return value;
}
inline bytes decode(const std::string &text, size_t maximum)
{
	need(!text.empty() && text.size() % 2 == 0 && text.size() <= maximum * 2);
	bytes result(text.size() / 2);
	for (size_t index = 0; index < text.size(); ++index)
	{
		const auto value = text[index];
		need((value >= '0' && value <= '9') || (value >= 'a' && value <= 'f'));
		result[index / 2] = static_cast<uint8_t>(
			(result[index / 2] << 4) | (value <= '9' ? value - '0' : value - 'a' + 10));
	}
	return result;
}
inline bool command(int argc, char **argv)
{
	if (argc < 2)
		return false;
	const std::string operation = argv[1];
	if (operation != "--economic-namespace-inventory" &&
	    operation != "--economic-namespace-context" && operation != "--economic-namespace-file")
		return false;
	need((argc == 4 && operation == "--economic-namespace-inventory") ||
	     (argc == 4 && operation == "--economic-namespace-context") ||
	     (argc == 5 && operation == "--economic-namespace-file"));
	const std::filesystem::path root = argv[2];
	need(root.is_absolute());
	audit_budget budget;
	if (operation == "--economic-namespace-inventory")
		budget.remaining_entries = entry_limit + 2;
	scoped_audit_budget scope(budget);
	authority_read_lock lock(root);
	if (!lock.locked() || !std::filesystem::exists(root / "economic-evidence") ||
	    std::filesystem::is_empty(root / "economic-evidence"))
	{
		lock.finish();
		audit_checkpoint();
		std::cout << "{\"initialized\":false}\n";
		return true;
	}
	using restore_economic_baseline::hex;
	if (operation == "--economic-namespace-inventory")
	{
		const auto [context, total, chunks] = inventory(root, decimal<int>(argv[3]));
		lock.finish();
		audit_checkpoint();
		std::cout
			<< "{\"initialized\":true,\"format\":\"flatfile_economic_namespace_inventory_v1\","
			<< "\"lineage\":\"" << hex(context.lineage) << "\",\"source_cut_sha256\":\""
			<< hex(context.cut)
			<< "\",\"legacy_unknown_epochs\":" << context.legacy_unknown_epochs
			<< ",\"entries\":" << total << ",\"chunks\":[";
		for (size_t index = 0; index < chunks.size(); ++index)
		{
			const auto &value = chunks[index];
			std::cout << (index ? "," : "") << "{\"offset\":" << value.offset
				  << ",\"bytes\":" << value.size << ",\"entries\":" << value.count
				  << ",\"sha256\":\"" << hex(value.checksum) << "\"}";
		}
		std::cout << "]}\n";
		return true;
	}
	const auto decoded = decode(argv[3], 32);
	need(decoded.size() == 32);
	digest expected;
	std::copy(decoded.begin(), decoded.end(), expected.begin());
	restore_economic_records::checker reader(root);
	const auto context = reader.history_context();
	need(reader.namespace_binding(context.cut) == expected);
	if (operation == "--economic-namespace-context")
	{
		lock.finish();
		audit_checkpoint();
		std::cout << "{\"initialized\":true,\"lineage\":\"" << hex(context.lineage)
			  << "\",\"source_cut_sha256\":\"" << hex(expected) << "\"}\n";
		return true;
	}
	const auto encoded = decode(argv[4], 255);
	const std::string name(encoded.begin(), encoded.end());
	bool valid = false, refused = false;
	std::string family = "invalid";
	try
	{
		family = reader.namespace_file(name, context);
		audit_checkpoint();
		valid = true;
	}
	catch (const audit_budget_refused &)
	{
		refused = true;
	}
	catch (const std::runtime_error &)
	{
	}
	// Cooperating writers remain excluded. A refused file gets no positive
	// verification and no further physical reads; do not reset its quotas.
	need(reader.namespace_binding(context.cut) == expected);
	lock.finish();
	if (!refused)
		audit_checkpoint();
	std::cout << "{\"initialized\":true,\"lineage\":\"" << hex(context.lineage)
		  << "\",\"source_cut_sha256\":\"" << hex(expected) << "\",\"name_sha256\":\""
		  << hex(hash(encoded)) << "\",\"family\":\"" << family
		  << "\",\"valid\":" << (valid ? "true" : "false")
		  << ",\"file_refused\":" << (refused ? "true" : "false") << "}\n";
	return true;
}
} // namespace restore_economic_namespace
#endif
