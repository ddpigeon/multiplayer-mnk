#include "window.h"
#include <raymath.h>
#include <format>

namespace mnk::ui {

Window::Window(int width, int height, const std::string& title, core::Match& match)
    : screen_width_(width), screen_height_(height), match_(match) {

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(screen_width_, screen_height_, title.c_str());
    SetTargetFPS(60);

    // Pre-calculate high-contrast colors for N players using HSV spacing
    const int num_p = match_.game().board().num_players();
    player_colors_.reserve(num_p + 1);
    player_colors_.push_back(BLANK); // Index 0 unused (EMPTY_CELL)

    for (int i = 0; i < num_p; ++i) {
        const float hue = (static_cast<float>(i) * 360.0f) / static_cast<float>(num_p);
        player_colors_.push_back(ColorFromHSV(hue, 0.75f, 0.90f));
    }

    metrics_ = InputHandler::compute_metrics(
        match_.game().board().rows(),
        match_.game().board().cols(),
        static_cast<float>(screen_width_),
        static_cast<float>(screen_height_),
        TOP_BAR_HEIGHT
    );
    }

Window::~Window() {
    CloseWindow();
}

std::optional<std::pair<int, int>> Window::get_grid_click() const {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return InputHandler::screen_to_grid(GetMousePosition(), metrics_, match_.game().board().rows(), match_.game().board().cols());
    }
    return std::nullopt;
}

Color Window::get_player_color(core::Player p) const noexcept {
    if (p > 0 && static_cast<std::size_t>(p) < player_colors_.size()) {
        return player_colors_[p];
    }
    return WHITE;
}

void Window::run() {
    while (!WindowShouldClose()) {
        update();
        render();
    }
}

void Window::update() {
    // Recalculate metrics on window resize
    if (IsWindowResized()) {
        screen_width_ = GetScreenWidth();
        screen_height_ = GetScreenHeight();
        metrics_ = InputHandler::compute_metrics(
                match_.game().board().rows(),
                match_.game().board().cols(),
                static_cast<float>(screen_width_),
                static_cast<float>(screen_height_),
                TOP_BAR_HEIGHT
                );
    }

    // Keyboard shortcuts go through the Match: the Window never touches
    // the Game directly
    if (IsKeyPressed(KEY_R)) {
        match_.restart();
    }
    if (IsKeyPressed(KEY_U) || (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z))) {
        match_.undo();
    }

    // One turn step per frame; not-ready players (e.g. waiting for a human
    // click) simply step as None and re-poll next frame
    match_.step();
}

void Window::render() {
    BeginDrawing();
    ClearBackground(Color{24, 28, 36, 255}); // Dark slate aesthetic

    draw_top_bar();
    draw_board();

    EndDrawing();
}

void Window::draw_top_bar() {
    DrawRectangle(0, 0, screen_width_, static_cast<int>(TOP_BAR_HEIGHT), Color{18, 20, 26, 255});
    DrawLine(0, static_cast<int>(TOP_BAR_HEIGHT), screen_width_, static_cast<int>(TOP_BAR_HEIGHT), Color{45, 52, 64, 255});

    const core::Game& game = match_.game();

    std::string status_text;
    Color status_color = RAYWHITE;

    switch (game.state()) {
        case core::GameState::IN_PROGRESS: {
                                               const core::Player p = game.current_player();
                                               status_text = std::format("Player {}'s Turn", p);
                                               status_color = get_player_color(p);
                                               break;
                                           }
        case core::GameState::WIN: {
                                       const core::Player p = game.winner();
                                       status_text = std::format("PLAYER {} WINS!", p);
                                       status_color = get_player_color(p);
                                       break;
                                   }
        case core::GameState::DRAW: {
                                        status_text = "GAME DRAW!";
                                        status_color = YELLOW;
                                        break;
                                    }
    }

    DrawText(status_text.c_str(), 20, 18, 28, status_color);

    const char* controls_text = "[R] Restart  |  [U / Ctrl+Z] Undo";
    const int font_size = 16;
    const int text_w = MeasureText(controls_text, font_size);
    DrawText(controls_text, screen_width_ - text_w - 20, 26, font_size, GRAY);
}

void Window::draw_board() {
    const core::Game& game = match_.game();
    const auto& board = game.board();
    const int rows = board.rows();
    const int cols = board.cols();

    // Board background
    Rectangle board_rect{
        metrics_.start_x, metrics_.start_y, metrics_.width, metrics_.height
    };
    DrawRectangleRec(board_rect, Color{32, 38, 48, 255});

    // Hover Highlight
    if (game.state() == core::GameState::IN_PROGRESS) {
        const auto hover = InputHandler::screen_to_grid(
                GetMousePosition(), metrics_, rows, cols);

        if (hover.has_value() && board.get_cell(hover->first, hover->second) == core::EMPTY_CELL) {
            Rectangle hover_rect{
                metrics_.start_x + (hover->second * metrics_.cell_size),
                    metrics_.start_y + (hover->first * metrics_.cell_size),
                    metrics_.cell_size, metrics_.cell_size
            };
            DrawRectangleRec(hover_rect, Color{255, 255, 255, 20});
        }
    }

    // Grid lines
    const Color grid_color{60, 70, 85, 255};
    for (int i = 0; i <= cols; ++i) {
        const float x = metrics_.start_x + (i * metrics_.cell_size);
        DrawLineEx({x, metrics_.start_y}, {x, metrics_.start_y + metrics_.height}, 2.0f, grid_color);
    }
    for (int i = 0; i <= rows; ++i) {
        const float y = metrics_.start_y + (i * metrics_.cell_size);
        DrawLineEx({metrics_.start_x, y}, {metrics_.start_x + metrics_.width, y}, 2.0f, grid_color);
    }

    // Cell tokens
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const core::Player p = board.get_cell(r, c);
            if (p != core::EMPTY_CELL) {
                draw_cell_content(r, c, p);
            }
        }
    }

    // Highlight Last Move
    if (const auto* last = board.last_move()) {
        const float center_x = metrics_.start_x + (last->col + 0.5f) * metrics_.cell_size;
        const float center_y = metrics_.start_y + (last->row + 0.5f) * metrics_.cell_size;
        DrawCircleLines(static_cast<int>(center_x), static_cast<int>(center_y), metrics_.cell_size * 0.38f, WHITE);
    }
}

void Window::draw_cell_content(int row, int col, core::Player player) {
    const float center_x = metrics_.start_x + (col + 0.5f) * metrics_.cell_size;
    const float center_y = metrics_.start_y + (row + 0.5f) * metrics_.cell_size;
    const float radius = metrics_.cell_size * 0.35f;
    const Color color = get_player_color(player);

    // Procedural Ring with Player Number text
    DrawCircleLines(static_cast<int>(center_x), static_cast<int>(center_y), radius, color);

    std::string p_str = std::to_string(player);
    int font_size = static_cast<int>(metrics_.cell_size * 0.4f);
    int text_w = MeasureText(p_str.c_str(), font_size);
    DrawText(p_str.c_str(),
            static_cast<int>(center_x - text_w / 2.0f),
            static_cast<int>(center_y - font_size / 2.0f),
            font_size, color);
}

} // namespace mnk::ui
