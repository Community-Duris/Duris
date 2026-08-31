#ifndef REDIS_LIFECYCLE_H
#define REDIS_LIFECYCLE_H

bool redis_init(void);
void redis_cleanup(void);
bool redis_runtime_enabled(void);

#endif
