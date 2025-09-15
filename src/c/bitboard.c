#include "bitboard.h"

#define _bb_flood(dir) BitBoard bb_flood_ ## dir (BitBoard board, BitBoard empty, bool captures) { \
    BitBoard gen = board; \
    for (int i = 0; i < 7; i++) { \
        gen |= bb_slide_ ## dir (gen) & empty; \
    } \
    return captures ? bb_slide_ ## dir (gen) : gen & empty; \
}

#define _bb_blocker(dir) BitBoard bb_blocker_ ## dir (BitBoard board, BitBoard empty) { \
    BitBoard gen = board; \
    for (int i = 0; i < 7; i++) { \
        gen = bb_slide_ ## dir (gen); \
        if ((gen & empty) == 0) return gen; \
    } \
    return gen; \
}

#define _all_dirs(submacro) submacro(n) \
    submacro(ne) \
    submacro(e) \
    submacro(se) \
    submacro(s) \
    submacro(sw) \
    submacro(w) \
    submacro(nw) \

// Debug print function
// [buffer] should be at least 72 bytes
void dump_bitboard(BitBoard board, char *buffer) {
    char *bufptr = buffer + 63;
    for (int i = 0; i < 64; i++) {
        *bufptr = (board & 1ul) > 0 ? 'X' : '-';
        board >>= 1;
        bufptr++;
        if (i % 8 == 7) {
            *bufptr = '\n';
            bufptr -= 17;
        } 
    }
    buffer[72] = '\0';
}

// Directional BitBoard functions below! Each function is related to sliding motion along the cardinal directions,
// and thus each has 8 copies. Sorry.

// Directional BitBoard slide functions translate the entire [board] in the given direction.

BitBoard bb_slide_n(BitBoard board) {
    return board << 8;
}

BitBoard bb_slide_s(BitBoard board) {
    return board >> 8;
}

BitBoard bb_slide_e(BitBoard board) {
    return (board << 1) & 0xfefefefefefefefe;
}

BitBoard bb_slide_w(BitBoard board) {
    return (board >> 1) & 0x7f7f7f7f7f7f7f7f;
}

BitBoard bb_slide_ne(BitBoard board) {
    return (board << 9) & 0xfefefefefefefefe;
}

BitBoard bb_slide_se(BitBoard board) {
    return (board >> 7) & 0xfefefefefefefefe;
}

BitBoard bb_slide_nw(BitBoard board) {
    return (board << 7) & 0x7f7f7f7f7f7f7f7f;
}

BitBoard bb_slide_sw(BitBoard board) {
    return (board >> 9) & 0x7f7f7f7f7f7f7f7f;
}

// Directional BitBoard flood functions travel from position [board] in the given direction
// marking spaces until encountering an occluded space according to [empty], then return all
// marked spaces. If [captures], then includes the occluded space.

_all_dirs(_bb_flood)

// Directional BitBoard blocker functions travel from position [board] in the given direction
// until encountering an occluded space according to [empty], then returns this occluded space.

_all_dirs(_bb_blocker)
