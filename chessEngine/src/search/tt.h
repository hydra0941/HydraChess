#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include "bitboard/types.h"

enum Bound : uint8_t { BOUND_NONE, BOUND_UPPER, BOUND_LOWER, BOUND_EXACT };

struct TTEntry {
    Key     key   = 0;
    Move    move  = 0;
    int16_t score = 0;
    int8_t  depth = 0;
    Bound   bound = BOUND_NONE;
};

class TranspositionTable {
    public:
        explicit TranspositionTable(size_t mb = 16) { resize(mb); }

        void resize(size_t mb);
        void clear();
        bool probe(Key key, TTEntry& out) const;
        void store(Key key, Move move, int score, int depth, Bound bound);

    private:
        struct Slot {
            std::atomic<uint64_t> check{0};
            std::atomic<uint64_t> data{0};
        };

        std::unique_ptr<Slot[]> table;
        size_t                  mask = 0;
};
