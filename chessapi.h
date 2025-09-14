// Hi! This is the C interface to the Chess API.
// Aside from allowed standard libs, you only may include this and "bitboard.h" for your bot.
// Please let me know if there are any bugs!

#include <stdbool.h>
#include "bitboard.h"

// Player color constants
#define WHITE false
#define BLACK true

// Piece constants
#define PAWN 1
#define BISHOP 2
#define KNIGHT 3
#define ROOK 4
#define QUEEN 5
#define KING 6

// Game end constants
#define GAME_NORMAL 0
#define GAME_CHECKMATE -1
#define GAME_STALEMATE 1

// The Board struct represents a single chess game
typedef struct Board Board;

// The Move struct represents a single chess move from a start location to an end location
typedef struct {
    BitBoard from;      // A BitBoard representing the origin of the move
    BitBoard to;        // A BitBorad representing the target of the move
    uint8_t promotion;  // Will be one of the BISHOP, KNIGHT, ROOK, QUEEN constants above, or 0 if not required
    bool capture;       // True if this move captures a piece
    bool castle;        // True if this move is castling
} Move;


///// BOARD-RELATED FUNCTIONS /////


// Returns the current board being played
// Caller must free the board with free_board
Board *chess_get_board();

// Returns an array of legal moves on the given [board], and store its length in [len]
// Caller must free array
Move *chess_get_legal_moves(Board *board, int *len);

// Returns true if it is currently white to move on [board]
bool chess_is_white_turn(Board *board);

// Returns true if the current player on [board] is in check
bool chess_is_check(Board *board);

// Skips your turn on the given board. You can't actually skip your turn, but it's useful for some search techniques.
// Can be un-done using undo_move() as usual.
void chess_skip_turn(Board *board);

// Returns true if in check
bool chess_in_check(Board *board);

// Returns true if in checkmate
bool chess_in_checkmate(Board *board);

// Returns true if in draw for any reason
bool chess_in_draw(Board *board);

// Returns true if [color] can castle kingside, where color is one of the Player color constants
bool chess_can_kingside_castle(Board *board, bool color);

// Returns true if [color] can castle queenside, where color is one of the Player color constants
bool chess_can_queenside_castle(Board *board, bool color);

// Returns one of the "Game end" constants for the given [board]
// (i.e. whether it is in checkmate, stalemate or neither if the game is ongoing)
// This is about the same cost as calls to in_check(), in_draw(), etc., so if you plan to check multiple you may wish to use this
int chess_is_game_ended(Board *board);

// Returns the Zobrist hash that represents [board].
uint64_t chess_zobrist_key(Board *board);

// Performs the [move] on the [board]
void chess_make_move(Board *board, Move move);

// Undo the previous move on the [board]
void chess_undo_move(Board *board);

// free() function for Boards
void chess_free_board(Board *board);

// Returns the BitBoard for the given [color] and [piece_type] from [board].
// For more info on working with BitBoards, see "bitboard.h"
BitBoard chess_get_bitboard(Board *board, int color, int piece_type);


///// MOVE SUBMISSION /////


// Submit a new [move] to the chess server to play
// You can call this more than once per turn
// The latest move pushed by the bot will be played by the server once chess_done() is called
void chess_push(Move move);

// Indicates that the engine has completed its search and ends the turn
// The latest move pushed will be played by the server
// This method will block until the opponent's turn has passed
void chess_done();


///// TIME MANAGEMENT /////


// Returns the remaining time this bot had at the start of its turn, in ms
long chess_get_time_millis();

// Returns the remaining time the opponent bot had at the start of its turn, in ms
long chess_get_opponent_time_millis();

// Returns how much time has elapsed this turn, in ms
long chess_get_elapsed_time_millis();
