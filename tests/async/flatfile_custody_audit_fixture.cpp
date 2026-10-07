#include "../../scripts/qualify_flatfile_native_custody.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_store.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sys/stat.h>

void logit(const char *, const char *, ...) {}

int main(int argc, char **argv)
{
	assert(argc == 4);
	const std::string root = argv[1];
	std::ifstream stream(argv[2], std::ios::binary);
	const std::vector<uint8_t> encoded((std::istreambuf_iterator<char>(stream)), {});
	const bool expected = std::string(argv[3]) == "1";
	std::filesystem::create_directories(root + "/domains");
	assert(chmod(root.c_str(), 0700) == 0 && chmod((root + "/domains").c_str(), 0700) == 0);
	std::string error;
	assert(flatfile_atomic_write(root + "/domains", "item_ownership", encoded, &error));
	std::vector<flatfile_item_ownership_record> native;
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root, &error));
		const auto result = flatfile_item_repository_recovery_catalog_locked(
			root, lock, &native, &error);
		assert((result == flatfile_item_repository_result::ok) == expected);
	}
	bool accepted = false;
	try
	{
		const auto independent = restore_native_custody::decode_catalog(encoded);
		accepted = true;
		assert(expected && independent.items.size() == native.size());
		for (size_t i = 0; i < native.size(); ++i)
		{
			const auto &left = independent.items[i];
			const auto &right = native[i];
			assert(left.uid == right.item_uid && left.root == right.root_item_uid &&
			       left.parent == right.parent_item_uid &&
			       left.revision == right.item_revision &&
			       left.location.type == static_cast<uint8_t>(right.owner.type) &&
			       left.location.id == right.owner.id &&
			       left.location.context == right.owner.context_id &&
			       left.vnum == right.vnum &&
			       left.state == static_cast<uint8_t>(right.state) &&
			       left.equipment == right.equipment_slot &&
			       restore_economic_authority::same(left.coin_payload,
								right.coin_payload));
		}
	}
	catch (const std::runtime_error &)
	{
		assert(!expected);
	}
	assert(accepted == expected);
	if (expected)
	{
		restore_economic_authority::audit_budget budget;
		const auto summary = restore_native_custody::audit_catalog(root, budget);
		assert(summary.present && summary.items == native.size());
		budget.remaining_bytes = 0;
		bool refused = false;
		try
		{
			(void)restore_native_custody::audit_catalog(root, budget);
		}
		catch (const restore_economic_authority::audit_budget_refused &)
		{
			refused = true;
		}
		assert(refused);
	}
	std::cout << "{\"native_accepted\":" << (expected ? "true" : "false")
		  << ",\"independent_accepted\":" << (accepted ? "true" : "false")
		  << ",\"items\":" << native.size() << ",\"item_fields_match\":true}\n";
}
