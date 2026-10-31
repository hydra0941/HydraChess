#pragma once
#include "position/position.h"

struct MoveList {
    Move moves[256];
    int  count = 0;

    void add(Move m)             { moves[count++] = m; }
    Move*       begin()          { return moves; }
    Move*       end()            { return moves + count; }
    const Move* begin() const    { return moves; }
    const Move* end()   const    { return moves + count; }
};

void generate(const Position& pos, MoveList& list);