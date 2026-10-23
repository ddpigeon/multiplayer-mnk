#include "core/match.h"
#include <stdexcept>

namespace mnk::core {

Match::Match(int rows, int cols, int win_length, int num_players)
    : game_(rows, cols, win_length, num_players),
      seats_(static_cast<std::size_t>(num_players) + 1, nullptr) {}

void Match::register_seat(int player_id, IPlayer* player) {
    if (player_id < 1 || static_cast<std::size_t>(player_id) >= seats_.size()) {
        throw std::invalid_argument("register_seat: player id out of range.");
    }
    if (player == nullptr) {
        throw std::invalid_argument("register_seat: player must not be null.");
    }
    if (seats_[static_cast<std::size_t>(player_id)] != nullptr) {
        throw std::invalid_argument("register_seat: seat already occupied.");
    }
    seats_[static_cast<std::size_t>(player_id)] = player;
}

StepResult Match::step() {
    if (game_.state() != GameState::IN_PROGRESS) {
        return StepResult::None;
    }

    IPlayer* player = seats_[game_.current_player()];
    if (player == nullptr) {
        return StepResult::None; // seat not filled yet; wait
    }

    const auto move = player->get_move(game_);
    if (!move.has_value()) {
        return StepResult::None; // player not ready yet
    }

    return game_.play_move(move->first, move->second)
               ? StepResult::Played
               : StepResult::Illegal;
}

bool Match::undo() {
    return game_.undo_move();
}

void Match::restart() {
    game_.restart();
}

const Game& Match::game() const noexcept {
    return game_;
}

} // namespace mnk::core
