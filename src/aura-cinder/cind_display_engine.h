#pragma once

#include <aura-core/display_engine.h>
#include <aura-core/ruleset.h>
#include <aura-core/local_rules_engine.h>
#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>
#include <cinder/Log.h>
#include <cinder/Text.h>
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
    auto const abs_path = std::filesystem::absolute(std::filesystem::current_path() / L"assets").wstring();
    AURA_LOG(L"assets path = %ls", abs_path.c_str());
    ci::app::addAssetDirectory(abs_path);  
    card_info in{};

    std::vector<bool> v(m_ruleset.max_lane_height, false);
    m_is_tile_revealed = std::vector<std::vector<bool>>(m_ruleset.num_lanes, v);

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
  void display_tile_overlay(
    player_info const& player,
    bool top,
    bool is_current_player,
    int lane_no,
    int lane_index,
    terrain_types tile_terrain,
    ci::Rectf const& tile_rect);

  void display_tile(player_info const& player,
    bool top,
    bool is_current_player,
    int lane_no,
    int lane_index,
    terrain_types tile_terrain,
    ci::Rectf const& tile_rect);

  void display_player_lanes(
    player_info const& player,
    bool top,
    bool is_current_player,
    ci::Rectf const& hand_area);

  void display_player_stats(
    player_info const& player,
    bool top,
    bool is_current_player,
    ci::Rectf const& hand_area);

  void display_player_hand(
    player_info const& player,
    bool top,
    bool is_current_player,
    ci::Rectf const& hand_area);

  void display_player(
    player_info const& player,
    bool top,
    bool is_current_player);

  void display_player_top(player_info const& player, bool is_current);

  void display_player_bottom(player_info const& player, bool is_current);

  enum class selection : int { passive, hovered, selected };

  ci::gl::Texture2dRef choose_texture(terrain_types t) const noexcept;

  void display_terrain();

  void display_text(std::string const& text, ci::Rectf const&, ci::ColorAf const& , float point_size, bool) const;

  void display_cost(ci::Rectf const& target, float scale, card_info const& card) const;

  bool display_preferred_terrain(ci::Rectf const& target, float scale, int index, card_info const& card) const;

  void display_player_overlay(ci::Rectf const& hand_area, player_info const& player, bool is_current_player);

  bool display_health(ci::Rectf const& target, float scale, int index, card_info const& card) const;

  bool display_strength(ci::Rectf const& target, float scale, int index, card_info const& card) const;

  void display_card_texture(ci::gl::Texture2dRef const& t) const;
  void display_card_full(card_info const&) const;

  void display_selected_card() const;

  void display_hovered_card() const;

  void display_hovered_description() const;

  void display_picks();

  void display_background() const;

  void display_mouse();

  void display_animations();

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

  std::vector<std::vector<bool>> m_is_tile_revealed{};
  //float m_ratio{0.0f};
};

} // namespace aura
