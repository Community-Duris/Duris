#ifndef DURIS_SQL_ECONOMIC_RUNTIME_H
#define DURIS_SQL_ECONOMIC_RUNTIME_H

// Boot-only owner: acquire before native writes or worker admission. The
// dedicated configured control connection remains alive until shutdown.
bool sql_economic_runtime_start() noexcept;
void sql_economic_runtime_shutdown() noexcept;

#endif
