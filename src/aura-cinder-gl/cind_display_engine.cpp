#include "cind_display_engine.h"
#include <aura-core/display_dag.h>
#include <cinder/gl/scoped.h>
#include <cinder/gl/gl.h>

namespace aura
{

template <typename Fn>
auto make_render_fn(Fn&& fn)
{
  return [f = std::move(fn)](auto const& any_val)
  {
    try
    {
      auto ptr = std::any_cast<render_data_t>(any_val);
      f(ptr);
    }
    catch (std::bad_any_cast const& e)
    {
      AURA_ASSERT(false);
    }
  };
}

namespace functions
{

std::shared_ptr<dag_node> create_ui_layer()
{
  dag_node_s frame{};
  frame.m_on_render = make_render_fn([](ci::app::AppBase* app)
  {
    auto const area = app->getWindowBounds();
    ci::gl::ScopedColor col{1.0f, 0.0f, 0.0f, 1.0f};
    ci::Rectf rect{static_cast<float>(area.getX1()), static_cast<float>(area.getY1()),
      static_cast<float>(area.getX2()), static_cast<float>(area.getY2())};

    ci::gl::drawStrokedRect(rect, 5.0f);
  });
  return std::make_shared<dag_node>(std::move(frame));
}

} // namespace functions

//! display_engine interface : update the dag
player_action
cind_display_engine::display_session(std::shared_ptr<session_info> info,
                                     bool redraw)
{
  return player_action{};
}

}
