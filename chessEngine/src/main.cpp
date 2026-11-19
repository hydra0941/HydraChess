#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <string>
#include "bitboard/types.h"
#include "bitboard/bitboard.h"
#include "position/position.h"
#include "movegen/movegen.h"

static int failures = 0;

static std::string uci(Move m) {
    std::string s;
    s += char('a' + file_of(from_sq(m))); s += char('1' + rank_of(from_sq(m)));
    s += char('a' + file_of(to_sq(m)));   s += char('1' + rank_of(to_sq(m)));
    return s;
}

static bool has(const MoveList& list, const std::string& move) {
    for (Move m : list) if (uci(m) == move) return true;
    return false;
}

static void check(bool ok, const std::string& what) {
    std::cout << (ok ? "  PASS  " : "  FAIL  ") << what << '\n';
    if (!ok) ++failures;
}

static uint64_t test_perft(Position& pos, int depth) {
    MoveList list;
    generate_legal(pos, list);
    if (depth == 1) return list.count;
    uint64_t nodes = 0;
    for (Move m : list) {
        StateInfo st;
        pos.do_move(m, st);
        nodes += test_perft(pos, depth - 1);
        pos.undo_move(m, st);
    }
    return nodes;
}

static void pseudo_case(const std::string& name, const std::string& fen, int expected) {
    Position pos;
    pos.set(fen);
    MoveList list;
    generate(pos, list);
    check(list.count == expected, name + " pseudo-legal moves = " + std::to_string(list.count) +
                                  " (expect " + std::to_string(expected) + ")");
}

static void position_case(const std::string& name, const std::string& fen, int expected, bool inCheck,
                          std::initializer_list<const char*> mustHave = {},
                          std::initializer_list<const char*> mustNotHave = {}) {
    std::cout << name << '\n';
    Position pos;
    pos.set(fen);
    MoveList list;
    generate_legal(pos, list);

    check(list.count == expected, "legal moves = " + std::to_string(list.count) + " (expect " + std::to_string(expected) + ")");
    check(pos.in_check() == inCheck, std::string("in_check = ") + (pos.in_check() ? "true" : "false"));
    check(pos.fen() == fen, "position unchanged after generate_legal");
    for (const char* m : mustHave)    check(has(list, m),  std::string("has ") + m);
    for (const char* m : mustNotHave) check(!has(list, m), std::string("rejects ") + m);
}

static void perft_case(const std::string& name, const std::string& fen, int depth, uint64_t expected) {
    Position pos;
    pos.set(fen);
    uint64_t got = test_perft(pos, depth);
    check(got == expected, name + " perft(" + std::to_string(depth) + ") = " + std::to_string(got) +
                           " (expect " + std::to_string(expected) + ")");
}

int main() {
    init_attacks();

    const std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    const std::string kiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

    std::cout << "=== Pseudo-legal generation and attack tables ===\n";
    pseudo_case("startpos", startFen, 20);
    pseudo_case("kiwipete", kiwipete, 48);
    check(popcount(KnightAttacks[G1]) == 3, "knight on g1 attacks 3 squares");
    check(popcount(rook_attacks(E4, 0)) == 14, "rook on empty e4 attacks 14 squares");
    check(popcount(bishop_attacks(E4, 0)) == 13, "bishop on empty e4 attacks 13 squares");

    std::cout << "\n=== Targeted legality cases ===\n";
    position_case("Pinned pawn cannot push (rook pins along rank)",
        "8/8/8/KP5r/8/8/8/7k w - - 0 1", 4, false, {"a5a4", "a5b6"}, {"b5b6"});

    position_case("En passant that exposes the king is illegal",
        "8/8/8/K2pP2r/8/8/8/7k w - d6 0 1", 6, false, {"e5e6"}, {"e5d6"});

    position_case("Pinned knight has no moves",
        "4r1k1/8/8/8/8/8/4N3/4K3 w - - 0 1", 4, false, {"e1d1", "e1f2"}, {"e2c3", "e2g3", "e2d4"});

    position_case("King cannot step back along the checking ray",
        "7k/8/8/8/8/8/8/r3K3 w - - 0 1", 3, true, {"e1e2", "e1d2", "e1f2"}, {"e1d1", "e1f1"});

    position_case("Pinned knight cannot block a check",
        "4k3/8/8/8/1b6/8/3N4/r3K3 w - - 0 1", 2, true, {"e1e2", "e1f2"}, {"d2b1"});

    position_case("Cannot castle out of check",
        "4k3/8/8/8/8/8/8/R3K2r w Q - 0 1", 3, true, {}, {"e1c1"});

    position_case("Both castles allowed when legal",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 26, false, {"e1g1", "e1c1"});

    position_case("Checkmate (fool's mate): 0 moves, in check",
        "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3", 0, true);

    position_case("Stalemate: 0 moves, not in check",
        "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", 0, false);

    std::cout << "\n=== Perft (chessprogramming.org reference counts) ===\n";
    perft_case("startpos", startFen, 5, 4865609);
    perft_case("kiwipete", kiwipete, 4, 4085603);
    perft_case("pos3", "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624);
    perft_case("pos4", "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333);
    perft_case("pos5", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379);
    perft_case("pos6", "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890);

    std::cout << '\n' << (failures ? std::to_string(failures) + " FAILURE(S)" : std::string("ALL TESTS PASSED")) << '\n';
    return failures ? 1 : 0;
}
