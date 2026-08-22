#include "input.h"
#include <algorithm>

namespace mnk::ui {

GridMetrics InputHandler::compute_metrics(
    int rows, int cols, float screen_w, float screen_h, float top_margin, float padding) noexcept {
    
    GridMetrics metrics;
    const float avail_w = screen_w - (padding * 2.0f);
    const float avail_h = screen_h - top_margin - (padding * 2.0f);

    if (avail_w <= 0.0f || avail_h <= 0.0f || rows <= 0 || cols <= 0) {
        return metrics;
    }

    const float cell_w = avail_w / static_cast<float>(cols);
    const float cell_h = avail_h / static_cast<float>(rows);

    metrics.cell_size = std::min(cell_w, cell_h);
    metrics.width = metrics.cell_size * static_cast<float>(cols);
    metrics.height = metrics.cell_size * static_cast<float>(rows);

    // Center grid in available canvas area
    metrics.start_x = (screen_w - metrics.width) * 0.5f;
    metrics.start_y = top_margin + ((avail_h - metrics.height) * 0.5f);

    return metrics;
}

std::optional<std::pair<int, int>> InputHandler::screen_to_grid(
    Vector2 mouse_pos, const GridMetrics& metrics, int rows, int cols) noexcept {

    if (metrics.cell_size <= 0.0f) {
        return std::nullopt;
    }

    const float rel_x = mouse_pos.x - metrics.start_x;
    const float rel_y = mouse_pos.y - metrics.start_y;

    if (rel_x < 0.0f || rel_x >= metrics.width || rel_y < 0.0f || rel_y >= metrics.height) {
        return std::nullopt;
    }

    const int col = static_cast<int>(rel_x / metrics.cell_size);
    const int row = static_cast<int>(rel_y / metrics.cell_size);

    if (row >= 0 && row < rows && col >= 0 && col < cols) {
        return std::make_pair(row, col);
    }

    return std::nullopt;
}

} // namespace mnk::ui
