#ifndef DEFERRED_SAVE_POLICY_H
#define DEFERRED_SAVE_POLICY_H

#define PERSISTENCE_DEFERRED_RETRY_INITIAL 4
#define PERSISTENCE_DEFERRED_RETRY_MAX 240

int deferred_save_next_retry_delay(int current);
bool persistence_should_extract_terminal_inventory(bool durable_success, bool terminal_type);

#endif
