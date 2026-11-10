#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "Graph.h"

#include <chrono>
#include <string>
#include <vector>

using namespace std;

struct TSPResult {
    string algorithmName;
    double minCost;
    vector<int> path;
    double timeMicroseconds;
    double timeMilliseconds;
};

class Profiler {
private:
    chrono::steady_clock::time_point startTime;

public:
    void start() {
        startTime = chrono::steady_clock::now();
    }

    double elapsedMicroseconds() const {
        auto endTime = chrono::steady_clock::now();
        return chrono::duration<double, micro>(endTime - startTime).count();
    }

    double elapsedMilliseconds() const {
        return elapsedMicroseconds() / 1000.0;
    }
};

class Algorithms {
private:
    static void backtrackRec(const Graph& graph,
                             int actual,
                             int contador,
                             double costoActual,
                             vector<bool>& visitado,
                             vector<int>& rutaActual,
                             double& mejorCosto,
                             vector<int>& mejorRuta);

    static void dfsPreorden(int u,
                            const vector<vector<int>>& mstAdj,
                            vector<bool>& visitado,
                            vector<int>& recorrido);

public:
    static TSPResult Backtracking(const Graph& graph);
    static TSPResult HeldKarp(const Graph& graph);
    static TSPResult MST_DFS(const Graph& graph);
    static TSPResult NearestNeighbor(const Graph& graph, int startNode = 0);
};

#endif
