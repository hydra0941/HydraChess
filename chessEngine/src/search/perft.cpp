#include "movegen/movegen.h"
#include "position/position.h"
#include <cstdint>

uint64_t perft(Position& pos, int depth) {
    if(depth == 0) return 1;
    MoveList list;
    generate_legal(pos, list);
    if(depth == 1) return list.count;
    uint64_t nodes = 0;
        for(Move m: list) {
            StateInfo st;
            pos.do_move(m, st);
            nodes += perft(pos, depth - 1);
            pos.undo_move(m, st);
        }
        return nodes;
}