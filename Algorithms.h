#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "Graph.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

// Estructura completa de metricas de instrumentacion (RF07, RF13)
struct TSPResult {
    string algoritmo;
    double distancia_km = 0.0;
    double costo_usd = 0.0;
    double desviacion_pct = 0.0;
    double tiempo_us = 0.0;
    double tiempo_ms = 0.0;
    double memoria_kb = 0.0;
    uint64_t operaciones_o_estados = 0;
    vector<int> ruta;
    bool ejecutado = true;
    string motivo_omision;
};

// Resultado enriquecido para el Recomendador Adaptativo (RF11)
struct RecomendacionAdaptativa {
    string algoritmoElegido;
    string categoriaEscala;
    bool cumpleDesigualdadTriangular = false;
    string justificacion;
    TSPResult resultado;
};

// Medidor de tiempo de alta resolucion y memoria del proceso (RF07, RF13)
class Profiler {
private:
    chrono::high_resolution_clock::time_point startTime;

public:
    void start() {
        startTime = chrono::high_resolution_clock::now();
    }

    double elapsedMicroseconds() const {
        auto endTime = chrono::high_resolution_clock::now();
        return chrono::duration<double, micro>(endTime - startTime).count();
    }

    double elapsedMilliseconds() const {
        return elapsedMicroseconds() / 1000.0;
    }

    // Consulta la memoria privada del proceso actual en Windows/MSYS2 (KB)
    static double obtenerMemoriaProcesoKB();
};

class Algorithms {
private:
    static void backtrackRec(const Graph& graph,
                             int actual,
                             int contador,
                             double costoActual,
                             double sumaMinRestante,
                             const vector<double>& minSalida,
                             vector<bool>& visitado,
                             vector<int>& rutaActual,
                             double& mejorCosto,
                             vector<int>& mejorRuta,
                             uint64_t& estadosExplorados);

    static void dfsPreorden(int u,
                            const vector<vector<int>>& mstAdj,
                            vector<bool>& visitado,
                            vector<int>& recorrido,
                            uint64_t& operaciones);

public:
    static constexpr int kMaxSafeBacktracking = 12;
    static constexpr int kMaxSafeHeldKarp = 20;

    static TSPResult Backtracking(const Graph& graph, int maxSafeN = kMaxSafeBacktracking);
    static TSPResult HeldKarp(const Graph& graph, int maxSafeN = kMaxSafeHeldKarp);
    static TSPResult MST_DFS(const Graph& graph);
    static TSPResult NearestNeighbor(const Graph& graph, int startNode = 0);

    // Seleccion adaptativa automatica segun N y propiedades metricas (RF11)
    static RecomendacionAdaptativa SeleccionAdaptativa(const Graph& graph,
                                                       bool exigirGarantiaTeorica = false);
};

#endif
