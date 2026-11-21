#include "search/search.h"
#include <algorithm>
#include <cstring>
#include "eval/eval.h"

namespace {

bool is_capture(Move m) {
    return flags(m) & CAPTURE;
}

int mvv_lva(const Position& pos, Move m) {
    PieceType victim   = flags(m) == EP_CAPTURE ? PAWN : type_of(pos.piece_on(to_sq(m)));
    PieceType attacker = type_of(pos.piece_on(from_sq(m)));
    return 10 * victim - attacker;
}

int score_to_tt(int score, int ply) {
    if (score >= MATE_BOUND)
        return score + ply;
    if (score <= -MATE_BOUND)
        return score - ply;
    return score;
}

int score_from_tt(int score, int ply) {
    if (score >= MATE_BOUND)
        return score - ply;
    if (score <= -MATE_BOUND)
        return score + ply;
    return score;
}

void sort_by_score(MoveList& list, int* scores) {
    for (int i = 1; i < list.count; ++i) {
        Move m = list.moves[i];
        int s  = scores[i];
        int j  = i - 1;
        while (j >= 0 && scores[j] < s) {
            list.moves[j + 1] = list.moves[j];
            scores[j + 1]     = scores[j];
            --j;
        }
        list.moves[j + 1] = m;
        scores[j + 1]     = s;
    }
}

}

int64_t Worker::elapsed_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - startTime).count();
}

void Worker::check_stop() {
    if ((nodes & 2047) != 0)
        return;
    if (stopSignal.load(std::memory_order_relaxed)
        || (canStop && limits.hardMs > 0 && elapsed_ms() >= limits.hardMs))
        stopped = true;
}

void Worker::order_captures(MoveList& list) const {
    int scores[256];
    for (int i = 0; i < list.count; ++i)
        scores[i] = is_capture(list.moves[i]) ? mvv_lva(pos, list.moves[i]) : -1;
    sort_by_score(list, scores);
}

void Worker::order_moves(MoveList& list, int ply, Move ttMove) const {
    Color us = pos.side_to_move();
    int scores[256];
    for (int i = 0; i < list.count; ++i) {
        Move m = list.moves[i];
        if (m == ttMove)
            scores[i] = 1'000'000;
        else if (is_capture(m))
            scores[i] = 100'000 + mvv_lva(pos, m);
        else if (m == killers[ply][0])
            scores[i] = 90'000;
        else if (m == killers[ply][1])
            scores[i] = 80'000;
        else
            scores[i] = history[us][from_sq(m)][to_sq(m)];
    }
    sort_by_score(list, scores);
}

void Worker::update_quiet(Move m, int depth, int ply) {
    if (killers[ply][0] != m) {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = m;
    }

    int& h = history[pos.side_to_move()][from_sq(m)][to_sq(m)];
    h += depth * depth;
    if (h > 70'000)
        for (auto& c : history)
            for (auto& f : c)
                for (int& v : f)
                    v /= 2;
}

int Worker::quiescence(int alpha, int beta, int ply) {
    ++nodes;
    check_stop();
    if (stopped)
        return 0;

    int standPat = evaluate(pos);
    if (standPat >= beta)
        return beta;
    if (standPat > alpha)
        alpha = standPat;

    MoveList list;
    generate_legal(pos, list);
    order_captures(list);

    for (Move m : list) {
        if (!is_capture(m))
            break;

        StateInfo st;
        pos.do_move(m, st);
        int score = -quiescence(-beta, -alpha, ply + 1);
        pos.undo_move(m, st);

        if (stopped)
            return 0;
        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }
    return alpha;
}

int Worker::alpha_beta(int depth, int alpha, int beta, int ply) {
    pvLength[ply] = ply;

    if (ply > 0 && pos.is_draw())
        return 0;

    if (depth == 0)
        return quiescence(alpha, beta, ply);

    ++nodes;
    check_stop();
    if (stopped)
        return 0;

    if (ply >= MAX_PLY - 1)
        return evaluate(pos);

    Key     key    = pos.key();
    Move    ttMove = 0;
    TTEntry entry;
    if (tt.probe(key, entry)) {
        ttMove = entry.move;
        if (ply > 0 && entry.depth >= depth) {
            int ttScore = score_from_tt(entry.score, ply);
            if (entry.bound == BOUND_EXACT)
                return ttScore;
            if (entry.bound == BOUND_LOWER && ttScore >= beta)
                return beta;
            if (entry.bound == BOUND_UPPER && ttScore <= alpha)
                return alpha;
        }
    }

    MoveList list;
    generate_legal(pos, list);

    if (list.count == 0)
        return pos.in_check() ? -MATE + ply : 0;

    order_moves(list, ply, ttMove);

    int  alphaOrig = alpha;
    Move bestMove  = 0;

    for (Move m : list) {
        StateInfo st;
        pos.do_move(m, st);
        int score = -alpha_beta(depth - 1, -beta, -alpha, ply + 1);
        pos.undo_move(m, st);

        if (stopped)
            return 0;

        if (score >= beta) {
            if (!is_capture(m))
                update_quiet(m, depth, ply);
            tt.store(key, m, score_to_tt(beta, ply), depth, BOUND_LOWER);
            return beta;
        }

        if (score > alpha) {
            alpha    = score;
            bestMove = m;
            pvTable[ply][ply] = m;
            for (int i = ply + 1; i < pvLength[ply + 1]; ++i)
                pvTable[ply][i] = pvTable[ply + 1][i];
            pvLength[ply] = pvLength[ply + 1];
        }
    }

    tt.store(key, bestMove, score_to_tt(alpha, ply), depth,
             alpha > alphaOrig ? BOUND_EXACT : BOUND_UPPER);
    return alpha;
}

SearchResult Worker::search(const Position& root, const SearchLimits& searchLimits,
                            const IterationCallback& onIteration) {
    SearchResult result;

    pos       = root;
    limits    = searchLimits;
    startTime = Clock::now();
    nodes     = 0;
    stopped   = false;
    canStop   = false;

    std::memset(killers, 0, sizeof(killers));
    std::memset(history, 0, sizeof(history));

    int maxDepth = std::clamp(limits.depth, 1, MAX_PLY - 1);

    for (int depth = 1; depth <= maxDepth; ++depth) {
        int score = alpha_beta(depth, -INF, INF, 0);
        if (stopped)
            break;

        result.depth    = depth;
        result.score    = score;
        result.nodes    = nodes;
        result.timeMs   = elapsed_ms();
        result.pv.assign(pvTable[0], pvTable[0] + pvLength[0]);
        result.bestMove = result.pv.empty() ? 0 : result.pv[0];

        if (onIteration)
            onIteration(result);

        if (result.pv.empty())
            break;

        canStop = true;
        if (limits.softMs > 0 && elapsed_ms() >= limits.softMs)
            break;
    }

    if (result.bestMove == 0) {
        MoveList list;
        generate_legal(pos, list);
        if (list.count > 0)
            result.bestMove = list.moves[0];
    }

    result.nodes  = nodes;
    result.timeMs = elapsed_ms();
    return result;
}

std::string move_to_uci(Move m) {
    if (m == 0)
        return "0000";
    Square from = from_sq(m), to = to_sq(m);
    std::string s = {
        char('a' + file_of(from)), char('1' + rank_of(from)),
        char('a' + file_of(to)),   char('1' + rank_of(to)),
    };
    int fl = flags(m);
    if (fl >= PROMO_N)
        s += "nbrq"[fl & 3];
    return s;
}
