#ifndef TEST_PERSISTENCE_H
#define TEST_PERSISTENCE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void test_persistence_reset(void);
int  test_persistence_run_one(const char *name);
void test_persistence_run_all(void);
void test_persistence_print_summary(void);

#ifdef TEST_PERSISTENCE
int  test_persistence_queue_flood_scalar(void);
int  test_persistence_worker_scalar_fallback(void);
int  test_persistence_worker_scalar_fifo_after_retry(void);
int  test_persistence_worker_item_fifo(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
