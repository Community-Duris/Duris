#ifndef DURIS_TELEMETRY_CONFIG_RELOAD_H
#define DURIS_TELEMETRY_CONFIG_RELOAD_H

/* Private E/#263 seam, deliberately link-independent until F/#265 registers the
 * config owner. All calls belong to the game thread. The observer must be a
 * bounded, nonthrowing, read-only capture: no SQL, file I/O or reentrant property
 * mutation. Its context is borrowed until exact-pair unregistration. */
using telemetry_config_reload_callback = void (*)(void *) noexcept;

namespace telemetry_config_reload_detail
{
inline telemetry_config_reload_callback observer = nullptr;
inline void *context = nullptr;
} // namespace telemetry_config_reload_detail

inline bool telemetry_config_reload_register(telemetry_config_reload_callback observer,
					     void *context) noexcept
{
	if (!observer)
		return false;
	if (telemetry_config_reload_detail::observer &&
	    (telemetry_config_reload_detail::observer != observer ||
	     telemetry_config_reload_detail::context != context))
		return false;
	telemetry_config_reload_detail::observer = observer;
	telemetry_config_reload_detail::context = context;
	return true;
}

inline bool telemetry_config_reload_unregister(telemetry_config_reload_callback observer,
					       void *context) noexcept
{
	if (!observer || telemetry_config_reload_detail::observer != observer ||
	    telemetry_config_reload_detail::context != context)
		return false;
	telemetry_config_reload_detail::observer = nullptr;
	telemetry_config_reload_detail::context = nullptr;
	return true;
}

inline void telemetry_config_reload_notify() noexcept
{
	const auto observer = telemetry_config_reload_detail::observer;
	void *const context = telemetry_config_reload_detail::context;
	if (observer)
		observer(context);
}

#endif
