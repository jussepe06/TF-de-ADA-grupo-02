#include "Algorithms.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
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
#elif defined(__linux__)
#include <unistd.h>
#include <fstream>
#endif

using namespace std;

double Profiler::obtenerMemoriaProcesoKB() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))) {
        return static_cast<double>(pmc.WorkingSetSize) / 1024.0;
    }
#elif defined(__linux__)
    ifstream statm("/proc/self/statm");
    if (statm.is_open()) {
        long size = 0, resident = 0;
        if (statm >> size >> resident) {
            long pageSizeKB = sysconf(_SC_PAGE_SIZE) / 1024;
            return static_cast<double>(resident * pageSizeKB);
        }
    }
#endif
    return 0.0;
}

bool Algorithms::validarRuta(const Graph& graph,
                             const vector<int>& ruta,
                             double costoReportadoKm,
                             string& mensajeError) {
    int n = graph.getNumNodes();
    if (n <= 1) {
        if (ruta.empty()) {
            mensajeError = "Ruta vacia para grafo no trivial.";
            return false;
        }
        return true;
    }

    // 1. Longitud obligatoria de un ciclo hamiltoniano TSP: N + 1
    if (static_cast<int>(ruta.size()) != n + 1) {
        mensajeError = "Longitud de ruta invalida: se esperaban " + to_string(n + 1) +
                       " nodos, pero tiene " + to_string(ruta.size());
        return false;
    }

    // 2. Inicia y finaliza en el centro de distribucion (Nodo 0)
    if (ruta.front() != 0) {
        mensajeError = "La ruta no inicia en el Centro de Distribucion (Nodo 0). Inicia en " + to_string(ruta.front());
        return false;
    }
    if (ruta.back() != 0) {
        mensajeError = "La ruta no retorna al Centro de Distribucion (Nodo 0). Finaliza en " + to_string(ruta.back());
        return false;
    }

    // 3. Ningun cliente repetido y todos los clientes visitados exactamente una vez
    vector<bool> visitado(static_cast<size_t>(n), false);
    visitado[0] = true;

    for (size_t i = 1; i + 1 < ruta.size(); i++) {
        int u = ruta[i];
        if (u < 0 || u >= n) {
            mensajeError = "Identificador de nodo fuera de rango: " + to_string(u);
            return false;
        }
        if (u == 0) {
            mensajeError = "Retorno prematuro al Almacen Central (Nodo 0) en la posicion " + to_string(i);
            return false;
        }
        if (visitado[u]) {
            mensajeError = "Cliente visitado mas de una vez: Nodo " + to_string(u);
            return false;
        }
        visitado[u] = true;
    }

    for (int i = 1; i < n; i++) {
        if (!visitado[i]) {
            mensajeError = "Cliente omitido en la ruta: Nodo " + to_string(i);
            return false;
        }
    }

    // 4. Verificacion de costo reportado contra la suma directa de aristas en el grafo
    double costoCalculado = 0.0;
    for (size_t i = 0; i + 1 < ruta.size(); i++) {
        costoCalculado += graph.getCost(ruta[i], ruta[i + 1]);
    }

    if (abs(costoCalculado - costoReportadoKm) > 1e-3) {
        mensajeError = "Discrepancia en costo reportado (" + to_string(costoReportadoKm) +
                       " km) vs costo real de la secuencia (" + to_string(costoCalculado) + " km).";
        return false;
    }

    return true;
}

void Algorithms::clasificarYCalcularDesviaciones(vector<TSPResult>& resultados,
                                                double cotaInferiorConocida) {
    bool hayOptimoExacto = false;
    double costoOptimoExacto = numeric_limits<double>::infinity();

    for (const auto& r : resultados) {
        if (r.ejecutado && r.tipo_solucion == TipoSolucion::EXACTA && r.distancia_km < costoOptimoExacto) {
            costoOptimoExacto = r.distancia_km;
            hayOptimoExacto = true;
        }
    }

    if (hayOptimoExacto && costoOptimoExacto > 0.0 && costoOptimoExacto < numeric_limits<double>::infinity()) {
        for (auto& r : resultados) {
            if (!r.ejecutado) continue;
            r.tipo_referencia = TipoReferencia::OPTIMO_EXACTO;
            r.tipo_referencia_str = "Optimo Exacto";
            r.desviacion_pct = max(0.0, ((r.distancia_km - costoOptimoExacto) / costoOptimoExacto) * 100.0);
            r.desviacion_valida = true;
            r.diferencia_mejor_observada_pct = r.desviacion_pct;
            if (cotaInferiorConocida > 0.0) {
                r.diferencia_cota_inferior_pct = max(0.0, ((r.distancia_km - cotaInferiorConocida) / cotaInferiorConocida) * 100.0);
                r.cota_inferior_mst_km = cotaInferiorConocida;
            }
        }
    } else {
        // Ningun algoritmo exacto ejecutado: NO asignar 0% de error como si fuera optimo
        double mejorObservado = numeric_limits<double>::infinity();
        for (const auto& r : resultados) {
            if (r.ejecutado && r.distancia_km < mejorObservado) {
                mejorObservado = r.distancia_km;
            }
        }

        for (auto& r : resultados) {
            if (!r.ejecutado) continue;
            r.tipo_referencia = TipoReferencia::MEJOR_OBSERVADO;
            r.tipo_referencia_str = "Mejor Observado";
            r.desviacion_pct = -1.0; // Invalida como error sobre optimo
            r.desviacion_valida = false;

            if (mejorObservado > 0.0 && mejorObservado < numeric_limits<double>::infinity()) {
                r.diferencia_mejor_observada_pct = max(0.0, ((r.distancia_km - mejorObservado) / mejorObservado) * 100.0);
            }
            if (cotaInferiorConocida > 0.0) {
                r.diferencia_cota_inferior_pct = max(0.0, ((r.distancia_km - cotaInferiorConocida) / cotaInferiorConocida) * 100.0);
                r.cota_inferior_mst_km = cotaInferiorConocida;
            }
        }
    }
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
                              uint64_t& estadosExplorados,
                              uint64_t& estadosPodados) {
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

    // Poda por Cota Inferior Admisible: Costo acumulado + Salida actual + Salidas minimas restantes
    double cotaInferior = costoActual + minSalida[actual] + sumaMinRestante;
    if (cotaInferior >= mejorCosto - 1e-9) {
        estadosPodados++;
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
                             estadosExplorados, estadosPodados);

                rutaActual.pop_back();
                visitado[sig] = false;
            } else {
                estadosPodados++;
            }
        }
    }
}

TSPResult Algorithms::Backtracking(const Graph& graph, int maxSafeN) {
    int n = graph.getNumNodes();
    TSPResult res;
    res.algoritmo = "Backtracking (B&B)";
    res.tipo_solucion = TipoSolucion::EXACTA;
    res.tipo_solucion_str = "Optima Exacta";

    if (n <= 0) {
        res.ejecutado = false;
        res.motivo_omision = "Grafo vacio.";
        return res;
    }
    if (n == 1) {
        res.ruta = {0, 0};
        res.distancia_km = 0.0;
        res.costo_usd = 0.0;
        res.ruta_valida = true;
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

    vector<double> minSalida(static_cast<size_t>(n), INF);
    uint64_t estadosExplorados = 0;
    uint64_t estadosPodados = 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i != j && mat[i][j] < minSalida[i]) {
                minSalida[i] = mat[i][j];
            }
        }
    }

    double sumaMinClientes = 0.0;
    for (int i = 1; i < n; i++) {
        sumaMinClientes += minSalida[i];
    }

    // Cota superior inicial voraz para maximizar la efectividad de la poda desde el inicio
    TSPResult semillaVoraz = NearestNeighbor(graph, 0);
    double mejorCosto = semillaVoraz.distancia_km;
    vector<int> mejorRuta = semillaVoraz.ruta;

    vector<bool> visitado(static_cast<size_t>(n), false);
    vector<int> rutaActual;
    rutaActual.reserve(static_cast<size_t>(n + 1));
    visitado[0] = true;
    rutaActual.push_back(0);

    backtrackRec(graph, 0, 1, 0.0, sumaMinClientes, minSalida,
                 visitado, rutaActual, mejorCosto, mejorRuta,
                 estadosExplorados, estadosPodados);

    double memDespues = Profiler::obtenerMemoriaProcesoKB();
    double memAuxiliarKB = static_cast<double>(
        minSalida.capacity() * sizeof(double) +
        rutaActual.capacity() * sizeof(int) +
        mejorRuta.capacity() * sizeof(int) +
        static_cast<size_t>(n) * 64 // Marco de pila de llamadas recursivas O(N)
    ) / 1024.0;

    res.tiempo_us = timer.elapsedMicroseconds();
    res.tiempo_ms = res.tiempo_us / 1000.0;
    res.distancia_km = mejorCosto;
    res.ruta = move(mejorRuta);
    res.costo_usd = graph.calcularCostoRutaUSD(res.ruta);

    res.estados_explorados = estadosExplorados;
    res.estados_podados = estadosPodados;
    res.operaciones_o_estados = estadosExplorados + estadosPodados;
    res.detalle_operaciones = "Estados explorados: " + to_string(estadosExplorados) +
                              ", Estados podados: " + to_string(estadosPodados);

    res.memoria_auxiliar_kb = memAuxiliarKB;
    res.memoria_proceso_kb = memDespues;
    res.memoria_kb = max(max(0.0, memDespues - memAntes), memAuxiliarKB);

    res.ruta_valida = validarRuta(graph, res.ruta, res.distancia_km, res.error_validacion_ruta);
    return res;
}

TSPResult Algorithms::HeldKarp(const Graph& graph, int maxSafeN, double maxMemoriaMB) {
    int n = graph.getNumNodes();
    TSPResult res;
    res.algoritmo = "Held-Karp (DP)";
    res.tipo_solucion = TipoSolucion::EXACTA;
    res.tipo_solucion_str = "Optima Exacta";

    if (n <= 0) {
        res.ejecutado = false;
        res.motivo_omision = "Grafo vacio.";
        return res;
    }
    if (n == 1) {
        res.ruta = {0, 0};
        res.distancia_km = 0.0;
        res.costo_usd = 0.0;
        res.ruta_valida = true;
        return res;
    }

    int numClientes = n - 1;

    // Proteccion frente a desbordamiento de desplazamiento de bits (bitmask 64-bit)
    if (numClientes >= 31 || n > maxSafeN) {
        res.ejecutado = false;
        res.motivo_omision = "Omitido para N=" + to_string(n) +
                             " (supera el limite seguro N <= " + to_string(maxSafeN) +
                             " por consumo de memoria exponencial O(N * 2^N)).";
        return res;
    }

    size_t totalMasks = 1ULL << numClientes;
    double memRequeridaBytes = static_cast<double>(totalMasks) * n * (sizeof(double) + sizeof(int));
    double memRequeridaMB = memRequeridaBytes / (1024.0 * 1024.0);

    // Proteccion previa de asignacion contra presupuesto de memoria configurado
    if (memRequeridaMB > maxMemoriaMB) {
        res.ejecutado = false;
        res.motivo_omision = "Memoria requerida estimada (" + to_string(static_cast<int>(memRequeridaMB)) +
                             " MB) supera el presupuesto configurado (" + to_string(static_cast<int>(maxMemoriaMB)) +
                             " MB). Held-Karp no es viable en estas condiciones.";
        return res;
    }

    double memAntes = Profiler::obtenerMemoriaProcesoKB();
    Profiler timer;
    timer.start();

    double INF = numeric_limits<double>::infinity();
    const auto& mat = graph.getMatrix();

    vector<double> dp;
    vector<int> padre;

    try {
        dp.assign(totalMasks * static_cast<size_t>(n), INF);
        padre.assign(totalMasks * static_cast<size_t>(n), -1);
    } catch (const bad_alloc&) {
        res.ejecutado = false;
        res.motivo_omision = "Memoria insuficiente (std::bad_alloc) al reservar tablas DP para N=" + to_string(n);
        return res;
    }

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

    double memPico = Profiler::obtenerMemoriaProcesoKB();
    double memAuxiliarKB = static_cast<double>(
        dp.capacity() * sizeof(double) +
        padre.capacity() * sizeof(int) +
        res.ruta.capacity() * sizeof(int)
    ) / 1024.0;

    res.tiempo_us = timer.elapsedMicroseconds();
    res.tiempo_ms = res.tiempo_us / 1000.0;
    res.distancia_km = mejorCosto;
    res.costo_usd = graph.calcularCostoRutaUSD(res.ruta);

    res.transiciones_dp = transiciones;
    res.operaciones_o_estados = transiciones;
    res.detalle_operaciones = "Transiciones DP computadas: " + to_string(transiciones);

    res.memoria_auxiliar_kb = memAuxiliarKB;
    res.memoria_proceso_kb = memPico;
    res.memoria_kb = max(max(0.0, memPico - memAntes), memAuxiliarKB);

    res.ruta_valida = validarRuta(graph, res.ruta, res.distancia_km, res.error_validacion_ruta);
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
        res.distancia_km = 0.0;
        res.costo_usd = 0.0;
        res.tipo_solucion = TipoSolucion::APROXIMADA;
        res.tipo_solucion_str = "2-Aproximacion (Trivial N=1)";
        res.ruta_valida = true;
        return res;
    }

    // Comprobacion de garantia formal de 2-aproximacion
    // Requiere: Grafo completo, distancias no negativas, simetrico y desigualdad triangular
    bool esMetrico = graph.checkTriangularInequality();
    if (esMetrico) {
        res.tipo_solucion = TipoSolucion::APROXIMADA;
        res.tipo_solucion_str = "2-Aproximacion (Garantia Teórica Costo <= 2*OPT)";
    } else {
        res.tipo_solucion = TipoSolucion::HEURISTICA;
        res.tipo_solucion_str = "Heuristica (Sin Garantia Teórica: No cumple Desigualdad Triangular)";
    }

    double memAntes = Profiler::obtenerMemoriaProcesoKB();
    Profiler timer;
    timer.start();

    double INF = numeric_limits<double>::infinity();
    const auto& mat = graph.getMatrix();
    uint64_t comparacionesPrim = 0;
    uint64_t visitasDFS = 0;

    vector<double> key(static_cast<size_t>(n), INF);
    vector<int> padre(static_cast<size_t>(n), -1);
    vector<bool> enMST(static_cast<size_t>(n), false);

    key[0] = 0.0;

    for (int count = 0; count < n; count++) {
        int u = -1;
        double minKey = INF;
        for (int v = 0; v < n; v++) {
            comparacionesPrim++;
            if (!enMST[v] && key[v] < minKey) {
                minKey = key[v];
                u = v;
            }
        }

        if (u == -1) break;
        enMST[u] = true;

        for (int v = 0; v < n; v++) {
            comparacionesPrim++;
            if (!enMST[v] && mat[u][v] < key[v]) {
                key[v] = mat[u][v];
                padre[v] = u;
            }
        }
    }

    // Calculo del peso exacto del MST (Cota inferior teorica de TSP metrico: W(MST) <= OPT)
    double pesoTotalMST = 0.0;
    vector<vector<int>> mstAdj(static_cast<size_t>(n));
    size_t aristasGuardadas = 0;

    for (int v = 1; v < n; v++) {
        if (padre[v] != -1) {
            pesoTotalMST += mat[padre[v]][v];
            mstAdj[padre[v]].push_back(v);
            mstAdj[v].push_back(padre[v]);
            aristasGuardadas += 2;
        }
    }
    res.cota_inferior_mst_km = pesoTotalMST;

    // Recorrido en preorden del MST y atajos de regreso (shortcutting)
    vector<bool> visitado(static_cast<size_t>(n), false);
    res.ruta.reserve(static_cast<size_t>(n + 1));
    dfsPreorden(0, mstAdj, visitado, res.ruta, visitasDFS);
    res.ruta.push_back(0);

    // Calculo del costo total sobre las aristas reales del grafo original (NO sobre el MST)
    double distanciaTotal = 0.0;
    for (size_t i = 0; i + 1 < res.ruta.size(); i++) {
        distanciaTotal += mat[res.ruta[i]][res.ruta[i + 1]];
    }

    double memPico = Profiler::obtenerMemoriaProcesoKB();
    double memAuxiliarKB = static_cast<double>(
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

    res.comparaciones_prim = comparacionesPrim;
    res.visitas_dfs = visitasDFS;
    res.operaciones_o_estados = comparacionesPrim + visitasDFS;
    res.detalle_operaciones = "Comparaciones Prim: " + to_string(comparacionesPrim) +
                              ", Visitas/Aristas DFS: " + to_string(visitasDFS);

    res.memoria_auxiliar_kb = memAuxiliarKB;
    res.memoria_proceso_kb = memPico;
    res.memoria_kb = max(max(0.0, memPico - memAntes), memAuxiliarKB);

    res.ruta_valida = validarRuta(graph, res.ruta, res.distancia_km, res.error_validacion_ruta);
    return res;
}

TSPResult Algorithms::NearestNeighbor(const Graph& graph, int startNode) {
    int n = graph.getNumNodes();
    TSPResult res;
    res.algoritmo = "Nearest Neighbor";
    res.tipo_solucion = TipoSolucion::HEURISTICA;
    res.tipo_solucion_str = "Heuristica Voraz";

    if (n <= 0) {
        res.ejecutado = false;
        res.motivo_omision = "Grafo vacio.";
        return res;
    }
    if (startNode < 0 || startNode >= n) {
        throw out_of_range("Nodo inicial fuera de rango: " + to_string(startNode));
    }
    if (n == 1) {
        res.ruta = {startNode, startNode};
        res.distancia_km = 0.0;
        res.costo_usd = 0.0;
        res.ruta_valida = true;
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
    double memAuxiliarKB = static_cast<double>(
        (visitado.capacity() / 8 + 1) +
        res.ruta.capacity() * sizeof(int)
    ) / 1024.0;

    res.tiempo_us = timer.elapsedMicroseconds();
    res.tiempo_ms = res.tiempo_us / 1000.0;
    res.distancia_km = distanciaTotal;
    res.costo_usd = graph.calcularCostoRutaUSD(res.ruta);

    res.comparaciones_nn = comparaciones;
    res.operaciones_o_estados = comparaciones;
    res.detalle_operaciones = "Comparaciones de distancia voraz: " + to_string(comparaciones);

    res.memoria_auxiliar_kb = memAuxiliarKB;
    res.memoria_proceso_kb = memPico;
    res.memoria_kb = max(max(0.0, memPico - memAntes), memAuxiliarKB);

    res.ruta_valida = validarRuta(graph, res.ruta, res.distancia_km, res.error_validacion_ruta);
    return res;
}

RecomendacionAdaptativa Algorithms::SeleccionAdaptativa(const Graph& graph,
                                                        const ConfiguracionAdaptativa& config) {
    int n = graph.getNumNodes();
    if (n < 2) {
        throw invalid_argument("Se requieren al menos 2 nodos para seleccionar estrategia.");
    }

    RecomendacionAdaptativa rec;
    rec.configUtilizada = config;
    rec.cumpleDesigualdadTriangular = graph.checkTriangularInequality();

    // 1. Caso de exigencia explicita de optimo global
    if (config.criterio == CriterioPrioridad::OPTIMO_EXACTO) {
        if (n <= config.maxSafeBacktracking) {
            rec.categoriaEscala = "Instancia Pequena (N = " + to_string(n) + " <= " + to_string(config.maxSafeBacktracking) + ")";
            rec.algoritmoElegido = "Backtracking con Poda (Branch & Bound)";
            rec.justificacion = "Usuario solicito Solucion Exacta. Para N=" + to_string(n) +
                                ", Backtracking explora el espacio podado con memoria O(N) garantizando 0% de error.";
            rec.resultado = Backtracking(graph, config.maxSafeBacktracking);
            rec.resultado.desviacion_pct = 0.0;
            rec.resultado.desviacion_valida = true;
            rec.resultado.tipo_referencia = TipoReferencia::OPTIMO_EXACTO;
            rec.resultado.tipo_referencia_str = "Optimo Exacto";
            return rec;
        } else if (n <= config.maxSafeHeldKarp) {
            rec.categoriaEscala = "Instancia Mediana (N = " + to_string(n) + " <= " + to_string(config.maxSafeHeldKarp) + ")";
            rec.algoritmoElegido = "Held-Karp (Programacion Dinamica + Bitmask)";
            rec.justificacion = "Usuario solicito Solucion Exacta. Para N=" + to_string(n) +
                                ", Held-Karp resuelve el optimo global en O(N^2 * 2^N) dentro del presupuesto de memoria (" +
                                to_string(static_cast<int>(config.memoriaMaximaMB)) + " MB).";
            rec.resultado = HeldKarp(graph, config.maxSafeHeldKarp, config.memoriaMaximaMB);
            if (rec.resultado.ejecutado) {
                rec.resultado.desviacion_pct = 0.0;
                rec.resultado.desviacion_valida = true;
                rec.resultado.tipo_referencia = TipoReferencia::OPTIMO_EXACTO;
                rec.resultado.tipo_referencia_str = "Optimo Exacto";
            }
            return rec;
        } else {
            // No es viable un algoritmo exacto sin colapsar memoria/tiempo
            rec.categoriaEscala = "Instancia Grande (N = " + to_string(n) + " > " + to_string(config.maxSafeHeldKarp) + ")";
            rec.algoritmoElegido = "Nearest Neighbor (Heuristica Alternativa por Limite de Recursos)";
            rec.justificacion = "AVISO: Se solicito optimo exacto, pero para N=" + to_string(n) +
                                " un algoritmo exacto requeriria memoria/tiempo exponencial prohibitivo. "
                                "Por seguridad operativa se recomienda la solucion heuristica rapida Nearest Neighbor.";
            rec.resultado = NearestNeighbor(graph, 0);
            return rec;
        }
    }

    // 2. Caso de exigencia de garantia matematica formal
    if (config.criterio == CriterioPrioridad::GARANTIA_TEORICA || config.exigirGarantiaTeorica) {
        if (rec.cumpleDesigualdadTriangular) {
            rec.categoriaEscala = "Instancia con Requisito de Garantia Formal (TSP Metrico)";
            rec.algoritmoElegido = "MST + DFS (Prim - 2-Aproximacion)";
            rec.justificacion = "El grafo cumple la Desigualdad Triangular (TSP metrico). "
                                "MST + DFS garantiza matematicamente una cota formal: Costo <= 2 * OPT en tiempo polinomial O(N^2).";
            rec.resultado = MST_DFS(graph);
            return rec;
        } else {
            rec.categoriaEscala = "Instancia No Metrica (Falla Desigualdad Triangular)";
            rec.algoritmoElegido = "Nearest Neighbor (Alternativa Heuristica)";
            rec.justificacion = "ADVERTENCIA: Se solicito garantia teorica de aproximacion, pero el grafo NO cumple "
                                "la Desigualdad Triangular. Sin esta condicion metrica, la cota 2*OPT no se sostiene. "
                                "Se selecciona Nearest Neighbor por mejor calidad empirica promedio.";
            rec.resultado = NearestNeighbor(graph, 0);
            return rec;
        }
    }

    // 3. Caso de maxima velocidad de respuesta
    if (config.criterio == CriterioPrioridad::MAXIMA_VELOCIDAD) {
        rec.categoriaEscala = "Despacho Rapido en Tiempo Real";
        rec.algoritmoElegido = "Nearest Neighbor (Heuristica Voraz)";
        rec.justificacion = "Prioridad: Maxima Velocidad. Nearest Neighbor ofrece respuesta inmediata en O(N^2) "
                            "con minima huella de memoria O(N).";
        rec.resultado = NearestNeighbor(graph, 0);
        return rec;
    }

    // 4. Criterio Balanceado (predeterminado segun escala N y recursos)
    if (n <= 10) {
        rec.categoriaEscala = "Instancia Pequena (N = " + to_string(n) + " <= 10)";
        rec.algoritmoElegido = "Backtracking con Poda (Branch & Bound)";
        rec.justificacion = "Con N=" + to_string(n) + ", el espacio factorial podado es pequeno. "
                            "Backtracking garantiza el optimo global exacto (0% error) con memoria lineal O(N).";
        rec.resultado = Backtracking(graph, config.maxSafeBacktracking);
        rec.resultado.desviacion_pct = 0.0;
        rec.resultado.desviacion_valida = true;
        rec.resultado.tipo_referencia = TipoReferencia::OPTIMO_EXACTO;
        rec.resultado.tipo_referencia_str = "Optimo Exacto";
    } else if (n <= config.maxSafeHeldKarp) {
        rec.categoriaEscala = "Instancia Mediana (11 <= N = " + to_string(n) + " <= " + to_string(config.maxSafeHeldKarp) + ")";
        rec.algoritmoElegido = "Held-Karp (Programacion Dinamica + Bitmask)";
        rec.justificacion = "Con N=" + to_string(n) + ", Backtracking O((N-1)!) se vuelve costoso, pero "
                            "Held-Karp O(N^2 * 2^N) halla el optimo global reutilizando subproblemas "
                            "dentro de un presupuesto de RAM de " + to_string(static_cast<int>(config.memoriaMaximaMB)) + " MB.";
        rec.resultado = HeldKarp(graph, config.maxSafeHeldKarp, config.memoriaMaximaMB);
        if (rec.resultado.ejecutado) {
            rec.resultado.desviacion_pct = 0.0;
            rec.resultado.desviacion_valida = true;
            rec.resultado.tipo_referencia = TipoReferencia::OPTIMO_EXACTO;
            rec.resultado.tipo_referencia_str = "Optimo Exacto";
        }
    } else {
        rec.categoriaEscala = "Instancia Grande (N = " + to_string(n) + " > " + to_string(config.maxSafeHeldKarp) + ")";
        if (config.exigirGarantiaTeorica && rec.cumpleDesigualdadTriangular) {
            rec.algoritmoElegido = "MST + DFS (Prim - 2-Aproximacion)";
            rec.justificacion = "Con N=" + to_string(n) + ", los metodos exactos son inviables. Como el grafo es metrico, "
                                "MST + DFS provee garantia matematica formal Costo <= 2 * OPT en tiempo polinomial O(N^2).";
            rec.resultado = MST_DFS(graph);
        } else {
            rec.algoritmoElegido = "Nearest Neighbor (Heuristica Voraz)";
            rec.justificacion = "Con N=" + to_string(n) + ", los algoritmos exactos exceden los limites operativos. "
                                "Nearest Neighbor ofrece complejidad O(N^2), memoria minima O(N) y el menor costo "
                                "empirico promedio en milisegundos para despacho en tiempo real.";
            rec.resultado = NearestNeighbor(graph, 0);
        }
    }

    return rec;
}

RecomendacionAdaptativa Algorithms::SeleccionAdaptativa(const Graph& graph,
                                                        bool exigirGarantiaTeorica) {
    ConfiguracionAdaptativa config;
    config.exigirGarantiaTeorica = exigirGarantiaTeorica;
    if (exigirGarantiaTeorica) {
        config.criterio = CriterioPrioridad::GARANTIA_TEORICA;
    }
    return SeleccionAdaptativa(graph, config);
}
