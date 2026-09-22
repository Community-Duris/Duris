#include "persistence/economic_accounting_repository.h"
#include <cassert>
#include <cerrno>

int main()
{
	economic_sql_authority_snapshot snapshot;
	snapshot.lineage_revision = 123;
	snapshot.lineage.bytes[0] = 7;
	snapshot.epoch.bytes[0] = 9;
	economic_sql_locked_mapping mapping = {};
	mapping.request.native_id = 42;
	mapping.revision = 81;
	snapshot.mappings.push_back(mapping);
	assert(economic_sql_lock_authority(nullptr, {}, {}, {}, &snapshot) == ENOTSUP);
	assert(snapshot.lineage_revision == 123 && snapshot.mappings.size() == 1);
	assert(snapshot.lineage.bytes[0] == 7 && snapshot.epoch.bytes[0] == 9);
	assert(snapshot.mappings[0].request.native_id == 42);
	assert(snapshot.mappings[0].revision == 81);
	assert(economic_sql_lock_authority(nullptr, {}, {}, {}, nullptr) == ENOTSUP);
}
