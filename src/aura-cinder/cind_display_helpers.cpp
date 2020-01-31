#include "cind_display_helpers.h"
#include <cinder/gl/gl.h>

namespace aura
{

void draw(ci::Rectf const& rect, ci::gl::Texture2dRef texture)
{
  ci::gl::draw(texture, rect);
}

void draw_line(ci::Rectf const& rect, std::string const& text, bool center)
{
  ci::TextBox box;

  box.font(ci::Font{"Cambria", (rect.getHeight()) * 1.5f});
  box.text(text);
  box.size(rect.getSize());
  box.color({0.0f, 0.0f, 0.0f, 1.0f});

  if (center)
  {
    box.setAlignment(ci::TextBox::Alignment::CENTER);
  }

  {
    auto const surf = box.render();
    auto const textr = ci::gl::Texture::create(surf);
    ci::gl::draw(textr, rect);
  }
}

void draw_multiline(ci::Rectf const& rect, std::string const& text, bool center)
{
  ci::TextBox box;

  auto const num_lines = [&]()
  {
    if (text.size() < 17)
    {
      return 1;
    }
    else if (text.size() < 34)
    {
      return 2;
    }
    else if (text.size() < 51)
    {
      return 3;
    }
    return 4;
  }();

  box.font(ci::Font{"Cambria", (rect.getHeight() / num_lines) * 1.0f});
  box.text(text);
  box.size(rect.getSize());
  box.color({0.0f, 0.0f, 0.0f, 1.0f});

  if (center)
  {
    box.setAlignment(ci::TextBox::Alignment::CENTER);
  }

  {
    auto const surf = box.render();
    auto const textr = ci::gl::Texture::create(surf);
    ci::gl::draw(textr, rect);
  }
}

} // namespace aura
