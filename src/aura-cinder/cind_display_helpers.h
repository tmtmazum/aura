#pragma once

#include <cinder/Rect.h>
#include <cinder/gl/Texture.h>
#include <string>

namespace aura
{

void draw(ci::Rectf const& rect, ci::gl::Texture2dRef texture);
void draw_line(ci::Rectf const& rect, std::string const& text, bool center = true);
void draw_multiline(ci::Rectf const& rect, std::string const& text, bool center = true);

} // namespace aura
