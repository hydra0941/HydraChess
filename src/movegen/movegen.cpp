#include "movegen/movegen.h"

namespace {

Bitboard attacks_from(PieceType pt, Square s, Bitboard occ) {
    switch (pt) {
        case KNIGHT: return KnightAttacks[s];
        case BISHOP: return bishop_attacks(s, occ);
        case ROOK:   return rook_attacks(s, occ);
        case QUEEN:  return queen_attacks(s, occ);
        default:     return KingAttacks[s];
    }
}

bool square_attacked(const Position& pos, Square s, Color by) {
    return pos.attackers_to(s, pos.pieces()) & pos.pieces(by);
}

void add_promotions(MoveList& list, Square from, Square to, bool capture) {
    int base = capture ? PROMO_CAPTURE_N : PROMO_N;
    for (int i = 0; i < 4; ++i)
        list.add(make_move(from, to, base + i));
}

void generate_pieces(const Position& pos, MoveList& list, Color us) {
    Bitboard own   = pos.pieces(us);
    Bitboard enemy = pos.pieces(~us);
    Bitboard occ   = pos.pieces();

    for (PieceType pt : {KNIGHT, BISHOP, ROOK, QUEEN, KING}) {
        Bitboard bb = pos.pieces(us, pt);
        while (bb) {
            Square from = pop_lsb(bb);
            Bitboard targets = attacks_from(pt, from, occ) & ~own;
            while (targets) {
                Square to = pop_lsb(targets);
                list.add(make_move(from, to, (square_bb(to) & enemy) ? CAPTURE : QUIET));
            }
        }
    }
}

void generate_pawns(const Position& pos, MoveList& list, Color us) {
    Bitboard pawns = pos.pieces(us, PAWN);
    Bitboard empty = ~pos.pieces();
    Bitboard enemy = pos.pieces(~us);

    const int up        = (us == WHITE) ? 8 : -8;
    const int startRank = (us == WHITE) ? 1 : 6;
    const int promoRank = (us == WHITE) ? 6 : 1;

    while (pawns) {
        Square from  = pop_lsb(pawns);
        bool   promo = rank_of(from) == promoRank;

        Square one = Square(from + up);
        if (empty & square_bb(one)) {
            if (promo) {
                add_promotions(list, from, one, false);
            } else {
                list.add(make_move(from, one, QUIET));
                if (rank_of(from) == startRank) {
                    Square two = Square(from + 2 * up);
                    if (empty & square_bb(two))
                        list.add(make_move(from, two, DOUBLE_PUSH));
                }
            }
        }

        Bitboard caps = PawnAttacks[us][from] & enemy;
        while (caps) {
            Square to = pop_lsb(caps);
            if (promo) add_promotions(list, from, to, true);
            else       list.add(make_move(from, to, CAPTURE));
        }

        Square ep = pos.ep_square();
        if (ep != NO_SQUARE && (PawnAttacks[us][from] & square_bb(ep)))
            list.add(make_move(from, ep, EP_CAPTURE));
    }
}

void generate_castling(const Position& pos, MoveList& list, Color us) {
    const int r = (us == WHITE) ? 0 : 7;
    const Color them = ~us;
    Bitboard occ = pos.pieces();
    int rights = pos.castling_rights();

    Square e = make_square(4, r), f = make_square(5, r), g = make_square(6, r);
        bool in_check() const {return attackers_to(king_square(side_to_move), pieces()) & pieces(~)}
    Square d = make_square(3, r), c = make_square(2, r), b = make_square(1, r);

    int kingSide  = (us == WHITE) ? WHITE_OO  : BLACK_OO;
    int queenSide = (us == WHITE) ? WHITE_OOO : BLACK_OOO;

    if ((rights & kingSide)
        && !(occ & (square_bb(f) | square_bb(g)))
        && !square_attacked(pos, e, them)
        && !square_attacked(pos, f, them)
        && !square_attacked(pos, g, them))
        list.add(make_move(e, g, KING_CASTLE));

    if ((rights & queenSide)
        && !(occ & (square_bb(b) | square_bb(c) | square_bb(d)))
        && !square_attacked(pos, e, them)
        && !square_attacked(pos, d, them)
        && !square_attacked(pos, c, them))
        list.add(make_move(e, c, QUEEN_CASTLE));
}

}

void generate(const Position& pos, MoveList& list) {
    Color us = pos.side_to_move();
    generate_pawns(pos, list, us);
    generate_pieces(pos, list, us);
    generate_castling(pos, list, us);
}