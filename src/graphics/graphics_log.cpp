#include "graphics_log.hpp"

namespace graphics_log {
namespace {

bool& LogEnabledFlag() {
  static bool enabled = false;
  return enabled;
}

}  // namespace

void SetLogEnabled(bool enabled) { LogEnabledFlag() = enabled; }

bool LogEnabled() { return LogEnabledFlag(); }

}  // namespace graphics_log
