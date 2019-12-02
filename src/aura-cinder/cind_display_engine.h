#pragma once

#include <aura-core/display_engine.h>
#include <aura-core/ruleset.h>
#include <aura-core/local_rules_engine.h>
#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>
#include <cinder/log.h>
#include <cinder/text.h>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>

#include <aura-core/session_info.h>

namespace aura
{

enum class rect_type : int
{
  solid_rounded,
  stroked_rounded
};

class renderer
{
public:
  explicit renderer(ci::Area const& parent_area)
    : m_parent_area{parent_area}
  {}

  void rect(rect_type type, ci::Rectf coords, ci::ColorAf const& col)
  {
    ci::gl::ScopedColor color{col};
    coords.scale(m_parent_area.getSize());

    if (type == rect_type::solid_rounded)
    {
      return ci::gl::drawSolidRoundedRect(coords, 10, 10);
    }
    else if (type == rect_type::stroked_rounded)
    {
      return ci::gl::drawStrokedRoundedRect(coords, 10, 10);
    }
    else
    {
      CI_LOG_D(L"Unknown rect_type");
    }
  }

private:
  ci::Area m_parent_area;
};

class drawable
{
public:
  virtual void draw(renderer const& opts) = 0;
};

inline std::error_code start_game_session(ruleset const& rules, rules_engine& engine, display_engine& display)
{
  display.clear_board();
  while(!engine.is_game_over())
  {
    auto sesh_info = std::make_shared<session_info>(engine.get_session_info());
    auto redraw = true;

    std::error_code e{};
    do {
      auto const action = display.display_session(std::move(sesh_info), redraw);
      e = engine.commit_action(action);
      redraw = false;
    } while(e);
  }
	return {};
}

class cind_display_engine
  : public display_engine
  , public ci::app::App
{
public:
  void clear_board() override {}
  
  //! ci::app::App interface
  void setup() override {
    m_logic_thread = std::thread{[this]
    {
      aura::ruleset rs;
      aura::local_rules_engine re{rs};

      start_game_session(rs, re, *this);
    }};
  }

  ~cind_display_engine()
  {
    if (m_logic_thread.joinable())
    {
      m_logic_thread.join();
    }
  }

  //! ci::app::App interface
  void draw() override;

  void draw2() {
    //CI_LOG_D(L"Drawing");
    ci::gl::clear();


    auto const win_size = ci::app::getWindowBounds().getSize();

    ci::TextBox box;
    //box.size({200, 200});
    box.font(ci::Font{box.getFont().getName(), 18.0});
    box.text("[1] Adept Assassin\nAttack:1\nHealth:1");
    box.setColor({0.0, 1.0, 0.0, 1.0});

    renderer r{ci::app::getWindowBounds()};
    r.rect(rect_type::stroked_rounded, {0.0, 0.0, 1.0, 1.0}, {1.0, 0.0, 0.0, 1.0});
    {
      ci::gl::ScopedColor(1.0, 1.0, 1.0, 1.0);
      //ci::Rectf coords{0.1, 0.1, 0.2, 0.2};
      //coords.scale(win_size);
      ci::Rectf coords{100, 100, 240, 180};

      box.size(coords.getSize());

      auto const surf = box.render({2, 2});
      auto const textr = ci::gl::Texture::create(surf);
      CI_LOG_D(L"Rect = " << coords);
      auto const mouse = getMousePos() - getWindowPos();
      if (coords.contains(mouse))
      {
        ci::gl::ScopedColor col(0.5, 0.0, 0.0, 1.0);
        ci::gl::drawSolidRoundedRect(coords, 10, 50);
      }
      ci::gl::drawStrokedRoundedRect(coords, 10, 50);
      ci::gl::draw(textr, coords);
    }
    {
      ci::gl::ScopedColor(1.0, 1.0, 1.0, 1.0);
      ci::Rectf coords{0.8, 0.8, 0.9, 0.9};
      coords.scale(win_size);
      ci::gl::drawSolidRoundedRect(coords, 10, 50);
    }
  }
  //! display_engine interface

  //! display_engine interface
  player_action display_session(std::shared_ptr<session_info> info, bool redraw) override;

private:
  using layers_container = std::vector<std::vector<std::unique_ptr<drawable>>>;
  layers_container m_layers;

  std::shared_ptr<session_info> m_session_info;

  std::mutex m_mutex;

  std::thread m_logic_thread;
};

} // namespace aura
