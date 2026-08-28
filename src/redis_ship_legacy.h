#ifndef REDIS_SHIP_LEGACY_H
#define REDIS_SHIP_LEGACY_H

struct redisContext;

void redis_invalidate_ship_snapshot(const char *owner_name);
bool redis_clear_ship_snapshots(struct redisContext *context);

#endif
