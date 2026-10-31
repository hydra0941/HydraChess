#include <cstdint>
#pragma once

using Bitboard = uint64_t;
enum Color : uint8_t {WHITE, BLACK, COLOR_NB = 2};
enum PieceType : uint8_t {
    NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    PIECE_TYPE_NB = 7
};

enum Piece : uint8_t {
    NO_PIECE,
    W_PAWN = 1, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = 9, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    PIECE_NB = 16
};

#define RANK(r) A##r, B##r, C##r, D##r, E##r, F##r, G##r, H##r
enum Square : uint8_t {
    RANK(1), RANK(2), RANK(3), RANK(4),
    RANK(5), RANK(6), RANK(7), RANK(8),
    SQUARE_NB, NO_SQUARE = SQUARE_NB
};
#undef RANK

enum CastlingRights : uint8_t {
    WHITE_OO = 1, WHITE_OOO = 2, BLACK_OO = 4, BLACK_OOO = 8
};

enum MoveFlag : int {
    QUIET = 0, DOUBLE_PUSH = 1, KING_CASTLE = 2, QUEEN_CASTLE = 3,
    CAPTURE = 4, EP_CAPTURE = 5,
    PROMO_N = 8, PROMO_B = 9, PROMO_R = 10, PROMO_Q = 11,
    PROMO_CAPTURE_N = 12, PROMO_CAPTURE_B = 13, PROMO_CAPTURE_R = 14, PROMO_CAPTURE_Q = 15
};

using Move = uint16_t;

constexpr Bitboard square_bb(Square s) {return 1ULL << s;}
constexpr int file_of(Square s) {return s & 7;}
constexpr int rank_of(Square s) {return s >> 3;}
constexpr Square make_square(int f, int r) {return Square(r * 8 + f);}

constexpr Color      color_of(Piece p) {return Color(p >> 3);}
constexpr PieceType  type_of(Piece p) {return PieceType(p & 7);}
constexpr Piece      make_piece(Color c, PieceType pt) {return Piece((c << 3) | pt);}
constexpr Color      operator~(Color c) {return Color(c ^ 1);}

constexpr Move make_move(Square from, Square to, int flags = 0) {
    return Move(from | (to << 6) | (flags << 12));
}
constexpr Square from_sq(Move m) {return Square(m & 63); }
constexpr Square to_sq(Move m) {return Square((m >> 6) & 63); }
constexpr int    flags(Move m) {return m >> 12;}


