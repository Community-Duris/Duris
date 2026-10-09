#ifndef ZONE_RESET_ROOM_PUBLICATION_H
#define ZONE_RESET_ROOM_PUBLICATION_H

#include "persistence/sql_room_item_payload.h"
#include "persistence/critical_command_coordinator.h"
class flatfile_authority_lock;
class quest_mobile_native_item_stage;
class zone_reset_original_room_placement_stage;
#include <unordered_set>
struct quest_mobile_native_item_progress;
struct quest_mobile_native_item_effect;
class critical_zone_reset_item_publication_owner;

// Explicit game-thread ownership of detached original factory stages. Never
// auto-discard admitted/uncertain work; the real recovery-context owner retains
// this handle until its genuine service/progress and publication ACK complete.
class zone_reset_room_publication_stage final
{
    public:
	zone_reset_room_publication_stage() noexcept = default;
	zone_reset_room_publication_stage(const zone_reset_room_publication_stage &) = delete;
	zone_reset_room_publication_stage &
	operator=(const zone_reset_room_publication_stage &) = delete;

    private:
	friend class zone_reset_room_publication_owner;
	struct implementation;
	implementation *state_ = nullptr;
};

class zone_reset_room_publication_owner final
{
    public:
	// Original SQL cold owner already authenticated retained origin, current
	// season/room/custody and current money head in its original transaction.
	// These pure carried values never replace that provenance or permit ACK.
	// Complete per-UID recipe/literal correlation, selected UID/global/cache
	// absence, all object construction and exact detached forest observation.
	// Strong output; no room/global placement, cache hydrate, RNG or events.
	static bool prepare(const sql_room_item_graph &, const std::unordered_set<uint64_t> &,
			    zone_reset_room_publication_stage *) noexcept;
	static bool discard_unpublished(zone_reset_room_publication_stage *) noexcept;
	// Existing SQL caller adapter. Actual consumption requires the shared
	// authenticated recovery-context/service owner; pending work refuses.
	static bool publish(const sql_room_item_graph &, std::unordered_set<uint64_t> *) noexcept;
	// Original SQL cold owner lends its reconnect-disabled IN_TRANS session.
	// Read current graph and real immutable terminal BODY on that same session;
	// no caller-provided graph or boolean supplies publication authority.
	// Caller owns the opaque handle before any admitted/native mutation, retains
	// it on refusal, and owns transaction termination and boot registration.
	static bool publish_locked(MYSQL *, uint64_t root_uid, std::unordered_set<uint64_t> *,
				   zone_reset_room_publication_stage *) noexcept;
	// Lifetime observation only, never SQL/source/native publication authority.
	static bool empty(const zone_reset_room_publication_stage &) noexcept;

    private:
	friend class critical_zone_reset_item_publication_owner;
	friend class zone_reset_item_owner;
	// Private original warm handoff borrows real factory handles. No cold
	// reconstruction or caller-supplied successful progress is accepted.
	static bool prepare_warm(const critical_command &,
				 std::span<quest_mobile_native_item_stage *>,
				 zone_reset_original_room_placement_stage *,
				 zone_reset_room_publication_stage *) noexcept;
	// Real warm root lends its retained flat factory tokens and exact O/P sources.
	// Root/source values are comparisons only. Caller proves closed registry,
	// whole binding, current projection and exact retained original command, and
	// admits all caller inputs/output/span objects in outer_live through return.
	// Candidate and full capture/codec requests are prospective; no publication.
	static bool prepare_warm_flat_bounded(const critical_command &, const std::string &,
					      const std::span<quest_mobile_native_item_stage *> &,
					      const std::span<const economic_source_event> &,
					      zone_reset_original_room_placement_stage *,
					      zone_reset_room_publication_stage *,
					      bool (*)(size_t, void *) noexcept, void *,
					      size_t outer_live) noexcept;
	// Same current original envelope and delivered receipt as the shared driver.
	// Adopt only a complete already-present forest whose original native actions
	// all returned successfully. Never restores a body or repeats native effects.
	static bool prepare_original_completed_locked(MYSQL *,
						      const critical_native_recovery_envelope &,
						      const critical_completion &,
						      zone_reset_room_publication_stage &,
						      bool (*)() noexcept) noexcept;
	// Exact full-present original forest with a known successful service prefix.
	// Native lifetimes and already requested enrollment must actually be present.
	static bool prepare_original_present_prefix_locked(
		MYSQL *, const critical_native_recovery_envelope &, const critical_completion &,
		zone_reset_room_publication_stage &, zone_reset_original_room_placement_stage *,
		bool (*)() noexcept) noexcept;
	static bool prepare_original_present_locked(MYSQL *,
						    const critical_native_recovery_envelope &,
						    const critical_completion &,
						    zone_reset_room_publication_stage &,
						    zone_reset_original_room_placement_stage *,
						    bool (*)() noexcept) noexcept;
	// Complete original terminal BODY, missing full native forest only. Restore
	// through actual frozen-image factories and original cold prepend ordering.
	static bool prepare_original_reconstructed_locked(
		MYSQL *, const critical_native_recovery_envelope &, const critical_completion &,
		zone_reset_room_publication_stage &, std::unordered_set<uint64_t> &,
		bool (*)(std::span<quest_mobile_native_item_stage *>, void *) noexcept, void *,
		bool (*)() noexcept) noexcept;
	// Missing forest with genuinely unstarted or recorded-successful original
	// actions. Rebuild exact known enrollment; journal owner resumes remainder.
	static bool prepare_original_pending_locked(
		MYSQL *, const critical_native_recovery_envelope &, const critical_completion &,
		zone_reset_room_publication_stage &, zone_reset_original_room_placement_stage *,
		std::unordered_set<uint64_t> &,
		bool (*)(std::span<quest_mobile_native_item_stage *>, void *) noexcept, void *,
		bool (*)() noexcept) noexcept;
	// Private recovered flat metadata only. No live factory scope is fabricated.
	// Caller already owns the recovery carrier, selected-root lock and actual
	// retained stage in its census. Refresh CURRENT rooted/global retention on
	// every return, including partial metadata growth or decoder refusal.
	static bool prepare_cold_shape_bounded(const std::string &,
					       const critical_native_recovery_envelope &,
					       zone_reset_room_publication_stage &,
					       bool (*)(size_t, void *) noexcept, void *,
					       size_t outer_live) noexcept;
	// Full original ordered literal/custody/current projection observation under
	// the same genuine recovered flat lock and successful receipt. No restoration,
	// binding, enrollment, cache hydration, placement or ACK authority is granted.
	static bool refresh_cold_flat_locked_bounded(const std::string &,
						     const flatfile_authority_lock &,
						     const critical_native_recovery_envelope &,
						     const critical_completion &,
						     zone_reset_room_publication_stage &,
						     bool (*)(size_t, void *) noexcept, void *,
						     size_t outer_live) noexcept;
	static bool restore_original_missing_forest(zone_reset_room_publication_stage &,
						    bool (*)() noexcept) noexcept;
	static bool pending_items(const zone_reset_room_publication_stage &, int,
				  size_t *) noexcept;
	static bool refresh_warm_locked(MYSQL *, const critical_command &,
					const critical_completion &,
					zone_reset_room_publication_stage &) noexcept;
	// Same genuine original warm root/command and recovered selected-root lock.
	// Reader authenticates the full CURRENT room; comparison values grant no
	// factory/physical/ACK permission. Caller owns prior stage/input/lock storage
	// in outer_live and retains admission peaks through metadata transfer.
	static bool refresh_warm_flat_locked_bounded(const std::string &,
						     const flatfile_authority_lock &,
						     const critical_native_recovery_envelope &,
						     const critical_completion &,
						     zone_reset_room_publication_stage &,
						     bool (*)(size_t, void *) noexcept, void *,
						     size_t outer_live) noexcept;
	// Complete original physical/cache proof for an already consumed warm flat
	// forest. Pure observation; no mutation, source or publication entitlement.
	static bool verify_warm_flat_current_bounded(const zone_reset_room_publication_stage &,
						     bool (*)(size_t, void *) noexcept, void *,
						     size_t outer_live) noexcept;
	static bool reserve_warm_consume(zone_reset_room_publication_stage &,
					 const std::unordered_set<uint64_t> &) noexcept;
	static bool place_warm(zone_reset_room_publication_stage &,
			       quest_mobile_native_item_effect &) noexcept;
	static bool discard_refused_warm(zone_reset_room_publication_stage &) noexcept;
	static bool cancel_warm(const critical_native_recovery_envelope &,
				const critical_completion &, uint64_t,
				bool (*)(const critical_command &, const critical_completion &,
					 void *) noexcept,
				void *) noexcept;
	static bool retained_size(const zone_reset_room_publication_stage &, size_t *) noexcept;
	// Passive actual registered-pool identity selects the paired complete flat
	// private census. SQL and the existing six-owner policy retain their observer.
	static bool retained_size_registered_literal_pool(const zone_reset_room_publication_stage &,
							  size_t *) noexcept;
	// Genuine passive outside observer for the private registered full ROOT
	// policy. Actual locked callbacks must use the paired coordinator lender.
	static bool current_coordinator_storage(size_t *) noexcept;
	static critical_submit_result submit_warm(critical_native_recovery_envelope);
	static bool copy_warm(const critical_command &,
			      critical_native_recovery_envelope *) noexcept;
	static bool checkpoint_warm(const critical_native_recovery_envelope &,
				    const critical_native_recovery_envelope &) noexcept;
	static bool generation_warm(const critical_native_recovery_envelope &, uint64_t *) noexcept;
	static bool acknowledge_warm(const critical_native_recovery_envelope &,
				     const critical_completion &, uint64_t) noexcept;
	static bool retire_warm(const critical_native_recovery_envelope &, uint64_t) noexcept;
	// Actual ROOM coordinator capability; full caller scratch survives each real
	// bounded clone, same-phase CAS, generation observation and receipt ACK.
	// These bridges grant no factory, source, native-effect or physical proof.
	static bool copy_warm_bounded(const critical_command &, critical_native_recovery_envelope *,
				      bool (*)(size_t, void *) noexcept, void *,
				      size_t outer_live) noexcept;
	static bool checkpoint_warm_bounded(const critical_native_recovery_envelope &,
					    const critical_native_recovery_envelope &,
					    uint64_t original_generation,
					    bool (*)(size_t, void *) noexcept, void *,
					    size_t outer_live) noexcept;
	static bool generation_warm_bounded(const critical_native_recovery_envelope &, uint64_t *,
					    bool (*)(size_t, void *) noexcept, void *,
					    size_t outer_live) noexcept;
	static bool acknowledge_warm_bounded(const critical_native_recovery_envelope &,
					     const critical_completion &, uint64_t,
					     bool (*)(size_t, void *) noexcept, void *,
					     size_t outer_live) noexcept;
	static bool retain_terminal(const critical_native_recovery_envelope &, void *) noexcept;
	// Same genuine selected-root lock held through terminal transfer AND guarded
	// mixed-journal retirement. Caller owns complete original envelope, generation,
	// UID provenance, root/lock storage and prior scratch in outer_live. No lock
	// acquisition, phase fabrication, ACK or native execution permission here.
	// Real storage recovers every retry and requires exact full BODY readback;
	// failed transfer/retirement keeps original recovery/carrier/fences. Callback
	// retains coordinator's full prefix; persistent journal attempt storage is
	// separately counted once by aggregate, outside outer_live. Root joins remain
	// responsible for complete physical proof, ACK and metadata lifetime.
	struct flat_terminal_retirement;
	static bool retain_terminal_flat_bounded(const critical_native_recovery_envelope &, void *,
						 size_t coordinator_live) noexcept;
	static bool retire_warm_flat_locked_bounded(
		const std::string &, const flatfile_authority_lock &, uint64_t origin_uid,
		const critical_native_recovery_envelope &, uint64_t original_generation,
		bool (*)(size_t, void *) noexcept, void *, size_t outer_live) noexcept;

	static size_t item_count(const zone_reset_room_publication_stage &) noexcept;
	static size_t original_index(const zone_reset_room_publication_stage &, size_t) noexcept;
	static const zone_reset_item_image *
	original(const zone_reset_room_publication_stage &) noexcept;
	static const sql_room_item_graph *
	current(const zone_reset_room_publication_stage &) noexcept;
	static bool read_progress(const zone_reset_room_publication_stage &, size_t,
				  quest_mobile_native_item_progress *) noexcept;
	static size_t step_count(const zone_reset_room_publication_stage &, size_t) noexcept;
	// Actual context admission precedes this metadata-retention operation.
	// A retained stage can no longer be discarded after checkpoint refusal.
	static bool retain_admitted(zone_reset_room_publication_stage &) noexcept;
	static bool consume(zone_reset_room_publication_stage &,
			    std::unordered_set<uint64_t> *) noexcept;
	static bool place(zone_reset_room_publication_stage &) noexcept;
	static bool service_step(zone_reset_room_publication_stage &, size_t, size_t,
				 quest_mobile_native_item_effect &) noexcept;
	// Genuine warm-FLAT service provider; caller retains CURRENT pool/output/Zombie
	// and private stage storage once, refreshing all on every return. Actual native
	// returned/succeeded markers precede any post-effect diagnostic refusal.
	static bool service_step_bounded(zone_reset_room_publication_stage &, size_t, size_t,
					 quest_mobile_native_item_effect &,
					 bool (*)(size_t, void *) noexcept, void *,
					 size_t outer_live) noexcept;
	static bool rebuild_enrollment(zone_reset_room_publication_stage &, size_t,
				       const quest_mobile_native_item_progress &,
				       std::span<const quest_mobile_native_item_effect>) noexcept;
	static bool verify_current(const zone_reset_room_publication_stage &) noexcept;
	static bool mark_published(zone_reset_room_publication_stage &,
				   std::unordered_set<uint64_t> *) noexcept;
	// Genuine warm-FLAT lifecycle companions only. Caller includes real stage,
	// partial union, published tracker and all other capacities in outer_live.
	// Reserve extends only the original proven partial UID union with unchanged
	// default hash growth; retain/recount that union after every return/refusal.
	// Consume includes initial runtime cache observer EXACTLY ONCE and admits
	// its three actual input spans before bounded DB publication. Retain admitted
	// peaks and refresh CURRENT cache allowance on every return, including false
	// after a global reserve. No allocating work/callback follows DB consumption.
	// Mark keeps full current physical/cache verification and original exact
	// union/swap/creation-marker predicates; factory progress/release remains in
	// original guarded terminal metadata owner. No SQL/cold/service/place wrapper,
	// new authority or native retry permission; unsupported GCC13 ABI refuses.
	static bool reserve_warm_consume_bounded(zone_reset_room_publication_stage &,
						 const std::unordered_set<uint64_t> &,
						 bool (*)(size_t, void *) noexcept, void *,
						 size_t outer_live) noexcept;
	static bool consume_bounded(zone_reset_room_publication_stage &,
				    std::unordered_set<uint64_t> *,
				    bool (*)(size_t, void *) noexcept, void *,
				    size_t outer_live) noexcept;
	static bool mark_published_bounded(zone_reset_room_publication_stage &,
					   std::unordered_set<uint64_t> *,
					   bool (*)(size_t, void *) noexcept, void *,
					   size_t outer_live) noexcept;
	static bool release_completed(zone_reset_room_publication_stage &) noexcept;
	// Only the original root after guarded terminal transfer/retirement.
	static bool
	release_original_terminal_metadata(zone_reset_room_publication_stage &) noexcept;
	// Complete private flat recovery companions. Caller includes all CURRENT
	// literal/event pools, output, Zombie/cache/activity/pending globals ONCE,
	// every rooted stage and complete outer locals. Callees refresh globals
	// before each later nested handoff and on every real native return. Retain
	// partial genuine stages and returned markers; never re-roll or retag source.
	// These remain unselected until all paired pool census consumers join.
	static bool read_cold_present_projection_bounded(
		const std::string &selected_root, const flatfile_authority_lock &lock,
		const critical_native_recovery_envelope &original,
		const critical_completion &receipt, zone_reset_room_publication_stage &stage,
		bool (*reserve)(size_t, void *) noexcept, void *context,
		size_t outer_live) noexcept;
	static bool
	reserve_rooted_flat_consume_bounded(zone_reset_room_publication_stage &stage,
					    const std::unordered_set<uint64_t> &published,
					    bool (*reserve)(size_t, void *) noexcept, void *context,
					    size_t outer_live) noexcept;
	static bool place_warm_bounded(zone_reset_room_publication_stage &stage,
				       quest_mobile_native_item_effect &effect,
				       bool (*reserve)(size_t, void *) noexcept, void *context,
				       size_t outer_live) noexcept;
	static bool
	rebuild_enrollment_bounded(zone_reset_room_publication_stage &stage, size_t at,
				   const quest_mobile_native_item_progress &progress,
				   const std::span<const quest_mobile_native_item_effect> &effects,
				   bool (*reserve)(size_t, void *) noexcept, void *context,
				   size_t outer_live) noexcept;
	static bool
	restore_original_missing_forest_bounded(zone_reset_room_publication_stage &held,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer_live) noexcept;
	static bool prepare_original_completed_flat_locked_bounded(
		const std::string &selected_root, const flatfile_authority_lock &lock,
		const critical_native_recovery_envelope &envelope,
		const critical_completion &receipt, zone_reset_room_publication_stage &held,
		bool (*reserve)(size_t, void *) noexcept, void *budget_context,
		size_t outer_live) noexcept;
	static bool prepare_original_present_prefix_flat_locked_bounded(
		const std::string &selected_root, const flatfile_authority_lock &lock,
		const critical_native_recovery_envelope &envelope,
		const critical_completion &receipt, zone_reset_room_publication_stage &held,
		zone_reset_original_room_placement_stage *placement,
		bool (*reserve)(size_t, void *) noexcept, void *budget_context,
		size_t outer_live) noexcept;
	static bool prepare_original_present_flat_locked_bounded(
		const std::string &selected_root, const flatfile_authority_lock &lock,
		const critical_native_recovery_envelope &envelope,
		const critical_completion &receipt, zone_reset_room_publication_stage &held,
		zone_reset_original_room_placement_stage *placement,
		bool (*reserve)(size_t, void *) noexcept, void *budget_context,
		size_t outer_live) noexcept;
	static bool prepare_original_pending_flat_locked_bounded(
		const std::string &selected_root, const flatfile_authority_lock &lock,
		const critical_native_recovery_envelope &envelope,
		const critical_completion &receipt, zone_reset_room_publication_stage &held,
		zone_reset_original_room_placement_stage *placement,
		std::unordered_set<uint64_t> &published,
		bool (*bindings)(const std::span<quest_mobile_native_item_stage *> &, void *,
				 bool (*)(size_t, void *) noexcept, void *, size_t) noexcept,
		void *binding_context, bool (*reserve)(size_t, void *) noexcept,
		void *budget_context, size_t outer_live) noexcept;
	static bool prepare_original_reconstructed_flat_locked_bounded(
		const std::string &selected_root, const flatfile_authority_lock &lock,
		const critical_native_recovery_envelope &envelope,
		const critical_completion &receipt, zone_reset_room_publication_stage &held,
		std::unordered_set<uint64_t> &published,
		bool (*bindings)(const std::span<quest_mobile_native_item_stage *> &, void *,
				 bool (*)(size_t, void *) noexcept, void *, size_t) noexcept,
		void *binding_context, bool (*reserve)(size_t, void *) noexcept,
		void *budget_context, size_t outer_live) noexcept;
	static bool place_cold_flat_bounded(zone_reset_room_publication_stage &held,
					    bool (*reserve)(size_t, void *) noexcept, void *context,
					    size_t outer_live) noexcept;
	// Genuine private flat metadata census paired with object/affect pool globals.
	// Includes every owned cold wrapper and raw body while actually private;
	// warm borrowed factories remain in their real ROOT census. Strong output.
	// Original retained_size stays selected until the real pool owner joins all
	// matching warm/cold/NPC consumers. This grants no admission or source proof.
	static bool retained_size_excluding_literal_pools(const zone_reset_room_publication_stage &,
							  size_t *) noexcept;
	// Real coordinator capabilities with complete outer and caller-owned
	// cleanup_called/cleanup_succeeded storage alive through native disposal.
	static bool completion_warm_bounded(const critical_operation_id &, critical_completion *,
					    bool (*)(size_t, void *) noexcept, void *,
					    size_t) noexcept;
	static bool cancel_warm_bounded(const critical_native_recovery_envelope &,
					const critical_completion &, uint64_t,
					bool (*)(const critical_command &,
						 const critical_completion &, void *,
						 size_t) noexcept,
					void *, bool (*)(size_t, void *) noexcept, void *, size_t,
					bool *, bool *) noexcept;
};

#endif
