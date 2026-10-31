#include <iostream>
#include "bitboard/bitboard.h"
#include <cstddef>

Bitboard KnightAttacks[SQUARE_NB];
Bitboard KingAttacks[SQUARE_NB];
Bitboard PawnAttacks[COLOR_NB][SQUARE_NB];

namespace{
    struct Step {int df, dr; };
    constexpr Step KnightSteps[8] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
    constexpr Step KingSteps[8] = {{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}};
    constexpr Step WhitePawn[2] = {{-1,1},{1,1}};
    constexpr Step BlackPawn[2] = {{-1,-1},{1,-1}};

    template<std::size_t N>
    Bitboard targets(int sq, const Step (&steps)[N]) {
        Bitboard bb = 0;
        int f = sq & 7, r = sq >> 3;
        for(const Step& s : steps) {
            int nf = f + s.df, nr = r + s.dr;
            if(nf >= 0 && nf < 8 && nr >= 0 && nr <8)
                bb |= 1ULL << (nr * 8 + nf);
        }
        return bb;
    }

    constexpr Step RookDirs[4]   = {{1,0},{-1,0},{0,1},{0,-1}};
    constexpr Step BishopDirs[4] = {{1,1},{1,-1},{-1,1},{-1,-1}};

    Bitboard slide(Square s, Bitboard occupied, const Step (&dirs)[4]) {
    Bitboard bb = 0;
    for (const Step& d : dirs) {
        int f = file_of(s) + d.df, r = rank_of(s) + d.dr;
        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            Bitboard sq = 1ULL << (r * 8 + f);
            bb |= sq;
            if (occupied & sq) break;
            f += d.df;
            r += d.dr;
        }
    }
    return bb;
    }

}

Bitboard rook_attacks(Square s, Bitboard occ)   {return slide(s, occ, RookDirs);}
Bitboard bishop_attacks(Square s, Bitboard occ) {return slide(s, occ, BishopDirs);}

void init_attacks() {
    for (int sq = 0; sq < SQUARE_NB; ++sq) {
        KnightAttacks[sq]        = targets(sq, KnightSteps);
        KingAttacks[sq]          = targets(sq, KingSteps);
        PawnAttacks[WHITE][sq]   = targets(sq, WhitePawn);
        PawnAttacks[BLACK][sq]   = targets(sq, BlackPawn);
    }
}


void print_bitboard(Bitboard b){
    for(int r = 7; r >= 0; --r){
        std::cout << r +1 << " ";
        for(int f =0; f < 8; f++)
            std::cout << ((b >> (r * 8 + f)) & 1 ? "1 " : ". ");
        std::cout << '\n';
    }
    std::cout << "\n a b c d e f g h\n\n";
}
