#include "board.h"
#include <stdexcept>
#include <algorithm>

namespace mnk::core {

    Board::Board(int rows, int cols, int win_length, int num_players)
        : rows_(rows),
        cols_(cols),
        win_length_(win_length),
        num_players_(num_players),
        grid_(rows * cols, EMPTY_CELL) {
            if (rows <= 0 || cols <= 0 || win_length <= 0 || num_players < 2) {
                throw std::invalid_argument("Invalid board dimensions or player configuration.");
            }
            move_history_.reserve(rows * cols);
        }

    bool Board::is_valid_move(int row, int col) const {
        if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
            return false;
        }
        return grid_[to_index(row, col)] == EMPTY_CELL;
    }

    Player Board::get_cell(int row, int col) const {
        if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
            return EMPTY_CELL;
        }
        return grid_[to_index(row, col)];
    }

    bool Board::make_move(Player player, int row, int col) {
        if (player < 1 || player > num_players_ || !is_valid_move(row, col)) {
            return false;
        }

        grid_[to_index(row, col)] = player;
        move_history_.push_back({row, col, player});
        ++moves_played_;
        return true;
    }

    bool Board::undo_move() {
        if (move_history_.empty()) {
            return false;
        }

        const Move last = move_history_.back();
        move_history_.pop_back();

        grid_[to_index(last.row, last.col)] = EMPTY_CELL;
        --moves_played_;
        return true;
    }

    int Board::count_consecutive(int row, int col, int dr, int dc, Player player) const {
        int count = 0;
        int r = row + dr;
        int c = col + dc;

        // Ray-march at most (k - 1) steps outward in target direction
        for (int step = 1; step < win_length_; ++step) {
            if (r < 0 || r >= rows_ || c < 0 || c >= cols_) {
                break;
            }
            if (grid_[to_index(r, c)] != player) {
                break;
            }
            ++count;
            r += dr;
            c += dc;
        }
        return count;
    }

    bool Board::is_win_at(int row, int col) const {
        const Player p = get_cell(row, col);
        if (p == EMPTY_CELL) {
            return false;
        }

        constexpr int directions[4][2] = {
            {0, 1},
            {1, 0},
            {1, 1},
            {1, -1}
        };

        for (const auto& dir : directions) {
            int forward  = count_consecutive(row, col, dir[0], dir[1], p);
            int backward = count_consecutive(row, col, -dir[0], -dir[1], p);

            if (1 + forward + backward >= win_length_) {
                return true;
            }
        }

        return false;
    }

    void Board::reset() {
        std::fill(grid_.begin(), grid_.end(), EMPTY_CELL);
        move_history_.clear();
        moves_played_ = 0;
    }

}
