#include "cind_display_engine.h"
#include <cinder/msw/CinderMsw.h>
#include <aura-core/session_info.h>
#include <aura-core/build.h>

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

std::string format_card_lane(card_info const& card)
{
  std::string s{"["};
  s += std::to_string(card.cost);
  s += "] ";
  s += ci::msw::toUtf8String(card.name);
  s += "\n---";
  s += "\nStrength: ";
  s += std::to_string(card.strength);
  s += "\nHealth: ";
  s += std::to_string(card.health);
  return s;
}


std::string format_card_hand(card_info const& card)
{
  std::string s{"["};
  s += std::to_string(card.cost);
  s += "] ";
  s += ci::msw::toUtf8String(card.name);
  s += "\n---";
  s += "\nStrength: ";
  s += std::to_string(card.strength);
  s += "\nHealth: ";
  s += std::to_string(card.health);
  return s;
}

auto to_string(std::wstring const& ws)
{
  return ci::msw::toUtf8String(ws);
}

} // namespace {}

void cind_display_engine::display_hand_card(card_info const& card, ci::Rectf const& rect, selection sel)
{
  auto const col = std::invoke([&]
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

  {
    ci::gl::ScopedColor scoped_col{col};
    ci::gl::drawSolidRoundedRect(rect, 10, 10);
  }

  ci::TextBox box;
  box.font(ci::Font{box.getFont().getName(), m_constants.card_font_point});

  box.text(format_card_hand(card));
  box.setColor({0.8, 0.8, 1.0, 1.0});
  box.size(rect.getSize());
  auto const surf = box.render({2, 2});
  auto const textr = ci::gl::Texture::create(surf);
  ci::gl::draw(textr, rect);
}

void cind_display_engine::display_hand(
  std::vector<card_info> const &hand,
  ci::Rectf const &bounds)
{
  auto const num_hand_cards = hand.size();
  auto [cur_pos_x, cur_pos_y] = std::make_pair(bounds.getX1(), bounds.getY1());
  cur_pos_y += m_constants.hand_vertical_padding;
  for (int i = 0; i < num_hand_cards; ++i)
  {
    cur_pos_x += m_constants.hand_horizontal_padding;
    ci::Rectf rect{cur_pos_x, cur_pos_y, cur_pos_x + m_constants.card_hand_width,
                   cur_pos_y + m_constants.card_hand_height};

    auto const& card = hand[i];

    auto const item_selected = std::invoke([&]
    {
      std::lock_guard lk{m_mutex};
      return m_selected;
    });

    // draw card background
    auto const sel = std::invoke([&]
    {
      bool selected = item_selected && (item_selected.value() == card.uid);
      if (selected)
      {
        return selection::selected;
      }

      bool hovered = rect.contains(ci::ivec2{m_constants.mouse_x, m_constants.mouse_y});
      if (hovered)
      {
        std::lock_guard lk{m_mutex};
        m_hovered = card.uid;
        m_in_hand = true;
        return selection::hovered;
      }

      return selection::passive;
    });

    display_hand_card(card, rect, sel);

    cur_pos_x += m_constants.card_hand_width;
    cur_pos_x += m_constants.hand_horizontal_padding;
  }
}

void cind_display_engine::display_text(std::string const& text, ci::Rectf const& rect, ci::ColorAf const& col)
{
    ci::TextBox box;
    box.font(ci::Font{box.getFont().getName(), m_constants.card_font_point});

    box.text(text);
    box.setColor(col);
    box.size(rect.getSize());
    auto const surf = box.render({2, 2});
    auto const textr = ci::gl::Texture::create(surf);
    ci::gl::draw(textr, rect);
}

void cind_display_engine::display_lane_marker(int i, ci::Rectf const& rect)
{
  display_text(format_lane_marker(i), rect, {0.3, 0.3, 0.3, 1.0});
}

void cind_display_engine::display_lane_card(card_info const& card, ci::Rectf const& rect, selection sel)
{
    {
      ci::gl::ScopedColor col{0.1, 0.1, 0.1, 1.0};
      ci::gl::drawSolidRoundedRect(rect, 10, 10);
    }

    ci::TextBox box;
    box.font(ci::Font{box.getFont().getName(), m_constants.card_font_point});

    box.text(format_card_lane(card));
    box.setColor({0.8, 0.8, 1.0, 1.0});
    box.size(rect.getSize());
    auto const surf = box.render({2, 2});
    auto const textr = ci::gl::Texture::create(surf);
    ci::gl::draw(textr, rect);
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

  auto lane_start_x = static_cast<float>(bounds.x1 /*+ (remaining_width / 2)*/);
  auto [cur_pos_x, cur_pos_y] = std::make_pair(lane_start_x, bounds.y1);

  {
    auto const [y1, y2] = std::minmax(cur_pos_y, cur_pos_y + (sign * m_constants.board_lane_marker_height));
    ci::Rectf rect{cur_pos_x + (remaining_width / 2), y1,  cur_pos_x + (remaining_width / 2) + 500, y2};

    auto const str = stringprintf<30>("Player | Health: %d | Mana: %d", player.health, player.mana);
    display_text(str, rect, {0.9, 0.9, 0.9, 1.0});

    cur_pos_y += (sign * (m_constants.board_lane_marker_height + m_constants.board_vertical_padding));
  }

  {
    auto const [y1, y2] = std::minmax(cur_pos_y, cur_pos_y + (sign * m_constants.board_lane_marker_height));
    ci::Rectf rect{cur_pos_x + (remaining_width / 2), y1,  cur_pos_x + (remaining_width / 2) + 120, y2};

    if (is_current)
    {
      if (rect.contains({m_constants.mouse_x, m_constants.mouse_y}))
      {
        ci::gl::ScopedColor col{1.0, 0.0, 0.0, 1.0};
        ci::gl::drawSolidRoundedRect(rect, 10, 10);
      }
      else
      {
        ci::gl::ScopedColor col{0.4, 0.4, 0.4, 1.0};
        ci::gl::drawSolidRoundedRect(rect, 10, 10);
      }
      display_text(std::string{"END TURN"}, rect, {0.9, 0.9, 0.9, 1.0});
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
    cur_pos_y = starting_y;
    for (int j = 0; j < player.lanes[i].size() + 1; ++j)
    {
      cur_pos_y += sign * m_constants.board_vertical_padding;
      cur_pos_x += m_constants.board_horizontal_padding;

      auto [y1, y2] = std::minmax(cur_pos_y, cur_pos_y + (sign * m_constants.card_board_height));
      ci::Rectf rect{cur_pos_x, y1, cur_pos_x + m_constants.card_board_width, y2};

      if (j == player.lanes[i].size())
      {
        if (rect.contains(ci::vec2{m_constants.mouse_x, m_constants.mouse_y}) && m_selected && is_current)
        {
          ci::gl::ScopedColor col{m_constants.hand_hovered_color};
          ci::gl::drawSolidRoundedRect(rect, 10, 10);
          std::lock_guard lk{m_mutex};
          m_hovered_lane = i;
        }
      }
      else
      {
        auto const& card = player.lanes[i][j];
        display_lane_card(card, rect, selection::passive);
      }

      cur_pos_x += m_constants.card_board_width + m_constants.board_horizontal_padding;
      cur_pos_y += (sign * m_constants.card_board_height);
      cur_pos_y += (sign * m_constants.board_vertical_padding);
    }
  }
}

void cind_display_engine::display_player_top(player_info const& player, bool is_current)
{
  auto [cur_pos_x, cur_pos_y] = std::make_pair(0.0f, 0.0f);
  auto const hand_height = static_cast<float>(m_constants.card_hand_height + (2*m_constants.hand_vertical_padding));
  display_hand(player.hand, ci::Rectf{cur_pos_x, cur_pos_y, 500.0f, hand_height});

  cur_pos_y += hand_height;

  display_lanes(player, ci::Rectf{cur_pos_x, cur_pos_y, 0, 0}, is_current, false);
}

void cind_display_engine::display_player_bottom(player_info const& player, bool is_current)
{
  auto [cur_pos_x, cur_pos_y] = std::make_pair(0.0f, 0.0f);
  auto const hand_height = static_cast<float>(m_constants.card_hand_height + (2*m_constants.hand_vertical_padding));
  cur_pos_y = m_constants.window_height - hand_height;
  display_hand(player.hand, ci::Rectf{cur_pos_x, cur_pos_y, 500.0f, hand_height});

  cur_pos_y = cur_pos_y - hand_height;
  display_lanes(player, ci::Rectf{cur_pos_x, cur_pos_y, 0, 0}, is_current, true);
  //display_lanes_top(player, m_constants, ci::Rectf{cur_pos_x, cur_pos_y, 0, 0});
}

void cind_display_engine::mouseDown(ci::app::MouseEvent event)
{
  std::lock_guard lk{m_mutex};
  if (m_hovered_lane && m_selected)
  {
    AURA_LOG(L"Setting action with %d, %d", m_selected.value(), m_hovered_lane.value() + 1);
    m_action.set_value(make_deploy_action(m_selected.value(), m_hovered_lane.value() + 1));
  }

  if (m_hovered)
  {
    auto const val = m_hovered.value();
    m_selected = val;
    m_can_be_targetted = m_rules_engine.get_target_list(val);
  }
}

void cind_display_engine::draw()
{
  auto const sesh = std::invoke([&]
  {
    std::lock_guard lk{m_mutex};
    m_hovered.reset();
    m_in_hand = false;
    m_hovered_lane.reset();

    auto const ws = getWindowBounds().getSize();
    m_constants.window_width = ws.x;
    m_constants.window_height = ws.y;
    m_constants.mouse_x = getMousePos().x - getWindowPos().x;
    m_constants.mouse_y = getMousePos().y - getWindowPos().y;
    return m_session_info;
  });

  ci::gl::clear();

  if (!sesh)
  {
    return;
  }

  assert(sesh->current_player == 0 || sesh->current_player == 1);
  display_player_top(sesh->players[0], sesh->current_player == 0);
  display_player_bottom(sesh->players[1], sesh->current_player == 1);
}

}

