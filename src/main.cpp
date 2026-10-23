#include "ai/10-10-4-3/ai_player.h"
#include "ai/trainer.h"
#include "core/launch_settings.h"
#include "core/match.h"
#include "core/player.h"
#include "ui/human_player.h"
#include "ui/window.h"

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

/// No-argument launch falls back to the reference settings document.
constexpr const char* kDefaultSettingsPath = "launch_settings/default.yaml";

/// The click poll a human seat's UI input adapter uses. Built once per mode
/// and handed to each human adapter by value in make_adapter.
using HumanClick = std::function<std::optional<std::pair<int, int>>()>;

std::string seat_type_name(mnk::core::SeatType type) {
    return type == mnk::core::SeatType::human ? "human" : "bot";
}

/// The one place that may name the AI and UI implementations: a human Seat
/// maps to the UI input adapter, a bot Seat to an AI adapter constructed with
/// that seat's strength. Every seat gets its own owning adapter (bot strengths
/// are per-seat values, not shared state).
std::unique_ptr<mnk::core::IPlayer> make_adapter(const mnk::core::Seat& seat,
                                                 const HumanClick& human_click) {
    if (seat.type == mnk::core::SeatType::human) {
        return std::make_unique<mnk::ui::HumanPlayer>(human_click);
    }
    return std::make_unique<mnk::ai::variant_10_10_4_3::AIPlayer>(seat.strength);
}

/// Builds one owning adapter per seat, in seat order (the loader guarantees
/// seats are exactly 1..p), so the adapter index maps to seat number - 1.
/// No dummy slot and no raw arrays: the Match registry owns who sits where;
/// the vector is only the owners keeping the adapters alive.
std::vector<std::unique_ptr<mnk::core::IPlayer>> build_adapters(
    const std::vector<mnk::core::Seat>& seats, const HumanClick& human_click) {
    std::vector<std::unique_ptr<mnk::core::IPlayer>> adapters;
    adapters.reserve(seats.size());
    for (const auto& seat : seats) {
        adapters.push_back(make_adapter(seat, human_click));
    }
    return adapters;
}

/// The one startup log line naming who is seated, for attribution in logs.
void log_seats(const std::vector<mnk::core::Seat>& seats) {
    std::cout << "Seats: ";
    for (std::size_t i = 0; i < seats.size(); ++i) {
        const auto& seat = seats[i];
        if (i != 0) std::cout << ", ";
        std::cout << "Seat " << seat.number << ": ";
        if (!seat.name.empty()) std::cout << seat.name << " ";
        std::cout << "(" << seat_type_name(seat.type);
        if (seat.type == mnk::core::SeatType::bot) {
            std::cout << ", strength " << seat.strength;
        }
        std::cout << ")";
    }
    std::cout << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        mnk::core::LaunchMode mode = mnk::core::LaunchMode::play;
        std::string settings_path = kDefaultSettingsPath;
        int num_games = 0;

        if (argc == 1) {
            // Common case: launch the default settings for play.
        } else if (const std::string flag = argv[1]; flag == "--launch" && argc == 3) {
            settings_path = argv[2];
        } else if (flag == "--train" && argc == 4) {
            mode = mnk::core::LaunchMode::training;
            settings_path = argv[2];
            const std::string num_text = argv[3];
            try {
                std::size_t parsed = 0;
                num_games = std::stoi(num_text, &parsed);
                if (parsed != num_text.size()) {
                    throw std::invalid_argument("trailing characters");
                }
            } catch (const std::exception&) {
                std::cerr << "invalid number of games: '" << num_text << "'\n";
                return 1;
            }
            if (num_games < 1) {
                std::cerr << "number of games must be at least 1, found "
                          << num_games << "\n";
                return 1;
            }
        } else {
            std::cerr << "usage: mnk_game [--launch <settings.yaml>] | "
                         "--train <settings.yaml> <num_games>\n";
            return 1;
        }

        // Settings load happens before anything opens: a bad file never gets
        // a half-launched window or a half-built adapter set.
        mnk::core::LaunchSettings settings =
            mnk::core::load(settings_path, mode);

        if (mode == mnk::core::LaunchMode::play) {
            const auto& game = settings.macro_config;

            // Match owns the turn loop, seats, and the Game; all numbers here
            // derive from the one macro configuration the settings declare.
            mnk::core::Match match(game.m, game.n, game.k, game.p);

            // Window renders and pumps frames; it drives the Match, never the
            // Game directly. The title derives from the same configuration.
            mnk::ui::Window window(1024, 800, game.label() + " Game Engine", match);

            // Seat adapters; the Window owns the human input.
            auto adapters = build_adapters(settings.seats,
                [&window]() { return window.get_grid_click(); });

            for (std::size_t i = 0; i < settings.seats.size(); ++i) {
                match.register_seat(settings.seats[i].number, adapters[i].get());
            }

            log_seats(settings.seats);

            window.run();

        } else { // training: headless, no UI. All seats already bots at load.
            const auto& game = settings.macro_config;

            // One AI adapter per seat with its own strength, in seat order.
            // The loader guarantees all-bot seats; no extra check. The human
            // click source is absent because no human can sit here.
            auto bots = build_adapters(settings.seats, HumanClick{});

            std::vector<mnk::core::IPlayer*> bot_ptrs;
            bot_ptrs.reserve(bots.size());
            for (const auto& bot : bots) {
                bot_ptrs.push_back(bot.get());
            }

            // The Trainer's data filename derives from the configured
            // dimensions; it owns its own per-game Match loop headless.
            mnk::ai::Trainer trainer(game.m, game.n, game.k, game.p, bot_ptrs);
            trainer.train(num_games);
        }

    } catch (const std::exception& e) {
        // Any settings_error (loader/grammar/legality) and any other
        // exception: report and exit nonzero, without a half-launched window.
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
