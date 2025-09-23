// Hi! This is the C interface to the Chess API.
// Aside from allowed standard libs, you only may include this and "bitboard.h" for your bot.
// Please let me know if there are any bugs!

#pragma once

#include <stdbool.h>
#include "bitboard.h"

//! Player color
typedef enum {
    WHITE, /*!< The player using white pieces*/
    BLACK  /*!< The player using black pieces*/
} PlayerColor;

//! Piece type
typedef enum {
    PAWN = 1, /*!< A pawn piece*/
    BISHOP,   /*!< A bishop piece*/
    KNIGHT,   /*!< A knight piece*/
    ROOK,     /*!< A rook piece*/
    QUEEN,    /*!< A queen piece*/
    KING      /*!< A king piece*/
} PieceType;

//! Game play state
typedef enum {
    GAME_CHECKMATE = -1, /*!< Indicates the game has ended in a checkmate*/
    GAME_NORMAL,         /*!< Indicates the game has not ended yet*/
    GAME_STALEMATE       /*!< Indicates the game has ended in a draw*/
} GameState;

//! A Board represents a single chess game
typedef struct Board Board;

//! A Move represents a single chess move from a start location to an end location
typedef struct {
    BitBoard from;      /*!< A BitBoard representing the origin of the move*/
    BitBoard to;        /*!< A BitBorad representing the target of the move*/
    uint8_t promotion;  /*!< Will be one of the BISHOP, KNIGHT, ROOK, QUEEN constants above, or 0 if not required*/
    bool capture;       /*!< True if this move captures a piece*/
    bool castle;        /*!< True if this move is castling*/
} Move;

#ifdef __cplusplus
extern "C" {
#endif

///// BOARD-RELATED FUNCTIONS /////


//! Returns the current board being played
/*!
Caller must free the board with free_board
\sa chess_free_board()
\return The current board being played in the chess match
*/
Board *chess_get_board();

//! Returns a clone of the given board
/*!
The clone is not a deep clone, but uses reference counting to ensure reused shallow objects are not freed early
Caller must free the board with free_board
\sa chess_free_board()
\return A clone of the given board
*/
Board *chess_clone_board(Board *board);

//! Returns an array of legal moves
/*!
Caller must free array
\param board The board to get legal moves on
\param len A pointer in which the array length will be stored
\return A pointer to the start of an array of moves
*/
Move *chess_get_legal_moves(Board *board, int *len);

//! Returns whether it is white's turn or not
/*!
\sa chess_is_black_turn()
\param board The board to consider
\return True if it is white to move
*/
bool chess_is_white_turn(Board *board);

//! Returns whether it is black's turn or not
/*!
\sa chess_is_white_turn()
\param board The board to consider
\return True if it is black to move
*/
bool chess_is_black_turn(Board *board);

//! Skips your turn on the given board.
/*!
You can't actually skip your turn, but it's useful for some search techniques.
Can be un-done using undo_move as usual.
\sa chess_undo_move()
\param board The board to consider
*/
void chess_skip_turn(Board *board);

//! Returns whether the current player is in check
/*!
\param board The board to consider
\return True if the current player is in check
*/
bool chess_in_check(Board *board);

//! Returns whether the current player is in checkmate
/*!
\param board The board to consider
\return True if the current player is in checkmate
*/
bool chess_in_checkmate(Board *board);

//! Returns whether the current player is in a draw
/*!
This function considers positions with no legal moves, the 50-move rule, and threefold repetition as draws.
\param board The board to consider
\return True if the current game is a draw for any reason
*/
bool chess_in_draw(Board *board);


//! Returns if the indicated player has kingside castling rights
/*!
You lose kingside castling rights if you move your king or the kingside rook
\sa chess_can_queenside_castle()
\param board The board to consider
\param color The player to consider
\return True if the player has kingside castling rights
*/
bool chess_can_kingside_castle(Board *board, PlayerColor color);

//! Returns if the indicated player has queenside castling rights
/*!
You lose queenside castling rights if you move your king or the queenside rook
\sa chess_can_kingside_castle()
\param board The board to consider
\param color The player to consider
\return True if the player has queenside castling rights
*/
bool chess_can_queenside_castle(Board *board, PlayerColor color);

//! Returns one of the GameState constants for the given board
/*!
The GameState constants indicate whether the game is in checkmate, stalemate or neither (if the game is ongoing)
This is about the same cost as calls to in_check(), in_draw(), etc., so if you plan to check multiple you may wish to use this
\sa chess_in_check()
\sa chess_in_checkmate()
\sa chess_in_draw()
\param board The board to consider
\return The current GameState
*/
GameState chess_get_game_state(Board *board);

//! Returns one of the GameState constants for the given board
/*!
DEPRECATED: Use chess_get_game_state instead.
\sa chess_get_game_state()
*/
GameState chess_is_game_ended(Board *board);

//! Returns the Zobrist hash that represents the board.
/*!
Zobrist hashes are not guaranteed to be unique for all boards. Collisions are unlikely, but if you consider enough boards you should expect a collision.
The hashes consider en passant and castling possibilities as part of the hash, these will create different hashes otherwise visually identical positions
\param board The board to consider
\return The hash associated with the board
*/
uint64_t chess_zobrist_key(Board *board);

//! Performs a move on the board
/*!
\sa chess_undo_move()
\param board The board to perform the move on
\param move The move to perform
*/
void chess_make_move(Board *board, Move move);

//! Undo the previous move on the board
/*!
This function can be invoked multiple times to undo a sequence of moves
It is an error to call this function on a board which has not had any moves played on it
\sa chess_make_move()
\param board The board to undo the move from
*/
void chess_undo_move(Board *board);

//! free() function for Board instances
/*!
Board instances are invalid after being freed and should not be used after being given to this function
\param board The board to be freed
*/
void chess_free_board(Board *board);

//! Returns the BitBoard for the given color and piece type from the board.
/*!
For more info on working with BitBoards, see "bitboard.h"
\param board The board to get the bitboard of
\param color The player color whose pieces to get
\param piece_type The type of piece to get
\return A BitBoard with bits set to 1 for all squares containing the described piece
*/
BitBoard chess_get_bitboard(Board *board, PlayerColor color, PieceType piece_type);

//! Returns the full move counter for the board.
/*
This number starts at 1, and increments each time black moves.
\param board the borad to get the full move counter of
\return The current value of the full move counter
*/
int chess_get_full_moves(Board *board);

//! Returns the half move counter for the board.
/*
This number starts at 0, and increments each time black or white move.
It resets to zero each time a pawn is moved or a capture occurs.
This is mainly used for tracking progress to the 50-move draw rule.
\param board the borad to get the full move counter of
\return The current value of the full move counter
*/
int chess_get_half_moves(Board *board);


///// MOVE SUBMISSION /////


//! Submit a new move to the chess server to play.
/*!
You can call this more than once per turn.
The latest move pushed by the bot will be played by the server once chess_done() is called.
\sa chess_done()
\param move The move to submit
*/
void chess_push(Move move);

//! Ends the current turn.
/*!
The latest move pushed will be played by the server.
This method will block until the opponent's turn has passed.
\sa chess_push()
*/
void chess_done();


///// TIME MANAGEMENT /////


//! Returns the remaining time this bot had at the start of its turn, in ms.
/*!
\sa chess_get_elapsed_time_millis()
\return Remaining time, in milliseconds.
*/
uint64_t chess_get_time_millis();

//! Returns the remaining time the opponent bot had at the start of its turn, in ms.
/*!
\return Remaining time, in milliseconds.
*/
uint64_t chess_get_opponent_time_millis();

//! Returns how much time has elapsed this turn, in ms.
/*!
\sa chess_get_time_millis()
\return Elapsed time, in milliseconds.
*/
uint64_t chess_get_elapsed_time_millis();


///// BITBOARDS /////


//! Returns the type of piece on the square at the given index.
/*!
Square index travels from 0 left-to-right, bottom-to-top from white's perspective.
That is, index 0 is a1, index 7 is h1, index 63 is h8.
\sa get_piece_from_bitboard()
\param board The board the square is from.
\param index The index of the square.
\return A PieceType constant representing the type of piece on that square.
*/
PieceType chess_get_piece_from_index(Board *board, int index);

//! Returns the type of piece on the square set on the given bitboard.
/*!
This function expects a bitboard with a single bit set, such as the kind you would get from a Move struct.
\sa get_piece_from_index()
\param board The board the square is from.
\param bitboard The bitboard of the square.
\return A PieceType constant representing the type of piece on that square.
*/
PieceType chess_get_piece_from_bitboard(Board *board, BitBoard bitboard);

//! Returns the color of piece on the square at the given index.
/*!
Square index travels from 0 left-to-right, bottom-to-top from white's perspective.
That is, index 0 is a1, index 7 is h1, index 63 is h8.
\sa get_color_from_bitboard()
\param board The board the square is from.
\param index The index of the square.
\return A PlayerColor constant representing the color of piece on that square.
*/
PlayerColor chess_get_color_from_index(Board *board, int index);

//! Returns the color of piece on the square set on the given bitboard.
/*!
This function expects a bitboard with a single bit set, such as the kind you would get from a Move struct.
\sa get_color_from_index()
\param board The board the square is from.
\param bitboard The bitboard of the square.
\return A PlayerColor constant representing the color of piece on that square.
*/
PlayerColor chess_get_color_from_bitboard(Board *board, BitBoard bitboard);

//! Returns a square index equivalent to the square indicated by the given bitboard.
/*!
This function expects a bitboard with a single bit set, such as the kind you would get from a Move struct.
\sa get_bitboard_from_index()
\param bitboard The bitboard to get the square index of.
\return An index from 0-63 indicating the set square.
*/
int chess_get_index_from_bitboard(BitBoard bitboard);

//! Returns a bitboard equivalent to the square indicated by the given index.
/*!
Square index travels from 0 left-to-right, bottom-to-top from white's perspective.
That is, index 0 is a1, index 7 is h1, index 63 is h8.
\sa get_index_from_bitboard()
\param index The square index to get the bitboard of.
\return A bitboard with a single bit set on the associated square.
*/
BitBoard chess_get_bitboard_from_index(int index);


///// OTHER /////


//! Returns the last move made by the opponent.
/*!
If no moves have been made yet, the returned move will have all its fields set to zero.
\return The move made by the opponent on the last ply.
*/
Move chess_get_opponent_move();

//! Free function for an array of moves.
/*!
This is intended for move arrays such as the one returned from get_legal_moves.
Move arrays are invalid after being given to this function and should not be used after.
Under the hood this is just a normal free(), but it's convenient for external bindings to include it here.
\param moves A pointer to the move array to free
*/
void chess_free_moves_array(Move *moves);

#ifdef __cplusplus
}
#endif