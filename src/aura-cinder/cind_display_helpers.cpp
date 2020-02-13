#include "cind_display_helpers.h"
#include <cinder/gl/gl.h>
#include <aura-core/build.h>

namespace aura
{

void draw(ci::Rectf const& rect, ci::gl::Texture2dRef texture)
{
  ci::gl::draw(texture, rect);
}

void draw_line(ci::Rectf const& orig_rect, std::string const& text, bool center)
{
  ci::TextBox box;

  auto rect = orig_rect;
  rect.inflate({20.0f, 0.0f});
  auto const [width_available, height_available] = std::pair{orig_rect.getWidth(), orig_rect.getHeight()};

  auto const final_font = std::invoke([&]
  {
    if (text.size() < 2)
    {
      return ci::Font{"Cambria", orig_rect.getHeight() * 1.5f};
    }

    else if (text.size() < 4)
    {
      return ci::Font{"Cambria", orig_rect.getHeight() * 1.0f};
    }
    return ci::Font{"Cambria", orig_rect.getHeight() * 1.0f};
  });

  box.font(final_font);
  //box.font(ci::Font{"Cambria", font_size});
  box.text(text);
  box.size(rect.getSize());
  box.color({0.1f, 0.1f, 0.1f, 1.0f});

  if (center)
  {
    box.setAlignment(ci::TextBox::Alignment::CENTER);
  }

  {
    auto const surf = box.render();
    auto const textr = ci::gl::Texture::create(surf);
    //e_rect.inflate({2.0f, 0.0f});
    ci::gl::draw(textr, rect);
  }
}

void draw_multiline(ci::Rectf const& rect, std::string const& text, bool center)
{
  ci::TextBox box;

  if (text.empty())
  {
    return;
  }

  auto en_rect = rect;
  en_rect.inflate({-40.0f, -40.0f});
  
  auto const [width_available, height_available] = std::pair{en_rect.getWidth(), en_rect.getHeight()};

  auto const final_font = ci::Font{"Cambria", 20.0f};
  box.font(final_font);
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
