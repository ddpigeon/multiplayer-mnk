#ifndef MNK_CORE_LAUNCH_SETTINGS_H
#define MNK_CORE_LAUNCH_SETTINGS_H

#include <stdexcept>
#include <string>
#include <vector>

namespace mnk::core {

/// Target launch the settings are loaded for. Mode is a load-time input
/// because mode-dependent rules (no human seats in training) are load errors.
enum class LaunchMode {
    play,
    training,
};

/// What sits on a seat. Anything else in the settings file is a load error.
enum class SeatType {
    human,
    bot,
};

/// One seat: number 1..p, its type, the optional display name, and the bot
/// strength (meaningful for bot seats only; the bot adapter owns its meaning).
struct Seat {
    int number = 0;
    SeatType type = SeatType::human;
    std::string name; // absent → empty
    int strength = 0;
};

/// The macro configuration, declared exactly once in the settings file, so
/// the UI title, the trainer filename, and the Match dimensions all derive
/// from one place.
struct MacroConfig {
    int m = 0;
    int n = 0;
    int k = 0;
    int p = 0;

    /// "m,n,k,p" without spaces, e.g. "10,10,4,3" — the one string the UI
    /// title / trainer filename can derive from.
    [[nodiscard]] std::string label() const;
};

/// Typed launch settings. Callers never see parser or document types.
struct LaunchSettings {
    MacroConfig macro_config;
    std::vector<Seat> seats; ///< ordered exactly seat 1..p
};

/// Thrown by load(); its message names the rule the settings file breaks.
class settings_error : public std::runtime_error {
public:
    explicit settings_error(const std::string& message) : std::runtime_error(message) {}
};

/// Loads and validates a settings file against the closed two-level grammar
/// (see launch_settings/default.yaml). Throws settings_error naming the first
/// broken rule.
LaunchSettings load(const std::string& path, LaunchMode mode);

} // namespace mnk::core

#endif // MNK_CORE_LAUNCH_SETTINGS_H
