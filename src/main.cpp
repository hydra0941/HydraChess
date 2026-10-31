
#include <iostream>
#include "bitboard/types.h"
#include "bitboard/bitboard.h"
#include "position/position.h"
#include "movegen/movegen.h"
#include <string>

int main() {
    init_attacks();
    print_bitboard(KnightAttacks[G1]);   // expect f3, h3, e2
    const std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    const std::string kiwipete = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

    Position pos;
    MoveList list;

    pos.set(startFen);
   generate(pos, list);
    std::cout << "start: " << list.count << " (expect 20)\n";

    pos.set(kiwipete);
    list = MoveList();
    generate(pos, list);
    std::cout << "kiwipete: " << list.count << " (expect 48)\n";

    print_bitboard(rook_attacks(E4, 0));     // expect full cross, 14 bits
    print_bitboard(bishop_attacks(E4, 0));   // expect both diagonals, 13 bits
}
