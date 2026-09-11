#ifndef DURIS_FLATFILE_CORPSE_REPOSITORY_H
#define DURIS_FLATFILE_CORPSE_REPOSITORY_H

#include "persistence/critical_command_coordinator.h"

#include <string>

critical_apply_result flatfile_corpse_repository_apply(const std::string &root,
						       const critical_command &command);

// Read-only validation of the complete replay catalog, including lazy entries.
bool flatfile_corpse_repository_validate(const std::string &root, std::string *error);

#endif
