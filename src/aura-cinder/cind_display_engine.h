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

#include "cind_action.h"

namespace aura
{

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

  //! display_engine interface
  player_action display_session(std::shared_ptr<session_info> info,
                                bool redraw) override;

 private:
  void display_hand(
    std::vector<card_info> const &hand,
    ci::Rectf const &bounds,
    bool is_current_player);

  void display_hand2(
    player_info const& player,
    bool top,
    bool is_current_player);


  void display_lanes(player_info const& player,
                     ci::Rectf const& bounds,
                     bool is_current,
                     bool reverse_y = false);

  void display_player_top(player_info const& player, bool is_current);

  void display_player_bottom(player_info const& player, bool is_current);

  enum class selection : int { passive, hovered, selected };

  ci::gl::Texture2dRef choose_texture(terrain_types t) const noexcept;

  void display_terrain();

  void display_hand_card(card_info const& card, ci::Rectf const& rect, selection sel, bool is_current_player) const;

  void display_lane_card(card_info const& info, ci::Rectf const& bounds, selection s) const;

  void display_lane_marker(int i, ci::Rectf const&);

  void display_text(std::string const& text, ci::Rectf const&, ci::ColorAf const& , float point_size, bool) const;

  void display_card_full(card_info const&) const;

  void display_selected_card() const;

  void display_hovered_card() const;

  void display_hovered_description() const;

  void display_picks();

  void display_background() const;

  void cind_display_engine::display_mouse();

  void cind_display_engine::display_animations();

  ci::gl::Texture2dRef hand_card_texture(card_info const&) const;

  ci::gl::Texture2dRef lane_card_texture(card_info const&) const;

  ci::gl::Texture2dRef tile_card_texture(card_info const&) const;

  ci::gl::Texture2dRef hovered_card_texture(card_info const&) const;

  ci::gl::Texture2dRef get_texture(std::wstring const& name) const;

  template <typename Fn>
  auto with_lock(Fn const& fn)
  {
    std::lock_guard lock{m_mutex};
    return fn();
  }

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

  // selected card is persisted across turns, so it must be copied
  std::optional<card_info> m_selected_card{};

  ci::gl::Texture2dRef m_mouse_texture{};

  std::vector<dynamic_animation> m_dynamic_animations;

  std::chrono::steady_clock::time_point m_last_frame_time;

  mutable std::unordered_map<std::wstring, ci::gl::Texture2dRef> m_textures;

  //! If a card is selected, this vector stores all other cards that can be targetted
  std::vector<int> m_can_be_targetted;

  std::promise<player_action> m_action;

  std::unordered_map<int, float> ratios;
  //float m_ratio{0.0f};
};

} // namespace aura
