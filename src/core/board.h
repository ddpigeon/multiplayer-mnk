#ifndef MNK_CORE_BOARD_H
#define MNK_CORE_BOARD_H

#include <vector>
#include <cstdint>
#include <cstddef>

namespace mnk::core {

using Player = std::uint8_t;
constexpr Player EMPTY_CELL = 0;

struct Move {
    int row{-1};
    int col{-1};
    Player player{EMPTY_CELL};
};

class Board {
public:
    Board(int rows, int cols, int win_length, int num_players);

    /// Applies a move to the board. Returns false if the move is out-of-bounds or cell occupied.
    bool make_move(Player player, int row, int col);

    /// Reverts the most recent move in O(1) time.
    bool undo_move();

    /// O(k) localized win-check around specified coordinate.
    [[nodiscard]] bool is_win_at(int row, int col) const;
    
    [[nodiscard]] bool is_full() const { return moves_played_ == rows_ * cols_; }
    [[nodiscard]] bool is_valid_move(int row, int col) const;

    [[nodiscard]] Player get_cell(int row, int col) const;
    [[nodiscard]] int rows() const { return rows_; }
    [[nodiscard]] int cols() const { return cols_; }
    [[nodiscard]] int win_length() const { return win_length_; }
    [[nodiscard]] int num_players() const { return num_players_; }
    [[nodiscard]] const std::vector<Move>& history() const { return move_history_; }
    [[nodiscard]] const Move* last_move() const {
        return move_history_.empty() ? nullptr : &move_history_.back();
    }

    void reset();

private:
    [[nodiscard]] inline std::size_t to_index(int row, int col) const noexcept {
        return static_cast<std::size_t>(row * cols_ + col);
    }

    int count_consecutive(int row, int col, int dr, int dc, Player player) const;

    int rows_;
    int cols_;
    int win_length_;
    int num_players_;
    int moves_played_{0};

    std::vector<Player> grid_;
    std::vector<Move> move_history_;
};

} // namespace mnk::core

#endif // MNK_CORE_BOARD_H
