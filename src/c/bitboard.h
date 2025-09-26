#pragma once

#include <stdint.h>
#include <stdbool.h>

//! For Windows(MSVC) compatibility
#ifndef DLLEXPORT
    #if defined(_MSC_VER)
        #define DLLEXPORT __declspec(dllexport)
    #else
        #define DLLEXPORT
    #endif
#endif

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

#ifdef __cplusplus
extern "C" {
#endif

// Debug print function
// [buffer] should be at least 73 bytes
DLLEXPORT void dump_bitboard(BitBoard board, char *buffer);

// Directional BitBoard functions below! Each function is related to sliding motion along the cardinal directions,
// and thus each has 8 copies. Sorry.

// Directional BitBoard slide functions translate the entire [board] in the given direction.

DLLEXPORT BitBoard bb_slide_n(BitBoard board);
DLLEXPORT BitBoard bb_slide_ne(BitBoard board);
DLLEXPORT BitBoard bb_slide_e(BitBoard board);
DLLEXPORT BitBoard bb_slide_se(BitBoard board);
DLLEXPORT BitBoard bb_slide_s(BitBoard board);
DLLEXPORT BitBoard bb_slide_sw(BitBoard board);
DLLEXPORT BitBoard bb_slide_w(BitBoard board);
DLLEXPORT BitBoard bb_slide_nw(BitBoard board);

// Directional BitBoard flood functions travel from position [board] in the given direction
// marking spaces until encountering an occluded space according to [empty], then return all
// marked spaces. If [captures], then includes the occluded space.

DLLEXPORT BitBoard bb_flood_n(BitBoard board, BitBoard empty, bool captures);
DLLEXPORT BitBoard bb_flood_ne(BitBoard board, BitBoard empty, bool captures);
DLLEXPORT BitBoard bb_flood_e(BitBoard board, BitBoard empty, bool captures);
DLLEXPORT BitBoard bb_flood_se(BitBoard board, BitBoard empty, bool captures);
DLLEXPORT BitBoard bb_flood_s(BitBoard board, BitBoard empty, bool captures);
DLLEXPORT BitBoard bb_flood_sw(BitBoard board, BitBoard empty, bool captures);
DLLEXPORT BitBoard bb_flood_w(BitBoard board, BitBoard empty, bool captures);
DLLEXPORT BitBoard bb_flood_nw(BitBoard board, BitBoard empty, bool captures);

// Directional BitBoard blocker functions travel from position [board] in the given direction
// until encountering an occluded space according to [empty], then returns this occluded space.

DLLEXPORT BitBoard bb_blocker_n(BitBoard board, BitBoard empty);
DLLEXPORT BitBoard bb_blocker_ne(BitBoard board, BitBoard empty);
DLLEXPORT BitBoard bb_blocker_e(BitBoard board, BitBoard empty);
DLLEXPORT BitBoard bb_blocker_se(BitBoard board, BitBoard empty);
DLLEXPORT BitBoard bb_blocker_s(BitBoard board, BitBoard empty);
DLLEXPORT BitBoard bb_blocker_sw(BitBoard board, BitBoard empty);
DLLEXPORT BitBoard bb_blocker_w(BitBoard board, BitBoard empty);
DLLEXPORT BitBoard bb_blocker_nw(BitBoard board, BitBoard empty);

#ifdef __cplusplus
}
#endif
