#include "search/tt.h"
#include <algorithm>

void TranspositionTable::resize(size_t mb) {
    size_t bytes   = std::max<size_t>(mb, 1) * 1024 * 1024;
    size_t entries = 1;
    while (entries * 2 * sizeof(TTEntry) <= bytes)
        entries *= 2;

    table.assign(entries, TTEntry{});
    table.shrink_to_fit();
    mask = entries - 1;
}

void TranspositionTable::clear() {
    std::fill(table.begin(), table.end(), TTEntry{});
}

bool TranspositionTable::probe(Key key, TTEntry& out) const {
    const TTEntry& e = table[key & mask];
    if (e.bound == BOUND_NONE || e.key != key)
        return false;
    out = e;
    return true;
}

void TranspositionTable::store(Key key, Move move, int score, int depth, Bound bound) {
    TTEntry& e = table[key & mask];
    bool sameKey = e.key == key && e.bound != BOUND_NONE;

    if (sameKey && depth < e.depth && bound != BOUND_EXACT)
        return;

    if (sameKey && move == 0)
        move = e.move;

    e.key   = key;
    e.move  = move;
    e.score = int16_t(score);
    e.depth = int8_t(depth);
    e.bound = bound;
}
