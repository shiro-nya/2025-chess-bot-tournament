#include "chessapi.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <semaphore.h>
#include <time.h>

#define CHESS_BOT_NAME "My Chess Bot"
#define BOT_AUTHOR_NAME "Author Name Here"

// ray direction constants (last 8 for knights)
#define DIR_N 0
#define DIR_NE 1
#define DIR_E 2
#define DIR_SE 3
#define DIR_S 4
#define DIR_SW 5
#define DIR_W 6
#define DIR_NW 7
#define DIR_NNW 8
#define DIR_NWW 9
#define DIR_SWW 10
#define DIR_SSW 11
#define DIR_NNE 12
#define DIR_NEE 13
#define DIR_SSE 14
#define DIR_SEE 15

typedef struct {
    pthread_t uci_thread;
    Board *shared_board;
    long wtime;
    long btime;
    clock_t turn_started_time;
    Move latest_move;
    pthread_mutex_t mutex;
    sem_t intermission_mutex;
} InternalAPI;

typedef uint64_t BitBoard;

struct Board {
    BitBoard bb_white_pawn;
    BitBoard bb_white_bishop;
    BitBoard bb_white_knight;
    BitBoard bb_white_rook;
    BitBoard bb_white_queen;
    BitBoard bb_white_king;
    BitBoard bb_black_pawn;
    BitBoard bb_black_bishop;
    BitBoard bb_black_knight;
    BitBoard bb_black_rook;
    BitBoard bb_black_queen;
    BitBoard bb_black_king;
    BitBoard *bb_white_moves;
    BitBoard *bb_black_moves;
    bool whiteToMove;
    Board *last_board;  // for move undo
    BitBoard en_passant_target;
    bool can_castle_bq;
    bool can_castle_bk;
    bool can_castle_wq;
    bool can_castle_wk;
    int halfmoves;
    int fullmoves;
    uint64_t hash;
};

InternalAPI *API = NULL;
unsigned long zobrist_keys[781];

uint64_t rand_long() {
    return ((uint64_t) rand()) ^ (((uint64_t) rand()) << 16) ^ (((uint64_t) rand()) << 32) ^ (((uint64_t) rand()) << 48);
}

int highest_bit(BitBoard v) {
    const unsigned long b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000, 0xFFFFFFFF00000000};
    const unsigned long S[] = {1, 2, 4, 8, 16, 32};
    int i;

    register unsigned long r = 0; // result of log2(v) will go here
    for (i = 5; i >= 0; i--) {
        if (v & b[i])
        {
            v >>= S[i];
            r |= S[i];
        } 
    }
    return (int)r;
}

// Returns true if the boards are equal.
bool board_equals(Board *board1, Board *board2) {
    return (board1->hash == board2->hash)
        && (board1->whiteToMove == board2->whiteToMove)
        && (board1->bb_white_pawn == board2->bb_white_pawn)
        && (board1->bb_black_pawn == board2->bb_black_pawn)
        && (board1->bb_white_queen == board2->bb_white_queen)
        && (board1->bb_black_queen == board2->bb_black_queen)
        && (board1->bb_white_knight == board2->bb_white_knight)
        && (board1->bb_black_knight == board2->bb_black_knight)
        && (board1->bb_white_bishop == board2->bb_white_bishop)
        && (board1->bb_black_bishop == board2->bb_black_bishop)
        && (board1->bb_white_rook == board2->bb_white_rook)
        && (board1->bb_black_rook == board2->bb_black_rook)
        && (board1->bb_white_king == board2->bb_white_king)
        && (board1->bb_black_king == board2->bb_black_king)
        && (board1->can_castle_bk == board2->can_castle_bk)
        && (board1->can_castle_bq == board2->can_castle_bq)
        && (board1->can_castle_wk == board2->can_castle_wk)
        && (board1->can_castle_wq == board2->can_castle_wq)
        && (board1->en_passant_target == board2->en_passant_target);
}

// creates a Move from a [movestr] in standard game notation and returns it
// if [board] is given, will augment move with flags; NULL is okay too
// TODO: augment so en passant is a capture too
Move load_move(char *movestr, Board *board) {
    Move m;
    m.from = 1ul << ((movestr[0] - 'a') + 8*(movestr[1] - '1'));
    m.to = 1ul << ((movestr[2] - 'a') + 8*(movestr[3] - '1'));
    switch(movestr[4]) {
        case 'p': m.promotion = PAWN; break;
        case 'b': m.promotion = BISHOP; break;
        case 'r': m.promotion = ROOK; break;
        case 'n': m.promotion = KNIGHT; break;
        case 'q': m.promotion = QUEEN; break;
        default: m.promotion = 0; break;
    }
    if (board == NULL) {
        m.capture = false;
        m.castle = false;
    } else {
        BitBoard all_pieces_white = board->bb_white_bishop | board->bb_white_king
            | board->bb_white_knight | board->bb_white_pawn | board->bb_white_queen
            | board->bb_white_rook;
        BitBoard all_pieces_black = board->bb_black_bishop | board->bb_black_king
            | board->bb_black_knight | board->bb_black_pawn | board->bb_black_queen
            | board->bb_black_rook;
        BitBoard all_pieces = all_pieces_black | all_pieces_white;
        m.capture = (m.from & all_pieces) > 0;
        m.castle = (m.from & (board->bb_black_king | board->bb_white_king)) > 0 &&
            ((bb_slide_e(bb_slide_e(m.from)) | bb_slide_w(bb_slide_w(bb_slide_w(m.from)))) & m.to) > 0;
    }
    return m;
}

// formats [move] in standard game notation, storing result in [buffer]
// [buffer] should be at least 7 bytes
void dump_move(char *buffer, Move move) {
    memset(buffer, '\0', 7);
    int sq_from = highest_bit(move.from);
    int sq_to = highest_bit(move.to);
    buffer[0] = 'a' + sq_from % 8;
    buffer[1] = '1' + sq_from / 8;
    buffer[2] = 'a' + sq_to % 8;
    buffer[3] = '1' + sq_to / 8;
    if (move.promotion != 0) {
        switch(move.promotion) {
            case ROOK: buffer[4] = 'r'; break;
            case BISHOP: buffer[4] = 'b'; break;
            case KNIGHT: buffer[4] = 'n'; break;
            case QUEEN: buffer[4] = 'q'; break;
            default: buffer[4] = '\0'; break;
        }
        buffer[5] = '\0';
    } else {
        buffer[4] = '\0';
    }
}

// Safely free a board from memory.
void free_board(Board *board) {
    if (board->bb_white_moves != NULL) {
        free(board->bb_white_moves);
    }
    if (board->bb_black_moves != NULL) {
        free(board->bb_black_moves);
    }
    if (board->last_board != NULL) {
        free_board(board->last_board);
    }
    free(board);
}

void set_board_from(Board *dest, Board *src) {
    dest->bb_black_bishop = src->bb_black_bishop;
    dest->bb_black_rook = src->bb_black_rook;
    dest->bb_black_queen = src->bb_black_queen;
    dest->bb_black_king = src->bb_black_king;
    dest->bb_black_knight = src->bb_black_knight;
    dest->bb_black_pawn = src->bb_black_pawn;
    dest->bb_white_bishop = src->bb_white_bishop;
    dest->bb_white_rook = src->bb_white_rook;
    dest->bb_white_queen = src->bb_white_queen;
    dest->bb_white_king = src->bb_white_king;
    dest->bb_white_knight = src->bb_white_knight;
    dest->bb_white_pawn = src->bb_white_pawn;
}

// Set the Zobrist hash for [board] from its current position
void calc_zobrist(Board *board) {
    uint64_t hash = 0;
    BitBoard board0 = board->bb_black_pawn;
    BitBoard board1 = board->bb_black_rook;
    BitBoard board2 = board->bb_black_bishop;
    BitBoard board3 = board->bb_black_queen;
    BitBoard board4 = board->bb_black_king;
    BitBoard board5 = board->bb_black_knight;
    BitBoard board6 = board->bb_white_pawn;
    BitBoard board7 = board->bb_white_rook;
    BitBoard board8 = board->bb_white_bishop;
    BitBoard board9 = board->bb_white_queen;
    BitBoard board10 = board->bb_white_king;
    BitBoard board11 = board->bb_white_knight;
    for (int i = 0; i < 64; i++) {
        hash ^= (board0 & 1) * zobrist_keys[0*64 + i];
        hash ^= (board1 & 1) * zobrist_keys[1*64 + i];
        hash ^= (board2 & 1) * zobrist_keys[2*64 + i];
        hash ^= (board3 & 1) * zobrist_keys[3*64 + i];
        hash ^= (board4 & 1) * zobrist_keys[4*64 + i];
        hash ^= (board5 & 1) * zobrist_keys[5*64 + i];
        hash ^= (board6 & 1) * zobrist_keys[6*64 + i];
        hash ^= (board7 & 1) * zobrist_keys[7*64 + i];
        hash ^= (board8 & 1) * zobrist_keys[8*64 + i];
        hash ^= (board9 & 1) * zobrist_keys[9*64 + i];
        hash ^= (board10 & 1) * zobrist_keys[10*64 + i];
        hash ^= (board11 & 1) * zobrist_keys[11*64 + i];
        board0 >>= 1;
        board1 >>= 1;
        board2 >>= 1;
        board3 >>= 1;
        board4 >>= 1;
        board5 >>= 1;
        board6 >>= 1;
        board7 >>= 1;
        board8 >>= 1;
        board9 >>= 1;
        board10 >>= 1;
        board11 >>= 1;
    }
    if (board->can_castle_bk) hash ^= zobrist_keys[768];
    if (board->can_castle_bq) hash ^= zobrist_keys[769];
    if (board->can_castle_wk) hash ^= zobrist_keys[770];
    if (board->can_castle_wq) hash ^= zobrist_keys[771];
    if (board->whiteToMove) hash ^= zobrist_keys[772];
    if (board->en_passant_target != 0) hash ^= zobrist_keys[773 + (highest_bit(board->en_passant_target) % 8)];
    board->hash = hash;
}

// Clears all piece bitboards for the [board], and clears the pseudo-legal move caches.
void clear_board(Board *board) {
    board->bb_black_bishop = 0;
    board->bb_black_king = 0;
    board->bb_black_queen = 0;
    board->bb_black_rook = 0;
    board->bb_black_pawn = 0;
    board->bb_black_knight = 0;
    if (board->bb_black_moves != NULL) {
        free(board->bb_black_moves);
        board->bb_black_moves = 0;
    }
    board->bb_white_bishop = 0;
    board->bb_white_king = 0;
    board->bb_white_queen = 0;
    board->bb_white_rook = 0;
    board->bb_white_pawn = 0;
    board->bb_white_knight = 0;
    if (board->bb_white_moves != NULL) {
        free(board->bb_white_moves);
        board->bb_white_moves = 0;
    }
    calc_zobrist(board);
}

void set_board_from_fen(Board *board, char *fen) {
    // if no fen given, use starting pos
    char *use_fen = (fen != NULL) ? fen : "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    clear_board(board);
    board->can_castle_bk = false;
    board->can_castle_bq = false;
    board->can_castle_wk = false;
    board->can_castle_wq = false;
    BitBoard place_piece = ((BitBoard)1) << 56;
    while (*use_fen != ' ') {
        switch (*use_fen) {
            case 'r': board->bb_black_rook |= place_piece; break;
            case 'n': board->bb_black_knight |= place_piece; break;
            case 'b': board->bb_black_bishop |= place_piece; break;
            case 'q': board->bb_black_queen |= place_piece; break;
            case 'k': board->bb_black_king |= place_piece; break;
            case 'p': board->bb_black_pawn |= place_piece; break;
            case 'R': board->bb_white_rook |= place_piece; break;
            case 'N': board->bb_white_knight |= place_piece; break;
            case 'B': board->bb_white_bishop |= place_piece; break;
            case 'Q': board->bb_white_queen |= place_piece; break;
            case 'K': board->bb_white_king |= place_piece; break;
            case 'P': board->bb_white_pawn |= place_piece; break;
            case '/': break;
            default:
                char spaces = *use_fen - '0';
                place_piece <<= (spaces - 1);
                break;
        }
        if (*use_fen == '/') {
            use_fen++;
            place_piece >>= 7;
            place_piece = bb_slide_s(place_piece);
            continue;
        }
        use_fen++;
        if (highest_bit(place_piece) % 8 < 7) {
            place_piece = bb_slide_e(place_piece);
        }
    }
    use_fen++;
    board->whiteToMove = (*use_fen == 'w');
    use_fen += 2;
    while (*use_fen != ' ') {
        switch (*use_fen) {
            case 'K': board->can_castle_wk = true; break;
            case 'Q': board->can_castle_wq = true; break;
            case 'k': board->can_castle_bk = true; break;
            case 'q': board->can_castle_bq = true; break;
        }
        use_fen++;
    }
    use_fen++;
    BitBoard ep_square = 1;
    while (*use_fen != ' ') {
        if (*use_fen == '-') {
            ep_square = 0;
            break;
        } else if (*use_fen >= 'a' && *use_fen <= 'h') {
            ep_square <<= (8 * (*use_fen - 'a'));
        } else if (*use_fen >= '1' && *use_fen <= '7') {
            ep_square <<= (*use_fen - '1');
        }
        use_fen++;
    }
    use_fen++;
    board->en_passant_target = ep_square;
    int halfmoves = 0;
    while (*use_fen != ' ') {
        halfmoves *= 10;
        halfmoves += *use_fen - '0';
        use_fen++;
    }
    use_fen++;
    board->halfmoves = halfmoves;
    int fullmoves = 0;
    while (*use_fen != ' ') {
        fullmoves *= 10;
        fullmoves += *use_fen - '0';
        use_fen++;
    }
    board->fullmoves = fullmoves;
    calc_zobrist(board);
}

// Makes a new, blank board. Caller responsible for freeing.
Board *create_board() {
    Board *board = (Board *)malloc(sizeof(Board));
    memset(board, 0, sizeof(Board));
    clear_board(board);
    board->can_castle_bk = false;
    board->can_castle_bq = false;
    board->can_castle_wk = false;
    board->can_castle_wq = false;
    board->fullmoves = 1;
    board->halfmoves = 0;
    board->last_board = NULL;
    board->en_passant_target = 0;
    board->whiteToMove = true;
    return board;
}

// Creates a shallow copy of the given board
Board *clone_board(Board * board) {
    Board *new_board = (Board *)malloc(sizeof(Board));
    memcpy(new_board, board, sizeof(Board));
    new_board->bb_black_moves = NULL;
    new_board->bb_white_moves = NULL;
    new_board->last_board = NULL;
    return new_board;
}

// Updates the [board] with the result of the given [move].
// The previous board can be restored with undo_move().
// Moves are presumed legal.
void make_move(Board *board, Move move) {
    Board *saved_board = clone_board(board);
    saved_board->last_board = board->last_board;
    board->last_board = saved_board;
    int from = highest_bit(move.from);
    int to = highest_bit(move.to);
    uint8_t hash = board->hash;
    board->halfmoves++;
    BitBoard flip_pieces = move.to | move.from;
    hash ^= highest_bit(board->en_passant_target) % 8; // xor out the old en passant hash
    board->en_passant_target = 0;
    bool do_promotion = false;
    if (move.from & (board->bb_black_pawn | board->bb_white_pawn)) {
        board->halfmoves = 0;
        // set en passant target if double pawn move
        if ((move.to & bb_slide_s(bb_slide_s(move.from))) > 0) {
            board->en_passant_target = bb_slide_s(move.from);
            hash ^= highest_bit(board->en_passant_target) % 8;  // xor in new en passant hash
        } else if ((move.to & bb_slide_n(bb_slide_n(move.from))) > 0) {
            board->en_passant_target = bb_slide_n(move.from);
            hash ^= highest_bit(board->en_passant_target) % 8;  // xor in new en passant hash
        } else if (move.to & 0xff000000000000fful) {
            do_promotion = true;
        }
    }
    if (move.from & board->bb_white_king) {
        if (board->can_castle_wk) hash ^= zobrist_keys[770];
        if (board->can_castle_wq) hash ^= zobrist_keys[771];
        board->can_castle_wk = false;
        board->can_castle_wq = false;
    } else if (move.from & board->bb_black_king) {
        if (board->can_castle_bk) hash ^= zobrist_keys[768];
        if (board->can_castle_bq) hash ^= zobrist_keys[769];
        board->can_castle_bk = false;
        board->can_castle_bq = false;
    } else if (move.from & 0x0000000000000001ul) {
        if (board->can_castle_wq) hash ^= zobrist_keys[771];
        board->can_castle_wq = false;
    } else if (move.from & 0x0000000000000080ul) {
        if (board->can_castle_wk) hash ^= zobrist_keys[770];
        board->can_castle_wk = false;
    } else if (move.from & 0x0100000000000000ul) {
        if (board->can_castle_bq) hash ^= zobrist_keys[769];
        board->can_castle_bq = false;
    } else if (move.from & 0x8000000000000000ul) {
        if (board->can_castle_bk) hash ^= zobrist_keys[768];
        board->can_castle_bk = false;
    }
    if (move.castle) {
        if ((move.from & board->bb_white_king) > 0 && (move.to > move.from)) {
            // white castle kingside
            hash ^= zobrist_keys[64*10+4]^zobrist_keys[64*10+6]^zobrist_keys[64*7+7]^zobrist_keys[64*7+5];
            if (board->can_castle_wk) hash ^= zobrist_keys[770];
            if (board->can_castle_wq) hash ^= zobrist_keys[771];
            board->bb_white_king ^= 80ul;
            board->bb_white_rook ^= 160ul;
            board->can_castle_wk = false;
            board->can_castle_wq = false;
        } else if ((move.from & board->bb_white_king) > 0 && (move.to < move.from)) {
            // white castle queenside
            hash ^= zobrist_keys[64*10+4]^zobrist_keys[64*10+2]^zobrist_keys[64*7+0]^zobrist_keys[64*7+3];
            if (board->can_castle_wk) hash ^= zobrist_keys[770];
            if (board->can_castle_wq) hash ^= zobrist_keys[771];
            board->bb_white_king ^= 20ul;
            board->bb_white_rook ^= 9ul;
            board->can_castle_wk = false;
            board->can_castle_wq = false;
        } else if ((move.from & board->bb_black_king) > 0 && (move.to > move.from)) {
            // black castle kingside
            hash ^= zobrist_keys[64*10+56+4]^zobrist_keys[64*10+56+6]^zobrist_keys[64*7+56+7]^zobrist_keys[64*7+56+5];
            if (board->can_castle_bk) hash ^= zobrist_keys[768];
            if (board->can_castle_bq) hash ^= zobrist_keys[769];
            board->bb_black_king ^= 5764607523034234880ul;
            board->bb_black_rook ^= 11529215046068469760ul;
            board->can_castle_bk = false;
            board->can_castle_bq = false;
        } else if ((move.from & board->bb_black_king) > 0 && (move.to < move.from)) {
            // black castle queenside
            hash ^= zobrist_keys[64*10+56+4]^zobrist_keys[64*10+56+2]^zobrist_keys[64*7+56+0]^zobrist_keys[64*7+56+3];
            if (board->can_castle_bk) hash ^= zobrist_keys[768];
            if (board->can_castle_bq) hash ^= zobrist_keys[769];
            board->bb_black_king ^= 1441151880758558720ul;
            board->bb_black_rook ^= 648518346341351424ul;
            board->can_castle_bk = false;
            board->can_castle_bq = false;
        }
        if (!board->whiteToMove) {
            board->fullmoves++;
        }
        board->whiteToMove = !board->whiteToMove;
        return;
    }
    if (move.capture) {
        board->halfmoves = 0;
        BitBoard cap_mask;
        // note: since en passant must be performed the turn after the double pawn move,
        // there is never a case where an en passant move could capture two pieces
        if ((move.from & board->bb_white_pawn) > 0 && (move.to & board->en_passant_target) > 0) {
            // en passant is the worst chess feature
            cap_mask = ~bb_slide_s(board->en_passant_target);
        } else if ((move.from & board->bb_black_pawn) > 0 && (move.to & board->en_passant_target) > 0) {
            // en passant is the worst chess feature
            cap_mask = ~bb_slide_n(board->en_passant_target);
        } else {
            cap_mask = ~move.to;
        }
        // remove captured piece
        board->bb_black_bishop &= cap_mask;
        board->bb_black_rook &= cap_mask;
        board->bb_black_queen &= cap_mask;
        board->bb_black_king &= cap_mask;
        board->bb_black_knight &= cap_mask;
        board->bb_black_pawn &= cap_mask;
        board->bb_white_bishop &= cap_mask;
        board->bb_white_rook &= cap_mask;
        board->bb_white_queen &= cap_mask;
        board->bb_white_king &= cap_mask;
        board->bb_white_knight &= cap_mask;
        board->bb_white_pawn &= cap_mask;
        // update hash for captured piece
        BitBoard inv_cap_mask = ~cap_mask;
        int cap_at = highest_bit(inv_cap_mask);
        hash ^= ((board->bb_black_pawn & inv_cap_mask) > 0) * (zobrist_keys[64*0 + cap_at]);
        hash ^= ((board->bb_black_rook & inv_cap_mask) > 0) * (zobrist_keys[64*1 + cap_at]);
        hash ^= ((board->bb_black_bishop & inv_cap_mask) > 0) * (zobrist_keys[64*2 + cap_at]);
        hash ^= ((board->bb_black_queen & inv_cap_mask) > 0) * (zobrist_keys[64*3 + cap_at]);
        hash ^= ((board->bb_black_king & inv_cap_mask) > 0) * (zobrist_keys[64*4 + cap_at]);
        hash ^= ((board->bb_black_knight & inv_cap_mask) > 0) * (zobrist_keys[64*5 + cap_at]);
        hash ^= ((board->bb_white_pawn & inv_cap_mask) > 0) * (zobrist_keys[64*6 + cap_at]);
        hash ^= ((board->bb_white_rook & inv_cap_mask) > 0) * (zobrist_keys[64*7 + cap_at]);
        hash ^= ((board->bb_white_bishop & inv_cap_mask) > 0) * (zobrist_keys[64*8 + cap_at]);
        hash ^= ((board->bb_white_queen & inv_cap_mask) > 0) * (zobrist_keys[64*9 + cap_at]);
        hash ^= ((board->bb_white_king & inv_cap_mask) > 0) * (zobrist_keys[64*10 + cap_at]);
        hash ^= ((board->bb_white_knight & inv_cap_mask) > 0) * (zobrist_keys[64*11 + cap_at]);
    }
    // remove old moved piece
    board->bb_black_bishop ^= ((board->bb_black_bishop & move.from) > 0) * flip_pieces;
    board->bb_black_rook ^= ((board->bb_black_rook & move.from) > 0) * flip_pieces;
    board->bb_black_queen ^= ((board->bb_black_queen & move.from) > 0) * flip_pieces;
    board->bb_black_king ^= ((board->bb_black_king & move.from) > 0) * flip_pieces;
    board->bb_black_knight ^= ((board->bb_black_knight & move.from) > 0) * flip_pieces;
    board->bb_black_pawn ^= ((board->bb_black_pawn & move.from) > 0) * flip_pieces;
    board->bb_white_bishop ^= ((board->bb_white_bishop & move.from) > 0) * flip_pieces;
    board->bb_white_rook ^= ((board->bb_white_rook & move.from) > 0) * flip_pieces;
    board->bb_white_queen ^= ((board->bb_white_queen & move.from) > 0) * flip_pieces;
    board->bb_white_king ^= ((board->bb_white_king & move.from) > 0) * flip_pieces;
    board->bb_white_knight ^= ((board->bb_white_knight & move.from) > 0) * flip_pieces;
    board->bb_white_pawn ^= ((board->bb_white_pawn & move.from) > 0) * flip_pieces;
    // update hash for moved piece
    hash ^= ((board->bb_black_pawn & move.from) > 0) * (zobrist_keys[64*0 + from] ^ zobrist_keys[64*0 + to]);
    hash ^= ((board->bb_black_rook & move.from) > 0) * (zobrist_keys[64*1 + from] ^ zobrist_keys[64*1 + to]);
    hash ^= ((board->bb_black_bishop & move.from) > 0) * (zobrist_keys[64*2 + from] ^ zobrist_keys[64*2 + to]);
    hash ^= ((board->bb_black_queen & move.from) > 0) * (zobrist_keys[64*3 + from] ^ zobrist_keys[64*3 + to]);
    hash ^= ((board->bb_black_king & move.from) > 0) * (zobrist_keys[64*4 + from] ^ zobrist_keys[64*4 + to]);
    hash ^= ((board->bb_black_knight & move.from) > 0) * (zobrist_keys[64*5 + from] ^ zobrist_keys[64*5 + to]);
    hash ^= ((board->bb_white_pawn & move.from) > 0) * (zobrist_keys[64*6 + from] ^ zobrist_keys[64*6 + to]);
    hash ^= ((board->bb_white_rook & move.from) > 0) * (zobrist_keys[64*7 + from] ^ zobrist_keys[64*7 + to]);
    hash ^= ((board->bb_white_bishop & move.from) > 0) * (zobrist_keys[64*8 + from] ^ zobrist_keys[64*8 + to]);
    hash ^= ((board->bb_white_queen & move.from) > 0) * (zobrist_keys[64*9 + from] ^ zobrist_keys[64*9 + to]);
    hash ^= ((board->bb_white_king & move.from) > 0) * (zobrist_keys[64*10 + from] ^ zobrist_keys[64*10 + to]);
    hash ^= ((board->bb_white_knight & move.from) > 0) * (zobrist_keys[64*11 + from] ^ zobrist_keys[64*11 + to]);
    if (do_promotion) {
        switch (move.promotion) {
            case BISHOP:
                board->bb_white_bishop |= (move.to & board->bb_white_pawn);
                board->bb_black_bishop |= (move.to & board->bb_black_pawn);
                hash ^= (move.to & board->bb_white_pawn)*(zobrist_keys[64*8+to]+zobrist_keys[64*6+to]);
                hash ^= (move.to & board->bb_black_pawn)*(zobrist_keys[64*2+to]+zobrist_keys[64*0+to]);
                break;
            case ROOK:
                board->bb_white_rook |= (move.to & board->bb_white_pawn);
                board->bb_black_rook |= (move.to & board->bb_black_pawn);
                hash ^= (move.to & board->bb_white_pawn)*(zobrist_keys[64*7+to]+zobrist_keys[64*6+to]);
                hash ^= (move.to & board->bb_black_pawn)*(zobrist_keys[64*1+to]+zobrist_keys[64*0+to]);
                break;
            case KNIGHT:
                board->bb_white_knight |= (move.to & board->bb_white_pawn);
                board->bb_black_knight |= (move.to & board->bb_black_pawn);
                hash ^= (move.to & board->bb_white_pawn)*(zobrist_keys[64*11+to]+zobrist_keys[64*6+to]);
                hash ^= (move.to & board->bb_black_pawn)*(zobrist_keys[64*5+to]+zobrist_keys[64*0+to]);
                break;
            case QUEEN:
                board->bb_white_queen |= (move.to & board->bb_white_pawn);
                board->bb_black_queen |= (move.to & board->bb_black_pawn);
                hash ^= (move.to & board->bb_white_pawn)*(zobrist_keys[64*9+to]+zobrist_keys[64*6+to]);
                hash ^= (move.to & board->bb_black_pawn)*(zobrist_keys[64*3+to]+zobrist_keys[64*0+to]);
                break;
        }
        board->bb_white_pawn &= ~move.to;
        board->bb_black_pawn &= ~move.to;
    }
    if (!board->whiteToMove) {
        board->fullmoves++;
    }
    board->whiteToMove = !board->whiteToMove;
    hash ^= zobrist_keys[772];  // update color-to-play hash
    board->hash = hash;  // commit hash
}

// Restores the previous board state for [board] if it exists.
void undo_move(Board *board) {
    if (board->last_board == NULL) return;  // no moves to undo
    // get the previous board
    Board *restore = board->last_board;
    set_board_from(board, restore);
    // set the new "previous board" to the previous "previous board"
    board->last_board = restore->last_board;
    // copy other data not copied by set_board_from
    board->can_castle_bk = restore->can_castle_bk;
    board->can_castle_bq = restore->can_castle_bq;
    board->can_castle_wk = restore->can_castle_wk;
    board->can_castle_wq = restore->can_castle_wq;
    board->halfmoves = restore->halfmoves;
    board->fullmoves = restore->fullmoves;
    board->whiteToMove = restore->whiteToMove;
    board->en_passant_target = restore->en_passant_target;
    // free old move caches before overwriting
    if (board->bb_white_moves != NULL) {
        free(board->bb_white_moves);
    }
    if (board->bb_black_moves != NULL) {
        free(board->bb_black_moves);
    }
    board->bb_white_moves = restore->bb_white_moves;
    board->bb_black_moves = restore->bb_black_moves;
    // ensure these are NULL before freeing the board to avoid the reused arrays being freed
    restore->bb_white_moves = NULL;
    restore->bb_black_moves = NULL;
    restore->last_board = NULL;
    // free the restored board
    free_board(restore);
}

// Listens for and responds to UCI messages from the GUI. Updates API state as needed.
void *uci_process(void *arg) {
    char line[1024];
    bool running = true;
    while (running) {
        scanf("%[\r\n]", line);
        memset(&line, 0, 1024);
        scanf("%[^\r\n]", line);
        line[1023] = 0;
        char *token = strtok(line, " ");
        while (running && (token != NULL)) {
            if (!strcmp(token, "uci")) {
                printf("id name %s\n", CHESS_BOT_NAME);
                printf("id author %s\n", BOT_AUTHOR_NAME);
                printf("uciok\n");
                fflush(stdout);
            } else if (!strcmp(token, "isready")) {
                printf("readyok\n");
                fflush(stdout);
            } else if (!strcmp(token, "position")) {
                pthread_mutex_lock(&API->mutex);
                token = strtok(NULL, " ");
                if (!strcmp(token, "fen")) {
                    char fenstring[256];
                    char *next_token = fenstring;
                    token = strtok(NULL, " ");
                    while (token && strcmp(token, "moves")) {
                        if (next_token > fenstring + 256) {
                            pthread_exit(NULL);
                        }
                        strcpy(next_token, token);
                        next_token += strlen(token) + 1;
                        token = strtok(NULL, " ");
                        if (token != NULL) *(next_token - 1) = ' ';
                    }
                    if (API->shared_board != NULL) free_board(API->shared_board);
                    API->shared_board = create_board();
                    set_board_from_fen(API->shared_board, fenstring);
                } else if (!strcmp(token, "startpos")) {
                    if (API->shared_board != NULL) free_board(API->shared_board);
                    API->shared_board = create_board();
                    set_board_from_fen(API->shared_board, NULL);
                    token = strtok(NULL, " ");
                }
                if (token != NULL && !strcmp(token, "moves")) {
                    char *move = strtok(NULL, " ");
                    while (move != NULL) {
                        Move m = load_move(move, API->shared_board);
                        make_move(API->shared_board, m);
                        move = strtok(NULL, " ");
                    }
                }
                pthread_mutex_unlock(&API->mutex);
            } else if (!strcmp(token, "go")) {
                pthread_mutex_lock(&API->mutex);
                token = strtok(NULL, " ");
                while (token != NULL) {
                    if (!strcmp(token, "wtime")) {
                        char *rawtime = strtok(NULL, " ");
                        API->wtime = strtol(rawtime, NULL, 10);
                    } else if (!strcmp(token, "btime")) {
                        char *rawtime = strtok(NULL, " ");
                        API->btime = strtol(rawtime, NULL, 10);
                    } else if (!strcmp(token, "infinite")) {
                        API->btime = (long)1<<31;
                        API->wtime = (long)1<<31;
                    }
                    token = strtok(NULL, " ");
                }
                sem_post(&API->intermission_mutex);
                API->turn_started_time = clock();
                pthread_mutex_unlock(&API->mutex);
            } else if (!strcmp(token, "stop")) {
                // does nothing for now
            } else if (!strcmp(token, "quit")) {
                pthread_cancel(API->uci_thread);
                running = false;
            }
            token = strtok(NULL, " ");
        }
    }
    pthread_exit(NULL);
}

// Start the UCI listener.
void uci_start(pthread_t *thread_id) {
    pthread_create(thread_id, NULL, &uci_process, NULL);
}

// gets API->latest_move and formats in standard game notation, storing result in buffer
// buffer should be at least 7 bytes
void dump_api_move(char *buffer) {
    dump_move(buffer, API->latest_move);
}

void uci_info() {
    char move[8];
    dump_api_move(move);
    printf("info currmove %s\n", move);
    fflush(stdout);
}

void uci_finished_searching() {
    char move[8];
    dump_api_move(move);
    printf("bestmove %s\n", move);
    fflush(stdout);
}

// any API methods that require thread safing are placed here

void interface_push(Move move) {
    pthread_mutex_lock(&API->mutex);
    API->latest_move = move;
    uci_info();
    pthread_mutex_unlock(&API->mutex);
}

void interface_done() {
    pthread_mutex_lock(&API->mutex);
    uci_finished_searching();
    pthread_mutex_unlock(&API->mutex);
    sem_wait(&API->intermission_mutex);
}

Board *interface_get_board() {
    pthread_mutex_lock(&API->mutex);
    Board *board = clone_board(API->shared_board);
    pthread_mutex_unlock(&API->mutex);
    return board;
}

long interface_get_time_millis() {
    pthread_mutex_lock(&API->mutex);
    long millis = API->shared_board->whiteToMove ? API->wtime : API->btime;
    pthread_mutex_unlock(&API->mutex);
    return millis;
}

long interface_get_opponent_time_millis() {
    pthread_mutex_lock(&API->mutex);
    long millis = API->shared_board->whiteToMove ? API->btime : API->wtime;
    pthread_mutex_unlock(&API->mutex);
    return millis;
}

long interface_get_elapsed_time_millis() {
    pthread_mutex_lock(&API->mutex);
    long millis = (clock() - API->turn_started_time) / (CLOCKS_PER_SEC / 1000);
    pthread_mutex_unlock(&API->mutex);
    return millis;
}

bool is_white_turn(Board *board) {
    return board->whiteToMove;
}

// Returns all pinned pieces for white if [white], otherwise for black.
BitBoard get_pins(Board *board, bool white) {
    BitBoard pins = 0;
    BitBoard white_diag_pieces = board->bb_white_bishop | board->bb_white_queen;
    BitBoard white_level_pieces = board->bb_white_rook | board->bb_white_queen;
    BitBoard black_diag_pieces = board->bb_black_bishop | board->bb_black_queen;
    BitBoard black_level_pieces = board->bb_black_rook | board->bb_black_queen;
    BitBoard all_pieces_white = board->bb_white_bishop | board->bb_white_king
        | board->bb_white_knight | board->bb_white_pawn | board->bb_white_queen
        | board->bb_white_rook;
    BitBoard all_pieces_black = board->bb_black_bishop | board->bb_black_king
        | board->bb_black_knight | board->bb_black_pawn | board->bb_black_queen
        | board->bb_black_rook;
    BitBoard my_pieces = white ? all_pieces_white : all_pieces_black;
    BitBoard opp_diag_pieces = white ? black_diag_pieces : white_diag_pieces;
    BitBoard opp_level_pieces = white ? black_level_pieces : white_level_pieces;
    BitBoard my_king = white ? board->bb_white_king : board->bb_black_king;
    // check for horz/vert pins
    BitBoard gen = my_king;
    BitBoard last_my = 0;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_n(gen);
        if ((gen & my_pieces) > 0 && last_my == 0) {
            last_my = gen;
        } else if ((gen & opp_level_pieces) > 0) {
            pins |= last_my;
            break;
        } else if ((gen & my_pieces) > 0) {
            break;
        }
    }
    gen = my_king;
    last_my = 0;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_e(gen);
        if ((gen & my_pieces) > 0 && last_my == 0) {
            last_my = gen;
        } else if ((gen & opp_level_pieces) > 0) {
            pins |= last_my;
            break;
        } else if ((gen & my_pieces) > 0) {
            break;
        }
    }
    gen = my_king;
    last_my = 0;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_s(gen);
        if ((gen & my_pieces) > 0 && last_my == 0) {
            last_my = gen;
        } else if ((gen & opp_level_pieces) > 0) {
            pins |= last_my;
            break;
        } else if ((gen & my_pieces) > 0) {
            break;
        }
    }
    gen = my_king;
    last_my = 0;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_w(gen);
        if ((gen & my_pieces) > 0 && last_my == 0) {
            last_my = gen;
        } else if ((gen & opp_level_pieces) > 0) {
            pins |= last_my;
            break;
        } else if ((gen & my_pieces) > 0) {
            break;
        }
    }
    // check for diagonal pins
    gen = my_king;
    last_my = 0;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_ne(gen);
        if ((gen & my_pieces) > 0 && last_my == 0) {
            last_my = gen;
        } else if ((gen & opp_diag_pieces) > 0) {
            pins |= last_my;
            break;
        } else if ((gen & my_pieces) > 0) {
            break;
        }
    }
    gen = my_king;
    last_my = 0;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_nw(gen);
        if ((gen & my_pieces) > 0 && last_my == 0) {
            last_my = gen;
        } else if ((gen & opp_diag_pieces) > 0) {
            pins |= last_my;
            break;
        } else if ((gen & my_pieces) > 0) {
            break;
        }
    }
    gen = my_king;
    last_my = 0;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_se(gen);
        if ((gen & my_pieces) > 0 && last_my == 0) {
            last_my = gen;
        } else if ((gen & opp_diag_pieces) > 0) {
            pins |= last_my;
            break;
        } else if ((gen & my_pieces) > 0) {
            break;
        }
    }
    gen = my_king;
    last_my = 0;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_sw(gen);
        if ((gen & my_pieces) > 0 && last_my == 0) {
            last_my = gen;
        } else if ((gen & opp_diag_pieces) > 0) {
            pins |= last_my;
            break;
        } else if ((gen & my_pieces) > 0) {
            break;
        }
    }
    return pins;
}

// Returns the pseudo-legal moves on [board] for white if [white], otherwise for black.
// Pseudo-legal moves are valid moves prior to evaluating for checks.
// If [all_attacked], will include all squares attacked by at least one piece, instead of filtering for legal captures.
// Squares in [exclude] are overridden and considered empty.
// Caller must free move array.
BitBoard *get_pseudo_legal_moves(Board *board, bool white, bool all_attacked, BitBoard exclude) {
    BitBoard *dirmoves = malloc(16*sizeof(BitBoard));
    if ((!all_attacked) && (exclude == 0)) {
        if (white && board->bb_white_moves) {
            memcpy(dirmoves, board->bb_white_moves, 16*sizeof(BitBoard));
            return dirmoves;
        } else if ((!white) && board->bb_black_moves) {
            memcpy(dirmoves, board->bb_black_moves, 16*sizeof(BitBoard));
            return dirmoves;
        }
    }
    memset(dirmoves, 0, 16);
    BitBoard all_pieces_white = board->bb_white_bishop | board->bb_white_king
        | board->bb_white_knight | board->bb_white_pawn | board->bb_white_queen
        | board->bb_white_rook;
    BitBoard all_pieces_black = board->bb_black_bishop | board->bb_black_king
        | board->bb_black_knight | board->bb_black_pawn | board->bb_black_queen
        | board->bb_black_rook;
    BitBoard all_pieces = all_pieces_black | all_pieces_white;
    BitBoard empty = exclude | ~all_pieces;
    BitBoard all_attacked_mask = all_attacked ? ~0ul : 0ul;
    if (white) {
        BitBoard pawn_moves = bb_slide_n(board->bb_white_pawn) & empty;
        BitBoard pawn_big_moves = bb_slide_n(pawn_moves & 0x0000000000ff0000ul) & empty;
        BitBoard pawn_attacks_ne = bb_slide_ne(board->bb_white_pawn) & (all_pieces_black | board->en_passant_target | all_attacked_mask);
        BitBoard pawn_attacks_nw = bb_slide_nw(board->bb_white_pawn) & (all_pieces_black | board->en_passant_target | all_attacked_mask);
        BitBoard ray_moves_n = bb_flood_n(board->bb_white_queen | board->bb_white_rook, empty, true);
        BitBoard ray_moves_e = bb_flood_e(board->bb_white_queen | board->bb_white_rook, empty, true);
        BitBoard ray_moves_s = bb_flood_s(board->bb_white_queen | board->bb_white_rook, empty, true);
        BitBoard ray_moves_w = bb_flood_w(board->bb_white_queen | board->bb_white_rook, empty, true);
        BitBoard ray_moves_ne = bb_flood_ne(board->bb_white_queen | board->bb_white_bishop, empty, true);
        BitBoard ray_moves_nw = bb_flood_nw(board->bb_white_queen | board->bb_white_bishop, empty, true);
        BitBoard ray_moves_se = bb_flood_se(board->bb_white_queen | board->bb_white_bishop, empty, true);
        BitBoard ray_moves_sw = bb_flood_sw(board->bb_white_queen | board->bb_white_bishop, empty, true);
        BitBoard king_moves_n = bb_slide_n(board->bb_white_king);
        BitBoard king_moves_ne = bb_slide_ne(board->bb_white_king);
        BitBoard king_moves_e = bb_slide_e(board->bb_white_king);
        BitBoard king_moves_se = bb_slide_se(board->bb_white_king);
        BitBoard king_moves_s = bb_slide_s(board->bb_white_king);
        BitBoard king_moves_sw = bb_slide_sw(board->bb_white_king);
        BitBoard king_moves_w = bb_slide_w(board->bb_white_king);
        BitBoard king_moves_nw = bb_slide_nw(board->bb_white_king);
        dirmoves[DIR_NNE] = bb_slide_n(bb_slide_n(bb_slide_e(board->bb_white_knight))) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_NEE] = bb_slide_n(bb_slide_e(bb_slide_e(board->bb_white_knight))) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_NNW] = bb_slide_n(bb_slide_n(bb_slide_w(board->bb_white_knight))) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_NWW] = bb_slide_n(bb_slide_w(bb_slide_w(board->bb_white_knight))) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_SSE] = bb_slide_s(bb_slide_s(bb_slide_e(board->bb_white_knight))) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_SEE] = bb_slide_s(bb_slide_e(bb_slide_e(board->bb_white_knight))) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_SSW] = bb_slide_s(bb_slide_s(bb_slide_w(board->bb_white_knight))) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_SWW] = bb_slide_s(bb_slide_w(bb_slide_w(board->bb_white_knight))) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_N] = (pawn_moves | pawn_big_moves | ray_moves_n | king_moves_n) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_NE] = (pawn_attacks_ne | ray_moves_ne | king_moves_ne) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_E] = (ray_moves_e | king_moves_e) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_SE] = (ray_moves_se | king_moves_se) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_S] = (ray_moves_s | king_moves_s) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_SW] = (ray_moves_sw | king_moves_sw) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_W] = (ray_moves_w | king_moves_w) & (all_attacked_mask | ~all_pieces_white);
        dirmoves[DIR_NW] = (pawn_attacks_nw | ray_moves_nw | king_moves_nw) & (all_attacked_mask | ~all_pieces_white);
    } else {
        BitBoard pawn_moves = bb_slide_s(board->bb_black_pawn) & empty;
        BitBoard pawn_big_moves = bb_slide_s(pawn_moves & 0x0000ff0000000000ul) & empty;
        BitBoard pawn_attacks_se = bb_slide_se(board->bb_black_pawn) & (all_pieces_white | board->en_passant_target | all_attacked_mask);
        BitBoard pawn_attacks_sw = bb_slide_sw(board->bb_black_pawn) & (all_pieces_white | board->en_passant_target | all_attacked_mask);
        BitBoard ray_moves_n = bb_flood_n(board->bb_black_queen | board->bb_black_rook, empty, true);
        BitBoard ray_moves_e = bb_flood_e(board->bb_black_queen | board->bb_black_rook, empty, true);
        BitBoard ray_moves_s = bb_flood_s(board->bb_black_queen | board->bb_black_rook, empty, true);
        BitBoard ray_moves_w = bb_flood_w(board->bb_black_queen | board->bb_black_rook, empty, true);
        BitBoard ray_moves_ne = bb_flood_ne(board->bb_black_queen | board->bb_black_bishop, empty, true);
        BitBoard ray_moves_nw = bb_flood_nw(board->bb_black_queen | board->bb_black_bishop, empty, true);
        BitBoard ray_moves_se = bb_flood_se(board->bb_black_queen | board->bb_black_bishop, empty, true);
        BitBoard ray_moves_sw = bb_flood_sw(board->bb_black_queen | board->bb_black_bishop, empty, true);
        BitBoard king_moves_n = bb_slide_n(board->bb_black_king);
        BitBoard king_moves_ne = bb_slide_ne(board->bb_black_king);
        BitBoard king_moves_e = bb_slide_e(board->bb_black_king);
        BitBoard king_moves_se = bb_slide_se(board->bb_black_king);
        BitBoard king_moves_s = bb_slide_s(board->bb_black_king);
        BitBoard king_moves_sw = bb_slide_sw(board->bb_black_king);
        BitBoard king_moves_w = bb_slide_w(board->bb_black_king);
        BitBoard king_moves_nw = bb_slide_nw(board->bb_black_king);
        dirmoves[DIR_NNE] = bb_slide_n(bb_slide_n(bb_slide_e(board->bb_black_knight))) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_NEE] = bb_slide_n(bb_slide_e(bb_slide_e(board->bb_black_knight))) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_NNW] = bb_slide_n(bb_slide_n(bb_slide_w(board->bb_black_knight))) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_NWW] = bb_slide_n(bb_slide_w(bb_slide_w(board->bb_black_knight))) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_SSE] = bb_slide_s(bb_slide_s(bb_slide_e(board->bb_black_knight))) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_SEE] = bb_slide_s(bb_slide_e(bb_slide_e(board->bb_black_knight))) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_SSW] = bb_slide_s(bb_slide_s(bb_slide_w(board->bb_black_knight))) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_SWW] = bb_slide_s(bb_slide_w(bb_slide_w(board->bb_black_knight))) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_N] = (ray_moves_n | king_moves_n) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_NE] = (ray_moves_ne | king_moves_ne) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_E] = (ray_moves_e | king_moves_e) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_SE] = (pawn_attacks_se | ray_moves_se | king_moves_se) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_S] = (pawn_moves | pawn_big_moves | ray_moves_s | king_moves_s) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_SW] = (pawn_attacks_sw | ray_moves_sw | king_moves_sw) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_W] = (ray_moves_w | king_moves_w) & (all_attacked_mask | ~all_pieces_black);
        dirmoves[DIR_NW] = (ray_moves_nw | king_moves_nw) & (all_attacked_mask | ~all_pieces_black);
    }
    if ((!all_attacked) && (exclude == 0)) {
        if (white) {
            board->bb_white_moves = malloc(16 * sizeof(BitBoard));
            memcpy(board->bb_white_moves, dirmoves, 16 * sizeof(BitBoard));
        } else {
            board->bb_black_moves = malloc(16 * sizeof(BitBoard));
            memcpy(board->bb_black_moves, dirmoves, 16 * sizeof(BitBoard));
        }
    }
    return dirmoves;
}

// Returns true if the king is in check on [board]. Checks this for white if [white], otherwise checks for black.
bool in_check(Board *board, bool white) {
    BitBoard *moves = get_pseudo_legal_moves(board, !white, true, 0);
    BitBoard king_square = white ? board->bb_white_king : board->bb_black_king;
    for (int dir = 0; dir < 16; dir++) {
        if ((moves[dir] & king_square) > 0) return true;
    }
    free(moves);
    return false;
}

// Returns the number of pieces on [board] which attack [target]. Checks this for black attackers if [defenderWhite], otherwise checks for white attackers.
int num_attackers(Board *board, BitBoard target, bool defenderWhite) {
    BitBoard *moves = get_pseudo_legal_moves(board, !defenderWhite, true, 0);
    BitBoard king_square = defenderWhite ? board->bb_white_king : board->bb_black_king;
    int count = 0;
    for (int dir = 0; dir < 16; dir++) {
        if ((moves[dir] & king_square) > 0) count++;
    }
    free(moves);
    return count;
}

// Returns valid positions from which an En Passant move can be performed on [board] by white if [white], otherwise by black
BitBoard en_passant_valid(Board *board, bool white) {
    BitBoard all_pieces_white = board->bb_white_bishop | board->bb_white_king
        | board->bb_white_knight | board->bb_white_pawn | board->bb_white_queen
        | board->bb_white_rook;
    BitBoard all_pieces_black = board->bb_black_bishop | board->bb_black_king
        | board->bb_black_knight | board->bb_black_pawn | board->bb_black_queen
        | board->bb_black_rook;
    BitBoard all_pieces = all_pieces_black | all_pieces_white;
    BitBoard king_square = white ? board->bb_white_king : board->bb_black_king;
    BitBoard ept = board->en_passant_target;
    BitBoard valid = bb_slide_e(ept) | bb_slide_w(ept);
    bool one_ept_source = (valid & (valid - 1)) == 0;
    if (!one_ept_source) return valid; // two en-passant available pawns, at least one will remain to block xrays, legal
    BitBoard empty = ~(all_pieces & ~(ept | valid));
    BitBoard xray = bb_blocker_e(king_square, empty) | bb_blocker_w(king_square, empty);
    if (xray & (white ? all_pieces_black : all_pieces_white)) return 0;  // xray on en passant rank, not legal
    return valid; // no xray on en passant rank, legal
}

// Returns squares on [board] which, if in single check, moving to would eliminate the check against white's king if [defenderWhite], black otherwise.
// Only valid if single check situation
BitBoard single_check_block_tiles(Board *board, bool defenderWhite) {
    BitBoard *moves = get_pseudo_legal_moves(board, !defenderWhite, true, 0);
    BitBoard all_pieces_white = board->bb_white_bishop | board->bb_white_king
        | board->bb_white_knight | board->bb_white_pawn | board->bb_white_queen
        | board->bb_white_rook;
    BitBoard all_pieces_black = board->bb_black_bishop | board->bb_black_king
        | board->bb_black_knight | board->bb_black_pawn | board->bb_black_queen
        | board->bb_black_rook;
    BitBoard all_pieces = all_pieces_black | all_pieces_white;
    BitBoard king_square = defenderWhite ? board->bb_white_king : board->bb_black_king;
    BitBoard (*flood[])(BitBoard board, BitBoard empty, bool captures) = {&bb_flood_n, &bb_flood_ne, &bb_flood_e, &bb_flood_se, &bb_flood_s, &bb_flood_sw, &bb_flood_w, &bb_flood_nw};
    for (int dir = 0; dir < 8; dir++) {
        if ((moves[dir] & king_square) > 0) {
            //printf("check is from dir %d\n", dir);
            free(moves);
            return (*flood[(dir + 4) % 8])(king_square, ~all_pieces, true);
        }
    }
    // if we didn't find it, it's a knight
    for (int dir = 8; dir < 16; dir++) {
        if ((moves[dir] & king_square) > 0) {
            free(moves);
            switch(dir) {
                case DIR_NNE: return bb_slide_s(bb_slide_s(bb_slide_w(king_square))); break;
                case DIR_NEE: return bb_slide_s(bb_slide_w(bb_slide_w(king_square))); break;
                case DIR_NNW: return bb_slide_s(bb_slide_s(bb_slide_e(king_square))); break;
                case DIR_NWW: return bb_slide_s(bb_slide_e(bb_slide_e(king_square))); break;
                case DIR_SSE: return bb_slide_n(bb_slide_n(bb_slide_w(king_square))); break;
                case DIR_SEE: return bb_slide_n(bb_slide_w(bb_slide_w(king_square))); break;
                case DIR_SSW: return bb_slide_n(bb_slide_n(bb_slide_e(king_square))); break;
                case DIR_SWW: return bb_slide_n(bb_slide_e(bb_slide_e(king_square))); break;
            }
        }
    }
    return 0;
}

// adds [move] to array [moves], automatically adjusting the max array size tracked by [maxlen_moves] as [len_moves] grows
Move *add_to_moves(Move *moves, size_t *len_moves, size_t *maxlen_moves, Move move) {
    moves[*len_moves] = move;
    (*len_moves)++;
    if (*len_moves == *maxlen_moves) {
        (*maxlen_moves) *= 2;
        return realloc(moves, *maxlen_moves * sizeof(Move));
    }
    return moves;
}

// Returns the fully legal moves on [board].
// Caller responsible for freeing array.
Move *get_legal_moves(Board *board, int *len) {
    bool white = is_white_turn(board);
    BitBoard my_king = white ? board->bb_white_king : board->bb_black_king;
    BitBoard *pseudo_moves = get_pseudo_legal_moves(board, is_white_turn(board), false, 0);
    BitBoard *opp_pseudo_moves = get_pseudo_legal_moves(board, !is_white_turn(board), true, my_king);
    /*char bitboard_dump[80];
    printf("DEBUG: directional attack boards follow\n");
    for (int i = 0; i < 16; i++) {
        dump_bitboard(pseudo_moves[i], bitboard_dump);
        printf("dir: %d\n%s\n", i, bitboard_dump);
    }*/
    // check situations
    bool check = in_check(board, white);
    bool double_check = check && (num_attackers(board, white ? board->bb_white_king : board->bb_black_king, white) > 1);
    // get pinned pieces
    BitBoard pins = get_pins(board, white);
    // map to origin pieces by ray
    // create some useful bbs
    BitBoard all_pieces_white = board->bb_white_bishop | board->bb_white_king
        | board->bb_white_knight | board->bb_white_pawn | board->bb_white_queen
        | board->bb_white_rook;
    BitBoard all_pieces_black = board->bb_black_bishop | board->bb_black_king
        | board->bb_black_knight | board->bb_black_pawn | board->bb_black_queen
        | board->bb_black_rook;
    BitBoard my_pieces = white ? all_pieces_white : all_pieces_black;
    BitBoard my_pawns = white ? board->bb_white_pawn : board->bb_black_pawn;
    BitBoard opp_pieces = white ? all_pieces_black : all_pieces_white;
    // special considerations for checks
    BitBoard near_my_king = 0;
    BitBoard check_attacks = 0;
    if (check) {  // when in check, this is needed to evaluate legal moves
        near_my_king = my_king | bb_slide_e(my_king) | bb_slide_w(my_king);
        near_my_king = (bb_slide_n(near_my_king) | bb_slide_s(near_my_king) | near_my_king) ^ my_king;
        if (!double_check) {
            check_attacks = single_check_block_tiles(board, white);
        }
    }
    // get attacked squares
    BitBoard all_opp_attacked = 0;
    for (int i = 0; i < 16; i++) {
        all_opp_attacked |= opp_pseudo_moves[i];
    }
    /*printf("DEBUG: attacked bitboard\n");
    char bitboard_dump2[80];
    dump_bitboard(all_opp_attacked, bitboard_dump2);
    printf("%s\n", bitboard_dump2);*/
    // set up moves array
    size_t len_moves = 0;
    size_t maxlen_moves = 1;
    Move *moves = (Move *)malloc(maxlen_moves*sizeof(Move));
    Move add_move;
    memset(&add_move, 0, sizeof(add_move));
    // check every target square, if it is attacked by a direction then find piece
    // for all ray directions except E, W, we must also consider promotions if it is a pawn move near the board edge
    BitBoard piecepos = 1;
    for (int square = 0; square < 64; square++) {
        add_move.to = piecepos;
        add_move.castle = false;
        if (double_check && ((piecepos & near_my_king) == 0)) {  // only very specific moves are valid
            continue;
        }
        if (pseudo_moves[DIR_N] & piecepos) {
            add_move.from = bb_blocker_s(piecepos, ~my_pieces);
            //char movestr[8];
            //dump_move(movestr, add_move);
            //printf("testing move: %s\n", movestr);
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            //printf("valid after pin check: %s\n", move_valid ? "true" : "false");
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            //printf("valid after double check check: %s\n", move_valid ? "true" : "false");
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            //printf("valid after king move attack squares check: %s\n", move_valid ? "true" : "false");
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            //printf("valid after single check valid move check: %s\n", move_valid ? "true" : "false");
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                if (((piecepos & 0xff000000000000ff) > 0) && moving_pawn) {
                    // pawn promotion
                    for (char promotion = BISHOP; promotion <= QUEEN; promotion++) {
                        add_move.promotion = promotion;
                        moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                    }
                } else {
                    add_move.promotion = 0;
                    moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                }
            }
        }
        if (pseudo_moves[DIR_NE] & piecepos) {
            add_move.from = bb_blocker_sw(piecepos, ~my_pieces);
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool en_passant = moving_pawn && ((board->en_passant_target & bb_slide_s(piecepos)) > 0);
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            move_valid &= ((!en_passant) || en_passant_valid(board, white));
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0 || en_passant;
                if (((piecepos & 0xff000000000000ff) > 0) && moving_pawn) {
                    // pawn promotion
                    for (char promotion = BISHOP; promotion <= QUEEN; promotion++) {
                        add_move.promotion = promotion;
                        moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                    }
                } else {
                    add_move.promotion = 0;
                    moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                }
            }
        }
        if (pseudo_moves[DIR_NW] & piecepos) {
            add_move.from = bb_blocker_se(piecepos, ~my_pieces);
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool en_passant = moving_pawn && ((board->en_passant_target & bb_slide_s(piecepos)) > 0);
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            move_valid &= ((!en_passant) || en_passant_valid(board, white));
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0 || en_passant;
                if (((piecepos & 0xff000000000000ff) > 0) && moving_pawn) {
                    // pawn promotion
                    for (char promotion = BISHOP; promotion <= QUEEN; promotion++) {
                        add_move.promotion = promotion;
                        moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                    }
                } else {
                    add_move.promotion = 0;
                    moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                }
            }
        }
        if (pseudo_moves[DIR_S] & piecepos) {
            add_move.from = bb_blocker_n(piecepos, ~my_pieces);
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                if (((piecepos & 0xff000000000000ff) > 0) && moving_pawn) {
                    // pawn promotion
                    for (char promotion = BISHOP; promotion <= QUEEN; promotion++) {
                        add_move.promotion = promotion;
                        moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                    }
                } else {
                    add_move.promotion = 0;
                    moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                }
            }
        }
        if (pseudo_moves[DIR_SE] & piecepos) {
            add_move.from = bb_blocker_nw(piecepos, ~my_pieces);
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool en_passant = moving_pawn && ((board->en_passant_target & bb_slide_n(piecepos)) > 0);
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            move_valid &= ((!en_passant) || en_passant_valid(board, white));
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0 || en_passant;
                if (((piecepos & 0xff000000000000ff) > 0) && moving_pawn) {
                    // pawn promotion
                    for (char promotion = BISHOP; promotion <= QUEEN; promotion++) {
                        add_move.promotion = promotion;
                        moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                    }
                } else {
                    add_move.promotion = 0;
                    moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                }
            }
        }
        if (pseudo_moves[DIR_SW] & piecepos) {
            add_move.from = bb_blocker_ne(piecepos, ~my_pieces);
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool en_passant = moving_pawn && ((board->en_passant_target & bb_slide_n(piecepos)) > 0);
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            move_valid &= ((!en_passant) || en_passant_valid(board, white));
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0 || en_passant;
                if (((piecepos & 0xff000000000000ff) > 0) && moving_pawn) {
                    // pawn promotion
                    for (char promotion = BISHOP; promotion <= QUEEN; promotion++) {
                        add_move.promotion = promotion;
                        moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                    }
                } else {
                    add_move.promotion = 0;
                    moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
                }
            }
        }
        add_move.promotion = 0;
        if (pseudo_moves[DIR_E] & piecepos) {
            add_move.from = bb_blocker_w(piecepos, ~my_pieces);
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
            }
        }
        if (pseudo_moves[DIR_W] & piecepos) {
            add_move.from = bb_blocker_e(piecepos, ~my_pieces);
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
            }
        }
        if (pseudo_moves[DIR_NNE] & piecepos) {
            add_move.from = bb_slide_s(bb_slide_s(bb_slide_w(piecepos)));
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                add_move.promotion = 0;
                moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
            }
        }
        if (pseudo_moves[DIR_NEE] & piecepos) {
            add_move.from = bb_slide_s(bb_slide_w(bb_slide_w(piecepos)));
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                add_move.promotion = 0;
                moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
            }
        }
        if (pseudo_moves[DIR_NNW] & piecepos) {
            add_move.from = bb_slide_s(bb_slide_s(bb_slide_e(piecepos)));
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                add_move.promotion = 0;
                moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
            }
        }
        if (pseudo_moves[DIR_NWW] & piecepos) {
            add_move.from = bb_slide_s(bb_slide_e(bb_slide_e(piecepos)));
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                add_move.promotion = 0;
                moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
            }
        }
        if (pseudo_moves[DIR_SSE] & piecepos) {
            add_move.from = bb_slide_n(bb_slide_n(bb_slide_w(piecepos)));
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                add_move.promotion = 0;
                moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
            }
        }
        if (pseudo_moves[DIR_SEE] & piecepos) {
            add_move.from = bb_slide_n(bb_slide_w(bb_slide_w(piecepos)));
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                add_move.promotion = 0;
                moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
            }
        }
        if (pseudo_moves[DIR_SSW] & piecepos) {
            add_move.from = bb_slide_n(bb_slide_n(bb_slide_e(piecepos)));
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                add_move.promotion = 0;
                moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
            }
        }
        if (pseudo_moves[DIR_SWW] & piecepos) {
            add_move.from = bb_slide_n(bb_slide_e(bb_slide_e(piecepos)));
            bool moving_king = (add_move.from & my_king) > 0;
            bool moving_pawn = (my_pawns & add_move.from) > 0;
            bool move_valid = (add_move.from & pins) == 0;  // not moving pinned piece
            move_valid &= (moving_king || !double_check);  // only king moves allowed in double check
            move_valid &= ((all_opp_attacked & piecepos) == 0 || !moving_king);  // if moving king, not to attacked square
            move_valid &= (((check_attacks & piecepos) > 0 || moving_king) || !check); // single check allows king move, taking checking piece, blocking
            if (move_valid) {
                add_move.capture = (piecepos & opp_pieces) > 0;
                add_move.promotion = 0;
                moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
            }
        }
        piecepos <<= 1;
    }
    BitBoard all_pieces = all_pieces_black | all_pieces_white;
    // castling moves
    /*char dumpboard[80];
    dump_bitboard(all_opp_attacked, dumpboard);
    printf("Opponent attacked:\n%s\n", dumpboard);
    printf("pieces clear (White kingside): %d\n", ((all_pieces & 0x0000000000000060) == 0));
    printf("path safe (White kingside): %d\n", ((all_opp_attacked & 0x000000000000001e) == 0));
    printf("castle rights (White kingside): %d\n", white && board->can_castle_wk);
    printf("pieces clear (White queenside): %d\n", ((all_pieces & 0x000000000000000e) == 0));
    printf("path safe (White queenside): %d\n", ((all_opp_attacked & 0x0000000000000070) == 0));
    printf("castle rights (White queenside): %d\n", white && board->can_castle_wq);
    printf("pieces clear (Black kingside): %d\n", ((all_pieces & 0x6000000000000000) == 0));
    printf("path safe (Black kingside): %d\n", ((all_opp_attacked & 0x1e00000000000000) == 0));
    printf("castle rights (Black kingside): %d\n", white && board->can_castle_bk);
    printf("pieces clear (Black queenside): %d\n", ((all_pieces & 0x0e00000000000000) == 0));
    printf("path safe (Black queenside): %d\n", ((all_opp_attacked & 0x7000000000000000) == 0));
    printf("castle rights (Black queenside): %d\n", white && board->can_castle_bq);
    printf("-------------------------------\n");*/
    if (white && board->can_castle_wk && ((all_opp_attacked & 0x0000000000000070) == 0) && ((all_pieces & 0x0000000000000060) == 0)) {
        // white kingside
        add_move.capture = false;
        add_move.castle = true;
        add_move.promotion = 0;
        add_move.from = board->bb_white_king;
        add_move.to = bb_slide_e(bb_slide_e(board->bb_white_king));
        moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
    }
    if (white && board->can_castle_wq && ((all_opp_attacked & 0x000000000000001e) == 0) && ((all_pieces & 0x000000000000000e) == 0)) {
        // white queenside
        add_move.capture = false;
        add_move.castle = true;
        add_move.promotion = 0;
        add_move.from = board->bb_white_king;
        add_move.to = bb_slide_w(bb_slide_w(bb_slide_w(board->bb_white_king)));
        moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
    }
    if ((!white) && board->can_castle_bk && ((all_opp_attacked & 0x7000000000000000) == 0) && ((all_pieces & 0x6000000000000000) == 0)) {
        // black kingside
        add_move.capture = false;
        add_move.castle = true;
        add_move.promotion = 0;
        add_move.from = board->bb_black_king;
        add_move.to = bb_slide_e(bb_slide_e(board->bb_black_king));
        moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
    }
    if ((!white) && board->can_castle_bq && ((all_opp_attacked & 0x1e00000000000000) == 0) && ((all_pieces & 0x0e00000000000000) == 0)) {
        // black queenside
        add_move.capture = false;
        add_move.castle = true;
        add_move.promotion = 0;
        add_move.from = board->bb_black_king;
        add_move.to = bb_slide_w(bb_slide_w(bb_slide_w(board->bb_black_king)));
        moves = add_to_moves(moves, &len_moves, &maxlen_moves, add_move);
    }
    // shrink array to fit
    moves = realloc(moves, len_moves * sizeof(Move));
    *len = len_moves;
    free(pseudo_moves);
    free(opp_pseudo_moves);
    return moves;
}

// Returns true if a threefold repetition has occurred on [board]
bool is_threefold_draw(Board *board) {
    if (API == NULL) start_chess_api();
    // i hate everything
    int cur_size = 0;
    int max_size = 1;
    Board **boards = malloc(sizeof(Board *));
    int *counts = malloc(sizeof(int));
    Board *cur_board = board;
    bool hit, found = false;
    while (cur_board) {
        hit = false;
        for (int i = 0; i < cur_size; i++) {
            if (board_equals(boards[i], cur_board)) {
                counts[i]++;
                hit = true;
                if (counts[i] >= 3) {
                    found = true;
                }
                break;
            }
        }
        if (found) break;
        if (!hit) {
            if (cur_size == max_size) {
                max_size *= 2;
                boards = realloc(boards, max_size*sizeof(Board *));
                counts = realloc(counts, max_size*sizeof(int));
            }
            boards[cur_size] = cur_board;
            counts[cur_size] = 0;
            cur_size++;
        }
        cur_board = cur_board->last_board;
    }
    free(boards);
    free(counts);
    if (!hit) return false;
    return true;
}

// Returns GAME_NORMAL, GAME_STALEMATE or GAME_CHECKMATE based on the state on [board]
int get_board_end_state(Board *board) {
    if (board->halfmoves >= 50) return GAME_STALEMATE;
    if (is_threefold_draw(board)) return GAME_STALEMATE;
    int num_legal_moves;
    free(get_legal_moves(board, &num_legal_moves));
    if (num_legal_moves > 0) return GAME_NORMAL;
    bool check = in_check(board, board->whiteToMove);
    if (check) return GAME_CHECKMATE;
    return GAME_STALEMATE;
}

// Starts the Chess API internals, and returns the interface to the bot for access.
void start_chess_api() {
    API = (InternalAPI *)malloc(sizeof(InternalAPI));
    API->shared_board = NULL;
    API->wtime = 0;
    API->btime = 0;
    sem_init(&API->intermission_mutex, 0, 0);
    // setup zobrist keys
    srand(time(NULL));
    for (int i = 0; i < 781; i++) {
        zobrist_keys[i] = rand_long();
    }
    // start the uci server in its own thread
    uci_start(&API->uci_thread);
    // block until uci endpoint says go
    sem_wait(&API->intermission_mutex);
}

Board *chess_get_board() {
    if (API == NULL) start_chess_api();
    return interface_get_board();
}

Move *chess_get_legal_moves(Board *board, int *len) {
    if (API == NULL) start_chess_api();
    return get_legal_moves(board, len);
}

bool chess_is_white_turn(Board *board) {
    return is_white_turn(board);
}

GameState chess_is_game_ended(Board *board) {
    return get_board_end_state(board);
}

uint64_t chess_zobrist_key(Board *board) {
    return board->hash;
}

void chess_make_move(Board *board, Move move) {
    if (API == NULL) start_chess_api();
    make_move(board, move);
}

void chess_undo_move(Board *board) {
    if (API == NULL) start_chess_api();
    undo_move(board);
}

void chess_free_board(Board *board) {
    if (API == NULL) start_chess_api();
    free_board(board);
}

long chess_get_time_millis() {
    if (API == NULL) start_chess_api();
    return interface_get_time_millis();
}

long chess_get_opponent_time_millis() {
    if (API == NULL) start_chess_api();
    return interface_get_opponent_time_millis();
}

long chess_get_elapsed_time_millis() {
    if (API == NULL) start_chess_api();
    return interface_get_elapsed_time_millis();
}

void chess_free_moves_array(Move *moves) {
    free(moves);
}

void chess_push(Move move) {
    if (API == NULL) start_chess_api();
    interface_push(move);
}

void chess_done() {
    if (API == NULL) start_chess_api();
    interface_done();
}

BitBoard chess_get_bitboard(Board *board, PlayerColor color, PieceType piece_type) {
    switch(piece_type) {
        case PAWN: return ((color == WHITE) ? board->bb_white_pawn : board->bb_black_pawn);
        case ROOK: return ((color == WHITE) ? board->bb_white_rook : board->bb_black_rook);
        case BISHOP: return ((color == WHITE) ? board->bb_white_bishop : board->bb_black_bishop);
        case KING: return ((color == WHITE) ? board->bb_white_king : board->bb_black_king);
        case KNIGHT: return ((color == WHITE) ? board->bb_white_knight : board->bb_black_knight);
        case QUEEN: return ((color == WHITE) ? board->bb_white_queen : board->bb_black_queen);
    }
    return 0;  // bad piece_type
}

bool chess_is_check(Board *board) {
    return in_check(board, board->whiteToMove);
}

void chess_skip_turn(Board *board) {
    Move null_move;  // quite literally!
    memset(&null_move, 0, sizeof(Move));
    make_move(board, null_move);
}

bool chess_in_check(Board *board) {
    return in_check(board, board->whiteToMove);
}

bool chess_in_checkmate(Board *board) {
    int num_legal_moves;
    free(get_legal_moves(board, &num_legal_moves));
    if (num_legal_moves > 0) return false;
    return in_check(board, board->whiteToMove);
}

bool chess_in_draw(Board *board) {
    if (board->halfmoves >= 50) return true;
    if (is_threefold_draw(board)) return true;
    int num_legal_moves;
    free(get_legal_moves(board, &num_legal_moves));
    if (num_legal_moves > 0) return false;
    return !in_check(board, board->whiteToMove);
}

bool chess_can_kingside_castle(Board *board, PlayerColor color) {
    return (color == BLACK) ? board->can_castle_bk : board->can_castle_wk;
}

bool chess_can_queenside_castle(Board *board, PlayerColor color) {
    return (color == BLACK) ? board->can_castle_bq : board->can_castle_wq;
}
