#ifndef SESSION_AUDIT_TRANSACTION_H
#define SESSION_AUDIT_TRANSACTION_H

#include "account/session_audit_command.h"
#include "core/structs.h"

bool session_audit_transaction_submit(P_char character, session_audit_event event);

#endif
