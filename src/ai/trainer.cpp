#include "ai/trainer.h"
#include "ai/ai_player_interface.h"
#include "core/match.h"
#include <iostream>
#include <fstream>
#include <format>
#include <chrono>
#include <stdexcept>

namespace mnk::ai {

struct TurnRecord {
    std::string board_state;
    std::vector<uint32_t> visit_counts;
};

Trainer::Trainer(int rows, int cols, int win_length, int num_players, std::vector<core::IPlayer*> bots)
    : rows_(rows), cols_(cols), win_length_(win_length), num_players_(num_players),
      total_moves_(rows * cols), bots_(std::move(bots)) {

    if (bots_.size() != static_cast<std::size_t>(num_players_)) {
        throw std::invalid_argument(
            "Trainer: expected one player per seat (" + std::to_string(num_players_)
            + "), got " + std::to_string(bots_.size()) + ".");
    }

    filename_ = std::format("training_{}_{}_{}_{}.dat", rows_, cols_, win_length_, num_players_);
    load_data();
}

std::string Trainer::board_to_string(const core::Board& board) const {
    std::string s;
    s.reserve(rows_ * cols_);
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            s += static_cast<char>(board.get_cell(r, c) + '0');
        }
    }
    return s;
}

void Trainer::train(int num_games) {
    std::cout << "Starting training mode. Target games: " << num_games << "\n";
    std::cout << "Data will be saved to: " << filename_ << "\n\n";

    // A fresh Match per game: seats are re-registered, the Game resets
    for (int i = 1; i <= num_games; ++i) {
        core::Match match(rows_, cols_, win_length_, num_players_);
        for (int seat = 1; seat <= num_players_; ++seat) {
            match.register_seat(seat, bots_[static_cast<std::size_t>(seat - 1)]);
        }

        std::vector<TurnRecord> game_records;

        auto start_time = std::chrono::steady_clock::now();

        while (match.game().state() == core::GameState::IN_PROGRESS) {
            // Record the board *before* the step so the visit counts
            // describe the move played from exactly this state.
            std::string state_str = board_to_string(match.game().board());
            const core::Player mover = match.game().current_player();

            if (match.step() != core::StepResult::Played) {
                break; // player not ready; a stuck bot shows up as a dead game
            }

            std::vector<uint32_t> visits(total_moves_, 0);
            if (auto* ai_bot = dynamic_cast<IAIPlayer*>(bots_[static_cast<std::size_t>(mover) - 1])) {
                visits = ai_bot->get_last_visit_counts();
            }

            game_records.push_back({state_str, visits});
        }

        // Push final terminal state with zero visits
        game_records.push_back({board_to_string(match.game().board()), std::vector<uint32_t>(total_moves_, 0)});

        int winner = (match.game().state() == core::GameState::WIN) ? match.game().winner() : 0;

        // Aggregate outcomes and move visit distributions
        for (const auto& rec : game_records) {
            auto [it, inserted] = state_table_.emplace(rec.board_state, StateStats(num_players_, total_moves_));
            it->second.outcomes[static_cast<std::size_t>(winner)]++;

            for (int m = 0; m < total_moves_; ++m) {
                it->second.move_visits[static_cast<std::size_t>(m)] += rec.visit_counts[static_cast<std::size_t>(m)];
            }
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

        std::cout << "[Game " << i << "/" << num_games << "] Finished in "
                  << duration << "s. Winner: " << (winner == 0 ? "Draw" : std::to_string(winner))
                  << " | Total Unique States known: " << state_table_.size() << "\n";

        if (i % 10 == 0 || i == num_games) {
            save_data();
        }
    }
    std::cout << "\nTraining complete.\n";
}

void Trainer::load_data() {
    std::ifstream in(filename_);
    if (!in.is_open()) return;

    std::string state;
    while (in >> state) {
        StateStats stats(num_players_, total_moves_);
        for (int i = 0; i <= num_players_; ++i) {
            in >> stats.outcomes[static_cast<std::size_t>(i)];
        }
        for (int i = 0; i < total_moves_; ++i) {
            in >> stats.move_visits[static_cast<std::size_t>(i)];
        }
        state_table_.insert_or_assign(state, stats);
    }
    std::cout << "Loaded " << state_table_.size() << " historical states from " << filename_ << "\n";
}

void Trainer::save_data() const {
    std::ofstream out(filename_, std::ios::trunc);
    for (const auto& [state, stats] : state_table_) {
        out << state;
        for (uint32_t val : stats.outcomes) {
            out << " " << val;
        }
        for (uint32_t count : stats.move_visits) {
            out << " " << count;
        }
        out << "\n";
    }
}

} // namespace mnk::ai
