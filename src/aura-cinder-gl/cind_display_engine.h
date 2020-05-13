#pragma once

#include "aura-core/display_engine.h"
#include "aura-core/display_dag.h"
#include "main_functions.h"
#include <cinder/app/App.h>
#include <cinder/gl/gl.h>

namespace aura
{

using render_data_t = ci::app::AppBase*; 

namespace functions
{
std::shared_ptr<dag_node> create_ui_layer();
}

class cind_display_engine
  : public display_engine
  , public ci::app::App
{
public:
  void clear_board() override {}
  
  //! ci::app::App interface
  void setup() override
  {
    //setup_assets();

    m_master_dag.add_layer(functions::create_ui_layer());
    //m_logic_thread = std::thread{[this]
    //{
    //  start_game_session(m_ruleset, m_rules_engine, *this);
    //}};
  }

  ~cind_display_engine()
  {
    //if (m_logic_thread.joinable())
    //{
    //  m_logic_thread.join();
    //}
  }

  //! Override to receive mouse-down events.
  //void mouseDown(ci::app::MouseEvent event) override;

  //! ci::app::App interface
  void draw() override
  {
    ci::gl::clear();

    m_master_dag.render_all(std::any{static_cast<render_data_t>(this)});
  }

  //! display_engine interface : update the dag
  player_action display_session(std::shared_ptr<session_info> info,
                                bool redraw) override;

 private:
  master_dag m_master_dag;
};

} // namespace aura
