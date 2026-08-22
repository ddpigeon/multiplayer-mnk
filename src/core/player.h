#ifndef MNK_CORE_PLAYER_H
#define MNK_CORE_PLAYER_H

#include "core/game.h"
#include <optional>
#include <utility>

namespace mnk::core {

class IPlayer {
public:
    virtual ~IPlayer() = default;

    /// Returns a move (row, col) if the player is ready, otherwise std::nullopt.
    [[nodiscard]] virtual std::optional<std::pair<int, int>> get_move(const Game& game) = 0;
};

} // namespace mnk::core

#endif // MNK_CORE_PLAYER_H
