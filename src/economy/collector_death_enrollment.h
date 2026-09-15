#ifndef DURIS_COLLECTOR_DEATH_ENROLLMENT_H
#define DURIS_COLLECTOR_DEATH_ENROLLMENT_H

#include "economy/collector_eligibility.h"
#include "item/item_transfer_command.h"

#include <vector>

struct char_data;
typedef struct char_data *P_char;
struct obj_data;
typedef struct obj_data *P_obj;

// Begin is called at corpse creation, before any asynchronous custody handoff.
// It snapshots feature policy once for the death. A death begun while disabled
// remains outside collector intake even if the setting changes mid-handoff.
void collector_death_enrollment_begin(P_char character, P_obj corpse);
void collector_death_enrollment_end(P_obj corpse);

enum class collector_death_enrollment_resume_result : uint8_t
{
	ready,
	outside,
	unavailable,
	invalid,
};

// Reconstructs an interrupted multi-batch handoff from the authoritative death
// projection. An unavailable projection is transient and must not turn the
// remaining death custody into a permanent dispute.
collector_death_enrollment_resume_result collector_death_enrollment_resume(P_char character,
									   P_obj corpse);

// Decorates an actual corpse-create transfer with the eligible items in that
// exact immutable payload. The proposed operation becomes the death identity
// only after the first decorated command commits and publishes its authority.
bool collector_death_enrollment_attach(P_char character, P_obj corpse,
				       const critical_operation_id &proposed_operation,
				       const std::vector<player_item_snapshot> &snapshots,
				       item_transfer_payload *payload);
void collector_death_enrollment_note_committed(P_obj corpse, const item_transfer_payload &payload);

void collector_death_enrollment_reset_for_tests(void);

#endif
