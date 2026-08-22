#ifndef MNK_UI_WINDOW_H
#define MNK_UI_WINDOW_H

#include "core/game.h"
#include "core/player.h"
#include "ui/input.h"
#include <raylib.h>
#include <vector>
#include <string>

namespace mnk::ui {

class Window {
public:
    Window(int width, int height, const std::string& title, core::Game& game);
    ~Window();

    // Prevent copy/move to maintain explicit window lifetime
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    // Assigns a player interface to a player number (1, 2, 3...)
    void register_player(int player_id, core::IPlayer* player);
    
    // Allows the HumanPlayer callback to ask the window for the grid coordinate clicked
    [[nodiscard]] std::optional<std::pair<int, int>> get_grid_click() const;

    /// Runs the application render and input loop.
    void run();

private:
    void update();
    void render();

    void draw_top_bar();
    void draw_board();
    void draw_cell_content(int row, int col, core::Player player);
    
    [[nodiscard]] Color get_player_color(core::Player p) const noexcept;

    int screen_width_;
    int screen_height_;
    core::Game& game_;

    GridMetrics metrics_;
    std::vector<core::IPlayer*> players_;
    std::vector<Color> player_colors_;
    
    static constexpr float TOP_BAR_HEIGHT = 70.0f;
};

} // namespace mnk::ui

#endif // MNK_UI_WINDOW_H
