#include "uci/uci.h"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include "movegen/movegen.h"
#include "position/position.h"
#include "search/search.h"

namespace {

const std::string START_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

constexpr int64_t MOVE_OVERHEAD_MS = 30;

void send(const std::string& line) {
    static std::mutex outputMutex;
    std::lock_guard<std::mutex> lock(outputMutex);
    std::cout << line << std::endl;
}

Move parse_move(Position& pos, const std::string& text) {
    MoveList list;
    generate_legal(pos, list);
    for (Move m : list)
        if (move_to_uci(m) == text)
            return m;
    return 0;
}

void set_position(Position& pos, std::istringstream& is) {
    std::string token, fen;
    is >> token;

    if (token == "startpos") {
        fen = START_FEN;
        is >> token;
    } else if (token == "fen") {
        while (is >> token && token != "moves")
            fen += token + " ";
    } else {
        return;
    }

    pos.set(fen);

    while (is >> token) {
        Move m = parse_move(pos, token);
        if (m == 0)
            break;
        StateInfo st;
        pos.do_move(m, st);
    }
}

std::string format_score(int score) {
    if (score >= MATE_BOUND)
        return "mate " + std::to_string((MATE - score + 1) / 2);
    if (score <= -MATE_BOUND)
        return "mate " + std::to_string(-((MATE + score) / 2));
    return "cp " + std::to_string(score);
}

void print_info(const SearchResult& r) {
    uint64_t nps = r.timeMs > 0 ? r.nodes * 1000 / r.timeMs : 0;
    std::string line = "info depth " + std::to_string(r.depth)
                     + " score " + format_score(r.score)
                     + " nodes " + std::to_string(r.nodes)
                     + " nps " + std::to_string(nps)
                     + " time " + std::to_string(r.timeMs)
                     + " pv";
    for (Move m : r.pv)
        line += " " + move_to_uci(m);
    send(line);
}

SearchLimits parse_go(const Position& pos, std::istringstream& is) {
    int64_t wtime = -1, btime = -1, winc = 0, binc = 0, movetime = 0;
    int movestogo = 0, depth = 0;
    bool infinite = false;

    std::string token;
    while (is >> token) {
        if      (token == "wtime")     is >> wtime;
        else if (token == "btime")     is >> btime;
        else if (token == "winc")      is >> winc;
        else if (token == "binc")      is >> binc;
        else if (token == "movestogo") is >> movestogo;
        else if (token == "movetime")  is >> movetime;
        else if (token == "depth")     is >> depth;
        else if (token == "infinite")  infinite = true;
    }

    SearchLimits limits;
    if (depth > 0)
        limits.depth = depth;
    if (infinite)
        return limits;

    if (movetime > 0) {
        limits.hardMs = std::max<int64_t>(1, movetime - MOVE_OVERHEAD_MS);
        limits.softMs = std::max<int64_t>(1, limits.hardMs / 2);
        return limits;
    }

    bool white   = pos.side_to_move() == WHITE;
    int64_t time = white ? wtime : btime;
    int64_t inc  = white ? winc : binc;
    if (time < 0)
        return limits;

    int64_t available = std::max<int64_t>(1, time - MOVE_OVERHEAD_MS);
    int64_t soft      = available / (movestogo > 0 ? movestogo : 30) + inc / 2;
    int64_t hard      = std::min(soft * 3, available / 2);
    limits.hardMs     = std::max<int64_t>(1, hard);
    limits.softMs     = std::max<int64_t>(1, std::min(soft, hard));
    return limits;
}

}

void uci_loop() {
    Position pos;
    pos.set(START_FEN);

    std::atomic<bool> stop{false};
    TranspositionTable tt(16);
    Worker worker(stop, tt);
    std::thread searchThread;

    auto stop_search = [&] {
        stop = true;
        if (searchThread.joinable())
            searchThread.join();
    };

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream is(line);
        std::string cmd;
        is >> cmd;

        if (cmd == "uci") {
            send("id name HydraChess");
            send("id author hydra0941");
            send("option name Hash type spin default 16 min 1 max 1024");
            send("uciok");
        } else if (cmd == "isready") {
            send("readyok");
        } else if (cmd == "ucinewgame") {
            stop_search();
            tt.clear();
            pos.set(START_FEN);
        } else if (cmd == "setoption") {
            stop_search();
            std::string token, name;
            int64_t value = 0;
            is >> token >> name >> token >> value;
            if (name == "Hash" && value > 0)
                tt.resize(size_t(std::min<int64_t>(value, 1024)));
        } else if (cmd == "position") {
            stop_search();
            set_position(pos, is);
        } else if (cmd == "go") {
            stop_search();
            SearchLimits limits = parse_go(pos, is);
            stop = false;
            searchThread = std::thread([&worker, root = pos, limits] {
                SearchResult r = worker.search(root, limits, print_info);
                send("bestmove " + move_to_uci(r.bestMove));
            });
        } else if (cmd == "stop") {
            stop_search();
        } else if (cmd == "quit") {
            break;
        }
    }

    stop_search();
}
