#ifndef DURIS_FLATFILE_CORPSE_REPOSITORY_H
#define DURIS_FLATFILE_CORPSE_REPOSITORY_H

#include "persistence/critical_command_coordinator.h"

#include <string>

critical_apply_result flatfile_corpse_repository_apply(const std::string &root,
						       const critical_command &command);

#endif
