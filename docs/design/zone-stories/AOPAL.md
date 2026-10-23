# The Tharnadian Causeway

Canonical key: `component:aopal`. Physical coverage row 361. This is a
source-only map, absent from both current `areas/AREA` manifests. The
[technical audit](../quest-story-followups/AOPAL_AUDIT_REVIEW.md) records its
story sources, isolated spawn room, six source-only approaches, numeric room
conflicts, and patrol callback.

## The road outside the city

The causeway presents Tharnadia's edge as a defended, worked road. White
brick, travelers, merchants, engineers, and a wagon station place commerce
beside the Outer Keep's warning signs, archer slits, and watch posts. The
source describes the keep as the city's first defense against invasion. Its
upper paths follow an aqueduct and reservoir, while the bastion stairs and
guard stations trace a four-story military post.

The waterway adds a small domestic thread: water rats haunt the reservoir,
dogs splash in its water, and a ghost girl named Samara is trapped beneath a
well. Her faint image and soaked white dress make the well a quiet haunting
inside an otherwise occupied road and fortress. The reset gives the dress to
Samara; the local files contain no QST giver or structured quest sidecar.

The Highlord Bastion Captain commands local guards and the highway patrol.
Its source code attaches a patrol-hiring handler to the Captain's Station;
the reset stages an elite patrolman and highway marshall in numeric room
#43342. This connects
the defensive setting to an authored protection service, while actual
availability remains dependent on loader state and room identity.

## Keep the map identity explicit

The comments in `areas/AREA` and `areas/qst/AREA` label the source `aopal`
and describe it as a road connected to Tharnadia. Its ZON title is “The
Tharnadian Causeway.” No repository WLD edge from the map leads to active
Tharnadia. The source instead has boundary records toward Shadow Forest and
Ancient Bridge, four number matches into active Ship Yards, and one exit to
an ambiguous unlisted Surface room. The shared Ship Yards numbers identify
different room records, not a replacement map.

Keep `aopal` **RETAIN IN PLACE** until loader ownership, the overlapping room
identities, the imported #445 patrolman, missing mobile #6116, and the
Captain's compiled service are resolved by the owner. See the [audit](../quest-story-followups/AOPAL_AUDIT_REVIEW.md#safe-disposition),
[builder questions](AOPAL_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[candidate row 10](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10),
and [coverage row 361](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L361).
