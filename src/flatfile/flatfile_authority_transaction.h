#ifndef DURIS_FLATFILE_AUTHORITY_TRANSACTION_H
#define DURIS_FLATFILE_AUTHORITY_TRANSACTION_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/*
 * Maximum after-images (or operations) a single authority transaction may carry.
 * Callers that build a transaction from a fixed domain contract must assert their
 * own maximum against this value at compile time.
 */
// One player save can carry 4096 operation receipts, its snapshot, a death
// disposition, and a custody after-image. The total byte limit still applies.
constexpr size_t flatfile_authority_transaction_maximum_operations = 4099;
constexpr size_t flatfile_authority_transaction_maximum_bytes = 256 * 1024 * 1024;

struct flatfile_authority_after_image
{
	std::string filename;
	std::vector<uint8_t> bytes;
};

enum class flatfile_authority_store : uint8_t
{
	domains = 1,
	players = 2,
	identities = 3,
	accounts = 4,
	metadata = 5,
	player_deaths = 6,
	economic_evidence = 7,
	item_accounting_references = 8
};

enum class flatfile_authority_operation_kind : uint8_t
{
	write = 1,
	remove = 2
};

struct flatfile_authority_operation
{
	flatfile_authority_store store = flatfile_authority_store::domains;
	flatfile_authority_operation_kind kind = flatfile_authority_operation_kind::write;
	std::string filename;
	std::vector<uint8_t> bytes;
};

enum class flatfile_authority_transaction_result
{
	ok,
	not_found,
	invalid,
	io_error
};

// A failed apply does not undo publication of the authority journal. A rename
// with failed directory sync is uncertain until native recovery finishes.
enum class flatfile_authority_commit_outcome
{
	not_published,
	publication_uncertain,
	committed
};

class flatfile_authority_lock
{
    public:
	flatfile_authority_lock() noexcept;
	~flatfile_authority_lock();
	flatfile_authority_lock(const flatfile_authority_lock &) = delete;
	flatfile_authority_lock &operator=(const flatfile_authority_lock &) = delete;

	bool acquire(const std::string &root, std::string *error);
	bool matches(const std::string &root) const;

    private:
	struct state;
	std::unique_ptr<state> state_;
	bool owns(const std::string &root) const;
	friend class flatfile_accounting_storage;
	friend flatfile_authority_transaction_result
	flatfile_authority_transaction_recover(const std::string &, const flatfile_authority_lock &,
					       std::string *);
	friend flatfile_authority_transaction_result
	flatfile_authority_transaction_commit(const std::string &, const flatfile_authority_lock &,
					      const std::vector<flatfile_authority_after_image> &,
					      std::string *);
	friend flatfile_authority_transaction_result
	flatfile_authority_transaction_commit_operations(
		const std::string &, const flatfile_authority_lock &,
		const std::vector<flatfile_authority_operation> &, std::string *);
};

flatfile_authority_transaction_result
flatfile_authority_transaction_recover(const std::string &root, const flatfile_authority_lock &lock,
				       std::string *error);
flatfile_authority_transaction_result
flatfile_authority_transaction_commit(const std::string &root, const flatfile_authority_lock &lock,
				      const std::vector<flatfile_authority_after_image> &images,
				      std::string *error);
flatfile_authority_transaction_result flatfile_authority_transaction_commit_operations(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<flatfile_authority_operation> &operations, std::string *error);
flatfile_authority_transaction_result flatfile_authority_transaction_commit_operations_with_outcome(
	const std::string &root, const flatfile_authority_lock &lock,
	const std::vector<flatfile_authority_operation> &operations, std::string *error,
	flatfile_authority_commit_outcome *outcome);

#endif
