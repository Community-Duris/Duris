#ifndef DURIS_ECONOMIC_SQL_LIFECYCLE_GUARD_H
#define DURIS_ECONOMIC_SQL_LIFECYCLE_GUARD_H

#include "persistence/economic_sql_lifecycle_lock_names.h"
#include <mysql/mysql.h>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <thread>

class economic_sql_lifecycle_guard;
class economic_sql_cutover_transaction_owner;
struct critical_operation_id;
struct economic_sql_activation_receipt;

// Opaque owner-issued proof of the composed SQL fence and coordinator lease.
// It stores identities only, never a pointer to a guard or SQL connection.
class economic_sql_cutover_capability final
{
    public:
	economic_sql_cutover_capability() = default;
	economic_sql_cutover_capability(const economic_sql_cutover_capability &) = delete;
	economic_sql_cutover_capability &
	operator=(const economic_sql_cutover_capability &) = delete;
	economic_sql_cutover_capability(economic_sql_cutover_capability &&) = delete;
	economic_sql_cutover_capability &operator=(economic_sql_cutover_capability &&) = delete;
	// This checks the idle acquisition authority only. Once the transaction
	// owner starts, validate through that exact-session owner; failed validation
	// never releases a lease whose transaction outcome may be unresolved.
	bool is_valid_for(const economic_sql_lifecycle_guard &) const noexcept;

    private:
	friend class economic_sql_lifecycle_guard;
	friend class economic_sql_cutover_transaction_owner;
	uint64_t sql_authority_id_ = 0;
	unsigned long sql_session_ = 0;
	uint64_t coordinator_generation_ = 0;
	uint64_t coordinator_lease_id_ = 0;
};

// Shared runtime admission + currency-writer fence. The runtime guard is held
// on a dedicated reconnect-disabled SQL control connection for the whole game
// process lifetime. Maintenance can acquire its exclusive named lock only while
// every runtime has released that lock. Do not replace either lock with a bool,
// digest, or caller assertion.
class economic_sql_lifecycle_guard
{
    public:
	economic_sql_lifecycle_guard() = default;
	economic_sql_lifecycle_guard(const economic_sql_lifecycle_guard &) = delete;
	economic_sql_lifecycle_guard &operator=(const economic_sql_lifecycle_guard &) = delete;
	~economic_sql_lifecycle_guard();

	// Acquire at process boot before admitting gameplay; retain until shutdown.
	// A failed factory can leave cleanup obligations in the supplied guard.
	// Keep that guard and original handle alive until release() succeeds.
	static unsigned int acquire_runtime(MYSQL *control_connection,
					    economic_sql_lifecycle_guard *) noexcept;
	// Acquire only from the quiesced maintenance process, before source capture.
	// It owns both the boot/runtime gate and the legacy currency writer fence.
	static unsigned int acquire_maintenance(MYSQL *control_connection,
						economic_sql_lifecycle_guard *) noexcept;
	bool is_maintenance_authority() const noexcept;
	// A failed attempt leaves output untouched. Once coordinator acquisition
	// starts, failure leaves admission quiesced; invalid guard/output arguments
	// are rejected before touching coordinator state. Begin a transaction only
	// through economic_sql_cutover_transaction_owner on this same thread/session.
	bool acquire_cutover_capability(uint64_t drain_timeout_msec,
					economic_sql_cutover_capability *output) noexcept;
	bool is_valid_authority() const noexcept;
	// Acquisition failure can retain cleanup obligations without granting authority.
	// Retry release while the original idle, non-reconnecting handle stays alive.
	// False retains exclusion; destruction cannot certify SQL cleanup.
	bool release() noexcept;

    private:
	friend class economic_sql_cutover_capability;
	friend class economic_sql_cutover_transaction_owner;
	friend class economic_sql_accounting_lifecycle_transaction;
	friend class economic_sql_currency_writer_guard;
	friend class economic_sql_runtime_world_writer_guard;
	// Exact data-only readback needs to prove supplied-handle ownership without
	// exposing the guard's connection/session through a general accessor.
	friend unsigned int
	economic_sql_activation_receipt_readback(MYSQL *, const economic_sql_lifecycle_guard &,
						 const critical_operation_id &,
						 economic_sql_activation_receipt *) noexcept;
	static void clear_transferred_local_authority(bool runtime, bool maintenance) noexcept;
	bool is_valid_cutover_capability(const economic_sql_cutover_capability &) const noexcept;
	static bool release_named_lock(MYSQL *, unsigned long, const char *, bool *) noexcept;
	MYSQL *connection_ = nullptr;
	unsigned long session_ = 0;
	std::thread::id owner_thread_;
	bool runtime_lock_ = false;
	bool writer_lock_ = false;
	bool acquisition_confirmed_ = false;
	bool runtime_release_attempted_ = false;
	bool writer_release_attempted_ = false;
	bool maintenance_ = false;
	bool local_runtime_ = false;
	bool local_maintenance_ = false;
	// Bound privately by the composed owner. Ordinary SQL guard consumers do
	// not acquire a coordinator dependency merely by releasing a SQL fence.
	bool (*coordinator_release_)(uint64_t, uint64_t) = nullptr;
	uint64_t authority_id_ = 0;
	uint64_t coordinator_generation_ = 0;
	uint64_t coordinator_lease_id_ = 0;
	std::unique_lock<std::shared_mutex> local_exclusive_;
};

// One-shot owner for the exact MYSQL session and coordinator lease. begin()
// transfers the SQL/coordinator resources before issuing START TRANSACTION.
// COMMIT/ROLLBACK is permitted only through this object. A failed live-session
// validation or ambiguous terminal result retains exclusion; rollback of an
// unresolved outcome may be retried only on the same non-reconnecting session.
// Destroying an unresolved owner deliberately leaks the local exclusion and
// never reopens admission.
// Keep the MYSQL handle alive and thread-confined through terminal success;
// use only that session for transaction statements and never commit it elsewhere.
// An outcome is known only after the exact-session terminal SQL succeeds.
enum class economic_sql_cutover_terminal_outcome : uint8_t
{
	unresolved,
	committed,
	rolled_back,
};

class economic_sql_cutover_transaction_owner final
{
    public:
	economic_sql_cutover_transaction_owner() = default;
	economic_sql_cutover_transaction_owner(const economic_sql_cutover_transaction_owner &) =
		delete;
	economic_sql_cutover_transaction_owner &
	operator=(const economic_sql_cutover_transaction_owner &) = delete;
	economic_sql_cutover_transaction_owner(economic_sql_cutover_transaction_owner &&) = delete;
	economic_sql_cutover_transaction_owner &
	operator=(economic_sql_cutover_transaction_owner &&) = delete;
	~economic_sql_cutover_transaction_owner();

	// begin() transfers the exact guard/capability resources before START TRANSACTION.
	// A false result with outcome_uncertain()==true still owns the exclusion and
	// requires this object and connection to remain alive; only same-session
	// rollback/recovery is allowed. commit()/rollback() return true only after the
	// matching SQL terminal result, SQL/local fence release, and coordinator owner
	// release. A resolved opposite outcome is rejected without SQL or cleanup.
	// retry_cleanup() issues only fence cleanup SQL, never COMMIT or ROLLBACK,
	// and only releases fences after a known outcome. The opt-in retained-publication
	// path commits once but keeps every fence until finish_publication() releases
	// the coordinator lease and transfers the maintenance SQL/local fences into
	// an empty lifetime guard without unlocking them.
	bool begin(economic_sql_lifecycle_guard &,
		   const economic_sql_cutover_capability &) noexcept;
	bool is_valid() noexcept;
	// Opt in only for a maintenance owner: a successful COMMIT records a known
	// durable outcome while retaining coordinator, SQL, and local exclusion.
	// A false return after terminal_outcome()==committed leaves publication pending.
	bool commit_and_retain_publication() noexcept;
	bool is_valid_for_publication() noexcept;
	// Call only after private publication. Release the coordinator while this
	// owner still holds the SQL/local fences, then move those same fences into
	// an empty lifetime guard without an unlock/reacquire gap.
	bool finish_publication(economic_sql_lifecycle_guard *lifetime_guard) noexcept;
	bool publication_pending() const noexcept { return publication_pending_; }
	bool commit() noexcept;
	bool rollback() noexcept;
	bool retry_cleanup() noexcept;
	bool outcome_uncertain() const noexcept { return outcome_uncertain_; }
	economic_sql_cutover_terminal_outcome terminal_outcome() const noexcept
	{
		return terminal_outcome_;
	}
	bool terminal() const noexcept { return terminal_; }

    private:
	friend class economic_sql_accounting_lifecycle_transaction;
	bool release_after_terminal() noexcept;
	MYSQL *connection_ = nullptr;
	unsigned long session_ = 0;
	std::thread::id owner_thread_;
	uint64_t sql_authority_id_ = 0;
	uint64_t coordinator_generation_ = 0;
	uint64_t coordinator_lease_id_ = 0;
	bool runtime_lock_ = false;
	bool writer_lock_ = false;
	bool runtime_release_attempted_ = false;
	bool writer_release_attempted_ = false;
	bool maintenance_ = false;
	bool local_runtime_ = false;
	bool local_maintenance_ = false;
	bool active_ = false;
	bool started_ = false;
	bool outcome_uncertain_ = false;
	bool publication_pending_ = false;
	economic_sql_cutover_terminal_outcome terminal_outcome_ =
		economic_sql_cutover_terminal_outcome::unresolved;
	bool sql_resources_released_ = false;
	bool terminal_ = false;
	std::unique_lock<std::shared_mutex> local_exclusive_;
};

// Legacy SQL currency and item writers must hold this guard from before their
// transaction starts through COMMIT/ROLLBACK. A staged installation or active
// epoch refuses the old writer path. Missing lifecycle schema/errors fail closed.
// The caller owns the MYSQL connection and must not reconnect it. Integration of
// this guard at one dispatcher does not establish source-complete writer coverage.
class economic_sql_currency_writer_guard
{
    public:
	economic_sql_currency_writer_guard() = default;
	economic_sql_currency_writer_guard(const economic_sql_currency_writer_guard &) = delete;
	economic_sql_currency_writer_guard &
	operator=(const economic_sql_currency_writer_guard &) = delete;
	~economic_sql_currency_writer_guard();

	static unsigned int acquire(MYSQL *connection,
				    economic_sql_currency_writer_guard *) noexcept;
	// Validate this exact held lease inside its transaction as well as before it.
	// This does not acquire authority or accept a caller assertion of admission.
	bool is_valid_for(MYSQL *connection) const noexcept;
	// False requires connection-owner retirement/retained recovery. No transaction
	// or borrowed named lock is ended on the caller's behalf.
	bool release() noexcept;
	// Exact pool-owned retirement, not a caller assertion. Consumes only a
	// currently borrowed pooled handle; a foreign/direct DB handle is untouched.
	// Call after transaction cleanup and never access the consumed pointer again.
	bool retire_pooled_session() noexcept;

    private:
	friend class player_sql_pool_lease;
	std::shared_lock<std::shared_mutex> local_shared_;
	MYSQL *connection_ = nullptr;
	unsigned long session_ = 0;
	std::thread::id owner_thread_;
	bool writer_lock_ = false;
	bool acquisition_confirmed_ = false;
	bool release_attempted_ = false;
};

// Boot-only whole native-world read/construction exclusion. The actual runtime
// guard remains unchanged; this separate owner holds the genuine writer named
// lock and local gate only AFTER accepted journal work has genuinely drained.
// Only the real boot owner can acquire, validate or release this lease.
class economic_sql_runtime_world_writer_guard final
{
	friend class sql_economic_runtime_boot_owner;
	economic_sql_runtime_world_writer_guard() noexcept = default;
	~economic_sql_runtime_world_writer_guard() noexcept;
	economic_sql_runtime_world_writer_guard(const economic_sql_runtime_world_writer_guard &) =
		delete;
	economic_sql_runtime_world_writer_guard &
	operator=(const economic_sql_runtime_world_writer_guard &) = delete;
	static unsigned int acquire(MYSQL *, const economic_sql_lifecycle_guard &,
				    economic_sql_runtime_world_writer_guard *) noexcept;
	bool valid() const noexcept;
	bool release() noexcept;
	MYSQL *connection_ = nullptr;
	const economic_sql_lifecycle_guard *runtime_ = nullptr;
	unsigned long session_ = 0;
	std::thread::id thread_{};
	std::unique_lock<std::shared_mutex> local_;
	bool lock_ = false, confirmed_ = false, release_attempted_ = false;
};

#endif
