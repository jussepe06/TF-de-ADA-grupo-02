#include "test_routeopt.h"
#include "../Graph.h"
#include "../Algorithms.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace {
int pruebasEjecutadas = 0;
int pruebasPasadas = 0;
int pruebasFalladas = 0;

void registrarPrueba(bool condicion, const string& nombre) {
    pruebasEjecutadas++;
    if (condicion) {
        pruebasPasadas++;
        cout << "  [PASS] " << nombre << "\n";
    } else {
        pruebasFalladas++;
        cout << "  [FAIL] " << nombre << "\n";
    }
}
}

int ejecutarSuitePruebas() {
    pruebasEjecutadas = 0;
    pruebasPasadas = 0;
    pruebasFalladas = 0;

    cout << "\n========================================================================\n";
    cout << "     SUITE DE PRUEBAS AUTOMATIZADAS - ROUTEOPT (C++17)\n";
    cout << "========================================================================\n";

    // -------------------------------------------------------------------------
    // CATEGORIA A: PRUEBAS DEL MODELO DE GRAFO Y VALIDACION ESTRUCTURAL
    // -------------------------------------------------------------------------
    cout << "\n--- CATEGORIA A: Pruebas del Grafo y Validacion Estructural ---\n";

    // A.1 Grafo de 1 solo nodo (debe ser rechazado por ser < 2 nodos para TSP)
    {
        Graph g1(1, "Grafo 1 Nodo");
        ValidacionGrafo v = g1.validateGraphDetailed(false);
        registrarPrueba(!v.esValido && !v.dimensionesValidas, "A.1: Grafo con 1 nodo es detectado como invalido");
    }

    // A.2 Grafo de 2 nodos (minimo valido)
    {
        Graph g2(2, "Grafo 2 Nodos");
        g2.addEdge(0, 1, 15.0);
        ValidacionGrafo v = g2.validateGraphDetailed(true);
        registrarPrueba(v.esValido && v.simetrico && v.diagonalCero, "A.2: Grafo de 2 nodos valido y simetrico");
    }

    // A.3 Coordenadas coincidentes (distancia 0 entre nodos distintos no debe colapsar)
    {
        Graph gCoinc(3, "Coincidentes");
        gCoinc.addNode("Almacen", 10.0, 10.0);
        gCoinc.addNode("Cliente Coincidente", 10.0, 10.0); // Misma coord que nodo 0
        gCoinc.addNode("Cliente Lejano", 10.0, 20.0);
        ValidacionGrafo v = gCoinc.validateGraphDetailed(false);
        registrarPrueba(v.esValido && gCoinc.getCost(0, 1) == 0.0,
                        "A.3: Coordenadas coincidentes permiten distancia 0.0 sin invalidar grafo");
    }

    // A.4 Matriz asimetrica detectada
    {
        Graph gAsim(3, "Asimetrico");
        // Forzar asimetria manual
        gAsim.addEdge(0, 1, 10.0);
        gAsim.addEdge(0, 2, 20.0);
        gAsim.addEdge(1, 2, 30.0);
        // Modificar solo una direccion
        const_cast<vector<vector<double>>&>(gAsim.getMatrix())[0][1] = 99.0;
        ValidacionGrafo v = gAsim.validateGraphDetailed(false);
        registrarPrueba(!v.esValido && !v.simetrico, "A.4: Deteccion de asimetria en la matriz de adyacencia");
    }

    // A.5 Distancia negativa detectada
    {
        Graph gNeg(2, "Distancia Negativa");
        bool lanzoExcepcion = false;
        try {
            gNeg.addEdge(0, 1, -5.0);
        } catch (const invalid_argument&) {
            lanzoExcepcion = true;
        }
        registrarPrueba(lanzoExcepcion, "A.5: addEdge rechaza distancias negativas con excepcion");
    }

    // A.6 Valores NaN detectados
    {
        Graph gNaN(2, "Valor NaN");
        bool lanzoExcepcion = false;
        try {
            gNaN.addEdge(0, 1, numeric_limits<double>::quiet_NaN());
        } catch (const invalid_argument&) {
            lanzoExcepcion = true;
        }
        registrarPrueba(lanzoExcepcion, "A.6: addEdge rechaza valores NaN con excepcion");
    }

    // A.7 Valores infinitos detectados
    {
        Graph gInf(2, "Valor Infinito");
        bool lanzoExcepcion = false;
        try {
            gInf.addEdge(0, 1, numeric_limits<double>::infinity());
        } catch (const invalid_argument&) {
            lanzoExcepcion = true;
        }
        registrarPrueba(lanzoExcepcion, "A.7: addEdge rechaza valores infinitos con excepcion");
    }

    // A.8 Identificadores duplicados en coordenadas
    {
        Graph gDup(3, "IDs Duplicados");
        gDup.addNode("N0", 0, 0);
        gDup.addNode("N1", 1, 1);
        gDup.addNode("N2", 2, 2);
        // Forzar id duplicado
        const_cast<vector<Punto>&>(gDup.getCoordinates())[2].id = 1;
        ValidacionGrafo v = gDup.validateGraphDetailed(false);
        registrarPrueba(!v.esValido && !v.idsValidos, "A.8: Deteccion de identificadores duplicados");
    }

    // A.9 Coordenadas geograficas fuera de rango
    {
        Graph gGeo(2, "Geo Invalida");
        gGeo.setGeographic(true);
        gGeo.addNode("Almacen", 95.0, 50.0); // Latitud 95 invalida (> 90)
        gGeo.addNode("Cliente", -12.0, -77.0);
        ValidacionGrafo v = gGeo.validateGraphDetailed(false);
        registrarPrueba(!v.esValido && !v.coordsValidas, "A.9: Deteccion de latitud fuera de rango [-90, 90]");
    }

    // A.10 Modificacion dinamica de nodos y reconstruccion de matriz
    {
        Graph gDyn(0, "Dinamico");
        gDyn.addNode("Almacen", 0, 0);
        gDyn.addNode("C1", 0, 10);
        gDyn.addNode("C2", 0, 20);
        double distPre = gDyn.getCost(0, 1);
        gDyn.updateNode(1, "C1_Movido", 0, 25);
        double distPost = gDyn.getCost(0, 1);
        gDyn.removeNode(2); // Eliminar C2 (queda con 2 nodos: Almacen y C1)
        registrarPrueba(abs(distPre - 10.0) < 1e-4 && abs(distPost - 25.0) < 1e-4 && gDyn.getNumNodes() == 2,
                        "A.10: Gestion dinamica (agregar, modificar, eliminar) actualiza la matriz");
    }

    // -------------------------------------------------------------------------
    // CATEGORIA B: PRUEBAS DE ALGORITMOS Y VALIDACION DE RUTAS
    // -------------------------------------------------------------------------
    cout << "\n--- CATEGORIA B: Pruebas de Algoritmos y Optimos Conocidos ---\n";

    // Grafo cuadrado euclidiano simetrico de 4 nodos:
    // (0,0), (0,10), (10,10), (10,0) -> Optimo perimetro = 10 + 10 + 10 + 10 = 40.0 km
    Graph gCuadrado(4, "Cuadrado 4 Nodos");
    gCuadrado.addNode("P0", 0, 0);
    gCuadrado.addNode("P1", 0, 10);
    gCuadrado.addNode("P2", 10, 10);
    gCuadrado.addNode("P3", 10, 0);

    // B.1 Backtracking halla el optimo conocido (40 km)
    TSPResult btRes = Algorithms::Backtracking(gCuadrado);
    registrarPrueba(abs(btRes.distancia_km - 40.0) < 1e-4 && btRes.ruta_valida,
                    "B.1: Backtracking halla el optimo exacto conocido (40.0 km)");

    // B.2 Held-Karp halla el optimo conocido (40 km)
    TSPResult hkRes = Algorithms::HeldKarp(gCuadrado);
    registrarPrueba(abs(hkRes.distancia_km - 40.0) < 1e-4 && hkRes.ruta_valida,
                    "B.2: Held-Karp halla el optimo exacto conocido (40.0 km)");

    // B.3 Comparacion Backtracking vs Held-Karp: coincidencia exacta de costos
    registrarPrueba(abs(btRes.distancia_km - hkRes.distancia_km) < 1e-6,
                    "B.3: Coincidencia matematica exacta entre Backtracking y Held-Karp");

    // B.4 MST + DFS en grafo metrico euclidiano (Costo <= 2 * OPT = 80.0 km)
    TSPResult mstMetricRes = Algorithms::MST_DFS(gCuadrado);
    registrarPrueba(mstMetricRes.ruta_valida && mstMetricRes.tipo_solucion == TipoSolucion::APROXIMADA &&
                    mstMetricRes.distancia_km <= 2.0 * btRes.distancia_km + 1e-4,
                    "B.4: MST + DFS en grafo metrico clasificado como 2-Aproximacion y cumple cota <= 2*OPT");

    // B.5 MST + DFS en grafo NO metrico (falla desigualdad triangular)
    {
        // Construir grafo donde (0,1)=100, pero (0,2)=1 y (2,1)=1
        Graph gNoMetrico(3, "No Metrico");
        gNoMetrico.addEdge(0, 1, 100.0);
        gNoMetrico.addEdge(0, 2, 1.0);
        gNoMetrico.addEdge(1, 2, 1.0);
        TSPResult mstNoMetRes = Algorithms::MST_DFS(gNoMetrico);
        registrarPrueba(mstNoMetRes.tipo_solucion == TipoSolucion::HEURISTICA,
                        "B.5: MST + DFS en grafo no metrico se degrada correctamente a Heuristica sin garantia");
    }

    // B.6 Nearest Neighbor sobre instancia pequena y grande
    TSPResult nnRes = Algorithms::NearestNeighbor(gCuadrado, 0);
    registrarPrueba(nnRes.ruta_valida && nnRes.distancia_km >= 40.0 - 1e-4,
                    "B.6: Nearest Neighbor genera ruta valida y cota valida");

    // B.7 Verificacion de retorno al deposito y clientes no repetidos
    {
        string errorVal;
        vector<int> rutaRepetida = {0, 1, 2, 1, 3, 0}; // Cliente 1 repetido
        bool esValida = Algorithms::validarRuta(gCuadrado, rutaRepetida, 40.0, errorVal);
        registrarPrueba(!esValida, "B.7: Funcion comun validarRuta rechaza clientes repetidos");
    }

    // B.8 Verificacion de retorno obligatorio al nodo 0
    {
        string errorVal;
        vector<int> rutaSinRetorno = {0, 1, 2, 3, 2}; // Termina en 2 en vez de 0
        bool esValida = Algorithms::validarRuta(gCuadrado, rutaSinRetorno, 40.0, errorVal);
        registrarPrueba(!esValida, "B.8: Funcion comun validarRuta rechaza ruta que no termina en el Almacen 0");
    }

    // -------------------------------------------------------------------------
    // CATEGORIA C: PRUEBAS DE METRICAS Y DESVIACION PORCENTUAL
    // -------------------------------------------------------------------------
    cout << "\n--- CATEGORIA C: Pruebas de Metricas y Calculo de Desviacion ---\n";

    // C.1 Desviacion calculada sobre optimo exacto conocido
    {
        vector<TSPResult> lista = {btRes, hkRes, nnRes, mstMetricRes};
        Algorithms::clasificarYCalcularDesviaciones(lista);
        registrarPrueba(lista[0].desviacion_pct == 0.0 && lista[0].desviacion_valida &&
                        lista[1].desviacion_pct == 0.0 && lista[1].desviacion_valida &&
                        lista[0].tipo_referencia == TipoReferencia::OPTIMO_EXACTO,
                        "C.1: Algoritmos exactos obtienen 0.0% de error sobre Optimo Exacto");
    }

    // C.2 Ausencia de error 0% en heuristicas cuando NO se conoce el optimo exacto
    {
        vector<TSPResult> soloHeuristicas = {nnRes, mstMetricRes};
        // Forzar tipo heuristica para simular escenario grande N=50
        soloHeuristicas[0].tipo_solucion = TipoSolucion::HEURISTICA;
        soloHeuristicas[1].tipo_solucion = TipoSolucion::HEURISTICA;
        Algorithms::clasificarYCalcularDesviaciones(soloHeuristicas);
        registrarPrueba(!soloHeuristicas[0].desviacion_valida &&
                        soloHeuristicas[0].tipo_referencia == TipoReferencia::MEJOR_OBSERVADO &&
                        soloHeuristicas[0].desviacion_pct < 0.0,
                        "C.2: NO se asigna 0% de error a heuristica cuando solo hay Mejor Solucion Observada");
    }

    // C.3 Contadores especificos e independientes por estrategia
    registrarPrueba(btRes.estados_explorados > 0 && btRes.estados_podados >= 0 &&
                    hkRes.transiciones_dp > 0 && mstMetricRes.comparaciones_prim > 0 &&
                    nnRes.comparaciones_nn > 0,
                    "C.3: Contadores especificos registrados (explorados, transiciones, comparaciones)");

    // C.4 Medicion de tiempo con std::chrono::steady_clock
    registrarPrueba(btRes.tiempo_us > 0.0 && hkRes.tiempo_us > 0.0 &&
                    nnRes.tiempo_us >= 0.0 && mstMetricRes.tiempo_us >= 0.0,
                    "C.4: Medicion de tiempo consistente con steady_clock (> 0 microsegundos)");

    // C.5 Cota inferior del MST (W(MST) <= OPT)
    registrarPrueba(mstMetricRes.cota_inferior_mst_km > 0.0 &&
                    mstMetricRes.cota_inferior_mst_km <= btRes.distancia_km + 1e-4,
                    "C.5: Cota inferior teorica del MST verificada matematicamente: W(MST) <= OPT");

    // -------------------------------------------------------------------------
    // CATEGORIA D: PRUEBAS DE IMPORTACION Y EXPORTACION
    // -------------------------------------------------------------------------
    cout << "\n--- CATEGORIA D: Pruebas de Importacion y Exportacion ---\n";

    // D.1 Carga desde CSV en memoria (fromCSVStream)
    {
        string csvData =
            "id,nombre,lat_o_x,lon_o_y\n"
            "0,Deposito Central,0.0,0.0\n"
            "1,Cliente A,0.0,10.0\n"
            "2,Cliente B,10.0,10.0\n";
        stringstream ss(csvData);
        Graph gCSV = Graph::fromCSVStream(ss, "Prueba CSV");
        registrarPrueba(gCSV.getNumNodes() == 3 && abs(gCSV.getCost(0, 1) - 10.0) < 1e-4,
                        "D.1: Carga estructurada de ubicaciones CSV desde stream");
    }

    // D.2 Carga de matriz de distancias directamente desde CSV
    {
        string matCSV =
            "0.0, 15.5, 30.2\n"
            "15.5, 0.0, 12.0\n"
            "30.2, 12.0, 0.0\n";
        stringstream ss(matCSV);
        Graph gMat = Graph::fromDistanceMatrixCSV(ss, "Matriz Externa");
        registrarPrueba(gMat.getNumNodes() == 3 && abs(gMat.getCost(0, 1) - 15.5) < 1e-4 &&
                        abs(gMat.getCost(1, 2) - 12.0) < 1e-4,
                        "D.2: Carga de matriz de distancias externa NxN desde CSV");
    }

    // D.3 Manejo robusto de archivo CSV con campos corruptos o incompletos
    {
        string csvCorrupto =
            "id,nombre,lat_o_x,lon_o_y\n"
            "0,Almacen,0.0,0.0\n"
            "invalido,Cliente,texto_no_numero\n"; // fila corrupta
        stringstream ss(csvCorrupto);
        bool detectoError = false;
        try {
            Graph gErr = Graph::fromCSVStream(ss, "Corrupto");
        } catch (...) {
            detectoError = true;
        }
        registrarPrueba(detectoError, "D.3: Deteccion y excepcion controlada ante CSV con formato corrupto");
    }

    // -------------------------------------------------------------------------
    // CATEGORIA E: BENCHMARKING REPRODUCIBLE Y PROTECCION DE MEMORIA
    // -------------------------------------------------------------------------
    cout << "\n--- CATEGORIA E: Benchmarking Reproducible y Proteccion de Memoria ---\n";

    // E.1 Reproducibilidad determinista con std::mt19937 y semilla identica
    {
        Graph gSeed1 = Graph::generateRandomInstance(15, 12345, 100.0);
        Graph gSeed2 = Graph::generateRandomInstance(15, 12345, 100.0);
        bool identicas = true;
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 15; j++) {
                if (abs(gSeed1.getCost(i, j) - gSeed2.getCost(i, j)) > 1e-9) {
                    identicas = false;
                    break;
                }
            }
        }
        registrarPrueba(identicas, "E.1: Generador determinista produce matrices identicas con misma semilla");
    }

    // E.2 Proteccion de memoria en Held-Karp para instancias grandes (N = 35)
    {
        Graph gGrande = Graph::generateRandomInstance(35, 42, 100.0);
        TSPResult hkGrande = Algorithms::HeldKarp(gGrande, 20, 512.0);
        registrarPrueba(!hkGrande.ejecutado && !hkGrande.motivo_omision.empty(),
                        "E.2: Held-Karp omite con seguridad N=35 sin provocar std::bad_alloc ni saturar RAM");
    }

    // E.3 Recomendador adaptativo inteligente con restricciones
    {
        Graph gPiloto = Graph::createPilotInstance(10);
        ConfiguracionAdaptativa cfg;
        cfg.criterio = CriterioPrioridad::OPTIMO_EXACTO;
        RecomendacionAdaptativa rec = Algorithms::SeleccionAdaptativa(gPiloto, cfg);
        registrarPrueba(rec.resultado.ejecutado && rec.algoritmoElegido.find("Backtracking") != string::npos,
                        "E.3: Recomendador Adaptativo selecciona Backtracking para N=10 con Criterio Optimo");
    }

    // -------------------------------------------------------------------------
    // RESUMEN FINAL DE LA SUITE
    // -------------------------------------------------------------------------
    cout << "\n========================================================================\n";
    cout << "  RESUMEN DE PRUEBAS AUTOMATIZADAS:\n";
    cout << "  Total ejecutadas : " << pruebasEjecutadas << "\n";
    cout << "  Superadas [PASS] : " << pruebasPasadas << "\n";
    cout << "  Fallidas  [FAIL] : " << pruebasFalladas << "\n";
    cout << "  Tasa de exito    : " << fixed << setprecision(1)
         << (static_cast<double>(pruebasPasadas) / pruebasEjecutadas) * 100.0 << " %\n";
    cout << "========================================================================\n";

    return (pruebasFalladas == 0) ? 0 : pruebasFalladas;
}
