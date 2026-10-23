#include "mcts.h"
#include <cmath>
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace mnk::ai {

MCTS::MCTS(int iterations, float exploration_constant)
    : iterations_(iterations), c_param_(exploration_constant) {
    std::random_device rd;
    rng_.seed(rd());
}

std::optional<uint16_t> MCTS::find_critical_move(const core::Game& game, int depth) const {
    core::Board temp = game.board();
    const int cols = temp.cols();
    const int rows = temp.rows();
    const core::Player me = game.current_player();
    const int num_players = temp.num_players();

    auto to_1d = [cols](int r, int c) { return static_cast<uint16_t>(r * cols + c); };

    // 1. Can I win right now?
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (temp.is_valid_move(r, c)) {
                temp.make_move(me, r, c);
                if (temp.is_win_at(r, c)) return to_1d(r, c);
                temp.undo_move();
            }
        }
    }

    // 2. Can ANY opponent win? (Strict Block - NO bystander skipping)
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (temp.is_valid_move(r, c)) {
                for (core::Player opp = 1; opp <= num_players; ++opp) {
                    if (opp == me) continue;
                    temp.make_move(opp, r, c);
                    if (temp.is_win_at(r, c)) return to_1d(r, c);
                    temp.undo_move();
                }
            }
        }
    }

    if (depth < 2) return std::nullopt;

    // 3. Forks (Open threats)
    auto count_winning_threats = [&](core::Player p) -> int {
        int threats = 0;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (temp.is_valid_move(r, c)) {
                    temp.make_move(p, r, c);
                    if (temp.is_win_at(r, c)) threats++;
                    temp.undo_move();
                    if (threats >= 2) return 2; 
                }
            }
        }
        return threats;
    };

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (temp.is_valid_move(r, c)) {
                temp.make_move(me, r, c);
                if (count_winning_threats(me) >= 2) return to_1d(r, c);
                temp.undo_move();
            }
        }
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (temp.is_valid_move(r, c)) {
                for (core::Player opp = 1; opp <= num_players; ++opp) {
                    if (opp == me) continue;
                    temp.make_move(opp, r, c);
                    if (count_winning_threats(opp) >= 2) return to_1d(r, c);
                    temp.undo_move();
                }
            }
        }
    }

    return std::nullopt;
}

int MCTS::find_best_move(const core::Game& current_game) {
    const int total_cells = current_game.board().rows() * current_game.board().cols();
    last_visit_counts_.assign(total_cells, 0);
    if (current_game.board().num_players() > MAX_MCTS_PLAYERS) {
        throw std::invalid_argument("Player count exceeds MAX_MCTS_PLAYERS.");
    }

    auto critical_move = find_critical_move(current_game, 2);
    if (critical_move.has_value()) {
        // all move visits are this move
        last_visit_counts_[critical_move.value()] = static_cast<uint32_t>(iterations_);
        return critical_move.value();
    }

    tree_.clear();
    tree_.reserve(iterations_ + 1);
    tree_.push_back(Node{}); 

    core::Player root_player = current_game.current_player();

    for (int i = 0; i < iterations_; ++i) {
        core::Game sim_game = current_game;
        int curr_node = 0;

        while (is_fully_expanded(curr_node, sim_game) && sim_game.state() == core::GameState::IN_PROGRESS) {
            curr_node = select_node(curr_node, sim_game, root_player);
        }

        if (sim_game.state() == core::GameState::IN_PROGRESS) {
            curr_node = expand_node(curr_node, sim_game);
        }

        std::vector<float> rewards = simulate(sim_game);
        backpropagate(curr_node, rewards);
    }

    int best_child = -1;
    uint32_t max_visits = 0;
    int child = tree_[0].first_child_idx;
    
    while (child != -1) {
        uint16_t move = tree_[child].move_played;
        if (move < total_cells) {
            last_visit_counts_[move] = tree_[child].visits;
        }
        if (tree_[child].visits > max_visits) {
            max_visits = tree_[child].visits;
            best_child = child;
        }
        child = tree_[child].next_sibling_idx;
    }

    return best_child != -1 ? tree_[best_child].move_played : -1;
}

bool MCTS::is_fully_expanded(int node_idx, const core::Game& game) const {
    const int total_cells = game.board().rows() * game.board().cols();
    const int valid_moves = total_cells - game.board().history().size();
    return tree_[node_idx].num_children == valid_moves;
}

int MCTS::select_node(int curr_node, core::Game& sim_game, core::Player root_player) const {
    int best_child = -1;
    float best_uct = -std::numeric_limits<float>::infinity();
    
    const uint32_t parent_visits = tree_[curr_node].visits;
    const core::Player turn_player = sim_game.current_player();

    int child = tree_[curr_node].first_child_idx;
    while (child != -1) {
        float uct = calculate_uct(parent_visits, tree_[child], turn_player, root_player);
        if (uct > best_uct) {
            best_uct = uct;
            best_child = child;
        }
        child = tree_[child].next_sibling_idx;
    }

    const int cols = sim_game.board().cols();
    uint16_t move = tree_[best_child].move_played;
    sim_game.play_move(move / cols, move % cols);

    return best_child;
}

// --- NEW: Paranoid MCTS UCT Formula ---
float MCTS::calculate_uct(int parent_visits, const Node& child, core::Player turn_player, core::Player root_player) const {
    if (child.visits == 0) return std::numeric_limits<float>::infinity();

    float q_value;
    if (turn_player == root_player) {
        // It's our turn in the tree. Maximize our win rate.
        q_value = child.scores[root_player] / static_cast<float>(child.visits);
    } else {
        // It's an opponent's turn. 
        // PARANOID ASSUMPTION: They will actively choose the move that minimizes OUR wins.
        q_value = 1.0f - (child.scores[root_player] / static_cast<float>(child.visits));
    }

    const float exploration = c_param_ * std::sqrt(std::log(static_cast<float>(parent_visits)) / static_cast<float>(child.visits));
    
    return q_value + exploration;
}

int MCTS::expand_node(int curr_node, core::Game& sim_game) {
    const auto& board = sim_game.board();
    const int cols = board.cols();
    
    std::vector<uint16_t> unexpanded;
    for (int r = 0; r < board.rows(); ++r) {
        for (int c = 0; c < cols; ++c) {
            if (board.get_cell(r, c) == core::EMPTY_CELL) {
                unexpanded.push_back(static_cast<uint16_t>(r * cols + c));
            }
        }
    }

    int child = tree_[curr_node].first_child_idx;
    while (child != -1) {
        std::erase(unexpanded, tree_[child].move_played);
        child = tree_[child].next_sibling_idx;
    }

    uint16_t chosen_move = 0xFFFF;
    bool found_tactical = false;
    core::Board temp = sim_game.board();
    core::Player me = sim_game.current_player();

    for (uint16_t m : unexpanded) {
        temp.make_move(me, m / cols, m % cols);
        if (temp.is_win_at(m / cols, m % cols)) {
            chosen_move = m;
            found_tactical = true;
            break;
        }
        temp.undo_move();
    }

    if (!found_tactical) {
        for (uint16_t m : unexpanded) {
            for (core::Player opp = 1; opp <= temp.num_players(); ++opp) {
                if (opp == me) continue;
                temp.make_move(opp, m / cols, m % cols);
                if (temp.is_win_at(m / cols, m % cols)) {
                    chosen_move = m;
                    found_tactical = true;
                    break;
                }
                temp.undo_move();
            }
            if (found_tactical) break;
        }
    }

    if (!found_tactical) {
        std::uniform_int_distribution<size_t> dist(0, unexpanded.size() - 1);
        chosen_move = unexpanded[dist(rng_)];
    }

    int new_node_idx = static_cast<int>(tree_.size());
    Node new_node;
    new_node.parent_idx = curr_node;
    new_node.move_played = chosen_move;
    new_node.next_sibling_idx = tree_[curr_node].first_child_idx;
    tree_[curr_node].first_child_idx = new_node_idx;
    tree_[curr_node].num_children++;

    tree_.push_back(new_node);
    sim_game.play_move(chosen_move / cols, chosen_move % cols);
    
    return new_node_idx;
}

// --- RESTORED: Purely random playouts for raw CPU speed ---
std::vector<float> MCTS::simulate(core::Game& sim_game) {
    const auto& board = sim_game.board();
    const int cols = board.cols();
    const int num_players = board.num_players();
    
    std::vector<uint16_t> empty_cells;
    for (int r = 0; r < board.rows(); ++r) {
        for (int c = 0; c < cols; ++c) {
            if (board.get_cell(r, c) == core::EMPTY_CELL) {
                empty_cells.push_back(static_cast<uint16_t>(r * cols + c));
            }
        }
    }

    while (sim_game.state() == core::GameState::IN_PROGRESS && !empty_cells.empty()) {
        std::uniform_int_distribution<size_t> dist(0, empty_cells.size() - 1);
        size_t idx = dist(rng_);
        uint16_t move = empty_cells[idx];
        
        empty_cells[idx] = empty_cells.back();
        empty_cells.pop_back();

        sim_game.play_move(move / cols, move % cols);
    }

    std::vector<float> rewards(num_players + 1, 0.0f);
    if (sim_game.state() == core::GameState::WIN) {
        rewards[sim_game.winner()] = 1.0f;
    } else if (sim_game.state() == core::GameState::DRAW) {
        for (int i = 1; i <= num_players; ++i) rewards[i] = 0.5f;
    }

    return rewards;
}

void MCTS::backpropagate(int node_idx, const std::vector<float>& rewards) {
    int curr = node_idx;
    while (curr != -1) {
        Node& n = tree_[curr];
        n.visits++;
        for (size_t i = 1; i < rewards.size(); ++i) {
            n.scores[i] += rewards[i];
        }
        curr = n.parent_idx;
    }
}

} // namespace mnk::ai
