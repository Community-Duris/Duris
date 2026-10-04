#ifndef DURIS_FLATFILE_ACCOUNTING_STAGING_VIEW_H
#define DURIS_FLATFILE_ACCOUNTING_STAGING_VIEW_H

#include "flatfile/flatfile_accounting_authority.h"
#include "economy/economic_baseline_adapter.h"
#include <algorithm>
#include <cerrno>
#include <map>
#include <openssl/sha.h>
#include <set>

// Private composition capability. Public readers never receive this view.
// Only lifecycle/baseline participants can construct or fork it; no disk writes.
class flatfile_accounting_staging_view
{
	friend class flatfile_accounting_lifecycle_transaction;
	friend class flatfile_accounting_baseline_storage;
	using operations = std::vector<flatfile_authority_operation>;
	const std::string &root_;
	const flatfile_authority_lock &lock_;
	operations &operations_;
	uint64_t generation_ = 0;
	using manifest = std::map<std::string, std::pair<size_t, economic_digest>>;
	mutable manifest expected_;
	manifest fork_origin_;
	const flatfile_accounting_staging_view *parent_ = nullptr;
	bool trusted_ = false;
	mutable std::map<std::string, std::pair<uint64_t, economic_digest>> read_witnesses_;

	flatfile_accounting_staging_view(const std::string &root,
					 const flatfile_authority_lock &lock, operations &ops)
		: root_(root)
		, lock_(lock)
		, operations_(ops)
		, trusted_(ops.empty())
	{
	}
	flatfile_accounting_staging_view(const flatfile_accounting_staging_view &parent,
					 operations &ops)
		: root_(parent.root_)
		, lock_(parent.lock_)
		, operations_(ops)
		, generation_(parent.generation_)
		, expected_(parent.expected_)
		, fork_origin_(parent.expected_)
		, parent_(&parent)
		, trusted_(!parent.consistent())
		, read_witnesses_(parent.read_witnesses_)
	{
	}

	static economic_digest digest(const std::vector<uint8_t> &bytes)
	{
		economic_digest value = {};
		SHA256(bytes.data(), bytes.size(), value.data());
		return value;
	}
	unsigned int consistent() const
	{
		if (!trusted_ || operations_.size() != expected_.size())
			return ESTALE;
		for (size_t index = 0; index < operations_.size(); ++index)
		{
			const auto &operation = operations_[index];
			const auto at = expected_.find(operation.filename);
			if (operation.store != flatfile_authority_store::economic_evidence ||
			    operation.kind != flatfile_authority_operation_kind::write ||
			    at == expected_.end() || at->second.first != index ||
			    at->second.second != digest(operation.bytes))
				return ESTALE;
		}
		return 0;
	}
	static unsigned int validate(const operations &ops)
	{
		if (ops.size() > flatfile_authority_transaction_maximum_operations)
			return ENOSPC;
		size_t total = 50;
		std::set<std::string> destinations;
		for (const auto &operation : ops)
		{
			if (operation.store != flatfile_authority_store::economic_evidence ||
			    operation.kind != flatfile_authority_operation_kind::write ||
			    operation.filename.empty() || operation.filename.size() > 192 ||
			    operation.filename == "." || operation.filename == ".." ||
			    !std::all_of(operation.filename.begin(), operation.filename.end(),
					 [](char c) {
						 return (c >= 'a' && c <= 'z') ||
							(c >= '0' && c <= '9') || c == '-' ||
							c == '_' || c == '.';
					 }) ||
			    !destinations.insert(operation.filename).second)
				return EINVAL;
			const auto &bytes = operation.bytes;
			if (bytes.size() < 48 ||
			    bytes.size() > flatfile_authority_transaction_maximum_bytes)
				return EILSEQ;
			if (operation.filename.ends_with(".eab"))
			{
				std::optional<economic_prepared_baseline> prepared;
				const auto code = economic_baseline_decode(bytes, &prepared);
				if (code != economic_accounting_error::ok || !prepared)
					return code == economic_accounting_error::capacity ?
						       ENOSPC :
						       EILSEQ;
			}
			else
			{
				uint32_t body_size = 0;
				for (size_t index = 0; index < 4; ++index)
					body_size |= uint32_t(bytes[12 + index]) << (index * 8);
				if (body_size != bytes.size() - 48)
					return EILSEQ;
				economic_digest body_digest = {};
				SHA256(bytes.data() + 48, bytes.size() - 48, body_digest.data());
				if (!std::equal(body_digest.begin(), body_digest.end(),
						bytes.begin() + 16))
					return EILSEQ;
			}
			const size_t extra = 8 + operation.filename.size() + bytes.size();
			if (extra > flatfile_authority_transaction_maximum_bytes - total)
				return ENOSPC;
			total += extra;
		}
		return 0;
	}

    public:
	flatfile_accounting_staging_view(const flatfile_accounting_staging_view &) = delete;
	flatfile_accounting_staging_view &
	operator=(const flatfile_accounting_staging_view &) = delete;
	bool matches(const std::string &root, const flatfile_authority_lock &lock,
		     const operations *ops = nullptr) const
	{
		return root == root_ && &lock == &lock_ && lock_.matches(root_) &&
		       (!ops || ops == &operations_);
	}
	unsigned int begin(const std::string &root, const flatfile_authority_lock &lock,
			   operations *ops)
	{
		if (!matches(root, lock, ops) || !ops || generation_ == UINT64_MAX)
			return EINVAL;
		if (const auto code = consistent())
			return code;
		const auto code = validate(operations_);
		if (code)
			return code;
		++generation_;
		read_witnesses_.clear();
		return 0;
	}
	unsigned int read(const std::string &name, size_t maximum, std::vector<uint8_t> *bytes,
			  bool *found) const
	{
		if (!bytes || !found || !lock_.matches(root_))
			return EINVAL;
		if (const auto code = consistent())
			return code;
		*found = false;
		for (const auto &operation : operations_)
			if (operation.filename == name)
			{
				if (operation.bytes.size() > maximum)
					return ENOSPC;
				read_witnesses_[name] = { generation_, digest(operation.bytes) };
				*bytes = operation.bytes;
				*found = true;
				break;
			}
		return 0;
	}
	// Replace only a predecessor read in this participant generation. Existing
	// codecs must validate its lineage/revision/body before constructing updates.
	unsigned int merge(const operations &updates, operations *out) const
	{
		if (!matches(root_, lock_, out) || !out)
			return EINVAL;
		if (const auto code = consistent())
			return code;
		const auto input_code = validate(*out);
		if (input_code)
			return input_code;
		const auto update_code = validate(updates);
		if (update_code)
			return update_code;
		auto candidate = *out;
		for (const auto &update : updates)
		{
			auto at = std::find_if(candidate.begin(), candidate.end(),
					       [&](const auto &operation)
					       { return operation.filename == update.filename; });
			if (at == candidate.end())
				candidate.push_back(update);
			else
			{
				const auto witness = read_witnesses_.find(update.filename);
				if (witness == read_witnesses_.end() ||
				    witness->second.first != generation_ ||
				    witness->second.second != digest(at->bytes))
					return ESTALE;
				*at = update;
			}
		}
		const auto code = validate(candidate);
		if (code)
			return code;
		manifest next;
		for (size_t index = 0; index < candidate.size(); ++index)
			next.emplace(candidate[index].filename,
				     std::make_pair(index, digest(candidate[index].bytes)));
		out->swap(candidate);
		expected_.swap(next);
		return 0;
	}
	// Publish a fully prepared private fork without allocation after changing
	// the caller's vector. Raw vector changes never advance provenance.
	unsigned int adopt(flatfile_accounting_staging_view &candidate)
	{
		if (candidate.parent_ != this || !candidate.matches(root_, lock_) ||
		    candidate.fork_origin_ != expected_)
			return ESTALE;
		if (const auto code = consistent())
			return code;
		if (const auto code = candidate.consistent())
			return code;
		operations_.swap(candidate.operations_);
		expected_.swap(candidate.expected_);
		read_witnesses_.swap(candidate.read_witnesses_);
		generation_ = candidate.generation_;
		return 0;
	}
	unsigned int control(flatfile_economic_control *, std::string *error = nullptr) const;
	unsigned int epoch(const critical_operation_id &lineage, const critical_operation_id &epoch,
			   flatfile_economic_epoch *, std::string *error = nullptr) const;
};
#endif
