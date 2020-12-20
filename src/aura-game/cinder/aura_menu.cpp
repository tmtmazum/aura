#include "aura_menu.h"
#include "visual_info.h"
#include <cinder/Text.h>
#include <cinder/Font.h>

namespace aura
{

void aura_menu::setup() noexcept
{


}

void aura_menu::draw(visual_info& info) const noexcept
{
    {
        ci::gl::ScopedModelMatrix mat{};
        ci::gl::scale(1.9f, 1.9f, 1.0f);
        info.get_plain2d_shader().draw({0.1f, 0.1f, 0.1f, 0.1f}, {plain2d_shader::prim::rect});
    }

    int i = 0;
    for (auto& child : m_cur_level->m_children)
    {

        i++;
    }

    {
        auto w = 1.0f;
        auto h = 0.04f;
        ci::gl::ScopedModelMatrix mat{};
        ci::gl::translate({0.0f, -1.0f + (h/2)});
        ci::gl::scale(2.0f, 2.0f, 1.0f);
        ci::gl::scale(w, h, 1.0f);
        info.get_plain2d_shader().draw(
            info.is_online() ? ci::ColorAf{0.0f, 1.0f, 0.0f, 0.2f} : ci::ColorAf{1.0f, 0.0f, 0.0f, 0.2f},
            {plain2d_shader::prim::rect}
        );
    }

    info.get_phong_shader().program()->bind();
    ci::TextLayout layout;
    layout.clear(ci::ColorA{0.2f, 0.2f, 0.2f, 0.2f});
    layout.setFont(ci::Font::getDefault());
    layout.setColor({1.0f, 0.0f, 0.0f, 1.0f});
    layout.addLine("Connecting to server");
    auto const texture = ci::gl::Texture2d::create(layout.render(true, true));
    ci::gl::draw(texture, ci::vec2{0.0f, 0.0f});
}

} // namespace aura

