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

#endif
