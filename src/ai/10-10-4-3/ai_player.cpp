#include "ai_player.h"

namespace mnk::ai::variant_10_10_4_3 {

    AIPlayer::AIPlayer(int iterations, float exploration_constant)
        : mcts_(iterations, exploration_constant) {}

    std::vector<uint32_t> AIPlayer::get_last_visit_counts() const {
        return mcts_.get_last_visit_counts();
    }

    std::optional<std::pair<int, int>> AIPlayer::get_move(const core::Game& game) {
        // 1. Ask MCTS to find the best move
        int move_1d = mcts_.find_best_move(game);

        // 2. Handle edge cases (e.g., board is entirely full)
        if (move_1d == -1) {
            return std::nullopt;
        }

        // 3. Convert 1D index back to (row, col)
        int row = move_1d / game.board().cols();
        int col = move_1d % game.board().cols();

        return std::make_pair(row, col);
    }

} // namespace mnk::ai::variant_10_10_4_3
