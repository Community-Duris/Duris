#ifndef DURIS_INFORMATION_CACHE_H
#define DURIS_INFORMATION_CACHE_H
#include <string>
bool information_cache_refresh();
void information_cache_pulse();
void information_cache_shutdown();
std::string information_cache_status();
// Null means unavailable or not an informational page. The pointer is valid
// until the next game-thread publication; pagers must own their text.
const std::string *information_cache_get(const std::string &name);
#endif
