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
#include <optional>
#include "display_constants.h"
#include <aura-core/build.h>
#include <aura-cinder/animation.h>

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

enum class cind_action_type : unsigned
{
  none                = 0b000000000,
  hovered_lane        = 0b000000001,
  hovered_hand_card   = 0b000000010,
  hovered_lane_card   = 0b000000100,
  hovered_end_turn    = 0b000001000,
  hovered_player      = 0b000010000,
  hovered_pick_card   = 0b000100000,
  selected_lane       = 0b001000000,
  selected_hand_card  = 0b010000000,
  selected_lane_card  = 0b100000000,
};

using uiact = cind_action_type;

struct cind_action
{
  unsigned type{};
  std::unordered_map<cind_action_type, int> targets;

  cind_action() = default;

  void reset_hovered()
  {
    // keep only the selected; remove the hovered
    type &= 0b111000000;
  }

  void reset_selected()
  {
    // keep only the hovered; remove the selected
    type &= 0b000111111;
  }

  auto value(cind_action_type t) const
  {
    AURA_ASSERT(is(t));
    return targets.at(t);
  }

  void add(cind_action_type new_type, int target)
  {
    type |= static_cast<unsigned>(new_type);
    targets[new_type] = target;
  }

  void rm(cind_action_type new_type)
  {
    type &= ~static_cast<unsigned>(new_type);
  }

  cind_action(cind_action_type new_type, int target)
    : cind_action{}
  {
    add(new_type, target);
  }

  bool is(cind_action_type check) const noexcept
  {
    return static_cast<unsigned>(check) & type;
  }
};

class cind_display_engine
  : public display_engine
  , public ci::app::App
{
public:
  void clear_board() override {}
  
  //! ci::app::App interface
  void setup() override {
    ci::app::addAssetDirectory(LR"(F:\cpp-projects\aura\assets\)");  
    card_info in{};
    //in.name = L"Militia";
    //hand_card_texture(in);

    m_logic_thread = std::thread{[this]
    {
      start_game_session(m_ruleset, m_rules_engine, *this);
    }};
  }

  ~cind_display_engine()
  {
    if (m_logic_thread.joinable())
    {
      m_logic_thread.join();
    }
  }

	//! Override to receive mouse-down events.
	void mouseDown(ci::app::MouseEvent event) override;

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

  void display_hand(
    std::vector<card_info> const &hand,
    ci::Rectf const &bounds,
    bool is_current_player);

  void display_lanes(player_info const& player, ci::Rectf const& bounds, bool is_current, bool reverse_y = false);

  void display_player_top(player_info const& player, bool is_current);

  void display_player_bottom(player_info const& player, bool is_current);

  enum class selection : int { passive, hovered, selected };

  ci::gl::Texture2dRef choose_texture(terrain_types t) noexcept;

  void display_terrain();

  void display_hand_card(card_info const& card, ci::Rectf const& rect, selection sel, bool is_current_player) const;

  void display_lane_card(card_info const& info, ci::Rectf const& bounds, selection s) const;

  void display_lane_marker(int i, ci::Rectf const&);

  void display_text(std::string const& text, ci::Rectf const&, ci::ColorAf const& , float point_size, bool) const;

  void display_hovered_card() const;
  void display_hovered_description() const;

  void display_picks();

  void display_background() const;
  void cind_display_engine::display_mouse();

  void add_animation(static_animation anim) noexcept;
  bool cind_display_engine::display_update_animation(static_animation const&);
  void cind_display_engine::display_animations();

  ci::gl::Texture2dRef hand_card_texture(card_info const&) const;
  ci::gl::Texture2dRef lane_card_texture(card_info const&) const;
  ci::gl::Texture2dRef tile_card_texture(card_info const&) const;
  ci::gl::Texture2dRef hovered_card_texture(card_info const&) const;
  ci::gl::Texture2dRef get_texture(std::wstring const& name) const;

private:
  display_constants m_constants;

  aura::ruleset m_ruleset;

  aura::local_rules_engine m_rules_engine{m_ruleset};

  std::shared_ptr<session_info> m_session_info;

  mutable std::mutex m_mutex;

  std::thread m_logic_thread;

  cind_action m_ui_action;

  //! Full description of the card currently hovered over
  mutable std::string m_hovered_description;
  card_info const* m_hovered_card{};
  ci::gl::Texture2dRef m_mouse_texture{};

  std::vector<static_animation> m_active_animations;
  std::chrono::steady_clock::time_point m_last_frame_time;

  mutable std::unordered_map<std::wstring, ci::gl::Texture2dRef> m_textures;

  //! If a card is selected, this vector stores all other cards that can be targetted
  std::vector<int> m_can_be_targetted;

  std::promise<player_action> m_action;
};

} // namespace aura
