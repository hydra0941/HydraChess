#pragma once
#include <bit>
#include "bitboard/types.h"

void print_bitboard(Bitboard b);

constexpr int popcount(Bitboard b) { return std::popcount(b); }
constexpr Square lsb(Bitboard b)   { return Square(std::countr_zero(b)); } // b is nonzero
inline Square pop_lsb(Bitboard& b) { Square s = lsb(b); b &= b - 1; return s; }
constexpr bool double_check(Bitboard b) { return b & (b - 1); }

constexpr Bitboard FILE_A_BB = 0x0101010101010101ULL;
constexpr Bitboard FILE_H_BB = FILE_A_BB << 7;
constexpr Bitboard RANK_1_BB = 0xFFULL;
constexpr Bitboard RANK_8_BB = RANK_1_BB << 56;

constexpr Bitboard file_bb(int f) { return FILE_A_BB << f; }
constexpr Bitboard rank_bb(int r) { return RANK_1_BB << (8 * r); }

extern Bitboard KnightAttacks[SQUARE_NB];
extern Bitboard KingAttacks[SQUARE_NB];
extern Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];
void init_attacks();

Bitboard rook_attacks(Square s, Bitboard occupied);
Bitboard bishop_attacks(Square s, Bitboard occupied);
inline Bitboard queen_attacks(Square s, Bitboard occupied) {
    return rook_attacks(s, occupied) | bishop_attacks(s, occupied);
}