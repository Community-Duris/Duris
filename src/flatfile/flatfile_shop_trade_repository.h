#ifndef DURIS_FLATFILE_SHOP_TRADE_REPOSITORY_H
#define DURIS_FLATFILE_SHOP_TRADE_REPOSITORY_H

#include "persistence/critical_command_coordinator.h"

#include <string>

critical_apply_result flatfile_shop_trade_repository_apply(const std::string &root,
							   const critical_command &command);

#endif
