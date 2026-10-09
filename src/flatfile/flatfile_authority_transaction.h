#ifndef DURIS_FLATFILE_AUTHORITY_TRANSACTION_H
#define DURIS_FLATFILE_AUTHORITY_TRANSACTION_H

#include "flatfile/flatfile_store.h"
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
	// Fresh explicit C++ state admission BEFORE the original nothrow allocation.
	// Unsupported storage policy or refusal leaves an empty, unacquirable lock.
	// Caller includes already-live values in outer_live_scratch and holds the
	// callback reservation through lock destruction; no lock is taken here.
	flatfile_authority_lock(flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
				size_t outer_live_scratch) noexcept;
	// Distinct allocation-admitted acquisition with original process/filesystem
	// exclusion and private-directory/file checks. outer_live_scratch includes
	// this actual lock/state/root retention; failures never become owned locks.
	// Caller includes its root/path inputs and retains peak through nested reads.
	bool acquire_bounded(const std::string &root,
			     flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
			     size_t outer_live_scratch) noexcept;
	// Actual explicit C++ lock/state/root storage under pinned libstdc++13.
	// Scalar only; strong output. Never proves root/source/admission authority.
	bool retained_bytes(size_t *output) const noexcept;
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

// DISTINCT bounded recovery admits explicit C++ storage requests under the
// libstdc++13 capacity policy before decoded materialization or first apply.
// The allocation-free wire scan sizes untrusted shape; original decode then
// authenticates the digest after complete admission and before any apply.
// OpenSSL/system internal allocations remain outside this measured scope.
// Budget refusal before apply preserves journal/disk. Partial apply failure
// before final removal leaves the pending journal; final unlink/sync failure
// has uncertain removal and the journal may already be absent. Same borrowed
// root lock and original ordering. Caller retains peak through return and
// restores its prior aggregate. No diagnostic string allocations.
flatfile_authority_transaction_result
flatfile_authority_transaction_recover_bounded(const std::string &root,
					       const flatfile_authority_lock &lock,
					       flatfile_scratch_reserve_fn reserve_scratch_peak,
					       void *context, size_t outer_live_scratch) noexcept;

// Distinct prospective commit with the original pending-journal refusal, full
// canonical journal encoding, publication/apply/fault/unlink order and outcome.
// Public callers retain the original economic_evidence prohibition; only the
// existing private typed storage boundary can include that entitled store.
// Null diagnostics avoid allocating error strings. Caller includes the input
// root/lock/operations and their capacities, output/context and already-live
// storage in outer, and holds the admitted maximum through return. All apply
// path/atomic storage is admitted BEFORE journal publication; later failure
// preserves committed/uncertain outcome exactly as the original algorithm.
// ENOBUFS refusal/overflow, ENOMEM allocation, ENOTSUP pinned request policy.
flatfile_authority_transaction_result
flatfile_authority_transaction_commit_operations_with_outcome_bounded(
	const std::string &, const flatfile_authority_lock &,
	const std::vector<flatfile_authority_operation> &, flatfile_authority_commit_outcome *,
	flatfile_scratch_reserve_fn, void *, size_t outer_live_scratch) noexcept;

#endif
