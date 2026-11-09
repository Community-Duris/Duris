#ifndef DURIS_AUCTION_WEBSITE_H
#define DURIS_AUCTION_WEBSITE_H

#include "economy/auction_command.h"

// The caller must be the authenticated web bridge. Ownership is checked again
// here against the MUD's catalog; a logged-in character is not required.
bool auction_website_build_bid(const char *account, uint32_t pid, uint32_t auction_id,
			       int64_t copper, const char *request_id, critical_command *command);

#endif
