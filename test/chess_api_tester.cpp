#include <iostream>
#include <string>
#include <sstream>
#include <set>

#include <chessapi.h>

int main()
{
	std::string fen;

	int return_value = 0;
	
	while (std::getline(std::cin, fen))
	{
		if (fen[0] == '#')
		{
			// Comment line - skip
			continue;
		}

		std::string moves;

		do
		{
			if (!std::getline(std::cin, moves))
			{
				std::cerr << "Unexpected EOF" << std::endl;
				return 2;
			}

		} while(moves[0] == '#');

		std::stringstream ss(moves);
		std::string move;
		std::set<std::string> move_set;
		
		while (ss >> move)
		{
			move_set.insert(move);
		}

		Board *board = chess_board_from_fen(fen.c_str());

		int num_api_moves;
		Move *api_moves = chess_get_legal_moves(board, &num_api_moves);

		bool mismatch_found = false;
		for (int i = 0; i < num_api_moves; ++i)
		{
			char buffer[8];
			chess_dump_move(buffer, api_moves[i]);

			std::string api_move(buffer);
			if (!move_set.contains(api_move))
			{
				std::cout << "# Illegal move \"" << api_move << "\":" << std::endl;
				mismatch_found = true;
			}
			else
			{
				move_set.erase(api_move);
			}				
		}

		if (move_set.size() > 0)
		{
			for (auto move : move_set)
			{
				std::cout << "# Missing move \"" << move << "\":" << std::endl;
				mismatch_found = true;
			}
		}
		
		if (mismatch_found)
		{
			// Dump board with mismatch for further testing
			std::cout << fen << std::endl;
			std::cout << moves << std::endl;
			return_value = 1;
		}

		free(api_moves);
		chess_free_board(board);
	}
	
	return return_value;
}
