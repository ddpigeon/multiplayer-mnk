#ifndef MNK_UI_INPUT_H
#define MNK_UI_INPUT_H

#include <raylib.h>
#include <optional>
#include <utility>

namespace mnk::ui {

struct GridMetrics {
    float cell_size{0.0f};
    float start_x{0.0f};
    float start_y{0.0f};
    float width{0.0f};
    float height{0.0f};
};

class InputHandler {
public:
    /// Calculates optimal grid size and positions centered within window bounds.
    [[nodiscard]] static GridMetrics compute_metrics(
        int rows, int cols, float screen_w, float screen_h, float top_margin, float padding = 20.0f) noexcept;

    /// Maps mouse position to zero-indexed (row, col). Returns std::nullopt if outside grid.
    [[nodiscard]] static std::optional<std::pair<int, int>> screen_to_grid(
        Vector2 mouse_pos, const GridMetrics& metrics, int rows, int cols) noexcept;
};

} // namespace mnk::ui

#endif // MNK_UI_INPUT_H
