// This is the C interface to the Chess API

#include <stdbool.h>
#include "bitboard.h"

// Player color constants
#define WHITE 0
#define BLACK 1

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

// The Board struct represents a single chessboard state
typedef struct Board Board;

// The Move struct represents a single chess move from a start location to an end location
// [to] and [from] are BitBoards with a single bit set, indicating the positions of the destination and origin squares
// [promotion] is a piece constant, determines what the new piece will be for a pawn promotion
// [capture] is a bool indicating if this piece is a capture or not
typedef struct {
    BitBoard from;
    BitBoard to;
    char promotion;
    bool capture;
    bool castle;
} Move;

// Returns the current board being played
// Caller must free the board with free_board
Board *chess_get_board();

// Returns an array of legal moves on the given [board], and store its length in [len]
// Caller must free array
Move *chess_get_legal_moves(Board *board, int *len);

// Returns true if it is currently white to move on [board]
bool chess_is_white_turn(Board *board);

// Returns one of the "Game end" constants for the given [board]
// (i.e. whether it is in checkmate, stalemate or neither)
int chess_is_game_ended(Board *board);

// Performs the [move] on the [board]
void chess_make_move(Board *board, Move move);

// Undoes the previous move on the [board]
void chess_undo_move(Board *board);

// free() function for Boards
void chess_free_board(Board *board);

// Returns the remaining time the bot had at the start of its turn, in millis
long chess_get_time_millis();

// Submit a new [move] to the chess server to play
// You can call this more than once per turn
// The latest move pushed by the bot will be played by the server
void chess_push(Move move);

// Indicates that the engine has completed its search
// The latest move pushed will be played by the server
// This method will block until the opponent's turn has passed
void chess_done();

// Returns the BitBoard for the given [color] and [piece_type] from [board].
// For more info on working with BitBoards, see "bitboard.h"
BitBoard chess_get_bitboard(Board *board, int color, int piece_type);

// Returns true if the current player on [board] is in check
bool chess_is_check(Board *board);
