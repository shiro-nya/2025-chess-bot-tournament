from cffi import FFI
ffibuilder = FFI()

ffibuilder.cdef(r"""
void free(void *ptr);
typedef uint64_t BitBoard;
typedef enum {
    WHITE,
    BLACK
} PlayerColor;
typedef enum {
    PAWN = 1,
    BISHOP,
    KNIGHT,
    ROOK,
    QUEEN,
    KING
} PieceType;
typedef enum {
    GAME_CHECKMATE = -1,
    GAME_NORMAL,
    GAME_STALEMATE
} GameState;
typedef ... Board;
typedef struct {
    BitBoard from;
    BitBoard to;
    uint8_t promotion;
    bool capture;
    bool castle;
} Move;
Board *chess_get_board();
Move *chess_get_legal_moves(Board *board, int *len);
bool chess_is_white_turn(Board *board);
void chess_skip_turn(Board *board);
bool chess_in_check(Board *board);
bool chess_in_checkmate(Board *board);
bool chess_in_draw(Board *board);
bool chess_can_kingside_castle(Board *board, PlayerColor color);
bool chess_can_queenside_castle(Board *board, PlayerColor color);
GameState chess_is_game_ended(Board *board);
uint64_t chess_zobrist_key(Board *board);
void chess_make_move(Board *board, Move move);
void chess_undo_move(Board *board);
void chess_free_board(Board *board);
BitBoard chess_get_bitboard(Board *board, PlayerColor color, PieceType piece_type);
void chess_push(Move move);
void chess_done();
long chess_get_time_millis();
long chess_get_opponent_time_millis();
long chess_get_elapsed_time_millis();
""")

ffibuilder.set_source("_chessapi_cffi",
r"""
    #include <stdlib.h>
    #include "chessapi.h"
""",
sources=['chessapi.c', 'bitboard.c'])

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)