#pragma once

namespace aura
{

class game_state;

class card
{
public:
	virtual void on_deploy(game_state& b, int lane_no, int pos);

	virtual void on_action(game_state& b, card& target);

	virtual void on_position_updated(game_state& b, int lane_no, int pos);

	virtual void on_death(game_state& b);
};

} // namespace aura
