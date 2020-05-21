#include "cind_display_engine.h"
#include <aura-core/display_dag.h>
#include <cinder/gl/scoped.h>
#include <cinder/gl/gl.h>
#include <optional>
#include "dag.h"

namespace aura
{

namespace functions
{

template <typename T>
auto to_float(T t) { return static_cast<float>(t); }

struct rounding_options
{
  float corner_radius = 4.0f;
  int num_segments = 4; //!< num segments per corner
};

enum class hover_animation_t { none, highlight_color, slide, fade };

struct button_opts
{
  std::string text = "button";
  hover_animation_t hover_animation = hover_animation_t::highlight_color;

  std::optional<ci::ColorAf> border_color = ci::ColorAf{1.0f, 1.0f, 1.0f, 1.0f};
  std::optional<ci::ColorAf> passive_color = ci::ColorAf{0.0f, 0.0f, 0.0f, 0.5f};
  std::optional<ci::ColorAf> hover_color = ci::ColorAf{1.0f, 1.0f, 1.0f, 1.0f};
  std::optional<ci::ColorAf> text_color = ci::ColorAf{1.0f, 1.0f, 1.0f, 1.0f};
  std::optional<rounding_options> rounding;
};

#if 0
auto make_button_renderer(button_opts&& o)
{
  return make_render_fn([opts = std::move(o)](cind_display_context_t app)
  {
    auto const mouse_pos = app->getMousePos() - app->getWindowPos();
    auto const coord = ci::gl::windowToObjectCoord(mouse_pos);
    ci::Rectf rect{0.0f, 0.0f, 40.0f, -40.0f};

    AURA_LOG(L"Coord at {%.2f, %.2f} w/ rect {%.2f, %.2f, %.2f, %.2f}", coord[0], coord[1],
      rect.getX1(), rect.getY1(), rect.getX2(), rect.getY2());

    bool is_hovered = rect.canonicalized().contains(ci::vec2{coord});

    if (opts.passive_color)
    {
      ci::gl::ScopedColor col{*opts.passive_color};
      if (opts.rounding)
      {
        ci::gl::drawSolidRoundedRect(rect, opts.rounding->corner_radius, opts.rounding->num_segments);
      }
      else
      {
        ci::gl::drawSolidRect(rect);
      }
    }

    if (!opts.text.empty())
    {
      AURA_ASSERT(opts.text_color);
      ci::gl::drawString(opts.text, {0.0f, -40.0f}, *opts.text_color);
    }

    if (is_hovered)
    {
      if (opts.hover_animation == hover_animation_t::highlight_color)
      {
        AURA_ASSERT(opts.hover_color);
        ci::gl::ScopedColor col{*opts.hover_color};
        if (opts.rounding)
        {
          ci::gl::drawSolidRoundedRect(rect, opts.rounding->corner_radius, opts.rounding->num_segments);
        }
        else
        {
          ci::gl::drawSolidRect(rect);
        }
      }
    }

    if (opts.border_color)
    {
      ci::gl::ScopedColor col{*opts.border_color};
      if (opts.rounding)
      {
        ci::gl::drawStrokedRoundedRect(rect, opts.rounding->corner_radius, opts.rounding->num_segments);
      }
      else
      {
        ci::gl::drawStrokedRect(rect);
      }
    }
  });
}
#endif

shared_dag_t create_ui_layer()
{
  dag_node_s passive{};
  dag_node_s highlighted{};

  auto passive_shared = std::make_shared<dag_node>(passive);
  auto highlighted_shared = std::make_shared<dag_node>(highlighted);

  passive_shared->set_render_fn<cind_display_context_t>([](cind_display_context_t app, dag_node& this_node)
  {
    auto const area = app->getWindowBounds();
    ci::gl::ScopedColor col{1.0f, 0.0f, 0.0f, 1.0f};
    ci::Rectf rect{static_cast<float>(area.getX1()), static_cast<float>(area.getY1()),
      static_cast<float>(area.getX2()), static_cast<float>(area.getY2())};

    ci::gl::drawStrokedRect(rect, 5.0f);
  }); 

  highlighted_shared->set_render_fn<cind_display_context_t>([](cind_display_context_t app, dag_node& this_node)
  {
    using namespace std::chrono;

    auto const area = app->getWindowBounds();
    ci::gl::ScopedColor col{1.0f, 0.0f, 0.0f, 1.0f};

    {
      ci::Rectf rect{static_cast<float>(area.getX1()), static_cast<float>(area.getY1()),
        static_cast<float>(area.getX2()), static_cast<float>(area.getY2())};

      ci::gl::drawStrokedRect(rect, 5.0f);
    }

    auto const max_distance = area.getX2() - area.getX1();
    auto const distance = this_node.transition_elapsed(milliseconds(800)) * max_distance;

    ci::Rectf rect{static_cast<float>(area.getX1()), static_cast<float>(area.getY1()),
      static_cast<float>(area.getX1()) + distance, static_cast<float>(area.getY2())};

    ci::gl::drawSolidRect(rect);
  }); 

  passive_shared->add_transition<cind_display_context_t>([](cind_display_context_t app, dag_node& this_node)
  {
    auto const area = app->getWindowBounds();
    ci::Rectf rect{static_cast<float>(area.getX1()), static_cast<float>(area.getY1()),
      static_cast<float>(area.getX2()), static_cast<float>(area.getY2())};

    auto const mouse_pos = app->getMousePos() - app->getWindowPos();
    auto const coord = ci::gl::windowToObjectCoord(mouse_pos);

    bool is_hovered = rect.canonicalized().contains(ci::vec2{coord});
    return is_hovered;
  }, highlighted_shared);

  highlighted_shared->add_transition<cind_display_context_t>([](cind_display_context_t app, dag_node& this_node)
  {
    auto const area = app->getWindowBounds();
     ci::Rectf rect{static_cast<float>(area.getX1()), static_cast<float>(area.getY1()),
      static_cast<float>(area.getX2()), static_cast<float>(area.getY2())};

    auto const mouse_pos = app->getMousePos() - app->getWindowPos();
    auto const coord = ci::gl::windowToObjectCoord(mouse_pos);

    bool is_hovered = rect.canonicalized().contains(ci::vec2{coord});
    return !is_hovered;
  }, passive_shared);

  auto form = std::make_shared<dag_node>(dag_node_s{});
  form->set_child(passive_shared);
  return form;
}

void print_all_internal(shared_dag_t const& d, int level, std::set<shared_dag_t>& printed)
{
  if (!d) return;

  AURA_PRINT(L"%llu ", d->m_id);

  if (printed.count(d)) return;
  printed.insert(d);

  if (d->m_child)
  {
    AURA_PRINT(L"(child: ");
    print_all_internal(d->m_child, level + 1, printed);
    AURA_PRINT(L")");
  }
  if (d->m_sibling)
  {
    for (int i = 0; i < level; ++i) AURA_PRINT(L" ");
    AURA_PRINT(L"(sibling: ");
    print_all_internal(d->m_child, level + 1, printed);
    AURA_PRINT(L")");
  }

  for (auto const& [pred, next] : d->m_transitions)
  {
    for (int i = 0; i < level; ++i) AURA_PRINT(L" ");
    AURA_PRINT(L"(transition: \n");
    print_all_internal(next, level + 1, printed);
    AURA_PRINT(L")");
  }
}

void print_all(shared_dag_t const& d, int level)
{
  std::set<shared_dag_t> printed;
  print_all_internal(d, level, printed);
}

#if 0
std::shared_ptr<dag_node> create_ui_layer2()
{
  dag_node_s frame{};
  frame.m_on_render = make_render_fn([](cind_display_context_t app)
  {
    auto const area = app->getWindowBounds();
    ci::gl::ScopedColor col{1.0f, 0.0f, 0.0f, 1.0f};
    ci::Rectf rect{static_cast<float>(area.getX1()), static_cast<float>(area.getY1()),
      static_cast<float>(area.getX2()), static_cast<float>(area.getY2())};

    ci::gl::drawStrokedRect(rect, 5.0f);
  });

  dag_node_s form{};
  form.m_on_render = make_render_fn([](cind_display_context_t app)
  {
    ci::gl::ScopedColor col{0.0f, 0.0f, 1.0f, 1.0f};
    ci::Rectf rect{0.0f, 0.0f, 400.0f, 400.0f};

    ci::gl::drawStrokedRect(rect, 2.0f);
  });
  form.m_on_push = make_render_fn([](cind_display_context_t app)
  {
    auto const area = app->getWindowBounds();
    ci::gl::pushModelMatrix();
    ci::gl::translate(to_float(area.getX1()) + (to_float(area.getWidth())/2) - 200.0f, 
      to_float(area.getY1()) + (to_float(area.getHeight())/2) - 200.0f);
  });

  form.m_on_pop = []()
  {
    ci::gl::popModelMatrix();
  };

  dag_node_s button1{};
  button1.m_on_render = make_button_renderer({});

  button1.m_on_push = make_render_fn([](cind_display_context_t app)
  {
    auto const area = app->getWindowBounds();
    ci::gl::pushModelMatrix();
    ci::gl::translate(0.0f, 40.0f + 20.0f);
  });

  button1.m_on_pop = []()
  {
    ci::gl::popModelMatrix();
  };

  button1.m_child = std::make_shared<dag_node>(button1);
  form.m_child = std::make_shared<dag_node>(button1);
  frame.m_child = std::make_shared<dag_node>(form);

  return std::make_shared<dag_node>(std::move(frame));
}
#endif

} // namespace functions

//! display_engine interface : update the dag
player_action
cind_display_engine::display_session(std::shared_ptr<session_info> info,
                                     bool redraw)
{
  return player_action{};
}

}
