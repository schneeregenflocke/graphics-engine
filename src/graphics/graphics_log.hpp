#ifndef GRAPHICS_LOG_HPP
#define GRAPHICS_LOG_HPP

// Runtime debug output of the engine, off unless the application switches it
// on through SetLogEnabled.
namespace graphics_log {

void SetLogEnabled(bool enabled);

bool LogEnabled();

}  // namespace graphics_log

#endif  // GRAPHICS_LOG_HPP
