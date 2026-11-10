#include "Algorithms.h"
#include "Graph.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

string formatearRuta(const vector<int>& ruta) {
    ostringstream oss;
    for (size_t i = 0; i < ruta.size(); i++) {
        oss << ruta[i];
        if (i + 1 < ruta.size()) {
            oss << " -> ";
        }
    }
    return oss.str();
}

int main() {
    int numNodos = 10;
    Graph grafo = Graph::createPilotInstance(numNodos);
    Profiler profiler;
    vector<TSPResult> resultados;

    profiler.start();
    TSPResult r1 = Algorithms::Backtracking(grafo);
    r1.timeMicroseconds = profiler.elapsedMicroseconds();
    r1.timeMilliseconds = profiler.elapsedMilliseconds();
    resultados.push_back(r1);

    profiler.start();
    TSPResult r2 = Algorithms::HeldKarp(grafo);
    r2.timeMicroseconds = profiler.elapsedMicroseconds();
    r2.timeMilliseconds = profiler.elapsedMilliseconds();
    resultados.push_back(r2);

    profiler.start();
    TSPResult r3 = Algorithms::MST_DFS(grafo);
    r3.timeMicroseconds = profiler.elapsedMicroseconds();
    r3.timeMilliseconds = profiler.elapsedMilliseconds();
    resultados.push_back(r3);

    profiler.start();
    TSPResult r4 = Algorithms::NearestNeighbor(grafo, 0);
    r4.timeMicroseconds = profiler.elapsedMicroseconds();
    r4.timeMilliseconds = profiler.elapsedMilliseconds();
    resultados.push_back(r4);

    double costoOptimo = resultados[0].minCost;

    cout << "====================================================================================================\n";
    cout << "                    ROUTEOPT - PROTOTIPO ALGORITMICO (AVANCE 2) | N = " << numNodos << "\n";
    cout << "====================================================================================================\n";
    cout << left << setw(20) << "Algoritmo"
         << right << setw(12) << "Costo"
         << setw(12) << "Diferencia"
         << setw(14) << "Tiempo (us)"
         << setw(14) << "Tiempo (ms)"
         << "   " << left << "Ruta\n";
    cout << "----------------------------------------------------------------------------------------------------\n";

    cout << fixed;
    for (const auto& res : resultados) {
        double gap = ((res.minCost - costoOptimo) / costoOptimo) * 100.0;
        cout << left << setw(20) << res.algorithmName
             << right << setw(12) << setprecision(2) << res.minCost
             << setw(11) << setprecision(2) << gap << "%"
             << setw(14) << setprecision(2) << res.timeMicroseconds
             << setw(14) << setprecision(4) << res.timeMilliseconds
             << "   " << formatearRuta(res.path) << "\n";
    }
    cout << "====================================================================================================\n";

    return 0;
}
