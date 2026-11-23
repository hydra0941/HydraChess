#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "bitboard/types.h"
#include "movegen/movegen.h"
#include "position/position.h"
#include "search/tt.h"

constexpr int INF        = 32001;
constexpr int MATE       = 32000;
constexpr int MATE_BOUND = MATE - 1000;
constexpr int MAX_PLY    = 128;

struct SearchLimits {
    int     depth  = MAX_PLY - 1;
    int64_t softMs = 0;
    int64_t hardMs = 0;
};

struct SearchResult {
    Move              bestMove = 0;
    int               score    = 0;
    int               depth    = 0;
    uint64_t          nodes    = 0;
    int64_t           timeMs   = 0;
    std::vector<Move> pv;
};

using IterationCallback = std::function<void(const SearchResult&)>;

class Worker {
    public:

        Worker(int id, std::atomic<bool>& stopSignal, std::atomic<uint64_t>& totalNodes,
               TranspositionTable& tt)
            : id(id), stopSignal(stopSignal), totalNodes(totalNodes), tt(tt) {}

        SearchResult search(const Position& root, const SearchLimits& limits,
                            const IterationCallback& onIteration = {});

    private:
        using Clock = std::chrono::steady_clock;

        int64_t  elapsed_ms() const;
        void     flush_nodes();
        uint64_t total_nodes() const;
        void    check_stop();
        void    order_moves(MoveList& list, int ply, Move ttMove) const;
        void    order_captures(MoveList& list) const;
        void    update_quiet(Move m, int depth, int ply);
        int     quiescence(int alpha, int beta, int ply);
        int     alpha_beta(int depth, int alpha, int beta, int ply, bool nullAllowed = true);

        int                    id;
        std::atomic<bool>&     stopSignal;
        std::atomic<uint64_t>& totalNodes;
        TranspositionTable&    tt;
        Position           pos;
        SearchLimits       limits;
        Clock::time_point  startTime;
        uint64_t           nodes   = 0;
        uint64_t           flushed = 0;   
        bool               stopped = false;
        bool               canStop = false;

        Move killers[MAX_PLY][2]                    = {};
        int  history[COLOR_NB][SQUARE_NB][SQUARE_NB] = {};
        Move pvTable[MAX_PLY][MAX_PLY]               = {};
        int  pvLength[MAX_PLY]                       = {};
};


class ThreadPool {
    public:
        ThreadPool(std::atomic<bool>& stopSignal, TranspositionTable& tt);
        ~ThreadPool();

        void set_threads(int n);
        int  size() const { return int(workers.size()); }

        SearchResult search(const Position& root, const SearchLimits& limits,
                            const IterationCallback& onIteration = {});

    private:
        std::atomic<bool>&                   stopSignal;
        TranspositionTable&                  tt;
        std::atomic<uint64_t>                totalNodes{0};
        std::vector<std::unique_ptr<Worker>> workers;
};

std::string move_to_uci(Move m);
