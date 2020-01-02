#pragma once

#include <aura-core/ruleset.h>
#include <string>

namespace aura
{

namespace rest
{

template <typename T>
struct server_response
{

};

// POST
struct new_session
{
  struct in
  {
    game_mode mode; 
  };

  struct out
  {
    int session_id; //!< unique identifier for this session
    std::string matched_player_name;
  };

  static std::string to_string(in const&) noexcept;
  static std::string to_string(out const&) noexcept;

  static std::pair<std::error_code, in> to_in(std::string const&) noexcept;
  static std::pair<std::error_code, out> to_out(std::string const&) noexcept;

  //! Called by client
  static server_response<out> make_request(in const& info_in);

  //! Called by server
  static std::string handle_request(std::string const& info_in);
};

// GET
struct get_session_info
{
  struct in
  {
    int session_id;
  };

  struct out
  {
    session_info session;
  };

  static std::string to_string(in const&) noexcept;
  static std::string to_string(out const&) noexcept;

  static std::pair<std::error_code, in> to_in(std::string const&) noexcept;
  static std::pair<std::error_code, out> to_out(std::string const&) noexcept;

  //! Called by client
  static server_response<out> make_request(in const& info_in);

  //! Called by server
  static std::string handle_request(std::string const& info_in);
};

// POST
struct commit_action
{
  struct in
  {
    player_action action;
  };

  struct out
  {
    std::error_code error;
  };

  static std::string to_string(in const&) noexcept;
  static std::string to_string(out const&) noexcept;

  static std::pair<std::error_code, in> to_in(std::string const&) noexcept;
  static std::pair<std::error_code, out> to_out(std::string const&) noexcept;

  //! Called by client
  static server_response<out> make_request(in const& info_in);

  //! Called by server
  static std::string handle_request(std::string const& info_in);
};

} // namespace rest

} // namespace aura
