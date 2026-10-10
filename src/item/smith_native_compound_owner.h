#ifndef SMITH_NATIVE_COMPOUND_OWNER_H
#define SMITH_NATIVE_COMPOUND_OWNER_H

#include "economy/smith_native_producer.h"
#include "economy/smith_native_before_sql.h"
#include "player/player_save_pipeline.h"

#include <memory>

// Borrowed immutable preparation values only. Valid until the actual retained
// owner dies; no source, participant, save release, publication or ACK permit.
struct smith_native_compound_prepared_view
{
	const smith_native_compound_terms *terms = nullptr;
	const smith_native_compound_images *images = nullptr;
	const smith_native_player_grant_projection *original_grant = nullptr;
	const economic_native_money_checkpoint_projection *admission_mapping = nullptr;
	const critical_command *command = nullptr;
	const player_smith_checkpoint_stage *checkpoint = nullptr;
	const player_snapshot *acknowledged_filtered_save = nullptr;
	const std::vector<uint8_t> *recipe_bytes = nullptr;
	const native_mobile_birth_item_recipe *factory_recipe = nullptr;
};

// Original producer alone may retain this preparation owner. No selected call
// is supplied here; the active Smith guard remains unchanged. Frozen values are
// observations/command preparation, never admission, commit, publication or ACK.
class smith_native_compound_owner final
{
	friend int smith(char_data *, char_data *, int, char *);
	struct preparation;
	const smith_native_original_selection &original_;
	quest_mobile_native_item_stage &factory_;
	smith_native_output_preparation &output_;
	smith_native_player_grant_preparation &grant_;
	smith_native_persisted_before &persisted_;
	const player_smith_checkpoint_token checkpoint_;
	const critical_operation_id operation_;
	const economic_source_event source_;
	const uint64_t accepted_at_usec_;
	std::unique_ptr<preparation> prepared_;

	// Actual same-root objects stay at their original addresses until this owner
	// dies. Constructor copies only actual caller's fixed retained identity data;
	// it generates no operation/source and acquires no factory/checkpoint/session.
	smith_native_compound_owner(const smith_native_original_selection &,
				    quest_mobile_native_item_stage &,
				    smith_native_output_preparation &,
				    smith_native_player_grant_preparation &,
				    smith_native_persisted_before &,
				    const player_smith_checkpoint_token &,
				    const critical_operation_id &, const economic_source_event &,
				    uint64_t actual_accepted_at_usec) noexcept;
	~smith_native_compound_owner();
	smith_native_compound_owner(const smith_native_compound_owner &) = delete;
	smith_native_compound_owner &operator=(const smith_native_compound_owner &) = delete;
	smith_native_compound_owner(smith_native_compound_owner &&) = delete;
	smith_native_compound_owner &operator=(smith_native_compound_owner &&) = delete;

	// Genuine SQL/common preparation only. Caller owns the original lifecycle/
	// writer exclusion and reconnect-disabled IN_TRANS session. Existing returned
	// factory/output/grant and held ACK must already exist; only rechecks run.
	// No begin/commit/rollback/DML, fee, native draw/effect or held release occurs.
	// False preserves all borrowed owners and previous successful preparation.
	bool prepare_sql(MYSQL *actual_original_session) noexcept;
	// The caller must recheck its live/root/session cut through prepare_sql before
	// handing these frozen values onward. This does not revalidate current authority.
	bool observe_prepared(smith_native_compound_prepared_view *) const noexcept;
};

#endif
