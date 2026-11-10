#include "Algorithms.h"

#include <limits>
#include <stdexcept>

using namespace std;

void Algorithms::backtrackRec(const Graph& graph,
                              int actual,
                              int contador,
                              double costoActual,
                              vector<bool>& visitado,
                              vector<int>& rutaActual,
                              double& mejorCosto,
                              vector<int>& mejorRuta) {
    int n = graph.getNumNodes();
    const auto& mat = graph.getMatrix();

    if (contador == n) {
        double costoTotal = costoActual + mat[actual][0];
        if (costoTotal < mejorCosto) {
            mejorCosto = costoTotal;
            mejorRuta = rutaActual;
            mejorRuta.push_back(0);
        }
        return;
    }

    for (int sig = 1; sig < n; sig++) {
        if (!visitado[sig]) {
            double nuevoCosto = costoActual + mat[actual][sig];
            if (nuevoCosto < mejorCosto) {
                visitado[sig] = true;
                rutaActual.push_back(sig);

                backtrackRec(graph, sig, contador + 1, nuevoCosto,
                             visitado, rutaActual, mejorCosto, mejorRuta);

                rutaActual.pop_back();
                visitado[sig] = false;
            }
        }
    }
}

TSPResult Algorithms::Backtracking(const Graph& graph) {
    int n = graph.getNumNodes();
    TSPResult res{"Backtracking", 0.0, {}, 0.0, 0.0};

    if (n <= 0) return res;
    if (n == 1) {
        res.path = {0, 0};
        return res;
    }

    vector<bool> visitado(n, false);
    vector<int> rutaActual;
    visitado[0] = true;
    rutaActual.push_back(0);

    double mejorCosto = numeric_limits<double>::infinity();
    vector<int> mejorRuta;

    backtrackRec(graph, 0, 1, 0.0, visitado, rutaActual, mejorCosto, mejorRuta);

    res.minCost = mejorCosto;
    res.path = mejorRuta;
    return res;
}

TSPResult Algorithms::HeldKarp(const Graph& graph) {
    int n = graph.getNumNodes();
    TSPResult res{"Held-Karp (DP)", 0.0, {}, 0.0, 0.0};

    if (n <= 0) return res;
    if (n == 1) {
        res.path = {0, 0};
        return res;
    }
    if (n > 20) {
        throw invalid_argument("Held-Karp soporta hasta N = 20.");
    }

    int numClientes = n - 1;
    int totalMasks = 1 << numClientes;
    double INF = numeric_limits<double>::infinity();
    const auto& mat = graph.getMatrix();

    vector<vector<double>> dp(totalMasks, vector<double>(n, INF));
    vector<vector<int>> padre(totalMasks, vector<int>(n, -1));

    for (int v = 1; v < n; v++) {
        int mask = 1 << (v - 1);
        dp[mask][v] = mat[0][v];
        padre[mask][v] = 0;
    }

    for (int mask = 1; mask < totalMasks; mask++) {
        for (int u = 1; u < n; u++) {
            if ((mask & (1 << (u - 1))) == 0) continue;
            if (dp[mask][u] == INF) continue;

            for (int v = 1; v < n; v++) {
                int bitV = 1 << (v - 1);
                if ((mask & bitV) != 0) continue;

                int sigMask = mask | bitV;
                double nuevoCosto = dp[mask][u] + mat[u][v];
                if (nuevoCosto < dp[sigMask][v]) {
                    dp[sigMask][v] = nuevoCosto;
                    padre[sigMask][v] = u;
                }
            }
        }
    }

    int maskCompleta = totalMasks - 1;
    double mejorCosto = INF;
    int ultimoNodo = -1;

    for (int u = 1; u < n; u++) {
        double candidato = dp[maskCompleta][u] + mat[u][0];
        if (candidato < mejorCosto) {
            mejorCosto = candidato;
            ultimoNodo = u;
        }
    }

    vector<int> invertido;
    int maskActual = maskCompleta;
    int nodoActual = ultimoNodo;

    while (nodoActual != 0 && nodoActual != -1) {
        invertido.push_back(nodoActual);
        int ant = padre[maskActual][nodoActual];
        maskActual ^= (1 << (nodoActual - 1));
        nodoActual = ant;
    }

    res.path.push_back(0);
    for (int i = (int)invertido.size() - 1; i >= 0; i--) {
        res.path.push_back(invertido[i]);
    }
    res.path.push_back(0);
    res.minCost = mejorCosto;

    return res;
}

void Algorithms::dfsPreorden(int u,
                             const vector<vector<int>>& mstAdj,
                             vector<bool>& visitado,
                             vector<int>& recorrido) {
    visitado[u] = true;
    recorrido.push_back(u);
    for (int v : mstAdj[u]) {
        if (!visitado[v]) {
            dfsPreorden(v, mstAdj, visitado, recorrido);
        }
    }
}

TSPResult Algorithms::MST_DFS(const Graph& graph) {
    int n = graph.getNumNodes();
    TSPResult res{"MST + DFS", 0.0, {}, 0.0, 0.0};

    if (n <= 0) return res;
    if (n == 1) {
        res.path = {0, 0};
        return res;
    }

    double INF = numeric_limits<double>::infinity();
    const auto& mat = graph.getMatrix();

    vector<double> key(n, INF);
    vector<int> padre(n, -1);
    vector<bool> enMST(n, false);

    key[0] = 0.0;

    for (int count = 0; count < n; count++) {
        int u = -1;
        double minKey = INF;
        for (int v = 0; v < n; v++) {
            if (!enMST[v] && key[v] < minKey) {
                minKey = key[v];
                u = v;
            }
        }

        if (u == -1) break;
        enMST[u] = true;

        for (int v = 0; v < n; v++) {
            if (!enMST[v] && mat[u][v] < key[v]) {
                key[v] = mat[u][v];
                padre[v] = u;
            }
        }
    }

    vector<vector<int>> mstAdj(n);
    for (int v = 1; v < n; v++) {
        if (padre[v] != -1) {
            mstAdj[padre[v]].push_back(v);
            mstAdj[v].push_back(padre[v]);
        }
    }

    vector<bool> visitado(n, false);
    dfsPreorden(0, mstAdj, visitado, res.path);
    res.path.push_back(0);

    double costoTotal = 0.0;
    for (size_t i = 0; i + 1 < res.path.size(); i++) {
        costoTotal += mat[res.path[i]][res.path[i + 1]];
    }
    res.minCost = costoTotal;

    return res;
}

TSPResult Algorithms::NearestNeighbor(const Graph& graph, int startNode) {
    int n = graph.getNumNodes();
    TSPResult res{"Nearest Neighbor", 0.0, {}, 0.0, 0.0};

    if (n <= 0) return res;
    if (startNode < 0 || startNode >= n) {
        throw out_of_range("Nodo inicial fuera de rango.");
    }
    if (n == 1) {
        res.path = {startNode, startNode};
        return res;
    }

    double INF = numeric_limits<double>::infinity();
    const auto& mat = graph.getMatrix();

    vector<bool> visitado(n, false);
    int actual = startNode;
    visitado[actual] = true;
    res.path.push_back(actual);

    double costoTotal = 0.0;

    for (int paso = 1; paso < n; paso++) {
        int masCercano = -1;
        double minDist = INF;

        for (int cand = 0; cand < n; cand++) {
            if (!visitado[cand] && mat[actual][cand] < minDist) {
                minDist = mat[actual][cand];
                masCercano = cand;
            }
        }

        visitado[masCercano] = true;
        res.path.push_back(masCercano);
        costoTotal += minDist;
        actual = masCercano;
    }

    costoTotal += mat[actual][startNode];
    res.path.push_back(startNode);
    res.minCost = costoTotal;

    return res;
}
