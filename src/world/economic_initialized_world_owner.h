#ifndef ECONOMIC_INITIALIZED_WORLD_OWNER_H
#define ECONOMIC_INITIALIZED_WORLD_OWNER_H

#include "world/economic_initialized_world_snapshot.h"
#include "persistence/economic_sql_initialized_activation_view.h"

#include <cstdint>
class sql_economic_runtime_boot_owner;
class economic_sql_cutover_transaction_owner;
class economic_sql_lifecycle_guard;
class economic_sql_runtime_world_writer_guard;

// Actual game_loop successful-initialization owner only. No caller may replace
// boot/copyover success, source completeness or held exclusion with a bool/hash.
class economic_initialized_world_owner final
{
    private:
	friend void game_loop(int, int);
	friend class sql_economic_runtime_boot_owner;
	friend class economic_sql_cutover_transaction_owner;
	friend class economic_sql_accounting_lifecycle_transaction;
	class boot_capability final
	{
		friend class economic_initialized_world_owner;
		friend class sql_economic_runtime_boot_owner;
		boot_capability(MYSQL *connection, unsigned long session, uint64_t authority,
				uint64_t epoch, int64_t process,
				const economic_sql_lifecycle_guard *runtime,
				const economic_sql_runtime_world_writer_guard *writer) noexcept
			: connection_(connection)
			, session_(session)
			, authority_id_(authority)
			, save_epoch_(epoch)
			, process_(process)
			, runtime_(runtime)
			, writer_(writer)
		{
		}
		MYSQL *connection_;
		unsigned long session_;
		uint64_t authority_id_, save_epoch_;
		int64_t process_;
		const economic_sql_lifecycle_guard *runtime_;
		const economic_sql_runtime_world_writer_guard *writer_;

	    public:
		boot_capability(const boot_capability &) = delete;
		boot_capability &operator=(const boot_capability &) = delete;
		boot_capability(boot_capability &&) = delete;
		boot_capability &operator=(boot_capability &&) = delete;
		~boot_capability() noexcept = default;
	};
	struct implementation;
	static implementation &state();
	// True means boot may continue. Inactive capture refusal preserves gameplay
	// behavior and grants no qualified cut. Selected active SQL requires full cut.
	static bool complete_boot() noexcept;
	// Invalidate BEFORE transport readiness, replay/native callbacks or world input.
	// Failure retains genuine SQL/pipeline/coordinator exclusion for terminal cleanup.
	static bool before_world_callbacks() noexcept;
	// The genuine SQL boot owner alone constructs this token from actual held
	// control-session/runtime/writer/reservation/save-epoch state. No raw DTO token.
	static unsigned int capture_sql_cut(const boot_capability &) noexcept;
	// Synchronous borrow only; no native callback, guard release or transaction
	// transfer while borrowed. Consumer must not retain these references. Guarded
	// census alone is neither full SQL normalization nor independent activation proof.
	using consumer = bool (*)(const economic_initialized_world_snapshot &,
				  const boot_capability &, void *) noexcept;
	static bool with_boot_cut(consumer, void *) noexcept;
	// Actual game-loop/SQL owner only. All borrowed preparation must finish first.
	// Revocation precedes promotion; these methods never select/activate an epoch.
	static unsigned int begin_cutover() noexcept;
	static bool abort_cutover() noexcept;
	// Actual transferred owner only; borrow the SAME retained world, not a new
	// capability/census. Callback cannot transfer/end ownership while borrowed.
	using cutover_consumer = bool (*)(const economic_initialized_world_snapshot &,
					  void *) noexcept;
	static bool with_cutover_cut(economic_sql_cutover_transaction_owner &, cutover_consumer,
				     void *) noexcept;
	static unsigned int
	verify_cutover_sources(const economic_sql_lifecycle_request &,
			       const economic_sql_activation_evidence &,
			       economic_sql_initialized_activation_verifier) noexcept;
	static bool sql_cutover_valid(economic_sql_cutover_transaction_owner &) noexcept;
	static unsigned int
	sql_verify_cutover_sources(const economic_sql_lifecycle_request &,
				   const economic_sql_activation_evidence &,
				   economic_sql_initialized_activation_verifier) noexcept;
	static unsigned int sql_prepare_cutover() noexcept;
	static unsigned int sql_begin_cutover() noexcept;
	static bool sql_abort_cutover() noexcept;
	// Implemented in the original SQL boot owner's translation unit. Terminal
	// maintenance transfer belongs to the root integration; never unlock/reacquire.
	static unsigned int sql_begin_cut() noexcept;
	static bool sql_cut_valid(const boot_capability &) noexcept;
	static bool sql_end_cut() noexcept;
};

#endif
