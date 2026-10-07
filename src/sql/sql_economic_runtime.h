#ifndef DURIS_SQL_ECONOMIC_RUNTIME_H
#define DURIS_SQL_ECONOMIC_RUNTIME_H

#include <cstddef>
#include <cstdint>
struct critical_completion;

// Boot-only owner: acquire before native writes or worker admission. The
// dedicated configured control connection remains alive until shutdown.
bool sql_economic_runtime_start() noexcept;
void sql_economic_runtime_shutdown() noexcept;
// Actual startup alone uses the split owner. Direct full recovery stays intact.
bool sql_economic_runtime_start_recovery() noexcept;
enum class sql_economic_boot_progress : uint8_t
{
	refused,
	pending,
	ready
};
sql_economic_boot_progress sql_economic_runtime_recover_boot_step(
	void (*original_completions)(const critical_completion *, size_t)) noexcept;
// After genuine full promotion and ordinary save execution startup, the same
// owner releases its real coordinator reservation. No public readiness setter.
bool sql_economic_runtime_finish_boot_admission() noexcept;

#endif
