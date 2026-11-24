#include "Algorithms.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#endif

using namespace std;

double Profiler::obtenerMemoriaProcesoKB() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))) {
        return static_cast<double>(pmc.PrivateUsage) / 1024.0;
    }
#endif
    return 0.0;
}

void Algorithms::backtrackRec(const Graph& graph,
                              int actual,
                              int contador,
                              double costoActual,
                              double sumaMinRestante,
                              const vector<double>& minSalida,
                              vector<bool>& visitado,
                              vector<int>& rutaActual,
                              double& mejorCosto,
                              vector<int>& mejorRuta,
                              uint64_t& estadosExplorados) {
    estadosExplorados++;
    int n = graph.getNumNodes();
    const auto& mat = graph.getMatrix();

    if (contador == n) {
        double costoTotal = costoActual + mat[actual][0];
        if (costoTotal < mejorCosto - 1e-9) {
            mejorCosto = costoTotal;
            mejorRuta = rutaActual;
            mejorRuta.push_back(0);
        }
        return;
    }

    // Cota inferior admisible: costo acumulado + salida minima desde actual + salidas minimas restantes
    double cotaInferior = costoActual + minSalida[actual] + sumaMinRestante;
    if (cotaInferior >= mejorCosto - 1e-9) {
        return;
    }

    for (int sig = 1; sig < n; sig++) {
        if (!visitado[sig]) {
            double nuevoCosto = costoActual + mat[actual][sig];
            double nuevaSumaRestante = sumaMinRestante - minSalida[sig];
            if (nuevoCosto + minSalida[sig] + nuevaSumaRestante < mejorCosto - 1e-9) {
                visitado[sig] = true;
                rutaActual.push_back(sig);

                backtrackRec(graph, sig, contador + 1, nuevoCosto, nuevaSumaRestante,
                             minSalida, visitado, rutaActual, mejorCosto, mejorRuta,
                             estadosExplorados);

                rutaActual.pop_back();
                visitado[sig] = false;
            }
        }
    }
}

TSPResult Algorithms::Backtracking(const Graph& graph, int maxSafeN) {
    int n = graph.getNumNodes();
    TSPResult res;
    res.algoritmo = "Backtracking (B&B)";

    if (n <= 0) {
        res.ejecutado = false;
        res.motivo_omision = "Grafo vacio.";
        return res;
    }
    if (n == 1) {
        res.ruta = {0, 0};
        return res;
    }
    if (n > maxSafeN) {
        res.ejecutado = false;
        res.motivo_omision = "Omitido para N=" + to_string(n) +
                             " (supera el tope seguro N <= " + to_string(maxSafeN) +
                             " por explosion factorial O((N-1)!)).";
        return res;
    }

    double memAntes = Profiler::obtenerMemoriaProcesoKB();
    Profiler timer;
    timer.start();

    const auto& mat = graph.getMatrix();
    double INF = numeric_limits<double>::infinity();

    // Precomputo de arista minima saliente por cada nodo para poda agresiva admisible
    vector<double> minSalida(static_cast<size_t>(n), INF);
    uint64_t estados = 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i != j && mat[i][j] < minSalida[i]) {
                minSalida[i] = mat[i][j];
            }
            estados++;
        }
    }

    double sumaMinClientes = 0.0;
    for (int i = 1; i < n; i++) {
        sumaMinClientes += minSalida[i];
    }

    // Cota superior inicial voraz (Nearest Neighbor) para acelerar la poda desde la raiz
    TSPResult semillaVoraz = NearestNeighbor(graph, 0);
    double mejorCosto = semillaVoraz.distancia_km;
    vector<int> mejorRuta = semillaVoraz.ruta;
    estados += semillaVoraz.operaciones_o_estados;

    vector<bool> visitado(static_cast<size_t>(n), false);
    vector<int> rutaActual;
    rutaActual.reserve(static_cast<size_t>(n + 1));
    visitado[0] = true;
    rutaActual.push_back(0);

    backtrackRec(graph, 0, 1, 0.0, sumaMinClientes, minSalida,
                 visitado, rutaActual, mejorCosto, mejorRuta, estados);

    double memDespues = Profiler::obtenerMemoriaProcesoKB();
    double memEstructuralKB = static_cast<double>(
        minSalida.capacity() * sizeof(double) +
        rutaActual.capacity() * sizeof(int) +
        mejorRuta.capacity() * sizeof(int) +
        static_cast<size_t>(n) * 64 // Marco de pila recursiva O(N)
    ) / 1024.0;

    res.tiempo_us = timer.elapsedMicroseconds();
    res.tiempo_ms = res.tiempo_us / 1000.0;
    res.distancia_km = mejorCosto;
    res.ruta = move(mejorRuta);
    res.costo_usd = graph.calcularCostoRutaUSD(res.ruta);
    res.operaciones_o_estados = estados;
    res.memoria_kb = max(max(0.0, memDespues - memAntes), memEstructuralKB);
    return res;
}

TSPResult Algorithms::HeldKarp(const Graph& graph, int maxSafeN) {
    int n = graph.getNumNodes();
    TSPResult res;
    res.algoritmo = "Held-Karp (DP)";

    if (n <= 0) {
        res.ejecutado = false;
        res.motivo_omision = "Grafo vacio.";
        return res;
    }
    if (n == 1) {
        res.ruta = {0, 0};
        return res;
    }
    if (n > maxSafeN) {
        res.ejecutado = false;
        res.motivo_omision = "Omitido para N=" + to_string(n) +
                             " (supera el limite seguro N <= " + to_string(maxSafeN) +
                             " por consumo exponencial de memoria O(N * 2^N)).";
        return res;
    }

    double memAntes = Profiler::obtenerMemoriaProcesoKB();
    Profiler timer;
    timer.start();

    int numClientes = n - 1;
    size_t totalMasks = 1ULL << numClientes;
    double INF = numeric_limits<double>::infinity();
    const auto& mat = graph.getMatrix();

    // Tablas contiguas 1D de tamano 2^(N-1) * N para eficiencia de cache y medicion limpia
    vector<double> dp(totalMasks * static_cast<size_t>(n), INF);
    vector<int> padre(totalMasks * static_cast<size_t>(n), -1);
    uint64_t transiciones = 0;

    for (int v = 1; v < n; v++) {
        size_t mask = 1ULL << (v - 1);
        dp[mask * static_cast<size_t>(n) + static_cast<size_t>(v)] = mat[0][v];
        padre[mask * static_cast<size_t>(n) + static_cast<size_t>(v)] = 0;
        transiciones++;
    }

    for (size_t mask = 1; mask < totalMasks; mask++) {
        for (int u = 1; u < n; u++) {
            if ((mask & (1ULL << (u - 1))) == 0) continue;
            double costoPrevio = dp[mask * static_cast<size_t>(n) + static_cast<size_t>(u)];
            if (costoPrevio == INF) continue;

            for (int v = 1; v < n; v++) {
                size_t bitV = 1ULL << (v - 1);
                if ((mask & bitV) != 0) continue;

                transiciones++;
                size_t sigMask = mask | bitV;
                size_t idxDestino = sigMask * static_cast<size_t>(n) + static_cast<size_t>(v);
                double nuevoCosto = costoPrevio + mat[u][v];

                if (nuevoCosto < dp[idxDestino]) {
                    dp[idxDestino] = nuevoCosto;
                    padre[idxDestino] = u;
                }
            }
        }
    }

    size_t maskCompleta = totalMasks - 1;
    double mejorCosto = INF;
    int ultimoNodo = -1;

    for (int u = 1; u < n; u++) {
        transiciones++;
        double candidato = dp[maskCompleta * static_cast<size_t>(n) + static_cast<size_t>(u)] + mat[u][0];
        if (candidato < mejorCosto) {
            mejorCosto = candidato;
            ultimoNodo = u;
        }
    }

    vector<int> invertido;
    invertido.reserve(static_cast<size_t>(numClientes));
    size_t maskActual = maskCompleta;
    int nodoActual = ultimoNodo;

    while (nodoActual != 0 && nodoActual != -1) {
        invertido.push_back(nodoActual);
        int ant = padre[maskActual * static_cast<size_t>(n) + static_cast<size_t>(nodoActual)];
        maskActual ^= (1ULL << (nodoActual - 1));
        nodoActual = ant;
    }

    res.ruta.reserve(static_cast<size_t>(n + 1));
    res.ruta.push_back(0);
    for (int i = static_cast<int>(invertido.size()) - 1; i >= 0; i--) {
        res.ruta.push_back(invertido[i]);
    }
    res.ruta.push_back(0);

    // Medir memoria antes de liberar las tablas DP y padre
    double memPico = Profiler::obtenerMemoriaProcesoKB();
    double memEstructuralKB = static_cast<double>(
        dp.capacity() * sizeof(double) +
        padre.capacity() * sizeof(int) +
        res.ruta.capacity() * sizeof(int)
    ) / 1024.0;

    res.tiempo_us = timer.elapsedMicroseconds();
    res.tiempo_ms = res.tiempo_us / 1000.0;
    res.distancia_km = mejorCosto;
    res.costo_usd = graph.calcularCostoRutaUSD(res.ruta);
    res.operaciones_o_estados = transiciones;
    res.memoria_kb = max(max(0.0, memPico - memAntes), memEstructuralKB);
    return res;
}

void Algorithms::dfsPreorden(int u,
                             const vector<vector<int>>& mstAdj,
                             vector<bool>& visitado,
                             vector<int>& recorrido,
                             uint64_t& operaciones) {
    visitado[u] = true;
    recorrido.push_back(u);
    operaciones++;
    for (int v : mstAdj[u]) {
        operaciones++;
        if (!visitado[v]) {
            dfsPreorden(v, mstAdj, visitado, recorrido, operaciones);
        }
    }
}

TSPResult Algorithms::MST_DFS(const Graph& graph) {
    int n = graph.getNumNodes();
    TSPResult res;
    res.algoritmo = "MST + DFS (Prim)";

    if (n <= 0) {
        res.ejecutado = false;
        res.motivo_omision = "Grafo vacio.";
        return res;
    }
    if (n == 1) {
        res.ruta = {0, 0};
        return res;
    }

    double memAntes = Profiler::obtenerMemoriaProcesoKB();
    Profiler timer;
    timer.start();

    double INF = numeric_limits<double>::infinity();
    const auto& mat = graph.getMatrix();
    uint64_t operaciones = 0;

    vector<double> key(static_cast<size_t>(n), INF);
    vector<int> padre(static_cast<size_t>(n), -1);
    vector<bool> enMST(static_cast<size_t>(n), false);

    key[0] = 0.0;

    for (int count = 0; count < n; count++) {
        int u = -1;
        double minKey = INF;
        for (int v = 0; v < n; v++) {
            operaciones++;
            if (!enMST[v] && key[v] < minKey) {
                minKey = key[v];
                u = v;
            }
        }

        if (u == -1) break;
        enMST[u] = true;

        for (int v = 0; v < n; v++) {
            operaciones++;
            if (!enMST[v] && mat[u][v] < key[v]) {
                key[v] = mat[u][v];
                padre[v] = u;
            }
        }
    }

    vector<vector<int>> mstAdj(static_cast<size_t>(n));
    size_t aristasGuardadas = 0;
    for (int v = 1; v < n; v++) {
        if (padre[v] != -1) {
            mstAdj[padre[v]].push_back(v);
            mstAdj[v].push_back(padre[v]);
            aristasGuardadas += 2;
        }
    }

    vector<bool> visitado(static_cast<size_t>(n), false);
    res.ruta.reserve(static_cast<size_t>(n + 1));
    dfsPreorden(0, mstAdj, visitado, res.ruta, operaciones);
    res.ruta.push_back(0);

    double distanciaTotal = 0.0;
    for (size_t i = 0; i + 1 < res.ruta.size(); i++) {
        distanciaTotal += mat[res.ruta[i]][res.ruta[i + 1]];
    }

    double memPico = Profiler::obtenerMemoriaProcesoKB();
    double memEstructuralKB = static_cast<double>(
        key.capacity() * sizeof(double) +
        padre.capacity() * sizeof(int) +
        aristasGuardadas * sizeof(int) +
        mstAdj.capacity() * sizeof(vector<int>) +
        res.ruta.capacity() * sizeof(int)
    ) / 1024.0;

    res.tiempo_us = timer.elapsedMicroseconds();
    res.tiempo_ms = res.tiempo_us / 1000.0;
    res.distancia_km = distanciaTotal;
    res.costo_usd = graph.calcularCostoRutaUSD(res.ruta);
    res.operaciones_o_estados = operaciones;
    res.memoria_kb = max(max(0.0, memPico - memAntes), memEstructuralKB);
    return res;
}

TSPResult Algorithms::NearestNeighbor(const Graph& graph, int startNode) {
    int n = graph.getNumNodes();
    TSPResult res;
    res.algoritmo = "Nearest Neighbor";

    if (n <= 0) {
        res.ejecutado = false;
        res.motivo_omision = "Grafo vacio.";
        return res;
    }
    if (startNode < 0 || startNode >= n) {
        throw out_of_range("Nodo inicial fuera de rango.");
    }
    if (n == 1) {
        res.ruta = {startNode, startNode};
        return res;
    }

    double memAntes = Profiler::obtenerMemoriaProcesoKB();
    Profiler timer;
    timer.start();

    double INF = numeric_limits<double>::infinity();
    const auto& mat = graph.getMatrix();
    uint64_t comparaciones = 0;

    vector<bool> visitado(static_cast<size_t>(n), false);
    res.ruta.reserve(static_cast<size_t>(n + 1));

    int actual = startNode;
    visitado[actual] = true;
    res.ruta.push_back(actual);

    double distanciaTotal = 0.0;

    for (int paso = 1; paso < n; paso++) {
        int masCercano = -1;
        double minDist = INF;

        for (int cand = 0; cand < n; cand++) {
            comparaciones++;
            if (!visitado[cand] && mat[actual][cand] < minDist) {
                minDist = mat[actual][cand];
                masCercano = cand;
            }
        }

        visitado[masCercano] = true;
        res.ruta.push_back(masCercano);
        distanciaTotal += minDist;
        actual = masCercano;
    }

    distanciaTotal += mat[actual][startNode];
    res.ruta.push_back(startNode);

    double memPico = Profiler::obtenerMemoriaProcesoKB();
    double memEstructuralKB = static_cast<double>(
        (visitado.capacity() / 8 + 1) +
        res.ruta.capacity() * sizeof(int)
    ) / 1024.0;

    res.tiempo_us = timer.elapsedMicroseconds();
    res.tiempo_ms = res.tiempo_us / 1000.0;
    res.distancia_km = distanciaTotal;
    res.costo_usd = graph.calcularCostoRutaUSD(res.ruta);
    res.operaciones_o_estados = comparaciones;
    res.memoria_kb = max(max(0.0, memPico - memAntes), memEstructuralKB);
    return res;
}

RecomendacionAdaptativa Algorithms::SeleccionAdaptativa(const Graph& graph,
                                                        bool exigirGarantiaTeorica) {
    int n = graph.getNumNodes();
    if (n < 2) {
        throw invalid_argument("Se requieren al menos 2 nodos para seleccionar estrategia.");
    }

    RecomendacionAdaptativa rec;
    rec.cumpleDesigualdadTriangular = graph.validateGraph();

    if (n <= 10) {
        rec.categoriaEscala = "Instancia Pequena (N <= 10)";
        rec.algoritmoElegido = "Backtracking con Poda (Branch & Bound)";
        rec.justificacion =
            "Con N=" + to_string(n) + ", el espacio de busqueda factorial podado es pequeno. "
            "Backtracking garantiza el optimo exacto global (0% desviacion) con memoria lineal O(N).";
        rec.resultado = Backtracking(graph, kMaxSafeBacktracking);
        rec.resultado.desviacion_pct = 0.0;
    } else if (n <= 20) {
        rec.categoriaEscala = "Instancia Mediana (11 <= N <= 20)";
        rec.algoritmoElegido = "Held-Karp (Programacion Dinamica + Bitmask)";
        rec.justificacion =
            "Con N=" + to_string(n) + ", Backtracking O((N-1)!) se vuelve costoso, pero "
            "Held-Karp O(N^2 * 2^N) resuelve el optimo global exacto reutilizando subproblemas "
            "dentro de un consumo de RAM manejable.";
        rec.resultado = HeldKarp(graph, kMaxSafeHeldKarp);
        rec.resultado.desviacion_pct = 0.0;
    } else {
        rec.categoriaEscala = "Instancia Grande (N > 20)";
        if (exigirGarantiaTeorica && rec.cumpleDesigualdadTriangular) {
            rec.algoritmoElegido = "MST + DFS (Prim - 2-Aproximacion)";
            rec.justificacion =
                "Con N=" + to_string(n) + ", los metodos exactos son inviables en tiempo/RAM. "
                "Como el grafo cumple la desigualdad triangular (TSP metrico), MST + DFS ejecuta "
                "en tiempo polinomial O(N^2) y garantiza teoricamente Costo <= 2 * OPT.";
            rec.resultado = MST_DFS(graph);
        } else {
            rec.algoritmoElegido = "Nearest Neighbor (Heuristica Voraz)";
            rec.justificacion =
                "Con N=" + to_string(n) + ", los algoritmos exactos exceden los limites operativos. "
                "Nearest Neighbor ofrece complejidad O(N^2), memoria minima O(N) y el menor costo "
                "empirico promedio en milisegundos para despacho en tiempo real.";
            rec.resultado = NearestNeighbor(graph, 0);
        }
    }

    return rec;
}
