#ifndef DURIS_KINGDOM_RESTORE_H
#define DURIS_KINGDOM_RESTORE_H
#include <string>
// Client-free build only: refuse even individually invalid realm records.
bool kingdom_flatfile_restore_validate(const std::string &root, std::string *error);
#endif
