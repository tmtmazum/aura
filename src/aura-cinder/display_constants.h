#pragma once

#include <cinder/Color.h>

namespace aura
{

struct display_constants
{
  float card_board_width = 120;
  float card_board_height = 70;
  float card_hand_width = card_board_width;
  float card_hand_height = card_board_height;

  float hand_horizontal_padding = 5;
  float hand_vertical_padding = 4;

  float board_horizontal_padding = 20;
  float board_vertical_padding = 4;

  float board_lane_marker_height = 20;

  float card_font_point = 18.0;

  float highlight_descr_width = card_board_width * 3.0f;
  float highlight_descr_height = card_board_height * 4.0f;

  float window_width{};
  float window_height{};

  float mouse_x{};
  float mouse_y{};

  ci::ColorAf hand_card_color{0.1, 0.1, 0.1, 1.0};
  ci::ColorAf hand_hovered_color{0.15, 0.15, 0.15, 1.0};
  ci::ColorAf hand_selected_color{0.1, 0.4, 0.1, 1.0};
};

} // namespace aura
