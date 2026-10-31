#include "position/position.h"
#include <sstream>

namespace {
    const std::string PieceChars = " PNBRQK  pnbrqk";
    uint8_t CastleMask[SQUARE_NB];
    struct CastleInit {
        CastleInit() {
            for(auto& m: CastleMask) m = 15;
            CastleMask[E1] = 15 & ~(WHITE_OO | WHITE_OOO);
            CastleMask[H1] = 15 & ~WHITE_OO;
            CastleMask[A1] = 15 & ~WHITE_OOO;
            CastleMask[E8] = 15 & ~(BLACK_OO | BLACK_OOO);
            CastleMask[H8] = 15 & ~BLACK_OO;
            CastleMask[A8] = 15 & ~BLACK_OOO;
        }
    } castleInit;
}

void Position::clear() {
    for (auto& p : board) p = NO_PIECE;
    for (auto& b : byType) b = 0;
    for (auto& b : byColor) b = 0;
    sideToMove = WHITE;
    castling = 0;
    epSquare = NO_SQUARE;
    halfmove = 0;
    fullmove = 1;
}

void Position::put_piece(Piece p, Square s) {
    board[s] = p;
    byType[type_of(p)]   |= square_bb(s);
    byColor[color_of(p)] |= square_bb(s);
}

void Position::remove_piece(Square s) {
    Piece p = board[s];
    byType[type_of(p)]   &= ~square_bb(s);
    byColor[color_of(p)] &= ~square_bb(s);
    board[s] = NO_PIECE;
}

void Position::set(const std::string& fenStr) {
    clear();
    std::istringstream ss(fenStr);
    std::string placement, stm, castl, ep;
    ss >> placement >> stm >> castl >> ep >> halfmove >> fullmove;


    int f = 0, r = 7;
    for (char c : placement) {
        if (c == '/')                { --r; f = 0; }
        else if (c >= '1' && c <= '8') f += c - '0';
        else {
            auto idx = PieceChars.find(c);
            put_piece(Piece(idx), make_square(f, r));
            ++f;
        }
    }


    sideToMove = (stm == "w") ? WHITE : BLACK;


    for (char c : castl) {
        if (c == 'K') castling |= WHITE_OO;
        if (c == 'Q') castling |= WHITE_OOO;
        if (c == 'k') castling |= BLACK_OO;
        if (c == 'q') castling |= BLACK_OOO;
    }


    if (ep != "-")
        epSquare = make_square(ep[0] - 'a', ep[1] - '1');
}

std::string Position::fen() const {
    std::ostringstream ss;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Piece p = board[make_square(f, r)];
            if (p == NO_PIECE) { ++empty; continue; }
            if (empty) { ss << empty; empty = 0; }
            ss << PieceChars[p];
        }
        if (empty) ss << empty;
        if (r > 0) ss << '/';
    }
    ss << (sideToMove == WHITE ? " w " : " b ");

    if (!castling) ss << '-';
    if (castling & WHITE_OO)  ss << 'K';
    if (castling & WHITE_OOO) ss << 'Q';
    if (castling & BLACK_OO)  ss << 'k';
    if (castling & BLACK_OOO) ss << 'q';

    ss << ' ';
    if (epSquare == NO_SQUARE) ss << '-';
    else ss << char('a' + file_of(epSquare)) << char('1' + rank_of(epSquare));

    ss << ' ' << halfmove << ' ' << fullmove;
    return ss.str();
}

Bitboard Position::attackers_to(Square s, Bitboard occ) const {
    return (PawnAttacks[BLACK][s] & pieces(WHITE, PAWN))
         | (PawnAttacks[WHITE][s] & pieces(BLACK, PAWN))
         | (KnightAttacks[s] & pieces(KNIGHT))
         | (KingAttacks[s] & pieces(KING))
         | (rook_attacks(s, occ)   & (pieces(ROOK)   | pieces(QUEEN)))
         | (bishop_attacks(s, occ) & (pieces(BISHOP) | pieces(QUEEN)));
}

void Position::do_move(Move m, StateInfo& st) {
    Square from = from_sq(m), to = to_sq(m);
    int fl = flags(m);
    Color us = side_to_move, them = ~us;
    Piece pc = board[from];
    
    st.castling = castling;
    st.epSquare = epSquare;
    st.halfmove = halfmove;
    st.captured = NO_PIECE;

    if(f1 == EP_CAPTURE) {
        Square capSq = Square(to + (us == WHITE ? -8 : 8));
        st.captured = board[capSq];
        remove_piece(capSq);
        halfmove = 0;
    }
    else if(f1 & CAPTURE) {
        st.capture = board[to];
        remove_piece(to);
        halfmove = 0;
    }

    remove_piece(from);
    if(f1 >= PROMO_N) {
        put_piece(make_piece(us, PieceType(KNIGHT + (f1 & 3))), to);
    }
    else
        put_piece(pc, to);

    if(f1 == DOUBLE_PUSH) {
        epSquare = Square(from + (us == WHITE ? 8:-8));
    }

    if(f1 == KING_CASTLE) {
        Square rf = Square(to +1), rt = Square(to -1);
        Piece rook  = board[rf];
        remove_piece(rf); put_piece(rook, rt);
    }
    else if(f1 == QUEEN_CASTLE) {
        Square rf = Square(to -2), rt = Square(to +1)
        Piece rook = board[rf]
        remove_piece(rf); put_piece(rook, rt)
    }
    if(type_of(pc) == PAWN) halfmove =0;
        castling &= CastleMaskp[from] & CastleMask[to];
    if(us == BLACK) ++fullmove {
        side_to_move = them;
    }
}

void Position::undo_move(Move m, const StateInfo st) {
    side_to_move = ~sideToMove;
    Color us = side_to_move;
    Square from = from_sq(m), to = to_sq(m);
    int f1 = flags(m);

    if(us == BLACK) --fullmove;
    castling = st.castling;
    epSquare = st.epSquare;
    halfmove = st.halfmove;

    if(f1 == KING_CASTLE) {
        Square rf = Square(to +1), rt = Square(to -1);
        Piece rook = board[rt];
        remove_piece(rt); put_piece(rook, rf);
    }
    else if(f1 == QUEEN_CASTLE) {
        Square rf = Square(to -2), rt = Square(to +1);
        Piece rook = board[rt];
        remove_piece(rt); put_piece(rook, rf);
    }

    Piece moved = board[to];
    remove_piece(to);
    put_piece(f1 >= PROMO_N ? make_piece(us, pawn) : moved, from);

    if(st.captured != NO_PIECE){
        Square capSq = (f1 == EP_CAPTURE) ? Square(to + (us == WHITE ? -8 : 8)) : to;
        put_piece(st.captured, capSq);
    }
}