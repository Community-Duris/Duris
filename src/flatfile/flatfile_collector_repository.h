#ifndef DURIS_FLATFILE_COLLECTOR_REPOSITORY_H
#define DURIS_FLATFILE_COLLECTOR_REPOSITORY_H

#include "economy/collector_command.h"
#include "economy/collector_custody_boundary.h"
#include "economy/collector_storage.h"
#include "flatfile/flatfile_authority_transaction.h"
#include "persistence/critical_command_coordinator.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class flatfile_collector_repository_result
{
	ok,
	not_found,
	unchanged,
	conflict,
	invalid,
	io_error,
};

struct flatfile_collector_enrollment_mutation
{
	flatfile_authority_after_image after_image;
	uint64_t catalog_revision = 0;
	size_t enrolled = 0;
	size_t cancelled = 0;
};

flatfile_collector_repository_result flatfile_collector_repository_read_bootstrap(
	const std::string &root, collector_bootstrap_snapshot *snapshot, std::string *error);
flatfile_collector_repository_result
flatfile_collector_repository_read_listing(const std::string &root, uint64_t listing,
					   collector_listing_detail *detail, bool *found,
					   std::string *error);

// Called while the item repository owns the shared authority lock. The returned
// catalog image must be committed with the already-prepared corpse/item images.
flatfile_collector_repository_result flatfile_collector_prepare_death_enrollment(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, const item_transfer_result &transfer,
	flatfile_collector_enrollment_mutation *mutation, unsigned int *result_code,
	std::string *error);

// Compose candidate cancellation (including every captured container child)
// with optional death enrollment into one collector after-image. The item
// repository commits this image with custody and all other domain images.
flatfile_collector_repository_result flatfile_collector_prepare_item_boundary(
	const std::string &root, const flatfile_authority_lock &lock,
	const item_transfer_payload &payload, const item_transfer_result &transfer,
	flatfile_collector_enrollment_mutation *mutation, unsigned int *result_code,
	std::string *error);

// Compose candidate cancellation with a corpse lifecycle mutation. The caller
// supplies the exact post-mutation item revisions prepared by the ownership
// repository and commits the returned image in that same authority transaction.
flatfile_collector_repository_result flatfile_collector_prepare_corpse_boundary(
	const std::string &root, const flatfile_authority_lock &lock,
	const corpse_lifecycle_payload &payload,
	const std::vector<collector_custody_boundary_item> &items,
	flatfile_collector_enrollment_mutation *mutation, unsigned int *result_code,
	std::string *error);

critical_apply_result flatfile_collector_repository_apply(const std::string &root,
							  const critical_command &command);

// Inactive schema-2 purchase and held-item terminal owner. Uses the same native
// collector transaction and commits its EAP1 record and exact item reference in
// that authority bundle.
critical_apply_result
flatfile_collector_repository_apply_accounted(const std::string &root,
					      const critical_command &command);

struct flatfile_collector_purchase_projection
{
	critical_apply_result receipt = {};
	collector_command_result result = {};
	uint64_t player_save_revision = 0;
};
// Borrow the existing authority lock through publication/ACK. Reads may finish
// an existing authority journal; they never submit/apply an absent operation.
// Success returns exact historical evidence separately from current projection.
// Receipt/projection outputs are unchanged on failure; the error may change.
// No authority is minted here.
unsigned int flatfile_collector_repository_verify_retained_locked(const std::string &,
								  const flatfile_authority_lock &,
								  const critical_command &,
								  critical_apply_result *,
								  std::string *);
unsigned int flatfile_collector_repository_read_purchase_projection_locked(
	const std::string &, const flatfile_authority_lock &, const critical_command &,
	flatfile_collector_purchase_projection *, std::string *);

#endif
