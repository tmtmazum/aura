#include "cind_display_engine.h"
#include <cinder/msw/CinderMsw.h>
#include <aura-core/session_info.h>
#include <aura-core/build.h>
#include <aura-cinder/grid.h>

namespace aura
{

player_action cind_display_engine::display_session(std::shared_ptr<session_info> info, bool redraw)
{
  {
    std::lock_guard lk{m_mutex};
    m_session_info = std::move(info);
  }

  auto const action = m_action.get_future().get();
  std::lock_guard lk{m_mutex};
  std::promise<player_action> p;
  m_action.swap(p);
  return action;
}

namespace
{

template <size_t N, typename... Args>
std::string stringprintf(char const* format, Args&&... args)
{
  char buf[N]{};
  auto const e = sprintf_s(buf, N, format, std::forward<Args>(args)...);
  return buf;
}

std::string format_lane_marker(int i)
{
  std::string s{"-- Lane "};
  s += std::to_string(i + 1);
  s += " --";
  return s;
}

std::string format_strength(card_info const& card)
{
  if (card.strength > 0)
  {
    auto const times = card.starting_energy > 1 ? std::string{"*"} + std::to_string(card.starting_energy) : "";
    return "\n" + std::to_string(card.effective_strength()) + times + " dmg/turn"; 
  }
  else if (card.strength < 0)
  {
    return "\n+" + std::to_string(-card.strength) + " HP/turn"; 
  }
  return "";
}

std::string format_card_lane(card_info const& card)
{
  std::string s{"["};
  s += std::to_string(card.cost);
  s += "] ";
  s += ci::msw::toUtf8String(card.name);
  if (card.has_trait(unit_traits::item))
  {
    return s;
  }
  if (card.is_resting() && card.strength)
  {
    s += " (Zzz..)";
  }
  s += "\n---";
  s += format_strength(card);
  s += "\nHealth: ";
  s += std::to_string(card.effective_health());
  return s;
}

std::string to_string(terrain_types const t)
{
  switch(t)
  {
  case terrain_types::forests:  return "forests";
  case terrain_types::mountains: return "mountains";
  case terrain_types::plains: return "plains";
  case terrain_types::riverlands: return "riverlands";
  }
  AURA_ASSERT(false);
  return "unknown";
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

std::string format_card_descr(rules_engine& re, card_info const& card)
{
  std::string s{ci::msw::toUtf8String(card.name)};
  if (!card.description.empty())
  {
    s += "\n";
    s += ci::msw::toUtf8String(card.description);
  }

  if (card.has_trait(unit_traits::item))
  {
    s += "\n";
    s += "item: can be applied to cards in play";
    return s;
  }

  if (card.strength)
  {
    s += "\nStrength: ";
    s += format_strength(card);
  }

  {
    s += "\nHealth: ";
    s += std::to_string(card.effective_strength());
  }

  {
    s += "\nCost: ";
    s += std::to_string(card.cost);
  }

  if (auto const pref_terrain = to_string(card.preferred_terrain); !pref_terrain.empty())
  {
    s += "\nPreferred Terrain: ";
    s += pref_terrain;
  }

  for (auto const& trait : card.traits)
  {
    s += "\n";
    s += ci::msw::toUtf8String(re.describe(trait));
  }
  if (card.is_resting())
  {
    s += "\nresting: This unit is currently resting and cannot act this turn";
  }
  if (!card.is_visible)
  {
    s += "\ncloaked: This unit is hidden and cannot be targetted this turn";
  }
  return s;
}

std::string format_card_hand(card_info const& card)
{
  std::string s{"["};
  s += std::to_string(card.cost);
  s += "] ";
  s += ci::msw::toUtf8String(card.name);
  s += "\n---";
  if (card.has_trait(unit_traits::item))
  {
    return s;
  }

  s += format_strength(card);
  s += "\nHealth: ";
  s += std::to_string(card.effective_health());
  return s;
}

auto to_string(std::wstring const& ws)
{
  return ci::msw::toUtf8String(ws);
}

} // namespace {}

void cind_display_engine::display_hand_card(card_info const& card, ci::Rectf const& rect, selection sel, bool is_current_player) const
{
  if (is_current_player)
  {
    display_lane_card(card, rect, sel);
  }
}

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
[[nodiscard]] struct scope_exit : public Fn
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

void cind_display_engine::display_lane_card(card_info const& card, ci::Rectf const& rect, selection sel) const
{
  auto col = std::invoke([&]
  {
    switch (sel)
    {
    case selection::selected: 
      return m_constants.hand_selected_color;
    case selection::hovered: return m_constants.hand_hovered_color;
    case selection::passive: 
    default:
      return m_constants.hand_card_color;
    }
  });
  col.a = 0.5;

  {
    ci::gl::ScopedLineWidth(2.0f);
    ci::gl::ScopedColor c{card.preferred_terrain.empty() ? 
      ci::ColorAf{0.1f, 0.1f, 0.1f, 1.0f} :
      terrain_to_color(card.preferred_terrain[0])};
    ci::gl::drawStrokedRoundedRect(rect, 10, 10);
  }

  auto texture_rect = rect;
  texture_rect.canonicalize();
  texture_rect.x2 = texture_rect.x1 + m_constants.card_texture_width;

  auto const mouse_coords = ci::dvec2{m_constants.mouse_x, m_constants.mouse_y};

  if (texture_rect.contains(mouse_coords))
  {
    std::lock_guard lock{m_mutex};
    m_hovered_description = ci::msw::toUtf8String(card.name);
    if (!card.description.empty())
    {
      m_hovered_description += "\n(" + ci::msw::toUtf8String(card.description) + ")";
    }
    if (!card.preferred_terrain.empty())
    {
      m_hovered_description += "\n(prefered terrain: " + to_string(card.preferred_terrain[0]) + ")";
    }
  }

  {
    std::lock_guard lk{m_mutex};
    if (auto const t = tile_card_texture(card))
    {
      ci::gl::draw(t, texture_rect); 
    }
  }

  auto cost_rect = texture_rect;
  {
    cost_rect.x1 = texture_rect.x2;
    cost_rect.x2 = cost_rect.x1 + m_constants.tile_icon_width;
    cost_rect.y1 = texture_rect.y1;
    cost_rect.y2 = cost_rect.y1 + m_constants.tile_icon_height;

    if (auto const t = get_texture(L"icon-gem.png"))
    {
      ci::gl::draw(t, cost_rect); 
    }

    auto cost_text_rect = cost_rect;
    cost_text_rect.offset({m_constants.tile_icon_width, 0.0f});

    if (cost_text_rect.contains(mouse_coords) || cost_rect.contains(mouse_coords))
    {
      std::lock_guard lock{m_mutex};
      m_hovered_description = stringprintf<64>("Cost: %d", card.cost);
    }
  
    ci::TextBox box;
    box.font(ci::Font{box.getFont().getName(), m_constants.tile_icon_font_point});

    box.text(stringprintf<64>("%d", card.cost));
    box.size(cost_text_rect.getSize());
    auto const surf = box.render();
    auto const textr = ci::gl::Texture::create(surf);
    ci::gl::draw(textr, cost_text_rect);
  }

  scope_exit e{[&]()
  {
    auto name_rect = rect;
    name_rect.y1 = rect.y2 - m_constants.tile_icon_height;
    name_rect.y2 = name_rect.y1 + m_constants.tile_icon_height;

    if (card.is_resting())
    {
      ci::gl::draw(get_texture(L"resting.png"), texture_rect); 
    }
    else if (sel == selection::selected)
    {
      ci::gl::ScopedColor col{0.0f, 0.0f, 1.0f, 0.2f};
      ci::gl::drawSolidRoundedRect(texture_rect, 10, 10);
    }
    else if (sel == selection::hovered)
    {
      ci::gl::ScopedColor col{1.0f, 1.0f, 1.0f, 0.2f};
      ci::gl::drawSolidRoundedRect(texture_rect, 10, 10);
    }

    if (card.on_preferred_terrain)
    {
      ci::gl::ScopedColor col{0.0f, 1.0f, 0.0f, 1.0f};
      ci::gl::drawStrokedRoundedRect(rect, 10, 4);
    }
  }};

  if (card.has_trait(unit_traits::item))
  {
    return;
  }

  auto str_rect = cost_rect;
  {
    str_rect.offset({0.0f, m_constants.tile_icon_height});
    if (card.strength)
    {
      auto const texture = [&]()
      {
        if (card.strength > 0 && card.has_trait(unit_traits::long_range))
        {
          return get_texture(L"icon-bow.png");
        }
        else if (card.strength > 0)
        {
          return get_texture(L"icon-swords.png");
        }
        return get_texture(L"icon-heal.png");
      }();

      if (texture)
      {
        ci::gl::draw(texture, str_rect); 
      }

      auto str_text_rect = str_rect;
      str_text_rect.offset({m_constants.tile_icon_width, 0.0f});

      if (str_text_rect.contains(mouse_coords) || str_rect.contains(mouse_coords))
      {
        std::lock_guard lock{m_mutex};
        if (card.strength > 0)
        {
          m_hovered_description = stringprintf<64>("Attack Strength: %d %hs", card.effective_strength(),
            card.on_preferred_terrain ? "(+1 terrain bonus)" : "");
        }
        else if (card.strength < 0)
        {
          m_hovered_description = stringprintf<64>("Healing Strength: %d", card.strength);
        }
      }
      
      ci::TextBox box;
      box.font(ci::Font{box.getFont().getName(), m_constants.tile_icon_font_point});

      box.text(stringprintf<64>("%d", card.strength > 0 ? card.effective_strength() : - card.strength));
      box.size(str_text_rect.getSize());
      auto const surf = box.render();
      auto const textr = ci::gl::Texture::create(surf);
      ci::gl::draw(textr, str_text_rect);
    }
  }

  auto health_rect = str_rect;
  {
    health_rect.offset({0.0f, m_constants.tile_icon_height});
    if (auto const t = get_texture(L"icon-shield.png"))
    {
      ci::gl::draw(t, health_rect); 
    }

    auto health_text_rect = health_rect;
    health_text_rect.offset({m_constants.tile_icon_width, 0.0f});
  
    if (health_text_rect.contains(mouse_coords) || health_rect.contains(mouse_coords))
    {
      std::lock_guard lock{m_mutex};
      m_hovered_description = stringprintf<64>("Health: %d %hs", card.effective_health(),
        card.on_preferred_terrain ? "(+1 terrain bonus)" : "");
    }
    ci::TextBox box;
    box.font(ci::Font{box.getFont().getName(), m_constants.tile_icon_font_point});

    box.text(stringprintf<64>("%d", card.effective_health()));
    box.size(health_text_rect.getSize());
    auto const surf = box.render();
    auto const textr = ci::gl::Texture::create(surf);
    ci::gl::draw(textr, health_text_rect);
  }

  auto speed_rect = health_rect;
  {
    speed_rect.offset({0.0f, m_constants.tile_icon_height});
    //if (card.energy > 1)
    {
      if (auto const t = get_texture(L"icon-speed.png"))
      {
        ci::gl::draw(t, speed_rect); 
      }

      auto speed_text_rect = speed_rect;
      speed_text_rect.offset({m_constants.tile_icon_width, 0.0f});
  
      if (speed_text_rect.contains(mouse_coords) || speed_rect.contains(mouse_coords))
      {
        std::lock_guard lock{m_mutex};
        m_hovered_description = stringprintf<64>("Speed: %d %hs", card.energy,
          "(number of times this unit can act per turn)");
      }
        
      ci::TextBox box;
      box.font(ci::Font{box.getFont().getName(), m_constants.tile_icon_font_point});

      box.text(stringprintf<64>("%d", card.energy));
      box.size(speed_text_rect.getSize());
      auto const surf = box.render();
      auto const textr = ci::gl::Texture::create(surf);
      ci::gl::draw(textr, speed_text_rect);
    }
  }
}

void cind_display_engine::display_hand(
  std::vector<card_info> const &hand,
  ci::Rectf const &bounds,
  bool is_current_player)
{
  auto const num_hand_cards = hand.size();

  auto const total_width = num_hand_cards * ((2*m_constants.hand_horizontal_padding) + m_constants.card_hand_width);

  //! Find the left corner co-ordinate to display a rectangle of 'width' at the center
  auto const lcenter_for = [&](auto const width)
  {
     return (m_constants.window_width - width) / 2;
  };

  auto [cur_pos_x, cur_pos_y] = std::make_pair(lcenter_for(total_width), bounds.getY1());
  cur_pos_y += m_constants.hand_vertical_padding;
  for (int i = 0; i < num_hand_cards; ++i)
  {
    cur_pos_x += m_constants.hand_horizontal_padding;
    ci::Rectf rect{cur_pos_x, cur_pos_y, cur_pos_x + m_constants.card_hand_width,
                   cur_pos_y + m_constants.card_hand_height};

    auto const& card = hand[i];

    // draw card background
    auto const sel = std::invoke([&]
    {
      bool hovered = rect.contains(ci::ivec2{m_constants.mouse_x, m_constants.mouse_y});

      {
        std::lock_guard lk{m_mutex};
        if (hovered)
        {
          m_ui_action.add(uiact::hovered_hand_card, card.uid);
          if (is_current_player)
          {
            m_hovered_card = &card;
            //m_hovered_description = ci::msw::toUtf8String(card.name);
          }
        }

        if (m_ui_action.is(uiact::selected_hand_card) && m_ui_action.value(uiact::selected_hand_card) == card.uid)
        {
          return selection::selected;
        }
      }

      if (hovered)
      {
        return selection::hovered;
      }

      return selection::passive;
    });

    display_hand_card(card, rect, sel, is_current_player);

    cur_pos_x += m_constants.card_hand_width;
    cur_pos_x += m_constants.hand_horizontal_padding;
  }
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
  box.font(ci::Font{box.getFont().getName(), point_size});

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

void cind_display_engine::display_lane_marker(int i, ci::Rectf const& rect)
{
  display_text(format_lane_marker(i), rect, {0.3, 0.3, 0.3, 1.0}, rect.getHeight(), true);
}

ci::gl::Texture2dRef cind_display_engine::choose_texture(terrain_types t) noexcept
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

void cind_display_engine::display_lanes(
  player_info const &player,
  ci::Rectf const &bounds,
  bool is_current,
  bool reverse_y)
{
  auto const num_lanes = player.lanes.size();

  auto const width_needed = 
    (m_constants.card_board_width * 4) +
    (num_lanes * 2 * m_constants.board_horizontal_padding);

  auto const sign = reverse_y ? -1.0f : 1.0f;

  auto const remaining_width = m_constants.window_width - width_needed;

  //! Find the left corner co-ordinate to display a rectangle of 'width' at the center
  auto const lcenter_for = [&](auto const width)
  {
     return (m_constants.window_width - width) / 2;
  };

  // whether or not to print lanes at the center of the screen
  bool const center_lanes = true;

  auto lane_start_x = static_cast<float>(bounds.x1 + center_lanes*(remaining_width / 2));
  auto [cur_pos_x, cur_pos_y] = std::make_pair(lane_start_x, bounds.y1);

  {
    auto const max_health_bar_width = 50.0f * player.starting_health;
    auto const actual_health_bar_width = ((float)player.health / player.starting_health) * max_health_bar_width;
    auto const lcenter = lcenter_for(actual_health_bar_width);

    auto const [x1, x2] = std::minmax(lcenter, lcenter + actual_health_bar_width);
    auto const [x3, x4] = std::minmax(lcenter_for(max_health_bar_width), lcenter_for(max_health_bar_width) + max_health_bar_width);
    auto const [y1, y2] = std::minmax(cur_pos_y, cur_pos_y + (sign * m_constants.board_lane_marker_height));

    ci::Rectf reduced_rect{x1, y1, x2, y2};
    ci::Rectf max_rect{x3, y1, x4, y2};
    //ci::Rectf rect{cur_pos_x + (remaining_width / 2), y1,  cur_pos_x + (remaining_width / 2) + 500, y2};

    {
      ci::gl::ScopedColor col{0.1, 0.1, 0.1, 1.0};
      ci::gl::drawSolidRoundedRect(max_rect, 10, 10);
    }

    if (reduced_rect.contains({m_constants.mouse_x, m_constants.mouse_y}))
    {
      ci::gl::ScopedColor col{0.0, 0.5, 0.0, 1.0};
      ci::gl::drawSolidRoundedRect(reduced_rect, 10, 10);

      std::lock_guard lk{m_mutex};
      m_ui_action.add(uiact::hovered_player, player.uid);
    }
    else
    {
      ci::gl::ScopedColor col{0.0, 0.4, 0.0, 1.0};
      ci::gl::drawSolidRoundedRect(reduced_rect, 10, 10);
    }

    auto const str = stringprintf<64>("Player | Health: %d / %d | Mana: %d", player.health, player.starting_health, player.mana);
    //reduced_rect.x2 = reduced_rect.x1 + max_health_bar_width;
    display_text(str, max_rect, {0.9, 0.9, 0.9, 1.0}, max_rect.getHeight(), true);

    cur_pos_y += (sign * (m_constants.board_lane_marker_height + m_constants.board_vertical_padding));
  }

  {
    auto const [x1, x2] = std::minmax(lcenter_for(120), lcenter_for(120) + 120);
    auto const [y1, y2] = std::minmax(cur_pos_y, cur_pos_y + (sign * m_constants.board_lane_marker_height));
    ci::Rectf rect{x1, y1,  x2, y2};
    //ci::Rectf rect{cur_pos_x, y1,  cur_pos_x + 120, y2};

    if (is_current)
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

    cur_pos_y += (sign * (m_constants.board_lane_marker_height + m_constants.board_vertical_padding));
  }
  
  for (int i = 0; i < num_lanes; ++i)
  {
    cur_pos_x += m_constants.board_horizontal_padding;
    auto const [y1, y2] = std::minmax(cur_pos_y, cur_pos_y + (sign * m_constants.board_lane_marker_height));
    ci::Rectf rect{cur_pos_x, y1, cur_pos_x + m_constants.card_board_width,
                   y2};

    display_lane_marker(i, rect);

    cur_pos_x += m_constants.card_board_width;
    cur_pos_x += m_constants.board_horizontal_padding;
  }

  cur_pos_x = lane_start_x;
  cur_pos_y += sign*(m_constants.board_lane_marker_height + m_constants.board_vertical_padding);
  auto const starting_y = cur_pos_y;
  for (int i = 0; i < num_lanes; ++i)
  {
    cur_pos_x += m_constants.board_horizontal_padding;
    cur_pos_y = starting_y;
    auto const n = static_cast<int>(player.lanes[i].size());
    auto const max_height = m_ruleset.max_lane_height;
    //auto const n = std::min(static_cast<int>(player.lanes[i].size()) + 1, m_ruleset.max_lane_height);
    for (int j = 0; j < max_height; ++j)
    {
      cur_pos_y += sign * m_constants.board_vertical_padding;

      auto [y1, y2] = std::minmax(cur_pos_y, cur_pos_y + (sign * m_constants.card_board_height));
      ci::Rectf rect{cur_pos_x, y1, cur_pos_x + m_constants.card_board_width, y2};
      rect.canonicalize();


      // draw terrain tile
      {
        auto const h_index = reverse_y ? (max_height - 1 - j) : j;
        auto const t = m_session_info->terrain[i][h_index];

        if (rect.contains({m_constants.mouse_x, m_constants.mouse_y}))
        {
          std::lock_guard lk{m_mutex};
          m_hovered_description = to_string(t);
        }

        ci::gl::ScopedColor col{terrain_to_color(t)};
        ci::gl::drawSolidRoundedRect(rect, 10, 20);
      }

      if (j == player.lanes[i].size())
      {
        // empty (deployable) lane : highlight
        if (rect.contains(ci::vec2{m_constants.mouse_x, m_constants.mouse_y}) && m_ui_action.is(uiact::selected_hand_card) && is_current)
        {
          ci::gl::ScopedColor col{m_constants.hand_hovered_color};
          ci::gl::drawSolidRoundedRect(rect, 10, 10);
          std::lock_guard lk{m_mutex};
          m_ui_action.add(uiact::hovered_lane, i);
        }
      }
      else if (j < n)
      {
        auto const& card = player.lanes[i][j];
        bool const hovered = rect.contains(ci::vec2{m_constants.mouse_x, m_constants.mouse_y});
        if (hovered)
        {
          std::lock_guard lk{m_mutex};
          m_ui_action.add(uiact::hovered_lane_card, card.uid);
          //m_hovered_description = ci::msw::toUtf8String(card.name);
          m_hovered_card = &card;
        }

        if (m_ui_action.is(uiact::selected_lane_card) && m_ui_action.value(uiact::selected_lane_card) == card.uid)
        {
          display_lane_card(card, rect, selection::selected);
        }
        else if (hovered)
        {
          display_lane_card(card, rect, selection::hovered);
        }
        else
        {
          display_lane_card(card, rect, selection::passive);
        }
      }

      { // draw terrain icon
        ci::Rectf icon_rect{rect.x1 - m_constants.terrain_icon_width, rect.y1, rect.x1, rect.y1 + m_constants.terrain_icon_height};
        icon_rect.offset({m_constants.terrain_icon_width * 0.4f, 0.0f});
        auto const h_index = reverse_y ? (max_height - 1 - j) : j;
        auto const t = m_session_info->terrain[i][h_index];
        if (auto const textr = choose_texture(t))
        {
          ci::gl::draw(textr, icon_rect); 
        }
      }

      cur_pos_y += (sign * m_constants.card_board_height);
      cur_pos_y += (sign * m_constants.board_vertical_padding);
    }
    cur_pos_x += m_constants.card_board_width + m_constants.board_horizontal_padding;
  }
}

void cind_display_engine::display_player_top(player_info const& player, bool is_current)
{
  auto [cur_pos_x, cur_pos_y] = std::make_pair(0.0f, 0.0f);
  auto const hand_height = static_cast<float>(m_constants.card_hand_height + (2*m_constants.hand_vertical_padding));
  display_hand(player.hand, ci::Rectf{cur_pos_x, cur_pos_y, 500.0f, hand_height}, is_current);

  cur_pos_y += hand_height;

  display_lanes(player, ci::Rectf{cur_pos_x, cur_pos_y, 0, 0}, is_current, false);
}

void cind_display_engine::display_player_bottom(player_info const& player, bool is_current)
{
  auto [cur_pos_x, cur_pos_y] = std::make_pair(0.0f, 0.0f);
  auto const hand_height = static_cast<float>(m_constants.card_hand_height + (2*m_constants.hand_vertical_padding));
  cur_pos_y = m_constants.window_height - hand_height;
  display_hand(player.hand, ci::Rectf{cur_pos_x, cur_pos_y, 500.0f, hand_height}, is_current);

  //cur_pos_y = cur_pos_y - hand_height;
  display_lanes(player, ci::Rectf{cur_pos_x, m_constants.window_height - hand_height, 0, 0}, is_current, true);
  //display_lanes_top(player, m_constants, ci::Rectf{cur_pos_x, cur_pos_y, 0, 0});
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

void cind_display_engine::display_hovered_card() const
{
  auto const mid_height = m_constants.window_height/2.0f;

  auto [x1, x2] = std::minmax(m_constants.window_width - m_constants.highlight_descr_width, m_constants.window_width);
  auto [y1, y2] = std::minmax(mid_height - m_constants.highlight_descr_height/2.0f, mid_height + m_constants.highlight_descr_height/2.0f);
  ci::Rectf rect{x1, y1, x2, y2};
  rect.offset({-m_constants.card_icon_width, 0.0f});
  
  if (auto const t = m_hovered_card ? hovered_card_texture(*m_hovered_card) : nullptr)
  {
    ci::gl::draw(t, rect);
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

  ci::Rectf hover_rect({m_constants.mouse_x, m_constants.mouse_y, m_constants.mouse_x + std::min(400.0f, 12.0f * m_hovered_description.size()), m_constants.mouse_y + (25.0f * num_lines)});
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

  auto [x1, x2] = std::minmax(0.0f, m_constants.window_width);
  auto const y_start = (m_constants.window_height - m_constants.pick_modal_height) / 2;
  auto [y1, y2] = std::minmax(y_start, y_start + m_constants.pick_modal_height);
  ci::Rectf modal_area{x1, y1, x2, y2};
  {
    ci::gl::ScopedColor col{0.1, 0.1, 0.1, 0.6};
    ci::gl::drawSolidRect(modal_area);
  }

  auto f = make_frame(modal_area);
  f.align_horizontal(horizontal_alignment_t::center);
  f.align_vertical(vertical_alignment_t::center);
  f.add(500.0f, 100.0f, [&](auto const& rect)
  {
    auto& player = m_session_info->players[m_session_info->current_player];
    auto const num_draws = player.picks_available; //player.num_draws_per_turn - player.num_drawn_this_turn;
    auto const text = stringprintf<64>("Turn %d\nPlayer %d, pick %d card%c: ", m_session_info->turn,
      m_session_info->current_player + 1, num_draws, num_draws > 1 ? 's' : ' ');
    display_text(text, rect, {0.9, 0.9, 0.9, 1.0}, rect.getHeight() / 2, true);
    auto const col = ci::gl::ScopedColor{1.0, 0.0, 0.0, 1.0};
    ci::gl::drawStrokedRect(rect);
  });

  auto g = make_grid(modal_area);
  g.set_padding(m_constants.hand_horizontal_padding, m_constants.hand_vertical_padding);
  g.set_element_size(m_constants.card_hand_width, m_constants.card_hand_height);
  g.align_vertical(vertical_alignment_t::center);
  g.align_horizontal(horizontal_alignment_t::center);

  auto [grid_x, grid_y] = g.measure(m_session_info->picks.size(), 1);

  f.add(grid_x, grid_y, [&](auto const& rect)
  {
    g.bounds = rect;    
    g.arrange_horizontally(m_session_info->picks, [&](auto const& card, auto const& element)
    {
      bool const hovered = element.contains(ci::vec2{m_constants.mouse_x, m_constants.mouse_y});
      if (hovered)
      {
        std::lock_guard lk{m_mutex};
        m_ui_action.add(uiact::hovered_pick_card, card.uid);
        m_hovered_description = ci::msw::toUtf8String(card.name);
        m_hovered_card = &card;
      }

      display_hand_card(card, element, hovered ? selection::hovered : selection::passive, true);
    });
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

void cind_display_engine::add_animation(static_animation anim) noexcept
{
  m_active_animations.emplace_back(std::move(anim));
}

void cind_display_engine::mouseDown(ci::app::MouseEvent event)
{
  std::lock_guard lk{m_mutex};

  AURA_LOG(L"+ 0x%x", m_ui_action.type);

  //if (event.isLeftDown())
  //{
  //  auto [x, y] = std::make_pair(getMousePos().x - getWindowPos().x, getMousePos().y - getWindowPos().y);

  //  ci::Rectf r{static_cast<float>(x) - 100.0f, static_cast<float>(y) - 100.0f, x + 100.0f, y + 100.0f};
  //  add_animation({r, L"Slash", std::chrono::milliseconds(400), 6});
  //}

  auto const start_animation = [&]()
  {
    auto [x, y] = std::make_pair(getMousePos().x - getWindowPos().x, getMousePos().y - getWindowPos().y);

    ci::Rectf r{static_cast<float>(x) - 100.0f, static_cast<float>(y) - 100.0f, x + 100.0f, y + 100.0f};
    add_animation({r, L"Slash", std::chrono::milliseconds(250), 6});
  };

  // Pick card
  if (m_ui_action.is(uiact::hovered_pick_card))
  {
    auto const hovered_card = m_ui_action.value(uiact::hovered_pick_card);
    m_action.set_value(make_pick_action(hovered_card));
    m_ui_action.reset_selected();
    return;
  }

  // Player hand -> lane
  if (m_ui_action.is(uiact::hovered_lane)
      && m_ui_action.is(uiact::selected_hand_card))
  {
    auto const selected_value = m_ui_action.value(uiact::selected_hand_card);
    auto const hovered_lane = m_ui_action.value(uiact::hovered_lane);
    AURA_LOG(L"Setting action with %d, %d", selected_value, hovered_lane + 1);
    m_ui_action.reset_selected();
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
    m_ui_action.reset_selected();
    return;
  }
  
  // Hand card -> Lane card
  if (m_ui_action.is(uiact::selected_hand_card)
      && m_ui_action.is(uiact::hovered_lane_card))
  {
    m_action.set_value(make_primary_action(m_ui_action.value(uiact::selected_hand_card), m_ui_action.value(uiact::hovered_lane_card)));
    m_ui_action.reset_selected();
    return;
  }

  // Hand card -> Player
  if (m_ui_action.is(uiact::selected_hand_card)
      && m_ui_action.is(uiact::hovered_player))
  {
    m_action.set_value(make_primary_action(m_ui_action.value(uiact::selected_hand_card), m_ui_action.value(uiact::hovered_player)));
    m_ui_action.reset_selected();
    return;
  }

  // Lane card -> Player
  if (m_ui_action.is(uiact::selected_lane_card)
    && m_ui_action.is(uiact::hovered_player))
  {
    m_action.set_value(make_primary_action(m_ui_action.value(uiact::selected_lane_card), m_ui_action.value(uiact::hovered_player)));
    m_ui_action.reset_selected();
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
    return;
  }

  // two consecutive clicks on same card means deselection
  if (m_ui_action.is(uiact::selected_lane_card)
    && m_ui_action.is(uiact::hovered_lane_card)
    && m_ui_action.value(uiact::selected_lane_card) == m_ui_action.value(uiact::hovered_lane_card))
  {
    m_ui_action.rm(uiact::selected_lane_card);
    return;
  }

  if (m_ui_action.is(uiact::hovered_lane_card))
  {
    auto const hovered_card = m_ui_action.value(uiact::hovered_lane_card);
    m_ui_action.reset_selected();
    m_ui_action.add(uiact::selected_lane_card, hovered_card);
    m_can_be_targetted = m_rules_engine.get_target_list(hovered_card);
    return;
  }

  if (m_ui_action.is(uiact::hovered_hand_card))
  {
    auto const hovered_card = m_ui_action.value(uiact::hovered_hand_card);
    m_ui_action.reset_selected();
    m_ui_action.add(uiact::selected_hand_card, hovered_card);
    m_can_be_targetted = m_rules_engine.get_target_list(hovered_card);
    return;
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
  //if (m_hovered_card)
  //{
  //  hideCursor();
  //  ci::Rectf r{m_constants.mouse_x, m_constants.mouse_y, m_constants.mouse_x + 10.0f, m_constants.mouse_y + 20.0f};
  //  ci::gl::draw(get_texture(L"mouse-pre-click.png"), r);
  //}
  //else if (m_mouse_texture)
  //{
  //  hideCursor();
  //  ci::Rectf r{m_constants.mouse_x, m_constants.mouse_y, m_constants.mouse_x + 10.0f, m_constants.mouse_y + 20.0f};
  //  ci::gl::draw(m_mouse_texture, r);
  //}
  //else
  //{
  //  showCursor();
  //}
}

bool cind_display_engine::display_update_animation(static_animation const& anim)
{
  auto const now = std::chrono::steady_clock::now();

  auto const time_elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - anim.time_triggered).count();
  auto const total = std::chrono::duration_cast<std::chrono::nanoseconds>(anim.duration).count();

  if (time_elapsed > total)
  {
    return true;
  }

  auto const ratio_elapsed = static_cast<float>(time_elapsed) / total;

  auto const frame_no = static_cast<int>(ratio_elapsed * anim.num_frames);

  auto const sprite_name = anim.sprite_base + std::to_wstring(frame_no) + L".png";
  AURA_LOG(L"Ratio elapsed: (%lld / %lld) %.02f, Frame: %d, Sprite: %ls", time_elapsed, total, ratio_elapsed, frame_no, sprite_name.c_str());
  if (auto const t = get_texture(sprite_name))
  {
    ci::gl::draw(t, anim.rect); 
  }
  return false;
}

void cind_display_engine::display_animations()
{
  std::vector<decltype(m_active_animations.begin())> erase_list;
  for (auto it = m_active_animations.begin(); it != m_active_animations.end(); ++it)
  {
    if (display_update_animation(*it))
    {
      erase_list.emplace_back(it); 
    }
  }
  for (auto const& e : erase_list)
  {
    m_active_animations.erase(e);
  }
  m_last_frame_time = std::chrono::steady_clock::now();
}

void cind_display_engine::draw()
{
  auto const sesh = std::invoke([&]
  {
    std::lock_guard lk{m_mutex};
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
    if (m_hovered_card)
    {
      display_hovered_card();
    }
  }
  display_mouse();
  display_animations();
}

}
