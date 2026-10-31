#pragma once
#include <string>
#include "bitboard/bitboard.h"

struct StateInfo {
    Piece   captured;
    uint8_t castling;
    Square  epSquare;
    int     halfmove;
};

class Position {
    public:
        void set(const std::string& fen);
        std::string fen() const;

        Bitboard pieces() const                      { return byColor[WHITE] | byColor[BLACK]; }
        Bitboard pieces(Color c) const               { return byColor[c]; }
        Bitboard pieces(PieceType pt) const          { return byType[pt]; }
        Bitboard pieces(Color c, PieceType pt) const { return byColor[c] & byType[pt]; }
        Piece    piece_on(Square s) const            { return board[s]; }
        Square   king_square(Color c) const          { return lsb(pieces(c, KING)); }
        Color    side_to_move() const                { return sideToMove; }
        int      castling_rights() const             { return castling; }
        Square   ep_square() const                   { return epSquare; }
        Bitboard attackers_to(Square s, Bitboard occ) const;
        void do_move(Move m,StateInfo& st);
        void undo_move(Move m, const StateInfo& st);
        
    
    private:
        void put_piece(Piece p, Square s);
        void remove_piece(Square s);
        void clear();

        Piece       board[SQUARE_NB];
        Bitboard    byType[PIECE_TYPE_NB];
        Bitboard    byColor[COLOR_NB];
        Color       sideToMove  = WHITE;
        uint8_t     castling    = 0;
        Square      epSquare    =NO_SQUARE;
        int         halfmove    =0;
        int         fullmove    =1;
};