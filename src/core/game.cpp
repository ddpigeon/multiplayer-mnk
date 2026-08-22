#include "game.h"

namespace mnk::core {

    Game::Game(int rows, int cols, int win_length, int num_players)
        : board_(rows, cols, win_length, num_players) {}

    bool Game::play_move(int row, int col) {
        if (state_ != GameState::IN_PROGRESS) {
            return false;
        }

        Player active = current_player_;
        if (!board_.make_move(active, row, col)) {
            return false;
        }

        if (board_.is_win_at(row, col)) {
            state_ = GameState::WIN;
            winner_ = active;
        } else if (board_.is_full()) {
            state_ = GameState::DRAW;
            winner_ = EMPTY_CELL;
        } else {
            advance_turn();
        }

        return true;
    }

    bool Game::undo_move() {
        if (board_.history().empty()) {
            return false;
        }

        // Capture the state before we alter it
        GameState old_state = state_;

        if (!board_.undo_move()) {
            return false;
        }

        state_ = GameState::IN_PROGRESS;
        winner_ = EMPTY_CELL;

        // Only regress the turn if the game was still active.
        // If the last move triggered a WIN or DRAW, the turn never advanced!
        if (old_state == GameState::IN_PROGRESS) {
            regress_turn();
        }

        return true;
    }

    void Game::advance_turn() {
        current_player_ = static_cast<Player>((current_player_ % board_.num_players()) + 1);
    }

    void Game::regress_turn() {
        if (current_player_ == 1) {
            current_player_ = static_cast<Player>(board_.num_players());
        } else {
            current_player_ = static_cast<Player>(current_player_ - 1);
        }
    }

    void Game::restart() {
        board_.reset();
        current_player_ = 1;
        winner_ = EMPTY_CELL;
        state_ = GameState::IN_PROGRESS;
    }

} // namespace mnk::core
