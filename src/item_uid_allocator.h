#ifndef ITEM_UID_ALLOCATOR_H
#define ITEM_UID_ALLOCATOR_H

#include <cstdint>

struct st_mysql;
typedef struct st_mysql MYSQL;

constexpr uint64_t ITEM_UID_BOOT_RESERVATION = UINT64_C(1000000);

bool item_uid_allocator_reserve(MYSQL *connection, uint64_t count);
uint64_t item_uid_allocator_next(void);
uint64_t item_uid_allocator_remaining(void);
void item_uid_allocator_reset_for_tests(void);

#endif
