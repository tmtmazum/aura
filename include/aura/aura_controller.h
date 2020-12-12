
// game types
class local_1v1;

class game_session
{

};

class controller
{
	template <typename GameType>
	std::unique_ptr<game_session> create_game();
};

