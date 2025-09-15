#include <stdint.h>
#include <stdbool.h>

// A BitBoard is a way of representing the spaces of the chess board. Each bit corresponds to
// a square on the board, and is on or off depending on what data that BitBoard represents.
// For example, if BitBoard [pawns] represents white pawn positions, we can find all pawn attacks with:
//
// BitBoard attacks = bb_slide_nw(pawns) | bb_slide_ne(pawns);
//
// By convention, the bits are matched to board squares from left-to-right, bottom-to-top.
// That is, the LSB corresponds to the a1 square, the 8th lowest bit corresponds to the a8 square, and the MSB corresponds to the h8 square.
// Bitboards enable very efficient operations on bulk sets of pieces.

typedef uint64_t BitBoard;

// Debug print function
// [buffer] should be at least 72 bytes
void dump_bitboard(BitBoard board, char *buffer);

// Directional BitBoard functions below! Each function is related to sliding motion along the cardinal directions,
// and thus each has 8 copies. Sorry.

// Directional BitBoard slide functions translate the entire [board] in the given direction.

BitBoard bb_slide_n(BitBoard board);
BitBoard bb_slide_ne(BitBoard board);
BitBoard bb_slide_e(BitBoard board);
BitBoard bb_slide_se(BitBoard board);
BitBoard bb_slide_s(BitBoard board);
BitBoard bb_slide_sw(BitBoard board);
BitBoard bb_slide_w(BitBoard board);
BitBoard bb_slide_nw(BitBoard board);

// Directional BitBoard flood functions travel from position [board] in the given direction
// marking spaces until encountering an occluded space according to [empty], then return all
// marked spaces. If [captures], then includes the occluded space.

BitBoard bb_flood_n(BitBoard board, BitBoard empty, bool captures);
BitBoard bb_flood_ne(BitBoard board, BitBoard empty, bool captures);
BitBoard bb_flood_e(BitBoard board, BitBoard empty, bool captures);
BitBoard bb_flood_se(BitBoard board, BitBoard empty, bool captures);
BitBoard bb_flood_s(BitBoard board, BitBoard empty, bool captures);
BitBoard bb_flood_sw(BitBoard board, BitBoard empty, bool captures);
BitBoard bb_flood_w(BitBoard board, BitBoard empty, bool captures);
BitBoard bb_flood_nw(BitBoard board, BitBoard empty, bool captures);

// Directional BitBoard blocker functions travel from position [board] in the given direction
// until encountering an occluded space according to [empty], then returns this occluded space.

BitBoard bb_blocker_n(BitBoard board, BitBoard empty);
BitBoard bb_blocker_ne(BitBoard board, BitBoard empty);
BitBoard bb_blocker_e(BitBoard board, BitBoard empty);
BitBoard bb_blocker_se(BitBoard board, BitBoard empty);
BitBoard bb_blocker_s(BitBoard board, BitBoard empty);
BitBoard bb_blocker_sw(BitBoard board, BitBoard empty);
BitBoard bb_blocker_w(BitBoard board, BitBoard empty);
BitBoard bb_blocker_nw(BitBoard board, BitBoard empty);
