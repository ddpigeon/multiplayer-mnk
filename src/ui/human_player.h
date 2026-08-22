#ifndef MNK_UI_HUMAN_PLAYER_H
#define MNK_UI_HUMAN_PLAYER_H

#include "core/player.h"
#include <functional>

namespace mnk::ui {

class HumanPlayer : public core::IPlayer {
public:
    // Takes a callback that polls the UI for a valid click.
    explicit HumanPlayer(std::function<std::optional<std::pair<int, int>>()> input_callback)
        : input_callback_(std::move(input_callback)) {}

    [[nodiscard]] std::optional<std::pair<int, int>> get_move(const core::Game& /*game*/) override {
        // The human doesn't strictly need the Game object to generate a move, 
        // they just need their mouse click evaluated.
        return input_callback_();
    }

private:
    std::function<std::optional<std::pair<int, int>>()> input_callback_;
};

} // namespace mnk::ui

#endif // MNK_UI_HUMAN_PLAYER_H
