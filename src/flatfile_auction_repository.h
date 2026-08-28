#ifndef DURIS_FLATFILE_AUCTION_REPOSITORY_H
#define DURIS_FLATFILE_AUCTION_REPOSITORY_H

#include "critical_command_coordinator.h"

#include <string>

critical_apply_result flatfile_auction_repository_apply(const std::string &root,
							const critical_command &command);

#endif
