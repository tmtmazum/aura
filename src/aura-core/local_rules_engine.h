#include <aura-core/rules_engine.h>
#include <aura-core/session_info.h>
#include <aura-core/ruleset.h>

namespace aura
{

class local_rules_engine : public rules_engine
{
public:
  local_rules_engine(ruleset const& rs);

  bool is_game_over() const noexcept override { return m_game_over; }

  session_info get_session_info() const { return m_session_info; }

  std::vector<int> get_target_list(int uid) const;

  //! Check if a player action is legal

  //! Commit a player action
  std::error_code commit_action(player_action const&) override;

private:
  bool m_game_over{false};
  session_info m_session_info;
  ruleset m_rules;
};

} // namespace aura
