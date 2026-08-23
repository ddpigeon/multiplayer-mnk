#include "core/game.h"
#include "ui/window.h"
#include "ui/human_player.h"
#include "ai/10-10-4-3/ai_player.h"
#include <iostream>

int main() {
    constexpr int ROWS = 10;
    constexpr int COLS = 10;
    constexpr int WIN_LENGTH = 4;
    constexpr int NUM_PLAYERS = 3;

    try {
        // 1. Initialize core game logic
        mnk::core::Game game(ROWS, COLS, WIN_LENGTH, NUM_PLAYERS);
        
        // 2. Initialize Window
        mnk::ui::Window window(1024, 800, "10,10,4,3 Game Engine", game);

        // 3. Create Players
        // Human player callback asks the window if a click happened this frame
        mnk::ui::HumanPlayer human([&window]() { 
            return window.get_grid_click(); 
        });
        
        mnk::ai::variant_10_10_4_3::AIPlayer bot(250000);
        mnk::ai::variant_10_10_4_3::AIPlayer bot2(250000);
        mnk::ai::variant_10_10_4_3::AIPlayer bot3(250000);

        // 4. Assign players to their turn slots
        window.register_player(1, &bot); // Player 1 is Human
        window.register_player(2, &bot2);   // Player 2 is AI
        window.register_player(3, &bot3);  // Player 3 is AI

        // 5. Start the engine
        window.run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
