#ifndef MNK_CORE_GAME_H
#define MNK_CORE_GAME_H

#include "board.h"

namespace mnk::core {

enum class GameState {
    IN_PROGRESS,
    WIN,
    DRAW
};

class Game {
public:
    Game(int rows, int cols, int win_length, int num_players);

    bool play_move(int row, int col);
    bool undo_move();

    [[nodiscard]] Player current_player() const { return current_player_; }
    [[nodiscard]] Player winner() const { return winner_; }
    [[nodiscard]] GameState state() const { return state_; }
    [[nodiscard]] const Board& board() const { return board_; }

    void restart();

private:
    void advance_turn();
    void regress_turn();

    Board board_;
    Player current_player_{1};
    Player winner_{EMPTY_CELL};
    GameState state_{GameState::IN_PROGRESS};
};

} // namespace mnk::core

#endif // MNK_CORE_GAME_H
