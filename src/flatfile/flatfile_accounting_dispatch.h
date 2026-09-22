#ifndef DURIS_FLATFILE_ACCOUNTING_DISPATCH_H
#define DURIS_FLATFILE_ACCOUNTING_DISPATCH_H
#include "persistence/critical_command_completion.h"

// Coordinator apply callback paired with economic_command_admission_supported.
// Routes accounting envelopes to their typed owners; schema-1 commands retain
// the existing selected repository path. Context is the optional flatfile root.
critical_apply_result flatfile_accounting_apply_selected(const critical_command &, void *);
#endif
