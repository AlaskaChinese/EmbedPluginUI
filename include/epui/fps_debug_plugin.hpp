#pragma once

#include "epui/diagnostics_plugin.hpp"

namespace epui {

// Source-compatible names retained for existing applications. New code can use
// DiagnosticsPlugin / DiagnosticsStyle directly.
using FpsDebugStyle = DiagnosticsStyle;
using FpsDebugPlugin = DiagnosticsPlugin;

} // namespace epui
