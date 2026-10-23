#ifndef MNK_AI_10_10_4_3_PLAYER_H
#define MNK_AI_10_10_4_3_PLAYER_H

#include "core/player.h"
#include "ai/mcts.h"
#include "ai/ai_player_interface.h"

namespace mnk::ai::variant_10_10_4_3 {

class AIPlayer : public IAIPlayer {
public:
    // Initialize MCTS with params tuned for a 10x10 board
    explicit AIPlayer(int iterations = 50000, float exploration_constant = 1.414f);

    std::optional<std::pair<int, int>> get_move(const core::Game& game) override;
    [[nodiscard]] std::vector<uint32_t> get_last_visit_counts() const override;


private:
    mnk::ai::MCTS mcts_;
};

} // namespace mnk::ai::variant_10_10_4_3

#endif // MNK_AI_10_10_4_3_PLAYER_H
