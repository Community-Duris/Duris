#ifndef DURIS_FLATFILE_ECONOMIC_RUNTIME_H
#define DURIS_FLATFILE_ECONOMIC_RUNTIME_H

// Trusted configured-root startup, before native/world hydration or replay.
// Only verifies/reconstructs an already selected lifecycle; never activates.
bool flatfile_economic_runtime_start() noexcept;
// Successful coordinator/save shutdown under the existing lifecycle guard.
// Refused or cancelled lifecycle operations must keep their projection.
void flatfile_economic_runtime_shutdown() noexcept;

#endif
