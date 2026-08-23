#ifndef MNK_AI_MCTS_H
#define MNK_AI_MCTS_H

#include "core/game.h"
#include <vector>
#include <random>
#include <cstdint>
#include <optional>

namespace mnk::ai {

constexpr int MAX_MCTS_PLAYERS = 8; 

struct Node {
    int parent_idx{-1};
    int first_child_idx{-1};
    int next_sibling_idx{-1};
    
    uint16_t move_played{0xFFFF};
    uint16_t num_children{0};
    
    uint32_t visits{0};
    float scores[MAX_MCTS_PLAYERS + 1]{0.0f}; 
};

class MCTS {
public:
    MCTS(int iterations = 50000, float exploration_constant = 1.414f);

    int find_best_move(const core::Game& current_game);

private:
    // --- UPDATED: Added root_player tracking for Paranoid mode ---
    int select_node(int curr_node, core::Game& sim_game, core::Player root_player) const;
    int expand_node(int curr_node, core::Game& sim_game);
    std::vector<float> simulate(core::Game& sim_game);
    void backpropagate(int node_idx, const std::vector<float>& rewards);
    
    [[nodiscard]] bool is_fully_expanded(int node_idx, const core::Game& game) const;
    [[nodiscard]] float calculate_uct(int parent_visits, const Node& child, core::Player turn_player, core::Player root_player) const;

    [[nodiscard]] std::optional<uint16_t> find_critical_move(const core::Game& game, int depth) const;

    int iterations_;
    float c_param_;
    std::vector<Node> tree_;
    std::mt19937 rng_;
};

} // namespace mnk::ai

#endif // MNK_AI_MCTS_H
