#include "cind_display_engine.h"
#include <cinder/msw/CinderMsw.h>
#include <aura-core/session_info.h>

namespace aura
{

struct display_constants
{
  float card_board_width = 120;
  float card_board_height = 80;
  float card_hand_width = card_board_width;
  float card_hand_height = card_board_height;

  float hand_horizontal_padding = 5;
  float hand_vertical_padding = 10;

  float board_horizontal_padding = 20;
  float board_vertical_padding = 10;

  float board_lane_marker_height = 20;

  float card_font_point = 18.0;

  float window_width{};
  float window_height{};

  float mouse_x{};
  float mouse_y{};

  ci::ColorAf hand_card_color{0.1, 0.1, 0.1, 1.0};
  ci::ColorAf hand_hovered_color{0.15, 0.15, 0.15, 1.0};
  ci::ColorAf hand_selected_color{0.2, 0.2, 0.2, 1.0};
};

player_action cind_display_engine::display_session(std::shared_ptr<session_info> info, bool redraw)
{
  {
    std::lock_guard lk{m_mutex};
    m_session_info = std::move(info);
  }
  player_action act{};
  act.type = action_type::no_action;
  return act;

  CI_LOG_D("Waiting at display_session(..)");

  std::this_thread::sleep_for(std::chrono::minutes(5));
  int a;
  std::cin >> a;
  return player_action{};
}

namespace
{

template <typename... Args>
std::string stringprintf(char const* format, Args&&... args)
{
  char buf[20]{};
  auto const e = sprintf_s(buf, 20, format, std::forward<Args>(args)...);
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

void display_hand(
  std::vector<card_info> const &hand,
  display_constants const &constants,
  ci::Rectf const &bounds)
{
  auto const num_hand_cards = hand.size();
  auto [cur_pos_x, cur_pos_y] = std::make_pair(bounds.getX1(), bounds.getY1());
  cur_pos_y += constants.hand_vertical_padding;
  for (int i = 0; i < num_hand_cards; ++i)
  {
    cur_pos_x += constants.hand_horizontal_padding;
    ci::Rectf rect{cur_pos_x, cur_pos_y, cur_pos_x + constants.card_hand_width,
                   cur_pos_y + constants.card_hand_height};

    // draw card background
    {
      bool hovered = rect.contains(ci::ivec2{constants.mouse_x, constants.mouse_y});
      auto const col = hovered ? constants.hand_hovered_color : constants.hand_card_color;
        
      ci::gl::ScopedColor scoped_col{col};
      ci::gl::drawSolidRoundedRect(rect, 10, 10);
    }

    ci::TextBox box;
    box.font(ci::Font{box.getFont().getName(), constants.card_font_point});

    auto const& card = hand[i];
    box.text(format_card_hand(card));
    box.setColor({0.8, 0.8, 1.0, 1.0});
    box.size(rect.getSize());
    auto const surf = box.render({2, 2});
    auto const textr = ci::gl::Texture::create(surf);
    ci::gl::draw(textr, rect);

    cur_pos_x += constants.card_hand_width;
    cur_pos_x += constants.hand_horizontal_padding;
  }
}

void display_lanes_top(player_info const& player, display_constants const& constants, ci::Rectf const& bounds)
{
  auto const num_lanes = player.lanes.size();

  auto const width_needed = 
    (constants.card_board_width * 4) +
    (num_lanes * 2 * constants.board_horizontal_padding);

  //CI_LOG_D(constants.window_width << ", " << constants.window_height);
  auto const remaining_width = constants.window_width - width_needed;

  auto lane_start_x = static_cast<float>(bounds.x1 /*+ (remaining_width / 2)*/);
  auto [cur_pos_x, cur_pos_y] = std::make_pair(lane_start_x, bounds.y1);
  
  for (int i = 0; i < num_lanes; ++i)
  {
    cur_pos_x += constants.board_horizontal_padding;
    ci::Rectf rect{cur_pos_x, cur_pos_y, cur_pos_x + constants.card_board_width,
                   cur_pos_y + static_cast<float>(constants.board_lane_marker_height)};

    ci::TextBox box;
    box.font(ci::Font{box.getFont().getName(), constants.card_font_point});

    box.text(format_lane_marker(i));
    box.setColor({0.3, 0.3, 0.3, 1.0});
    box.size(rect.getSize());
    box.alignment(ci::TextBox::Alignment::CENTER);
    auto const surf = box.render({2, 2});
    auto const textr = ci::gl::Texture::create(surf);
    ci::gl::draw(textr, rect);

    cur_pos_x += constants.card_board_width;
    cur_pos_x += constants.board_horizontal_padding;
  }

  cur_pos_x = lane_start_x;
  cur_pos_y += constants.board_lane_marker_height + constants.board_vertical_padding;
  for (int i = 0; i < num_lanes; ++i)
  {
    for (int j = 0; j < player.lanes[i].size(); ++j)
    {
      cur_pos_y += constants.board_vertical_padding;
      cur_pos_x += constants.board_horizontal_padding;

      ci::Rectf rect{cur_pos_x, cur_pos_y, cur_pos_x + constants.card_board_width,
        cur_pos_y + constants.card_board_height};

      {
        ci::gl::ScopedColor col{0.1, 0.1, 0.1, 1.0};
        //ci::gl::ScopedColor col{0.1, 0.1, 0.1, 1.0};
        ci::gl::drawSolidRoundedRect(rect, 10, 10);
      }

      ci::TextBox box;
      box.font(ci::Font{box.getFont().getName(), constants.card_font_point});

      auto const& card = player.lanes[i][j];
      box.text(format_card_lane(card));
      box.setColor({0.8, 0.8, 1.0, 1.0});
      box.size(rect.getSize());
      auto const surf = box.render({2, 2});
      auto const textr = ci::gl::Texture::create(surf);
      ci::gl::draw(textr, rect);


      cur_pos_y += constants.card_board_height;
      cur_pos_y += constants.board_vertical_padding;
    }
  }
}

void display_player_top(player_info const& player, display_constants const& constants)
{
  auto [cur_pos_x, cur_pos_y] = std::make_pair(0.0f, 0.0f);
  auto const hand_height = static_cast<float>(constants.card_hand_height + (2*constants.hand_vertical_padding));
  display_hand(player.hand, constants, ci::Rectf{cur_pos_x, cur_pos_y, 500.0f, hand_height});

  cur_pos_y += hand_height;

  display_lanes_top(player, constants, ci::Rectf{cur_pos_x, cur_pos_y, 0, 0});
}

void display_player_bottom(player_info const& player, display_constants const& constants)
{
  auto [cur_pos_x, cur_pos_y] = std::make_pair(0.0f, 0.0f);
  auto const hand_height = static_cast<float>(constants.card_hand_height + (2*constants.hand_vertical_padding));
  cur_pos_y = constants.window_height - hand_height;
  display_hand(player.hand, constants, ci::Rectf{cur_pos_x, cur_pos_y, 500.0f, hand_height});

  //display_lanes_top(player, constants, ci::Rectf{cur_pos_x, cur_pos_y, 0, 0});
}

} // namespace {}

void cind_display_engine::draw()
{
  auto const sesh = std::invoke([&]
  {
    std::lock_guard lk{m_mutex};
    return m_session_info;
  });

  ci::gl::clear();

  if (!sesh)
  {
    return;
  }

  display_constants constants{};

  auto const ws = getWindowBounds().getSize();
  constants.window_width = ws.x;
  constants.window_height = ws.y;
  constants.mouse_x = getMousePos().x - getWindowPos().x;
  constants.mouse_y = getMousePos().y - getWindowPos().y;
  display_player_top(sesh->players[0], constants);
  display_player_bottom(sesh->players[1], constants);
}

}

