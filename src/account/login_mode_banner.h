#pragma once

#include <string>

/* DURIS_STAGING=TRUE (.env) marks a public staging server.  Only the login
 * banner reads it. */
bool duris_staging_enabled(void);

/* One blinking, all-caps line naming each enabled server mode, ending in CRLF,
 * or an empty string when no mode is enabled. */
std::string login_mode_banner(bool staging, bool chaos, bool all_races, bool all_classes);
