#pragma once
#include <string>
#include <vector>
#include "bitboard/bitboard.h"
#include "bitboard/types.h"

struct StateInfo {
    Piece   captured;
    uint8_t castling;
    Square  epSquare;
    int     halfmove;
    Key     key;
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
        int      halfmove_clock() const              { return halfmove; }
        Key      key() const                         { return hashKey; }
        Key      compute_key() const;
        bool     is_repetition() const;
        bool     is_draw() const                     { return halfmove >= 100 || is_repetition(); }
        Bitboard attackers_to(Square s, Bitboard occ) const;
        bool     in_check() const { return attackers_to(king_square(sideToMove), pieces()) & pieces(~sideToMove); }
        void do_move(Move m,StateInfo& st);
        void undo_move(Move m, const StateInfo& st);
        void do_null_move(StateInfo& st);
        void undo_null_move(const StateInfo& st);
        int  non_pawn_count(Color c) const { return popcount(pieces(c) & ~(pieces(PAWN) | pieces(KING))); }
        bool is_legal(Move m);
        
    
    private:
        void put_piece(Piece p, Square s);
        void remove_piece(Square s);
        void clear();
        bool ep_capturable() const;

        Piece       board[SQUARE_NB];
        Bitboard    byType[PIECE_TYPE_NB];
        Bitboard    byColor[COLOR_NB];
        Color       sideToMove  = WHITE;
        uint8_t     castling    = 0;
        Square      epSquare    =NO_SQUARE;
        int         halfmove    =0;
        int         fullmove    =1;
        Key         hashKey     = 0;
        std::vector<Key> keyHistory;
};