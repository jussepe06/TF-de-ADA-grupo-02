#ifndef ALGORITHMS_H
#define ALGORITHMS_H

#include "Graph.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

using namespace std;

// Clasificacion formal del tipo de solucion generada (RF08)
enum class TipoSolucion {
    EXACTA,      // Solucion con garantia formal de optimo global (Backtracking, Held-Karp)
    APROXIMADA,  // Solucion polinomial con cota matematica garantizada (MST + DFS: Costo <= 2 * OPT)
    HEURISTICA   // Solucion aproximada sin cota formal garantizada (Nearest Neighbor o MST no metrico)
};

// Clasificacion rigurosa de la referencia empleada para la desviacion (RF08, RF09)
enum class TipoReferencia {
    OPTIMO_EXACTO,    // Desviacion porcentual respecto al optimo matematico comprobado
    MEJOR_OBSERVADO,  // Diferencia porcentual respecto a la mejor heuristica ejecutada (NO es error sobre optimo)
    COTA_INFERIOR,    // Diferencia respecto a una cota teorica inferior (ej. peso del MST)
    SIN_REFERENCIA    // Sin base valida de comparacion (N/A)
};

// Estructura completa de metricas de instrumentacion y auditoria (RF07, RF13)
struct TSPResult {
    string algoritmo;
    TipoSolucion tipo_solucion = TipoSolucion::HEURISTICA;
    string tipo_solucion_str = "Heuristica";

    double distancia_km = 0.0;
    double costo_usd = 0.0;

    // Referencia y desviacion porcentual
    TipoReferencia tipo_referencia = TipoReferencia::SIN_REFERENCIA;
    string tipo_referencia_str = "Sin Referencia";
    double desviacion_pct = -1.0;
    bool desviacion_valida = false;
    double diferencia_mejor_observada_pct = 0.0;
    double diferencia_cota_inferior_pct = 0.0;
    double cota_inferior_mst_km = 0.0;

    // Tiempos medidos con std::chrono::steady_clock
    double tiempo_us = 0.0;
    double tiempo_ms = 0.0;

    // Metricas de memoria
    double memoria_auxiliar_kb = 0.0;
    double memoria_proceso_kb = 0.0;
    double memoria_kb = 0.0;

    // Contadores especificos por estrategia
    uint64_t operaciones_o_estados = 0;
    uint64_t estados_explorados = 0;   // Backtracking
    uint64_t estados_podados = 0;      // Backtracking
    uint64_t transiciones_dp = 0;      // Held-Karp
    uint64_t comparaciones_prim = 0;   // MST Prim
    uint64_t visitas_dfs = 0;          // Recorrido DFS
    uint64_t comparaciones_nn = 0;     // Nearest Neighbor
    string detalle_operaciones;

    // Ruta y validacion
    vector<int> ruta;
    bool ruta_valida = true;
    string error_validacion_ruta;

    // Control de ejecucion
    bool ejecutado = true;
    string motivo_omision;
};

// Criterio prioritario del usuario para el Recomendador Adaptativo
enum class CriterioPrioridad {
    BALANCEADO,         // Seleccion automatica optima segun tamano y recursos
    OPTIMO_EXACTO,      // Exigir optimo global absoluto si es viable
    GARANTIA_TEORICA,   // Exigir garantia matematica formal (Costo <= 2*OPT)
    MAXIMA_VELOCIDAD    // Priorizar tiempo de respuesta minimo para despacho en tiempo real
};

// Estructura de configuracion para el motor de adaptabilidad (RF11)
struct ConfiguracionAdaptativa {
    double memoriaMaximaMB = 512.0;
    double tiempoMaximoSegundos = 30.0;
    int maxSafeBacktracking = 12;
    int maxSafeHeldKarp = 20;
    CriterioPrioridad criterio = CriterioPrioridad::BALANCEADO;
    bool exigirGarantiaTeorica = false;
};

// Resultado enriquecido para el Recomendador Adaptativo (RF11)
struct RecomendacionAdaptativa {
    string algoritmoElegido;
    string categoriaEscala;
    bool cumpleDesigualdadTriangular = false;
    string justificacion;
    TSPResult resultado;
    ConfiguracionAdaptativa configUtilizada;
};

// Medidor de tiempo de alta resolucion y memoria del proceso (RF07, RF13)
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

    // Consulta la memoria del proceso actual en Windows/Linux (KB)
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
                             uint64_t& estadosExplorados,
                             uint64_t& estadosPodados);

    static void dfsPreorden(int u,
                            const vector<vector<int>>& mstAdj,
                            vector<bool>& visitado,
                            vector<int>& recorrido,
                            uint64_t& operaciones);

public:
    static constexpr int kMaxSafeBacktracking = 12;
    static constexpr int kMaxSafeHeldKarp = 20;

    // Funcion comun de validacion rigurosa de ciclo TSP (Seccion 4.5)
    static bool validarRuta(const Graph& graph,
                            const vector<int>& ruta,
                            double costoReportadoKm,
                            string& mensajeError);

    // Calculo y clasificacion rigurosa de desviacion porcentual (Seccion 5)
    static void clasificarYCalcularDesviaciones(vector<TSPResult>& resultados,
                                               double cotaInferiorConocida = 0.0);

    // Algoritmos principales
    static TSPResult Backtracking(const Graph& graph, int maxSafeN = kMaxSafeBacktracking);
    static TSPResult HeldKarp(const Graph& graph,
                              int maxSafeN = kMaxSafeHeldKarp,
                              double maxMemoriaMB = 512.0);
    static TSPResult MST_DFS(const Graph& graph);
    static TSPResult NearestNeighbor(const Graph& graph, int startNode = 0);

    // Seleccion adaptativa automatica con configuracion de recursos (RF11)
    static RecomendacionAdaptativa SeleccionAdaptativa(const Graph& graph,
                                                       const ConfiguracionAdaptativa& config);

    // Sobrecarga de conveniencia compatible con versiones anteriores
    static RecomendacionAdaptativa SeleccionAdaptativa(const Graph& graph,
                                                       bool exigirGarantiaTeorica = false);
};

#endif
