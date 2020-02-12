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
    for (auto scale_factor = 1.5f; scale_factor > 0.1f; scale_factor = scale_factor - 0.02f)
    {
      auto full_font = ci::Font{"Cambria", orig_rect.getHeight() * scale_factor};
      auto const bb = full_font.getGlyphBoundingBox(full_font.getGlyphChar('A'));
      auto width_required = bb.getWidth() * text.size();
      auto height_required = bb.getHeight();
      if (width_required <= width_available && height_required <= height_available)
      {
        return full_font;
      }
    }
    return ci::Font{"Cambria", orig_rect.getHeight() * 0.1f};
  });

  //auto const font_size = [&]()
  //{
  //  if (text.size() == 1)
  //  {
  //    return rect.getHeight() * 1.5f;
  //  }
  //  if (text.size() == 2)
  //  {
  //    return rect.getHeight() * 1.4f;
  //  }
  //  if (text.size() == 4)
  //  {
  //    return rect.getHeight() * 1.3f;
  //  }
  //  if (text.size() == 5)
  //  {
  //    return rect.getHeight() * 1.2f;
  //  }
  //  else
  //    return rect.getHeight();
  //}();

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

  auto const num_lines = [&]()
  {
    if (text.size() < 8)
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

  if (text.empty())
  {
    return;
  }

  auto en_rect = rect;
  en_rect.inflate({-40.0f, -40.0f});
  
  auto const [width_available, height_available] = std::pair{en_rect.getWidth(), en_rect.getHeight()};

  auto const final_font = std::invoke([&]
  {
    //for (auto scale_factor = 1.5f; scale_factor > 0.1f; scale_factor = scale_factor - 0.02f)
    for (auto font_size = 40.0f; font_size > 1.0f; font_size -= 1.0f)
    {
      auto full_font = ci::Font{"Cambria", font_size};
      auto const bb = full_font.getGlyphBoundingBox(full_font.getGlyphChar(text.at(0)));
      auto height_per_line = bb.getHeight();
      auto const num_lines_possible = std::floor(height_available / height_per_line);
      auto const num_chars_possible = num_lines_possible * std::floor(width_available / (bb.getWidth()));

      if (num_chars_possible > text.size())
      {
        AURA_LOG(L"Text '%hs', size %zu, chose %.2f lines and %.2f chars per line", text.c_str(), text.size(),
          num_lines_possible, (width_available / bb.getWidth()));
        return full_font;
      }
    }
    return ci::Font{"Cambria", 1.0f};
  });

  box.font(final_font);
  //box.font(ci::Font{"Cambria", (rect.getHeight() / num_lines) * 0.8f});
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
