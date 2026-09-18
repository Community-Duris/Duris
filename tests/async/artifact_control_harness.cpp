#include "artifact/artifact_control.h"

#include <cassert>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
	assert(argc == 2);
	artifact_control::catalog loaded;
	std::vector<artifact_control::diagnostic> diagnostics;
	assert(artifact_control::catalog_load_file(argv[1], &loaded, &diagnostics));
	assert(loaded.definitions.size() == 7);
	assert(loaded.hash == artifact_control::catalog_hash(loaded));

	std::string error;
	assert(artifact_control::control_initialize(argv[1], &error));
	assert(artifact_control::control_status().active_revision == 1);
	assert(artifact_control::control_set_variant(31514, artifact_control::holder_kind::player,
						     "telegraphic", &error) ==
	       artifact_control::control_result::ok);
	assert(artifact_control::control_status().dirty);
	const auto *active_before_publish =
		artifact_control::catalog_find(artifact_control::control_catalog(), 31514);
	assert(active_before_publish && active_before_publish->player_variant == "legacy");
	assert(artifact_control::control_publish(&error));
	assert(!artifact_control::control_status().dirty);
	assert(artifact_control::control_status().active_revision == 2);
	const auto *active_after_publish =
		artifact_control::catalog_find(artifact_control::control_catalog(), 31514);
	assert(active_after_publish && active_after_publish->player_variant == "telegraphic");

	assert(artifact_control::control_set_enabled(31514, false, &error) ==
	       artifact_control::control_result::ok);
	assert(artifact_control::control_discard(&error));
	assert(artifact_control::control_enabled(31514));
	return 0;
}
