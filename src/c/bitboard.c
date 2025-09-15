#include "bitboard.h"

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

BitBoard bb_flood_n(BitBoard board, BitBoard empty, bool captures) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen |= bb_slide_n(gen) & empty;
    }
    return captures ? bb_slide_n(gen) : gen & empty;
}

BitBoard bb_flood_ne(BitBoard board, BitBoard empty, bool captures) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen |= bb_slide_ne(gen) & empty;
    }
    return captures ? bb_slide_ne(gen) : gen & empty;
}

BitBoard bb_flood_e(BitBoard board, BitBoard empty, bool captures) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen |= bb_slide_e(gen) & empty;
    }
    return captures ? bb_slide_e(gen) : gen & empty;
}

BitBoard bb_flood_se(BitBoard board, BitBoard empty, bool captures) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen |= bb_slide_se(gen) & empty;
    }
    return captures ? bb_slide_se(gen) : gen & empty;
}

BitBoard bb_flood_s(BitBoard board, BitBoard empty, bool captures) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen |= bb_slide_s(gen) & empty;
    }
    return captures ? bb_slide_s(gen) : gen & empty;
}

BitBoard bb_flood_sw(BitBoard board, BitBoard empty, bool captures) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen |= bb_slide_sw(gen) & empty;
    }
    return captures ? bb_slide_sw(gen) : gen & empty;
}

BitBoard bb_flood_w(BitBoard board, BitBoard empty, bool captures) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen |= bb_slide_w(gen) & empty;
    }
    return captures ? bb_slide_w(gen) : gen & empty;
}

BitBoard bb_flood_nw(BitBoard board, BitBoard empty, bool captures) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen |= bb_slide_nw(gen) & empty;
    }
    return captures ? bb_slide_nw(gen) : gen & empty;
}

// Directional BitBoard blocker functions travel from position [board] in the given direction
// until encountering an occluded space according to [empty], then returns this occluded space.

BitBoard bb_blocker_n(BitBoard board, BitBoard empty) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_n(gen);
        if ((gen & empty) == 0) return gen;
    }
    return gen;
}

BitBoard bb_blocker_ne(BitBoard board, BitBoard empty) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_ne(gen);
        if ((gen & empty) == 0) return gen;
    }
    return gen;
}

BitBoard bb_blocker_e(BitBoard board, BitBoard empty) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_e(gen);
        if ((gen & empty) == 0) return gen;
    }
    return gen;
}

BitBoard bb_blocker_se(BitBoard board, BitBoard empty) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_se(gen);
        if ((gen & empty) == 0) return gen;
    }
    return gen;
}

BitBoard bb_blocker_s(BitBoard board, BitBoard empty) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_s(gen);
        if ((gen & empty) == 0) return gen;
    }
    return gen;
}

BitBoard bb_blocker_sw(BitBoard board, BitBoard empty) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_sw(gen);
        if ((gen & empty) == 0) return gen;
    }
    return gen;
}

BitBoard bb_blocker_w(BitBoard board, BitBoard empty) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_w(gen);
        if ((gen & empty) == 0) return gen;
    }
    return gen;
}

BitBoard bb_blocker_nw(BitBoard board, BitBoard empty) {
    BitBoard gen = board;
    for (int i = 0; i < 7; i++) {
        gen = bb_slide_nw(gen);
        if ((gen & empty) == 0) return gen;
    }
    return gen;
}
