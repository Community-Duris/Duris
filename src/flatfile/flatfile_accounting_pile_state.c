#include "flatfile/flatfile_accounting_pile_state.h"
#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cerrno>
#include <charconv>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <new>
#include <openssl/sha.h>

namespace
{
constexpr size_t head_bytes = 4 + 16 + 16 + 8 + 8 + 32 + 1 + 16 + SHA256_DIGEST_LENGTH;

std::string filename(uint64_t uid)
{
	char name[32] = {};
	std::snprintf(name, sizeof(name), "pile-head-%016llx.eph",
		      static_cast<unsigned long long>(uid));
	return name;
}

void number(std::vector<uint8_t> &bytes, uint64_t value)
{
	for (size_t index = 0; index < 8; ++index)
		bytes.push_back(static_cast<uint8_t>(value >> (8 * index)));
}

uint64_t number(std::span<const uint8_t> bytes, size_t offset)
{
	uint64_t value = 0;
	for (size_t index = 0; index < 8; ++index)
		value |= uint64_t(bytes[offset + index]) << (8 * index);
	return value;
}

bool valid_balance(const economic_coin_vector &balance)
{
	return std::all_of(balance.begin(), balance.end(),
			   [](int64_t value) { return value >= 0 && value <= INT32_MAX; });
}

bool valid(const flatfile_accounting_pile_state &state)
{
	return economic_account_key_valid(state.account) &&
	       state.account.kind == economic_account_kind::pile && state.account.context_id == 0 &&
	       state.item_revision > 0 && !critical_operation_id_is_zero(state.epoch) &&
	       !critical_operation_id_is_zero(state.operation_id) && valid_balance(state.balance) &&
	       (!state.retired || state.balance == economic_coin_vector{});
}

std::vector<uint8_t> encode(const flatfile_accounting_pile_state &state)
{
	std::vector<uint8_t> bytes{ 'E', 'P', 'H', '1' };
	bytes.insert(bytes.end(), state.account.lineage.bytes.begin(),
		     state.account.lineage.bytes.end());
	bytes.insert(bytes.end(), state.epoch.bytes.begin(), state.epoch.bytes.end());
	number(bytes, state.account.authority_id);
	number(bytes, state.item_revision);
	for (auto coin : state.balance)
		number(bytes, static_cast<uint64_t>(coin));
	bytes.push_back(state.retired ? 1 : 0);
	bytes.insert(bytes.end(), state.operation_id.bytes.begin(), state.operation_id.bytes.end());
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	bytes.insert(bytes.end(), digest.begin(), digest.end());
	return bytes;
}

bool decode(std::span<const uint8_t> bytes, uint64_t uid, flatfile_accounting_pile_state *state)
{
	if (bytes.size() != head_bytes || !std::equal(bytes.begin(), bytes.begin() + 4, "EPH1") ||
	    bytes[84] > 1)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size() - digest.size(), digest.data());
	if (!std::equal(digest.begin(), digest.end(), bytes.end() - digest.size()))
		return false;
	flatfile_accounting_pile_state value;
	std::copy_n(bytes.begin() + 4, 16, value.account.lineage.bytes.begin());
	value.account.kind = economic_account_kind::pile;
	std::copy_n(bytes.begin() + 20, 16, value.epoch.bytes.begin());
	value.account.authority_id = number(bytes, 36);
	value.item_revision = number(bytes, 44);
	for (size_t index = 0; index < 4; ++index)
		value.balance[index] = static_cast<int64_t>(number(bytes, 52 + index * 8));
	value.retired = bytes[84] == 1;
	std::copy_n(bytes.begin() + 85, 16, value.operation_id.bytes.begin());
	if (value.account.authority_id != uid || !valid(value))
		return false;
	*state = value;
	return true;
}

flatfile_accounting_status read(const std::string &root, uint64_t uid,
				flatfile_accounting_pile_state *state, std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto result = flatfile_read(root + "/economic-evidence", filename(uid), head_bytes,
					  &bytes, error);
	if (result == flatfile_read_result::not_found)
		return flatfile_accounting_status::not_found;
	if (result == flatfile_read_result::io_error)
		return flatfile_accounting_status::io_error;
	if (result != flatfile_read_result::ok || !decode(bytes, uid, state))
		return flatfile_accounting_status::invalid;
	return flatfile_accounting_status::ok;
}
} // namespace

flatfile_accounting_status
flatfile_accounting_pile_state_read(const std::string &root, const flatfile_authority_lock &lock,
				    uint64_t uid, flatfile_accounting_pile_state *state,
				    std::string *error)
try
{
	if (root.empty() || !lock.matches(root) || !uid || !state)
		return flatfile_accounting_status::invalid;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_accounting_status::io_error :
			       flatfile_accounting_status::invalid;
	flatfile_accounting_pile_state candidate;
	const auto status = read(root, uid, &candidate, error);
	if (status == flatfile_accounting_status::ok)
		*state = candidate;
	return status;
}
catch (const std::bad_alloc &)
{
	return flatfile_accounting_status::io_error;
}

flatfile_accounting_status flatfile_accounting_pile_state_list(
	const std::string &root, const flatfile_authority_lock &lock, size_t maximum,
	std::vector<flatfile_accounting_pile_state> *states, std::string *error)
try
{
	if (root.empty() || !lock.matches(root) || !maximum || !states)
		return flatfile_accounting_status::invalid;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			flatfile_accounting_status::io_error : flatfile_accounting_status::invalid;
	const int fd = open((root + "/economic-evidence").c_str(),
		O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	if (fd < 0)
		return errno == ENOENT ? flatfile_accounting_status::not_found :
			flatfile_accounting_status::io_error;
	struct stat info{};
	if (fstat(fd, &info) || !S_ISDIR(info.st_mode) || info.st_uid != geteuid() ||
		(info.st_mode & 0077))
	{
		close(fd);
		return flatfile_accounting_status::invalid;
	}
	std::unique_ptr<DIR, int (*)(DIR *)> directory(fdopendir(fd), closedir);
	if (!directory)
	{
		close(fd);
		return flatfile_accounting_status::io_error;
	}
	std::vector<flatfile_accounting_pile_state> candidate;
	for (;;)
	{
		errno = 0;
		const auto *entry = readdir(directory.get());
		if (!entry)
		{
			if (errno)
				return flatfile_accounting_status::io_error;
			break;
		}
		if (std::strncmp(entry->d_name, "pile-head-", 10))
			continue;
		const std::string name(entry->d_name);
		uint64_t uid = 0;
		if (name.size() != 30 || name.substr(26) != ".eph")
			return flatfile_accounting_status::invalid;
		const auto parsed = std::from_chars(name.data() + 10, name.data() + 26, uid, 16);
		if (parsed.ec != std::errc{} || parsed.ptr != name.data() + 26 || !uid ||
			filename(uid) != name)
			return flatfile_accounting_status::invalid;
		if (candidate.size() == maximum)
			return flatfile_accounting_status::capacity;
		flatfile_accounting_pile_state state;
		const auto status = read(root, uid, &state, error);
		if (status != flatfile_accounting_status::ok)
			return status == flatfile_accounting_status::not_found ?
				flatfile_accounting_status::invalid : status;
		candidate.push_back(std::move(state));
	}
	std::sort(candidate.begin(), candidate.end(), [](const auto &a, const auto &b)
		{ return a.account.authority_id < b.account.authority_id; });
	*states = std::move(candidate);
	return flatfile_accounting_status::ok;
}
catch (const std::bad_alloc &)
{
	return flatfile_accounting_status::io_error;
}

flatfile_accounting_status flatfile_accounting_pile_state_stage_baseline(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_pile_state &state,
	std::vector<flatfile_authority_operation> *operations, std::string *error)
try
{
	if (root.empty() || !lock.matches(root) || !operations || !valid(state) || state.retired ||
	    operations->size() >= flatfile_authority_transaction_maximum_operations)
		return flatfile_accounting_status::invalid;
	const auto name = filename(state.account.authority_id);
	for (const auto &operation : *operations)
		if (operation.store == flatfile_authority_store::economic_evidence &&
		    operation.filename == name)
			return flatfile_accounting_status::conflict;
	flatfile_accounting_pile_state prior;
	const auto status = flatfile_accounting_pile_state_read(
		root, lock, state.account.authority_id, &prior, error);
	if (status == flatfile_accounting_status::ok)
		return economic_account_key_equal(prior.account, state.account) &&
				       prior.epoch.bytes == state.epoch.bytes &&
				       prior.operation_id.bytes == state.operation_id.bytes &&
				       prior.item_revision == state.item_revision &&
				       prior.balance == state.balance && !prior.retired ?
			       flatfile_accounting_status::already_exists :
			       flatfile_accounting_status::conflict;
	if (status != flatfile_accounting_status::not_found)
		return status;
	auto candidate = *operations;
	candidate.push_back({ flatfile_authority_store::economic_evidence,
			      flatfile_authority_operation_kind::write, name, encode(state) });
	*operations = std::move(candidate);
	return flatfile_accounting_status::ok;
}
catch (const std::bad_alloc &)
{
	return flatfile_accounting_status::io_error;
}

flatfile_accounting_status flatfile_accounting_pile_state_stage(
	const std::string &root, const flatfile_authority_lock &lock,
	const economic_account_effect &effect, const critical_operation_id &epoch,
	const critical_operation_id &operation_id, bool retired,
	std::vector<flatfile_authority_operation> *operations, std::string *error)
try
{
	if (root.empty() || !lock.matches(root) || !operations ||
	    effect.key.kind != economic_account_kind::pile || effect.key.context_id != 0 ||
	    !effect.key.authority_id || !economic_account_key_valid(effect.key) ||
	    critical_operation_id_is_zero(epoch) || critical_operation_id_is_zero(operation_id) ||
	    effect.after_revision <= effect.before_revision || !valid_balance(effect.before) ||
	    !valid_balance(effect.after) || (retired && effect.after != economic_coin_vector{}) ||
	    operations->size() >= flatfile_authority_transaction_maximum_operations)
		return flatfile_accounting_status::invalid;
	const auto name = filename(effect.key.authority_id);
	for (const auto &operation : *operations)
		if (operation.store == flatfile_authority_store::economic_evidence &&
		    operation.filename == name)
			return flatfile_accounting_status::conflict;
	flatfile_accounting_pile_state prior;
	const auto status = flatfile_accounting_pile_state_read(root, lock, effect.key.authority_id,
								&prior, error);
	if (effect.before_revision == 0)
	{
		if (effect.before != economic_coin_vector{} || effect.after_revision != 1)
			return flatfile_accounting_status::invalid;
		if (status == flatfile_accounting_status::ok)
			return flatfile_accounting_status::already_exists;
		if (status != flatfile_accounting_status::not_found)
			return status;
	}
	else
	{
		if (status != flatfile_accounting_status::ok)
			return status;
		if (!economic_account_key_equal(prior.account, effect.key) || prior.retired ||
		    prior.item_revision != effect.before_revision || prior.balance != effect.before)
			return flatfile_accounting_status::conflict;
	}
	flatfile_accounting_pile_state next{
		effect.key, epoch, operation_id, effect.after, effect.after_revision, retired
	};
	if (!valid(next))
		return flatfile_accounting_status::invalid;
	auto candidate = *operations;
	candidate.push_back({ flatfile_authority_store::economic_evidence,
			      flatfile_authority_operation_kind::write, name, encode(next) });
	*operations = std::move(candidate);
	return flatfile_accounting_status::ok;
}
catch (const std::bad_alloc &)
{
	return flatfile_accounting_status::io_error;
}

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
struct pile_bounded_workspace
{
	explicit pile_bounded_workspace(size_t directory_size)
		: directory(directory_size, '\0')
		, name(30, '\0')
	{
	}
	std::string directory, name;
	std::vector<uint8_t> bytes;
	flatfile_accounting_pile_state candidate;
};
bool pile_bounded_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
#endif
}

flatfile_accounting_status flatfile_accounting_pile_state_read_bounded(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t uid,
	flatfile_accounting_pile_state *state, flatfile_scratch_reserve_fn reserve_scratch_peak,
	void *context, size_t outer_live_scratch) noexcept
{
	if (root.empty() || !lock.matches(root) || !uid || !state || !reserve_scratch_peak)
		return flatfile_accounting_status::invalid;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)context;
	(void)outer_live_scratch;
	errno = ENOTSUP;
	return flatfile_accounting_status::io_error;
#else
	// Recovery precedes the reader's paths and candidate, as in the original.
	const auto recovered = flatfile_authority_transaction_recover_bounded(
		root, lock, reserve_scratch_peak, context, outer_live_scratch);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_accounting_status::io_error :
			       flatfile_accounting_status::invalid;
	size_t directory_size = root.size(), live = outer_live_scratch;
	if (!pile_bounded_add(directory_size, sizeof("/economic-evidence") - 1) ||
	    !pile_bounded_add(live, sizeof(pile_bounded_workspace)) ||
	    (directory_size > 15 &&
	     (directory_size == SIZE_MAX || !pile_bounded_add(live, directory_size + 1))) ||
	    !pile_bounded_add(live, 31) || !pile_bounded_add(live, sizeof(char[32])) ||
	    !reserve_scratch_peak(live, context))
	{
		errno = ENOBUFS;
		return flatfile_accounting_status::io_error;
	}
	try
	{
		pile_bounded_workspace work(directory_size);
		std::copy(root.begin(), root.end(), work.directory.begin());
		std::copy_n("/economic-evidence", sizeof("/economic-evidence") - 1,
			    work.directory.begin() + root.size());
		{
			char name[32] = {};
			std::snprintf(name, sizeof(name), "pile-head-%016llx.eph",
				      static_cast<unsigned long long>(uid));
			std::copy_n(name, work.name.size(), work.name.begin());
		}
		// The formatting array dies before secure file metadata inspection.
		live -= sizeof(char[32]);
		const auto loaded = flatfile_read_bounded(work.directory, work.name, head_bytes,
							  &work.bytes, reserve_scratch_peak,
							  context, live);
		if (loaded == flatfile_read_result::not_found)
			return flatfile_accounting_status::not_found;
		if (loaded == flatfile_read_result::io_error)
			return flatfile_accounting_status::io_error;
		if (loaded != flatfile_read_result::ok)
			return flatfile_accounting_status::invalid;
		// Original decode owns its span, digest and value concurrently. Its
		// number() span and retired-zero comparison temporary are sequential.
		if (!pile_bounded_add(live, work.bytes.capacity()) ||
		    !pile_bounded_add(live, sizeof(std::span<const uint8_t>)) ||
		    !pile_bounded_add(live, sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>)) ||
		    !pile_bounded_add(live, sizeof(flatfile_accounting_pile_state)) ||
		    !pile_bounded_add(live, std::max(sizeof(std::span<const uint8_t>),
						     sizeof(economic_coin_vector))) ||
		    !reserve_scratch_peak(live, context))
		{
			errno = ENOBUFS;
			return flatfile_accounting_status::io_error;
		}
		if (!decode(work.bytes, uid, &work.candidate))
			return flatfile_accounting_status::invalid;
		*state = work.candidate;
		return flatfile_accounting_status::ok;
	}
	catch (...)
	{
		return flatfile_accounting_status::io_error;
	}
#endif
}
