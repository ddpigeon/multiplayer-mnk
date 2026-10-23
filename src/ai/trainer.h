#ifndef MNK_AI_TRAINER_H
#define MNK_AI_TRAINER_H

#include "core/game.h"
#include "core/player.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace mnk::ai {

struct StateStats {
    std::vector<uint32_t> outcomes; // Index 0 = Draw, Index 1 = P1 Win, etc.
    std::vector<uint32_t> move_visits; // Size = rows * cols
    StateStats(int num_players, int total_moves) : outcomes(num_players + 1, 0), move_visits(total_moves, 0) {}
};

class Trainer {
public:
    Trainer(int rows, int cols, int win_length, int num_players, std::vector<core::IPlayer*> bots);
    void train(int num_games);

private:
    std::string board_to_string(const core::Board& board) const;
    void load_data();
    void save_data() const;

    int rows_, cols_, win_length_, num_players_;
    int total_moves_;
    std::vector<core::IPlayer*> bots_;
    std::unordered_map<std::string, StateStats> state_table_;
    std::string filename_;
};

} // namespace mnk::ai

#endif // MNK_AI_TRAINER_H
