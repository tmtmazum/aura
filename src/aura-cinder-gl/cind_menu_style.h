#pragma once

namespace aura
{

struct cind_menu_style
{
  enum class layout
  {
    vertical_stack,
    horizontal_stack
  };

  //! Padding before and after elements
  float x_padding = 20.0f;

  float y_padding = 20.0f;

  float button_height = 60.0f;

  float button_width = 360.0f;

  enum class button_highlight_style
  {
    color_slide

  };

  float form_height = 600.0f;
  float form_width = 400.0f;

  enum class form_alignment_t
  {
    center_center
  } form_alignment;
};

} // namespace aura
