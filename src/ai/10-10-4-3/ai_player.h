#ifndef MNK_AI_10_10_4_3_PLAYER_H
#define MNK_AI_10_10_4_3_PLAYER_H

#include "core/player.h"
#include "ai/mcts.h"

namespace mnk::ai::variant_10_10_4_3 {

class AIPlayer : public core::IPlayer {
public:
    // Initialize MCTS with params tuned for a 10x10 board
    AIPlayer(int iterations = 25000, float exploration_constant = 1.414f);

    [[nodiscard]] std::optional<std::pair<int, int>> get_move(const core::Game& game) override;

private:
    mnk::ai::MCTS mcts_;
};

} // namespace mnk::ai::variant_10_10_4_3

#endif // MNK_AI_10_10_4_3_PLAYER_H
