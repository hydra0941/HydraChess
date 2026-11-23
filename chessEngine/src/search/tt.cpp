#include "search/tt.h"
#include <algorithm>

namespace {

uint64_t pack(Move move, int score, int depth, Bound bound) {
    return uint64_t(move)
         | uint64_t(uint16_t(int16_t(score))) << 16
         | uint64_t(uint8_t(int8_t(depth)))   << 32
         | uint64_t(bound)                    << 40;
}

TTEntry unpack(Key key, uint64_t data) {
    TTEntry e;
    e.key   = key;
    e.move  = Move(data & 0xFFFF);
    e.score = int16_t(uint16_t(data >> 16));
    e.depth = int8_t(uint8_t(data >> 32));
    e.bound = Bound((data >> 40) & 0xFF);
    return e;
}

}

void TranspositionTable::resize(size_t mb) {
    size_t bytes   = std::max<size_t>(mb, 1) * 1024 * 1024;
    size_t entries = 1;
    while (entries * 2 * sizeof(Slot) <= bytes)
        entries *= 2;

    table = std::make_unique<Slot[]>(entries);
    mask  = entries - 1;
}

void TranspositionTable::clear() {
    for (size_t i = 0; i <= mask; ++i) {
        table[i].check.store(0, std::memory_order_relaxed);
        table[i].data.store(0, std::memory_order_relaxed);
    }
}

bool TranspositionTable::probe(Key key, TTEntry& out) const {
    const Slot& s  = table[key & mask];
    uint64_t data  = s.data.load(std::memory_order_relaxed);
    uint64_t check = s.check.load(std::memory_order_relaxed);
    if ((check ^ data) != key)
        return false;

    out = unpack(key, data);
    return out.bound != BOUND_NONE;
}

void TranspositionTable::store(Key key, Move move, int score, int depth, Bound bound) {
    Slot& s = table[key & mask];

    TTEntry old;
    if (probe(key, old)) {
        if (depth < old.depth && bound != BOUND_EXACT)
            return;
        if (move == 0)
            move = old.move;
    }

    uint64_t data = pack(move, score, depth, bound);
    s.data.store(data, std::memory_order_relaxed);
    s.check.store(key ^ data, std::memory_order_relaxed);
}
