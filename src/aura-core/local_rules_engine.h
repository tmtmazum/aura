#include <aura-core/rules_engine.h>
#include <aura-core/session_info.h>
#include <aura-core/ruleset.h>
#include <aura-core/terrain_types.h>
#include <unordered_map>

namespace aura
{

class local_rules_engine : public rules_engine
{
public:
  local_rules_engine(ruleset const& rs);

  bool is_game_over() const noexcept override { return m_session_info.game_over; }

  session_info get_session_info() const;

  std::vector<int> get_target_list(int uid) const;

  //! Check if a player action is legal

  //! Commit a player action
  std::error_code commit_action(player_action const&) override;

  std::wstring describe(unit_traits trait) const noexcept override;

  card_info to_card_info(card_preset const& preset, int cid) override;

  std::error_code trigger_pick_action(int num_picks, int num_choices) override;
  std::error_code ready_draft_picks();
  std::error_code trigger_draft_pick();

private:
  card_info* find_actor(int uid);
  card_info* find_target(int uid);

  void add_specials(player_info& player);

  card_info generate_card(ruleset const& r, deck& d, int turn = 1);

  terrain_t generate_terrain();

  void apply_terrain_modifiers(int cur_player, int lane_num, int tile_num, card_info& card) const;
  void apply_all_terrain_modifiers(session_info& sesh) const;

private:
  session_info m_session_info;
  ruleset m_rules;
  bool end_of_turn{false};

  std::unordered_map<int, card_action_t> m_primary_actions;
  std::unordered_map<int, card_action_t> m_deploy_actions;
  std::unordered_map<int, card_action_t> m_death_actions;

  std::vector<card_info> m_draft_choices;
  bool                   m_starting_drafts{true};

  //using primary_action_t = std::function<std::error_code(card_info& actor, card_info& target)>;
  // Card uid -> primary action
  //std::unordered_map<int, primary_action_t> m_primary_actions;
};

} // namespace aura
