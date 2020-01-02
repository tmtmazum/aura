#include <aura-core/build.h>
#include "cli_display_engine.h"
#include <aura-core/session_info.h>
#include <cstdio>
#include <iostream>

namespace aura
{

void cli_display_engine::clear_board()
{

}

void split_helper(std::wstring_view const& line, std::vector<std::wstring_view>& vs)
{
  auto const p = line.find_first_of(L" \n");
  if (p == line.npos)
  {
    vs.emplace_back(line);
    return;
  }
  auto copy = line;
  copy.remove_suffix(copy.size() - p);
  vs.emplace_back(std::move(copy));

  auto copy2 = line;
  copy2.remove_prefix(p + 1);
  split_helper(copy2, vs);
  return;
}

std::vector<std::wstring_view> split(std::wstring const& line)
{
  std::vector<std::wstring_view> vs;

  split_helper(line, vs);
  return vs;
}

void print_help()
{
  
}

player_action get_args2(action_type type, std::wstring types_expected, std::vector<std::wstring_view> parts)
{
  player_action act{};
  act.type = type;

  try
  {
    //if (parts.at(1).front() != types_expected.at(0))
    //{
    //  AURA_LOG(L"Incorrect type '%ls', expected '%lc'", std::wstring{parts.at(1)}.c_str(), types_expected.at(0));
    //}
    //parts.at(1).remove_prefix(1);
    act.target1 = std::stoi(std::wstring{parts.at(1)});

    //if (parts.at(2).front() != types_expected.at(1))
    //{
    //  AURA_LOG(L"Incorrect type '%ls', expected '%lc'", std::wstring{parts.at(2)}.c_str(), types_expected.at(1));
    //}
    //parts.at(2).remove_prefix(1);
    act.target2 = std::stoi(std::wstring{parts.at(2)});
  }
  catch(std::exception const& e)
  {
    AURA_PRINT(L"Expected more arguments with command");
  }
  return act;
}

void print_board_quick(session_info const& info)
{
  AURA_PRINT(L"Turn: %d\n", info.turn);

  for (int i = 0; i < info.players.size(); ++i)
  {
    auto const& p = info.players[i];
    AURA_PRINT(L"Player %d <u%d> | ", i + 1, info.players[i].uid);
    AURA_PRINT(L"Health: (%d/%d) | ", info.players[i].health,
               info.players[i].starting_health);
    AURA_PRINT(L"Mana: %d \n", info.players[i].mana);
    AURA_PRINT(L"Cards in Hand :- \n");
    
    for (auto const& card : p.hand)
    {
      AURA_PRINT(L"[%d %-.10ls (%d/%d) <u%d>]\n", card.cost, card.name.c_str(), card.effective_strength(), card.effective_health(), card.uid);
    }
    AURA_PRINT(L"\nCards in Play:- \n");
    for (int j = 0; j < p.lanes.size(); ++j)
    {
      AURA_PRINT(L"Lane %d: ", j + 1);
      auto const& lane = p.lanes[j];
      for (auto const& card : lane)
      {
        AURA_PRINT(L"[%d %.10ls (%d/%d) <u%d>] ", card.cost, card.name.c_str(), card.effective_strength(), card.effective_health(), card.uid);
      }
      AURA_PRINT(L"\n");
    }
    AURA_PRINT(L"\n\n");
  }
}

std::wstring refit(std::wstring_view view, size_t size)
{
  auto copy = std::wstring{view};
  while (copy.size() < size)
  {
    copy.push_back(' ');
  }

  while (copy.size() > size)
  {
    copy.pop_back();
  }
  return copy;
}

void print_board_card(card_info const& card)
{
  AURA_PRINT(L"[<u%d> %ls %lc(%d/%d)]", card.uid, refit(card.name, 9).c_str(),
             card.is_resting() && card.strength ? L'R' : ' ', card.effective_strength(),
             card.effective_health());
}

void print_board_player(player_info const& p, int i)
{
    AURA_PRINT(L"[<u%d> %*ls %d (%2d/%2d)%-*lc]", p.uid, 38, L"Player", i + 1, p.health, p.starting_health, 37, L' ');
}

void print_board_pretty(session_info const& info)
{
  AURA_PRINT(L"Turn: %d\n\n", info.turn);

  auto i = 0;

  {
    auto const& p = info.players[i];
    auto const is_current_player = (info.current_player == i);
    if (is_current_player)
    {
      AURA_PRINT(L"Player %d <u%d> | ", i + 1, info.players[i].uid);
      AURA_PRINT(L"Health: (%d/%d) | ", info.players[i].health,
                 info.players[i].starting_health);
      AURA_PRINT(L"Mana: %d \n", info.players[i].mana);
      AURA_PRINT(L"Cards in Hand :- \n");
      for (auto const& card : p.hand)
      {
        AURA_PRINT(L"[<u%d> %ls (%d/%d) Cost:%d]\n", card.uid, card.name.c_str(), card.strength, card.health, card.cost);
      }
    }

    AURA_PRINT(L"\nCards in Play:- \n");
    
    print_board_player(p, i);
    //AURA_PRINT(L"[<u%d> %*ls %d (%2d/%2d)%-*lc]", p.uid, 38, L"Player", i + 1, p.health, p.starting_health, 37, L' ');
    AURA_PRINT(L"\n");
    auto const num_lanes = p.lanes.size();
    for (int i = 0; i < num_lanes; ++i)
    {
      AURA_PRINT(L"[%*ls %-*d]", 10, L"Lane", 10, i + 1);
    }
    AURA_PRINT(L"\n");


    for (int j = 0; j < 4; ++j)
    {
      for (int i = 0; i < num_lanes; ++i)
      {
        if (p.lanes[i].size() <= j)
        {
          AURA_PRINT(L"[%21ls]", L" ");
          continue;
        }
        auto const& card = p.lanes[i][j];
        print_board_card(card);
        //AURA_PRINT(L"[<u%d> %ls (%d/%d)]", card.uid, refit(card.name, 10).c_str(), card.strength, card.health);
      }
      AURA_PRINT(L"\n");
    }
  }

  {
    i = 1;
    auto const& p = info.players[i];
    
    auto const num_lanes = p.lanes.size();
    for (int j = 3; j >= 0; --j)
    {
      for (int i = 0; i < num_lanes; ++i)
      {
        if (p.lanes[i].size() <= j)
        {
          AURA_PRINT(L"[%21ls]", L" ");
          continue;
        }
        auto const& card = p.lanes[i][j];
        print_board_card(card);
        //AURA_PRINT(L"[<u%d> %ls (%d/%d)]", card.uid, refit(card.name, 10).c_str(), card.strength, card.health);
      }
      AURA_PRINT(L"\n");
    }
    for (int i = 0; i < num_lanes; ++i)
    {
      AURA_PRINT(L"[%*ls %-*d]", 10, L"Lane", 10, i + 1);
    }
    AURA_PRINT(L"\n");
    print_board_player(p, i);
    AURA_PRINT(L"\n\n");
    auto const is_current_player = (info.current_player == i);
    if (is_current_player)
    {
      AURA_PRINT(L"Player %d <u%d> | ", i + 1, info.players[i].uid);
      AURA_PRINT(L"Health: (%d/%d) | ", info.players[i].health,
                 info.players[i].starting_health);
      AURA_PRINT(L"Mana: %d \n", info.players[i].mana);
      AURA_PRINT(L"Cards in Hand :- \n");
      for (auto const& card : p.hand)
      {
        AURA_PRINT(L"[<u%d> %ls (%d/%d) Cost:%d]\n", card.uid, refit(card.name, 8).c_str(), card.strength, card.health, card.cost);
      }
    }
  }
  AURA_PRINT(L"\n");
}

player_action cli_display_engine::display_session(std::shared_ptr<session_info> info_in, bool redraw)
{
  AURA_ENTER();

  auto const& info = *info_in;

  if (redraw)
  {
    print_board_pretty(info);
  }

  while(1)
  {
    AURA_PRINT(L"P%d>", info.current_player + 1);
    std::wstring line;
    std::getline(std::wcin, line);

    auto const parts = split(line);
    auto const& cmd = parts[0];
    if (cmd == L"forfeit")
    {
      AURA_PRINT(L"Player %d forfeits match..\n", info.current_player + 1);
      player_action act{};
      act.type = action_type::forfeit;
      return act;
    }
    else if(cmd == L"pass" || cmd == L"s")
    {
      AURA_PRINT(L"Player %d ends their turn..\n", info.current_player + 1);
      player_action act{};
      act.type = action_type::end_turn;
      return act;
    }
    else if(cmd == L"deploy" || cmd == L"d")
    {
      return get_args2(action_type::deploy, L"ul", parts);
    }
    else if(cmd == L"redraw")
    {
      return display_session(std::move(info_in), redraw);
    }
    else if(cmd == L"action" || cmd == L"a")
    {
      return get_args2(action_type::primary_action, L"uu", parts);
    }
    else
    {
      AURA_PRINT(L"Unrecognized commend '%ls'\n", std::wstring{cmd}.c_str());
    }
  }
}

} // namespace aura
