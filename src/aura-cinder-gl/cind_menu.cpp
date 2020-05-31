#include "cind_menu.h"
#include <aura-core/display_dag.h>
#include <aura-cinder-gl/cind_display_engine.h>

namespace aura
{


//! can be optimized : cache the form bounds and update lambdas only when screen bounds change
static auto get_form_bounds(cind_display_context_t const& app, cind_menu_style const& style)
{
    auto const area = app->getWindowBounds();

    auto const mid_x = ((float)area.getX1() + area.getX2()) / 2.0f;
    auto const mid_y = ((float)area.getY1() + area.getY2()) / 2.0f;

    ci::Rectf form_bounds{mid_x - (style.form_width / 2.0f),
                          mid_y - (style.form_height / 2.0f), 0.0f, 0.0f};
    form_bounds.x2 = form_bounds.x1 + style.form_width;
    form_bounds.y2 = form_bounds.y1 + style.form_height;
    form_bounds.canonicalize();
    return form_bounds;
}

static auto get_button_bounds(ci::Rectf const& area, cind_menu_style const& style, int i)
{
    ci::Rectf button_bounds{
        (float)area.getX1() + style.x_padding,
        (float)area.getY1() + style.y_padding + i*(style.y_padding + style.button_height),
        0.0f, 0.0f
    };
    button_bounds.x2 = button_bounds.x1 + style.button_width;
    button_bounds.y2 = button_bounds.y1 + style.button_height;

    return button_bounds;
}

void cind_menu::setup_form()
{
    AURA_ASSERT(m_style->form_alignment == cind_menu_style::form_alignment_t::center_center);

    m_node->set_render_fn<cind_display_context_t>(
        [style=m_style](cind_display_context_t app, dag_node& this_node)
        {
            auto const form_bounds = get_form_bounds(app, *style);
            ci::gl::ScopedColor form_border_color{1.0f, 1.0f, 1.0f, 1.0f};
            ci::gl::drawStrokedRect(form_bounds, 5.0f);
        });
}

void cind_menu::add_button_internal(std::string text, action_internal_t action)
{
    auto button_node = std::make_shared<dag_node>(dag_node_s{});
    auto const highlighted_node = std::make_shared<dag_node>(dag_node_s{});

    auto const button_index = m_num_buttons++;

    button_node->set_render_fn<cind_display_context_t>(
    [style=m_style, i=button_index, txt=text](cind_display_context_t app, dag_node& this_node)
    {
        auto const area = get_form_bounds(app, *style);

        auto const button_bounds = get_button_bounds(area, *style, i);

        {
            ci::gl::ScopedColor background_color{0.1f, 0.1f, 0.1f, 1.0f};
            ci::gl::drawStrokedRect(button_bounds, 5.0f);
        }

        ci::Font f{"Times New Roman", 48.0f};
        ci::gl::drawString(txt, {button_bounds.getX1(), button_bounds.getY1()}, {1.0f, 1.0f, 1.0f, 1.0f}, f);
    });

    highlighted_node->set_render_fn<cind_display_context_t>(
        [style = m_style, i=button_index, txt = text](cind_display_context_t app, dag_node& this_node) {
            using namespace std::chrono_literals;
            auto const area = get_form_bounds(app, *style);

            auto const alpha = this_node.transition_elapsed(
                1500ms, transition_function::ease_out_cubic);

            auto const button_bounds = get_button_bounds(area, *style, i);

            {
                ci::gl::ScopedColor background_color{0.5f, 0.5f, 0.5f, alpha};
                ci::gl::drawSolidRect(button_bounds);
            }

            {
                ci::gl::ScopedColor background_color{0.1f, 0.1f, 0.1f, 1.0f};
                ci::gl::drawStrokedRect(button_bounds, 5.0f);
            }

            ci::Font f{"Times New Roman", 48.0f};
            ci::gl::drawString(txt,
                               {button_bounds.getX1(), button_bounds.getY1()},
                               {1.0f, 1.0f, 1.0f, 1.0f}, f);
        });

    button_node->add_transition<cind_display_context_t>(
        [style=m_style, i=button_index, txt=text](cind_display_context_t app, dag_node& this_node)
        {
            auto const area = get_form_bounds(app, *style);

            auto const button_bounds = get_button_bounds(area, *style, i);

            auto const mouse_pos = app->getMousePos() - app->getWindowPos();
            auto const coord = ci::gl::windowToObjectCoord(mouse_pos);

            bool is_hovered = button_bounds.canonicalized().contains(ci::vec2{coord});
            return is_hovered;
        }, highlighted_node);

    highlighted_node->add_transition<cind_display_context_t>(
        [style=m_style, i=button_index, txt=text](cind_display_context_t app, dag_node& this_node)
        {
            auto const area = get_form_bounds(app, *style);

            auto const button_bounds = get_button_bounds(area, *style, i);

            auto const mouse_pos = app->getMousePos() - app->getWindowPos();
            auto const coord = ci::gl::windowToObjectCoord(mouse_pos);

            bool is_hovered = button_bounds.canonicalized().contains(ci::vec2{coord});
            return !is_hovered;
        }, button_node);

    m_node->add_child(std::move(button_node));
}

void cind_menu::add_close_button_internal(action_internal_t action)
{

}

} // namespace aura
