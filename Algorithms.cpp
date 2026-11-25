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
        return static_cast<double>(pmc.PrivateUsage) / 1024.0;
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

double Algorithms::estimarMemoriaHeldKarpKB(int n) {
    if (n <= 1) return 0.0;
    if (n >= 31) return numeric_limits<double>::infinity();
    size_t totalMasks = 1ULL << (n - 1);
    double bytes = static_cast<double>(totalMasks) * static_cast<double>(n) *
                   static_cast<double>(sizeof(double) + sizeof(int));
    return bytes / 1024.0;
}

double Algorithms::calcularCotaInferiorMST(const Graph& graph) {
    int n = graph.getNumNodes();
    if (n <= 1) return 0.0;

    double INF = numeric_limits<double>::infinity();
    const auto& mat = graph.getMatrix();
    vector<double> key(static_cast<size_t>(n), INF);
    vector<bool> enMST(static_cast<size_t>(n), false);
    key[0] = 0.0;

    double pesoTotalMST = 0.0;
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
        pesoTotalMST += minKey;

        for (int v = 0; v < n; v++) {
            if (!enMST[v] && mat[u][v] < key[v]) {
                key[v] = mat[u][v];
            }
        }
    }
    return pesoTotalMST;
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

    if (static_cast<int>(ruta.size()) != n + 1) {
        mensajeError = "Longitud de ruta invalida: se esperaban " + to_string(n + 1) +
                       " nodos, pero tiene " + to_string(ruta.size());
        return false;
    }

    if (ruta.front() != 0) {
        mensajeError = "La ruta no inicia en el Almacen Central (Nodo 0). Inicia en " + to_string(ruta.front());
        return false;
    }
    if (ruta.back() != 0) {
        mensajeError = "La ruta no retorna al Almacen Central (Nodo 0). Finaliza en " + to_string(ruta.back());
        return false;
    }

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

    double costoCalculado = 0.0;
    for (size_t i = 0; i + 1 < ruta.size(); i++) {
        costoCalculado += graph.getCost(ruta[i], ruta[i + 1]);
    }

    if (abs(costoCalculado - costoReportadoKm) > 1e-3) {
        mensajeError = "Discrepancia en distancia reportada (" + to_string(costoReportadoKm) +
                       " km) vs suma real de aristas (" + to_string(costoCalculado) + " km).";
        return false;
    }

    return true;
}

void Algorithms::clasificarYCalcularDesviaciones(vector<TSPResult>& resultados,
                                                 double cotaInferiorConocida) {
    bool hayOptimoExacto = false;
    double costoOptimoExacto = numeric_limits<double>::infinity();
    double mejorObservadoKm = numeric_limits<double>::infinity();
    double mejorCostoUSD = numeric_limits<double>::infinity();
    double tiempoMasLentoMs = 0.0;

    for (const auto& r : resultados) {
        if (!r.ejecutado) continue;
        if (r.tipo_solucion == TipoSolucion::EXACTA && r.distancia_km < costoOptimoExacto) {
            costoOptimoExacto = r.distancia_km;
            hayOptimoExacto = true;
        }
        if (r.distancia_km < mejorObservadoKm) {
            mejorObservadoKm = r.distancia_km;
        }
        if (r.costo_usd < mejorCostoUSD) {
            mejorCostoUSD = r.costo_usd;
        }
        if (r.tiempo_ms > tiempoMasLentoMs) {
            tiempoMasLentoMs = r.tiempo_ms;
        }
        if (cotaInferiorConocida <= 0.0 && r.cota_inferior_mst_km > 0.0) {
            cotaInferiorConocida = r.cota_inferior_mst_km;
        }
    }

    for (auto& r : resultados) {
        if (!r.ejecutado) continue;

        if (mejorCostoUSD < numeric_limits<double>::infinity()) {
            r.sobrecosto_usd_vs_mejor = max(0.0, r.costo_usd - mejorCostoUSD);
        }
        if (r.tiempo_ms > 1e-6 && tiempoMasLentoMs > 0.0) {
            r.speedup_vs_mas_lento = max(1.0, tiempoMasLentoMs / r.tiempo_ms);
        }

        if (hayOptimoExacto && costoOptimoExacto > 0.0 && costoOptimoExacto < numeric_limits<double>::infinity()) {
            r.tipo_referencia = TipoReferencia::OPTIMO_EXACTO;
            r.tipo_referencia_str = "Optimo Exacto";
            r.desviacion_pct = max(0.0, ((r.distancia_km - costoOptimoExacto) / costoOptimoExacto) * 100.0);
            r.desviacion_valida = true;
            r.diferencia_mejor_observada_pct = r.desviacion_pct;
        } else {
            // Sin algoritmo exacto ejecutado: NO falsear 0% de error sobre el optimo
            r.tipo_referencia = TipoReferencia::MEJOR_OBSERVADO;
            r.tipo_referencia_str = "Mejor Observado";
            r.desviacion_pct = -1.0;
            r.desviacion_valida = false;
            if (mejorObservadoKm > 0.0 && mejorObservadoKm < numeric_limits<double>::infinity()) {
                r.diferencia_mejor_observada_pct = max(0.0, ((r.distancia_km - mejorObservadoKm) / mejorObservadoKm) * 100.0);
            }
        }

        if (cotaInferiorConocida > 0.0) {
            r.cota_inferior_mst_km = cotaInferiorConocida;
            r.diferencia_cota_inferior_pct = max(0.0, ((r.distancia_km - cotaInferiorConocida) / cotaInferiorConocida) * 100.0);
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

    ValidacionGrafo val = graph.validateGraphDetailed(false);
    if (!val.esValido) {
        res.ejecutado = false;
        res.motivo_omision = "Instancia invalida: " + (val.errores.empty() ? "Error estructural" : val.errores.front());
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
        static_cast<size_t>(n) * 64
    ) / 1024.0;

    res.tiempo_us = timer.elapsedMicroseconds();
    res.tiempo_ms = res.tiempo_us / 1000.0;
    res.distancia_km = mejorCosto;
    res.ruta = move(mejorRuta);
    res.costo_usd = graph.calcularCostoRutaUSD(res.ruta);

    res.estados_explorados = estadosExplorados;
    res.estados_podados = estadosPodados;
    res.operaciones_o_estados = estadosExplorados + estadosPodados;
    if (res.operaciones_o_estados > 0) {
        res.tasa_poda_pct = (static_cast<double>(estadosPodados) /
                             static_cast<double>(res.operaciones_o_estados)) * 100.0;
    }

    ostringstream ossDet;
    ossDet << "Nodos visitados en arbol: " << estadosExplorados
           << " | Ramas podadas: " << estadosPodados
           << " (" << fixed << setprecision(1) << res.tasa_poda_pct << "% podado)";
    res.detalle_operaciones = ossDet.str();

    res.memoria_auxiliar_kb = memAuxiliarKB;
    res.memoria_proceso_kb = memDespues;
    res.memoria_kb = max(max(0.0, memDespues - memAntes), memAuxiliarKB);

    res.tipo_referencia = TipoReferencia::OPTIMO_EXACTO;
    res.tipo_referencia_str = "Optimo Exacto";
    res.desviacion_pct = 0.0;
    res.desviacion_valida = true;

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
        res.ruta_valida = true;
        return res;
    }

    int numClientes = n - 1;
    if (numClientes >= 31 || n > maxSafeN) {
        res.ejecutado = false;
        res.motivo_omision = "Omitido para N=" + to_string(n) +
                             " (supera el limite seguro N <= " + to_string(maxSafeN) +
                             " por consumo exponencial de memoria O(N * 2^N)).";
        return res;
    }

    ValidacionGrafo val = graph.validateGraphDetailed(false);
    if (!val.esValido) {
        res.ejecutado = false;
        res.motivo_omision = "Instancia invalida: " + (val.errores.empty() ? "Error estructural" : val.errores.front());
        return res;
    }

    double memRequeridaMB = estimarMemoriaHeldKarpKB(n) / 1024.0;
    if (memRequeridaMB > maxMemoriaMB) {
        ostringstream oss;
        oss << fixed << setprecision(1)
            << "Memoria requerida (" << memRequeridaMB << " MB) supera el presupuesto maximo ("
            << maxMemoriaMB << " MB).";
        res.ejecutado = false;
        res.motivo_omision = oss.str();
        return res;
    }

    size_t totalMasks = 1ULL << numClientes;
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
        res.motivo_omision = "Memoria fisica insuficiente (std::bad_alloc) al reservar tablas DP para N=" + to_string(n);
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
    res.detalle_operaciones = "Subconjuntos 2^(N-1): " + to_string(totalMasks) +
                              " | Transiciones DP: " + to_string(transiciones);

    res.memoria_auxiliar_kb = memAuxiliarKB;
    res.memoria_proceso_kb = memPico;
    res.memoria_kb = max(max(0.0, memPico - memAntes), memAuxiliarKB);

    res.tipo_referencia = TipoReferencia::OPTIMO_EXACTO;
    res.tipo_referencia_str = "Optimo Exacto";
    res.desviacion_pct = 0.0;
    res.desviacion_valida = true;

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
        res.tipo_solucion = TipoSolucion::APROXIMADA;
        res.tipo_solucion_str = "2-Aproximacion (Trivial N=1)";
        res.ruta_valida = true;
        return res;
    }

    ValidacionGrafo val = graph.validateGraphDetailed(true);
    if (!val.esValido) {
        res.ejecutado = false;
        res.motivo_omision = "Instancia invalida: " + (val.errores.empty() ? "Error estructural" : val.errores.front());
        return res;
    }

    if (val.cumpleDesigualdadTriangular) {
        res.tipo_solucion = TipoSolucion::APROXIMADA;
        res.tipo_solucion_str = "2-Aproximacion (Garantia Costo <= 2*OPT)";
    } else {
        res.tipo_solucion = TipoSolucion::HEURISTICA;
        res.tipo_solucion_str = "Heuristica (Sin cota 2*OPT: grafo no metrico)";
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

    vector<bool> visitado(static_cast<size_t>(n), false);
    res.ruta.reserve(static_cast<size_t>(n + 1));
    dfsPreorden(0, mstAdj, visitado, res.ruta, visitasDFS);
    res.ruta.push_back(0);

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

    if (val.cumpleDesigualdadTriangular && pesoTotalMST > 0.0) {
        res.tipo_referencia = TipoReferencia::COTA_INFERIOR;
        res.tipo_referencia_str = "Cota Inferior MST";
        res.diferencia_cota_inferior_pct = max(0.0, ((distanciaTotal - pesoTotalMST) / pesoTotalMST) * 100.0);
    }

    res.comparaciones_prim = comparacionesPrim;
    res.visitas_dfs = visitasDFS;
    res.operaciones_o_estados = comparacionesPrim + visitasDFS;
    res.detalle_operaciones = "Comparaciones Prim: " + to_string(comparacionesPrim) +
                              " | Recorrido arbol DFS: " + to_string(visitasDFS);

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
        res.ruta_valida = true;
        return res;
    }

    ValidacionGrafo val = graph.validateGraphDetailed(false);
    if (!val.esValido) {
        res.ejecutado = false;
        res.motivo_omision = "Instancia invalida: " + (val.errores.empty() ? "Error estructural" : val.errores.front());
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
    res.detalle_operaciones = "Comparaciones voraces de cercania: " + to_string(comparaciones);

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
    rec.diagnosticoInstancia = graph.validateGraphDetailed(true);
    rec.instanciaValida = rec.diagnosticoInstancia.esValido;
    rec.cumpleDesigualdadTriangular = rec.diagnosticoInstancia.cumpleDesigualdadTriangular;

    if (!rec.instanciaValida) {
        rec.categoriaEscala = "Instancia Rechazada en Validacion Previa";
        rec.algoritmoElegido = "Ninguno (Grafo Estructuralmente Invalido)";
        string errPrincipal = rec.diagnosticoInstancia.errores.empty()
                                  ? "Error desconocido"
                                  : rec.diagnosticoInstancia.errores.front();
        rec.justificacion = "La seleccion adaptativa aborto la ejecucion porque la instancia no aprobo "
                            "la validacion estructural: " + errPrincipal;
        rec.resultado.ejecutado = false;
        rec.resultado.motivo_omision = errPrincipal;
        return rec;
    }

    // Estimacion a priori de recursos para poblar la matriz de decision de los 4 candidatos
    double memBT_KB = static_cast<double>(n * 80) / 1024.0;
    double memHK_KB = estimarMemoriaHeldKarpKB(n);
    double memHK_MB = memHK_KB / 1024.0;
    double memMST_KB = static_cast<double>(n * 48) / 1024.0;
    double memNN_KB = static_cast<double>(n * 8) / 1024.0;

    bool btViable = (n <= config.maxSafeBacktracking);
    bool hkViable = (n <= config.maxSafeHeldKarp) && (memHK_MB <= config.memoriaMaximaMB);
    bool mstConGarantia = rec.cumpleDesigualdadTriangular;

    EvaluacionCandidato cBT{"Backtracking (B&B)", "O((N-1)!)", memBT_KB, "Optimo Exacto (0%)", btViable, false, ""};
    EvaluacionCandidato cHK{"Held-Karp (DP)", "O(N^2 * 2^N)", memHK_KB, "Optimo Exacto (0%)", hkViable, false, ""};
    EvaluacionCandidato cMST{"MST + DFS (Prim)", "O(N^2)", memMST_KB,
                             mstConGarantia ? "Cota <= 2*OPT" : "Sin cota (No metrico)", true, false, ""};
    EvaluacionCandidato cNN{"Nearest Neighbor", "O(N^2)", memNN_KB, "Heuristica Voraz", true, false, ""};

    cBT.razonDecision = btViable
                            ? "Viable en escala pequena con memoria O(N)."
                            : ("Descartado: N=" + to_string(n) + " supera limite seguro N<=" +
                               to_string(config.maxSafeBacktracking) + " (riesgo factorial).");

    if (n > config.maxSafeHeldKarp) {
        cHK.razonDecision = "Descartado: N=" + to_string(n) + " supera N<=" +
                            to_string(config.maxSafeHeldKarp) + " (explosion exponencial RAM).";
    } else if (!hkViable) {
        ostringstream oss;
        oss << fixed << setprecision(1) << "Descartado: requiere " << memHK_MB
            << " MB y el limite es " << config.memoriaMaximaMB << " MB.";
        cHK.razonDecision = oss.str();
    } else {
        cHK.razonDecision = "Viable: garantiza optimo global reutilizando subproblemas.";
    }

    cMST.razonDecision = mstConGarantia
                             ? "Viable: grafo metrico habilita garantia formal Costo <= 2*OPT."
                             : "Viable como heuristica, pero pierde cota 2*OPT al fallar desig. triangular.";
    cNN.razonDecision = "Viable: respuesta inmediata en microsegundos para cualquier escala.";

    // Reglas de decision segun prioridad y factibilidad real de recursos
    int indiceElegido = 3; // 0=BT, 1=HK, 2=MST, 3=NN

    if (config.criterio == CriterioPrioridad::MAXIMA_VELOCIDAD) {
        rec.categoriaEscala = "Prioridad de Despacho en Tiempo Real (N = " + to_string(n) + ")";
        indiceElegido = 3;
        rec.algoritmoElegido = "Nearest Neighbor (Heuristica Voraz)";
        rec.justificacion = "Se priorizo latencia minima para operacion en vivo. Nearest Neighbor responde en O(N^2) "
                            "con huella minima de memoria O(N).";
    } else if (config.criterio == CriterioPrioridad::GARANTIA_TEORICA ||
               (config.exigirGarantiaTeorica && config.criterio != CriterioPrioridad::OPTIMO_EXACTO)) {
        if (mstConGarantia) {
            rec.categoriaEscala = "Instancia con Garantia Matematica Formal (N = " + to_string(n) + ", TSP Metrico)";
            indiceElegido = 2;
            rec.algoritmoElegido = "MST + DFS (Prim - 2-Aproximacion)";
            rec.justificacion = "La instancia aprobo la Desigualdad Triangular. MST + DFS ejecuta en O(N^2) y "
                                "garantiza matematicamente que el recorrido nunca excede 2 veces el optimo.";
        } else {
            rec.categoriaEscala = "Instancia No Metrica (N = " + to_string(n) + " - Degradacion Controlada)";
            rec.huboDegradacionPorRecursos = true;
            indiceElegido = 3;
            rec.algoritmoElegido = "Nearest Neighbor (Alternativa por Fallo de Desigualdad Triangular)";
            rec.justificacion = "Se solicito cota formal 2*OPT, pero la matriz viola la Desigualdad Triangular. "
                                "Al no cumplirse el supuesto metrico, se selecciona Nearest Neighbor por menor costo empirico.";
        }
    } else if (config.criterio == CriterioPrioridad::OPTIMO_EXACTO) {
        if (btViable) {
            rec.categoriaEscala = "Instancia Pequena Exacta (N = " + to_string(n) + " <= " + to_string(config.maxSafeBacktracking) + ")";
            indiceElegido = 0;
            rec.algoritmoElegido = "Backtracking con Poda (Branch & Bound)";
            rec.justificacion = "Se exigio optimo exacto. Para N=" + to_string(n) +
                                ", Backtracking con cota inferior garantiza 0% de desviacion usando solo memoria O(N).";
        } else if (hkViable) {
            rec.categoriaEscala = "Instancia Mediana Exacta (N = " + to_string(n) + " <= " + to_string(config.maxSafeHeldKarp) + ")";
            indiceElegido = 1;
            rec.algoritmoElegido = "Held-Karp (Programacion Dinamica + Bitmask)";
            rec.justificacion = "Se exigio optimo exacto. Como N=" + to_string(n) + " supera el umbral de Backtracking, "
                                "Held-Karp halla el optimo global exacto dentro del presupuesto de RAM.";
        } else {
            rec.categoriaEscala = "Instancia Fuera de Rango Exacto (N = " + to_string(n) + " - Degradacion Segura)";
            rec.huboDegradacionPorRecursos = true;
            indiceElegido = 3;
            rec.algoritmoElegido = "Nearest Neighbor (Fallback Seguro por Limite de Tiempo/RAM)";
            rec.justificacion = "Se solicito optimo exacto, pero para N=" + to_string(n) +
                                " los metodos exactos exceden el presupuesto seguro de RAM (" +
                                to_string(static_cast<int>(config.memoriaMaximaMB)) +
                                " MB) o tiempo. Se degrada automaticamente a Nearest Neighbor para evitar bloqueo.";
        }
    } else {
        // Criterio BALANCEADO (por defecto): elige el mejor algoritmo viable segun N y recursos
        if (n <= 10 && btViable) {
            rec.categoriaEscala = "Escala Pequena (N = " + to_string(n) + " <= 10)";
            indiceElegido = 0;
            rec.algoritmoElegido = "Backtracking con Poda (Branch & Bound)";
            rec.justificacion = "En redes pequenas (N <= 10), Backtracking con poda resuelve el optimo exacto (0% error) "
                                "en fraccion de milisegundo sin reservar tablas exponenciales en RAM.";
        } else if (hkViable) {
            rec.categoriaEscala = "Escala Mediana (11 <= N = " + to_string(n) + " <= " + to_string(config.maxSafeHeldKarp) + ")";
            indiceElegido = 1;
            rec.algoritmoElegido = "Held-Karp (Programacion Dinamica + Bitmask)";
            rec.justificacion = "En escala mediana, Backtracking sufre explosion factorial, mientras Held-Karp "
                                "obtiene el optimo global exacto (0% error) dentro del limite de memoria configurado.";
        } else if (btViable) {
            rec.categoriaEscala = "Escala Mediana con Restriccion de RAM (N = " + to_string(n) + ")";
            rec.huboDegradacionPorRecursos = true;
            indiceElegido = 0;
            rec.algoritmoElegido = "Backtracking con Poda (Alternativa Exacta de Baja Memoria)";
            rec.justificacion = "Held-Karp excede el limite de RAM configurado, pero N=" + to_string(n) +
                                " aun permite obtener el optimo exacto con Backtracking en espacio lineal O(N).";
        } else {
            rec.categoriaEscala = "Escala Grande (N = " + to_string(n) + " > " + to_string(config.maxSafeHeldKarp) + ")";
            if (n > 10 && n <= config.maxSafeHeldKarp) {
                rec.huboDegradacionPorRecursos = true;
            }
            indiceElegido = 3;
            rec.algoritmoElegido = "Nearest Neighbor (Heuristica Voraz Escalable)";
            rec.justificacion = "Con N=" + to_string(n) + ", los algoritmos exactos son inviables en RAM/CPU. "
                                "Nearest Neighbor escala en O(N^2) y entrega rutas competitivas en menos de 1 ms.";
        }
    }

    // Marcar el candidato elegido en la matriz explicativa
    vector<EvaluacionCandidato> matriz = {cBT, cHK, cMST, cNN};
    matriz[indiceElegido].seleccionado = true;
    matriz[indiceElegido].razonDecision = "SELECCIONADO -> " + matriz[indiceElegido].razonDecision;
    for (int i = 0; i < 4; i++) {
        if (i != indiceElegido && matriz[i].viable) {
            if (indiceElegido == 0 || indiceElegido == 1) {
                matriz[i].razonDecision = "No elegido: es factible obtener el Optimo Exacto (0% error) en esta escala.";
            } else if (indiceElegido == 2 && i == 3) {
                matriz[i].razonDecision = "No elegido: Nearest Neighbor carece de cota matematica <= 2*OPT.";
            } else if (indiceElegido == 3 && i == 2) {
                matriz[i].razonDecision = "No elegido: suele producir mayor kilometraje empirico que Nearest Neighbor.";
            }
        }
    }
    rec.evaluacionCandidatos = move(matriz);

    // Ejecutar el algoritmo elegido y enriquecer sus metricas de referencia
    if (indiceElegido == 0) {
        rec.resultado = Backtracking(graph, config.maxSafeBacktracking);
    } else if (indiceElegido == 1) {
        rec.resultado = HeldKarp(graph, config.maxSafeHeldKarp, config.memoriaMaximaMB);
        // Fallback de emergencia si ocurriese std::bad_alloc real del sistema operativo
        if (!rec.resultado.ejecutado) {
            rec.huboDegradacionPorRecursos = true;
            rec.algoritmoElegido = "Nearest Neighbor (Fallback de Emergencia por RAM)";
            rec.resultado = NearestNeighbor(graph, 0);
        }
    } else if (indiceElegido == 2) {
        rec.resultado = MST_DFS(graph);
    } else {
        rec.resultado = NearestNeighbor(graph, 0);
    }

    // Si se ejecuto una heuristica en un grafo metrico sin optimo exacto, calcular la cota inferior MST
    if (rec.resultado.ejecutado && rec.resultado.tipo_solucion != TipoSolucion::EXACTA && rec.cumpleDesigualdadTriangular) {
        double cotaMST = (rec.resultado.cota_inferior_mst_km > 0.0)
                             ? rec.resultado.cota_inferior_mst_km
                             : calcularCotaInferiorMST(graph);
        if (cotaMST > 0.0) {
            rec.resultado.cota_inferior_mst_km = cotaMST;
            rec.resultado.diferencia_cota_inferior_pct =
                max(0.0, ((rec.resultado.distancia_km - cotaMST) / cotaMST) * 100.0);
            if (rec.resultado.tipo_referencia == TipoReferencia::SIN_REFERENCIA) {
                rec.resultado.tipo_referencia = TipoReferencia::COTA_INFERIOR;
                rec.resultado.tipo_referencia_str = "Cota Inferior MST";
            }
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
