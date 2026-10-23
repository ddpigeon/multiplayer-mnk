#ifndef MNK_CORE_MATCH_H
#define MNK_CORE_MATCH_H

#include "core/game.h"
#include "core/player.h"
#include <vector>

namespace mnk::core {

/// Outcome of one polling step of a Match.
enum class StepResult {
    None,    ///< nothing happened: game over, seat unfilled, or player not ready
    Played,  ///< the seated player's move was legal and applied
    Illegal  ///< the seated player returned a move the Game rejected
};

/// One running Game between seated players.
///
/// The Match owns the turn loop: it asks the seated IPlayer of the current
/// turn for a move and applies it to the Game. Callers drive the Match
/// through step()/undo()/restart() and read state through game(); they never
/// mutate the Game directly. Polling design: step() returns without blocking,
/// so a frame-driven UI (raylib) and a batch driver (self-play training) are
/// both just callers that step in a loop.
class Match {
public:
    Match(int rows, int cols, int win_length, int num_players);

    /// Seats `player` on the 1-based seat `player_id`. Throws when the seat
    /// number is out of range, the player is null, or the seat is taken.
    void register_seat(int player_id, IPlayer* player);

    /// Polls once: if the Seat's player is ready, plays its move.
    /// A human Seat legitimately steps as None every frame until it clicks.
    StepResult step();

    /// Takes back the last move. The turn regress handles the win corner
    /// case (the last move never advanced the turn).
    bool undo();

    void restart();

    /// Read-only view of the underlying Game, for rendering and training.
    [[nodiscard]] const Game& game() const noexcept;

private:
    Game game_;
    // Index 0 unused; seats are 1-based. An internal detail of this module.
    std::vector<IPlayer*> seats_;
};

} // namespace mnk::core

#endif // MNK_CORE_MATCH_H
