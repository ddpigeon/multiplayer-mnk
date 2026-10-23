#ifndef MNK_AI_PLAYER_INTERFACE_H
#define MNK_AI_PLAYER_INTERFACE_H

#include "core/player.h"
#include <vector>
#include <cstdint>

namespace mnk::ai {

class IAIPlayer : public core::IPlayer {
public:
    virtual ~IAIPlayer() = default;
    [[nodiscard]] virtual std::vector<uint32_t> get_last_visit_counts() const = 0;
};

} // namespace mnk::ai

#endif // MNK_AI_PLAYER_INTERFACE_H
