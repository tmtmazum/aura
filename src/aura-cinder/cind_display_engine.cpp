#include "cind_display_engine.h"
#include <aura-core/platform.h>
#include <aura-core/session_info.h>
#include <aura-core/build.h>
#include <aura-cinder/grid.h>

#include "cind_display_helpers.h"

namespace aura
{

namespace
{

template <size_t N, typename... Args>
std::string stringprintf(char const* format, Args&&... args)
{
  char buf[N]{};
  auto const e = snprintf(buf, N, format, std::forward<Args>(args)...);
  return buf;
}

std::string format_lane_marker(int i)
{
  std::string s{"-- Lane "};
  s += std::to_string(i + 1);
  s += " --";
  return s;
}


std::string to_string(std::vector<terrain_types> const& t)
{
  if (t.empty())
  {
    return "";
  }

  if (t.size() == 1)
  {
    return to_string(t[0]);
  }

  return std::accumulate(begin(t), end(t), std::string{""}, [](auto const& accum, auto& next)
  {
    return (accum.empty() ? "" : accum + ", ") + to_string(next); 
  });
}

std::string format_card_descr(rules_engine const& re, card_info const& card)
{
  std::string s;
  //std::string s{to_utf8_string(card.name)};
  if (!card.description.empty())
  {
    return to_utf8_string(card.description);
    //s += to_utf8_string(card.description);
    //s += "\n";
  }

  for (auto const& trait : card.traits)
  {
    if (auto d = re.describe(trait); !d.empty())
    {
      if (!s.empty())
        s += "; ";
      s += to_utf8_string(d);
    }
  }
  if (card.is_resting())
  {
    if (!s.empty())
      s += "; ";
    s += "resting: This unit is currently resting and cannot act this turn";
  }
  if (!card.is_visible)
  {
    if (!s.empty())
      s += "; ";
    s += "cloaked: This unit is hidden and cannot be targetted this turn";
  }
  return s;
}

} // namespace {}

auto terrain_to_color(terrain_types t)
{
  constexpr auto alpha = 0.5f;
  switch(t)
  {
  case terrain_types::plains:   return ci::ColorAf{0.49f, 0.76f, 0.26f, 0.5f}; // color: asda green
  case terrain_types::forests: return ci::ColorAf{0.0f, 0.62f, 0.41f, 0.5f}; // color: green mist 
  case terrain_types::mountains: return ci::ColorAf{1.0f, 0.98f, 0.98f, 0.5f}; // color: snow
  case terrain_types::riverlands: return ci::ColorAf{0.0f, 0.50f, 1.0f, 0.5f}; // color: azure
  }
  return ci::ColorAf{1.0f, 0.0f, 0.0f, 1.0f};
}

template <typename Fn>
struct [[nodiscard]] scope_exit : public Fn
{
  scope_exit(Fn&& fn)
    : Fn{std::move(fn)}
  {
  }

  ~scope_exit()
  {
    Fn::operator()();  
  }
};

void cind_display_engine::display_tile_overlay(
  player_info const& player,
  bool top,
  bool is_current_player,
  int lane_no,
  int lane_index,
  terrain_types tile_terrain,
  ci::Rectf const& tile_rect)
{
  auto const normalized_lane_index = top ? lane_index : m_ruleset.max_lane_height - lane_index - 1;
  auto const hovered = tile_rect.contains({m_constants.mouse_x, m_constants.mouse_y});
  auto const has_card = (player.lanes[lane_no].size() > normalized_lane_index);

  //! whether this is the next usable (empty) tile in the lane
  auto const is_next_tile_in_lane = (player.lanes[lane_no].size() == normalized_lane_index);

  std::lock_guard lk{m_mutex};
  if (!has_card)
  {
    if (!hovered)
    {
      return;
    }

    if (is_current_player
      && m_ui_action.is(cind_action_type::selected_hand_card)
      && is_next_tile_in_lane
      && m_selected_card && m_selected_card->can_be_deployed())
    {
      ci::gl::draw(get_texture(L"tile-move.png"), tile_rect);

      m_ui_action.add(uiact::hovered_lane, lane_no);
      m_hovered_description = to_string(tile_terrain);
      if (m_selected_card && m_selected_card->prefers_terrain(tile_terrain))
      {
        m_hovered_description += " (terrain bonus: +1 defense)";
        ci::gl::ScopedColor col{0.0f, 1.0f, 0.0f, 1.0f};
        ci::gl::drawStrokedRoundedRect(tile_rect, 5.0f);
      }
    }
    else if (player.lanes[lane_no].size() == normalized_lane_index
      && is_current_player
      && m_ui_action.is(cind_action_type::selected_hand_card))
    {
      ci::gl::ScopedColor col{1.0f, 1.0f, 1.0f, 0.3f};
      ci::gl::drawSolidRoundedRect(tile_rect, 4.0f);
      m_hovered_description = to_string(tile_terrain);
    }
    return;
  }

  auto& card = player.lanes[lane_no][normalized_lane_index];
  auto const is_selected = (m_ui_action.is(uiact::selected_lane_card) && card.uid == m_ui_action.value(uiact::selected_lane_card));

  if (card.on_preferred_terrain)
  {
    ci::gl::ScopedColor col{0.0f, 1.0f, 0.0f, 1.0f};
    ci::gl::drawStrokedRoundedRect(tile_rect, 5.0f);
  }

  if (is_selected)
  {
    ci::gl::draw(get_texture(L"tile-selected.png"), tile_rect);
  }

  if (card.is_resting() && is_current_player)
  {
    ci::gl::draw(get_texture(L"resting.png"), tile_rect);
  }

  if (!hovered)
  {
    return;
  }
  m_hovered_description = to_utf8_string(card.get_hovered_description());
  if (m_selected_card && m_selected_card->prefers_terrain(tile_terrain))
  {
    m_hovered_description += " (terrain bonus: +1 damage)";
  }

  if (is_selected)
  {
    m_ui_action.add(uiact::hovered_lane_card, card.uid);
    return;
  }

  m_hovered_card = &card;

  if (m_ui_action.is(cind_action_type::selected_lane_card) ||
       m_ui_action.is(cind_action_type::selected_hand_card))
  {
    bool in_sight = (m_selected_card && m_selected_card->action_type == card_action_type::ranged_attack)
      || ((player.lanes[lane_no].size() - 1) == normalized_lane_index);

    bool can_target = m_selected_card && 
      ((m_selected_card->can_target_enemy() && !is_current_player && in_sight)
      || (m_selected_card->can_target_friendly() && is_current_player)); 

    if (can_target)
    {
      auto const texture = std::invoke([&]
      {
        switch (m_selected_card->action_type)
        {
          case card_action_type::melee_attack: return L"tile-attack.png";
          case card_action_type::ranged_attack: return L"tile-attack.png";
          case card_action_type::healer: return L"icon-heal.png";
          case card_action_type::spell: return L"tile-attack.png";
        }
        return L"tile-attack.png";
      });
      ci::gl::draw(get_texture(texture), tile_rect);

      ci::gl::ScopedColor col{1.0f, 0.0f, 0.0f, 1.0f};
      ci::gl::drawStrokedRoundedRect(tile_rect, 5.0f);
      m_ui_action.add(uiact::hovered_lane_card, card.uid);
    }
    else
    {
      //if (m_selected_card)
      //{
      //  AURA_LOG(L"%ls: %d %d %d", m_selected_card->name.c_str(), m_selected_card->action_targets, m_selected_card->action_type, in_sight);
      //}
      ci::gl::draw(get_texture(L"tile-not-allowed.png"), tile_rect);

      ci::gl::ScopedColor col{1.0f, 0.0f, 0.0f, 1.0f};
      ci::gl::drawStrokedRoundedRect(tile_rect, 5.0f);
    }
  }
  else if (card.can_act() && is_current_player)
  {
    ci::gl::draw(get_texture(L"tile-highlight.png"), tile_rect);
    m_ui_action.add(uiact::hovered_lane_card, card.uid);
  }
}

void cind_display_engine::display_tile(
  player_info const& player,
  bool top,
  bool is_current_player,
  int lane_no,
  int lane_index,
  terrain_types tile_terrain,
  ci::Rectf const& tile_rect)
{
  auto const texture_name = std::invoke([&]
  {
    switch (tile_terrain)
    {
    case terrain_types::plains:     return L"tile-plains.png";
    case terrain_types::riverlands: return L"tile-riverlands.png";
    case terrain_types::mountains:  return L"tile-mountains.png";
    case terrain_types::forests:    return L"tile-forests.png";
    default:                        return L"tile-placeholder.png";
    }
  });

  auto const normalized_lane_index = top ? lane_index : m_ruleset.max_lane_height - lane_index - 1;

  if (player.lanes[lane_no].size() <= normalized_lane_index)
  {
    if (m_constants.always_display_terrain_tiles ||
      (player.lanes[lane_no].size() == normalized_lane_index
      && is_current_player && m_ui_action.is(uiact::selected_hand_card)
      && m_selected_card && m_selected_card->can_be_deployed()))
    {
      m_is_tile_revealed[lane_no][normalized_lane_index] = true;
      ci::gl::draw(get_texture(texture_name), tile_rect);
    }
    else
    {
      //ci::gl::draw(get_texture(L"tile-empty.png"), tile_rect);
    }
    return;
  }

  m_is_tile_revealed[lane_no][normalized_lane_index] = true;

  auto& card = player.lanes[lane_no][normalized_lane_index];

  ci::gl::draw(tile_card_texture(card), tile_rect);
  
  {
    auto const icon_name = std::invoke([&]
    {
      switch (tile_terrain)
      {
      case terrain_types::plains:     return L"icon-plains.png";
      case terrain_types::riverlands: return L"icon-riverlands.png";
      case terrain_types::mountains:  return L"icon-mountains.png";
      case terrain_types::forests:    return L"icon-forests.png";
      default:                        return L"icon-placeholder.png";
      }
    });
    auto [icon_w, icon_h] = std::pair{40.0f, 40.0f};
    ci::Rectf terrain_rect{tile_rect.x1, (tile_rect.y1 + tile_rect.y2)/2.0f - icon_w/2.0f, 0.0f, 0.0f};
    terrain_rect.x2 = terrain_rect.x1 + icon_w;
    terrain_rect.y2 = terrain_rect.y1 + icon_h;
    ci::gl::draw(get_texture(icon_name), terrain_rect);

    auto line_h = 15.0f;
    ci::Rectf line_rect{tile_rect.x1, tile_rect.y2 - line_h, tile_rect.x2, tile_rect.y2};
    aura::draw_line(line_rect, to_utf8_string(card.name));
  }

  auto const scale_factor = 0.8f;

  int index = 0;
  if (display_strength(tile_rect, scale_factor, index, card))
  {
    index++;
  }

  if (display_health(tile_rect, scale_factor, index, card))
  {
    index++;
  }
}

void cind_display_engine::display_player_overlay(ci::Rectf const& hand_area, player_info const& player, bool is_current_player)
{
  if (!hand_area.contains({m_constants.mouse_x, m_constants.mouse_y}))
  {
    return;
  }

  if (!m_selected_card)
  {
    return;
  }
  
  if (!is_current_player && !m_selected_card->can_target_enemy())
  {
    std::lock_guard lk{m_mutex};
    m_hovered_description = player.name + ": no actions possible at this time";
    return;
  }

  if (is_current_player && !((int)m_selected_card->action_targets & (int)card_action_targets::friendly_hero))
  {
    std::lock_guard lk{m_mutex};
    m_hovered_description = "Player: can't attack self";
    return;
  }

  auto x3 = m_constants.window_width/2.0f - m_constants.card_board_width/2.0f;
  auto x4 = x3 + m_constants.card_board_width;
  auto y3 = hand_area.y1;
  auto y4 = y3 + m_constants.card_board_height;

  ci::Rectf r2{x3, y3, x4, y4};

  if (!is_current_player && !player.has_free_lane() && m_selected_card->action_type != card_action_type::ranged_attack)
  {
    ci::gl::draw(get_texture(L"tile-not-allowed.png"), r2);
    {
      ci::gl::ScopedColor col{1.0f, 0.0f, 0.0f, 1.0f};
      ci::gl::drawStrokedRoundedRect(hand_area, 20.0f, 10.0f);
    }
    std::lock_guard lk{m_mutex};
    m_hovered_description = player.name + ": no free lane available to attack";
    return;
  }
  ci::gl::draw(get_texture(L"tile-attack.png"), r2);

  ci::gl::ScopedColor col{1.0f, 0.0f, 0.0f, 1.0f};
  ci::gl::drawStrokedRoundedRect(hand_area, 20.0f, 10.0f);

  std::lock_guard lk{m_mutex};
  m_ui_action.add(uiact::hovered_player, player.uid);
}

void cind_display_engine::display_player_hand(
  player_info const& player,
  bool top,
  bool is_current_player,
  ci::Rectf const& hand_area)
{
  auto const& c = m_constants;
  auto const scale_factor = is_current_player ? m_constants.card_active_hand_height_multiplier : m_constants.card_passive_hand_width_multiplier;

  auto f = make_grid(hand_area);
  f.align_horizontal(horizontal_alignment_t::center);
  f.align_vertical(top ? vertical_alignment_t::top : vertical_alignment_t::bottom);
  f.set_padding(m_constants.hand_horizontal_padding, m_constants.hand_vertical_padding);
  f.set_element_size(m_constants.full_card_width * scale_factor, m_constants.full_card_height * scale_factor);

  f.arrange_horizontally(player.hand, [&](auto const& item, ci::Rectf const& orig_rect)
  {
    auto const cur_mana = m_session_info->players[m_session_info->current_player].mana;
    auto const playable = item.cost <= cur_mana;

    auto rect = orig_rect;
    if (is_current_player && !playable)
    {
      auto const top_sign = top ? -1.0f : 1.0f;
      rect.offset({0.0f, top_sign * 4.0f});
    }

    ci::gl::ScopedModelMatrix mat{};
    ci::gl::translate(rect.x1, rect.y1);
    ci::gl::scale(scale_factor, scale_factor);

    if (is_current_player)
    {
      display_card_full(item);
    }

    auto [x1, x2] = std::minmax(0.0f, m_constants.full_card_width);
    auto [y1, y2] = std::minmax(0.0f, m_constants.full_card_height);

    ci::Rectf size_rect{x1, y1, x2, y2};
    auto const coord = ci::gl::windowToObjectCoord({m_constants.mouse_x, m_constants.mouse_y});

    auto const hovered = size_rect.contains(coord);
    auto const selected = m_ui_action.is(uiact::selected_hand_card) && m_ui_action.value(uiact::selected_hand_card) == item.uid;
    // draw hover highlight
    if (!is_current_player)
    {
      ci::gl::draw(get_texture(L"card-hidden.png"), size_rect);
    }
    else if (!playable)
    {
      ci::gl::draw(get_texture(L"card-passive.png"), size_rect);
    }
    else if (selected)
    {
      ci::gl::draw(get_texture(L"card-selected.png"), size_rect);
    }
    else if (hovered)
    {
      ci::gl::draw(get_texture(L"card-highlight.png"), size_rect);
    }

    std::lock_guard lk{m_mutex};
    if (hovered)
    {
      m_ui_action.add(uiact::hovered_hand_card, item.uid);
      if (is_current_player)
      {
        m_hovered_card = &item;
        if (playable)
        {
          m_hovered_description = to_utf8_string(item.get_hovered_description());
        }
        else
        {
          m_hovered_description = to_utf8_string(item.name) + " (Insufficient mana to play this card)";
        }
      }

    }
  });
}

void cind_display_engine::display_player_stats(
  player_info const& player,
  bool top,
  bool is_current_player,
  ci::Rectf const& hand_area)
{
  constexpr auto health_bar_h = 40.0f;
  ci::gl::draw(get_texture(L"player-bar.png"), hand_area);

  auto f = make_frame(hand_area);
  f.align_horizontal(horizontal_alignment_t::center);
  f.align_vertical(vertical_alignment_t::center);
  f.set_min_padding(4.0f, 4.0f);
  f.set_stretch(false);
  f.add_element(200.0f, health_bar_h, [&](auto const& rect)
  {
    draw_line(rect, "Player");
  });
  f.add_element(40.0f, health_bar_h, [&](auto const& rect)
  {
    ci::gl::draw(get_texture(L"icon-health.png"), rect);
    auto const r2 = rect.inflated({40.0f, 0.0f});
    aura::draw_line(r2, std::to_string(player.health) + "/" + std::to_string(player.starting_health));
  });
  f.add_element(40.0f, health_bar_h, [&](auto const& ){});
  f.add_element(40.0f, health_bar_h, [&](auto const& rect)
  {
    ci::gl::draw(get_texture(L"icon-gem.png"), rect);
    auto const r2 = rect.inflated({40.0f, 0.0f});
    aura::draw_line(r2, std::to_string(player.mana) + "/" + std::to_string(player.starting_mana));
  });
  f.add_element(40.0f, health_bar_h, [&](auto const& ){});
  f.add_element(120.0f, 30.0f, [&](auto const& rect)
  {

  if (is_current_player)
  {
    if (rect.contains({m_constants.mouse_x, m_constants.mouse_y}))
    {
      ci::gl::ScopedColor col{1.0, 0.0, 0.0, 1.0};
      ci::gl::drawSolidRoundedRect(rect, 10, 10);
      std::lock_guard lk{m_mutex};
      m_ui_action.add(uiact::hovered_end_turn, 0);
    }
    else
    {
      ci::gl::ScopedColor col{0.4, 0.4, 0.4, 1.0};
      ci::gl::drawSolidRoundedRect(rect, 10, 10);
    }
    display_text(std::string{"END TURN"}, rect, {0.9, 0.9, 0.9, 1.0}, rect.getHeight(), true);
  }
  else
  {
    {
      ci::gl::ScopedColor col{0.9, 0.9, 0.4, 1.0};
      ci::gl::drawSolidRoundedRect(rect, 10, 10);
    }
    display_text(std::string{"WAITING"}, rect, {0.1, 0.1, 0.1, 1.0}, rect.getHeight(), true);
  }
  });
  f.arrange_horizontally();

  // attacking the opponent champion
  display_player_overlay(hand_area, player, is_current_player);
}

void cind_display_engine::display_player_lanes(
    player_info const& player,
    bool top,
    bool is_current_player,
    ci::Rectf const& hand_area)
{
  auto g = make_grid(hand_area);
  g.set_padding(m_constants.board_horizontal_padding, 0.0f);
  g.set_element_size(m_constants.card_board_width, hand_area.getHeight());
  g.align_vertical(top ? vertical_alignment_t::top : vertical_alignment_t::bottom);

  int lane_no = 0;
  g.arrange_horizontally(m_session_info->terrain, [&](auto const& item, auto const& rect)
  {
    if (item.empty())
    {
      return;
    }

    auto g2 = make_grid(rect);
    g2.set_element_size(m_constants.card_board_width, m_constants.card_board_height);
    g2.set_padding(0.0f, m_constants.board_vertical_padding);
    g2.align_vertical(top ? vertical_alignment_t::top : vertical_alignment_t::bottom);
    g2.set_reverse(top);

    int lane_index = 0;
    g2.arrange(1, m_ruleset.max_lane_height, [&](auto const& tile_rect)
    {
      auto const& tile_terrain = item[top ? lane_index : lane_index + item.size() / 2];//item[top ? lane_index : (item.size() - lane_index - 1)];
      display_tile(player, top, is_current_player, lane_no, lane_index, tile_terrain, tile_rect);
      display_tile_overlay(player, top, is_current_player, lane_no, lane_index, tile_terrain, tile_rect);
      lane_index++;
    });
    lane_no++;
  });

  g.arrange_horizontally(player.lanes, [&](auto const& item, auto const& rect)
  {
    if (item.empty())
    {
      return;
    }

    auto g2 = make_grid(rect);
    g2.set_element_size(m_constants.card_board_width, m_constants.card_board_height);
    g2.set_padding(0.0f, m_constants.board_vertical_padding);
    g2.align_vertical(top ? vertical_alignment_t::top : vertical_alignment_t::bottom);

    g2.arrange_vertically(item, [&](auto const& sub_item, auto const& sub_rect)
    {
      ci::gl::ScopedModelMatrix mat{};
      ci::gl::translate(sub_rect.x1, sub_rect.y1);
      //ci::gl::rotate(-3.14159f * 0.5f);
      ci::gl::scale(0.2f, 0.2f);
    });
  });
}

void cind_display_engine::display_player(
  player_info const& player,
  bool top,
  bool is_current_player)
{
  auto const card_height =
      is_current_player ? (m_constants.card_active_hand_height_multiplier * m_constants.full_card_height)
                        : (m_constants.card_passive_hand_width_multiplier * m_constants.full_card_height);
  auto const hand_height = static_cast<float>(
      card_height + (2 * m_constants.hand_vertical_padding));

  auto win_frame = make_frame(ci::Rectf{0.0f, 0.0f, m_constants.window_width, m_constants.window_height});
  win_frame.align_vertical(top ? vertical_alignment_t::top : vertical_alignment_t::bottom);
  win_frame.align_horizontal(horizontal_alignment_t::center);
  win_frame.set_stretch(false);
 
  win_frame.add_element(m_constants.window_width, hand_height, [&](auto const& hand_area)
  {
    display_player_hand(player, top, is_current_player, hand_area);
  });

  //! Find the left corner co-ordinate to display a rectangle of 'width' at the center
  auto const lcenter_for = [&](auto const width)
  {
    return (m_constants.window_width - width) / 2;
  };

  // health bar
  auto const health_bar_h = 40.0f;
  win_frame.add_element(490.0f, health_bar_h, [&](auto const& hand_area)
  {
    display_player_stats(player, top, is_current_player, hand_area);
  });

  // lanes
  auto const lane_height = (m_constants.card_board_height * 4) + (m_constants.board_vertical_padding * 8);
  auto const lane_width = (m_constants.card_board_width * 4) + (m_constants.board_horizontal_padding * 8);

  win_frame.add_element(lane_width, lane_height, [&](auto const& hand_area)
  {
    display_player_lanes(player, top, is_current_player, hand_area);
  });
  win_frame.arrange_vertically();
}

void
cind_display_engine::display_text(
  std::string const& text,
  ci::Rectf const& rect,
  ci::ColorAf const& col,
  float point_size,
  bool center) const
{
  ci::TextBox box;
  box.font(ci::Font{"Cambria", point_size});

  box.text(text);
  box.setColor(col);
  box.size(rect.getSize());
  if (center) {
    box.setAlignment(ci::TextBox::Alignment::CENTER);
  }
  auto const surf = box.render({2, 2});
  auto const textr = ci::gl::Texture::create(surf);
  ci::gl::draw(textr, rect);
}

ci::gl::Texture2dRef cind_display_engine::choose_texture(terrain_types t) const noexcept
{
  switch(t)
  {
  case terrain_types::plains:   return get_texture(L"icon-plains.png");
  case terrain_types::forests: return get_texture(L"icon-forests.png");
  case terrain_types::mountains: return get_texture(L"icon-mountains.png");
  case terrain_types::riverlands: return get_texture(L"icon-riverlands.png");
  }
  return get_texture(L"icon-swords.png");
}

void cind_display_engine::display_player_top(player_info const& player, bool is_current)
{
  display_player(player, true, is_current);
}

void cind_display_engine::display_player_bottom(player_info const& player, bool is_current)
{
  display_player(player, false, is_current);
}

ci::gl::Texture2dRef cind_display_engine::hand_card_texture(card_info const& card) const
{
  auto const card_name = L"card-" + card.name + L".png";
  if (auto const t = get_texture(card_name))
  {
    return t;
  }
  return get_texture(L"card-placeholder.png");
}

ci::gl::Texture2dRef cind_display_engine::lane_card_texture(card_info const& card) const
{
  return hand_card_texture(card);
}

ci::gl::Texture2dRef cind_display_engine::tile_card_texture(card_info const& card) const
{
  auto const card_name = L"tile-" + card.name + L".png";
  if (auto const t = get_texture(card_name))
  {
    return t;
  }
  return get_texture(L"tile-placeholder.png");
}

ci::gl::Texture2dRef cind_display_engine::get_texture(std::wstring const& card_name) const
{
  if (auto const it = m_textures.find(card_name); it != m_textures.end())
  {
    return it->second;
  }

  auto const path = ci::app::getAssetPath(card_name);
  if (path.empty())
  {
    return nullptr;
  }

  auto const asset = ci::app::loadAsset(card_name);
  if (!asset)
  {
    return nullptr;
  }

  auto const img = ci::loadImage(asset);
  if (!img)
  {
    return nullptr;
  }

  auto const textr = ci::gl::Texture2d::create(img);
  if (!textr)
  {
    return nullptr;
  }

  m_textures.emplace(card_name, textr);
  return textr;
}

ci::gl::Texture2dRef cind_display_engine::hovered_card_texture(card_info const& card) const
{
  auto const card_name = L"card-" + card.name + L".png";
  if (auto const t = get_texture(card_name))
  {
    return t;
  }
  return get_texture(L"card-placeholder.png");
}

void cind_display_engine::display_cost(ci::Rectf const& target, float scale, card_info const& card) const
{
  ci::gl::ScopedModelMatrix mat{};
  ci::gl::translate(target.x1, target.y1);
  ci::gl::scale(scale, scale); 

  // show cost
  {
    ci::Rectf cost_rect{0.0f, 0.0f, 40.0f, 40.0f};
    ci::gl::draw(get_texture(L"icon-gem.png"), cost_rect);
    aura::draw_line(cost_rect, std::to_string(card.cost));
  }
}

template <typename Fn>
static void display_generic_icon(ci::Rectf const& target, float scale, int index, Fn const& display)
{
  auto [icon_w, icon_h] = std::pair{40.0f * scale, 40.0f * scale};
  auto vertical_padding = 5.0f * scale;

  ci::gl::ScopedModelMatrix mat{};
  ci::gl::translate(target.x2 - icon_w, target.y1 + index*(icon_h + vertical_padding));
  ci::gl::scale(scale, scale);

  display();
}

bool cind_display_engine::display_strength(ci::Rectf const& target, float scale, int index, card_info const& card) const
{
  auto const strength_texture = std::invoke([&]() -> ci::gl::Texture2dRef
  {
    if (!card.strength || card.has_trait(unit_traits::item))
    {
      return nullptr;
    }

    if (card.strength < 0)
    {
      return get_texture(L"icon-heal.png");
    }

    if (card.has_trait(unit_traits::long_range))
    {
      return get_texture(L"icon-bow.png");
    }
    return get_texture(L"icon-swords.png");
  });

  // show strength
  if (!strength_texture)
  {
    return false;
  }

  display_generic_icon(target, scale, index, [&]()
  {
    ci::Rectf sub_rect{0.0f, 0.0f, 40.0f, 40.0f};

    ci::gl::draw(strength_texture, sub_rect);
    auto const raw_strength = std::to_string(std::abs(card.strength));
    auto const str = card.starting_energy > 1 ? raw_strength + "x" + std::to_string(card.starting_energy) : raw_strength;
    aura::draw_line(sub_rect, str);
  });

  return true;
}

bool cind_display_engine::display_health(ci::Rectf const& target, float scale, int index, card_info const& card) const
{
  if (card.has_trait(unit_traits::item))
  {
    return false;
  }

  auto [icon_w, icon_h] = std::pair{40.0f * scale, 40.0f * scale};
  auto vertical_padding = 10.0f * scale;

  display_generic_icon(target, scale, index, [&]()
  {
    ci::Rectf sub_rect{0.0f, 0.0f, 40.0f, 40.0f};

    ci::gl::draw(get_texture(L"icon-health.png"), sub_rect);
    aura::draw_line(sub_rect, card.health_as_string());
  });

  return true;
}

bool cind_display_engine::display_preferred_terrain(ci::Rectf const& target, float scale, int index, card_info const& card) const
{
  auto const terrain_texture = std::invoke([&]() -> ci::gl::Texture2dRef
  {
    if (card.preferred_terrain.empty())
    {
      return nullptr;
    }
    return choose_texture(card.preferred_terrain[0]);
  });

  if (!terrain_texture)
  {
    return false;
  }

  display_generic_icon(target, scale, index, [&]()
  {
    ci::Rectf sub_rect{0.0f, 0.0f, 40.0f, 40.0f};

    ci::gl::draw(terrain_texture, sub_rect);
  });
  return true;
}

void cind_display_engine::display_card_texture(ci::gl::Texture2dRef const& t) const
{
  auto [x1, x2] = std::minmax(0.0f, m_constants.full_card_width);
  auto [y1, y2] = std::minmax(0.0f, m_constants.full_card_height);

  ci::Rectf rect{x1, y1, x2, y2};
  
  ci::gl::draw(t, rect);
}

void cind_display_engine::display_card_full(card_info const& card) const
{
  auto const mid_height = m_constants.window_height/2.0f;

  auto [x1, x2] = std::minmax(0.0f, m_constants.full_card_width);
  auto [y1, y2] = std::minmax(0.0f, m_constants.full_card_height);

  ci::Rectf rect{x1, y1, x2, y2};
  
  {
    ci::gl::draw(hovered_card_texture(card), rect);
  }

  // show name
  {
    auto const pad_x = 10.0f;
    auto const pad_y = 12.0f;
    auto const height = 20.0f;

    ci::Rectf sub_rect{x1 + pad_x, y2 - height - pad_y, x2 - pad_x, y2 - pad_y};
    aura::draw_line(sub_rect, to_utf8_string(card.name));
  }

  // show description
  {
    auto const pad_x = 10.0f;
    auto const pad_y = 12.0f + 20.0f;
    auto const height = 80.0f;

    ci::Rectf sub_rect{x1 + pad_x, y2 - height - pad_y, x2 - pad_x, y2 - pad_y};
    auto const d = format_card_descr(m_rules_engine, card);
    aura::draw_multiline(sub_rect, d, false);
  }

  auto [icon_w, icon_h] = std::pair{40.0f, 40.0f};

  // show cost
  display_cost(rect, 1.0f, card);

  auto cur_y = y1;

  int index = 0;
  if (display_strength(rect, 1.0f, index, card))
  {
    index++;
  }

  if (display_health(rect, 1.0f, index, card))
  {
    index++;
  }

  display_preferred_terrain(rect, 1.0f, index, card);
}

void cind_display_engine::display_selected_card() const
{
  if (!m_selected_card)
  {
    return;
  }

  auto const mid_height = m_constants.window_height/2.0f;

  auto [x1, x2] = std::minmax(m_constants.window_width - m_constants.full_card_width, m_constants.window_width);
  auto [y1, y2] =
      std::minmax(mid_height + m_constants.board_vertical_padding,
                  mid_height + m_constants.board_vertical_padding +
                      m_constants.full_card_height);
  ci::Rectf rect{x1, y1, x2, y2};

  {
    ci::gl::ScopedModelMatrix mat{};
    ci::gl::translate(x1, y1);

    display_card_full(m_selected_card.value());
  }
}

void cind_display_engine::display_hovered_card() const
{
  auto const mid_height = m_constants.window_height/2.0f;

  auto [x1, x2] = std::minmax(m_constants.window_width - m_constants.full_card_width, m_constants.window_width);

  auto [y1, y2] = m_selected_card
    ? std::minmax(mid_height - m_constants.board_vertical_padding, mid_height - (m_constants.board_vertical_padding + m_constants.full_card_height))
    : std::minmax(mid_height - m_constants.full_card_height/2.0f, mid_height + m_constants.full_card_height/2.0f);

  ci::Rectf rect{x1, y1, x2, y2};

  auto const en_rect = rect.inflated({40.0f, 40.0f});

  // if the mouse overlaps with the hover region, we want to display on the left instead
  bool display_on_left = false;
  if (en_rect.contains({m_constants.mouse_x, m_constants.mouse_y}))
  {
    display_on_left = true;
    rect.x1 = 0.0f;
    rect.x2 = m_constants.full_card_width;
  };

  {
    ci::gl::ScopedModelMatrix mat{};
    ci::gl::translate(rect.x1, rect.y1);

    display_card_full(*m_hovered_card);
  }

  if (m_selected_card)
  {
    auto x3 = display_on_left
      ? m_constants.full_card_width/2.0f - m_constants.card_board_width/2.0f
      : m_constants.window_width - m_constants.full_card_width/2.0f - m_constants.card_board_width/2.0f;
    auto x4 = x3 + m_constants.card_board_width;
    auto y3 = mid_height - m_constants.card_board_height/2.0f;
    auto y4 = y3 + m_constants.card_board_height;

    ci::Rectf r2{x3, y3, x4, y4};
    ci::gl::draw(get_texture(L"tile-attack.png"), r2);
  }
}

void cind_display_engine::display_hovered_description() const
{
  if (m_hovered_description.empty())
  {
    return;
  }

  auto const num_lines = [&]()
  {
    auto const n = std::count(begin(m_hovered_description), end(m_hovered_description), '\n');
    return n + 1;
  }();

  ci::Rectf hover_rect{m_constants.mouse_x, m_constants.mouse_y, m_constants.mouse_x + std::min(400.0f, 12.0f * m_hovered_description.size()), m_constants.mouse_y + (25.0f * num_lines)};
  hover_rect.offset({10.0f, 20.0f});
  {
    ci::gl::ScopedColor col{0.1f, 0.1f, 0.1f, 0.5f};
    ci::gl::drawSolidRect(hover_rect);
  }
  display_text(m_hovered_description, hover_rect, {1.0, 1.0, 1.0, 1.0}, hover_rect.getHeight() / num_lines, false);
}

void cind_display_engine::display_picks()
{
  if (m_session_info->picks.empty())
  {
    return;
  }

  auto const modal_area = std::invoke([&]()
  {
    ci::Rectf sub_area{};

    auto wind = make_frame(ci::Rectf{0.0f, 0.0f, m_constants.window_width, m_constants.window_height});
    wind.align_vertical(vertical_alignment_t::center);
    wind.add_element(m_constants.window_width, m_constants.pick_modal_height, [&](auto const& rect)
    {
      ci::gl::ScopedColor col{0.1, 0.1, 0.1, 0.6};
      ci::gl::drawSolidRect(rect);
      sub_area = rect;
    });
    wind.arrange_horizontally();
    return sub_area;
  });

  // Draw red-bordered turn indicator
  auto f = make_frame(modal_area);
  f.align_horizontal(horizontal_alignment_t::center);
  f.align_vertical(vertical_alignment_t::center);

  auto g = make_grid(modal_area);
  g.set_padding(m_constants.hand_horizontal_padding, m_constants.hand_vertical_padding);

  g.set_element_size(m_constants.full_card_width / 2, m_constants.full_card_height / 2);
  g.align_vertical(vertical_alignment_t::center);
  g.align_horizontal(horizontal_alignment_t::center);

  auto [grid_x, grid_y] = g.measure(m_session_info->picks.size(), 1);

  f.add_element(grid_x, grid_y, [&](auto const& rect)
  {
    g.bounds = rect;    
    g.arrange_horizontally(m_session_info->picks, [&](auto const& card, auto const& element)
    {
      bool const hovered = element.contains(ci::vec2{m_constants.mouse_x, m_constants.mouse_y});

      ci::gl::ScopedModelMatrix mat{};
      ci::gl::translate(element.getX1(), element.getY1());
      ci::gl::scale(0.5f, 0.5f);

      display_card_full(card);

      if (hovered)
      {
        display_card_texture(get_texture(L"card-highlight.png"));

        std::lock_guard lk{m_mutex};
        m_ui_action.add(uiact::hovered_pick_card, card.uid);
        m_hovered_description = to_utf8_string(card.name);
        m_hovered_card = &card;
      }
    });
  });
  f.add_element(500.0f, 100.0f, [&](auto const& rect)
  {
    auto& player = m_session_info->players[m_session_info->current_player];
    auto const num_draws = player.picks_available; //player.num_draws_per_turn - player.num_drawn_this_turn;
    auto const text = stringprintf<64>("Turn %d\nPlayer %d, pick %d card%c: ", m_session_info->turn,
      m_session_info->current_player + 1, num_draws, num_draws > 1 ? 's' : ' ');
    display_text(text, rect, {0.9, 0.9, 0.9, 1.0}, rect.getHeight() / 2, true);
    auto const col = ci::gl::ScopedColor{1.0, 0.0, 0.0, 1.0};
    ci::gl::drawStrokedRect(rect);
  });

  f.arrange_vertically();
}

void cind_display_engine::display_background() const
{
  auto [x1, x2] = std::minmax(0.0f, m_constants.window_width);
  auto [y1, y2] = std::minmax(0.0f, m_constants.window_height);
  ci::Rectf rect{x1, y1, x2, y2};
  if (auto const t = get_texture(L"woodfloor_c.jpg"))
  {
    ci::gl::draw(t, rect);
  }
}

void cind_display_engine::mouseDown(ci::app::MouseEvent event)
{
  std::lock_guard lk{m_mutex};

  AURA_LOG(L"+ 0x%x", m_ui_action.type);

  auto const start_animation = [&]()
  {
    auto [x, y] = std::make_pair(getMousePos().x - getWindowPos().x, getMousePos().y - getWindowPos().y);

    ci::Rectf r{static_cast<float>(x) - 100.0f, static_cast<float>(y) - 100.0f, x + 100.0f, y + 100.0f};
    m_dynamic_animations.emplace_back(make_animation(std::chrono::milliseconds(250), [rect = r, this](auto const& info)
    {
      constexpr auto num_frames = 6;
      auto const sprite_base = std::wstring{L"Slash"};
        
      auto const frame_no = static_cast<int>(info.ratio_elapsed * num_frames);

      auto const sprite_name = sprite_base + std::to_wstring(frame_no) + L".png";
      if (auto const t = get_texture(sprite_name))
      {
        ci::gl::draw(t, rect); 
      }
    }));
  };

  auto const reset_action = [&]()
  {
    m_ui_action.reset_selected();
    m_selected_card.reset();
  };

  // Pick card
  if (m_ui_action.is(uiact::hovered_pick_card))
  {
    auto const hovered_card = m_ui_action.value(uiact::hovered_pick_card);
    m_action.set_value(make_pick_action(hovered_card));
    reset_action();
    return;
  }

  // Player hand -> lane
  if (m_ui_action.is(uiact::hovered_lane)
      && m_ui_action.is(uiact::selected_hand_card))
  {
    auto const selected_value = m_ui_action.value(uiact::selected_hand_card);
    auto const hovered_lane = m_ui_action.value(uiact::hovered_lane);
    AURA_LOG(L"Setting action with %d, %d", selected_value, hovered_lane + 1);
    reset_action();
    m_action.set_value(make_deploy_action(selected_value, hovered_lane + 1));
    return;
  }

  // Lane card -> Lane card
  if (m_ui_action.is(uiact::selected_lane_card)
      && m_ui_action.is(uiact::hovered_lane_card)
      && m_ui_action.value(uiact::selected_lane_card) != m_ui_action.value(uiact::hovered_lane_card)
    )
  {
    m_action.set_value(make_primary_action(m_ui_action.value(uiact::selected_lane_card), m_ui_action.value(uiact::hovered_lane_card)));
    start_animation();
    reset_action();
    return;
  }
  
  // Hand card -> Lane card
  if (m_ui_action.is(uiact::selected_hand_card)
      && m_ui_action.is(uiact::hovered_lane_card))
  {
    m_action.set_value(make_primary_action(m_ui_action.value(uiact::selected_hand_card), m_ui_action.value(uiact::hovered_lane_card)));
    reset_action();
    return;
  }

  // Hand card -> Player
  if (m_ui_action.is(uiact::selected_hand_card)
      && m_ui_action.is(uiact::hovered_player))
  {
    m_action.set_value(make_primary_action(m_ui_action.value(uiact::selected_hand_card), m_ui_action.value(uiact::hovered_player)));
    reset_action();
    return;
  }

  // Lane card -> Player
  if (m_ui_action.is(uiact::selected_lane_card)
    && m_ui_action.is(uiact::hovered_player))
  {
    m_action.set_value(make_primary_action(m_ui_action.value(uiact::selected_lane_card), m_ui_action.value(uiact::hovered_player)));
    reset_action();
    return;
  }

  if (m_ui_action.is(uiact::hovered_end_turn))
  {
    m_action.set_value(make_end_turn_action());
    return;
  }

  // two consecutive clicks on same card means deselection
  if (m_ui_action.is(uiact::selected_hand_card)
    && m_ui_action.is(uiact::hovered_hand_card)
    && m_ui_action.value(uiact::selected_hand_card) == m_ui_action.value(uiact::hovered_hand_card))
  {
    m_ui_action.rm(uiact::selected_hand_card);
    m_selected_card = {};
    return;
  }

  // two consecutive clicks on same card means deselection
  if (m_ui_action.is(uiact::selected_lane_card)
    && m_ui_action.is(uiact::hovered_lane_card)
    && m_ui_action.value(uiact::selected_lane_card) == m_ui_action.value(uiact::hovered_lane_card))
  {
    AURA_LOG(L"Unsetting selected lane card");
    m_ui_action.rm(uiact::selected_lane_card);
    m_selected_card = {};
    return;
  }

  if (m_ui_action.is(uiact::hovered_lane_card))
  {
    if (m_hovered_card && !m_hovered_card->can_act())
    {
      return;
    }
    auto const hovered_card = m_ui_action.value(uiact::hovered_lane_card);
    m_ui_action.reset_selected();
    m_ui_action.add(uiact::selected_lane_card, hovered_card);
    m_can_be_targetted = m_rules_engine.get_target_list(hovered_card);
    m_selected_card = *m_hovered_card;
    return;
  }

  if (m_ui_action.is(uiact::hovered_hand_card))
  {
    auto const hovered_card = m_ui_action.value(uiact::hovered_hand_card);
    m_ui_action.reset_selected();
    m_ui_action.add(uiact::selected_hand_card, hovered_card);
    m_can_be_targetted = m_rules_engine.get_target_list(hovered_card);
    m_selected_card = *m_hovered_card;
  }
  AURA_LOG(L"- 0x%x", m_ui_action.type);
}

void cind_display_engine::display_terrain()
{
  //auto g = make_grid();
  //g.
}

void cind_display_engine::display_mouse()
{
  showCursor();
}

void cind_display_engine::display_animations()
{
  std::vector<decltype(m_dynamic_animations.begin())> erase_list;
  for (auto it = begin(m_dynamic_animations); it != end(m_dynamic_animations); ++it)
  {
    auto const now = std::chrono::steady_clock::now();
    auto const time_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - it->time_triggered);
    
    if (time_elapsed > it->total_duration)
    {
      erase_list.emplace_back(it);
      break;
    }

    anim_info in{
      time_elapsed,
      static_cast<float>(time_elapsed.count()) / it->total_duration.count(),
      it->index++
    };

    //AURA_LOG(L"Ratio elapsed: (%lld / %lld) %.02f, Index: %d", time_elapsed.count(), it->total_duration.count(), 
    //  in.ratio_elapsed, in.render_index);
    it->renderer(in);
  }

  for (auto const& e : erase_list)
  {
    m_dynamic_animations.erase(e);
  }
  m_last_frame_time = std::chrono::steady_clock::now();
}

player_action cind_display_engine::display_session(std::shared_ptr<session_info> info, bool redraw)
{
  with_lock([&] { m_session_info = std::move(info); });

  auto const action = m_action.get_future().get();
  std::promise<player_action> p;

  with_lock([&] { m_action.swap(p); });
  
  return action;
}

void cind_display_engine::draw()
{
  auto const sesh = with_lock([&]
  {
    m_ui_action.reset_hovered();

    auto const ws = getWindowBounds().getSize();
    m_constants.window_width = ws.x;
    m_constants.window_height = ws.y;
    m_constants.mouse_x = getMousePos().x - getWindowPos().x;
    m_constants.mouse_y = getMousePos().y - getWindowPos().y;

    m_hovered_description.clear();
    m_mouse_texture = get_texture(L"mouse-pointer.png");
    m_hovered_card = nullptr;
    return m_session_info;
  });

  ci::gl::clear();

  if (!sesh)
  {
    return;
  }

  assert(sesh->current_player == 0 || sesh->current_player == 1);
  //display_background();

  display_terrain();
  display_player_top(sesh->players[0], sesh->current_player == 0);
  display_player_bottom(sesh->players[1], sesh->current_player == 1);

  display_picks();
  {
    std::lock_guard lk{m_mutex};
    if (!m_hovered_description.empty())
    {
      display_hovered_description();
    }
    if (m_selected_card)
    {
      display_selected_card();
    }
    if (m_hovered_card)
    {
      display_hovered_card();
    }
  }
  display_mouse();
  display_animations();
}

}
