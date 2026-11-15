// Benchmark for the real team implementations; does not modify Graph.h.
#include "modules/Graph.h"
#include "modules/StationIndex.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

using Clock = std::chrono::steady_clock;
static double ms(Clock::time_point a, Clock::time_point b) {
    return std::chrono::duration<double, std::milli>(b - a).count();
}
static std::string idOf(std::size_t i) {
    std::ostringstream os;
    os << std::setw(5) << std::setfill('0') << (i + 1);
    return os.str();
}
static std::uint32_t nextRand(std::uint32_t& x) {
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return x;
}
static std::size_t workingSetBytes() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc)))
        return static_cast<std::size_t>(pmc.WorkingSetSize);
#endif
    return 0; // unsupported OS; report NA, never fake memory figures
}
static std::size_t parseNumber(const char* s) {
    char* end = nullptr;
    unsigned long long v = std::strtoull(s, &end, 10);
    if (s == end || *end != '\0' || v == 0) throw std::invalid_argument("positive integer expected");
    return static_cast<std::size_t>(v);
}
int main(int argc, char** argv) {
    try {
        // Usage: benchmark_graph.exe [tripCount] [stationCount] [destinationsPerStation] [queryCount]
        const std::size_t n = argc > 1 ? parseNumber(argv[1]) : 10000;
        const std::size_t stationsN = argc > 2 ? parseNumber(argv[2]) : 2789;
        const std::size_t fanout = argc > 3 ? parseNumber(argv[3]) : 8;
        const std::size_t queries = argc > 4 ? parseNumber(argv[4]) : 100000;
        if (argc > 5 || stationsN > 99998 || fanout > stationsN || n > 100000000)
            throw std::invalid_argument("invalid arguments: 1<=fanout<=stationCount<=99998; trips<=100000000");
        std::uint32_t seed = 0x12345678u; // reproducible data
        DynamicArray<Station> sorted;
        sorted.reserve(stationsN);
        for (std::size_t i = 0; i < stationsN; ++i) {
            Station s; s.id = idOf(i); sorted.pushBack(s);
        }
        StationTree tree;
        auto t0 = Clock::now();
        const bool okTree = buildStationTree(sorted, tree);
        auto t1 = Clock::now();
        if (!okTree || tree.size() != stationsN) throw std::runtime_error("BST build failed");
        const double bstMs = ms(t0,t1);
        const std::size_t memAfterTree = workingSetBytes();

        LinkedList<Trip> trips;
        auto t2 = Clock::now();
        for (std::size_t i = 0; i < n; ++i) {
            // Balanced station usage, bounded number of distinct neighbors.
            const std::size_t from = static_cast<std::size_t>(nextRand(seed)) % stationsN;
            const std::size_t offset = static_cast<std::size_t>(nextRand(seed)) % fanout;
            Trip trip;
            trip.rentStationId = idOf(from);
            trip.returnStationId = idOf((from + offset) % stationsN);
            trip.useMinutes = 1 + static_cast<int>(nextRand(seed) % 60);
            trips.pushBack(trip);
        }
        auto t3 = Clock::now();
        const double generationMs = ms(t2,t3);
        const std::size_t memBeforeGraph = workingSetBytes();
        Graph graph(tree);
        Graph::BuildStats stats;
        auto t4 = Clock::now();
        const auto result = graph.buildGraph(trips, stats);
        auto t5 = Clock::now();
        const double graphMs = ms(t4,t5);
        const std::size_t memAfterGraph = workingSetBytes();
        if (result != Graph::BuildSuccess || stats.added + stats.updated != static_cast<long long>(n) ||
            stats.invalidTrip != 0 || stats.missingStation != 0)
            throw std::runtime_error("Graph correctness check failed");

        std::size_t edges = 0;
        long long summedTrips = 0;
        tree.inorder([&](const std::string&, const Station& station) {
            for (auto* p = station.edges.head(); p != nullptr; p = p->next) {
                ++edges; summedTrips += p->data.tripCount;
            }
        });
        if (summedTrips != static_cast<long long>(n) || edges != static_cast<std::size_t>(stats.added))
            throw std::runtime_error("Edge totals check failed");

        // Reconstruct guaranteed-hit queries from actual edges, avoiding invalid assumptions.
        std::vector<std::pair<std::string,std::string>> hitPairs;
        hitPairs.reserve(edges);
        tree.inorder([&](const std::string& from, const Station& station) {
            for (auto* p = station.edges.head(); p != nullptr; p = p->next)
                hitPairs.emplace_back(from, p->data.toStationId);
        });
        volatile std::uint64_t checksum = 0;
        auto q0 = Clock::now();
        for (std::size_t i = 0; i < queries; ++i) {
            const auto& pair = hitPairs[i % hitPairs.size()];
            const Edge* edge = graph.getEdge(pair.first, pair.second);
            if (!edge) throw std::runtime_error("hit query failed");
            checksum += static_cast<std::uint64_t>(edge->tripCount);
        }
        auto q1 = Clock::now();
        // All misses use a non-existent destination, but a valid source.
        for (std::size_t i = 0; i < queries; ++i) {
            const auto& pair = hitPairs[i % hitPairs.size()];
            if (graph.getEdge(pair.first, "99999") != nullptr)
                throw std::runtime_error("miss query failed");
            checksum += 1;
        }
        auto q2 = Clock::now();
        // Neighbor traversal: each callback counts one existing edge.
        std::uint64_t callbacks = 0;
        auto q3 = Clock::now();
        for (std::size_t i = 0; i < queries; ++i) {
            graph.forEachNeighbor(idOf(i % stationsN), [&](const std::string& to) {
                callbacks += !to.empty();
            });
        }
        auto q4 = Clock::now();
        checksum += callbacks;
        std::cout << "trips,stations,fanout,queries,bst_ms,trip_generation_ms,graph_build_ms,edge_count,added,updated,hit_query_ms,miss_query_ms,neighbors_ms,working_set_after_bst_bytes,working_set_before_graph_bytes,working_set_after_graph_bytes,checksum\n";
        std::cout << n << ',' << stationsN << ',' << fanout << ',' << queries << ','
                  << std::fixed << std::setprecision(3)
                  << bstMs << ',' << generationMs << ',' << graphMs << ','
                  << edges << ',' << stats.added << ',' << stats.updated << ','
                  << ms(q0,q1) << ',' << ms(q1,q2) << ',' << ms(q3,q4) << ','
                  << memAfterTree << ',' << memBeforeGraph << ',' << memAfterGraph << ','
                  << checksum << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "BENCHMARK FAILED: " << e.what() << '\n'; return 1;
    }
}
