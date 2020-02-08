#pragma once

#include <cinder/Color.h>

namespace aura
{

struct display_constants
{
  float card_texture_width = 90;
  float card_texture_height = 60;

  float tile_icon_width = card_texture_height / 4;
  float tile_icon_height = card_texture_height / 4;
  float tile_icon_font_point = 20;

  float card_board_width = 120;
  float card_board_height = 60;
  float card_hand_width = 240;
  float card_hand_height = 120;

  float terrain_icon_width = std::min(card_board_width, card_board_height) * 0.5f;
  float terrain_icon_height = std::min(card_board_width, card_board_height) * 0.5f;

  float hand_horizontal_padding = 5;
  float hand_vertical_padding = 4;

  float board_horizontal_padding = 20;
  float board_vertical_padding = 4;

  float board_lane_marker_height = 20;

  float card_font_point = 18.0;

  float highlight_descr_width = 240.0f;
  float highlight_descr_height = 360.0f;

  float card_icon_width = 40.0f;
  float card_icon_height = 40.0f;

  float pick_modal_height = 400.0f;
  //float highlight_descr_width = card_board_width * 2.0f;
  //float highlight_descr_height = card_board_height * 4.0f;

  float window_width{};
  float window_height{};

  float mouse_x{};
  float mouse_y{};

  ci::ColorAf hand_card_color{0.1, 0.1, 0.1, 1.0};
  ci::ColorAf hand_hovered_color{0.15f, 0.15f, 0.15f, 0.4f};
  ci::ColorAf hand_selected_color{0.1, 0.4, 0.1, 1.0};
};

} // namespace aura
