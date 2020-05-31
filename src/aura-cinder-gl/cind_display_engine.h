#pragma once

#include "aura-core/display_engine.h"
#include "aura-core/display_dag.h"
#include "main_functions.h"
#include <optional>
#include <cinder/app/App.h>
#include <cinder/gl/gl.h>
#include "cind_menu.h"

namespace aura
{

namespace functions
{
std::shared_ptr<dag_node> create_ui_layer();
void print_all(shared_dag_t const&, int level = 1);
}

class cind_display_engine;
using cind_display_context_t = cind_display_engine*;

class cind_display_engine
  : public display_engine
  , public ci::app::App
{
public:
  void clear_board() override {}
  
  //! ci::app::App interface
  void setup() override
  {
    //m_master_dag.add_layer(functions::create_ui_layer());
    m_menu.add_button<cind_display_context_t>("New (Local) Game", [](auto){});
    m_menu.add_button<cind_display_context_t>("New (Multiplayer) Game", [](auto){});
    m_menu.add_button<cind_display_context_t>("Settings", [](auto){});
    m_menu.add_button<cind_display_context_t>("Quit To Desktop", [](auto){});
    m_menu.add_close_button<cind_display_context_t>([](auto this_ref){});
    m_master_dag.add_layer(m_menu.get_node());
  }

  ~cind_display_engine()
  {
  }

  //! Override to receive mouse-down events.
  void mouseDown(ci::app::MouseEvent event) override
  {
    m_last_event = event;
  }

  void mouseUp(ci::app::MouseEvent event) override
  {
    m_last_event = event;
  }

  auto last_event() const noexcept { return m_last_event; }

  //! ci::app::App interface
  void draw() override
  {
    ci::gl::clear();

    m_master_dag.render_all(static_cast<cind_display_context_t>(this));
    AURA_PRINT(L"DAG: ");
    functions::print_all(m_master_dag.top_layer());
    AURA_PRINT(L"");
  }

  //! display_engine interface : update the dag
  player_action display_session(std::shared_ptr<session_info> info,
                                bool redraw) override;

 private:
  master_dag  m_master_dag;
  cind_menu   m_menu{cind_menu_style{}};
  std::optional<ci::app::MouseEvent> m_last_event;
};

} // namespace aura